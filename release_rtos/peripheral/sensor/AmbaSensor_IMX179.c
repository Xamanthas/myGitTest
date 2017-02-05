/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_IMX179.c
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of SONY IMX179 CMOS sensor with MIPI interface
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaKAL.h"

#include "AmbaDSP_VIN.h"
#include "AmbaVIN.h"

#include "AmbaI2C.h"
#include "AmbaGPIO.h"
#include "AmbaPLL.h"
#include "AmbaSensor.h"
#include "AmbaSensor_IMX179.h"

#include "AmbaPrintk.h"

/*-----------------------------------------------------------------------------------------------*\
 * IMX179 sensor runtime info
\*-----------------------------------------------------------------------------------------------*/
IMX179_CTRL_s IMX179Ctrl = {0};

/*-----------------------------------------------------------------------------------------------*\
 * Configurations for Amba. VIN side
\*-----------------------------------------------------------------------------------------------*/
static AMBA_DSP_VIN_MIPI_CONFIG_s IMX179VinConfig = {
    .Info = {
        .FrameRate = {
            .Interlace      = 0,
            .TimeScale      = 0,
            .NumUnitsInTick = 0,
        },
        .DspPhaseShift      = {
            .Horizontal     =   AMBA_DSP_PHASE_SHIFT_MODE_0,
            .Vertical       =   AMBA_DSP_PHASE_SHIFT_MODE_0,
        },
        .BayerPattern       = AMBA_DSP_BAYER_RG,
        .NumDataBits        = 0,
        .NumSkipFrame       = 1,
    },
    .DataTypeMask           = 0x00,
    .DataType               = AMBA_DSP_VIN_MIPI_RAW10,
    .NumActiveLanes         = 4,
    .RxHvSyncCtrl           = {
        .NumActivePixels    = 0,
        .NumActiveLines     = 0,
        .NumTotalPixels     = 0,
        .NumTotalLines      = 0,
    },

    .VinVoutSync            = {
        {
            .SignalFreq = AMBA_DSP_VIN_VOUT_SYNC_FRAME,
            .SignalLine = 1,
        },
        {
            .SignalFreq = AMBA_DSP_VIN_VOUT_SYNC_FRAME,
            .SignalLine = 1,
        },
    },
    .MipiCtrl               = {
        .HsSettleTime   = 0,
        .HsTermTime     = 0,
        .ClkSettleTime  = 0,
        .ClkTermTime    = 0,
        .ClkMissTime    = 0,
        .RxInitTime     = 0,
    }
};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_PrepareModeInfo
 *
 *  @Description:: Get sensor mode information
 *
 *  @Input      ::
 *      Mode:   Sensor readout mode number
 *
 *  @Output     ::
 *      pModeInfo:  Details of the specified readout mode
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX179_PrepareModeInfo(AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo)
{
    UINT16 SensorMode                             = Mode.Bits.Mode;
    IMX179_READOUT_MODE_e           ReadoutMode   = IMX179ModeInfoList[SensorMode].ReadoutMode;
    const IMX179_FRAME_TIMING_s     *pFrameTiming = &IMX179ModeInfoList[SensorMode].FrameTiming;
    const AMBA_SENSOR_OUTPUT_INFO_s *pOutputInfo  = &IMX179OutputInfo[ReadoutMode];
    const AMBA_SENSOR_INPUT_INFO_s  *pInputInfo;

    if (Mode.Bits.VerticalFlip == 0)
        pInputInfo = &IMX179InputInfoNormalReadout[ReadoutMode];
    else if (Mode.Bits.VerticalFlip == 1)
        pInputInfo = &IMX179InputInfoInversionReadout[ReadoutMode];

    pModeInfo->Mode = Mode;
    pModeInfo->LineLengthPck = pFrameTiming->LineLengthPck;
    pModeInfo->FrameLengthLines = pFrameTiming->FrameLengthLines;
    pModeInfo->NumExposureStepPerFrame = pFrameTiming->FrameLengthLines;
    pModeInfo->PixelRate = (float) pOutputInfo->DataRate * pOutputInfo->NumDataLanes / pOutputInfo->NumDataBits;
    pModeInfo->RowTime = (float) pModeInfo->LineLengthPck / pModeInfo->PixelRate;
    pModeInfo->FrameTime.InputClk = pFrameTiming->InputClk;
    memcpy(&pModeInfo->FrameTime.FrameRate, &pFrameTiming->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    memcpy(&pModeInfo->InputInfo, pInputInfo, sizeof(AMBA_SENSOR_INPUT_INFO_s));
    memcpy(&pModeInfo->OutputInfo, pOutputInfo, sizeof(AMBA_SENSOR_OUTPUT_INFO_s));
    memcpy(&pModeInfo->MinFrameRate, &pFrameTiming->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));

    /* HDR information */
    memset(&pModeInfo->HdrInfo, 0, sizeof(AMBA_SENSOR_HDR_INFO_s)); // Not support HDR

    /* Only support 3 stage slow shutter */
    pModeInfo->MinFrameRate.TimeScale /= 8;

}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_ConfigVin
 *
 *  @Description:: Configure VIN to receieve output frames of the new readout mode
 *
 *  @Input      ::
 *      pModeInfo:  Details of the specified readout mode
 *      pFrameTime: Sensor frame time configuration
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX179_ConfigVin(AMBA_SENSOR_MODE_INFO_s *pModeInfo, IMX179_FRAME_TIMING_s *pFrameTime)
{
    AMBA_SENSOR_OUTPUT_INFO_s  *pImgInfo = &pModeInfo->OutputInfo;
    AMBA_DSP_VIN_MIPI_CONFIG_s *pVinCfg = &IMX179VinConfig;

    pVinCfg->NumActiveLanes = pImgInfo->NumDataLanes;
    pVinCfg->Info.NumDataBits = pImgInfo->NumDataBits;

    pVinCfg->RxHvSyncCtrl.NumActivePixels  = pImgInfo->OutputWidth;
    pVinCfg->RxHvSyncCtrl.NumActiveLines   = pImgInfo->OutputHeight;
    pVinCfg->RxHvSyncCtrl.NumTotalPixels   = pModeInfo->LineLengthPck - 1;
    pVinCfg->RxHvSyncCtrl.NumTotalLines    = pModeInfo->FrameLengthLines;

    pVinCfg->Info.BayerPattern = (AMBA_DSP_SENSOR_PATTERN_e) pImgInfo->CfaPattern;

    memcpy(&pVinCfg->Info.FrameRate, &pFrameTime->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    memcpy(&pVinCfg->Info.DspPhaseShift, &pImgInfo->DspPhaseShift, sizeof(AMBA_DSP_PHASE_SHIFT_CTRL_s));

    AmbaDSP_VinConfigMIPI(AMBA_VIN_CHANNEL0, pVinCfg);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_HardwareReset
 *
 *  @Description:: Reset IMX179 Image Sensor Device
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static inline void IMX179_HardwareReset(void)
{
    AmbaUserGPIO_SensorResetCtrl(0);
    AmbaKAL_TaskSleep(1);               /* XCLR Low level pulse width >= 100ns */
    AmbaUserGPIO_SensorResetCtrl(1);
}

static int IMX179_RegRead(UINT16 Addr, UINT8 *pRxData, int Size)
{
    int i, RetStatus;
    UINT16 TxData[4];
    UINT8  RxData[128];

    TxData[0] = AMBA_I2C_RESTART_FLAG | (0x20);
    TxData[1] = (Addr >> 8);    /* Register Address [15:8] */
    TxData[2] = (Addr & 0xff);  /* Register Address [7:0]  */
    TxData[3] = AMBA_I2C_RESTART_FLAG | (0x21);

    RetStatus = AmbaI2C_ReadAfterWrite(AMBA_I2C_CHANNEL0,
                                       AMBA_I2C_SPEED_FAST,
                                       4, (UINT16 *) TxData,
                                       Size, RxData,
                                       AMBA_KAL_WAIT_FOREVER);
    for (i = 0; i < Size; i++)
        *pRxData++ = RxData[i];

    return RetStatus;
}

static int IMX179_RegWrite(UINT16 Addr, UINT8 *pTxData, int Size)
{
    int i;
    UINT8 TxData[128];

    TxData[0] = (UINT8) ((Addr & 0xff00) >> 8);
    TxData[1] = (UINT8) (Addr & 0x00ff);

    for (i = 0; i < Size; i++)
        TxData[i + 2] = pTxData[i];

    if (AmbaI2C_Write(AMBA_I2C_CHANNEL0,
                      AMBA_I2C_SPEED_FAST,
                      0x20, Size + 2, TxData,
                      AMBA_KAL_WAIT_FOREVER) != OK)
        AmbaPrint("I2C does not work!!!!!");

#if 0 // for debugging
    {
        UINT8 RxData;

        for (i = 0; i < Size; i++, Addr++) {
            IMX179_RegRead(Addr, &RxData, 1);
            AmbaPrint("[IMX179] Addr=0x%04X, Data=0x%02X, Rd=0x%02X",
                      Addr, pTxData[i], RxData);
        }
    }
#endif

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_RegisterRead
 *
 *  @Description:: Read sensor registers through I2C bus
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *      Addr:   Register address
 *
 *  @Output     ::
 *      pData:  Pointer to Read data buffer
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_RegisterRead(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 *pData)
{
    UINT8 RxData;

    IMX179_RegRead(Addr, &RxData, 1);
    *pData = RxData;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_RegisterWrite
 *
 *  @Description:: Write sensor registers through I2C bus
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *      Addr:   Register address
 *      Data:   Register value
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_RegisterWrite(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 Data)
{
    UINT8 TxData = (UINT8) Data;

    IMX179_RegWrite(Addr, &TxData, 1);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetAnalogGainReg
 *
 *  @Description:: Configure sensor analog gain setting
 *
 *  @Input      ::
 *      PGC:    Analog gain setting
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX179_SetAnalogGainReg(UINT8 PGC)
{
    UINT8 TxData[1];

    TxData[0] = PGC & 0xff;

    IMX179_RegWrite(IMX179_ANA_GAIN_GLOBAL_REG, TxData, 1);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetDigitalGainReg
 *
 *  @Description:: Configure sensor digital gain setting
 *
 *  @Input      ::
 *           DGC:    Digital gain setting
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX179_SetDigitalGainReg(UINT16 DGC)
{
    UINT8 TxData[8];

    TxData[0] = TxData[2] = TxData[4] = TxData[6] = (DGC & 0xff00) >> 8;
    TxData[1] = TxData[3] = TxData[5] = TxData[7] = (DGC & 0xff);

    IMX179_RegWrite(IMX179_DIG_GAIN_GR_MSB_REG, TxData, 8);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetShutterReg
 *
 *  @Description:: Apply electronic shutter setting
 *
 *  @Input      ::
 *      NumLinesEshrSpeed:    Integration time in number of Line
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX179_SetShutterReg(UINT32 NumLinesEshrSpeed)
{
    UINT8 TxData[2];

    TxData[0] = (NumLinesEshrSpeed & 0xff00) >> 8;
    TxData[1] = (NumLinesEshrSpeed & 0xff);

    IMX179_RegWrite(IMX179_COARSE_INTEG_TIME_MSB_REG, TxData, 2);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetSlowShutterReg
 *
 *  @Description:: Apply slow shutter setting
 *
 *  @Input      ::
 *     ExposureFrames: Number of frames in integation period
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX179_SetSlowShutterReg(UINT32 ExposureFrames)
{
    IMX179_FRAME_TIMING_s *pFrameTime = &IMX179Ctrl.FrameTime;
    UINT32 TargetFrameLengthLines = pFrameTime->FrameLengthLines * ExposureFrames;
    UINT8  TxData[2];

    TxData[0] = (TargetFrameLengthLines >> 8) & 0xff;
    TxData[1] = TargetFrameLengthLines & 0xff;

    IMX179_RegWrite(IMX179_FRM_LENGTH_MSB_REG, TxData, 2);

    IMX179Ctrl.Status.ModeInfo.FrameTime.FrameRate.NumUnitsInTick =
        IMX179ModeInfoList[IMX179Ctrl.Status.ModeInfo.Mode.Bits.Mode].FrameTiming.FrameRate.NumUnitsInTick * ExposureFrames;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetStandbyOn
 *
 *  @Description:: Configure sensor into standby mode
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX179_SetStandbyOn(void)
{
    UINT8 RegData = 0x0;

    IMX179_RegWrite(IMX179_MODE_SELECT_REG, &RegData, 1);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetStandbyOff
 *
 *  @Description:: Configure sensor to leave standby mode
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX179_SetStandbyOff(void)
{
    UINT8 RegData = 0x1;

    IMX179_RegWrite(IMX179_MODE_SELECT_REG, &RegData, 1);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_ChangeReadoutMode
 *
 *  @Description:: Switch sensor to requested mode.
 *                 It's for the case that sensor have been initialized.
 *
 *  @Input      ::
 *      ReadoutMode:   Sensor readout mode number
 *      pFrameTime:    Sensor frame time configuration
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_ChangeReadoutMode(IMX179_READOUT_MODE_e ReadoutMode, IMX179_FRAME_TIMING_s *pFrameTime)
{
    int i;
    UINT8 TxData[4];

    for (i = 0; i < IMX179_NUM_READOUT_MODE_REG; i++) {
        IMX179_RegWrite(IMX179ModeRegTable[i].Addr, &IMX179ModeRegTable[i].Data[ReadoutMode], 1);
    }

    TxData[0] = (pFrameTime->FrameLengthLines & 0xff00) >> 8;
    TxData[1] = (pFrameTime->FrameLengthLines & 0x00ff);
    TxData[2] = (pFrameTime->LineLengthPck & 0xff00) >> 8;
    TxData[3] = (pFrameTime->LineLengthPck & 0x00ff);

    IMX179_RegWrite(IMX179_FRM_LENGTH_MSB_REG, &TxData[0], 4);

    /* reset gain/shutter ctrl information */
    IMX179Ctrl.ShutterCtrl = 0xffffffff;
    IMX179Ctrl.CurrentAgc = 0xffffffff;
    IMX179Ctrl.CurrentDgc = 0xffffffff;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_Init
 *
 *  @Description:: Initialize sensor driver
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_Init(AMBA_DSP_CHANNEL_ID_u Chan)
{
    const IMX179_FRAME_TIMING_s *pFrameTime = &IMX179ModeInfoList[0].FrameTiming;
    AmbaPLL_SetSensorClk(pFrameTime->InputClk);             /* The default sensor input clock frequency */
    AmbaPLL_SetSensorClkDir(AMBA_PLL_SENSOR_CLK_OUTPUT);    /* Config clock output to sensor (or sensor I2C/SPI won't work) */

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_Enable
 *
 *  @Description:: Power on sensor
 *
 *  @Input      ::
 *      Chan: Vin ID and sensor ID
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_Enable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (!IMX179Ctrl.Status.DevStat.Bits.Sensor0Power) {
#if 0
        /* Turn On the Sensor Power if applicable */
        AmbaUserGPIO_SensorPowerCtrl(1);    /* Turn On the Sensor Power */
#endif
        /* enable internal pull-down for CLK_SI and master sync */
        // AmbaVIN_SetSensorPullCtrl(AMBA_VIN_SENSOR_PULL_DOWN);

        /* the Sensor Power is ON, Sensor is at Standby mode */
        IMX179Ctrl.Status.DevStat.Bits.Sensor0Power =
            IMX179Ctrl.Status.DevStat.Bits.Sensor0Standby = 1;
    }

    /* config mipi phy*/
    AmbaDSP_VinPhySetMIPI(Chan.Bits.VinID);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_Disable
 *
 *  @Description:: Power down sensor
 *
 *  @Input      ::
 *      Chan: Vin ID and sensor ID
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_Disable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (IMX179Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return OK;

    IMX179_SetStandbyOn();
    IMX179Ctrl.Status.DevStat.Bits.Sensor0Standby = 1;  /* Sensor is at Standby mode */

#if 0
    /* Turn Off the Sensor Power if applicable */
    AmbaUserGPIO_SensorPowerCtrl(0);    /* Turn Off the Sensor Power */
    IMX179Ctrl.Status.DevStat.Bits.Sensor0Power = 0;
#endif

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_GetStatus
 *
 *  @Description:: Get current sensor status
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *
 *  @Output     ::
 *      pStatus:    pointer to current sensor status
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_GetStatus(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_STATUS_INFO_s *pStatus)
{
    if (pStatus == NULL || IMX179Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return NG;

    IMX179Ctrl.Status.TimeToAllPelExposed = 0;  /* TODO: for flash light or mechanical shutter control */

    memcpy(pStatus, &IMX179Ctrl.Status, sizeof(AMBA_SENSOR_STATUS_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_GetModeInfo
 *
 *  @Description:: Get Mode Info of indicated mode
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *      Mode:   Sensor Readout Mode Number
 *
 *  @Output     ::
 *      pModeInfo: pointer to requested Mode Info
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_GetModeInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo)
{
    if (Mode.Bits.Mode == AMBA_SENSOR_CURRENT_MODE)
        Mode.Bits.Mode = IMX179Ctrl.Status.ModeInfo.Mode.Bits.Mode;

    if (Mode.Bits.Mode >= AMBA_SENSOR_IMX179_NUM_MODE || pModeInfo == NULL)
        return NG;

    IMX179_PrepareModeInfo(Mode, pModeInfo);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_GetDeviceInfo
 *
 *  @Description:: Get Sensor Device Info
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *
 *  @Output     ::
 *      pDeviceInfo:    pointer to device info
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_GetDeviceInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_DEVICE_INFO_s *pDeviceInfo)
{
    if (pDeviceInfo == NULL)
        return NG;

    memcpy(pDeviceInfo, &IMX179DeviceInfo, sizeof(AMBA_SENSOR_DEVICE_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_GetCurrentGainFactor
 *
 *  @Description:: Get Sensor Current Gain Factor
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *
 *  @Output     ::
 *      pDeviceInfo:    pointer to current gain factor
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_GetCurrentGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float *pGainFactor)
{
    *pGainFactor = (256.0 / (256 - IMX179Ctrl.CurrentAgc)) * (IMX179Ctrl.CurrentDgc / 256.0);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_GetCurrentShutterSpeed
 *
 *  @Description:: Get Sensor Current Exposure Time
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *
 *  @Output     ::
 *      pDeviceInfo:    pointer to current exposure time
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_GetCurrentShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float *pExposureTime)
{
    *pExposureTime = (IMX179Ctrl.Status.ModeInfo.RowTime * (float) IMX179Ctrl.ShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_ConvertGainFactor
 *
 *  @Description:: Convert gain factor to analog and digital gain control
 *
 *  @Input      ::
 *      Chan:               Vin ID and sensor ID
 *      DesiredFactor:      Desired gain factor
 *
 *  @Output     ::
 *      pActualFactor:      Achievable gain factor (12.20 fixed point)
 *      pAnalogGainCtrl:    Analog gain control for achievable gain factor
 *      pDigitalGainCtrl:   Digital gain control for achievable gain factor
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_ConvertGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float DesiredFactor, UINT32 *pActualFactor, UINT32 *pAnalogGainCtrl, UINT32 *pDigitalGainCtrl)
{
    if (pActualFactor == NULL || pAnalogGainCtrl == NULL || pDigitalGainCtrl == NULL)
        return NG;

    if (DesiredFactor < 1.0) {
        *pAnalogGainCtrl = 0;
        *pDigitalGainCtrl = 0x100;

        *pActualFactor = 1 << 20; /* .20 fixed point */
    } else {
        double ActualGain = 1.0;

        if (DesiredFactor <= 8.0 /* max analog gain */) {
            *pAnalogGainCtrl = (UINT32) (256.0 * (DesiredFactor - 1.0) / DesiredFactor + 0.5);
            *pDigitalGainCtrl = 0x100;

            ActualGain = 256.0 / (256 - *pAnalogGainCtrl);
        } else if (DesiredFactor <= 127.96875 /* max gain */) {
            *pAnalogGainCtrl = 224;
            *pDigitalGainCtrl = (UINT32) (256.0 / 8 * DesiredFactor + 0.5);

            ActualGain = 8.0 * ((double) *pDigitalGainCtrl / 256);
        } else {
            *pAnalogGainCtrl = 224;
            *pDigitalGainCtrl = 0xfff;

            ActualGain = 8.0 * 4095 / 256.0;
        }

        *pActualFactor = (UINT32) (ActualGain * 1024 * 1024); /* .20 fixed point */
    }

    // AmbaPrint("DesireFactor:%f, ActualFactor:%u, AnalogGainCtrl:0x%04X, DigitalGainCtrl: 0x%04X",
    //           DesiredFactor, *pActualFactor, *pAnalogGainCtrl, *pDigitalGainCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_ConvertShutterSpeed
 *
 *  @Description:: Convert exposure time to shutter control
 *
 *  @Input      ::
 *      Chan:           Vin ID and sensor ID
 *      ExposureTime:   Exposure time / shutter speed
 *
 *  @Output     ::
 *      pShutterCtrl:   Electronic shutter control
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_ConvertShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float ExposureTime, UINT32 *pShutterCtrl)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo = &IMX179Ctrl.Status.ModeInfo;
    const AMBA_DSP_FRAME_RATE_s *pFrameRate = &IMX179ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming.FrameRate;
    UINT32 NumExposureStepPerFrame = IMX179ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming.FrameLengthLines;
    UINT32 ExposureFrames, ExposureTimeMaxMargin;

    if (pShutterCtrl == NULL)
        return NG;

    /*---------------------------------------------------------------------------------*\
     * Number of Exposure Steps = (ExposureTime / FrameTime) * NumExposureStepPerFrame
     *                          = ExposureTime * FrameRate * NumExposureStepPerFrame
    \*---------------------------------------------------------------------------------*/
    *pShutterCtrl = (UINT32) (ExposureTime * pFrameRate->TimeScale / pFrameRate->NumUnitsInTick * NumExposureStepPerFrame);

    ExposureFrames = (*pShutterCtrl / NumExposureStepPerFrame);
    ExposureFrames = (*pShutterCtrl % NumExposureStepPerFrame) ? ExposureFrames + 1 : ExposureFrames;
    ExposureTimeMaxMargin = ExposureFrames * NumExposureStepPerFrame - 4;
    if (*pShutterCtrl > ExposureTimeMaxMargin)
        *pShutterCtrl = ExposureTimeMaxMargin;

    // AmbaPrint("ExposureTime:%f, ShutterCtrl:0x%04X", ExposureTime, *pShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetAnalogGainCtrl
 *
 *  @Description:: Set analog gain control
 *
 *  @Input      ::
 *      Chan:               Vin ID and sensor ID
 *      AnalogGainCtrl:     Analog gain control for achievable gain factor
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_SetAnalogGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 AnalogGainCtrl)
{
    IMX179_SetAnalogGainReg(AnalogGainCtrl);

    IMX179Ctrl.CurrentAgc = AnalogGainCtrl;

    // AmbaPrint("Set Again: 0x%04X", AnalogGainCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetDigitalGainCtrl
 *
 *  @Description:: Set digital gain control
 *
 *  @Input      ::
 *      Chan:               Vin ID and sensor ID
 *      DigitalGainCtrl:    Digital gain control for achievable gain factor
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_SetDigitalGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 DigitalGainCtrl)
{
    IMX179_SetDigitalGainReg(DigitalGainCtrl);

    IMX179Ctrl.CurrentDgc = DigitalGainCtrl;

    // AmbaPrint("Set Dgain: 0x%04X", DigitalGainCtrl);

    return OK;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetShutterCtrl
 *
 *  @Description:: set shutter control
 *
 *  @Input      ::
 *      Chan:           Vin ID and sensor ID
 *      ShutterCtrl:    Electronic shutter control
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_SetShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 ShutterCtrl)
{
    IMX179_SetShutterReg(ShutterCtrl);

    IMX179Ctrl.ShutterCtrl = ShutterCtrl;

    // AmbaPrint("Set Shutter: 0x%04X", ShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_SetSlowShutterCtrl
 *
 *  @Description:: set slow shutter control
 *
 *  @Input      ::
 *      Chan:               Vin ID and sensor ID
 *      pSlowShutterCtrl:   Electronic slow shutter control
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_SetSlowShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 SlowShutterCtrl)
{
    IMX179_SetSlowShutterReg(SlowShutterCtrl);

    // AmbaPrint("Set SlowShutter: 0x%04X", SlowShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX179_Config
 *
 *  @Description:: Set sensor to indicated mode
 *
 *  @Input      ::
 *      Chan:               Vin ID and sensor ID
 *      Mode:               Sensor Readout Mode Number
 *      ElecShutterMode:    Operating Mode of the Electronic Shutter
 *
 *  @Output     ::
 *      pModeInfo: pointer to mode info of target mode
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX179_Config(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_ESHUTTER_TYPE_e ElecShutterMode)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo;
    UINT16 SensorMode = Mode.Bits.Mode;
    IMX179_READOUT_MODE_e ReadoutMode = IMX179ModeInfoList[SensorMode].ReadoutMode;
    AMBA_VIN_CHANNEL_e VinChanNo = (AMBA_VIN_CHANNEL_e) Chan.Bits.VinID;

    if (SensorMode >= AMBA_SENSOR_IMX179_NUM_MODE)
        return NG;

    if (Mode.Bits.DummyMasterSync == 1)
        return OK;

    if ((IMX179Ctrl.Status.ModeInfo.Mode.Bits.Mode == SensorMode) &&
        (IMX179Ctrl.Status.ModeInfo.Mode.Bits.VerticalFlip != Mode.Bits.VerticalFlip) &&
        (IMX179Ctrl.Status.ElecShutterMode == ElecShutterMode) &&
        (IMX179Ctrl.Status.DevStat.Bits.Sensor0Power) &&
        (!IMX179Ctrl.Status.DevStat.Bits.Sensor0Standby)) {
        /* run-time flip sensor readout direction */
        /* TODO */
        UINT8 TxData = Mode.Bits.VerticalFlip ? 2 : 0;
        IMX179_RegWrite(IMX179_IMG_ORIENTATION_REG, &TxData, 1);

        /* update status */
        pModeInfo = &IMX179Ctrl.Status.ModeInfo;
        IMX179_PrepareModeInfo(Mode, pModeInfo);
    } else {
        UINT8 TxData = 0;
        IMX179_FRAME_TIMING_s *pFrameTime = &IMX179Ctrl.FrameTime;

        /* update status */
        IMX179Ctrl.Status.ElecShutterMode = ElecShutterMode;

        pModeInfo = &IMX179Ctrl.Status.ModeInfo;
        IMX179_PrepareModeInfo(Mode, pModeInfo);
        memcpy(pFrameTime, &IMX179ModeInfoList[SensorMode].FrameTiming, sizeof(IMX179_FRAME_TIMING_s));

        IMX179VinConfig.NumActiveLanes = pModeInfo->OutputInfo.NumDataLanes;

        AmbaVIN_CalculateMphyConfig(pModeInfo->OutputInfo.DataRate, &IMX179VinConfig.MipiCtrl);
        AmbaVIN_Reset(VinChanNo, AMBA_VIN_MIPI, &IMX179VinConfig.MipiCtrl);

        /* set pll */
        AmbaPLL_SetSensorClk(pModeInfo->FrameTime.InputClk);
        AmbaKAL_TaskSleep(1);

        IMX179_HardwareReset();
        AmbaKAL_TaskSleep(6);

        /* write registers of mode change to sensor */
        IMX179_ChangeReadoutMode(ReadoutMode, pFrameTime);

        /* config sensor readout direction */
        TxData = Mode.Bits.VerticalFlip ? 2 : 0;
        IMX179_RegWrite(IMX179_IMG_ORIENTATION_REG, &TxData, 1);

        IMX179_SetSlowShutterCtrl(Chan, 1);
        IMX179_SetShutterCtrl(Chan, pModeInfo->NumExposureStepPerFrame >> 1);
        IMX179_SetAnalogGainCtrl(Chan, 0);
        IMX179_SetDigitalGainCtrl(Chan, 0x100);

        IMX179_SetStandbyOff();

        /* config vin */
        IMX179_ConfigVin(pModeInfo, pFrameTime);

        IMX179Ctrl.Status.DevStat.Bits.Sensor0Standby = 0;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*/
AMBA_SENSOR_OBJ_s AmbaSensor_IMX179Obj = {
    .Init                   = IMX179_Init,
    .Enable                 = IMX179_Enable,
    .Disable                = IMX179_Disable,
    .Config                 = IMX179_Config,
    .GetStatus              = IMX179_GetStatus,
    .GetModeInfo            = IMX179_GetModeInfo,
    .GetDeviceInfo          = IMX179_GetDeviceInfo,
    .GetCurrentGainFactor   = IMX179_GetCurrentGainFactor,
    .GetCurrentShutterSpeed = IMX179_GetCurrentShutterSpeed,

    .ConvertGainFactor      = IMX179_ConvertGainFactor,
    .ConvertShutterSpeed    = IMX179_ConvertShutterSpeed,

    .SetAnalogGainCtrl      = IMX179_SetAnalogGainCtrl,
    .SetDigitalGainCtrl     = IMX179_SetDigitalGainCtrl,
    .SetShutterCtrl         = IMX179_SetShutterCtrl,
    .SetSlowShutterCtrl     = IMX179_SetSlowShutterCtrl,

    .RegisterRead           = IMX179_RegisterRead,
    .RegisterWrite          = IMX179_RegisterWrite,
};
