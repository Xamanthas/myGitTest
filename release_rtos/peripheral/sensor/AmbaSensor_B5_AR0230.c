/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaSensor_B5_AR0230.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Control APIs of OmniVision B5_AR0230 CMOS sensor with MIPI interface
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "AmbaDataType.h"
#include "AmbaKAL.h"
#include "AmbaPrintk.h"
#include "AmbaINT.h"
#include "AmbaI2C.h"

#include "AmbaDSP_VIN.h"
#include "AmbaVIN.h"

#include "AmbaGPIO.h"
#include "AmbaPLL.h"
#include "AmbaSensor.h"
#include "AmbaSensor_B5_AR0230.h"
#include "AmbaINT.h"

#include "AmbaB5.h"
#include "AmbaB5_Communicate.h"
#include "AmbaB5_PLL.h"
#include "AmbaB5_VIN.h"
#include "AmbaB5_Prescaler.h"
#include "AmbaB5_VOUT.h"
#include "AmbaB5_SPI.h"
#include "AmbaB5_I2C.h"
#include "AmbaB5_GPIO.h"

#define AMBA_B5_AHB_BASE_ADDR                       0xe0000000
#define AMBA_B5_APB_BASE_ADDR                       0xe8000000

/*-----------------------------------------------------------------------------------------------*\
 * B5_AR0230 sensor runtime info
\*-----------------------------------------------------------------------------------------------*/
B5_AR0230_CTRL_s B5_AR0230Ctrl = {0};
AMBA_SENSOR_WB_GAIN_s B5_AR0230DesiredWBGainFactor = {1.0, 1.0, 1.0, 1.0};

static AMBA_B5_CHANNEL_s B5_AR0230_SysChan;
static AMBA_B5_CHANNEL_s B5_AR0230_CurChan;
static AMBA_B5_PIN_CONFIG_s B5_AR0230_PinConfig = {
    .B5nPinMux = AMBA_B5_PIN_B5N_CTRL_SPI,
    .B5fPinMux = AMBA_B5_PIN_B5F_CTRL_PWM_SINGLE_ENDED,
    .SensorPinMux = AMBA_B5_PIN_SENSOR_CTRL_I2C1,
    .VideoSyncPinMux = AMBA_B5_PIN_VIDEO_SYNC_NONE,
    .SlvsEcType = AMBA_B5_SLVS_EC_3LANE,
};
static AMBA_B5_COMPRESS_RATIO_e B5_AR0230_CompressRatio = AMBA_B5_COMPRESS_NONE;
extern AMBA_KAL_SEM_t AmbaB5_PllGetSwPllSem(void);
static AMBA_KAL_SEM_t SensorAeSem;

/*-----------------------------------------------------------------------------------------------*\
 * Lane mapping for SoC side VIN (receiving video from B5N)
\*-----------------------------------------------------------------------------------------------*/
static AMBA_DSP_VIN_SLVS_PIN_PAIR_e B5_LvdsLaneMux[AMBA_DSP_NUM_VIN_SLVS_DATA_LANE] = {
    AMBA_DSP_VIN_PIN_SD_LVDS_0,
    AMBA_DSP_VIN_PIN_SD_LVDS_1,
    AMBA_DSP_VIN_PIN_SD_LVDS_2,
    AMBA_DSP_VIN_PIN_SD_LVDS_3,
    AMBA_DSP_VIN_PIN_SD_LVDS_4,
    AMBA_DSP_VIN_PIN_SD_LVDS_5,
    AMBA_DSP_VIN_PIN_SD_LVDS_6,
    AMBA_DSP_VIN_PIN_SD_LVDS_7,
    AMBA_DSP_VIN_PIN_SD_LVDS_8,
    AMBA_DSP_VIN_PIN_SD_LVDS_9
};

