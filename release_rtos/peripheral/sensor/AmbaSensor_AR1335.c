/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_AR1335.c
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of Aptina AR1335 CMOS sensor with MIPI interface
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
#include "AmbaSensor_AR1335.h"

#include "AmbaPrintk.h"

/*-----------------------------------------------------------------------------------------------*\
 * AR1335 sensor runtime info
\*-----------------------------------------------------------------------------------------------*/
AR1335_CTRL_s AR1335Ctrl = {0};

/*-----------------------------------------------------------------------------------------------*\
 * Configurations for Amba. VIN side
\*-----------------------------------------------------------------------------------------------*/
static AMBA_DSP_VIN_MIPI_CONFIG_s AR1335VinConfig = {
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
        .BayerPattern       = AMBA_DSP_BAYER_GR,
        .NumDataBits        = 0,
        .NumSkipFrame       = 1,
    },
    .DataTypeMask           = 0x0,
    .DataType               = AMBA_DSP_VIN_MIPI_RAW10,
    .NumActiveLanes         = 4,
    .RxHvSyncCtrl = {
        .NumActivePixels    = 0,
        .NumActiveLines     = 0,
        .NumTotalPixels     = 0,
        .NumTotalLines      = 0
    },

    .VinVoutSync = {
        {
            .SignalFreq     = AMBA_DSP_VIN_VOUT_SYNC_FRAME,
            .SignalLine     = 1
        },
        {
            .SignalFreq     = AMBA_DSP_VIN_VOUT_SYNC_FRAME,
            .SignalLine     = 1
        }
    },

    .MipiCtrl  = {
        .HsSettleTime       = 32,
        .HsTermTime         = 16,
        .ClkSettleTime      = 32,
        .ClkTermTime        = 8,
        .ClkMissTime        = 8,
        .RxInitTime         = 64,
    }
};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_PrepareModeInfo
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
static void AR1335_PrepareModeInfo(AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo)
{
    UINT16 SensorMode                             = Mode.Bits.Mode;
    AR1335_READOUT_MODE_e           ReadoutMode   = AR1335ModeInfoList[SensorMode].ReadoutMode;
    const AR1335_FRAME_TIMING_s     *pFrameTiming = &AR1335ModeInfoList[SensorMode].FrameTiming;
    const AMBA_SENSOR_OUTPUT_INFO_s *pOutputInfo  = &AR1335OutputInfo[ReadoutMode];
    const AMBA_SENSOR_INPUT_INFO_s  *pInputInfo;

    if (Mode.Bits.VerticalFlip == 0)
        pInputInfo = &AR1335InputInfoNormalReadout[ReadoutMode];
    else if (Mode.Bits.VerticalFlip == 1)
        pInputInfo = &AR1335InputInfoInversionReadout[ReadoutMode];

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
 *  @RoutineName:: AR1335_ConfigVin
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
static void AR1335_ConfigVin(AMBA_SENSOR_MODE_INFO_s *pModeInfo, AR1335_FRAME_TIMING_s *pFrameTime)
{
    AMBA_SENSOR_OUTPUT_INFO_s  *pImgInfo = &pModeInfo->OutputInfo;
    AMBA_DSP_VIN_MIPI_CONFIG_s *pVinCfg = &AR1335VinConfig;

    pVinCfg->NumActiveLanes = pImgInfo->NumDataLanes;
    pVinCfg->Info.NumDataBits = pImgInfo->NumDataBits;
    pVinCfg->Info.BayerPattern = (AMBA_DSP_SENSOR_PATTERN_e) pImgInfo->CfaPattern;

    pVinCfg->RxHvSyncCtrl.NumActivePixels  = pImgInfo->OutputWidth;
    pVinCfg->RxHvSyncCtrl.NumActiveLines   = pImgInfo->OutputHeight;
    pVinCfg->RxHvSyncCtrl.NumTotalPixels   = pModeInfo->LineLengthPck - 1;
    pVinCfg->RxHvSyncCtrl.NumTotalLines    = pModeInfo->FrameLengthLines;

    memcpy(&pVinCfg->Info.FrameRate, &pFrameTime->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    memcpy(&pVinCfg->Info.DspPhaseShift, &pImgInfo->DspPhaseShift, sizeof(AMBA_DSP_PHASE_SHIFT_CTRL_s));

    AmbaDSP_VinConfigMIPI(AMBA_VIN_CHANNEL0, pVinCfg);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_HardwareReset
 *
 *  @Description:: Reset AR1335 Image Sensor Device
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static inline void AR1335_HardwareReset(void)
{
    AmbaUserGPIO_SensorResetCtrl(0);
    AmbaKAL_TaskSleep(1);               /* XCLR Low level pulse width >= 100ns */
    AmbaUserGPIO_SensorResetCtrl(1);
}

static int AR1335_RegRead(UINT16 Addr, UINT8 *pRxData, int Size)
{
    int i, RetStatus;
    UINT16 TxData[4];
    UINT8  RxData[128];

    TxData[0] = AMBA_I2C_RESTART_FLAG | (0x6C);
    TxData[1] = (Addr >> 8);    /* Register Address [15:8] */
    TxData[2] = (Addr & 0xFF);  /* Register Address [7:0]  */
    TxData[3] = AMBA_I2C_RESTART_FLAG | (0x6D);

    RetStatus = AmbaI2C_ReadAfterWrite(AMBA_I2C_CHANNEL0,
                                       AMBA_I2C_SPEED_FAST,
                                       4, (UINT16 *) TxData,
                                       Size, RxData,
                                       AMBA_KAL_WAIT_FOREVER);
    for (i = 0; i < Size; i++)
        *pRxData++ = RxData[i];

    return RetStatus;
}

static int AR1335_RegWrite(UINT16 Addr, UINT8 *pTxData, int Size)
{
    int i;
    UINT8 TxData[128];

    TxData[0] = (UINT8) (Addr >> 8);
    TxData[1] = (UINT8) (Addr & 0xFF);

    for (i = 0; i < Size; i++) {
        TxData[i + 2] = pTxData[i];
    }

    if (AmbaI2C_Write(AMBA_I2C_CHANNEL0,
                      AMBA_I2C_SPEED_FAST,
                      0x6C, Size + 2, TxData,
                      AMBA_KAL_WAIT_FOREVER) != OK) {
        AmbaPrint("I2C does not work!!!!!");
        return NG;
    }

#if 0 // for debugging
    {
        UINT8 RxData[128];

        AR1335_RegRead(Addr, RxData, Size);
        for (i = 0; i < Size; i++, Addr++) {
            AmbaPrint("[AR1335] Addr = 0x%04x, Data = 0x%02X, Rd = 0x%02X", Addr, pTxData[i], RxData[i]);
        }
    }
#endif

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_RegisterRead
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
static int AR1335_RegisterRead(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 *pData)
{
    UINT8 RxData[2];

    AR1335_RegRead(Addr, RxData, 2);
    *pData = ((UINT16) (RxData[0]) << 8) | RxData[1];

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_RegisterWrite
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
static int AR1335_RegisterWrite(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 Data)
{
    UINT8 TxData[2];

    TxData[0] = (UINT8) (Data >> 8);
    TxData[1] = (UINT8) (Data & 0xFF);

    AR1335_RegWrite(Addr, TxData, 2);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetAnalogGainReg
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
static void AR1335_SetAnalogGainReg(UINT16 PGC)
{
#ifndef AR1335_ANA_DIG_GAIN_DIRECT
    UINT8 TxData[2];

    TxData[0] = (UINT8) (PGC >> 8);
    TxData[1] = (UINT8) (PGC & 0xFF);

    AR1335_RegWrite(AR1335_ANA_GAIN_GLOBAL_REG, TxData, 2);
#else
#ifdef AR1335_ANA_DIG_GAIN_GLOBAL
    UINT8 TxData[2];

    TxData[0] = (UINT8) (PGC >> 8);
    TxData[1] = (UINT8) (PGC & 0xFF);

    AR1335_RegWrite(AR1335_GLOBAL_GAIN_REG, TxData, 2);
#else
    int i;
    UINT8 TxData[8];

    for (i = 0; i < 4; i++) {
        TxData[i*2]   = (UINT8) (PGC >> 8);
        TxData[i*2+1] = (UINT8) (PGC & 0xFF);
    }

    AR1335_RegWrite(AR1335_GAIN_GB_REG, TxData, 8);
#endif
#endif
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetDigitalGainReg
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
#ifndef AR1335_ANA_DIG_GAIN_DIRECT
static void AR1335_SetDigitalGainReg(UINT16 DGC)
{
    int i;
    UINT8 TxData[8];

    for (i = 0; i < 4; i++) {
        TxData[i*2]   = (UINT8) (DGC >> 8);
        TxData[i*2+1] = (UINT8) (DGC & 0xFF);
    }

    AR1335_RegWrite(AR1335_DIG_GAIN_GR_REG, TxData, 8);
}
#endif
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetShutterReg
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
static void AR1335_SetShutterReg(UINT32 NumLinesEshrSpeed)
{
    UINT8 TxData[2];
    UINT16 Data = (UINT16) NumLinesEshrSpeed;

    TxData[0] = (UINT8) (Data >> 8);
    TxData[1] = (UINT8) (Data & 0xFF);

    AR1335_RegWrite(AR1335_COARSE_INTEG_TIME_REG, TxData, 2);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetSlowShutterReg
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
static void AR1335_SetSlowShutterReg(UINT32 ExposureFrames)
{
    AR1335_FRAME_TIMING_s *pFrameTime = &AR1335Ctrl.FrameTime;
    UINT32 TargetFrameLengthLines = pFrameTime->FrameLengthLines * ExposureFrames;
    UINT8  TxData[2];

    TxData[0] = (UINT8) (TargetFrameLengthLines >> 8);
    TxData[1] = (UINT8) (TargetFrameLengthLines & 0xFF);

    AR1335_RegWrite(AR1335_FRAME_LENGTH_REG, TxData, 2);

    AR1335Ctrl.Status.ModeInfo.FrameTime.FrameRate.NumUnitsInTick =
        AR1335ModeInfoList[AR1335Ctrl.Status.ModeInfo.Mode.Bits.Mode].FrameTiming.FrameRate.NumUnitsInTick * ExposureFrames;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetStandbyOn
 *
 *  @Description:: Configure sensor into standby mode
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void AR1335_SetStandbyOn(void)
{
    UINT8 RegData[2];

    RegData[0] = 0x00;
    RegData[1] = 0x02;
    AR1335_RegWrite(0x3F3C, RegData, 2);

    RegData[0] = 0x00;
    RegData[1] = 0x01;
    AR1335_RegWrite(0x3FE0, RegData, 2);

    RegData[0] = 0x00;
    AR1335_RegWrite(AR1335_MODE_SELECT_REG, RegData, 1);

    RegData[0] = 0x00;
    RegData[1] = 0x00;
    AR1335_RegWrite(0x3FE0, RegData, 2);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetStandbyOff
 *
 *  @Description:: Configure sensor to leave standby mode
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void AR1335_SetStandbyOff(void)
{
    UINT8 RegData[2];

    RegData[0] = 0x00;
    RegData[1] = 0x03;
    AR1335_RegWrite(0x3F3C, RegData, 2);

    RegData[0] = 0x65; /* no_bad_frame_by_frame_length_change (0x3044-5[13]) */
    RegData[1] = 0x80;
    AR1335_RegWrite(0x3044, RegData, 2);

    RegData[0] = 0x01; /* model select: streaming (alias 0x301A-B[2]) */
    AR1335_RegWrite(AR1335_MODE_SELECT_REG, RegData, 1);

    RegData[0] = 0x40; /* gain insert all frame (0x301A-B[14]) */
    RegData[1] = 0x1C; /* streaming mode (0x301A-B[2]) */
    AR1335_RegWrite(0x301A, RegData, 2);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_InitRegAfterReset
 *
 *  @Description:: init register of sensor after reset.
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int AR1335_InitRegAfterReset(void)
{
    int i;
    UINT16 Data;
    UINT8 TxData[2];

    for (i = 0; i < AR1335_NUM_INIT_REG; i++) {
        Data = AR1335InitRegTable[i].Data;
        TxData[0] = (UINT8) (Data >> 8);
        TxData[1] = (UINT8) (Data & 0xFF);
        AR1335_RegWrite(AR1335InitRegTable[i].Addr, TxData, 2);
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_ChangeReadoutMode
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
static int AR1335_ChangeReadoutMode(AR1335_READOUT_MODE_e ReadoutMode, AR1335_FRAME_TIMING_s *pFrameTime)
{
    int i;
    UINT16 Data;
    UINT8 TxData[4];

    /* change readout mode */
    for (i = 0; i < AR1335_NUM_READOUT_MODE_REG; i++) {
        Data = AR1335ModeRegTable[i].Data[ReadoutMode];
        TxData[0] = (UINT8) (Data >> 8);
        TxData[1] = (UINT8) (Data & 0xFF);
        AR1335_RegWrite(AR1335ModeRegTable[i].Addr, TxData, 2);
    }

    /* settle the fps */
    Data = pFrameTime->FrameLengthLines;

    TxData[0] = (UINT8) (Data >> 8);
    TxData[1] = (UINT8) (Data & 0xFF);

    Data = pFrameTime->LineLengthPck;

    TxData[2] = (UINT8) (Data >> 8);
    TxData[3] = (UINT8) (Data & 0xFF);

    AR1335_RegWrite(AR1335_FRM_LENGTH_REG, TxData, 4);

    /* reset gain/shutter ctrl information */
    AR1335Ctrl.ShutterCtrl = 0xFFFFFFFF;
    AR1335Ctrl.CurrentAgc  = 0xFFFFFFFF;
    AR1335Ctrl.CurrentDgc  = 0xFFFFFFFF;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_Init
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
static int AR1335_Init(AMBA_DSP_CHANNEL_ID_u Chan)
{
    const AR1335_FRAME_TIMING_s *pFrameTime = &AR1335ModeInfoList[0].FrameTiming;
    AmbaPLL_SetSensorClk(pFrameTime->InputClk);             /* The default sensor input clock frequency */
    AmbaPLL_SetSensorClkDir(AMBA_PLL_SENSOR_CLK_OUTPUT);    /* Config clock output to sensor (or sensor I2C/SPI won't work) */

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_Enable
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
static int AR1335_Enable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (!AR1335Ctrl.Status.DevStat.Bits.Sensor0Power) {
#if 0
        /* Turn On the Sensor Power if applicable */
        AmbaUserGPIO_SensorPowerCtrl(1);    /* Turn On the Sensor Power */
#endif
        /* enable internal pull-down for CLK_SI and master sync */
        // AmbaVIN_SetSensorPullCtrl(AMBA_VIN_SENSOR_PULL_DOWN);

        AR1335_HardwareReset();

        /* config mipi phy */
        AmbaDSP_VinPhySetMIPI(Chan.Bits.VinID);

        /* the Sensor Power is ON, Sensor is at Standby mode */
        AR1335Ctrl.Status.DevStat.Bits.Sensor0Power =
            AR1335Ctrl.Status.DevStat.Bits.Sensor0Standby = 1;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_Disable
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
static int AR1335_Disable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (AR1335Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return OK;

    AR1335_SetStandbyOn();
    AR1335Ctrl.Status.DevStat.Bits.Sensor0Standby = 1;  /* Sensor is at Standby mode */

#if 0
    /* Turn Off the Sensor Power if applicable */
    AmbaUserGPIO_SensorPowerCtrl(0);    /* Turn Off the Sensor Power */
    AR1335Ctrl.Status.DevStat.Bits.Sensor0Power = 0;
#endif

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_GetStatus
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
static int AR1335_GetStatus(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_STATUS_INFO_s *pStatus)
{
    if (pStatus == NULL || AR1335Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return NG;

    AR1335Ctrl.Status.TimeToAllPelExposed = 0;  /* TODO: for flash light or mechanical shutter control */

    memcpy(pStatus, &AR1335Ctrl.Status, sizeof(AMBA_SENSOR_STATUS_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_GetModeInfo
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
static int AR1335_GetModeInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo)
{
    if (Mode.Bits.Mode == AMBA_SENSOR_CURRENT_MODE)
        Mode.Bits.Mode = AR1335Ctrl.Status.ModeInfo.Mode.Bits.Mode;

    if (Mode.Bits.Mode >= AMBA_SENSOR_AR1335_NUM_MODE || pModeInfo == NULL)
        return NG;

    AR1335_PrepareModeInfo(Mode, pModeInfo);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_GetDeviceInfo
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
static int AR1335_GetDeviceInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_DEVICE_INFO_s *pDeviceInfo)
{
    if (pDeviceInfo == NULL)
        return NG;

    memcpy(pDeviceInfo, &AR1335DeviceInfo, sizeof(AMBA_SENSOR_DEVICE_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_GetCurrentGainFactor
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
static int AR1335_GetCurrentGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float *pGainFactor)
{
#ifndef AR1335_ANA_DIG_GAIN_DIRECT
    *pGainFactor = (AR1335Ctrl.CurrentAgc/4.)*((AR1335Ctrl.CurrentDgc >> 2)/64.);
#else
    *pGainFactor = (1 << (((AR1335Ctrl.CurrentAgc >> 4) & 0x07) - 1))*((16. + (AR1335Ctrl.CurrentAgc & 0x0F))/16.)*((AR1335Ctrl.CurrentDgc >> 2)/64.);
#endif

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_GetCurrentShutterSpeed
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
static int AR1335_GetCurrentShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float *pExposureTime)
{
    *pExposureTime = (AR1335Ctrl.Status.ModeInfo.RowTime * (float) AR1335Ctrl.ShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_ConvertGainFactor
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
#define AMBA_SENSOR_DGC_DSP    0 /* for debug only */
static int AR1335_ConvertGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float DesiredFactor, UINT32 *pActualFactor, UINT32 *pAnalogGainCtrl, UINT32 *pDigitalGainCtrl)
{
    if (pActualFactor == NULL || pAnalogGainCtrl == NULL || pDigitalGainCtrl == NULL)
        return NG;

    if (DesiredFactor < 1.0) {
#ifndef AR1335_ANA_DIG_GAIN_DIRECT
        *pAnalogGainCtrl = 0x04;
#else
        *pAnalogGainCtrl = 0x10;
#endif
#if AMBA_SENSOR_DGC_DSP
        *pDigitalGainCtrl = (UINT32) (DesiredFactor*4096);
#else
        *pDigitalGainCtrl = 0x40 << 2;
#ifdef AR1335_ANA_DIG_GAIN_DIRECT
        *pAnalogGainCtrl = *pAnalogGainCtrl | ((*pDigitalGainCtrl) << 5);
#endif
#endif
        *pActualFactor = 1 << 20; /* .20 fixed point */
    } else {
        int i;
        float AnalogGain1;
        float AnalogGain2;
        float DigitalGain;
        float ActualGain;

        if  (DesiredFactor < 7.75) {
            for (i = 1; i < 4; i++) {
                if (DesiredFactor < (1 << i)) {
                    break;
                }
            }
            AnalogGain1 = 1 << (i - 1);
#if 1
            AnalogGain2 = 1 + ((((UINT32) (DesiredFactor/AnalogGain1*16 - 16)) >> (3 - i)) << (3 - i))/16.;
#else
            AnalogGain2 = 1 + ((UINT32) (DesiredFactor/AnalogGain1*16 - 16))/16.;
#endif
#ifndef AR1335_ANA_DIG_GAIN_DIRECT
            *pAnalogGainCtrl = (UINT32) (AnalogGain1*AnalogGain2*4); /* 3.2 format */
#else
            *pAnalogGainCtrl = (i << 4) | ((UINT32) ((AnalogGain2 - 1)*16));
#endif
        } else {
            AnalogGain1 = 4;
            AnalogGain2 = 1.9375;
#ifndef AR1335_ANA_DIG_GAIN_DIRECT
            *pAnalogGainCtrl = (UINT32) (AnalogGain1*AnalogGain2*4); /* 3.2 format */
#else
            *pAnalogGainCtrl = 0x3F;
#endif
        }

#if AMBA_SENSOR_DGC_DSP
        DigitalGain = ((UINT32) (DesiredFactor/(AnalogGain1*AnalogGain2)*4096))/4096.;
        *pDigitalGainCtrl = (UINT32) (DigitalGain*4096);
#ifdef AR1335_ANA_DIG_GAIN_DIRECT
        /* directly write to 0x305E */
        *pAnalogGainCtrl = *pAnalogGainCtrl | (0x40 << 7);
#endif
#else
        DigitalGain = ((UINT32) (DesiredFactor/(AnalogGain1*AnalogGain2)*64))/64.;
        *pDigitalGainCtrl = ((UINT32) (DigitalGain*64)) << 2;
#ifdef AR1335_ANA_DIG_GAIN_DIRECT
        /* directly write to 0x305E instead of 0x3028(analog), 0x3032 ~ 0x3038(digital) */
        *pAnalogGainCtrl = *pAnalogGainCtrl | ((*pDigitalGainCtrl) << 5);
#endif
#endif
        ActualGain = AnalogGain1*AnalogGain2*DigitalGain;

        *pActualFactor = (UINT32) (ActualGain * 1024 * 1024); /* .20 fixed point */
    }

    //AmbaPrint("DesireFactor:%f, ActualFactor:%u, AnalogGainCtrl:0x%04X, DigitalGainCtrl: 0x%04X",
    //           DesiredFactor, *pActualFactor, *pAnalogGainCtrl, *pDigitalGainCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_ConvertShutterSpeed
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
static int AR1335_ConvertShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float ExposureTime, UINT32 *pShutterCtrl)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo = &AR1335Ctrl.Status.ModeInfo;
    const AMBA_DSP_FRAME_RATE_s *pFrameRate = &AR1335ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming.FrameRate;
    UINT32 NumExposureStepPerFrame = AR1335ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming.FrameLengthLines;
    UINT32 ExposureFrames, ExposureTimeMaxMargin;

    if (pShutterCtrl == NULL)
        return NG;

    /*---------------------------------------------------------------------------------*\
     * Number of Exposure Steps = (ExposureTime / FrameTime) * NumExposureStepPerFrame
     *                          = ExposureTime * FrameRate * NumExposureStepPerFrame
    \*---------------------------------------------------------------------------------*/
    *pShutterCtrl = (UINT32) (ExposureTime * pFrameRate->TimeScale / pFrameRate->NumUnitsInTick * NumExposureStepPerFrame);

    ExposureFrames = (*pShutterCtrl - 1) / NumExposureStepPerFrame;
    ExposureFrames ++;
    ExposureTimeMaxMargin = ExposureFrames * NumExposureStepPerFrame - 1;
    if (*pShutterCtrl > ExposureTimeMaxMargin) {
        *pShutterCtrl = ExposureTimeMaxMargin;
    }

    if (*pShutterCtrl < 8) {
        *pShutterCtrl = 8;
    }

    // AmbaPrint("ExposureTime:%f, ShutterCtrl:0x%04X", ExposureTime, *pShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetAnalogGainCtrl
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
static int AR1335_SetAnalogGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 AnalogGainCtrl)
{
    AR1335_SetAnalogGainReg(AnalogGainCtrl);

    AR1335Ctrl.CurrentAgc = AnalogGainCtrl;

    // AmbaPrint("Set Again: 0x%04X", AnalogGainCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetDigitalGainCtrl
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
static int AR1335_SetDigitalGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 DigitalGainCtrl)
{
#ifndef AR1335_ANA_DIG_GAIN_DIRECT
    AR1335_SetDigitalGainReg(DigitalGainCtrl);
#endif
    AR1335Ctrl.CurrentDgc = DigitalGainCtrl;

    // AmbaPrint("Set Dgain: 0x%04X", DigitalGainCtrl);

    return OK;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetShutterCtrl
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
static int AR1335_SetShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 ShutterCtrl)
{
    AR1335_SetShutterReg(ShutterCtrl);

    AR1335Ctrl.ShutterCtrl = ShutterCtrl;

    // AmbaPrint("Set Shutter: 0x%04X", ShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_SetSlowShutterCtrl
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
static int AR1335_SetSlowShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 SlowShutterCtrl)
{
    AR1335_SetSlowShutterReg(SlowShutterCtrl);

    // AmbaPrint("Set SlowShutter: 0x%04X", SlowShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AR1335_Config
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
static int AR1335_Config(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_ESHUTTER_TYPE_e ElecShutterMode)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo;
    UINT16 SensorMode = Mode.Bits.Mode;
    AR1335_READOUT_MODE_e ReadoutMode = AR1335ModeInfoList[SensorMode].ReadoutMode;
    AMBA_VIN_CHANNEL_e VinChanNo = (AMBA_VIN_CHANNEL_e) Chan.Bits.VinID;

    if (SensorMode >= AMBA_SENSOR_AR1335_NUM_MODE)
        return NG;

    if (Mode.Bits.DummyMasterSync == 1)
        return OK;

    if ((AR1335Ctrl.Status.ModeInfo.Mode.Bits.Mode == SensorMode) &&
        (AR1335Ctrl.Status.ModeInfo.Mode.Bits.VerticalFlip != Mode.Bits.VerticalFlip) &&
        (AR1335Ctrl.Status.ElecShutterMode == ElecShutterMode) &&
        (AR1335Ctrl.Status.DevStat.Bits.Sensor0Power) &&
        (!AR1335Ctrl.Status.DevStat.Bits.Sensor0Standby)) {
        /* run-time flip sensor readout direction */
        /* TODO */
        UINT8 TxData = Mode.Bits.VerticalFlip ? 2 : 0;
        AR1335_RegWrite(AR1335_IMG_ORIENTATION_REG, &TxData, 1);

        /* update status */
        pModeInfo = &AR1335Ctrl.Status.ModeInfo;
        AR1335_PrepareModeInfo(Mode, pModeInfo);
    } else {
        UINT8 TxData = 0;
        AR1335_FRAME_TIMING_s *pFrameTime = &AR1335Ctrl.FrameTime;

        /* update status */
        AR1335Ctrl.Status.ElecShutterMode = ElecShutterMode;

        pModeInfo = &AR1335Ctrl.Status.ModeInfo;
        AR1335_PrepareModeInfo(Mode, pModeInfo);
        memcpy(pFrameTime, &AR1335ModeInfoList[SensorMode].FrameTiming, sizeof(AR1335_FRAME_TIMING_s));

        AR1335VinConfig.NumActiveLanes = pModeInfo->OutputInfo.NumDataLanes;

        AmbaVIN_CalculateMphyConfig(pModeInfo->OutputInfo.DataRate, &AR1335VinConfig.MipiCtrl);
        AmbaVIN_Reset(VinChanNo, AMBA_VIN_MIPI, &AR1335VinConfig.MipiCtrl);

        /* set pll */
        AmbaPLL_SetSensorClk(pModeInfo->FrameTime.InputClk);
        AmbaPrintColor(RED, "sensor input clock = %d(%d)", pModeInfo->FrameTime.InputClk, AmbaPLL_GetSensorClk());
        AmbaKAL_TaskSleep(2);

        AR1335_HardwareReset();
        AmbaKAL_TaskSleep(6);

        /* write init reg table after hardware reset */
        AR1335_InitRegAfterReset();

        /* write registers of mode change to sensor */
        AR1335_ChangeReadoutMode(ReadoutMode, pFrameTime);

        /* config sensor readout direction */
        TxData = Mode.Bits.VerticalFlip ? 2 : 0;
        AR1335_RegWrite(AR1335_IMG_ORIENTATION_REG, &TxData, 1);

        AR1335_SetSlowShutterCtrl(Chan, 1);
        AR1335_SetShutterCtrl(Chan, pModeInfo->NumExposureStepPerFrame - 1);
#ifndef AR1335_ANA_DIG_GAIN_DIRECT
        AR1335_SetDigitalGainCtrl(Chan, 0x40 << 2);
        AR1335_SetAnalogGainCtrl(Chan, 0x08);
#else
        AR1335_SetAnalogGainCtrl(Chan, 0x20 | 0x40 << 7);
#endif
        AR1335_SetStandbyOff();

        /* config vin */
        AR1335_ConfigVin(pModeInfo, pFrameTime);

        AR1335Ctrl.Status.DevStat.Bits.Sensor0Standby = 0;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*/
AMBA_SENSOR_OBJ_s AmbaSensor_AR1335Obj = {
    .Init                   = AR1335_Init,
    .Enable                 = AR1335_Enable,
    .Disable                = AR1335_Disable,
    .Config                 = AR1335_Config,
    .GetStatus              = AR1335_GetStatus,
    .GetModeInfo            = AR1335_GetModeInfo,
    .GetDeviceInfo          = AR1335_GetDeviceInfo,
    .GetCurrentGainFactor   = AR1335_GetCurrentGainFactor,
    .GetCurrentShutterSpeed = AR1335_GetCurrentShutterSpeed,

    .ConvertGainFactor      = AR1335_ConvertGainFactor,
    .ConvertShutterSpeed    = AR1335_ConvertShutterSpeed,

    .SetAnalogGainCtrl      = AR1335_SetAnalogGainCtrl,
    .SetDigitalGainCtrl     = AR1335_SetDigitalGainCtrl,
    .SetShutterCtrl         = AR1335_SetShutterCtrl,
    .SetSlowShutterCtrl     = AR1335_SetSlowShutterCtrl,

    .RegisterRead           = AR1335_RegisterRead,
    .RegisterWrite          = AR1335_RegisterWrite,
};
