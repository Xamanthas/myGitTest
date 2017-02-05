/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_IMX317.c
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of SONY IMX317 CMOS sensor with LVDS interface
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaKAL.h"

#include "AmbaDSP.h"
#include "AmbaVIN.h"
#include "AmbaDSP_VIN.h"

#include "AmbaSPI.h"
#include "AmbaGPIO.h"
#include "AmbaPLL.h"
#include "AmbaSensor.h"
#include "AmbaSensor_IMX317.h"

#include "AmbaPrintk.h"

/*-----------------------------------------------------------------------------------------------*\
 * IMX317 sensor runtime info
\*-----------------------------------------------------------------------------------------------*/
IMX317_CTRL_s IMX317Ctrl = {0};
UINT8 IMX317SpiBuf[2][128];

/*-----------------------------------------------------------------------------------------------*\
 * Configurations for Amba. VIN side
\*-----------------------------------------------------------------------------------------------*/
static AMBA_DSP_VIN_SLVS_PIN_PAIR_e IMX317LvdsLaneMux[][AMBA_DSP_NUM_VIN_SLVS_DATA_LANE] = {
    [IMX317_LVDS_10CH]  = {
        AMBA_DSP_VIN_PIN_SD_LVDS_0,
        AMBA_DSP_VIN_PIN_SD_LVDS_1,
        AMBA_DSP_VIN_PIN_SD_LVDS_2,
        AMBA_DSP_VIN_PIN_SD_LVDS_8,
        AMBA_DSP_VIN_PIN_SD_LVDS_3,
        AMBA_DSP_VIN_PIN_SD_LVDS_4,
        AMBA_DSP_VIN_PIN_SD_LVDS_9,
        AMBA_DSP_VIN_PIN_SD_LVDS_5,
        AMBA_DSP_VIN_PIN_SD_LVDS_6,
        AMBA_DSP_VIN_PIN_SD_LVDS_7
    },

    [IMX317_LVDS_8CH]   = {
        AMBA_DSP_VIN_PIN_SD_LVDS_0,
        AMBA_DSP_VIN_PIN_SD_LVDS_1,
        AMBA_DSP_VIN_PIN_SD_LVDS_2,
        AMBA_DSP_VIN_PIN_SD_LVDS_3,
        AMBA_DSP_VIN_PIN_SD_LVDS_4,
        AMBA_DSP_VIN_PIN_SD_LVDS_5,
        AMBA_DSP_VIN_PIN_SD_LVDS_6,
        AMBA_DSP_VIN_PIN_SD_LVDS_7
    },

    [IMX317_LVDS_6CH]   = {
        AMBA_DSP_VIN_PIN_SD_LVDS_1,
        AMBA_DSP_VIN_PIN_SD_LVDS_2,
        AMBA_DSP_VIN_PIN_SD_LVDS_3,
        AMBA_DSP_VIN_PIN_SD_LVDS_4,
        AMBA_DSP_VIN_PIN_SD_LVDS_5,
        AMBA_DSP_VIN_PIN_SD_LVDS_6
    },

    [IMX317_LVDS_4CH]   = {
        AMBA_DSP_VIN_PIN_SD_LVDS_2,
        AMBA_DSP_VIN_PIN_SD_LVDS_3,
        AMBA_DSP_VIN_PIN_SD_LVDS_4,
        AMBA_DSP_VIN_PIN_SD_LVDS_5
    },

    [IMX317_LVDS_2CH]   = {
        AMBA_DSP_VIN_PIN_SD_LVDS_3,
        AMBA_DSP_VIN_PIN_SD_LVDS_4
    },

    [IMX317_LVDS_1CH]   = {
        AMBA_DSP_VIN_PIN_SD_LVDS_3
    },
};

static AMBA_DSP_VIN_SLVS_CONFIG_s IMX317VinConfig = {
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
    .pLaneMapping   = NULL,
    .NumActiveLanes = 0,
    .SyncDetectCtrl = {
        .SyncInterleaving   = 0,
        .ITU656Type         = 0,
        .CustomSyncCode     = {
            .PatternAlign   = 1,        /* MSB */
            .SyncCodeMask   = 0xff00,
            .DetectEnable   = {
                .Sol    =  1,
                .Eol    =  1,
                .Sof    =  0,
                .Eof    =  0,
                .Sov    =  1,
                .Eov    =  1,
            },
            .PatternSol = 0x8000,
            .PatternEol = 0x9d00,
            .PatternSof = 0,
            .PatternEof = 0,
            .PatternSov = 0xab00,
            .PatternEov = 0xb600,
        },
    },
    .SplitCtrl = {
        .NumSplits = 0,
        .SplitWidth = 0,
    },
    .RxHvSyncCtrl   = {0, 0, 0, 0},
    .VinTrigPulse   = { {0}, {0} },
    .VinVoutSync    = { {AMBA_DSP_VIN_VOUT_SYNC_FIELD, 5}, {AMBA_DSP_VIN_VOUT_SYNC_FIELD, 5} }
};

