/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_IMX377.c
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of SONY IMX377 CMOS sensor with MIPI interface
 *
 *  @History        ::
 *      Date        Name        Comments
 *      03/17/2015  S.Y.Cheng   Created
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
#include "AmbaSensor_IMX377_1440M.h"

#include "AmbaPrintk.h"

/*-----------------------------------------------------------------------------------------------*\
 * IMX377 sensor runtime info
\*-----------------------------------------------------------------------------------------------*/
IMX377_CTRL_s IMX377Ctrl = {0};

/*-----------------------------------------------------------------------------------------------*\
 * Configurations for Amba. VIN side
\*-----------------------------------------------------------------------------------------------*/
static AMBA_DSP_VIN_MIPI_CONFIG_s IMX377VinConfig = {
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
    .DataTypeMask           = 0x1f,
    .DataType               = 0x20,
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
        .HsSettleTime   = 32,
        .HsTermTime     = 16,
        .ClkSettleTime  = 32,
        .ClkTermTime    = 8,
        .ClkMissTime    = 8,
        .RxInitTime     = 64,
    }
};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_PrepareModeInfo
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
static void IMX377_PrepareModeInfo(AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo)
{
    UINT16 SensorMode                             = Mode.Bits.Mode;
    IMX377_READOUT_MODE_e           ReadoutMode   = IMX377ModeInfoList[SensorMode].ReadoutMode;
    const IMX377_FRAME_TIMING_s     *pFrameTiming = &IMX377ModeInfoList[SensorMode].FrameTiming;
    const AMBA_SENSOR_OUTPUT_INFO_s *pOutputInfo  = &IMX377OutputInfo[ReadoutMode];
    const AMBA_SENSOR_INPUT_INFO_s  *pInputInfo;
    UINT32 RefClk                                 = pFrameTiming->InputClk * pFrameTiming->RefClkMul; /* 72MHz */

    if (Mode.Bits.VerticalFlip == 0)
        pInputInfo = &IMX377InputInfoNormalReadout[ReadoutMode];
    else if (Mode.Bits.VerticalFlip == 1)
        pInputInfo = &IMX377InputInfoInversionReadout[ReadoutMode];

    pModeInfo->Mode = Mode;
    pModeInfo->LineLengthPck = (pFrameTiming->NumTickPerXhs * pFrameTiming->NumXhsPerH) *
                               (pOutputInfo->DataRate / RefClk) *
                               (pOutputInfo->NumDataLanes) / (pOutputInfo->NumDataBits);
    pModeInfo->FrameLengthLines = pFrameTiming->NumXhsPerV / pFrameTiming->NumXhsPerH;
    pModeInfo->NumExposureStepPerFrame = pFrameTiming->NumXhsPerV;
    pModeInfo->PixelRate        = (float) pOutputInfo->DataRate * pOutputInfo->NumDataLanes / pOutputInfo->NumDataBits;
    pModeInfo->RowTime = (float) (pFrameTiming->NumTickPerXhs * pFrameTiming->NumXhsPerH) / RefClk;
    pModeInfo->FrameTime.InputClk = pFrameTiming->InputClk;
    memcpy(&pModeInfo->FrameTime.FrameRate, &pFrameTiming->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    memcpy(&pModeInfo->InputInfo, pInputInfo, sizeof(AMBA_SENSOR_INPUT_INFO_s));
    memcpy(&pModeInfo->OutputInfo, pOutputInfo, sizeof(AMBA_SENSOR_OUTPUT_INFO_s));
    memcpy(&pModeInfo->MinFrameRate, &pFrameTiming->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));

    /* HDR information */
    memset(&pModeInfo->HdrInfo, 0, sizeof(AMBA_SENSOR_HDR_INFO_s)); // Not support HDR

    /* updated minimum frame rate limitation */
    pModeInfo->MinFrameRate.NumUnitsInTick *= 0x3fff / (pFrameTiming->NumXhsPerV / pFrameTiming->NumXvsPerV);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_ConfigVin
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
static void IMX377_ConfigVin(AMBA_SENSOR_MODE_INFO_s *pModeInfo, IMX377_FRAME_TIMING_s *pFrameTime)
{
    AMBA_SENSOR_OUTPUT_INFO_s  *pImgInfo = &pModeInfo->OutputInfo;
    AMBA_DSP_VIN_MIPI_CONFIG_s *pVinCfg = &IMX377VinConfig;

    pVinCfg->NumActiveLanes = pImgInfo->NumDataLanes;
    pVinCfg->Info.BayerPattern = (AMBA_DSP_SENSOR_PATTERN_e) pImgInfo->CfaPattern;
    pVinCfg->Info.NumDataBits = pImgInfo->NumDataBits;

    pVinCfg->RxHvSyncCtrl.NumActivePixels  = pImgInfo->OutputWidth;
    pVinCfg->RxHvSyncCtrl.NumActiveLines   = pImgInfo->OutputHeight;
    pVinCfg->RxHvSyncCtrl.NumTotalPixels   = pModeInfo->LineLengthPck - 1;
    pVinCfg->RxHvSyncCtrl.NumTotalLines    = pModeInfo->FrameLengthLines;

    memcpy(&pVinCfg->Info.FrameRate, &pFrameTime->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    memcpy(&pVinCfg->Info.DspPhaseShift, &pImgInfo->DspPhaseShift, sizeof(AMBA_DSP_PHASE_SHIFT_CTRL_s));

    AmbaDSP_VinConfigMIPI(AMBA_VIN_CHANNEL0, pVinCfg);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_HardwareReset
 *
 *  @Description:: Reset IMX377 Image Sensor Device
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static inline void IMX377_HardwareReset(void)
{
    AmbaUserGPIO_SensorResetCtrl(0);
    AmbaKAL_TaskSleep(1);               /* XCLR Low level pulse width >= 100ns */
    AmbaUserGPIO_SensorResetCtrl(1);
}

static int IMX377_RegRead(UINT16 Addr, UINT8 *pRxData, int Size)
{
    int i, RetStatus;
    UINT16 TxData[4];
    UINT8  RxData[128];

    TxData[0] = AMBA_I2C_RESTART_FLAG | (0x34);
    TxData[1] = (Addr >> 8);    /* Register Address [15:8] */
    TxData[2] = (Addr & 0xff);  /* Register Address [7:0]  */
    TxData[3] = AMBA_I2C_RESTART_FLAG | (0x35);

    RetStatus = AmbaI2C_ReadAfterWrite(AMBA_I2C_CHANNEL0,
                                       AMBA_I2C_SPEED_FAST,
                                       4, (UINT16 *) TxData,
                                       Size, RxData,
                                       AMBA_KAL_WAIT_FOREVER);
    for (i = 0; i < Size; i++)
        *pRxData++ = RxData[i];

    return RetStatus;
}

static int IMX377_RegWrite(UINT16 Addr, UINT8 *pTxData, int Size)
{
    int i;
    UINT8 TxData[128];

    TxData[0] = (UINT8) ((Addr & 0xff00) >> 8);
    TxData[1] = (UINT8) (Addr & 0x00ff);

    for (i = 0; i < Size; i++)
        TxData[i + 2] = pTxData[i];

    if (AmbaI2C_Write(AMBA_I2C_CHANNEL0,
                      AMBA_I2C_SPEED_FAST,
                      0x34, Size + 2, TxData,
                      AMBA_KAL_WAIT_FOREVER) != OK)
        AmbaPrint("I2C does not work!!!!!");

#if 0 // for debugging
    {
        UINT8 RxData;

        for (i = 0; i < Size; i++, Addr++) {
            IMX377_RegRead(Addr, &RxData, 1);
            AmbaPrint("[IMX377] Addr=0x%04X, Data=0x%02X, Rd=0x%02X",
                      Addr, pTxData[i], RxData);
        }
    }
#endif

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_RegisterRead
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
static int IMX377_RegisterRead(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 *pData)
{
    UINT8 RxData;

    IMX377_RegRead(Addr, &RxData, 1);
    *pData = RxData;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_RegisterWrite
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
static int IMX377_RegisterWrite(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 Data)
{
    UINT8 TxData = (UINT8) Data;

    IMX377_RegWrite(Addr, &TxData, 1);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetAnalogGainReg
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
static void IMX377_SetAnalogGainReg(UINT16 PGC)
{
    UINT8 TxData[2];

    TxData[1] = (PGC >> 8) & 0x00ff;
    TxData[0] = PGC & 0x00ff;
    IMX377_RegWrite(IMX377_PGC_LSB_REG, TxData, 2);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetDigitalGainReg
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
static void IMX377_SetDigitalGainReg(UINT8 DGC)
{
    UINT8 TxData[1];

    TxData[0] = DGC;
    IMX377_RegWrite(IMX377_DGAIN_REG, TxData, 1);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetShutterReg
 *
 *  @Description:: Apply electronic shutter setting
 *
 *  @Input      ::
 *      NumXhsEshrSpeed:    Integration time in number of XHS period
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static UINT32 IMX377_IntegrationPeriodInXVS = 1;  /* current intergration period (unit: XVS) */
static void IMX377_SetShutterReg(UINT32 NumXhsEshrSpeed)
{
    UINT32 NumXhsPerXvs = IMX377Ctrl.FrameTime.NumXhsPerV / IMX377Ctrl.FrameTime.NumXvsPerV;
    UINT32 SVR; /* specifies the exposure end vertical period */
    UINT32 SHR; /* specifies the exposure start horizontal period */

    UINT8  TxData[2];

    SVR = IMX377_IntegrationPeriodInXVS - 1;
    SHR = (SVR + 1) * NumXhsPerXvs - NumXhsEshrSpeed;

    /* Since IMX377 doesn't support SPL, SHR should
       not be greater than 0xffff */
    if (SHR > 0xffff)
        AmbaPrint("[IMX377] SHR (%u) should not be greater than 0xffff.", SHR);

    TxData[0] = SHR & 0x00ff;
    TxData[1] = (SHR >> 8) & 0x00ff;
    IMX377_RegWrite(IMX377_SHR_LSB_REG, TxData, 2);

    IMX377Ctrl.CurrentShrCtrlSHR = SHR;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetSlowShutterReg
 *
 *  @Description:: Apply slow shutter setting
 *
 *  @Input      ::
 *     IntegrationPeriodInFrame: Number of frames in integation period
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX377_SetSlowShutterReg(UINT32 IntegrationPeriodInFrame)
{
    IMX377_FRAME_TIMING_s *pFrameTime = &IMX377Ctrl.FrameTime;
    UINT32 SVR = 0;
    UINT8  TxData[2];

    /* 1 frame might includes more than 1 XVS */
    IMX377_IntegrationPeriodInXVS = IntegrationPeriodInFrame * pFrameTime->NumXvsPerV;

    SVR = IMX377_IntegrationPeriodInXVS - 1;

    TxData[0] = SVR & 0x00ff;
    TxData[1] = (SVR >> 8) & 0x00ff;

    IMX377_RegWrite(IMX377_SVR_LSB_REG, TxData, 2);
    IMX377Ctrl.CurrentShrCtrlSVR = SVR;
    IMX377Ctrl.Status.ModeInfo.FrameTime.FrameRate.NumUnitsInTick =
        IMX377ModeInfoList[IMX377Ctrl.Status.ModeInfo.Mode.Bits.Mode].FrameTiming.FrameRate.NumUnitsInTick * IntegrationPeriodInFrame;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetStandbyOn
 *
 *  @Description:: Configure sensor into standby mode
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX377_SetStandbyOn(void)
{
    UINT8 RegData = 0x16 | 0x01;
    IMX377_RegWrite(IMX377_OP_REG, &RegData, 1);
}

#if 0
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetStandbyOff
 *
 *  @Description:: Configure sensor to leave standby mode
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX377_SetStandbyOff(void)
{
    UINT8 RegData = 0x16 | 0x00;
    IMX377_RegWrite(IMX377_OP_REG, &RegData, 1);
}
#endif

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetReadoutPlusTimingRegs
 *
 *  @Description:: Set all registers of PLSTMG settings
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX377_SetReadoutPlusTimingRegs(void)
{
    int i;

    for (i = 0; i < IMX377_NUM_READOUT_PSTMG_REG; i++) {
        IMX377_RegWrite(IMX377PlstmgRegTable[i].Addr, &IMX377PlstmgRegTable[i].Data, 1);
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_ChangeReadoutMode
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
static int IMX377_ChangeReadoutMode(IMX377_READOUT_MODE_e ReadoutMode, IMX377_FRAME_TIMING_s *pFrameTime)
{
    int i;
    UINT32 HMAX, VMAX;
    UINT8 TxRegData[8];

    for (i = 0; i < IMX377_NUM_READOUT_MODE_REG; i++) {
        IMX377_RegWrite(IMX377ModeRegTable[i].Addr, &IMX377ModeRegTable[i].Data[ReadoutMode], 1);
    }

    HMAX = pFrameTime->NumTickPerXhs;
    VMAX = pFrameTime->NumXhsPerV / pFrameTime->NumXvsPerV;
    TxRegData[0] = (HMAX & 0xff);
    TxRegData[1] = (HMAX & 0xff00) >> 8;
    TxRegData[2] = (VMAX & 0xff);
    TxRegData[3] = (VMAX & 0xff00) >> 8;
    TxRegData[4] = (VMAX & 0x0f0000) >> 16;

    IMX377_RegWrite(IMX377_HMAX_LSB_REG, &TxRegData[0], 5);

    /* reset gain/shutter ctrl information */
    IMX377Ctrl.CurrentShrCtrlSHR = 0xffffffff;
    IMX377Ctrl.CurrentShrCtrlSVR = 0xffffffff;
    IMX377Ctrl.CurrentAgc = 0xffffffff;
    IMX377Ctrl.CurrentDgc = 0xffffffff;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_Init
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
static int IMX377_Init(AMBA_DSP_CHANNEL_ID_u Chan)
{
    const IMX377_FRAME_TIMING_s *pFrameTime = &IMX377ModeInfoList[0].FrameTiming;
    AmbaPLL_SetSensorClk(pFrameTime->InputClk);             /* The default sensor input clock frequency */
    AmbaPLL_SetSensorClkDir(AMBA_PLL_SENSOR_CLK_OUTPUT);    /* Config clock output to sensor (or sensor SPI won't work) */

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_Enable
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
static int IMX377_Enable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (!IMX377Ctrl.Status.DevStat.Bits.Sensor0Power) {
#if 0
        /* Turn On the Sensor Power if applicable */
        AmbaUserGPIO_SensorPowerCtrl(1);    /* Turn On the Sensor Power */
#endif
        /* enable internal pull-down for CLK_SI and master sync */
        // AmbaVIN_SetSensorPullCtrl(AMBA_VIN_SENSOR_PULL_DOWN);

        /* the Sensor Power is ON, Sensor is at Standby mode */
        IMX377Ctrl.Status.DevStat.Bits.Sensor0Power =
            IMX377Ctrl.Status.DevStat.Bits.Sensor0Standby = 1;
    }

    /* config mipi phy*/
    AmbaDSP_VinPhySetMIPI(Chan.Bits.VinID);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_Disable
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
static int IMX377_Disable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (IMX377Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return OK;

    IMX377_SetStandbyOn();
    IMX377Ctrl.Status.DevStat.Bits.Sensor0Standby = 1;  /* Sensor is at Standby mode */

#if 0
    /* Turn Off the Sensor Power if applicable */
    AmbaUserGPIO_SensorPowerCtrl(0);    /* Turn Off the Sensor Power */
    IMX377Ctrl.Status.DevStat.Bits.Sensor0Power = 0;
#endif

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_GetStatus
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
static int IMX377_GetStatus(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_STATUS_INFO_s *pStatus)
{
    if (pStatus == NULL || IMX377Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return NG;

    IMX377Ctrl.Status.TimeToAllPelExposed = 0;  /* TODO: for flash light or mechanical shutter control */

    memcpy(pStatus, &IMX377Ctrl.Status, sizeof(AMBA_SENSOR_STATUS_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_GetModeInfo
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
static int IMX377_GetModeInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo)
{
    if (Mode.Bits.Mode == AMBA_SENSOR_CURRENT_MODE)
        Mode.Bits.Mode = IMX377Ctrl.Status.ModeInfo.Mode.Bits.Mode;

    if (Mode.Bits.Mode >= AMBA_SENSOR_IMX377_NUM_MODE || pModeInfo == NULL)
        return NG;

    IMX377_PrepareModeInfo(Mode, pModeInfo);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_GetDeviceInfo
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
static int IMX377_GetDeviceInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_DEVICE_INFO_s *pDeviceInfo)
{
    if (pDeviceInfo == NULL)
        return NG;

    memcpy(pDeviceInfo, &IMX377DeviceInfo, sizeof(AMBA_SENSOR_DEVICE_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_GetCurrentGainFactor
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
static int IMX377_GetCurrentGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float *pGainFactor)
{
    *pGainFactor = 2048. / (2048 - IMX377Ctrl.CurrentAgc) * (1 << IMX377Ctrl.CurrentDgc);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_GetCurrentShutterSpeed
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
static int IMX377_GetCurrentShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float *pExposureTime)
{
    *pExposureTime = (IMX377Ctrl.Status.ModeInfo.RowTime / IMX377Ctrl.FrameTime.NumXhsPerH) *
                     ((IMX377Ctrl.CurrentShrCtrlSVR + 1) * (IMX377Ctrl.FrameTime.NumXhsPerV / IMX377Ctrl.FrameTime.NumXvsPerV) - IMX377Ctrl.CurrentShrCtrlSHR);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_ConvertGainFactor
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
static int IMX377_ConvertGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float DesiredFactor, UINT32 *pActualFactor, UINT32 *pAnalogGainCtrl, UINT32 *pDigitalGainCtrl)
{
    if (pActualFactor == NULL || pAnalogGainCtrl == NULL || pDigitalGainCtrl == NULL)
        return NG;

    if (DesiredFactor < 1.0) {
        *pAnalogGainCtrl = 0;
        *pDigitalGainCtrl = 0;

        *pActualFactor = 1 << 20; /* .20 fixed point */
    } else {
        float AnalogGain;
        float DigitalGain;

        DigitalGain = DesiredFactor / 22.5;

        if (DigitalGain <= 1.0) {
            *pDigitalGainCtrl = 0;
            AnalogGain = DesiredFactor;
        } else {
            int i;

            for (i = 1; i < 3; i++) {
                if (DigitalGain < (1 << i)) {
                    break;
                }
            }

            *pDigitalGainCtrl = i;
            DigitalGain = 1 << i;

            AnalogGain = DesiredFactor / DigitalGain;
        }

        *pAnalogGainCtrl = (UINT32) ((2048 * (AnalogGain - 1.0) / AnalogGain) + 0.5);

        if (*pAnalogGainCtrl > 1957) {
            *pAnalogGainCtrl = 1957;
        }

        *pActualFactor = (UINT32) (2048. * (1 << 20) / (2048 - *pAnalogGainCtrl) * (1 << *pDigitalGainCtrl) + 0.5); /* ActualFactor .20 fixed point */
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_ConvertShutterSpeed
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
static int IMX377_ConvertShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float ExposureTime, UINT32 *pShutterCtrl)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo = &IMX377Ctrl.Status.ModeInfo;
    const AMBA_DSP_FRAME_RATE_s *pFrameRate = &IMX377ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming.FrameRate;
    IMX377_READOUT_MODE_e ReadoutMode = IMX377ModeInfoList[pModeInfo->Mode.Bits.Mode].ReadoutMode;
    UINT32 ExpXhsNum    = 0;
    UINT32 MinExpXhsNum = 0;
    UINT32 MaxExpXhsNum = 0;

    if (pShutterCtrl == NULL)
        return NG;
    /*---------------------------------------------------------------------------------*\
     * Number of Exposure Steps = (ExposureTime / FrameTime) * NumExposureStepPerFrame
     *                          = ExposureTime * FrameRate * NumExposureStepPerFrame
    \*---------------------------------------------------------------------------------*/
    ExpXhsNum = (UINT32)(ExposureTime * pFrameRate->TimeScale / pFrameRate->NumUnitsInTick * pModeInfo->NumExposureStepPerFrame);

    if (IMX377Ctrl.Status.ElecShutterMode == AMBA_SENSOR_ESHUTTER_TYPE_GLOBAL_RESET) {
        return NG; /* not supported */
    } else {
        switch (ReadoutMode) {
        case IMX377_TYPE_2_3_MODE_0A:
        case IMX377_TYPE_2_3_MODE_0:
        case IMX377_TYPE_2_3_MODE_1:
        case IMX377_TYPE_2_5_MODE_0:
        case IMX377_TYPE_2_5_MODE_1:
            MinExpXhsNum = 4;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 8;
            break;

        case IMX377_TYPE_2_3_MODE_1A:
        case IMX377_TYPE_2_5_MODE_1A:
            MinExpXhsNum = 4;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 12;
            break;

        case IMX377_TYPE_2_3_MODE_2:
        case IMX377_TYPE_2_5_MODE_2:
            MinExpXhsNum = 4;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 7;
            break;

        case IMX377_TYPE_2_3_MODE_3:
            MinExpXhsNum = 1;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 8;
            break;

        case IMX377_TYPE_2_3_MODE_4:
        case IMX377_TYPE_2_5_MODE_4:
            MinExpXhsNum = 6;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 12;
            break;

        default:
            MinExpXhsNum = 4;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 8;
            break;
        }
    }

    if (ExpXhsNum > MaxExpXhsNum)
        *pShutterCtrl = MaxExpXhsNum;
    else if (ExpXhsNum < MinExpXhsNum)
        *pShutterCtrl = MinExpXhsNum;
    else
        *pShutterCtrl = ExpXhsNum;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetAnalogGainCtrl
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
static int IMX377_SetAnalogGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 AnalogGainCtrl)
{
    IMX377_SetAnalogGainReg(AnalogGainCtrl);

    IMX377Ctrl.CurrentAgc = AnalogGainCtrl;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetDigitalGainCtrl
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
static int IMX377_SetDigitalGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 DigitalGainCtrl)
{
    IMX377_SetDigitalGainReg(DigitalGainCtrl);

    IMX377Ctrl.CurrentDgc = DigitalGainCtrl;

    return OK;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetShutterCtrl
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
static int IMX377_SetShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 ShutterCtrl)
{
    IMX377_SetShutterReg(ShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_SetSlowShutterCtrl
 *
 *  @Description:: set slow shutter control
 *
 *  @Input      ::
 *      Chan:           Vin ID and sensor ID
 *      pSlowShutterCtrl:   Electronic slow shutter control
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX377_SetSlowShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 SlowShutterCtrl)
{
    IMX377_SetSlowShutterReg(SlowShutterCtrl);
    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX377_Config
 *
 *  @Description:: Set sensor to indicated mode
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *      Mode:   Sensor Readout Mode Number
 *      ElecShutterMode:    Operating Mode of the Electronic Shutter
 *
 *  @Output     ::
 *      pModeInfo: pointer to mode info of target mode
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX377_Config(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_ESHUTTER_TYPE_e ElecShutterMode)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo;
    UINT16 SensorMode = Mode.Bits.Mode;
    IMX377_READOUT_MODE_e ReadoutMode = IMX377ModeInfoList[SensorMode].ReadoutMode;
    AMBA_VIN_CHANNEL_e VinChanNo = (AMBA_VIN_CHANNEL_e) Chan.Bits.VinID;

    if (SensorMode >= AMBA_SENSOR_IMX377_NUM_MODE)
        return NG;

    if (Mode.Bits.DummyMasterSync == 1)
        return OK;

    if ((IMX377Ctrl.Status.ModeInfo.Mode.Bits.Mode == SensorMode) &&
        (IMX377Ctrl.Status.ModeInfo.Mode.Bits.VerticalFlip != Mode.Bits.VerticalFlip) &&
        (IMX377Ctrl.Status.ElecShutterMode == ElecShutterMode) &&
        (IMX377Ctrl.Status.DevStat.Bits.Sensor0Power) &&
        (!IMX377Ctrl.Status.DevStat.Bits.Sensor0Standby)) {
        /* run-time flip sensor readout direction */
        UINT8 TxData = Mode.Bits.VerticalFlip;
        IMX377_RegWrite(IMX377_MDVREV_REG, &TxData, 1);

        /* update status */
        pModeInfo = &IMX377Ctrl.Status.ModeInfo;
        IMX377_PrepareModeInfo(Mode, pModeInfo);
    } else {
        UINT8 TxData = 0;
        IMX377_FRAME_TIMING_s *pFrameTime = &IMX377Ctrl.FrameTime;

        /* update status */
        IMX377Ctrl.Status.ElecShutterMode = ElecShutterMode;

        pModeInfo = &IMX377Ctrl.Status.ModeInfo;
        IMX377_PrepareModeInfo(Mode, pModeInfo);
        memcpy(pFrameTime, &IMX377ModeInfoList[SensorMode].FrameTiming, sizeof(IMX377_FRAME_TIMING_s));

        IMX377VinConfig.NumActiveLanes = pModeInfo->OutputInfo.NumDataLanes;

        AmbaVIN_CalculateMphyConfig(pModeInfo->OutputInfo.DataRate, &IMX377VinConfig.MipiCtrl);
        AmbaVIN_Reset(VinChanNo, AMBA_VIN_MIPI, &IMX377VinConfig.MipiCtrl);

        /* set pll */
        AmbaPLL_SetSensorClk(pModeInfo->FrameTime.InputClk);
        AmbaKAL_TaskSleep(1);

        IMX377_HardwareReset();
        AmbaKAL_TaskSleep(1);

        /* ----------------------------- */
        /* [Analog stabilization period] */
        /* ----------------------------- */
        /* 1st stabilization period */
        TxData = 0x00;
        IMX377_RegWrite(IMX377_PLRD10_REG, &TxData, 1);
        TxData = 0x00;
        IMX377_RegWrite(IMX377_PLRD11_REG, &TxData, 1);
        TxData = 0x02;
        IMX377_RegWrite(IMX377_PLRD13_REG, &TxData, 1);
        TxData = 0x02;
        IMX377_RegWrite(IMX377_PLRD14_REG, &TxData, 1);
        TxData = 0x01;
        IMX377_RegWrite(IMX377_PLRD12_REG, &TxData, 1);

        TxData = 0xf0;
        IMX377_RegWrite(IMX377_PLRD1_LSB_REG, &TxData, 1);
        TxData = 0x00;
        IMX377_RegWrite(IMX377_PLRD1_MSB_REG, &TxData, 1);
        TxData = 0x90;
        IMX377_RegWrite(IMX377_PLRD2_REG, &TxData, 1);
        TxData = 0x02;
        IMX377_RegWrite(IMX377_PLRD3_REG, &TxData, 1);
        TxData = 0x02;
        IMX377_RegWrite(IMX377_PLRD4_REG, &TxData, 1);

        IMX377_SetReadoutPlusTimingRegs();

        /* 2nd stabilization period */
        TxData = 0x16;
        IMX377_RegWrite(IMX377_OP_REG, &TxData, 1);
        TxData = 0xa2;
        IMX377_RegWrite(IMX377_SYNCDRV_REG, &TxData, 1);
        TxData = 0x01;
        IMX377_RegWrite(IMX377_LMRSVRG_REG, &TxData, 1);
        TxData = 0x11;
        IMX377_RegWrite(IMX377_STBPL_IF_AD_REG, &TxData, 1);
        TxData = 0x00;
        IMX377_RegWrite(IMX377_MIPIEBD_TAGEN_REG, &TxData, 1);

        AmbaKAL_TaskSleep(10);

        /* 3rd stabilization period */
        TxData = 0x00;
        IMX377_RegWrite(IMX377_STBPL_IF_AD_REG, &TxData, 1);

        AmbaKAL_TaskSleep(1);

        /* 4th stabilization period */
        TxData = 0x04;
        IMX377_RegWrite(IMX377_OP_REG, &TxData, 1);

        AmbaKAL_TaskSleep(7);

        /* ------------------------- */
        /* [Standby cancel sequence] */
        /* ------------------------- */
        /* write registers of mode change to sensor */
        IMX377_ChangeReadoutMode(ReadoutMode, pFrameTime);

        /* config sensor readout direction */
        TxData = Mode.Bits.VerticalFlip;
        IMX377_RegWrite(IMX377_MDVREV_REG, &TxData, 1);

        IMX377_SetSlowShutterReg(1);
        IMX377_SetShutterReg(pModeInfo->NumExposureStepPerFrame >> 1);
        IMX377_SetAnalogGainCtrl(Chan, 0);
        IMX377_SetDigitalGainCtrl(Chan, 0);

        TxData = 0x10;
        IMX377_RegWrite(IMX377_CLPSQRST_REG, &TxData, 1);
        TxData = 0x00;
        IMX377_RegWrite(IMX377_XMSTA_REG, &TxData, 1);

        AmbaKAL_TaskSleep(34);

        /* config vin */
        IMX377_ConfigVin(pModeInfo, pFrameTime);

        IMX377Ctrl.Status.DevStat.Bits.Sensor0Standby = 0;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*/
AMBA_SENSOR_OBJ_s AmbaSensor_IMX377Obj = {
    .Init                   = IMX377_Init,
    .Enable                 = IMX377_Enable,
    .Disable                = IMX377_Disable,
    .Config                 = IMX377_Config,
    .GetStatus              = IMX377_GetStatus,
    .GetModeInfo            = IMX377_GetModeInfo,
    .GetDeviceInfo          = IMX377_GetDeviceInfo,
    .GetCurrentGainFactor   = IMX377_GetCurrentGainFactor,
    .GetCurrentShutterSpeed = IMX377_GetCurrentShutterSpeed,

    .ConvertGainFactor      = IMX377_ConvertGainFactor,
    .ConvertShutterSpeed    = IMX377_ConvertShutterSpeed,

    .SetAnalogGainCtrl      = IMX377_SetAnalogGainCtrl,
    .SetDigitalGainCtrl     = IMX377_SetDigitalGainCtrl,
    .SetShutterCtrl         = IMX377_SetShutterCtrl,
    .SetSlowShutterCtrl     = IMX377_SetSlowShutterCtrl,

    .RegisterRead           = IMX377_RegisterRead,
    .RegisterWrite          = IMX377_RegisterWrite,
};