#if 0
/*-----------------------------------------------------------------------------------------------*\
 * Lane mapping for B5F side VIN (receiving video from B5F)
\*-----------------------------------------------------------------------------------------------*/
static AMBA_DSP_VIN_SLVS_PIN_PAIR_e B5_AR0230LvdsLaneMux[][AMBA_DSP_NUM_VIN_SLVS_DATA_LANE] = {
    [B5_AR0230_LVDS_4CH]    = {
        AMBA_DSP_VIN_PIN_SD_LVDS_0,
        AMBA_DSP_VIN_PIN_SD_LVDS_1,
        AMBA_DSP_VIN_PIN_SD_LVDS_2,
        AMBA_DSP_VIN_PIN_SD_LVDS_3
    },

};
#endif
/*-----------------------------------------------------------------------------------------------*\
 * Configurations for SoC side VIN (receiving video from B5N)
\*-----------------------------------------------------------------------------------------------*/
static AMBA_DSP_VIN_SLVS_CONFIG_s B5_VinConfig = {
    .Info = {
        .FrameRate = {
            .Interlace      = 0,
            .TimeScale      = 0,
            .NumUnitsInTick = 0,
        },
        .DspPhaseShift      = {
            .Horizontal     = AMBA_DSP_PHASE_SHIFT_MODE_0,
            .Vertical       = AMBA_DSP_PHASE_SHIFT_MODE_0,
        },
        .BayerPattern       = AMBA_DSP_BAYER_BG,
        .NumDataBits        = 0,
        .NumSkipFrame       = 1,
    },
    .pLaneMapping   = NULL,
    .NumActiveLanes = 0,
    .SyncDetectCtrl = {
        .SyncInterleaving   = 0,
        .ITU656Type         = 0,
        .CustomSyncCode     = {
            .PatternAlign   = 1, /* MSB */
            .SyncCodeMask   = 0xff00,
            .DetectEnable   = {
                .Sol    =  1,
                .Eol    =  0,
                .Sof    =  0,
                .Eof    =  0,
                .Sov    =  1,
                .Eov    =  0,
             },
            .PatternSol = 0x8000,
            .PatternEol = 0,
            .PatternSof = 0,
            .PatternEof = 0,
            .PatternSov = 0xab00,
            .PatternEov = 0,
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

/*-----------------------------------------------------------------------------------------------*\
 * Configurations for B5 side VIN (receiving video from Aptina AR0230)
\*-----------------------------------------------------------------------------------------------*/
static AMBA_B5_VIN_SLVS_CONFIG_s B5_AR0230_SlvsVinConfig = {
    .NumDataBits        = 0,
    .NumDataLane        = 0,
    .DataLaneSelect     = { 0, 1, 2, 3 },
    .SyncDetectCtrl = {
        .SyncInterleaving    = 2,
        .ITU656Type         = 0,
        .CustomSyncCode     = {
            .PatternAlign    = 1,        /* MSB */
            .SyncCodeMask    = 0xff00,
            .DetectEnable    = {
                .Sol    =  1,
                .Eol    =  0,
                .Sof    =  0,
                .Eof    =  0,
                .Sov    =  1,
                .Eov    =  0,
            },
            .PatternSol = 0x8000,
            .PatternEol = 0,
            .PatternSof = 0,
            .PatternEof = 0,
            .PatternSov = 0xab00,
            .PatternEov = 0,
        },
    },
    .SplitCtrl = {
        .NumSplits = 0,
        .SplitWidth = 0,
    },
    .TxHvSyncCtrl       = { 0 },
    .RxHvSyncCtrl       = { 0 },
    .VinTrigPulse       = { {0}, {0} },
    .VinVoutSync        = { {AMBA_B5_VIN_VOUT_SYNC_FIELD, 5}, {AMBA_B5_VIN_VOUT_SYNC_FIELD, 5} },
};

/*-----------------------------------------------------------------------------------------------*\
 * Vout configuration
\*-----------------------------------------------------------------------------------------------*/
static AMBA_B5_VOUT_CONFIG_s B5_VoutConfig = {
    .MaxHBlank = 0,
    .NumDataLane = 0,
    .PixelWidth = 0,
    .InputLinePixel = 0,
    .OutputFrameLine = 0,
    .MinHBlank = 100,
    .MinVBlank = 10,    /* larger value will cause voutf fifo overflow (resulted in top visual errors) */
};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_B5N_GPIO_Set
 *
 *  @Description:: Set B5N GPIO pin to output heigh
 *
 *  @Input      ::
 *      PinNum:    GPIO pin number
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void B5_AR0230_B5N_GPIO_Set(UINT32 PinNum)
{
    UINT32 DataBuf[1];
    UINT32 Addr;

    DataBuf[0] = 0x1;
    Addr = AMBA_B5_APB_BASE_ADDR + 0x102c;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = (UINT32) (1 << PinNum);
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1028;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    Addr = AMBA_B5_APB_BASE_ADDR + 0x1004;
    AmbaB5N_RegRead(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);
    DataBuf[0] = DataBuf[0] | (UINT32) (1 << PinNum);
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = (UINT32) (1 << PinNum); // set 1
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1000;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0xffffffff;
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1024;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_B5N_GPIO_Clr
 *
 *  @Description:: Set B5N GPIO pin to output low
 *
 *  @Input      ::
 *      PinNum:    GPIO pin number
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void B5_AR0230_B5N_GPIO_Clr(UINT32 PinNum)
{
    UINT32 DataBuf[1];
    UINT32 Addr;

    DataBuf[0] = 0x1;
    Addr = AMBA_B5_APB_BASE_ADDR + 0x102c;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = (UINT32) (1 << PinNum);
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1028;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    Addr = AMBA_B5_APB_BASE_ADDR + 0x1004;
    AmbaB5N_RegRead(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);
    DataBuf[0] = DataBuf[0] | (UINT32) (1 << PinNum);
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = (UINT32) (0 << PinNum); // set 0
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1000;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0xffffffff;
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1024;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_B5F_GPIO_Set
 *
 *  @Description:: Set B5F GPIO pin to output heigh
 *
 *  @Input      ::
 *      ChipID:    B5F ChipID
 *      PinNum:    GPIO pin number
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void B5_AR0230_B5F_GPIO_Set(AMBA_B5_CHIP_ID_u ChipID, UINT32 PinNum)
{
    UINT32 DataBuf[1];
    UINT32 Addr;

    ChipID.Data = ChipID.Data & 0xf;

    DataBuf[0] = 0x1;
    Addr = AMBA_B5_APB_BASE_ADDR + 0x102c;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = (UINT32) (1 << PinNum);
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1028;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    Addr = AMBA_B5_APB_BASE_ADDR + 0x1004;
    AmbaB5_RegRead(ChipID.Data, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);
    DataBuf[0] = DataBuf[0] | (UINT32) (1 << PinNum);
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = (UINT32) (1 << PinNum); // set 1
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1000;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0xffffffff;
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1024;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_B5F_GPIO_Clr
 *
 *  @Description:: Set B5F GPIO pin to output low
 *
 *  @Input      ::
 *      ChipID:    B5F ChipID
 *      PinNum:    GPIO pin number
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void B5_AR0230_B5F_GPIO_Clr(AMBA_B5_CHIP_ID_u ChipID, UINT32 PinNum)
{
    UINT32 DataBuf[1];
    UINT32 Addr;

    ChipID.Data = ChipID.Data & 0xf;

    DataBuf[0] = 0x1;
    Addr = AMBA_B5_APB_BASE_ADDR + 0x102c;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = (UINT32) (1 << PinNum);
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1028;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    Addr = AMBA_B5_APB_BASE_ADDR + 0x1004;
    AmbaB5_RegRead(ChipID.Data, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);
    DataBuf[0] = DataBuf[0] | (UINT32) (1 << PinNum);
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = (UINT32) (0 << PinNum); // set 0
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1000;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0xffffffff;
    Addr = AMBA_B5_APB_BASE_ADDR + 0x1024;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_PrepareModeInfo
 *
 *  @Description:: Get sensor mode information
 *
 *  @Input      ::
 *      SensorCount: B5 ouput channel number
 *      Mode:   Sensor readout mode number
 *
 *  @Output     ::
 *      pModeInfo:  Details of the specified readout mode
 *      pSensorPrivate: Sensor frame time configuration
 *      pSensorHdrInfo: Sensor HDR information
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_PrepareModeInfo(UINT8 SensorCount, UINT16 Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo, B5_AR0230_MODE_INFO_s *pPrivateModeInfo)
{
    const AMBA_SENSOR_INPUT_INFO_s  *pInputInfo= &B5_AR0230InputInfo[B5_AR0230ModeInfoList[Mode].InputMode];
    const AMBA_SENSOR_OUTPUT_INFO_s *pOutputInfo= &B5_AR0230OutputInfo[B5_AR0230ModeInfoList[Mode].OutputMode];

    if (pPrivateModeInfo == NULL)
        pPrivateModeInfo = (B5_AR0230_MODE_INFO_s *) &B5_AR0230ModeInfoList[Mode];
    else
        memcpy(pPrivateModeInfo, &B5_AR0230ModeInfoList[Mode], sizeof(B5_AR0230_MODE_INFO_s));

    //AmbaPrint("OutputWidth = %d ", pPrivateModeInfo->CfaOutputWidth);
    //AmbaPrint("Table OutputWidth = %d, Mode = %d ", B5_AR0230ModeInfoList[Mode].CfaOutputWidth, Mode);
    pPrivateModeInfo->CfaOutputWidth = 1920;

    pModeInfo->Mode.Data = Mode;

    pModeInfo->LineLengthPck = pPrivateModeInfo->FrameTiming.Linelengthpck;
    pModeInfo->FrameLengthLines = pPrivateModeInfo->FrameTiming.FrameLengthLines;

    //AmbaPrint("LLP:%d, FLL:%d",pModeInfo->LineLengthPck,pModeInfo->FrameLengthLines);

    if(B5_AR0230ModeInfoList[Mode].OperationMode == B5_AR0230_OPERATION_MODE_INTERLEAVE)
        pModeInfo->NumExposureStepPerFrame = (B5_AR0230ModeInfoList[Mode].FrameTiming.FrameLengthLines / 2);
    else
        pModeInfo->NumExposureStepPerFrame = B5_AR0230ModeInfoList[Mode].FrameTiming.FrameLengthLines;

    pModeInfo->PixelRate = (float) pOutputInfo->DataRate * pOutputInfo->NumDataLanes / pOutputInfo->NumDataBits;
    pModeInfo->RowTime = (float) pPrivateModeInfo->FrameTiming.FrameRate.NumUnitsInTick / pPrivateModeInfo->FrameTiming.FrameRate.TimeScale / pPrivateModeInfo->FrameTiming.FrameLengthLines;
    pModeInfo->FrameTime.InputClk = pPrivateModeInfo->FrameTiming.InputClk;

    memcpy(&pModeInfo->FrameTime.FrameRate, &pPrivateModeInfo->FrameTiming.FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    memcpy(&pModeInfo->MinFrameRate, &pPrivateModeInfo->FrameTiming.FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    memcpy(&pModeInfo->InputInfo, pInputInfo, sizeof(AMBA_SENSOR_INPUT_INFO_s));
    memcpy(&pModeInfo->OutputInfo, pOutputInfo, sizeof(AMBA_SENSOR_OUTPUT_INFO_s));

    pModeInfo->OutputInfo.DataRate = B5_AR0230_B5nOutputInfo[0].DataRate;
    pModeInfo->OutputInfo.NumDataLanes = (SensorCount << 1);
    pModeInfo->OutputInfo.OutputWidth = SensorCount * pPrivateModeInfo->CfaOutputWidth;
    pModeInfo->OutputInfo.OutputHeight = pModeInfo->OutputInfo.RecordingPixels.Height;
    pModeInfo->OutputInfo.RecordingPixels.StartX = 0;
    pModeInfo->OutputInfo.RecordingPixels.StartY = 0;
    pModeInfo->OutputInfo.RecordingPixels.Width = pModeInfo->OutputInfo.OutputWidth;
    pModeInfo->OutputInfo.RecordingPixels.Height = pModeInfo->OutputInfo.OutputHeight;

    //AmbaPrint("NumDataLanes       = %d ", pModeInfo->OutputInfo.NumDataLanes);
    //AmbaPrint("OutputWidth      = %d ", pModeInfo->OutputInfo.OutputWidth);
    //AmbaPrint("OutputHeight    = %d ", pModeInfo->OutputInfo.OutputHeight);
    //AmbaPrint("RecordingPixels.Width = %d ", pModeInfo->OutputInfo.RecordingPixels.Width);
    //AmbaPrint("RecordingPixels.Height = %d ", pModeInfo->OutputInfo.RecordingPixels.Height);

    /* Only support 3 stage slow shutter */
    if(B5_AR0230ModeInfoList[Mode].HdrType == AMBA_SENSOR_HDR_NONE)
        pModeInfo->MinFrameRate.TimeScale /= 8;

    /* HDR information */
    pModeInfo->HdrInfo.HdrType = B5_AR0230ModeInfoList[Mode].HdrType;
    pModeInfo->HdrInfo.ActiveChannels = B5_AR0230ModeInfoList[Mode].ActiveChannels;
    memcpy(&pModeInfo->HdrInfo.ChannelInfo[0], &(B5_AR0230HdrLongExposureInfo[Mode]), sizeof(AMBA_SENSOR_HDR_CHANNEL_INFO_s));
    memcpy(&pModeInfo->HdrInfo.ChannelInfo[1], &(B5_AR0230HdrLongExposureInfo[Mode]), sizeof(AMBA_SENSOR_HDR_CHANNEL_INFO_s));
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ConfigVin
 *
 *  @Description:: Configure VIN to receieve output frames of the new readout mode
 *
 *  @Input      ::
 *      Chan:       Vin ID and sensor ID
 *      pModeInfo:  Details of the specified readout mode
 *      pFrameTime: Sensor frame time configuration
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_ConfigVin(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_INFO_s *pModeInfo, B5_AR0230_MODE_INFO_s *pPrivateModeInfo)
{
    AMBA_B5_VIN_WINDOW_s B5CropWindow;
    AMBA_SENSOR_OUTPUT_INFO_s *pImgInfo;
    AMBA_B5_VIN_SLVS_CONFIG_s *pB5SlvsVinCfg = &B5_AR0230_SlvsVinConfig;
    AMBA_DSP_VIN_SLVS_CONFIG_s *pVinCfg = &B5_VinConfig;
    UINT32 VoutHBlank;
    UINT32 BorderWidth = 0;  /* TODO */
    UINT32 FrameNum;
    UINT32 LaneNum ;
    UINT32 LvdsPixelNum; /* number of pixels per lane during a row period */

    /* Config B5 side VOUTF */
    pImgInfo = &pModeInfo->OutputInfo;

    FrameNum = pImgInfo->NumDataLanes / 2;  /* TODO: number of voutf input source */
    LaneNum = pImgInfo->NumDataLanes;  /* TODO */
    LvdsPixelNum = (UINT32)((double)pImgInfo->DataRate / pImgInfo->NumDataBits * pModeInfo->RowTime);
    VoutHBlank = LvdsPixelNum - 8 - (UINT32)((pPrivateModeInfo->CfaOutputWidth * FrameNum + BorderWidth * (FrameNum - 1)) / LaneNum + 1);

    B5_VoutConfig.InputLinePixel = pPrivateModeInfo->CfaOutputWidth;
    B5_VoutConfig.OutputFrameLine = pImgInfo->OutputHeight;
    B5_VoutConfig.PixelWidth = pImgInfo->NumDataBits;
    B5_VoutConfig.NumDataLane = AmbaB5_GetNumActiveChannel(&B5_AR0230_CurChan) * 2;  /* TODO */
    B5_VoutConfig.Channel[0] = (B5_AR0230_CurChan.SensorID & 1) ? B5_AR0230_CurChan.Active[0] : B5_AR0230_CurChan.Inactive[0];
    B5_VoutConfig.Channel[1] = (B5_AR0230_CurChan.SensorID & 2) ? B5_AR0230_CurChan.Active[1] : B5_AR0230_CurChan.Inactive[1];
    B5_VoutConfig.Channel[2] = (B5_AR0230_CurChan.SensorID & 4) ? B5_AR0230_CurChan.Active[2] : B5_AR0230_CurChan.Inactive[2];
    B5_VoutConfig.Channel[3] = (B5_AR0230_CurChan.SensorID & 8) ? B5_AR0230_CurChan.Active[3] : B5_AR0230_CurChan.Inactive[3];
    B5_VoutConfig.MinVBlank = pPrivateModeInfo->FrameTiming.FrameLengthLines - pImgInfo->OutputHeight;
    B5_VoutConfig.MinHBlank = 1 + (UINT32)(VoutHBlank / 2.0 - 1);
    B5_VoutConfig.MaxHBlank = (UINT32)((VoutHBlank + LvdsPixelNum) + 1);
    B5_VoutConfig.CompressRatio = B5_AR0230_CompressRatio;

    AmbaB5_VoutConfig(&B5_VoutConfig);
    AmbaB5_VoutEnable();

    /* Config B5 side VIN */
    pImgInfo = (AMBA_SENSOR_OUTPUT_INFO_s *)&B5_AR0230OutputInfo[pPrivateModeInfo->OutputMode];

    pB5SlvsVinCfg->RxHvSyncCtrl.NumActivePixels  = pImgInfo->OutputWidth;
    pB5SlvsVinCfg->RxHvSyncCtrl.NumActiveLines   = pImgInfo->OutputHeight;

    B5CropWindow.StartX = pImgInfo->RecordingPixels.StartX;
    B5CropWindow.StartY = pImgInfo->RecordingPixels.StartY;
    B5CropWindow.EndX   = pImgInfo->RecordingPixels.Width + pImgInfo->RecordingPixels.StartX - 1;
    B5CropWindow.EndY   = pImgInfo->RecordingPixels.Height + pImgInfo->RecordingPixels.StartY - 1;

    pB5SlvsVinCfg->NumDataBits = pImgInfo->NumDataBits;
    pB5SlvsVinCfg->NumDataLane = pImgInfo->NumDataLanes;

    if (pB5SlvsVinCfg->NumDataLane == 4) {
        pB5SlvsVinCfg->SyncDetectCtrl.SyncInterleaving = 2;
    }
    else if (pB5SlvsVinCfg->NumDataLane == 2){
        pB5SlvsVinCfg->SyncDetectCtrl.SyncInterleaving = 1;
    }

    AmbaB5_VinConfigSLVS(&B5_AR0230_CurChan, pB5SlvsVinCfg);

    AmbaB5_VinCaptureConfig(&B5_AR0230_CurChan, &B5CropWindow);

    /* Config SoC side VIN */
    AmbaDSP_VinPhySetSLVS((AMBA_VIN_CHANNEL_e)Chan.Bits.VinID);

    pImgInfo = &pModeInfo->OutputInfo;
    pVinCfg->Info.NumDataBits = pImgInfo->NumDataBits;
    pVinCfg->NumActiveLanes = pImgInfo->NumDataLanes;
    pVinCfg->pLaneMapping = B5_LvdsLaneMux;
    pVinCfg->RxHvSyncCtrl.NumActivePixels = pImgInfo->OutputWidth;
    pVinCfg->RxHvSyncCtrl.NumActiveLines = pImgInfo->OutputHeight;
    pVinCfg->RxHvSyncCtrl.NumTotalPixels = pImgInfo->OutputWidth + B5_VoutConfig.MinHBlank - 1;
    pVinCfg->RxHvSyncCtrl.NumTotalLines = pModeInfo->FrameLengthLines;
    pVinCfg->Info.BayerPattern = (AMBA_DSP_SENSOR_PATTERN_e) pImgInfo->CfaPattern;

    memcpy(&pVinCfg->Info.FrameRate, &pPrivateModeInfo->FrameTiming.FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    memcpy(&pVinCfg->Info.DspPhaseShift, &pImgInfo->DspPhaseShift, sizeof(AMBA_DSP_PHASE_SHIFT_CTRL_s));

    AmbaDSP_VinConfigSLVS((AMBA_VIN_CHANNEL_e)Chan.Bits.VinID, pVinCfg);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_HardwareReset
 *
 *  @Description:: Reset B5_AR0230
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static inline void B5_AR0230_HardwareReset(AMBA_DSP_CHANNEL_ID_u Chan)
{
    /* For Second VIN */
    if (Chan.Bits.VinID == 1) {
        /* B5N GPIO 8 is for B5F power supply */
        /* GPIO_PIN_21 is fot B5N reset */
        B5_AR0230_B5N_GPIO_Clr(30);
        AmbaKAL_TaskSleep(5);
        AmbaGPIO_ConfigOutput(GPIO_PIN_19, 0);
        AmbaKAL_TaskSleep(600);
        AmbaGPIO_ConfigOutput(GPIO_PIN_19, 1);
        AmbaKAL_TaskSleep(20);
        B5_AR0230_B5N_GPIO_Set(30);

    /* For Main VIN */
    } else {
        /* GPIO_PIN_19 is for B5F power supply */
        /* GPIO_PIN_21 is fot B5N reset */
        AmbaGPIO_ConfigOutput(GPIO_PIN_19, 0);
        AmbaKAL_TaskSleep(5);
        AmbaGPIO_ConfigOutput(GPIO_PIN_21, 0);
        AmbaKAL_TaskSleep(600);
        AmbaGPIO_ConfigOutput(GPIO_PIN_21, 1);
        AmbaKAL_TaskSleep(20);
        AmbaGPIO_ConfigOutput(GPIO_PIN_19, 1);
    }

    AmbaKAL_TaskSleep(500); // Need to wait for a while before programing B5F/B5N
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SensorHardwareReset
 *
 *  @Description:: Reset Sensors
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_SensorHardwareReset(void)
{
    AMBA_B5_CHIP_ID_u ChipID;
    ChipID.Data = B5_AR0230_CurChan.SensorID;

    /* B5F GPIO 9 is for senosr PWDN pin */
    /* B5F GPIO 8 is for senosr reset pin */
    B5_AR0230_B5F_GPIO_Clr(ChipID, 31);
    AmbaKAL_TaskSleep(5);
    B5_AR0230_B5F_GPIO_Clr(ChipID, 30);

    AmbaKAL_TaskSleep(20);

    B5_AR0230_B5F_GPIO_Set(ChipID, 31);
    AmbaKAL_TaskSleep(5);
    B5_AR0230_B5F_GPIO_Set(ChipID, 30);

    AmbaKAL_TaskSleep(5);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_RegWrite
 *
 *  @Description:: Write sensor registers through I2C bus
 *
 *  @Input      ::
 *      pB5Chan:   Vin ID and sensor ID
 *      pTxData:   Pointer to Write data buffer
 *      Size:      Number of Write data
 *
 *  @Output     :: none
 *
 *  @Return     :: none
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_RegWrite(AMBA_B5_CHANNEL_s *pB5Chan, UINT8 *pTxDataBuf, UINT8 DataSize)
{
    AMBA_B5_I2C_CHANNEL_e I2cChanNo = AMBA_B5_I2C_CHANNEL1;
    UINT32 SlaveAddr = 0x20;
    UINT32 RegAddr;
    UINT16 Data;
    UINT16 LData = 0;
    UINT16 HData = 0;

    RegAddr = pTxDataBuf[0];
    RegAddr = (RegAddr << 8) | pTxDataBuf[1];

    AmbaKAL_SemTake(&SensorAeSem, AMBA_KAL_WAIT_FOREVER);

    // AmbaB5_I2cWrite(pB5Chan, I2cChanNo, AMBA_B5_I2C_SPEED_FAST, SlaveAddr, DataSize, pTxDataBuf);
    AmbaB5_I2cBurstWrite(pB5Chan, I2cChanNo, AMBA_B5_I2C_SPEED_FAST, SlaveAddr, DataSize, pTxDataBuf);

    AmbaKAL_SemGive(&SensorAeSem);

    HData = pTxDataBuf[2];
    LData = pTxDataBuf[3];

    Data = LData + (HData << 8);
    //Debug usage
    //AmbaPrint("[Write]ChipID = 0x%02x, Addr = 0x%04x, Data = 0x%04X",
                   //pB5Chan->SensorID, RegAddr, Data);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_RegRead
 *
 *  @Description:: Read sensor registers through I2C bus
 *
 *  @Input      ::
 *      pB5Chan:   Vin ID and sensor ID
 *      RegAddr:   Sensor Address
 *      pRxData:   Pointer to Read data buffer
 *
 *  @Output     :: none
 *
 *  @Return     :: none
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int B5_AR0230_RegRead(AMBA_B5_CHANNEL_s *pB5Chan, UINT16 RegAddr, UINT16 *pRxDataBuf)
{
    AMBA_B5_I2C_TRANSACTION_s I2cTransaction[2];
    AMBA_B5_I2C_CHANNEL_e I2cChanNo = AMBA_B5_I2C_CHANNEL1;
    AMBA_B5_CHANNEL_e RxChan = AMBA_B5_CHANNEL0;

    UINT32 SlaveAddr = 0x20;
    UINT8 TxDataBuf[4];
    UINT8 RxDataBuf[4];
    UINT8 DataSize = 4;
    UINT16 LData = 0;
    UINT16 HData = 0;

    if (pB5Chan->SensorID & 0xf) {
        if (pB5Chan->SensorID == 1)
            RxChan = AMBA_B5_CHANNEL0;
        else if (pB5Chan->SensorID == 2)
            RxChan = AMBA_B5_CHANNEL1;
        else if (pB5Chan->SensorID == 4)
            RxChan = AMBA_B5_CHANNEL2;
        else if (pB5Chan->SensorID == 8)
            RxChan = AMBA_B5_CHANNEL3;
    }

    /* Fill the data buffer */
    TxDataBuf[0] = RegAddr >> 8;;
    TxDataBuf[1] = RegAddr & 0xff;

    I2cTransaction[0].SlaveAddr = SlaveAddr;
    I2cTransaction[0].DataSize = 2;
    I2cTransaction[0].pDataBuf = TxDataBuf;
    I2cTransaction[1].SlaveAddr = SlaveAddr | 0x1;
    I2cTransaction[1].DataSize = DataSize - 2;
    I2cTransaction[1].pDataBuf = RxDataBuf;

    AmbaKAL_SemTake(&SensorAeSem, AMBA_KAL_WAIT_FOREVER);

    AmbaB5_I2cReadAfterWrite(pB5Chan, I2cChanNo, AMBA_B5_I2C_SPEED_FAST, 1,
                             &I2cTransaction[0], &I2cTransaction[1], RxChan);

    AmbaKAL_SemGive(&SensorAeSem);

    HData = RxDataBuf[0];
    LData = RxDataBuf[1];

    *pRxDataBuf = LData + (HData << 8);

    //Debug usage
    //AmbaPrint("[Read]ChipID = 0x%02x, Addr = 0x%04x, Data = 0x%04X",
               //pB5Chan->SensorID, RegAddr, *pRxDataBuf);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_Write
 *
 *  @Description:: Write sensor registers through I2C bus
 *
 *  @Input      ::
 *      pB5Chan:   Vin ID and sensor ID
 *      Addr:      Register Address
 *      Data:      Write data
 *
 *  @Output     :: none
 *
 *  @Return     :: none
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_Write(AMBA_B5_CHANNEL_s *pB5Chan, UINT16 Addr, UINT16 Data)
{
    UINT8 TxData[4];

    TxData[0] = Addr >> 8;
    TxData[1] = Addr & 0xff;
    TxData[2] = Data >> 8;
    TxData[3] = Data & 0xff;

    B5_AR0230_RegWrite(pB5Chan, TxData, 4);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_RegisterWrite
 *
 *  @Description:: Write sensor registers API
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
static int B5_AR0230_RegisterWrite(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 Data)
{
    UINT8 TxData[4];

    TxData[0] = Addr >> 8;
    TxData[1] = Addr & 0xff;
    TxData[2] = Data >> 8;
    TxData[3] = Data & 0xff;

    B5_AR0230_CurChan.SensorID = Chan.Bits.SensorID;
    B5_AR0230_RegWrite(&B5_AR0230_CurChan, TxData, 4);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_RegisterRead
 *
 *  @Description:: Read sensor registers API
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *      Addr:   Register address
 *      pData:  pointer to Rx data buffer
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_RegisterRead(AMBA_DSP_CHANNEL_ID_u Chan, UINT16 Addr, UINT16 *pData)
{
    B5_AR0230_CurChan.SensorID = Chan.Bits.SensorID;
    B5_AR0230_RegRead(&B5_AR0230_CurChan, Addr, pData);

    return OK;
}

static void B5_AR0230_SetRegR0x301A(AMBA_B5_CHANNEL_s *pB5Chan, UINT32 RestartFrame, UINT32 Streaming)
{
    UINT16 R0x301A;

    R0x301A =
            0 << 12                  | // 1: disable serial interface (HiSPi)
            0 << 11                  | // force_pll_on
            0 << 10                  | // DONT restart if bad frame is detected
            0 << 9                   | // The sensor will produce bad frames as some register changed
            0 << 8                   | // input buffer related to GPI0/1/2/3 inputs are powered down & disabled
            0 << 7                   | // Parallel data interface is enabled (dep on bit[6])
            1 << 6                   | // Parallel interface is driven
            1 << 4                   | // reset_reg_unused
            1 << 3                   | // Forbids to change value of SMIA registers
            (Streaming > 0) << 2     | // Put the sensor in streaming mode
            (RestartFrame > 0) << 1  | // Causes sensor to truncate frame at the end of current row and start integrating next frame
            0 << 0;                    // Set the bit initiates a reset sequence

    // Typically, in normal streamming mode (restart_frame=0, streaming=1), the value is 0x10DC
    B5_AR0230_Write(pB5Chan, 0x301A, R0x301A);
}

static int B5_AR0230_ConfirmStreamingMode(AMBA_B5_CHANNEL_s *pB5Chan)
{
    int PollingCnt = 200, RetStatus;
    UINT16 Data = 0xFFFF;

    /* Make sure that sensor is streaming */
    while (1) {
        /* Read frame count */
        B5_AR0230_RegRead(pB5Chan, 0x303A, &Data);

        //AmbaPrint("confirm_streaming: frame_count = 0x%04X", Data);

        if (Data != 0xFFFF) {
            RetStatus = OK;
            break;
        }

        if (PollingCnt < 0) {
            RetStatus = NG;
            break;
        }
        AmbaKAL_TaskSleep(1);
        PollingCnt--;
    }
    return RetStatus;
}

static int B5_AR0230_ConfirmStandbyMode(AMBA_B5_CHANNEL_s *pB5Chan)
{
    int PollingCnt = 200, RetStatus;
    UINT16 Data = 0xFFFF;

    while (1) {
        /* Read frame status */
        B5_AR0230_RegRead(pB5Chan, 0x303C, &Data);

        //AmbaPrint("confirm_standby: R0x303C = 0x%04X", Data);

        if ((Data & 0x0002) && (Data != 0xFFFF)) {
            RetStatus = OK;
            break;
        }

        if (PollingCnt < 0) {
            RetStatus = NG;
            break;
        }
        AmbaKAL_TaskSleep(1);
        PollingCnt--;
    }
    return RetStatus;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SoftwareReset
 *
 *  @Description:: Reset (SW) AR0230 Image Sensor Device
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static inline void B5_AR0230_SoftwareReset(AMBA_B5_CHANNEL_s *pB5Chan)
{
    /* R0x301A[0]: software reset */
    B5_AR0230_Write(pB5Chan, 0x301A, 0x0059);

    /* Wait for internal initialization */
    AmbaKAL_TaskSleep(10);

    //AmbaPrint("Sensor SW Reset");
    if (B5_AR0230_ConfirmStandbyMode(pB5Chan) == -1) {
        AmbaPrint("cannot enter standby mode after SW reset!");
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetStandbyOn
 *
 *  @Description:: Configure sensor into standby mode
 *
 *  @Input      :: none
 *      pB5Chan:   Vin ID and sensor ID
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_SetStandbyOn(AMBA_B5_CHANNEL_s *pB5Chan)
{
    B5_AR0230_SetRegR0x301A(pB5Chan, 0/*restart_frame*/, 0/*streaming*/);

    if (B5_AR0230_ConfirmStandbyMode(pB5Chan) == -1)
        AmbaPrint("cannot enter standby mode!");
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetStandbyOff
 *
 *  @Description:: Configure sensor to leave standby mode.
 *
 *  @Input      ::
 *      pB5Chan:   Vin ID and sensor ID
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_SetStandbyOff(AMBA_B5_CHANNEL_s *pB5Chan)
{
    B5_AR0230_SetRegR0x301A(pB5Chan, 0/*restart_frame*/, 1/*streaming*/);

    //B5_AR0230_Write(pB5Chan, (0x301A, 0x005C);

    if (B5_AR0230_ConfirmStreamingMode(pB5Chan) == -1)
        AmbaPrint("cannot enter streaming mode");
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ConfigPLL
 *
 *  @Description:: Configure sensor PLL.
 *
 *  @Input      ::
 *      Mode:   Sensor readout mode number
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_ConfigPLL(AMBA_B5_CHANNEL_s *pB5Chan, UINT16 Mode)
{
    int i;
    UINT16 Addr, Data;

    /* Configure sensor PLL */
    for (i = 0; i < B5_AR0230_NUM_PLL_REG; i++) {
        Addr = B5_AR0230PllRegTable[i].Addr;
        Data = B5_AR0230PllRegTable[i].Data[B5_AR0230ModeInfoList[Mode].PllConfig];
        B5_AR0230_Write(pB5Chan, Addr, Data);
    }

    AmbaKAL_TaskSleep(1);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ConfigOptSetting
 *
 *  @Description:: Configure sensor optimized setting.
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_ConfigOptSetting(AMBA_B5_CHANNEL_s *pB5Chan)
{
    int i;
    UINT16 Addr, Data;

    /* Configure sensor PLL */
    for (i = 0; i < B5_AR0230_NUM_SEQUENCER_OPTIMIZED_REG; i++) {
        Addr = B5_AR0230OptSeqRegTable[i].Addr;
        Data = B5_AR0230OptSeqRegTable[i].Data[0];
        B5_AR0230_Write(pB5Chan, Addr, Data);
    }

    AmbaKAL_TaskSleep(1);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ConfigLinearSeq
 *
 *  @Description:: Configure sensor linear mode setting.
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_ConfigLinearSeq(AMBA_B5_CHANNEL_s *pB5Chan)
{
    int i;
    UINT16 Addr, Data;

    /* Configure sensor PLL */
    for (i = 0; i < B5_AR0230_NUM_SEQUENCER_LINEAR_REG; i++) {
        Addr = B5_AR0230LinearSeqRegTable[i].Addr;
        Data = B5_AR0230LinearSeqRegTable[i].Data[0];
        B5_AR0230_Write(pB5Chan, Addr, Data);
    }

    AmbaKAL_TaskSleep(1);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ConfigHDRSeq
 *
 *  @Description:: Configure sensor HDR mode setting.
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_ConfigHDRSeq(AMBA_B5_CHANNEL_s *pB5Chan)
{
    int i;
    UINT16 Addr, Data;

    /* Configure sensor PLL */
    for (i = 0; i < B5_AR0230_NUM_SEQUENCER_HDR_REG; i++) {
        Addr = B5_AR0230HDRSeqRegTable[i].Addr;
        Data = B5_AR0230HDRSeqRegTable[i].Data[0];
        B5_AR0230_Write(pB5Chan, Addr, Data);
    }

    AmbaKAL_TaskSleep(1);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ChangeReadoutMode
 *
 *  @Description:: Switch sensor to requested mode.
 *                 It's for the case that sensor have been initialized.
 *
 *  @Input      ::
 *      pB5Chan:   Vin ID and sensor ID
 *      Mode:      Sensor mode ID
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_ChangeReadoutMode(AMBA_B5_CHANNEL_s *pB5Chan, UINT16 Mode)
{
    int i;
    UINT16 Addr, Data;

    for (i = 0; i < B5_AR0230_NUM_READOUT_MODE_REG; i++) {
        Addr = B5_AR0230RegTable[i].Addr;
        Data = B5_AR0230RegTable[i].Data[B5_AR0230ModeInfoList[Mode].ReadoutMode];

        if (Data == NA)
            continue;

        B5_AR0230_Write(pB5Chan, Addr, Data);
    }
    B5_AR0230Ctrl.GainFactor = 0xffffffff;
    B5_AR0230Ctrl.AnalogGain = 1.0;
    B5_AR0230Ctrl.DigitalGain = 1.0;
    B5_AR0230Ctrl.ShutterCtrl= 0xffffffff;
    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_EnableALTM
 *
 *  @Description:: set AR0230 ALTM
 *
 *  @Input      ::
 *      Enable:    Enable ALTM or not
 *
 *  @Output     ::
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_EnableALTM(AMBA_B5_CHANNEL_s *pB5Chan, UINT8 Enable)
{
    if(Enable) {
        B5_AR0230_Write(pB5Chan, 0x2420, 0x0000);
        B5_AR0230_Write(pB5Chan, 0x2440, 0x0004);
        B5_AR0230_Write(pB5Chan, 0x2442, 0x0080);
        B5_AR0230_Write(pB5Chan, 0x301E, 0x0000);//Data Pedestal(Black Level)
        B5_AR0230_Write(pB5Chan, 0x2450, 0x0000);//ALTM Pedstal
        B5_AR0230_Write(pB5Chan, 0x320A, 0x0080);
        B5_AR0230_Write(pB5Chan, 0x31D0, 0x0000);
        B5_AR0230_Write(pB5Chan, 0x2400, 0x0002);//Enable ALTM 0x2400[0] = 0
        B5_AR0230_Write(pB5Chan, 0x2410, 0x0005);
        B5_AR0230_Write(pB5Chan, 0x2412, 0x002D);
        B5_AR0230_Write(pB5Chan, 0x2444, 0xF400);
        B5_AR0230_Write(pB5Chan, 0x2446, 0x0001);
        B5_AR0230_Write(pB5Chan, 0x2438, 0x0010);
        B5_AR0230_Write(pB5Chan, 0x243A, 0x0012);
        B5_AR0230_Write(pB5Chan, 0x243C, 0xFFFF);
        B5_AR0230_Write(pB5Chan, 0x243E, 0x0100);
    } else {
        B5_AR0230_Write(pB5Chan, 0x2400, 0x0001);//Disable ALTM 0x2400[0] = 1 Disable 0x2400[1] for power saving 20150402
        B5_AR0230_Write(pB5Chan, 0x301E, 0x00A8);//Data Pedestal(Black Level)
        B5_AR0230_Write(pB5Chan, 0x2450, 0x0000);
        B5_AR0230_Write(pB5Chan, 0x320A, 0x0080);
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_Enable2DMC
 *
 *  @Description:: Set AR0230 2DMC
 *
 *  @Input      ::
 *      Enable:    Enable 2DMC or not
 *
 *  @Output     ::
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_Enable2DMC(AMBA_B5_CHANNEL_s *pB5Chan, UINT8 Enable)
{
    if(Enable) {
        B5_AR0230_Write(pB5Chan, 0x3190, 0x0000);
        B5_AR0230_Write(pB5Chan, 0x318A, 0x0E74);
        B5_AR0230_Write(pB5Chan, 0x318C, 0x4000);//Disable smoothing on 2DMC 20150402
        B5_AR0230_Write(pB5Chan, 0x3192, 0x0400);
        B5_AR0230_Write(pB5Chan, 0x3198, 0x183C);//Motion_detect_q1 0x3198[7:0]; Motion_detect_q2 0x3198[15:8]
    } else {
        B5_AR0230_Write(pB5Chan, 0x318C, 0x0000);
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_EnableCompanding
 *
 *  @Description:: Set AR0230 Companding
 *
 *  @Input      ::
 *      Enable:    Enable Companding or not
 *
 *  @Output     ::
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_EnableCompanding(AMBA_B5_CHANNEL_s *pB5Chan, UINT8 Enable)
{
    if(Enable) {
        B5_AR0230_Write(pB5Chan, 0x31AC, 0x100C);
        B5_AR0230_Write(pB5Chan, 0x31D0, 0x0001);
    } else {
        B5_AR0230_Write(pB5Chan, 0x31D0, 0x0000);
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetADACD
 *
 *  @Description:: Set AR0230 ADACD
 *
 *  @Input      ::
 *      Operation:    ADACD operation mode
 *
 *  @Output     ::
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_SetADACD(AMBA_B5_CHANNEL_s *pB5Chan, UINT32 Operation)
{
    UINT16 R0x3200;
    switch(Operation) {
        case B5_AR0230_ADACD_DISABLE:
            B5_AR0230_Write(pB5Chan, 0x3200, 0x0000);//disable AdaCD 0x3200[1] = 0
            break;
        case B5_AR0230_ADACD_LOWLIGHT:
            B5_AR0230_RegRead(pB5Chan, 0x3200, &R0x3200);
            R0x3200 |= 2;
            B5_AR0230_Write(pB5Chan, 0x3200, R0x3200);//Enable AdaCD 0x3200[1] = 1
            break;
        case B5_AR0230_ADACD_LOWDCG:
            B5_AR0230_Write(pB5Chan, 0x3206, 0x0B08);//adacd_noise_floor1
            B5_AR0230_Write(pB5Chan, 0x3208, 0x1E13);//adacd_noise_floor2
            B5_AR0230_Write(pB5Chan, 0x3202, 0x0080);//Adjusts AdaCD filter strength
            B5_AR0230_Write(pB5Chan, 0x3200, 0x02);
            break;
        case B5_AR0230_ADACD_HIGHDCG:
            B5_AR0230_Write(pB5Chan, 0x3206, 0x1C0E);//adacd_noise_floor1
            B5_AR0230_Write(pB5Chan, 0x3208, 0x4E39);//adacd_noise_floor2
            B5_AR0230_Write(pB5Chan, 0x3202, 0x00B0);//Adjusts AdaCD filter strength
            B5_AR0230_Write(pB5Chan, 0x3200, 0x02);
            break;
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetConversionGain
 *
 *  @Description:: Set AR0230 Conversion Gain Mode
 *
 *  @Input      ::
 *      Operation:    Conversion Gain Mode
 *
 *  @Output     ::
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_SetConversionGain(AMBA_B5_CHANNEL_s *pB5Chan, UINT32 Operation)
{
    switch(Operation) {
        case B5_AR0230_CONVERSION_GAIN_LINEAR_LOWDCG:
            //B5_AR0230_Write(0x3060, 0x000B);
            B5_AR0230_Write(pB5Chan, 0x3096, 0x0080);//ROW_NOISE_ADJUST_TOP
            B5_AR0230_Write(pB5Chan, 0x3098, 0x0080);//ROW_NOISE_ADJUST_BTM
            B5_AR0230_SetADACD(pB5Chan, B5_AR0230_ADACD_DISABLE);
            B5_AR0230_Write(pB5Chan, 0x3100, 0x0000);//Enable LCG
            break;
        case B5_AR0230_CONVERSION_GAIN_LINEAR_HIGHDCG:
            //B5_AR0230_Write(0x3060, 0x000B);
            B5_AR0230_Write(pB5Chan, 0x3096, 0x0080);//ROW_NOISE_ADJUST_TOP
            B5_AR0230_Write(pB5Chan, 0x3098, 0x0080);//ROW_NOISE_ADJUST_BTM
            B5_AR0230_SetADACD(pB5Chan, B5_AR0230_ADACD_DISABLE);
            B5_AR0230_Write(pB5Chan, 0x3100, 0x0004);//Enable HCG additional 2.7x analog gain
            break;
        case B5_AR0230_CONVERSION_GAIN_HDR_LOWDCG:
            //B5_AR0230_Write(0x3060, 0x000B);
            B5_AR0230_Write(pB5Chan, 0x3096, 0x0480);//ROW_NOISE_ADJUST_TOP
            B5_AR0230_Write(pB5Chan, 0x3098, 0x0480);//ROW_NOISE_ADJUST_BTM
            B5_AR0230_SetADACD(pB5Chan, B5_AR0230_ADACD_LOWDCG);
            B5_AR0230_Write(pB5Chan, 0x3100, 0x0000);//Enable LCG
            break;
        case B5_AR0230_CONVERSION_GAIN_HDR_HIGHDCG:
            //B5_AR0230_Write(0x3060, 0x000B);
            B5_AR0230_Write(pB5Chan, 0x3096, 0x0780);//ROW_NOISE_ADJUST_TOP
            B5_AR0230_Write(pB5Chan, 0x3098, 0x0780);//ROW_NOISE_ADJUST_BTM
            B5_AR0230_SetADACD(pB5Chan, B5_AR0230_ADACD_HIGHDCG);
            B5_AR0230_Write(pB5Chan, 0x3100, 0x0004);//Enable HCG additional 2.7x analog gain
            break;
    }
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetHDRRatio
 *
 *  @Description:: Set AR0230 HDR ratio
 *
 *  @Input      ::
 *      Ratio:    HDR ratio
 *
 *  @Output     ::
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_SetHDRRatio(AMBA_B5_CHANNEL_s *pB5Chan, UINT8 Ratio)
{
    switch(Ratio) {
        case 32:
            B5_AR0230_Write(pB5Chan, 0x3082, 0x000C);
            break;
        case 16:
            B5_AR0230_Write(pB5Chan, 0x3082, 0x0008);
            break;
        case 8:
            B5_AR0230_Write(pB5Chan, 0x3082, 0x0004);
            break;
        case 4:
            B5_AR0230_Write(pB5Chan, 0x3082, 0x0000);
            break;
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetHiSPi
 *
 *  @Description:: Set AR0230 HiSPi
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_SetHiSPi(AMBA_B5_CHANNEL_s *pB5Chan, UINT8 NumLane)
{
    if(NumLane == 4)
        B5_AR0230_Write(pB5Chan, 0x31AE, 0x0304);
    else if(NumLane == 2)
        B5_AR0230_Write(pB5Chan, 0x31AE, 0x0302);

    B5_AR0230_Write(pB5Chan, 0x31C6, 0x0006);
    B5_AR0230_Write(pB5Chan, 0x306E, 0x9210);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SensorModeInit
 *
 *  @Description:: Set AR0230 sensor init register
 *
 *  @Input      ::
 *      Mode    Sensor mode
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_SensorModeInit(AMBA_B5_CHANNEL_s *pB5Chan, UINT32 Mode)
{
    if(B5_AR0230ModeInfoList[Mode].OperationMode == B5_AR0230_OPERATION_MODE_HDR_LINEAR) {
        B5_AR0230_ConfigHDRSeq(pB5Chan);
        B5_AR0230_ConfigOptSetting(pB5Chan);
        B5_AR0230_Write(pB5Chan, 0x30B0, 0x0118); //Update R0x30B0 at init 20150402
        B5_AR0230_SetHDRRatio(pB5Chan, 16);
        //B5_AR0230_Write(pB5Chan, 0x30BA, 0x770C); //Remove R0x30BA setting from HDR init 20150402
        B5_AR0230_Write(pB5Chan, 0x31E0, 0x0200);  //2DDC Enable
        B5_AR0230_EnableALTM(pB5Chan, 0);
        B5_AR0230_Enable2DMC(pB5Chan, 1);
        B5_AR0230_Write(pB5Chan, 0x3060, 0x000B); //Set default analog gain 1.5x
        B5_AR0230_SetConversionGain(pB5Chan, B5_AR0230_CONVERSION_GAIN_HDR_LOWDCG);
        B5_AR0230_EnableCompanding(pB5Chan, 0);
        B5_AR0230_Write(pB5Chan, 0x31AC, 0x100E); //data_format_bits
        B5_AR0230_Write(pB5Chan, 0x318E, 0x0000); //Disable gain_before_dlo 20150402

        B5_AR0230_SetHiSPi(pB5Chan, B5_AR0230OutputInfo[B5_AR0230ModeInfoList[Mode].OutputMode].NumDataLanes);
        B5_AR0230_ConfigPLL(pB5Chan, Mode);
        B5_AR0230_ChangeReadoutMode(pB5Chan, Mode);
    } else if (B5_AR0230ModeInfoList[Mode].OperationMode == B5_AR0230_OPERATION_MODE_HDR_ALTM){
        B5_AR0230_ConfigHDRSeq(pB5Chan);
        B5_AR0230_ConfigOptSetting(pB5Chan);
        B5_AR0230_Write(pB5Chan, 0x30B0, 0x0118); //Update R0x30B0 at init 20150402
        B5_AR0230_Write(pB5Chan, 0x31AC, 0x0C0C); //data_format_bits
        B5_AR0230_SetHDRRatio(pB5Chan, 16);
        //B5_AR0230_Write(pB5Chan, 0x30BA, 0x770C); //Remove R0x30BA setting from HDR init 20150402
        B5_AR0230_Write(pB5Chan, 0x31E0, 0x0200);  //2DDC Enable
        B5_AR0230_EnableALTM(pB5Chan, 1);
        B5_AR0230_Enable2DMC(pB5Chan, 1);
        B5_AR0230_Write(pB5Chan, 0x3060, 0x000B); //Set default analog gain 1.5x
        B5_AR0230_SetConversionGain(pB5Chan, B5_AR0230_CONVERSION_GAIN_HDR_LOWDCG);
        B5_AR0230_Write(pB5Chan, 0x318E, 0x0000); //Disable gain_before_dlo 20150402

        B5_AR0230_SetHiSPi(pB5Chan, B5_AR0230OutputInfo[B5_AR0230ModeInfoList[Mode].OutputMode].NumDataLanes);
        B5_AR0230_ConfigPLL(pB5Chan, Mode);
        B5_AR0230_ChangeReadoutMode(pB5Chan, Mode);
    } else if (B5_AR0230ModeInfoList[Mode].OperationMode == B5_AR0230_OPERATION_MODE_LINEAR){
        B5_AR0230_ConfigLinearSeq(pB5Chan);
        B5_AR0230_ConfigOptSetting(pB5Chan);
        B5_AR0230_Write(pB5Chan, 0x30B0, 0x0118); //Update R0x30B0 at init 20150402
        B5_AR0230_Write(pB5Chan, 0x31AC, 0x0C0C); //data_format_bits
        B5_AR0230_Write(pB5Chan, 0x3082, 0x0009);
        B5_AR0230_Write(pB5Chan, 0x30BA, 0x769C); //Update R0x30BA setting 20150402
        B5_AR0230_Write(pB5Chan, 0x31E0, 0x0200); //2DDC Enable
        B5_AR0230_EnableALTM(pB5Chan, 0);
        B5_AR0230_Enable2DMC(pB5Chan, 0);
        B5_AR0230_Write(pB5Chan, 0x3060, 0x000B); //Set default analog gain 1.5x
        B5_AR0230_SetConversionGain(pB5Chan, B5_AR0230_CONVERSION_GAIN_LINEAR_LOWDCG);
        B5_AR0230_SetADACD(pB5Chan, B5_AR0230_ADACD_DISABLE);
        B5_AR0230_EnableCompanding(pB5Chan, 0);

        B5_AR0230_SetHiSPi(pB5Chan, B5_AR0230OutputInfo[B5_AR0230ModeInfoList[Mode].OutputMode].NumDataLanes);
        B5_AR0230_ConfigPLL(pB5Chan, Mode);
        B5_AR0230_ChangeReadoutMode(pB5Chan, Mode);
    } else if (B5_AR0230ModeInfoList[Mode].OperationMode == B5_AR0230_OPERATION_MODE_INTERLEAVE){
        B5_AR0230_ConfigHDRSeq(pB5Chan);
        B5_AR0230_ConfigOptSetting(pB5Chan);
        B5_AR0230_Write(pB5Chan, 0x30B0, 0x0118); //Update R0x30B0 at init 20150402
        B5_AR0230_Write(pB5Chan, 0x31AC, 0x0C0C); //data_format_bits
        B5_AR0230_SetHDRRatio(pB5Chan, 16);
        //B5_AR0230_Write(pB5Chan, 0x30BA, 0x770C); //Remove R0x30BA setting from HDR init 20150402
        B5_AR0230_Write(pB5Chan, 0x31E0, 0x0200);  //2DDC Enable
        B5_AR0230_SetADACD(pB5Chan, B5_AR0230_ADACD_DISABLE);
        B5_AR0230_Enable2DMC(pB5Chan, 0);
        B5_AR0230_Write(pB5Chan, 0x3060, 0x000B); //Set default analog gain 1.5x
        B5_AR0230_SetConversionGain(pB5Chan, B5_AR0230_CONVERSION_GAIN_HDR_LOWDCG);
        B5_AR0230_EnableALTM(pB5Chan, 0);
        B5_AR0230_EnableCompanding(pB5Chan, 0);
        B5_AR0230_Write(pB5Chan, 0x318E, 0x1000); //Disable gain_before_dlo 20150402

        B5_AR0230_SetHiSPi(pB5Chan, B5_AR0230OutputInfo[B5_AR0230ModeInfoList[Mode].OutputMode].NumDataLanes);
        B5_AR0230_ConfigPLL(pB5Chan, Mode);
        B5_AR0230_ChangeReadoutMode(pB5Chan, Mode);
    }
}


#if 0
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_VinHookFunc
 *
 *  @Description:: Hook this callback function as VIN ISR of Ambarella SoC
 *
 *  @Input      ::
 *      EntryArg:  Dummy argument
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_VinHookFunc(int Entry) {
    AmbaB5_PllSwPllVinHookFunc((UINT32)Entry);
}
#endif

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SysChanConvert
 *
 *  @Description:: Convert B5_AR0230_SysChan to mw configuration
 *
 *  @Input      ::
 *      Chan: Vin ID and sensor ID
 *
 *  @Output     :: none
 *      pB5Chan: Vin ID and sensor ID
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_SysChanConvert(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_B5_CHANNEL_s *pB5Chan)
{
    int i;

    pB5Chan->SensorID = 0;

    for (i = 0; i< AMBA_NUM_B5_CHANNEL; i++) {
        if (Chan.Bits.SensorID & (1 << i)) {
            pB5Chan->Active[i] = AMBA_B5_CHANNEL_FAR_END; // Fixed to far end sensor input
//            pB5Chan->Inactive[i] = AMBA_B5_CHANNEL_INTERNAL; // enable internal test pattern if there is sensor input
            pB5Chan->SensorID |= (1 << i);
        } else {
            pB5Chan->Active[i] = AMBA_B5_CHANNEL_DISABLED;
            pB5Chan->Inactive[i] = AMBA_B5_CHANNEL_DISABLED;
        }
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_CurChanConvert
 *
 *  @Description:: Convert B5_AR0230_CurChan to current configuration
 *
 *  @Input      ::
 *      Chan: Vin ID and sensor ID
 *
 *  @Output     :: none
 *      pB5Chan: Vin ID and sensor ID
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_CurChanConvert(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_B5_CHANNEL_s *pB5Chan)
{
    int i;

    pB5Chan->SensorID = 0;

    for (i = 0; i< AMBA_NUM_B5_CHANNEL; i++) {
        if (Chan.Bits.SensorID & (1 << i) & B5_AR0230_SysChan.SensorID) {
            pB5Chan->Active[i] = B5_AR0230_SysChan.Active[i];
            pB5Chan->SensorID |= (1 << i);
        } else
            pB5Chan->Inactive[i] = B5_AR0230_SysChan.Inactive[i];
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_Init
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
static int B5_AR0230_Init(AMBA_DSP_CHANNEL_ID_u Chan)
{
#if 0
    /* For Second VIN */
    if (Chan.Bits.VinID == 1) {
        AmbaPLL_SetEnetClkConfig(1);
        AmbaPLL_SetEnetClk(24000000);

    /* For Main VIN */
    } else {
        AmbaPLL_SetSensorClk(24000000);
        AmbaPLL_SetSensorClkDir(AMBA_PLL_SENSOR_CLK_OUTPUT);
    }
#else
    AmbaPLL_SetSensorClk(24000000);
    AmbaPLL_SetSensorClkDir(AMBA_PLL_SENSOR_CLK_OUTPUT);
#endif
    B5_AR0230_SysChanConvert(Chan, &B5_AR0230_SysChan); // Convert SysChan

#if 0 /* TBD */
    if (B5_AR0230_PinConfig.B5fPinMux == AMBA_B5_PIN_B5F_CTRL_PWM_SINGLE_ENDED) {
       AmbaINT_IsrHook(AMBA_VIC_INT_ID53_IDSP_PIP_VSYNC_SLAVE, AMBA_VIC_INT_ID53_IDSP_PIP_VSYNC_SLAVE, AMBA_VIC_INT_RISING_EDGE_TRIGGER, B5_AR0230_VinHookFunc);
       AmbaINT_IntEnable(AMBA_VIC_INT_ID53_IDSP_PIP_VSYNC_SLAVE);
    }
#endif

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_Enable
 *
 *  @Description:: Power on sensor
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_Enable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (B5_AR0230Ctrl.Status.DevStat.Bits.Sensor0Power == 1)
        return OK;

    B5_AR0230Ctrl.Status.DevStat.Bits.Sensor0Power   = 1;
    B5_AR0230Ctrl.Status.DevStat.Bits.Sensor0Standby = 1;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_Disable
 *
 *  @Description:: Power down sensor
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_Disable(AMBA_DSP_CHANNEL_ID_u Chan)
{
    if (B5_AR0230Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return OK;

    B5_AR0230_CurChan.SensorID = Chan.Bits.SensorID;

    B5_AR0230_SetStandbyOn(&B5_AR0230_CurChan);

    B5_AR0230Ctrl.Status.DevStat.Bits.Sensor0Power = 0;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ChangeFrameRate
 *
 *  @Description:: change frame rate of current sensor mode
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *      pFrameRate: target frame rate
 *
 *  @Output     ::
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_ChangeFrameRate(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_DSP_FRAME_RATE_s *pFrameRate)
{
    return NG;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetGainFactor
 *
 *  @Description:: Set gain value by the combination of analog gain and digital gain
 *
 *  @Input      ::
 *      Chan:       Vin ID and sensor ID
 *      GainFactor: A ratio of the sensor gain (both analog and digital) output to the sensor gain input.
 *                  Minimum is 0.0 dB, Maximum 45.045771199653990948648128508689 dB
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_SetGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float GainFactor)
{
    return NG;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetEshutterSpeed
 *
 *  @Description:: Set sensor eshutter electronic shutter speed
 *
 *  @Input      ::
 *      Chan:   Vin ID and sensor ID
 *      EshutterSpeed:  Electronic shutter speed value (in unit of seconds)
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_SetEshutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float ShutterSpeed)
{
    return NG;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_GetStatus
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
static int B5_AR0230_GetStatus(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_STATUS_INFO_s *pStatus)
{
    if (pStatus == NULL || B5_AR0230Ctrl.Status.DevStat.Bits.Sensor0Power == 0)
        return NG;

    B5_AR0230Ctrl.Status.TimeToAllPelExposed = 0;  /* TODO: for flash light or mechanical shutter control */

    memcpy(pStatus, &B5_AR0230Ctrl.Status, sizeof(AMBA_SENSOR_STATUS_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_GetModeInfo
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
static int B5_AR0230_GetModeInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_MODE_INFO_s *pModeInfo)
{
    AMBA_B5_CHANNEL_s B5Chan;
    UINT16 SensorMode = Mode.Bits.Mode;

    if (SensorMode == AMBA_SENSOR_CURRENT_MODE)
        SensorMode = B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode;

    if (SensorMode >= AMBA_SENSOR_B5_AR0230_NUM_MODE || pModeInfo == NULL)
        return NG;

    B5_AR0230_SysChanConvert(Chan, &B5Chan);
    B5_AR0230_PrepareModeInfo(AmbaB5_GetNumActiveChannel(&B5Chan), SensorMode, pModeInfo, NULL);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_GetDeviceInfo
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
static int B5_AR0230_GetDeviceInfo(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_DEVICE_INFO_s *pDeviceInfo)
{
    if (pDeviceInfo == NULL)
        return NG;

    memcpy(pDeviceInfo, &B5_AR0230DeviceInfo, sizeof(AMBA_SENSOR_DEVICE_INFO_s));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_GetHdrInfo
 *
 *  @Description:: Get Sensor HDR Info
 *
 *  @Input      ::
 *      Chan:           Vin ID and sensor ID
 *      ShutterCtrl:    pointer to sensor exposure line
 *
 *  @Output     ::
 *      pHdrInfo:       pointer to Hdr info
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_GetHdrInfo(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 *pShutterCtrl, AMBA_SENSOR_HDR_INFO_s *pHdrInfo)
{
    UINT16 SensorMode = B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode;
    UINT32 ShutterCtrl = *pShutterCtrl;

    if(B5_AR0230ModeInfoList[SensorMode].OperationMode == B5_AR0230_OPERATION_MODE_INTERLEAVE) {
        UINT32 MinShrWidth = 0;
        UINT32 MaxShrWidth = 0;
        UINT32 ShortExposure = 0;
        UINT16 T1T2Ratio = 1;
        UINT16 R0x3082 = 0;

        if (pHdrInfo == NULL)
            return NG;

        B5_AR0230_RegRead(&B5_AR0230_CurChan, 0x3082, &R0x3082);
        T1T2Ratio = 1 << (((R0x3082 & 0x000c) >> 2) + 2);

        if (T1T2Ratio == 32)
            MinShrWidth = T1T2Ratio * 2;
        else
            MinShrWidth = T1T2Ratio;

        MaxShrWidth = B5_AR0230Ctrl.Status.ModeInfo.NumExposureStepPerFrame * T1T2Ratio / (T1T2Ratio + 1) - 1;
        if(MaxShrWidth > B5_AR0230Ctrl.Status.ModeInfo.NumExposureStepPerFrame - 71)
            MaxShrWidth = B5_AR0230Ctrl.Status.ModeInfo.NumExposureStepPerFrame -71;

        if(ShutterCtrl > MaxShrWidth)
            ShutterCtrl = MaxShrWidth;
        if(ShutterCtrl < MinShrWidth)
            ShutterCtrl = MinShrWidth;

        ShortExposure = ShutterCtrl / T1T2Ratio;

        B5_AR0230Ctrl.Status.ModeInfo.HdrInfo.ChannelInfo[1].EffectiveArea.StartY = ShortExposure;

        memcpy(pHdrInfo, &B5_AR0230Ctrl.Status.ModeInfo.HdrInfo, sizeof(AMBA_SENSOR_HDR_INFO_s));
#if 0
        AmbaPrint("HDR channel number:%d" ,AR0230_HdrInfo.ActiveChannels);
        for(int i = 0; i < AR0230_HdrInfo.ActiveChannels; ++i)
            AmbaPrint("Channel_%d StartX:%d, StartY:%d Width:%d, Height:%d" ,i,AR0230_HdrInfo.EffectiveArea[i].StartX, AR0230_HdrInfo.EffectiveArea[i].StartY, AR0230_HdrInfo.EffectiveArea[i].Width, AR0230_HdrInfo.EffectiveArea[i].Height);
#endif
    } else
        memcpy(pHdrInfo, &B5_AR0230Ctrl.Status.ModeInfo.HdrInfo, sizeof(AMBA_SENSOR_HDR_INFO_s));

   /*T2 could be read by register R0x307C*/

    return OK;

}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_GetCurrentGainFactor
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
static int B5_AR0230_GetCurrentGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float *pGainFactor)
{
    *pGainFactor = ((float)B5_AR0230Ctrl.GainFactor / (1024.0 * 1024.0) );

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_GetCurrentShutterSpeed
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
static int B5_AR0230_GetCurrentShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float *pExposureTime)
{
    *pExposureTime = (B5_AR0230Ctrl.Status.ModeInfo.RowTime * (float)B5_AR0230Ctrl.ShutterCtrl);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ConvertGainFactor
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
static int B5_AR0230_ConvertGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, float DesiredFactor, UINT32 *pActualFactor, UINT32 *pAnalogGainCtrl, UINT32 *pDigitalGainCtrl)
{
    float AgcGain = 0.0;
    float DgcGain = 0.0;
    UINT16 AgcR3060 = 0;
    UINT16 Dgc305E = 0;
    UINT8 ConvGain = 2;// 0:LCG, 1:HCG 2:Default

    if( DesiredFactor >= 2.7) {
        ConvGain = 1;
    } else if (DesiredFactor < 2.7) {
        ConvGain = 0;
    } else
        ConvGain = 0;

    //AmbaPrint("Original DesiredFactor:%f, ConvGain:%d,",DesiredFactor,ConvGain);

    /*20150313 HDR and linear have different gain table*/
    if(B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_LINEAR) {
        if (DesiredFactor >= 43.2) {
            AgcR3060 = 0x0040;
            AgcGain = 43.2;
        } else if (DesiredFactor >= 38.4) {
            AgcR3060 = 0x003E;
            AgcGain = 38.4;
        } else if (DesiredFactor >= 34.56) {
            AgcR3060 = 0x003C;
            AgcGain = 34.56;
        }
    } else {
        if (DesiredFactor >= 32.4) {
            AgcR3060 = 0x003B;  // Max Agc
            AgcGain = 32.4;
        }
    }

    if ( AgcR3060 == 0 && AgcGain == 0.0) {
        if (DesiredFactor >= 31.43){
            AgcR3060 =  0x003A;
            AgcGain = 31.43;
        } else if (DesiredFactor >= 28.79){
            AgcR3060 =  0x0038;
            AgcGain = 28.79;
        } else if (DesiredFactor >= 26.59){
            AgcR3060 =  0x0036;
            AgcGain = 26.59;
        } else if (DesiredFactor >= 24.69){
            AgcR3060 =  0x0034;
            AgcGain = 24.69;
        } else if (DesiredFactor >= 23.05){
            AgcR3060 =  0x0032;
            AgcGain = 23.05;
        } else if (DesiredFactor >= 21.6) {
            AgcR3060 =  0x0030;
            AgcGain = 21.6;
        } else if (DesiredFactor >= 19.2) {
            AgcR3060 =  0x002E;
            AgcGain = 19.2;
        } else if (DesiredFactor >= 17.28) {
            AgcR3060 =  0x002C;
            AgcGain = 17.28;
        } else if (DesiredFactor >= 15.71) {
            AgcR3060 =  0x002A;
            AgcGain = 15.71;
        } else if (DesiredFactor >= 14.4) {
            AgcR3060 =  0x0028;
            AgcGain = 14.4;
        } else if (DesiredFactor >= 13.29) {
            AgcR3060 =  0x0026;
            AgcGain = 13.29;
        } else if (DesiredFactor >= 12.34) {
            AgcR3060 =  0x0024;
            AgcGain = 12.34;
        } else if (DesiredFactor >= 11.52) {
            AgcR3060 =  0x0022;
            AgcGain = 11.52;
        } else if (DesiredFactor >= 10.8) {
            AgcR3060 =  0x0020;
            AgcGain = 10.8;
        } else if (DesiredFactor >= 9.6) {
            AgcR3060 =  0x001E;
            AgcGain = 9.6;
        } else if (DesiredFactor >= 8.64) {
            AgcR3060 =  0x001C;
            AgcGain = 8.64;
        } else if (DesiredFactor >= 7.86) {
            AgcR3060 =  0x001A;
            AgcGain = 7.86;
        } else if (DesiredFactor >= 7.2) {
            AgcR3060 =  0x0018;
            AgcGain = 7.2;
        } else if (DesiredFactor >= 6.65) {
            AgcR3060 =  0x0016;
            AgcGain = 6.65;
        } else if (DesiredFactor >= 6.17) {
            AgcR3060 =  0x0014;
            AgcGain = 6.17;
        } else if (DesiredFactor >= 5.76) {
            AgcR3060 =  0x0012;
            AgcGain = 5.76;
        } else if (DesiredFactor >= 5.4) {
            AgcR3060 =  0x0010;
            AgcGain = 5.4;
        } else if (DesiredFactor >= 5.08) {
            AgcR3060 =  0x000F;
            AgcGain = 5.08;
        } else if (DesiredFactor >= 4.8) {
            AgcR3060 =  0x000E;
            AgcGain = 4.8;
        } else if (DesiredFactor >= 4.55) {
            AgcR3060 =  0x000D;
            AgcGain = 4.55;
        } else if (DesiredFactor >= 4.32) {
            AgcR3060 =  0x000C;
            AgcGain = 4.32;
        } else if (DesiredFactor >= 4.11) {
            AgcR3060 =  0x000B;
            AgcGain = 4.11;
        } else if (DesiredFactor >= 3.93) {
            AgcR3060 =  0x000A;
            AgcGain = 3.93;
        } else if (DesiredFactor >= 3.76) {
            AgcR3060 =  0x0009;
            AgcGain = 3.76;
        } else if (DesiredFactor >= 3.6) {
            AgcR3060 =  0x0008;
            AgcGain = 3.6;
        } else if (DesiredFactor >= 3.46) {
            AgcR3060 =  0x0007;
            AgcGain = 3.46;
        } else if (DesiredFactor >= 3.32) {
            AgcR3060 =  0x0006;
            AgcGain = 3.32;
        } else if (DesiredFactor >= 3.2) {
            AgcR3060 =  0x0005;
            AgcGain = 3.2;
        } else if (DesiredFactor >= 3.09) {
            AgcR3060 =  0x0004;
            AgcGain = 3.09;
        } else if (DesiredFactor >= 2.98) {
            AgcR3060 =  0x0003;
            AgcGain = 2.98;
        } else if (DesiredFactor >= 2.88) {
            AgcR3060 =  0x0002;
            AgcGain = 2.88;
        } else if (DesiredFactor >= 2.79) {
            AgcR3060 =  0x0001;
            AgcGain = 2.79;
        } else if (DesiredFactor >= 2.7) {
            AgcR3060 =  0x0000;
            AgcGain = 2.7;
        } else if (DesiredFactor >= 2.67) {
            AgcR3060 =  0x0018;
            AgcGain = 2.67;
        } else if (DesiredFactor >= 2.46) {
            AgcR3060 =  0x0016;
            AgcGain = 2.46;
        } else if (DesiredFactor >= 2.29) {
            AgcR3060 =  0x0014;
            AgcGain = 2.29;
        } else if (DesiredFactor >= 2.13) {
            AgcR3060 =  0x0012;
            AgcGain = 2.13;
        } else if (DesiredFactor >= 2) {
            AgcR3060 =  0x0010;
            AgcGain = 2.0;
        } else if (DesiredFactor >= 1.88) {
            AgcR3060 =  0x000F;
            AgcGain = 1.88;
        } else if (DesiredFactor >= 1.78) {
            AgcR3060 =  0x000E;
            AgcGain = 1.78;
        } else if (DesiredFactor >= 1.68) {
            AgcR3060 =  0x000D;
            AgcGain = 1.68;
        } else if (DesiredFactor >= 1.6) {
            AgcR3060 =  0x000C;
            AgcGain = 1.6;
        } else {
            AgcR3060 =  0x000B;
            AgcGain = 1.52;
        }
    }
    if(DesiredFactor < 39.81) {
        DgcGain = DesiredFactor / AgcGain;
        Dgc305E = DgcGain * 133;
    }

    *pAnalogGainCtrl  = (ConvGain << 31) + AgcR3060;
    *pDigitalGainCtrl  = Dgc305E;
    *pActualFactor = (UINT32)(1024 * 1024 * AgcGain * DgcGain);

    B5_AR0230Ctrl.GainFactor = *pActualFactor;
    B5_AR0230Ctrl.AnalogGain = AgcGain;
    B5_AR0230Ctrl.DigitalGain = DgcGain;
    //AmbaPrint("DesiredFactor = %f, DgcR305E = 0x%08x, AgcR3060 = 0x%08x, ActualFactor = %d",DesiredFactor, *pDigitalGainCtrl, *pAnalogGainCtrl, *pActualFactor);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ConvertWbGainFactor
 *
 *  @Description:: Convert HDR WB gain factor to WB gain control
 *
 *  @Input      ::
 *      Chan:             Vin ID and sensor ID
 *      DesiredFactor:      Desired WB gain factor
 *
 *  @Output     ::
 *      pWBGainCtrl:        HDR WB gain control
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_ConvertWbGainFactor(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_WB_GAIN_s DesiredWBGainFactor, AMBA_SENSOR_WB_GAIN_s *pActualWBGainFactor,  AMBA_SENSOR_WB_CTRL_s *pWBGainCtrl)
{
    float Dgc = 0.0;
    UINT16 DgcReg = 0;

    B5_AR0230DesiredWBGainFactor.R = DesiredWBGainFactor.R;
    B5_AR0230DesiredWBGainFactor.Gr = DesiredWBGainFactor.Gr;
    B5_AR0230DesiredWBGainFactor.B = DesiredWBGainFactor.B;
    B5_AR0230DesiredWBGainFactor.Gb = DesiredWBGainFactor.Gb;

    Dgc = DesiredWBGainFactor.R;
    DgcReg = (UINT16)(Dgc * 133);
    pWBGainCtrl->R = DgcReg;
    pActualWBGainFactor->R = (float)DgcReg / 133.0;

    Dgc = DesiredWBGainFactor.Gr;
    DgcReg = (UINT16)(Dgc * 133);
    pWBGainCtrl->Gr = DgcReg;
    pActualWBGainFactor->Gr = (float)DgcReg / 133.0;

    Dgc = DesiredWBGainFactor.Gb;
    DgcReg = (UINT16)(Dgc * 133);
    pWBGainCtrl->Gb = DgcReg;
    pActualWBGainFactor->Gb = (float)DgcReg / 133.0;

    Dgc = DesiredWBGainFactor.B;
    DgcReg = (UINT16)(Dgc * 133);
    pWBGainCtrl->B = DgcReg;
    pActualWBGainFactor->B = (float)DgcReg / 133.0;

    //AmbaPrint("DesiredWBGainFactor R:%f, Gr:%f, Gb:%f, B:%f", DesiredWBGainFactor.R, DesiredWBGainFactor.Gr, DesiredWBGainFactor.Gb, DesiredWBGainFactor.B);
    //AmbaPrint("WBGainCtrl R:%d, Gr:%d, Gb:%d, B:%d", pWBGainCtrl.R, pWBGainCtrl.Gr, pWBGainCtrl.Gb, pWBGainCtrl.B);
    //AmbaPrint("ActualWBGainFactor R:%f, Gr:%f, Gb:%f, B:%f", pActualWBGainFactor.R, pActualWBGainFactor.Gr, pActualWBGainFactor.Gb, pActualWBGainFactor.B);

    return OK;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_ConvertShutterSpeed
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
static int B5_AR0230_ConvertShutterSpeed(AMBA_DSP_CHANNEL_ID_u Chan, float ExposureTime, UINT32 *pShutterCtrl)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo = &B5_AR0230Ctrl.Status.ModeInfo;
    const AMBA_DSP_FRAME_RATE_s *pFrameRate = &B5_AR0230ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming.FrameRate;
    UINT32 NumExposureStepPerFrame = 0;

    if (pShutterCtrl == NULL)
        return NG;

    /*Using default NumExposureStepPerFrame to convert exposure line*/
    if(B5_AR0230ModeInfoList[pModeInfo->Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_INTERLEAVE)
        NumExposureStepPerFrame = (B5_AR0230ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming.FrameLengthLines / 2);
    else
        NumExposureStepPerFrame = B5_AR0230ModeInfoList[pModeInfo->Mode.Bits.Mode].FrameTiming.FrameLengthLines;

    /*---------------------------------------------------------------------------------*\
     * Number of Exposure Steps = (ExposureTime / FrameTime) * NumExposureStepPerFrame
     *                          = ExposureTime * FrameRate * NumExposureStepPerFrame
    \*---------------------------------------------------------------------------------*/
    *pShutterCtrl = (UINT32)(ExposureTime * pFrameRate->TimeScale / pFrameRate->NumUnitsInTick * NumExposureStepPerFrame);


    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetAnalogGainCtrl
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
static int B5_AR0230_SetAnalogGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 AnalogGainCtrl)
{
    UINT16 Data = (UINT16) AnalogGainCtrl;
    UINT8 ConvGain = ((AnalogGainCtrl & 0x80000000) >> 31);

    if(B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_LINEAR) {
        if(ConvGain)
            B5_AR0230_SetConversionGain(&B5_AR0230_CurChan, B5_AR0230_CONVERSION_GAIN_LINEAR_HIGHDCG);
        else
            B5_AR0230_SetConversionGain(&B5_AR0230_CurChan, B5_AR0230_CONVERSION_GAIN_LINEAR_LOWDCG);
    } else {
        if(ConvGain)
            B5_AR0230_SetConversionGain(&B5_AR0230_CurChan, B5_AR0230_CONVERSION_GAIN_HDR_HIGHDCG);
        else
            B5_AR0230_SetConversionGain(&B5_AR0230_CurChan, B5_AR0230_CONVERSION_GAIN_HDR_LOWDCG);
    }

    B5_AR0230_CurChan.SensorID = Chan.Bits.SensorID;

    B5_AR0230_Write(&B5_AR0230_CurChan, 0x3060, Data);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetHdrAnalogGainCtrl
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
static int B5_AR0230_SetHdrAnalogGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 *AnalogGainCtrl)
{
    if (((Chan.Bits.HdrID & 0x1) && (Chan.Bits.HdrID & 0x2)) || (Chan.Bits.HdrID & 0x1)) {
        UINT32 Agc = *AnalogGainCtrl;
        UINT16 Data = (UINT16) Agc;
        UINT8 ConvGain = ((Agc & 0x80000000) >> 31);

        if(B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_LINEAR) {
            if(ConvGain)
                B5_AR0230_SetConversionGain(&B5_AR0230_CurChan, B5_AR0230_CONVERSION_GAIN_LINEAR_HIGHDCG);
            else
                B5_AR0230_SetConversionGain(&B5_AR0230_CurChan, B5_AR0230_CONVERSION_GAIN_LINEAR_LOWDCG);
        } else {
            if(ConvGain)
                B5_AR0230_SetConversionGain(&B5_AR0230_CurChan, B5_AR0230_CONVERSION_GAIN_HDR_HIGHDCG);
            else
                B5_AR0230_SetConversionGain(&B5_AR0230_CurChan, B5_AR0230_CONVERSION_GAIN_HDR_LOWDCG);
        }

        B5_AR0230_Write(&B5_AR0230_CurChan, 0x3060, Data);
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetDigitalGainCtrl
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
static int B5_AR0230_SetDigitalGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 DigitalGainCtrl)
{
    UINT16 Data = (UINT16) DigitalGainCtrl;
    B5_AR0230_Write(&B5_AR0230_CurChan, 0x305E, Data);
    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetHdrDigitalGainCtrl
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
static int B5_AR0230_SetHdrDigitalGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 *DigitalGainCtrl)
{
    UINT32 Dgc = *DigitalGainCtrl;
    B5_AR0230_Write(&B5_AR0230_CurChan, 0x3056, (UINT16)(B5_AR0230DesiredWBGainFactor.Gr * Dgc));
    B5_AR0230_Write(&B5_AR0230_CurChan, 0x3058, (UINT16)(B5_AR0230DesiredWBGainFactor.B * Dgc));
    B5_AR0230_Write(&B5_AR0230_CurChan, 0x305A, (UINT16)(B5_AR0230DesiredWBGainFactor.R * Dgc));
    B5_AR0230_Write(&B5_AR0230_CurChan, 0x305C, (UINT16)(B5_AR0230DesiredWBGainFactor.Gb * Dgc));

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetShutterCtrl
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
static int B5_AR0230_SetShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 ShutterCtrl)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo = &B5_AR0230Ctrl.Status.ModeInfo;
    UINT32 MinShrWidth = 0;
    UINT32 MaxShrWidth = 0;
    UINT16 T1T2Ratio = 1;
    UINT16 R0x3082 = 0;
    UINT16 R0x300A = 0;
    UINT16 Data = 0;

    B5_AR0230_CurChan.SensorID = Chan.Bits.SensorID;

    if(B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_LINEAR) {
        MaxShrWidth= pModeInfo->NumExposureStepPerFrame - 4;
        MinShrWidth = 1;
        //AmbaPrint("[Linear]MaxShrWidth:%d, MinShrWidth,:%d, ShutterCtrl:%d",MaxShrWidth,MinShrWidth,ShutterCtrl);
    } else if (B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_HDR_ALTM ||
        B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_HDR_LINEAR){
        B5_AR0230_RegRead(&B5_AR0230_CurChan, 0x3082, &R0x3082);
        T1T2Ratio = 1 << (((R0x3082 & 0x000c) >> 2) + 2);

        if (T1T2Ratio == 32)
            MinShrWidth = T1T2Ratio * 2;
        else
            MinShrWidth = T1T2Ratio;

        MaxShrWidth = 70 * T1T2Ratio;
        if(MaxShrWidth > pModeInfo->NumExposureStepPerFrame - 70)
            MaxShrWidth = pModeInfo->NumExposureStepPerFrame -70;
        MaxShrWidth -= 6;
        //AmbaPrint("[HDR]T1T2Ratio:%d, MaxShrWidth:%d, MinShrWidth,:%d, ShutterCtrl:%d",T1T2Ratio,MaxShrWidth,MinShrWidth,ShutterCtrl);
    } else if (B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_INTERLEAVE){
        B5_AR0230_RegRead(&B5_AR0230_CurChan, 0x3082, &R0x3082);
        T1T2Ratio = 1 << (((R0x3082 & 0x000c) >> 2) + 2);
        B5_AR0230_RegRead(&B5_AR0230_CurChan, 0x300a, &R0x300A);

        if (T1T2Ratio == 32)
            MinShrWidth = T1T2Ratio * 2;
        else
            MinShrWidth = T1T2Ratio;

        MaxShrWidth = R0x300A * T1T2Ratio / (T1T2Ratio + 1);
        if(MaxShrWidth > pModeInfo->NumExposureStepPerFrame - 70)
            MaxShrWidth = pModeInfo->NumExposureStepPerFrame -70;
        MaxShrWidth -= 6;
        //AmbaPrint("[Interleave]T1T2Ratio:%d, MaxShrWidth:%d, MinShrWidth,:%d, ShutterCtrl:%d",T1T2Ratio,MaxShrWidth,MinShrWidth,ShutterCtrl);
    }

    if(ShutterCtrl > MaxShrWidth)
        ShutterCtrl = MaxShrWidth;
    if(ShutterCtrl < MinShrWidth)
        ShutterCtrl = MinShrWidth;

    Data = (UINT16)ShutterCtrl;

    B5_AR0230_Write(&B5_AR0230_CurChan, 0x3012, Data);

    B5_AR0230Ctrl.ShutterCtrl = ShutterCtrl;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetHdrShutterCtrl
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
static int B5_AR0230_SetHdrShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 *ShutterCtrl)
{
    if (((Chan.Bits.HdrID & 0x1) && (Chan.Bits.HdrID & 0x2)) || (Chan.Bits.HdrID & 0x1)) {
        AMBA_SENSOR_MODE_INFO_s *pModeInfo = &B5_AR0230Ctrl.Status.ModeInfo;
        UINT32 MinShrWidth = 0;
        UINT32 MaxShrWidth = 0;
        UINT32 Shr = *ShutterCtrl;
        UINT16 T1T2Ratio = 1;
        UINT16 R0x3082 = 0;
        UINT16 R0x300A = 0;
        UINT16 Data = 0;

        if (B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_HDR_ALTM ||
            B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_HDR_LINEAR){
            B5_AR0230_RegRead(&B5_AR0230_CurChan, 0x3082, &R0x3082);
            T1T2Ratio = 1 << (((R0x3082 & 0x000c) >> 2) + 2);

            if (T1T2Ratio == 32)
                MinShrWidth = T1T2Ratio * 2;
            else
                MinShrWidth = T1T2Ratio;

            MaxShrWidth = 70 * T1T2Ratio ;
            if(MaxShrWidth > pModeInfo->NumExposureStepPerFrame - 70)
                MaxShrWidth = pModeInfo->NumExposureStepPerFrame -70;
            MaxShrWidth -= 6;
            //AmbaPrint("[HDR]T1T2Ratio:%d, MaxShrWidth:%d, MinShrWidth,:%d, ShutterCtrl:%d",T1T2Ratio,MaxShrWidth,MinShrWidth,Shr);
        } else if (B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_INTERLEAVE){
            B5_AR0230_RegRead(&B5_AR0230_CurChan, 0x3082, &R0x3082);
            T1T2Ratio = 1 << (((R0x3082 & 0x000c) >> 2) + 2);
            B5_AR0230_RegRead(&B5_AR0230_CurChan, 0x300a, &R0x300A);

            if (T1T2Ratio == 32)
                MinShrWidth = T1T2Ratio * 2;
            else
                MinShrWidth = T1T2Ratio;

            MaxShrWidth = R0x300A * T1T2Ratio / (T1T2Ratio + 1);
            if(MaxShrWidth > pModeInfo->NumExposureStepPerFrame - 70)
                MaxShrWidth = pModeInfo->NumExposureStepPerFrame -70;
            MaxShrWidth -= 6;
            //AmbaPrint("[Interleave]T1T2Ratio:%d, MaxShrWidth:%d, MinShrWidth,:%d, ShutterCtrl:%d",T1T2Ratio,MaxShrWidth,MinShrWidth,Shr);
        }

        if(Shr > MaxShrWidth)
            Shr = MaxShrWidth;
        if(Shr < MinShrWidth)
            Shr = MinShrWidth;

        Data = (UINT16)Shr;

        B5_AR0230_Write(&B5_AR0230_CurChan, 0x3012, Data);

        B5_AR0230Ctrl.ShutterCtrl = Shr;
    }
    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName::B5_AR0230_SetHdrWbGainCtrl
 *
 *  @Description:: Set HDR WB gain control
 *
 *  @Input      ::
 *      Chan:         Vin ID and sensor ID
 *      pWBGainCtrl:  HDR WB gain control
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_SetHdrWbGainCtrl(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_WB_CTRL_s *pWBGainCtrl)
{
    B5_AR0230_Write(&B5_AR0230_CurChan, 0x3056, (UINT16)(pWBGainCtrl->Gr * B5_AR0230Ctrl.DigitalGain));
    B5_AR0230_Write(&B5_AR0230_CurChan, 0x3058, (UINT16)(pWBGainCtrl->B * B5_AR0230Ctrl.DigitalGain));
    B5_AR0230_Write(&B5_AR0230_CurChan, 0x305A, (UINT16)(pWBGainCtrl->R * B5_AR0230Ctrl.DigitalGain));
    B5_AR0230_Write(&B5_AR0230_CurChan, 0x305C, (UINT16)(pWBGainCtrl->Gb * B5_AR0230Ctrl.DigitalGain));

    return OK;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_SetSlowShutterCtrl
 *
 *  @Description:: set slow shutter control
 *
 *  @Input      ::
 *      Chan:           Vin ID and sensor ID
 *      SlowShutterCtrl:   Electronic slow shutter control
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int B5_AR0230_SetSlowShutterCtrl(AMBA_DSP_CHANNEL_ID_u Chan, UINT32 SlowShutterCtrl)
{
    UINT16 SensorMode = B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode;
    if (SlowShutterCtrl < 1)
        SlowShutterCtrl = 1;

    B5_AR0230_CurChan.SensorID = Chan.Bits.SensorID;

    if (B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_HDR_ALTM ||
        B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_HDR_LINEAR){

        UINT32 TargetLineLengthPck = B5_AR0230ModeInfoList[SensorMode].FrameTiming.Linelengthpck * SlowShutterCtrl / 2;

        B5_AR0230Ctrl.Status.ModeInfo.LineLengthPck = TargetLineLengthPck * 2;
        B5_AR0230Ctrl.Status.ModeInfo.FrameTime.FrameRate.NumUnitsInTick = B5_AR0230ModeInfoList[SensorMode].FrameTiming.FrameRate.NumUnitsInTick * SlowShutterCtrl;

        B5_AR0230_Write(&B5_AR0230_CurChan, 0x300C, (UINT16)TargetLineLengthPck);
        //AmbaPrint("[AR0230]HDR slowshutter Linelengthpck:%d, SlowShutterCtrl:%d, TargetLineLengthPck:%d", B5_AR0230ModeInfoList[SensorMode].FrameTiming.Linelengthpck, SlowShutterCtrl, TargetLineLengthPck);
    } else {

        UINT32 TargetFrameLengthLines = B5_AR0230ModeInfoList[SensorMode].FrameTiming.FrameLengthLines * SlowShutterCtrl;

        B5_AR0230Ctrl.Status.ModeInfo.FrameLengthLines = TargetFrameLengthLines;
        B5_AR0230Ctrl.Status.ModeInfo.FrameTime.FrameRate.NumUnitsInTick = B5_AR0230ModeInfoList[SensorMode].FrameTiming.FrameRate.NumUnitsInTick * SlowShutterCtrl;

        if(B5_AR0230ModeInfoList[B5_AR0230Ctrl.Status.ModeInfo.Mode.Bits.Mode].OperationMode == B5_AR0230_OPERATION_MODE_INTERLEAVE)
            TargetFrameLengthLines /= 2;

        B5_AR0230Ctrl.Status.ModeInfo.NumExposureStepPerFrame = TargetFrameLengthLines;
        B5_AR0230_Write(&B5_AR0230_CurChan, 0x300A, (UINT16)TargetFrameLengthLines);
        //AmbaPrint("[AR0230]Linear slowshutter FrameLengthLines:%d, SlowShutterCtrl:%d, TargetFrameLengthLines:%d", B5_AR0230ModeInfoList[SensorMode].FrameTiming.FrameLengthLines, SlowShutterCtrl, TargetFrameLengthLines);
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_PowerOptimization
 *
 *  @Description:: B5 power optimization
 *
 *  @Input      ::
 *      pB5Chan:    B5 channel config
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
static void B5_AR0230_PowerOptimization(AMBA_B5_CHANNEL_s *pB5Chan)
{
    UINT32 DataBuf[1];
    UINT32 Addr;
    AMBA_B5_CHIP_ID_u ChipID;

    ChipID.Data = pB5Chan->SensorID;

    /* B5N */
    DataBuf[0] = 0x00d90f55;
    Addr = AMBA_B5_AHB_BASE_ADDR + 0xf8;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0x80000a21;
    Addr = AMBA_B5_AHB_BASE_ADDR + 0x114;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0x13fd0080;
    Addr = AMBA_B5_AHB_BASE_ADDR + 0x118;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    //DataBuf[0] = 0x6ea1fd48;
    DataBuf[0] = 0x43e05d5e;
    Addr = AMBA_B5_AHB_BASE_ADDR + 0x134;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0xf;
    Addr = AMBA_B5_AHB_BASE_ADDR + 0x138;
    AmbaB5N_RegWrite(Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    /* B5F */
    DataBuf[0] = 0xd908aa; // turn off b5f tx
    Addr = AMBA_B5_AHB_BASE_ADDR + 0xf8;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0x80000ae7;
    Addr = AMBA_B5_AHB_BASE_ADDR + 0x114;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0x10013480; // turn off b5f 10 lvds tx
    Addr = AMBA_B5_AHB_BASE_ADDR + 0x118;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    //DataBuf[0] = 0x6e14f481;
    DataBuf[0] = 0x6a14b481;
    Addr = AMBA_B5_AHB_BASE_ADDR + 0x134;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);

    DataBuf[0] = 0x4;
    Addr = AMBA_B5_AHB_BASE_ADDR + 0x138;
    AmbaB5F_RegWrite(ChipID, Addr, 0, AMBA_B5_DATA_WIDTH_32BIT, 1, DataBuf);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: B5_AR0230_Config
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
static int B5_AR0230_Config(AMBA_DSP_CHANNEL_ID_u Chan, AMBA_SENSOR_MODE_ID_u Mode, AMBA_SENSOR_ESHUTTER_TYPE_e ElecShutterMode)
{
    AMBA_SENSOR_MODE_INFO_s *pModeInfo;
    B5_AR0230_MODE_INFO_s *pPrivateModeInfo;
    UINT16 SensorMode = Mode.Bits.Mode;
    static int NotFirstBoot = 0;

    if (SensorMode >= AMBA_SENSOR_B5_AR0230_NUM_MODE)
        return NG;

    AmbaPrint("============  [ B5_AR0230 ]  ============");
    AmbaPrint("============ init mode:%d ============", SensorMode);

    B5_AR0230Ctrl.Status.ElecShutterMode = ElecShutterMode;

    /* Hack for Second VIN (only for testing) */
    //Chan.Bits.VinID = 1;

    AmbaVIN_Reset((AMBA_VIN_CHANNEL_e)Chan.Bits.VinID, AMBA_VIN_LVDS, NULL);

    AmbaB5_PllSwPllDisable(); // Disable sw pll

    if (NotFirstBoot == 1) {
        AmbaB5_VoutReset();
        AmbaB5_VinReset(&B5_AR0230_CurChan);
        NotFirstBoot = 1;
    }

    B5_AR0230_CurChanConvert(Chan, &B5_AR0230_CurChan); // Convert CurChan based on SysChan and Chan

    //if (NotFirstBoot == 0) {
    if (1) { // hardware reset B5N/B5F each time
        int InitRval = 0;
        UINT8 BrokenFlag = 0;
        AmbaB5_SetPwmBrokenFlag(0); // Must reset the broken flag to 0
ReInit:
        B5_AR0230_HardwareReset(Chan);
        InitRval = AmbaB5_Init(&B5_AR0230_SysChan, &B5_AR0230_CurChan, &B5_AR0230_PinConfig);

        if (InitRval == NG) {
            BrokenFlag = AmbaB5_GetPwmBrokenFlag();
            //AmbaPrint("BrokenFlag = %d", BrokenFlag);
            Chan.Bits.SensorID = Chan.Bits.SensorID & ~BrokenFlag; // Block broken channels
            B5_AR0230_CurChanConvert(Chan, &B5_AR0230_CurChan); // Re-convert CurChan(passing broken channels)
            goto ReInit;
        }

        AmbaKAL_TaskSleep(10);
        AmbaB5_SetPwmBrokenFlag(BrokenFlag); // Set the actual broken flag(flag before re-init)
        SensorAeSem = AmbaB5_PllGetSwPllSem();
        AmbaB5_PllSwPllShowMsg(0); // Disable sw pll debug message
    }

    pModeInfo = &B5_AR0230Ctrl.Status.ModeInfo;
    pPrivateModeInfo = &B5_AR0230Ctrl.ModeInfo;
    B5_AR0230_PrepareModeInfo(AmbaB5_GetNumActiveChannel(&B5_AR0230_CurChan), SensorMode, pModeInfo, pPrivateModeInfo);

    AmbaB5_PllSetSensorClk(&B5_AR0230_CurChan, B5_AR0230Ctrl.ModeInfo.FrameTiming.InputClk); // Must be set before programing sensor
    AmbaKAL_TaskSleep(3);

    if (1) { // hardware reset sensor each time
        B5_AR0230_SensorHardwareReset();
    }

    B5_AR0230_SoftwareReset(&B5_AR0230_CurChan);

    B5_AR0230_SensorModeInit(&B5_AR0230_CurChan, SensorMode);

    B5_AR0230_PowerOptimization(&B5_AR0230_CurChan);

    AmbaB5_VinPhySetSLVS(&B5_AR0230_CurChan);

    //AmbaPrint("RecordingPixels:%d, CfaOutputWidth;%d",B5_AR0230OutputInfo[B5_AR0230ModeInfoList[SensorMode].OutputMode].RecordingPixels.Width, pPrivateModeInfo->CfaOutputWidth);
    //AmbaPrint("OutputHeight:%d, NumDataBits;%d",pModeInfo->OutputInfo.OutputHeight, pModeInfo->OutputInfo.NumDataBits);
    AmbaB5_PrescalerInit(&B5_AR0230_CurChan, B5_AR0230OutputInfo[B5_AR0230ModeInfoList[SensorMode].OutputMode].RecordingPixels.Width, pPrivateModeInfo->CfaOutputWidth, 0);
    AmbaB5_Enable(&B5_AR0230_CurChan, pPrivateModeInfo->CfaOutputWidth, pModeInfo->OutputInfo.OutputHeight, pModeInfo->OutputInfo.NumDataBits, B5_AR0230_CompressRatio);
    AmbaB5_PllSwPllConfig(pPrivateModeInfo->FrameTiming.FrameRate.TimeScale /
                          pPrivateModeInfo->FrameTiming.FrameRate.NumUnitsInTick);

    B5_AR0230_SetStandbyOff(&B5_AR0230_CurChan);
    B5_AR0230_ConfigVin(Chan, pModeInfo, pPrivateModeInfo);

    AmbaB5_PllSwPllEnable(); // Enable sw pll

    B5_AR0230Ctrl.Status.DevStat.Bits.Sensor0Standby = 0;

    return OK;
}

/*-----------------------------------------------------------------------------------------------*/
AMBA_SENSOR_OBJ_s AmbaSensor_B5_AR0230Obj = {
    .Init                   = B5_AR0230_Init,
    .Enable                 = B5_AR0230_Enable,
    .Disable                = B5_AR0230_Disable,
    .Config                 = B5_AR0230_Config,
    .ChangeFrameRate        = B5_AR0230_ChangeFrameRate,
    .GetStatus              = B5_AR0230_GetStatus,
    .GetModeInfo            = B5_AR0230_GetModeInfo,
    .GetDeviceInfo          = B5_AR0230_GetDeviceInfo,
    .GetCurrentGainFactor   = B5_AR0230_GetCurrentGainFactor,
    .GetCurrentShutterSpeed = B5_AR0230_GetCurrentShutterSpeed,
    .ConvertGainFactor      = B5_AR0230_ConvertGainFactor,
    .ConvertShutterSpeed    = B5_AR0230_ConvertShutterSpeed,
    .ConvertWbGainFactor    = B5_AR0230_ConvertWbGainFactor,
    .SetHdrWbGainCtrl       = B5_AR0230_SetHdrWbGainCtrl,
    .SetAnalogGainCtrl      = B5_AR0230_SetAnalogGainCtrl,
    .SetDigitalGainCtrl     = B5_AR0230_SetDigitalGainCtrl,
    .SetShutterCtrl         = B5_AR0230_SetShutterCtrl,
    .SetSlowShutterCtrl     = B5_AR0230_SetSlowShutterCtrl,
    .SetGainFactor          = B5_AR0230_SetGainFactor,
    .SetEshutterSpeed       = B5_AR0230_SetEshutterSpeed,
    .SetHdrAnalogGainCtrl   = B5_AR0230_SetHdrAnalogGainCtrl,
    .SetHdrDigitalGainCtrl  = B5_AR0230_SetHdrDigitalGainCtrl,
    .SetHdrShutterCtrl      = B5_AR0230_SetHdrShutterCtrl,
    .RegisterRead           = B5_AR0230_RegisterRead,
    .RegisterWrite          = B5_AR0230_RegisterWrite,
    .GetHdrInfo             = B5_AR0230_GetHdrInfo,
};