static AMBA_DSP_VIN_MASTER_SYNC_CONFIG_s IMX317MasterSyncConfig = {
    .HSync = {
        .Period     = 0,
        .PulseWidth = 0,
        .Polarity   = 0
    },
    .VSync = {
        .Period     = 0,
        .PulseWidth = 0,
        .Polarity   = 0
    },
    .VSyncDelayCycles = 0,
    .ToggleHsyncInVblank = 1
};

/*-----------------------------------------------------------------------------------------------*\
 * SPI configuration, {SpiChanNo, SpiSlaveId, SpiMode, CsPol, DataFrmSize, BaudRate}
\*-----------------------------------------------------------------------------------------------*/
static AMBA_SPI_CONFIG_s IMX317SpiConfig = {
    .SlaveID       = AMBA_SENSOR_SPI_SLAVE_ID,          /* Slave ID */
    .ProtocolMode  = AMBA_SPI_CPOL_HIGH_CPHA_HIGH,      /* SPI Protocol mode */
    .CsPolarity    = AMBA_SPI_CHIP_SELECT_ACTIVE_LOW,   /* Slave select polarity */
    .DataFrameSize = 8,                                 /* Data Frame Size in Bit */
    .BaudRate      = 10000000                           /* Transfer BaudRate in Hz */
};

/*-----------------------------------------------------------------------------------------------*\
 * Reverse bit order in a byte for IMX317 SPI communication
\*-----------------------------------------------------------------------------------------------*/
static inline UINT8 IMX317_SpiReverseBitOrder(UINT16 Data)
{
    UINT8 DataOut = ((Data & 0x01) << 7) |
                    ((Data & 0x02) << 5) |
                    ((Data & 0x04) << 3) |
                    ((Data & 0x08) << 1) |
                    ((Data & 0x10) >> 1) |
                    ((Data & 0x20) >> 3) |
                    ((Data & 0x40) >> 5) |
                    ((Data & 0x80) >> 7);

    return DataOut;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_PrepareModeInfo
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
static void IMX317_PrepareModeInfo(AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo)
{
    UINT16 SensorMode                             = Mode.Bits.Mode;
    IMX317_READOUT_MODE_e           ReadoutMode   = IMX317ModeInfoList[SensorMode].ReadoutMode;
    const IMX317_FRAME_TIMING_s     *pFrameTiming = &IMX317ModeInfoList[SensorMode].FrameTiming;
    const AMBA_SENSOR_OUTPUT_INFO_s *pOutputInfo  = &IMX317OutputInfo[ReadoutMode];
    const AMBA_SENSOR_INPUT_INFO_s  *pInputInfo;

    if (Mode.Bits.VerticalFlip == 0)
        pInputInfo = &IMX317InputInfoNormalReadout[ReadoutMode];
    else if (Mode.Bits.VerticalFlip == 1)
        pInputInfo = &IMX317InputInfoInversionReadout[ReadoutMode];

    pModeInfo->Mode = Mode;
    pModeInfo->LineLengthPck = (pFrameTiming->NumTickPerXhs * pFrameTiming->NumXhsPerH) *
                               (pOutputInfo->DataRate / pFrameTiming->InputClk) *
                               (pOutputInfo->NumDataLanes) / (pOutputInfo->NumDataBits);
    pModeInfo->FrameLengthLines = pFrameTiming->NumXhsPerV / pFrameTiming->NumXhsPerH;
    pModeInfo->NumExposureStepPerFrame = pFrameTiming->NumXhsPerV;
    pModeInfo->PixelRate = (float) pOutputInfo->DataRate * pOutputInfo->NumDataLanes / pOutputInfo->NumDataBits;
    pModeInfo->RowTime = (float) (pFrameTiming->NumTickPerXhs * pFrameTiming->NumXhsPerH) / pFrameTiming->InputClk;
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
 *  @RoutineName:: IMX317_ConfigVin
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
static void IMX317_ConfigVin(AMBA_SENSOR_MODE_INFO_s *pModeInfo, IMX317_FRAME_TIMING_s *pFrameTime)
{
    AMBA_SENSOR_OUTPUT_INFO_s  *pImgInfo = &pModeInfo->OutputInfo;
    const IMX317_FRAME_TIMING_s *pImx317FrameTiming = &IMX317ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming;
    AMBA_DSP_VIN_SLVS_CONFIG_s *pVinCfg = &IMX317VinConfig;
    AMBA_DSP_VIN_MASTER_SYNC_CONFIG_s *pMasterSyncCfg = &IMX317MasterSyncConfig;

    AmbaDSP_VinPhySetSLVS(AMBA_VIN_CHANNEL0);

    memcpy(&pVinCfg->Info.FrameRate, &pFrameTime->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    memcpy(&pVinCfg->Info.DspPhaseShift, &pImgInfo->DspPhaseShift, sizeof(AMBA_DSP_PHASE_SHIFT_CTRL_s));
    pVinCfg->Info.BayerPattern = (AMBA_DSP_SENSOR_PATTERN_e) pImgInfo->CfaPattern;
    pVinCfg->Info.NumDataBits = pImgInfo->NumDataBits;

    pVinCfg->NumActiveLanes = pImgInfo->NumDataLanes;
    switch (pVinCfg->NumActiveLanes) {
    case 10:
        pVinCfg->pLaneMapping = IMX317LvdsLaneMux[IMX317_LVDS_10CH];
        break;
    case 8:
        pVinCfg->pLaneMapping = IMX317LvdsLaneMux[IMX317_LVDS_8CH];
        break;
    case 6:
        pVinCfg->pLaneMapping = IMX317LvdsLaneMux[IMX317_LVDS_6CH];
        break;
    case 4:
        pVinCfg->pLaneMapping = IMX317LvdsLaneMux[IMX317_LVDS_4CH];
        break;
    case 2:
        pVinCfg->pLaneMapping = IMX317LvdsLaneMux[IMX317_LVDS_2CH];
        break;
    case 1:
    default:
        pVinCfg->pLaneMapping = IMX317LvdsLaneMux[IMX317_LVDS_1CH];
        break;
    }
    pVinCfg->RxHvSyncCtrl.NumActivePixels = pImgInfo->OutputWidth;
    pVinCfg->RxHvSyncCtrl.NumActiveLines  = pImgInfo->OutputHeight;
    pVinCfg->RxHvSyncCtrl.NumTotalPixels  = pModeInfo->LineLengthPck - 1;
    pVinCfg->RxHvSyncCtrl.NumTotalLines   = pModeInfo->FrameLengthLines;

    AmbaDSP_VinConfigSLVS(AMBA_VIN_CHANNEL0, pVinCfg);

    pMasterSyncCfg->HSync.Period = pImx317FrameTiming->NumTickPerXhs;
    pMasterSyncCfg->HSync.PulseWidth = 8;
    pMasterSyncCfg->VSync.Period = pImx317FrameTiming->NumXhsPerV / pImx317FrameTiming->NumXvsPerV;
    pMasterSyncCfg->VSync.PulseWidth = 8;

    AmbaDSP_VinConfigMasterSync(AMBA_VIN_CHANNEL0, pMasterSyncCfg);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_HardwareReset
 *
 *  @Description:: Reset IMX317 Image Sensor Device
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static inline void IMX317_HardwareReset(void)
{
    AmbaUserGPIO_SensorResetCtrl(0);
    AmbaKAL_TaskSleep(1);               /* XCLR Low level pulse width >= 100ns */
    AmbaUserGPIO_SensorResetCtrl(1);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_RegRW
 *
 *  @Description:: Read/Write sensor registers through SPI bus
 *
 *  @Input      ::
 *      Addr:       Register Address
 *      pTxData:    Pointer to Write data buffer
 *      pRxData:    Pointer to Echo data buffer
 *      Size:       Number of Read/Write data
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static int IMX317_RegRW(UINT16 Addr, UINT8 *pTxData, UINT8 *pRxData, UINT8 Size)
{

    UINT32  i;
    UINT8   *pDataBuf;

    if ((Size + 3) > sizeof(IMX317SpiBuf[0]))
        return NG;

    pDataBuf = &IMX317SpiBuf[0][0];
    *pDataBuf++ = IMX317_SpiReverseBitOrder(0x81);         /* Chip ID */
    *pDataBuf++ = IMX317_SpiReverseBitOrder(Addr >> 8);    /* Register Address (high byte) */
    *pDataBuf++ = IMX317_SpiReverseBitOrder(Addr & 0xff);  /* Register Address (low byte) */

    for (i = 0; i < Size; i++) {
        *pDataBuf++ = IMX317_SpiReverseBitOrder(pTxData[i]);
    }

    AmbaSPI_Transfer(AMBA_SPI_MASTER, &IMX317SpiConfig, Size + 3, IMX317SpiBuf[0], IMX317SpiBuf[1], 1000);
#if 0 //for debug
    AmbaSPI_Transfer(AMBA_SPI_MASTER, &IMX317SpiConfig, Size + 3, IMX317SpiBuf[0], IMX317SpiBuf[1], 1000);

    for (i = 0; i < Size; i++) {
        AmbaPrint("Wr %04Xh %02Xh, Rd %02Xh", Addr + i, pTxData[i], IMX317_SpiReverseBitOrder(IMX317SpiBuf[1][3 + i]));
    }
#endif
    if (pRxData != NULL) {
        for (i = 0; i < Size; i++) {
            *pRxData++ = IMX317_SpiReverseBitOrder(IMX317SpiBuf[1][3 + i]);
        }
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_RegisterWrite
 *
 *  @Description:: Write sensor registers through SPI bus
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
static int IMX317_RegisterWrite(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 Data)
{
    UINT8 TxData[1];

    TxData[0] = (UINT8) Data;
    IMX317_RegRW(Addr, TxData, NULL, 1);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_RegisterRead
 *
 *  @Description:: Read sensor registers through SPI bus
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *      Addr:   Register address
 *
 *  @Output     ::
 *      pData:   Pointer to Read data buffer
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int IMX317_RegisterRead(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 *pData)
{
    UINT8 TxData[1];
    UINT8 RxData[1];

    TxData[0] = 0x00;
    IMX317_RegRW(Addr, TxData, RxData, 1);
    TxData[0] = RxData[0];
    IMX317_RegRW(Addr, TxData, NULL, 1);

    *pData = RxData[0];

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetAnalogGainReg
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
static void IMX317_SetAnalogGainReg(UINT16 PGC)
{
    UINT8 TxData[2];

    TxData[0] = PGC & 0x00ff;
    TxData[1] = (PGC >> 8) & 0x00ff;
    IMX317_RegRW(IMX317_PGC_LSB_REG, TxData, NULL, 2);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetDigitalGainReg
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
static void IMX317_SetDigitalGainReg(UINT8 DGC)
{
    UINT8 TxData[1];

    TxData[0] = DGC;
    IMX317_RegRW(IMX317_DGAIN_REG, TxData, NULL, 1);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetShutterReg
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
static UINT32 IMX317_IntegrationPeriodInXVS = 1;  /* current intergration period (unit: XVS) */
static void IMX317_SetShutterReg(UINT32 NumXhsEshrSpeed)
{
    UINT32 NumXhsPerXvs = IMX317Ctrl.FrameTime.NumXhsPerV / IMX317Ctrl.FrameTime.NumXvsPerV;
    UINT32 SVR; /* specifies the exposure end vertical period */
    UINT32 SHR; /* specifies the exposure start horizontal period */
    UINT8  TxData[2];

    SVR = IMX317_IntegrationPeriodInXVS - 1;
    SHR = (SVR + 1) * NumXhsPerXvs - NumXhsEshrSpeed;

    /* Since IMX317 doesn't support SPL, SHR should
       not be greater than 0xffff */
    if (SHR > 0xffff)
        AmbaPrint("[IMX317] SHR (%u) should not be greater than 0xffff.", SHR);

    TxData[0] = SHR & 0x00ff;
    TxData[1] = (SHR >> 8) & 0x00ff;
    IMX317_RegRW(IMX317_SHR_LSB_REG, TxData, TxData, 2);

    IMX317Ctrl.CurrentShrCtrlSHR = SHR;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetSlowShutterReg
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
static void IMX317_SetSlowShutterReg(UINT32 IntegrationPeriodInFrame)
{
    IMX317_FRAME_TIMING_s *pFrameTime = &IMX317Ctrl.FrameTime;
    UINT32 SVR = 0;
    UINT8  TxData[2];

    /* 1 frame might includes more than 1 XVS */
    IMX317_IntegrationPeriodInXVS = IntegrationPeriodInFrame * pFrameTime->NumXvsPerV;

    SVR = IMX317_IntegrationPeriodInXVS - 1;

    TxData[0] = SVR & 0x00ff;
    TxData[1] = (SVR >> 8) & 0x00ff;
    IMX317_RegRW(IMX317_SVR_LSB_REG, TxData, NULL, 2);

    IMX317Ctrl.CurrentShrCtrlSVR = SVR;
    IMX317Ctrl.Status.ModeInfo.FrameTime.FrameRate.NumUnitsInTick =
        IMX317ModeInfoList[IMX317Ctrl.Status.ModeInfo.Mode.Bits.Mode].FrameTiming.FrameRate.NumUnitsInTick * IntegrationPeriodInFrame;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetStandbyOn
 *
 *  @Description:: Configure sensor into standby mode
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void IMX317_SetStandbyOn(void)
{
    UINT8 TxData[1];

    TxData[0] = 0x13;
    IMX317_RegRW(IMX317_OP_REG, TxData, NULL, 1);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetReadoutPlusTimingRegs
 *
 *  @Description:: Set all registers of PLSTMG settings
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
static int IMX317_SetReadoutPlusTimingRegs(IMX317_READOUT_MODE_e ReadoutMode, IMX317_FRAME_TIMING_s *pFrameTime)
{
    int i;
    UINT32 XVS;
    UINT8 TxData[4];

    XVS = pFrameTime->NumXhsPerV / pFrameTime->NumXvsPerV;

    TxData[0] = ((XVS - 0x0a) & 0xff);
    TxData[1] = ((XVS - 0x0a) & 0xff00) >> 8;
    IMX317_RegRW(IMX317_PSLVDS1_LSB_REG, &TxData[0], NULL, 2);
    IMX317_RegRW(IMX317_PSLVDS2_LSB_REG, &TxData[0], NULL, 2);
    IMX317_RegRW(IMX317_PSLVDS3_LSB_REG, &TxData[0], NULL, 2);
    IMX317_RegRW(IMX317_PSLVDS4_LSB_REG, &TxData[0], NULL, 2);

    for (i = 0; i < IMX317_NUM_READOUT_PSTMG_REG; i++) {
        TxData[0] = IMX317PlstmgRegTable[i].Data;
        IMX317_RegRW(IMX317PlstmgRegTable[i].Addr, TxData, NULL, 1);
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_ChangeReadoutMode
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
static int IMX317_ChangeReadoutMode(IMX317_READOUT_MODE_e ReadoutMode, IMX317_FRAME_TIMING_s *pFrameTime)
{
    int i;
    UINT8 TxData[4];

    for (i = 0; i < IMX317_NUM_READOUT_MODE_REG; i++) {
        TxData[0] = IMX317ModeRegTable[i].Data[ReadoutMode];
        IMX317_RegRW(IMX317ModeRegTable[i].Addr, TxData, NULL, 1);
    }

    /* reset gain/shutter ctrl information */
    IMX317Ctrl.CurrentShrCtrlSHR = 0xffffffff;
    IMX317Ctrl.CurrentShrCtrlSVR = 0xffffffff;
    IMX317Ctrl.CurrentAgc = 0xffffffff;
    IMX317Ctrl.CurrentDgc = 0xffffffff;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_Init
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
static int IMX317_Init(AMBA_DSP_CHANNEL_ID_u Chan)
{
    const IMX317_FRAME_TIMING_s *pFrameTime = &IMX317ModeInfoList[0].FrameTiming;
    AmbaPLL_SetSensorClk(pFrameTime->InputClk);             /* The default sensor input clock frequency */
    AmbaPLL_SetSensorClkDir(AMBA_PLL_SENSOR_CLK_OUTPUT);    /* Config clock output to sensor (or sensor SPI won't work) */

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_Enable
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
static int IMX317_Enable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (!IMX317Ctrl.Status.DevStat.Bits.Sensor0Power) {
#if 0
        /* Turn On the Sensor Power if applicable */
        AmbaUserGPIO_SensorPowerCtrl(1);    /* Turn On the Sensor Power */
#endif
        /* enable internal pull-down for CLK_SI and master sync */
        // AmbaVIN_SetSensorPullCtrl(AMBA_VIN_SENSOR_PULL_DOWN);

        /* the Sensor Power is ON, Sensor is at Standby mode */
        IMX317Ctrl.Status.DevStat.Bits.Sensor0Power =
            IMX317Ctrl.Status.DevStat.Bits.Sensor0Standby = 1;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_Disable
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
static int IMX317_Disable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (IMX317Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return OK;

    IMX317_SetStandbyOn();
    IMX317Ctrl.Status.DevStat.Bits.Sensor0Standby = 1;  /* Sensor is at Standby mode */

#if 0
    /* Turn Off the Sensor Power if applicable */
    AmbaUserGPIO_SensorPowerCtrl(0);    /* Turn Off the Sensor Power */
    IMX317Ctrl.Status.DevStat.Bits.Sensor0Power = 0;
#endif

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_GetStatus
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
static int IMX317_GetStatus(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_STATUS_INFO_s *pStatus)
{
    if (pStatus == NULL || IMX317Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return NG;

    IMX317Ctrl.Status.TimeToAllPelExposed = 0;  /* TODO: for flash light or mechanical shutter control */

    memcpy(pStatus, &IMX317Ctrl.Status, sizeof(AMBA_SENSOR_STATUS_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_GetModeInfo
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
static int IMX317_GetModeInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo)
{
    if (Mode.Bits.Mode == AMBA_SENSOR_CURRENT_MODE)
        Mode.Bits.Mode = IMX317Ctrl.Status.ModeInfo.Mode.Bits.Mode;

    if (Mode.Bits.Mode >= AMBA_SENSOR_IMX317_NUM_MODE || pModeInfo == NULL)
        return NG;

    IMX317_PrepareModeInfo(Mode, pModeInfo);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_GetDeviceInfo
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
static int IMX317_GetDeviceInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_DEVICE_INFO_s *pDeviceInfo)
{
    if (pDeviceInfo == NULL)
        return NG;

    memcpy(pDeviceInfo, &IMX317DeviceInfo, sizeof(AMBA_SENSOR_DEVICE_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_GetCurrentGainFactor
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
static int IMX317_GetCurrentGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float *pGainFactor)
{
    *pGainFactor = 2048. / (2048 - IMX317Ctrl.CurrentAgc) * (1 << IMX317Ctrl.CurrentDgc);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_GetCurrentShutterSpeed
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
static int IMX317_GetCurrentShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float *pExposureTime)
{
    *pExposureTime = (IMX317Ctrl.Status.ModeInfo.RowTime / IMX317Ctrl.FrameTime.NumXhsPerH) *
                     ((IMX317Ctrl.CurrentShrCtrlSVR + 1) * (IMX317Ctrl.FrameTime.NumXhsPerV / IMX317Ctrl.FrameTime.NumXvsPerV) - IMX317Ctrl.CurrentShrCtrlSHR);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_ConvertGainFactor
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
static int IMX317_ConvertGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float DesiredFactor, UINT32 *pActualFactor, UINT32 *pAnalogGainCtrl, UINT32 *pDigitalGainCtrl)
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

    // AmbaPrint("DesireFactor:%f, ActualFactor:%u, AnalogGainCtrl:0x%04X, DigitalGainCtrl: 0x%04X",
    //           DesiredFactor, *pActualFactor, *pAnalogGainCtrl, *pDigitalGainCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_ConvertShutterSpeed
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
static int IMX317_ConvertShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float ExposureTime, UINT32 *pShutterCtrl)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo = &IMX317Ctrl.Status.ModeInfo;
    const AMBA_DSP_FRAME_RATE_s *pFrameRate = &IMX317ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming.FrameRate;
    IMX317_READOUT_MODE_e ReadoutMode = IMX317ModeInfoList[pModeInfo->Mode.Bits.Mode].ReadoutMode;
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

    if (IMX317Ctrl.Status.ElecShutterMode == AMBA_SENSOR_ESHUTTER_TYPE_GLOBAL_RESET) {
        MinExpXhsNum = 130;
        MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame;
    } else {
        switch (ReadoutMode) {
        case IMX317_TYPE_2_5_MODE_C:
        case IMX317_TYPE_2_5_MODE_0:
        case IMX317_TYPE_2_5_MODE_1:
            MinExpXhsNum = 4;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 12;
            break;

        case IMX317_TYPE_2_5_MODE_2:
        case IMX317_TYPE_2_5_MODE_3:
        case IMX317_TYPE_2_5_MODE_4:
            MinExpXhsNum = 4;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 8;
            break;

        case IMX317_TYPE_2_5_MODE_5:
            MinExpXhsNum = 4;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 8;
            break;

        case IMX317_TYPE_2_5_MODE_6:
            MinExpXhsNum = 2;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 4;
            break;

        default:
            MinExpXhsNum = 4;
            MaxExpXhsNum = ((ExpXhsNum -1) / pModeInfo->NumExposureStepPerFrame + 1) * pModeInfo->NumExposureStepPerFrame - 12;
            break;
        }
    }

    if (ExpXhsNum > MaxExpXhsNum)
        *pShutterCtrl = MaxExpXhsNum;
    else if (ExpXhsNum < MinExpXhsNum)
        *pShutterCtrl = MinExpXhsNum;
    else
        *pShutterCtrl = ExpXhsNum;

    // AmbaPrint("ExposureTime:%f, ShutterCtrl:0x%04X", ExposureTime, *pShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetAnalogGainCtrl
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
static int IMX317_SetAnalogGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 AnalogGainCtrl)
{
    IMX317_SetAnalogGainReg(AnalogGainCtrl);

    IMX317Ctrl.CurrentAgc = AnalogGainCtrl;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetDigitalGainCtrl
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
static int IMX317_SetDigitalGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 DigitalGainCtrl)
{
    IMX317_SetDigitalGainReg(DigitalGainCtrl);

    IMX317Ctrl.CurrentDgc = DigitalGainCtrl;

    return OK;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetShutterCtrl
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
static int IMX317_SetShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 ShutterCtrl)
{
    IMX317_SetShutterReg(ShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_SetSlowShutterCtrl
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
static int IMX317_SetSlowShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 SlowShutterCtrl)
{
    IMX317_SetSlowShutterReg(SlowShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: IMX317_Config
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
static int IMX317_Config(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_ESHUTTER_TYPE_e ElecShutterMode)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo;
    UINT16 SensorMode = Mode.Bits.Mode;
    IMX317_READOUT_MODE_e ReadoutMode = IMX317ModeInfoList[SensorMode].ReadoutMode;
    // AMBA_VIN_CHANNEL_e VinChanNo = (AMBA_VIN_CHANNEL_e) Chan.Bits.VinID;

    if (SensorMode >= AMBA_SENSOR_IMX317_NUM_MODE)
        return NG;

    if (Mode.Bits.DummyMasterSync == 1)
        return OK;

    if ((IMX317Ctrl.Status.ModeInfo.Mode.Bits.Mode == SensorMode) &&
        (IMX317Ctrl.Status.ModeInfo.Mode.Bits.VerticalFlip != Mode.Bits.VerticalFlip) &&
        (IMX317Ctrl.Status.ElecShutterMode == ElecShutterMode) &&
        (IMX317Ctrl.Status.DevStat.Bits.Sensor0Power) &&
        (!IMX317Ctrl.Status.DevStat.Bits.Sensor0Standby)) {
        /* run-time flip sensor readout direction */
        UINT8 TxData = Mode.Bits.VerticalFlip;
        IMX317_RegRW(IMX317_MDVREV_REG, &TxData, NULL, 1);

        /* update status */
        pModeInfo = &IMX317Ctrl.Status.ModeInfo;
        IMX317_PrepareModeInfo(Mode, pModeInfo);
    } else {
        UINT8 TxData = 0;
        IMX317_FRAME_TIMING_s *pFrameTime = &IMX317Ctrl.FrameTime;

        /* update status */
        IMX317Ctrl.Status.ElecShutterMode = ElecShutterMode;

        pModeInfo = &IMX317Ctrl.Status.ModeInfo;
        IMX317_PrepareModeInfo(Mode, pModeInfo);
        memcpy(pFrameTime, &IMX317ModeInfoList[SensorMode].FrameTiming, sizeof(IMX317_FRAME_TIMING_s));

        /* reset master sync*/
        AmbaVIN_ResetHvSyncOutput((AMBA_VIN_CHANNEL_e) Chan.Bits.VinID);
        /* Make VIN not to capture bad frames during readout mode transition */
        AmbaVIN_Reset((AMBA_VIN_CHANNEL_e) Chan.Bits.VinID, AMBA_VIN_LVDS, NULL);

        AmbaPLL_SetSensorClk(pModeInfo->FrameTime.InputClk);
        AmbaPLL_SetSoVinClk(pModeInfo->FrameTime.InputClk);
        AmbaKAL_TaskSleep(1);

        IMX317_HardwareReset();
        AmbaKAL_TaskSleep(1);

        /* ----------------------------- */
        /* [Analog stabilization period] */
        /* ----------------------------- */
        /* 1st stabilization period */
        TxData = 0x1a;
        IMX317_RegRW(IMX317_OP_REG, &TxData, NULL, 1);
        TxData = 0x00;
        IMX317_RegRW(IMX317_SYS_MODE_REG, &TxData, NULL, 1);

        TxData = 0x80;
        IMX317_RegRW(IMX317_PLRD1_LSB_REG, &TxData, NULL, 1);
        TxData = 0x00;
        IMX317_RegRW(IMX317_PLRD1_MSB_REG, &TxData, NULL, 1);
        TxData = 0x03;
        IMX317_RegRW(IMX317_PLRD2_REG, &TxData, NULL, 1);
        TxData = 0x68;
        IMX317_RegRW(IMX317_PLRD3_REG, &TxData, NULL, 1);
        TxData = 0x03;
        IMX317_RegRW(IMX317_PLRD4_REG, &TxData, NULL, 1);
        TxData = 0x02;
        IMX317_RegRW(IMX317_PLRD5_REG, &TxData, NULL, 1);
        TxData = 0x00;
        IMX317_RegRW(IMX317_STBPL_IF_AD_REG, &TxData, NULL, 1);

        /* initialize communication */
        TxData = 0x01;
        IMX317_RegRW(IMX317_PSMOVEN_REG, &TxData, NULL, 1);

        IMX317_SetReadoutPlusTimingRegs(ReadoutMode, pFrameTime);

        AmbaKAL_TaskSleep(10);

        /* 2nd stabilization period */
        TxData = 0x01;
        IMX317_RegRW(IMX317_PLL_CKEN_REG, &TxData, NULL, 1);
        TxData = 0x01;
        IMX317_RegRW(IMX317_PACKEN_REG, &TxData, NULL, 1);
        TxData = 0x18;
        IMX317_RegRW(IMX317_OP_REG, &TxData, NULL, 1);

        AmbaKAL_TaskSleep(7);

        /* ------------------------- */
        /* [Standby cancel sequence] */
        /* ------------------------- */
        TxData = 0x10;
        IMX317_RegRW(IMX317_CLPSQRST_REG, &TxData, NULL, 1);
        TxData = 0x01;
        IMX317_RegRW(IMX317_DCKRST_REG, &TxData, NULL, 1);

        IMX317_ChangeReadoutMode(ReadoutMode, pFrameTime);

        /* config sensor readout direction */
        TxData = Mode.Bits.VerticalFlip;
        IMX317_RegRW(IMX317_MDVREV_REG, &TxData, NULL, 1);

        IMX317_SetSlowShutterReg(1);
        IMX317_SetShutterReg(pModeInfo->NumExposureStepPerFrame >> 1);
        IMX317_SetAnalogGainCtrl(Chan, 0);
        IMX317_SetDigitalGainCtrl(Chan, 0);

        AmbaKAL_TaskSleep(34);

        /* config vin */
        IMX317_ConfigVin(pModeInfo, pFrameTime);

        AmbaVIN_Reset((AMBA_VIN_CHANNEL_e) Chan.Bits.VinID, AMBA_VIN_LVDS, NULL);

        IMX317Ctrl.Status.DevStat.Bits.Sensor0Standby = 0;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*/
AMBA_SENSOR_OBJ_s AmbaSensor_IMX317Obj = {
    .Init                   = IMX317_Init,
    .Enable                 = IMX317_Enable,
    .Disable                = IMX317_Disable,
    .Config                 = IMX317_Config,
    .GetStatus              = IMX317_GetStatus,
    .GetModeInfo            = IMX317_GetModeInfo,
    .GetDeviceInfo          = IMX317_GetDeviceInfo,
    .GetCurrentGainFactor   = IMX317_GetCurrentGainFactor,
    .GetCurrentShutterSpeed = IMX317_GetCurrentShutterSpeed,

    .ConvertGainFactor      = IMX317_ConvertGainFactor,
    .ConvertShutterSpeed    = IMX317_ConvertShutterSpeed,

    .SetAnalogGainCtrl      = IMX317_SetAnalogGainCtrl,
    .SetDigitalGainCtrl     = IMX317_SetDigitalGainCtrl,
    .SetShutterCtrl         = IMX317_SetShutterCtrl,
    .SetSlowShutterCtrl     = IMX317_SetSlowShutterCtrl,

    .RegisterRead           = IMX317_RegisterRead,
    .RegisterWrite          = IMX317_RegisterWrite,
};
