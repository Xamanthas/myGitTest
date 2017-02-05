/**
  * @file src/app/sample/unittest/AmpUT_StillTuning.c
  *
  * still Tuning unit test
  *
  * History:
  *    2013/10/22 - [Edgar Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include "AmpUnitTest.h"
#include "AmpUT_Input.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vin/vin.h>
#include <recorder/Encode.h>
#include <recorder/VideoEnc.h>
#include <recorder/StillEnc.h>
#include <display/Osd.h>
#include <display/Display.h>
#include "AmbaUtility.h"
#include "AmbaDSP_VIN.h"
#include "AmbaDSP_ImgFilter.h"
#include "AmbaDSP_ImgUtility.h"
#include "AmbaDSP_ImgHighIsoFilter.h"
#include <AmbaDSP_WarpCore.h>
#include "AmpUT_StillIsoConfig.h"
#include "AmbaDSP.h"
#include "AmbaDSP_VOUT.h"
#include <scheduler.h>
#include "AmbaCache.h"
#include <cfs/AmpCfs.h>
#include "AmbaImgCalibItuner.h"
#include "AmbaTUNE_HdlrManager.h"
#include <AmbaSensor.h>
#include <AmbaCalibInfo.h>
#include "AmbaLCD.h"
#include "AmpUT_Display.h"
#include <AmbaImg_Proc.h>
#include <AmbaImg_Impl_Cmd.h>
#include <AmbaImg_VIn_Handler.h>
#include <AmbaImg_VDsp_Handler.h>
#include <AmbaImg_Adjustment_Def.h>
#include <AmbaImg_AaaDef.h>
#include <util.h>
#ifdef CONFIG_SOC_A9
#include <AmbaImg_AeAwb.h>
#endif

static char DefaultSlot[] = "C";


void *UT_StillTunningEncodefopen(const char *pFileName, const char *pMode)
{
    AMP_CFS_FILE_PARAM_s cfsParam;
    AmpCFS_GetFileParam(&cfsParam);

    if (pMode[0] == 'w' && pMode[1] == '+') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_WRITE_READ;
    } else if (pMode[0] == 'w') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_WRITE_ONLY;
    } else if (pMode[0] == 'r' && pMode[1] == '+') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_READ_WRITE;
    } else if (pMode[0] == 'r') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    } else if (pMode[0] == 'a' && pMode[1] == '+') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_APPEND_READ;
    } else if (pMode[0] == 'a') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_APPEND_ONLY;
    }

    if(AmbaFS_GetCodeMode() == AMBA_FS_UNICODE) {
        //strcpy(cfsParam.Filename, pFileName);
        //return (void *)AmpCFS_fopen(&cfsParam);
        AmbaPrint("[%s]: Unicode should be abandoned", __func__);
        return NULL;
    } else {
        strcpy((char *)cfsParam.Filename, pFileName);
        return (void *)AmpCFS_fopen(&cfsParam);
    }
}

int UT_StillTunningEncodefclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_StillTunningEncodefwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_StillTunningEncodefsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

int AmpUT_StillTuningDumpAoi(UINT8 *Addr, UINT16 Pitch, AMP_AREA_s Aoi, void *Fp)
{
    UINT8 *StartAddress = Addr;
    UINT16 i;
    StartAddress += (Aoi.X + Aoi.Y*Pitch);
    for (i=0; i<Aoi.Height; i++) {
        UT_StillTunningEncodefwrite((const void *)StartAddress, 1, Aoi.Width, Fp);
        StartAddress += Pitch;
    }

    return OK;
}

UINT32 UT_StillTuningFrameRateIntConvert(UINT32 OriFrameRate)
{
    UINT32 FrameRateInt = OriFrameRate;

    if (OriFrameRate==29 || OriFrameRate==59 || OriFrameRate==119 || OriFrameRate==239) {
        FrameRateInt++;
    }

    return FrameRateInt;
}


#ifdef CONFIG_SOC_A12
extern int AmpUT_GetYuvWorkingBuffer(UINT16 MainWidth, UINT16 MainHeight, UINT16 RawWidth, UINT16 RawHeight, UINT16 *BufWidth, UINT16 *BufHeight);
#endif

// global var for StillTuning
static UINT32 StillTuningPicCnt = 1;
static UINT32 StillTuningRawCnt = 1;
static UINT32 StillTuningYuvCnt = 1;
static UINT32 StillTuningCfaCnt = 1;
static UINT32 StillTuningTimeout = 120000;
static UINT8 StillTuningDumpItuner = 0;   // Dump ituner text file or not
static UINT8 StillTuningDumpSkip = 0x1;
#define STILL_TUNING_SKIP_JPG   (0x1)
/* still codec flags */
static UINT8 StillTuningCodecInit = 0;
static UINT8 StillTuningLCDLiveview = 1;
static UINT8 StillTuningTvLiveview = 1;
static UINT8 StillTuning3AEnable = 1;
static UINT8 StillTuningRawCaptureRunning = 0;
static UINT8 StillTuningBGProcessing = 0;
static UINT8 AutoBackToLiveview = 0;
static UINT8 StillTuningOutputFlag = 0;
static UINT8 StillRawEncodeNeedPreLoad = 0;
static UINT8 StillRawEncodePreLoadDone = 0;
static UINT8 StillRawEncodeNeedJpegOutput = 1;
static AMP_ROTATION_e StillTuningRotate = AMP_ROTATE_0;
static UINT16 TileNumber = 1;

static AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s StillTuningItunerRawCapCtrl;

static AMBA_KAL_EVENT_FLAG_t StillTuningUTTaskFlag;
#define STILL_TUNING_BG_PROCESS_DONE   (0x1)
#define STILL_TUNING_RAW_CAPTURE_DONE  (0x2)
#define STILL_TUNING_JPEG_DATA_READY   (0x3)

/* still codec HDLRs */
static AMP_VIN_HDLR_s *StillTuningVinA = NULL;
static AMP_STLENC_HDLR_s *StillTuningPri = NULL;
AMP_ENC_PIPE_HDLR_s *StillTuningPipe = NULL;
static UINT8 *StillTuningVinWorkBuf = NULL;         // Vin module working buffer
static UINT8 *StillTuningImgSchdlrWorkBuf = NULL;   // Img scheduler working buffer
static UINT8 *StillTuningWorkBuf = NULL;            // StillTuning working buffer
static AMBA_DSP_CHANNEL_ID_u TuningChannel;
#define TUNING_STATUS_STILL_IDLE                1
#define TUNING_STATUS_STILL_INIT                2
#define TUNING_STATUS_STILL_LIVEVIEW            3
static UINT8 Status = TUNING_STATUS_STILL_IDLE;

#ifdef CONFIG_LCD_WDF9648W
extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;
#endif

static UINT8 StillTuningLiveViewProcMode = 0;             // LiveView Process Mode, 0: Express 1:Hybrid
static UINT8 StillTuningLiveViewAlgoMode = 0;             // LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
static UINT8 StillTuningLiveViewOSMode = 0;               // LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 StillTuningEncodeSystem = 0;                 // Encode System, 0:NTSC, 1: PAL

static UINT8 StillTuningIsIqParamInit = 0;
static UINT8 StillTuningIsHdrIqParam = 0;

static UINT16 CustomIdspFreq = AMP_SYSTEM_FREQ_POWERSAVING;
static UINT16 CustomCoreFreq = AMP_SYSTEM_FREQ_POWERSAVING;
static UINT16 CustomCortexFreq = AMP_SYSTEM_FREQ_POWERSAVING;
#ifdef CONFIG_SOC_A12
static UINT16 CustomMaxIdspFreq = A12_IDSP_PLL_MAX;
static UINT16 CustomMaxCoreFreq = A12_CORE_PLL_MAX;
static UINT16 CustomMaxCortexFreq = A12_CORTEX_PLL_MAX;
#else //A9
static UINT16 CustomMaxIdspFreq = 504;
static UINT16 CustomMaxCoreFreq = 396;
static UINT16 CustomMaxCortexFreq = 800;
#endif
static UINT8 DspSuspendEnable = 0;
static UINT8 WirelessModeEnable = 0;

// Image scheduler
AMBA_IMG_SCHDLR_HDLR_s *STuningImgSchdlr = NULL;

// Ituner Information
ITUNER_SYSTEM_s StillTuningSystemInfo;
ITUNER_INFO_s StillTuningItunerInfo;

/* Single capture buffers */
static UINT8 *ScriptAddr = NULL;
static UINT8 *OriScriptAddr = NULL;
static UINT8 *RawBuffAddr = NULL;
static UINT8 *DstRawBuffAddr = NULL;
static UINT8 *YuvBuffAddr = NULL;
static UINT8 *ScrnBuffAddr = NULL;
static UINT8 *ThmBuffAddr = NULL;
static UINT8 *Raw3ARoiBuffAddr = NULL;
static UINT8 *Raw3AStatBuffAddr = NULL;
#ifdef CONFIG_SOC_A12
static UINT8 *QvLCDBuffAddr = NULL;
static UINT8 *QvHDMIBuffAddr = NULL;
#endif

typedef enum _STILL_TUNING_MODE_e_ {
    STILL_TUNING_RAWENC,
    STILL_TUNING_RAWCAP,
    STILL_TUNING_UNKNOWN = 0xFF
} STILL_TUNING_MODE_e;
static STILL_TUNING_MODE_e TuningMode = STILL_TUNING_RAWENC;

static UINT8 QvDisplayCfg = 1;
static UINT8 QvLcdEnable = 0;
static UINT8 QvTvEnable = 0;
static UINT8 QvLcdShowBuffIndex = 0;
static UINT8 QvTvShowBuffIndex = 0;
#ifdef CONFIG_SOC_A9
static UINT8 *QvLcdShowBuffAddrOdd = NULL;
static UINT8 *QvLcdShowBuffAddrEven = NULL;
static UINT8 *QvTvShowBuffAddrOdd = NULL;
static UINT8 *QvTvShowBuffAddrEven = NULL;
#endif
/* QuickView show Time Table */
#define MAX_QV_SHOW_TIME_ENTRY  4
static int QvShowTimeTable[MAX_QV_SHOW_TIME_ENTRY] = {
    1000,       /* 1 second */
    3000,       /* 3 second */
    5000,       /* 5 second */
    0xFFFFFFFF, /* Forever */
};
static AMBA_KAL_TIMER_t QvTimerHdlr;
static int QvTimerHdlrID = -1;
static UINT32 QvTimerID = 0;
typedef enum _AMPUT_STILL_VOUT_SRC_e_ {
    VOUT_SRC_DEFAULT_IMG = 0,    /* Default Image */
    VOUT_SRC_BACK_COLOR,         /* Mixer Background Color */
    VOUT_SRC_VIDEO_CAPTURE,      /* Video Plane */
} AMPUT_STILL_VOUT_SRC_e;
static AMBA_DSP_VOUT_VIDEO_SRC_e TuningVoutLcdSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;
static AMBA_DSP_VOUT_VIDEO_SRC_e TuningVoutTvSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;

#define AMPUT_STILLTUNING_MAX_ENC_ID   2

static INPUT_ENC_MGT_s *StillTuningMgt = NULL;
static UINT8 InputDeviceId = 0;
static UINT8 StillTuningModeIdx = 0;

#define AMPUT_STILLTUNING_MAX_VOUT_ID   2
static struct {
    UINT16 OffsetX;
    UINT16 OffsetY;
    UINT16 Width;
    UINT16 Height;
} StillTuningVoutMgt[AMPUT_STILLTUNING_MAX_VOUT_ID][AMPUT_STILLTUNING_MAX_ENC_ID] = {
    /* LCD */
    [0] = {
        /* 4:3 */
        [0] = {
            .OffsetX = 0,
            .OffsetY = 0,
            .Width = 960,
            .Height = 480,
        },
        /* 16:9 */
        [1] = {
            .OffsetX = 0,
            .OffsetY = 0,
            .Width = 960,
            .Height = 360,
        },
    },
    /* HDMI */
    [1] = {
        /* 4:3 */
        [0] = {
            .OffsetX = 0,
            .OffsetY = 0,
            .Width = 1440,
            .Height = 1080,
        },
        /* 16:9 */
        [1] = {
            .OffsetX = 0,
            .OffsetY = 0,
            .Width = 1920,
            .Height = 1080,
        },
    },
};

static UINT8 StillTuningMultiChan = 0;
static UINT8 StillTuningMultiChanMainViewID = 0;

/** Description of the real sensors mounted on B5F */
/* First MainViewWindow */
static AMBA_DSP_CHANNEL_ID_u SensorChannel1st[1] = {
    [0] = {
        .Bits.VinID = 0,
        .Bits.SensorID = 1,
    },
};
static AMP_AREA_s SensorCaptureWindow1st[1] = {
    [0] = {
        .X = 0, .Y = 0,
        .Width = 1920, .Height = 1080,
    },
};

/* Second MainViewWindow */
static AMBA_DSP_CHANNEL_ID_u SensorChannel2nd[1] = {
    [0] = {
        .Bits.VinID = 0,
        .Bits.SensorID = 2,
    },
};
static AMP_AREA_s SensorCaptureWindow2nd[1] = {
    [0] = {
        .X = 0, .Y = 0,
        .Width = 1920, .Height = 1080,
    },
};

/* Third MainViewWindow */
static AMBA_DSP_CHANNEL_ID_u SensorChannel3rd[1] = {
    [0] = {
        .Bits.VinID = 0,
        .Bits.SensorID = 4,
    },
};
static AMP_AREA_s SensorCaptureWindow3rd[1] = {
    [0] = {
        .X = 0, .Y = 0,
        .Width = 1920, .Height = 1080,
    },
};

/* Fourth MainViewWindow */
static AMBA_DSP_CHANNEL_ID_u SensorChannel4th[1] = {
    [0] = {
        .Bits.VinID = 0,
        .Bits.SensorID = 8,
    },
};
static AMP_AREA_s SensorCaptureWindow4th[1] = {
    [0] = {
        .X = 0, .Y = 0,
        .Width = 1920, .Height = 1080,
    },
};

#define STILL_TUNNING_MAX_MAIN_WINDOW_VIEW 4
#define SUR_MAIN_WIDTH 992
#define SUR_MAIN_HEIGHT 576

static AMP_MULTI_CHAN_MAIN_WINDOW_CFG_s MultiChanMainWindow[STILL_TUNNING_MAX_MAIN_WINDOW_VIEW] = {
#if 0 //virtual
    [0] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = SensorChannel1st,
        .SensorCapWin = SensorCaptureWindow1st,
        .MainViewID = 0,
        .MainCapWin = {.X = 0, .Y = 0, .Width = 1920, .Height = 1080},
        .MainWidth = 1280,
        .MainHeight = 720,
    },
    [1] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = SensorChannel2nd,
        .SensorCapWin = SensorCaptureWindow2nd,
        .MainViewID = 1,
        .MainCapWin = {.X = 1920, .Y = 0, .Width = 1920, .Height = 1080},
        .MainWidth = 1280,
        .MainHeight = 720,
    },
    [2] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = SensorChannel3rd,
        .SensorCapWin = SensorCaptureWindow3rd,
        .MainViewID = 2,
        .MainCapWin = {.X = 0, .Y = 1080, .Width = 1920, .Height = 1080},
        .MainWidth = 1280,
        .MainHeight = 720,
    },
    [3] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = SensorChannel4th,
        .SensorCapWin = SensorCaptureWindow4th,
        .MainViewID = 3,
        .MainCapWin = {.X = 1920, .Y = 1080, .Width = 1920, .Height = 1080},
        .MainWidth = 1280,
        .MainHeight = 720,
    },
#else
    /* Liveview */
    [0] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = SensorChannel1st,
        .SensorCapWin = SensorCaptureWindow1st,
        .MainViewID = 0,
        .MainCapWin = {.X = 0, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = SUR_MAIN_WIDTH, /* 992 */
        .MainHeight = SUR_MAIN_HEIGHT, /* 576*/
    },
    [1] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = SensorChannel2nd,
        .SensorCapWin = SensorCaptureWindow2nd,
        .MainViewID = 1,
        .MainCapWin = {.X = 1920, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = SUR_MAIN_WIDTH, /* 992 */
        .MainHeight = SUR_MAIN_HEIGHT, /* 576*/
    },
    [2] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = SensorChannel3rd,
        .SensorCapWin = SensorCaptureWindow3rd,
        .MainViewID = 2,
        .MainCapWin = {.X = 3840, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = SUR_MAIN_WIDTH, /* 992 */
        .MainHeight = SUR_MAIN_HEIGHT, /* 576*/
    },
    [3] = {
        .Enable = 1,
        .ChannelNumber = 1,
        .ChannelID = SensorChannel4th,
        .SensorCapWin = SensorCaptureWindow4th,
        .MainViewID = 3,
        .MainCapWin = {.X = 5760, .Y = 0, .Width = 1280, .Height = 720},
        .MainWidth = SUR_MAIN_WIDTH, /* 992 */
        .MainHeight = SUR_MAIN_HEIGHT, /* 576*/
    },
#endif
};

#ifdef CONFIG_SOC_A9
// Window handler
static AMP_DISP_WINDOW_HDLR_s *TuningLcdWinHdlr = NULL;
static AMP_DISP_WINDOW_HDLR_s *TuningTvWinHdlr = NULL;
#endif

/* Simple still_ut */
static AMBA_KAL_TASK_t StillTuningUtTask = {0};
static char *AmpUT_StillTuningUtStack;
#define STILL_TUNING_MUX_TASK_STACK_SIZE (8192)
static AMBA_KAL_TASK_t StillTuningMuxTask = {0};
static void *AmpUT_StillTuningMuxStack = NULL;
#define STILL_TUNING_UT_TASK_STACK_SIZE (8192)
static AMBA_KAL_MSG_QUEUE_t TuningUtMsgQueue;
#define MAX_AMPUT_STILLTUNING_UT_MSG_NUM  64
typedef struct _STILLTUNING_UT_MSG_S_ {
    UINT32 Event;
    union {
        UINT32 DataInfo[2];
        AMP_ENC_YUV_INFO_s yuvInfo;
    } Data;
} STILLTUNING_UT_MSG_S;
static STILLTUNING_UT_MSG_S *AmpUT_StillTuningUtMsgQPool;

#define STILL_TUNING_BISFIFO_SIZE (16*1024*1024)
#define STILL_TUNING_DESC_SIZE (32*128)
static void *StillTuningBitsBuf = NULL;
static AMP_FIFO_HDLR_s *StillTuningVirtualFifoHdlr = NULL;
static UINT8 *StillTuningDescBuf = NULL;
static AMBA_KAL_SEM_t StillTuningSem = {0};
static AMP_CFS_FILE_s *StillTuningOutputFile = NULL;

//define UT message
#define MSG_STILLTUNING_UT_TIMER_TIMEOUT       0x0000001
#define MSG_STILLTUNING_UT_QV_LCD_READY        0x0000002
#define MSG_STILLTUNING_UT_QV_TV_READY         0x0000003
#define MSG_STILLTUNING_UT_RAW_CAPTURE_DONE    0x0000004
#define MSG_STILLTUNING_UT_BG_PROCESS_DONE     0x0000005

/*  Function prototype*/
void AmpUT_StillTuning_MuxTask(UINT32 info);
int AmpUT_StillTuning_Init(int sensorID, int LcdID);
int AmpUT_StillTuning_ChangeResolution(UINT32 encID);
UINT32 AmpUT_StillTuningAAALock(void);
UINT32 AmpUT_StillTuningUnLockAE(void);
UINT32 AmpUT_ItunerRawEncodePOSTCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_ItunerRawCapturePOSTCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_ItunerRawEncodePRECB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_ItunerRawCapturePRECB(AMP_STILLENC_PREP_INFO_s *info);

static int AmpUT_StillTuningCallback(void *hdlr,UINT32 event, void *info);
static int AmpUT_StillTuning_FifoCB(void *hdlr, UINT32 event, void* info);
static int AmpUT_StillTuningPipeCallback(void *hdlr,UINT32 event, void *info);

static AMP_STILLENC_POSTP_s post_ItunerRawEncode_cb = {.Process = AmpUT_ItunerRawEncodePOSTCB};
//static AMP_STILLENC_POSTP_s post_ItunerRawCapture_cb = {.Process = AmpUT_ItunerRawCapturePOSTCB};
static AMP_STILLENC_PREP_s pre_ItunerRawEncode_cb = {.Process = AmpUT_ItunerRawEncodePRECB};
//static AMP_STILLENC_PREP_s pre_ItunerRawCapture_cb = {.Process = AmpUT_ItunerRawCapturePRECB};

int AmpUT_StillTuningDumpItunerFile(char *pItunerFileName, char *pTuningModeExt, char *pRawPath)
{
    AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};

    if (TuningMode == STILL_TUNING_RAWCAP) {
        UINT32 ImgIpChNo = 0, AeIdx = 0;
        ITUNER_SYSTEM_s ItunerSysInfo = {0};
        ITUNER_AE_INFO_s ItunerAeInfo = {0};
        AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
        AMBA_AE_INFO_s StillAeInfo[MAX_AEB_NUM];

        snprintf(ItunerSysInfo.ChipRev, sizeof(ItunerSysInfo.ChipRev), "A12");
        snprintf(ItunerSysInfo.TuningModeExt, sizeof(ItunerSysInfo.TuningModeExt), pTuningModeExt);
        memcpy(ItunerSysInfo.RawPath, pRawPath, sizeof(ItunerSysInfo.RawPath));

        AmbaSensor_GetStatus(TuningChannel, &SensorStatus);
        ItunerSysInfo.RawBayer = SensorStatus.ModeInfo.OutputInfo.CfaPattern;
        ItunerSysInfo.RawResolution = SensorStatus.ModeInfo.OutputInfo.NumDataBits;
        ItunerSysInfo.HSubSampleFactorDen = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorDen;
        ItunerSysInfo.HSubSampleFactorNum = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorNum;
        ItunerSysInfo.VSubSampleFactorDen = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorDen;
        ItunerSysInfo.VSubSampleFactorNum = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorNum;
        ItunerSysInfo.RawStartX = SensorStatus.ModeInfo.InputInfo.PhotodiodeArray.StartX; // Unit in pixel. Before down-sample.
        ItunerSysInfo.RawStartY = SensorStatus.ModeInfo.InputInfo.PhotodiodeArray.StartY; // Unit in pixel. Before down-sample.
        ItunerSysInfo.CompressedRaw = 0;

        if (StillTuningMultiChan) {
            ItunerSysInfo.RawWidth = MultiChanMainWindow[0].MainCapWin.Width;
            ItunerSysInfo.RawHeight = MultiChanMainWindow[0].MainCapWin.Height;
        } else {
            ItunerSysInfo.RawWidth = SensorStatus.ModeInfo.OutputInfo.RecordingPixels.Width;
            ItunerSysInfo.RawHeight = SensorStatus.ModeInfo.OutputInfo.RecordingPixels.Height;
        }
        ItunerSysInfo.RawPitch = ALIGN_32(ItunerSysInfo.RawWidth*2);
        AmbaTUNE_Set_SystemInfo(&ItunerSysInfo);

        AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, ImgIpChNo, IP_MODE_STILL, (UINT32)StillAeInfo);
        ItunerAeInfo.EvIndex = StillAeInfo[AeIdx].EvIndex;
        ItunerAeInfo.NfIndex = StillAeInfo[AeIdx].NfIndex;
        ItunerAeInfo.ShutterIndex = StillAeInfo[AeIdx].ShutterIndex;
        ItunerAeInfo.AgcIndex = StillAeInfo[AeIdx].AgcIndex;
        ItunerAeInfo.IrisIndex = StillAeInfo[AeIdx].IrisIndex;
        ItunerAeInfo.Dgain = StillAeInfo[AeIdx].Dgain;
        ItunerAeInfo.IsoValue = StillAeInfo[AeIdx].IsoValue;
        ItunerAeInfo.Flash = StillAeInfo[AeIdx].Flash;
        ItunerAeInfo.Mode = StillAeInfo[AeIdx].Mode;
        ItunerAeInfo.ShutterTime = (INT32) StillAeInfo[AeIdx].ShutterTime;
        ItunerAeInfo.AgcGain = (INT32) StillAeInfo[AeIdx].AgcGain;
        ItunerAeInfo.Target = StillAeInfo[AeIdx].Target;
        ItunerAeInfo.LumaStat = StillAeInfo[AeIdx].LumaStat;
        ItunerAeInfo.LimitStatus = StillAeInfo[AeIdx].LimitStatus;
        ItunerAeInfo.Multiplier = 0; // FIXME:
        AmbaTUNE_Set_AeInfo(&ItunerAeInfo);

        ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
        ImgMode.ConfigId = 0;
        ImgMode.ContextId = 0;
    } else {
        ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
        ImgMode.AlgoMode = StillTuningItunerInfo.TuningAlgoMode.AlgoMode;
        ImgMode.ConfigId = 0;
        ImgMode.ContextId = 0;
    }

    if (NULL != pItunerFileName) {
        TUNE_Save_Param_s SaveParam = {0};
        SaveParam.Text.Filepath = pItunerFileName;
        if (0 != AmbaTUNE_Save_IDSP(&ImgMode, &SaveParam)) {
            AmbaPrintColor(RED,"TextHdlr_Save_IDSP() failed");
        } else {
            AmbaPrint("[Amp_UT] Dump ituner text to %s done", pItunerFileName);
        }
    }

    return 0;
}

/**
 * UnitTest: Calculate dsp working address and size base on current capture mode
 *
 * @param [in] return dsp reserved working area address
 * @param [in] return dsp reserved working area size
 *
 * @return 0 - success, -1 - fail
 */
static int AmpUT_StillTuning_DspWork_Calculate(UINT8 **addr, UINT32 *size)
{
    extern UINT8 *DspWorkAreaResvStart;
    extern UINT32 DspWorkAreaResvSize;
    int Er = 0;
    UINT32 TotalSize = 0;
#ifdef CONFIG_SOC_A12
    UINT8 ArIndex = 0;

    if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;
#endif

    if (TuningMode == STILL_TUNING_UNKNOWN) {
        //temp only support raw2yuv, allocate raw+yuv/scrn/thm buffer
        //LiveView only
        (*addr) = DspWorkAreaResvStart;
        (*size) = DspWorkAreaResvSize;
    } else if (TuningMode == STILL_TUNING_RAWENC) {
        //allocate YUV buffer
        UINT16 RawPitch = 0, RawHeight = 0, RawWidth = 0;
        UINT16 DstRawWidth = 0, DstRawHeight = 0, DstRawPitch = 0;
        UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
        UINT32 RawSize = 0, DstRawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
    #ifdef CONFIG_SOC_A12
        UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
        UINT32 QvLCDSize = 0, QvHDMISize = 0;
    #endif
        ITUNER_VIDEO_HDR_RAW_INFO_s DstRawInfo = {0};

        RawPitch = StillTuningSystemInfo.RawPitch;
    #ifdef CONFIG_SOC_A12
        RawWidth = StillTuningSystemInfo.RawWidth;
    #else
        (void) RawWidth;
    #endif
        RawHeight = StillTuningSystemInfo.RawHeight;

        RawSize = RawPitch*RawHeight;

        AmbaItuner_Get_VideoHdrRawInfo(&DstRawInfo);
        DstRawWidth = DstRawInfo.RawAreaWidth[0];
        DstRawHeight = DstRawInfo.RawAreaHeight[0];
        DstRawPitch = DstRawWidth<<1; // We only have uncompressed dst raw now
        DstRawSize = (UINT32) DstRawPitch*DstRawHeight;

    #ifdef CONFIG_SOC_A9
        YuvWidth = StillTuningSystemInfo.MainWidth;
        YuvHeight = StillTuningSystemInfo.MainHeight;
        YuvSize = YuvWidth*YuvHeight*2;
        YuvSize += (YuvSize*10)/100;
    #else
        AmpUT_GetYuvWorkingBuffer(StillTuningSystemInfo.MainWidth,\
            StillTuningSystemInfo.MainHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
        YuvSize = YuvWidth*YuvHeight*2;
    #endif
        ScrnW = ALIGN_32(StillTuningMgt[StillTuningModeIdx].ScrnWidth);
        ScrnH = ALIGN_16(StillTuningMgt[StillTuningModeIdx].ScrnHeight);
        ScrnH += ScrnH/10;
        ScrnSize = ScrnW*ScrnH*2;
        ScrnSize += (ScrnSize*10)/100;
        ThmW = ALIGN_32(StillTuningMgt[StillTuningModeIdx].ThmWidth);
        ThmH = ALIGN_16(StillTuningMgt[StillTuningModeIdx].ThmHeight);
        ThmH += ThmH/10;
        ThmSize = ThmW*ThmH*2;
        ThmSize += (ThmSize*10)/100;
    #ifdef CONFIG_SOC_A12
        QvLCDW = ALIGN_32(StillTuningVoutMgt[0][ArIndex].Width);
        QvLCDH = ALIGN_16(StillTuningVoutMgt[0][ArIndex].Height);
        QvHDMIW = ALIGN_32(StillTuningVoutMgt[1][ArIndex].Width);
        QvHDMIH = ALIGN_16(StillTuningVoutMgt[1][ArIndex].Height);
        QvLCDSize = QvLCDW*QvLCDH*2;
        QvLCDSize += (QvLCDSize*10)/100;
        QvHDMISize = QvHDMIW*QvHDMIH*2;
        QvHDMISize += (QvHDMISize*10)/100;

        TotalSize = RawSize + DstRawSize + YuvSize + ScrnSize + ThmSize + QvLCDSize*1 + QvHDMISize*1;
    #else
        TotalSize = RawSize + YuvSize + ScrnSize + ThmSize;
    #endif
        if (TotalSize > DspWorkAreaResvSize) {
            AmbaPrintColor(RED,"[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, DspWorkAreaResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            Er = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }
    } else if (TuningMode == STILL_TUNING_RAWCAP) {
        //allocate Raw buffer
        UINT16 RawPitch = 0, RawHeight = 0;
        UINT32 RawSize = 0;
        AMBA_SENSOR_MODE_INFO_s modeInfo;

        AmbaSensor_GetModeInfo(TuningChannel, StillTuningItunerRawCapCtrl.SensorMode, &modeInfo);
        RawPitch = ALIGN_32(modeInfo.OutputInfo.RecordingPixels.Width*2);
        RawHeight = modeInfo.OutputInfo.RecordingPixels.Height;
        RawSize = RawPitch*RawHeight;
        TotalSize = RawSize;
        if (TotalSize > DspWorkAreaResvSize) {
            AmbaPrintColor(RED,"[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, DspWorkAreaResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            Er = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }
    }

    AmbaPrintColor(BLUE,"[DspWork_Calculate] Addr 0x%X ~ 0x%X, Sz %u, req %d", *addr, *addr+*size, *size, TotalSize);
    return Er;
}

/* qv timer handler */
static void AmpUT_qvTimer_Handler(UINT32 exinf)
{
    STILLTUNING_UT_MSG_S Msg = {0};

    /* TimeUP!! */
    Msg.Event = MSG_STILLTUNING_UT_TIMER_TIMEOUT;
    Msg.Data.DataInfo[0] = exinf; //TimerID
    AmbaPrint("[qvTmr_Hdlr]Snd 0x%X (%d)!", Msg.Event, Msg.Data.DataInfo[0]);
    AmbaKAL_MsgQueueSend(&TuningUtMsgQueue, &Msg, AMBA_KAL_NO_WAIT);
}


int AmpUT_StillTuning_LiveviewStop(void)
{
    UINT32 Flag = AMP_ENC_FUNC_FLAG_WAIT;

    if (Status != TUNING_STATUS_STILL_LIVEVIEW)
        return -1;

    if (STuningImgSchdlr) {
        AmbaImgSchdlr_Enable(STuningImgSchdlr, 0);
    }
    Amba_Img_VIn_Invalid(StillTuningPri, (UINT32 *)NULL);

    if (WirelessModeEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        DspSuspendEnable = 0;
    } else if (DspSuspendEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        WirelessModeEnable = 0;
    }
    AmpEnc_StopLiveview(StillTuningPipe, Flag);

    if (AmpResource_GetVoutSeamlessEnable() == 0) {
        if (StillTuningLCDLiveview) {
            AmpUT_Display_Stop(0);
        }
    }

    if (StillTuningTvLiveview) {
        AmpUT_Display_Stop(1);
    }

    return 0;
}

/* CB for raw capture */
UINT32 AmpUT_StillTuningRawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl)
{
    AMBA_DSP_WINDOW_s VinCapture = {0};
    AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
    AMBA_SENSOR_AREA_INFO_s *RecPixels = &SensorStatus.ModeInfo.OutputInfo.RecordingPixels;
    AMP_STILLENC_RAWCAP_DSP_CTRL_s DspCtrl = {0};
    AMBA_AE_INFO_s StillAeInfo[MAX_AEB_NUM];
    AMBA_DSP_IMG_WB_GAIN_s WbGain = {0};
    UINT8 ExposureFrames = 0;
    UINT32 ImgIpChNo = 0, GainFactor = 0, AGainCtrl = 0, DGainCtrl = 0, ShutterCtrl = 0;

    /* Stop LiveView capture */
    AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_CAPTURE, 0);

    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, ImgIpChNo, IP_MODE_STILL, (UINT32)StillAeInfo);
    AmbaPrintColor(GREEN, "AE info[%d]: EV:%d NF:%d ShIdx:%d AgcIdx:%d IrisIdx:%d Dgain:%d ISO:%d Flash:%d Mode:%d ShTime:%f AgcGain:%f", \
        ctrl->AeIdx, \
        StillAeInfo[ctrl->AeIdx].EvIndex, StillAeInfo[ctrl->AeIdx].NfIndex, \
        StillAeInfo[ctrl->AeIdx].ShutterIndex, StillAeInfo[ctrl->AeIdx].AgcIndex, \
        StillAeInfo[ctrl->AeIdx].IrisIndex, StillAeInfo[ctrl->AeIdx].Dgain, \
        StillAeInfo[ctrl->AeIdx].IsoValue, StillAeInfo[ctrl->AeIdx].Flash, \
        StillAeInfo[ctrl->AeIdx].Mode, \
        StillAeInfo[ctrl->AeIdx].ShutterTime, StillAeInfo[ctrl->AeIdx].AgcGain);
    AmbaImg_Proc_Cmd(MW_IP_GET_WB_GAIN, ImgIpChNo, (UINT32)&WbGain, 0);

    DspCtrl.VinChan = ctrl->VinChan;
    DspCtrl.VidSkip = 0;
    DspCtrl.RawCapNum = ctrl->TotalRawToCap;

    /* Program sensor */
    AmbaSensor_Config(ctrl->VinChan, ctrl->SensorMode, ctrl->ShType);
    AmbaSensor_GetStatus(ctrl->VinChan, &SensorStatus);
    AmbaSensor_ConvertGainFactor(ctrl->VinChan, StillAeInfo[ctrl->AeIdx].AgcGain, &GainFactor, &AGainCtrl, &DGainCtrl);
    AmbaSensor_SetAnalogGainCtrl(ctrl->VinChan, AGainCtrl);
    AmbaSensor_SetDigitalGainCtrl(ctrl->VinChan, DGainCtrl);
    AmbaSensor_ConvertShutterSpeed(ctrl->VinChan, StillAeInfo[ctrl->AeIdx].ShutterTime, &ShutterCtrl);
    ExposureFrames = (ShutterCtrl/SensorStatus.ModeInfo.NumExposureStepPerFrame);
    ExposureFrames = (ShutterCtrl%SensorStatus.ModeInfo.NumExposureStepPerFrame)? ExposureFrames+1: ExposureFrames;
    AmbaSensor_SetSlowShutterCtrl(ctrl->VinChan, ExposureFrames);
    AmbaSensor_SetShutterCtrl(ctrl->VinChan, ShutterCtrl);
    AmbaPrint("RawCapCB: (sensor) %d %d", ctrl->SensorMode.Bits.Mode, ctrl->ShType);

    /* Program Vin */
    VinCapture.Width = ctrl->VcapWidth;
    VinCapture.Height = ctrl->VcapHeight;
    if (StillTuningMultiChan) {
        VinCapture.OffsetX = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.X;
        VinCapture.OffsetY = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.Y;
    } else {
        VinCapture.OffsetX = (RecPixels->StartX + ((RecPixels->Width - VinCapture.Width)/2)) & 0xFFFE;
        VinCapture.OffsetY = (RecPixels->StartY + ((RecPixels->Height - VinCapture.Height)/2)) & 0xFFFE;
    }
    AmbaPrint("RawCapCB: (vin) %d %d %d %d", VinCapture.Width, VinCapture.Height, VinCapture.OffsetX, VinCapture.OffsetY);
    AmbaDSP_VinCaptureConfig(0, &VinCapture);

    /* set still idsp param */
    //TBD

    /* calc warp and execute ISO config */
    //TBD
    DspCtrl.StillProc = 2; //Hack as FastStillMode

    AmpStillEnc_StartRawCapture(StillTuningPri, &DspCtrl);

    return 0;
}

/**
 * UnitTest: Ituner raw capture postproc callback
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_ItunerRawCapturePOSTCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    if (info->StageCnt == 1) {
        AMP_STILLENC_POSTP_INFO_s PpInfo = {0};
        PpInfo.media.RawInfo.compressed = info->media.RawInfo.compressed;
        PpInfo.media.RawInfo.RawAddr = info->media.RawInfo.RawAddr;
        PpInfo.media.RawInfo.RawPitch = info->media.RawInfo.RawPitch;
        PpInfo.media.RawInfo.RawWidth = info->media.RawInfo.RawWidth;
        PpInfo.media.RawInfo.RawHeight = info->media.RawInfo.RawHeight;
        if (StillTuningItunerRawCapCtrl.RawBuff.PostProc) {
            StillTuningItunerRawCapCtrl.RawBuff.PostProc->Process(&PpInfo);
            //release raw buffers
            if (RawBuffAddr) {
                RawBuffAddr = NULL;
            }
        }
    } else {
        //nothing to do
    }
    return 0;
}

/**
 * UnitTest: Ituner raw capture preproc callback
 *
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_ItunerRawCapturePRECB(AMP_STILLENC_PREP_INFO_s *info)
{
    //nothing to do
    return 0;
}

/**
 * UnitTest: raw capture for Ituner
 *
 * @param [in] OutputFlag indicate output type
 *                       0x1 : need Raw
 *                       0x2 : need Yuv
 *                       0x4 : need Jpeg
 * @param [in] ItunerRawCapCtrl indicate ituner raw captur information
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_ItunerRawCapture(UINT8 OutputFlag, AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s ItunerRawCapCtrl)
{
    int Er = OK;
    void *TempPtr = NULL, *TempRawPtr = NULL;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT32 RawSize = 0;
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0, ActualFlag = 0;
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt;
    AMP_SENC_SCRPT_RAWCAP_s *RawCapScrpt;
    AMP_SCRPT_CONFIG_s Scrpt = {0};

    if (StillTuningRawCaptureRunning) {
        AmbaPrint("Error status %d", Status);
        goto _DONE;
    }

    // 20140625 Chester: 3A need this to provide AE for still capture
    AmpUT_StillTuningAAALock();

    TuningMode = STILL_TUNING_RAWCAP;
    AmbaKAL_EventFlagClear(&StillTuningUTTaskFlag, STILL_TUNING_BG_PROCESS_DONE);
    AmbaKAL_EventFlagClear(&StillTuningUTTaskFlag, STILL_TUNING_RAW_CAPTURE_DONE);

    StillTuningOutputFlag = OutputFlag;
    memset(&StillTuningItunerRawCapCtrl, 0x0, sizeof(AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s));
    memcpy(&StillTuningItunerRawCapCtrl, &ItunerRawCapCtrl, sizeof(AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s));


    /* Step1. calc raw buffer memory */
    if (StillTuningMultiChan) {
        RawPitch = MultiChanMainWindow[0].MainCapWin.Width*2;
        RawPitch = ALIGN_32(RawPitch);
        RawWidth = MultiChanMainWindow[0].MainCapWin.Width;
        RawHeight = MultiChanMainWindow[0].MainCapWin.Height;
    } else {
        AMBA_SENSOR_MODE_INFO_s modeInfo;
        AmbaSensor_GetModeInfo(TuningChannel, ItunerRawCapCtrl.SensorMode, &modeInfo);
        RawPitch = modeInfo.OutputInfo.RecordingPixels.Width*2;
        RawPitch = ALIGN_32(RawPitch);
        RawWidth = modeInfo.OutputInfo.RecordingPixels.Width;
        RawHeight = modeInfo.OutputInfo.RecordingPixels.Height;
    }
    RawSize = RawPitch*RawHeight;

    /* Step2. allocate raw buffer address, script address */
    if (StillTuningItunerRawCapCtrl.RawBufSource == 1) {
        UINT32 inputRawSize = ALIGN_32(StillTuningItunerRawCapCtrl.RawBuff.Raw.RawPitch)*StillTuningItunerRawCapCtrl.RawBuff.Raw.RawHeight;
        if (inputRawSize < RawSize) {
            AmbaPrint("[ItunerRawEnc]RawBufferSize Error!!");
            Er = NG;
            goto _DONE;
        }

        RawPitch = ALIGN_32(StillTuningItunerRawCapCtrl.RawBuff.Raw.RawPitch);
        RawWidth = StillTuningItunerRawCapCtrl.RawBuff.Raw.RawWidth;
        RawHeight = StillTuningItunerRawCapCtrl.RawBuff.Raw.RawHeight;

        AmbaPrint("[ItunerRawEnc]raw(%u %u %u)", RawPitch, RawWidth, RawHeight);
        RawBuffAddr = StillTuningItunerRawCapCtrl.RawBuff.Raw.RawAddr;
        AmbaPrint("[ItunerRawEnc]rawBuffAddr (0x%08X)", RawBuffAddr);
    } else {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        UINT8 *bufAddr;

        AmbaPrint("[ItunerRawEnc]raw(%u %u %u)", RawPitch, RawWidth, RawHeight);
        Er = AmpUT_StillTuning_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        if (Er == -1) goto _DONE;

        bufAddr = dspWorkAddr + dspWorkSize;
        RawBuffAddr = bufAddr;
        AmbaPrint("[ItunerRawCap]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);
    }
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr,&TempRawPtr, ScriptSize, 32);
        if (Er != OK) {
            AmbaPrint("[ItunerRawCap]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)TempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[ItunerRawCap]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    StageAddr = ScriptAddr;
    GenScrpt = (AMP_SENC_SCRPT_GENCFG_s *)StageAddr;
    memset(GenScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    GenScrpt->Cmd = SENC_GENCFG;
    GenScrpt->RawEncRepeat = 0;
    GenScrpt->RawToCap = 1;
    GenScrpt->StillProcMode = 2; //Hack as FastStillMode

    GenScrpt->QVConfig.DisableLCDQV = 1;
    GenScrpt->QVConfig.DisableHDMIQV = 1;
    QvLcdEnable = 0;
    QvTvEnable = 0;
    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (GenScrpt->b2LVCfg)? 1: 0;
    GenScrpt->ScrnEnable = 0;
    GenScrpt->ThmEnable = 0;
    GenScrpt->PostProc = (StillTuningItunerRawCapCtrl.RawBufSource == 1)? \
        NULL: StillTuningItunerRawCapCtrl.RawBuff.PostProc;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum ++;
    AmbaPrint("[ItunerRawCap]Stage_0 0x%08X", StageAddr);

    //raw cap config
    StageAddr = ScriptAddr + TotalScriptSize;
    RawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)StageAddr;
    memset(RawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    RawCapScrpt->Cmd = SENC_RAWCAP;
    RawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    RawCapScrpt->ShType = ItunerRawCapCtrl.ShType;
    RawCapScrpt->SensorMode = ItunerRawCapCtrl.SensorMode;
    RawCapScrpt->FvRawCapArea.VcapWidth = RawWidth;
    RawCapScrpt->FvRawCapArea.VcapHeight = RawHeight;
    if (StillTuningMultiChan) {
        RawCapScrpt->FvRawCapArea.EffectArea.X = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.X;
        RawCapScrpt->FvRawCapArea.EffectArea.Y = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.Y;
    } else {
        RawCapScrpt->FvRawCapArea.EffectArea.X = RawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    }
    RawCapScrpt->FvRawCapArea.EffectArea.Width = RawCapScrpt->FvRawCapArea.VcapWidth;
    RawCapScrpt->FvRawCapArea.EffectArea.Height = RawCapScrpt->FvRawCapArea.VcapHeight;
    RawCapScrpt->FvRawType = AMP_STILLENC_RAW_UNCOMPR;
    RawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    RawCapScrpt->FvRawBuf.Width = RawWidth;
    RawCapScrpt->FvRawBuf.Height = RawHeight;
    RawCapScrpt->FvRawBuf.Pitch = RawPitch;
    RawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    RawCapScrpt->FvRingBufSize = RawSize*1;
    RawCapScrpt->CapCB.RawCapCB = AmpUT_StillTuningRawCapCB;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    TotalStageNum ++;
    AmbaPrint("[ItunerRawCap]Stage_1 0x%X", StageAddr);

    //script config
    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[ItunerRawCap]Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    StillTuningRawCaptureRunning = 1;
    StillTuningBGProcessing = 1;

    /* Step4. execute script */
    AmpEnc_RunScript(StillTuningPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step5. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (Scrpt)");
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");

    // 20141217 Chester: RAW_CAPTURE_DONE and BG_PROCESS_DONE only appear when we run script by no wait
    if (StillTuningOutputFlag & 0x1) {
        Er = AmbaKAL_EventFlagTake(&StillTuningUTTaskFlag, STILL_TUNING_RAW_CAPTURE_DONE, \
            AMBA_KAL_AND, &ActualFlag, 5000/*TimeOut*/);
        if (Er != OK) {
            AmbaPrintColor(RED, "[ItunerRawCap] Execute RawCap Script TimeOut!!");
            goto _DONE;
        } else {
            AmbaPrint("[ItunerRawCap] Wait Execute RawCap Script Done!!");
        }
    }

    if ((StillTuningOutputFlag & 0x2) || (StillTuningOutputFlag & 0x4)) {
        Er = AmbaKAL_EventFlagTake(&StillTuningUTTaskFlag, STILL_TUNING_BG_PROCESS_DONE, \
            AMBA_KAL_AND, &ActualFlag, 5000/*TimeOut*/);
        if (Er != OK) {
            AmbaPrintColor(RED, "[ItunerRawCap] Execute BG Script TimeOut!!");
            goto _DONE;
        } else {
            AmbaPrint("[ItunerRawCap] Wait Execute BG Script Done!!");
        }
    }

_DONE:
    return Er;
}

/**
 * UnitTest: Ituner raw encode postproc callback
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_ItunerRawEncodePOSTCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    if (info->StageCnt == 1) {
        if (info->IsRaw2Raw3A == 1) {
        #ifdef CONFIG_SOC_A12
            char CfaFileName[32];
            Ituner_Ext_File_Param_s ExtFileParam = {0};
            snprintf(CfaFileName, sizeof(CfaFileName), "%s:\\%u.txt", DefaultSlot, StillTuningCfaCnt);
            ExtFileParam.Stat_Save_Param.Address = (void *)info->media.CfaStatInfo;
            ExtFileParam.Stat_Save_Param.Size = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
            ExtFileParam.Stat_Save_Param.Target_File_Path = CfaFileName;
            AmbaTUNE_Save_Data(EXT_FILE_CFA_STAT, &ExtFileParam);
            StillTuningCfaCnt++;
        #endif
        } else if (info->IsRaw2RawResample == 1) {
            char RawFileName[32];
            Ituner_Ext_File_Param_s ExtFileParam = {0};
            snprintf(RawFileName, sizeof(RawFileName), "%s:\\%u.raw", DefaultSlot, StillTuningRawCnt);
            AmbaCache_Invalidate((void *)info->media.RawInfo.RawAddr, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight);
            ExtFileParam.Raw_Save_Param.Address = (void *)info->media.RawInfo.RawAddr;
            ExtFileParam.Raw_Save_Param.Size = (UINT32) info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight;
            ExtFileParam.Stat_Save_Param.Target_File_Path = RawFileName;
            AmbaTUNE_Save_Data(EXT_FILE_RAW, &ExtFileParam);
            StillTuningRawCnt++;
        } else {
            if (info->media.YuvInfo.LumaAddr) {
                char FnY[MAX_FILENAME_LENGTH], FnUV[MAX_FILENAME_LENGTH];

                Ituner_Ext_File_Param_s ExtFileParam = {0};

                if (0 == strlen(StillTuningSystemInfo.OutputFilename)) {
                    snprintf(FnY, sizeof(FnY), "%s:\\%u.y", DefaultSlot, StillTuningYuvCnt);
                    snprintf(FnUV, sizeof(FnUV),"%s:\\%u.uv", DefaultSlot, StillTuningYuvCnt);
                    StillTuningYuvCnt++;
                } else {
                    snprintf(FnY, sizeof(FnY), "%s:\\%s.y", DefaultSlot, StillTuningSystemInfo.OutputFilename);
                    snprintf(FnUV, sizeof(FnUV), "%s:\\%s.uv", DefaultSlot, StillTuningSystemInfo.OutputFilename);
                }

                AmbaPrint("[ItunerRawEnc] Dump YUV(%d) %s (0x%X 0x%X) %d %d %d Start!", \
                    info->media.YuvInfo.DataFormat, \
                    StillTuningSystemInfo.OutputFilename, \
                    info->media.YuvInfo.LumaAddr, \
                    info->media.YuvInfo.ChromaAddr, \
                    info->media.YuvInfo.Pitch, \
                    info->media.YuvInfo.Width, \
                    info->media.YuvInfo.Height);

                ExtFileParam.YUV_Save_Param.Y_Info.Address = (void *) info->media.YuvInfo.LumaAddr;
                ExtFileParam.YUV_Save_Param.Y_Info.Pitch = info->media.YuvInfo.Pitch;
                ExtFileParam.YUV_Save_Param.Y_Info.Width = info->media.YuvInfo.Width;
                ExtFileParam.YUV_Save_Param.Y_Info.Height = info->media.YuvInfo.Height;
                ExtFileParam.YUV_Save_Param.Y_Info.StartX = 0;
                ExtFileParam.YUV_Save_Param.Y_Info.StartY = 0;
                ExtFileParam.YUV_Save_Param.Y_Info.Target_File_Path = FnY;
                AmbaCache_Invalidate(ExtFileParam.YUV_Save_Param.Y_Info.Address, ExtFileParam.YUV_Save_Param.Y_Info.Pitch*ExtFileParam.YUV_Save_Param.Y_Info.Height);

                ExtFileParam.YUV_Save_Param.UV_Info.Address = (void *) info->media.YuvInfo.ChromaAddr;
                ExtFileParam.YUV_Save_Param.UV_Info.Pitch = info->media.YuvInfo.Pitch;
                ExtFileParam.YUV_Save_Param.UV_Info.Width = info->media.YuvInfo.Width;
                ExtFileParam.YUV_Save_Param.UV_Info.Height = info->media.YuvInfo.Height;
                ExtFileParam.YUV_Save_Param.UV_Info.StartX = 0;
                ExtFileParam.YUV_Save_Param.UV_Info.StartY = 0;
                ExtFileParam.YUV_Save_Param.UV_Info.Target_File_Path = FnUV;
                AmbaCache_Invalidate(ExtFileParam.YUV_Save_Param.UV_Info.Address, ExtFileParam.YUV_Save_Param.UV_Info.Pitch*ExtFileParam.YUV_Save_Param.UV_Info.Height);

                AmbaTUNE_Save_Data(EXT_FILE_YUV, &ExtFileParam);
                AmbaPrint("[ItunerRawEnc] Dump YUV Done!");
            }

            //release yuv buffers
            if (info->media.YuvInfo.ThmLumaAddr && ThmBuffAddr) {
                ThmBuffAddr = NULL;
            } else if (info->media.YuvInfo.ScrnLumaAddr && ScrnBuffAddr) {
                ScrnBuffAddr = NULL;
            } else if (info->media.YuvInfo.LumaAddr && YuvBuffAddr) {
                YuvBuffAddr = NULL;
            }

            if (ThmBuffAddr == NULL && ScrnBuffAddr == NULL && YuvBuffAddr == NULL) {
                //release raw buffers
                if (RawBuffAddr) {
                    RawBuffAddr = NULL;
                }
            }
        }

        if (StillTuningDumpItuner == 1) {
            char ItunerFileName[64];
            UINT8 FileCnt;
            FileCnt = (info->IsRaw2Raw3A)? StillTuningCfaCnt: (info->IsRaw2RawResample)? StillTuningRawCnt: StillTuningYuvCnt;
            snprintf(ItunerFileName, sizeof(ItunerFileName), "%s:\\A12_%04d.txt", DefaultSlot, FileCnt);
            AmpUT_StillTuningDumpItunerFile(ItunerFileName, NULL, NULL);
        }
    } else {
        //nothing to do
    }

    return 0;
}

/**
 * UnitTest: Ituner raw encode preproc callback
 *
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_ItunerRawEncodePRECB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 1) {
    #ifdef CONFIG_SOC_A12
        if (StillTuningSystemInfo.EnableRaw2Raw == 1) { // R2R case
            UINT16 RawWidth = 0, RawHeight = 0;
            AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
            AMBA_DSP_IMG_BLACK_CORRECTION_s StaticBlackLevel = {0};
            AMBA_DSP_IMG_AAA_STAT_INFO_s AaaStatInfo = {0};
            AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s CfaLeakage = {0};
            AMBA_DSP_IMG_DGAIN_SATURATION_s DgainSaturation = {0};
            AMBA_DSP_IMG_DBP_CORRECTION_s DbpCorr = {0};
            AMBA_DSP_IMG_SBP_CORRECTION_s SbpCorr = {0};
            AMBA_DSP_IMG_ANTI_ALIASING_s AntiAliasing = {0};

            RawWidth = StillTuningSystemInfo.RawWidth;
            RawHeight = StillTuningSystemInfo.RawHeight;

            ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
            if (StillTuningItunerInfo.TuningAlgoMode.AlgoMode == 2) {
                ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
            } else if (StillTuningItunerInfo.TuningAlgoMode.AlgoMode == 1) {
                ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
            }
            ImgMode.BatchId = AMBA_DSP_RAW_TO_RAW_FILTER;
            ImgMode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_RAW2RAW;
            ImgMode.ContextId = 0;
            ImgMode.ConfigId = 0;

            /*
             *  Cfa stat calculation will take following IDSP filter into account.
             *  And please NOTE that it should be setup by following sequence.
             *    1) Static black level
             *    2) AAA stat info
             *    3) CA warp
             *    4) Vignetter
             *    5) Cfa leakage
             *    6) Dgain saturation
             *    7) Dynamic bad pixel
             *    8) Static bad pixel
             *    9) Anti-aliasing
             *
             */
            AmbaDSP_ImgGetStaticBlackLevel(&ImgMode, &StaticBlackLevel);
            AmbaDSP_ImgSetStaticBlackLevel(&ImgMode, &StaticBlackLevel);

            AmbaDSP_Img3aGetAaaStatInfo(&ImgMode, &AaaStatInfo);
            AaaStatInfo.AeTileWidth = RawWidth/AaaStatInfo.AeTileNumCol;
            AaaStatInfo.AeTileHeight = RawHeight/AaaStatInfo.AeTileNumRow;
            AaaStatInfo.AfTileWidth = RawWidth/AaaStatInfo.AfTileNumCol;
            AaaStatInfo.AfTileHeight = RawHeight/AaaStatInfo.AfTileNumRow;
            AaaStatInfo.AfTileActiveWidth = AaaStatInfo.AfTileWidth;
            AaaStatInfo.AfTileActiveHeight = AaaStatInfo.AfTileHeight;
            AaaStatInfo.AwbTileWidth = RawWidth/AaaStatInfo.AwbTileNumCol;
            AaaStatInfo.AwbTileHeight = RawHeight/AaaStatInfo.AwbTileNumRow;
            AaaStatInfo.AwbTileActiveWidth = AaaStatInfo.AwbTileWidth;
            AaaStatInfo.AwbTileActiveHeight = AaaStatInfo.AwbTileHeight;
            AmbaDSP_Img3aSetAaaStatInfo(&ImgMode, &AaaStatInfo);

            AmbaDSP_ImgSetCawarpCompensation(&ImgMode);
            AmbaDSP_ImgSetVignetteCompensation(&ImgMode);

            AmbaDSP_ImgGetCfaLeakageFilter(&ImgMode,&CfaLeakage);
            AmbaDSP_ImgSetCfaLeakageFilter(&ImgMode,&CfaLeakage);

            AmbaDSP_ImgGetDgainSaturationLevel(&ImgMode,&DgainSaturation);
            AmbaDSP_ImgSetDgainSaturationLevel(&ImgMode,&DgainSaturation);

            AmbaDSP_ImgGetDynamicBadPixelCorrection(&ImgMode,&DbpCorr);
            AmbaDSP_ImgSetDynamicBadPixelCorrection(&ImgMode,&DbpCorr);

            AmbaDSP_ImgGetStaticBadPixelCorrection(&ImgMode,&SbpCorr);
            AmbaDSP_ImgSetStaticBadPixelCorrection(&ImgMode,&SbpCorr);

            AmbaDSP_ImgGetAntiAliasing(&ImgMode, &AntiAliasing);
            AmbaDSP_ImgSetAntiAliasing(&ImgMode, &AntiAliasing);
        }
    #endif
    }
    return 0;
}

int AmpUT_StillTuningGetRawEncodeBuffer(AMBA_DSP_RAW_BUF_s *rawBuf)
{
    int Er = OK;
    if (0 == StillRawEncodeNeedPreLoad) {
        UINT8 Ar = 0;
        UINT16 RawWidth = 0, RawHeight = 0, RawPitch = 0;
        UINT16 DstRawWidth = 0, DstRawHeight = 0, DstRawPitch = 0;
        UINT16 YuvMainWidth = 0, YuvMainHeight = 0;
        UINT16 YuvScrnWidth = 0, YuvScrnHeight = 0;
        UINT16 YuvThmWidth = 0, YuvThmHeight = 0;
        UINT16 QvLcdWidth = 0, QvLcdHeight = 0, QvHdmiWidth = 0, QvHdmiHeight = 0;
        UINT32 RawSize = 0, DstRawSize = 0;
        UINT32 YuvMainSize = 0, YuvScrnSize = 0, YuvThmSize = 0, QvLcdSize = 0, QvHdmiSize = 0;
        UINT32 RoiSize = 0, Raw3aSize = 0;
        UINT8 *DspWorkAddr = NULL, *BufAddr = NULL;
        UINT32 DspWorkSize = 0;
        ITUNER_VIDEO_HDR_RAW_INFO_s DstRawInfo = {0};

        AmbaTUNE_Get_SystemInfo(&StillTuningSystemInfo);
        RawWidth = StillTuningSystemInfo.RawWidth;
        RawHeight = StillTuningSystemInfo.RawHeight;
        RawPitch = ALIGN_32(StillTuningSystemInfo.RawPitch);
        RawSize = (UINT32) RawPitch*RawHeight;
        AmbaPrint("[ItunerRawEnc] <Get buffer> raw(%u %u %u %u)", RawWidth, RawHeight, RawPitch, RawSize);

        AmbaItuner_Get_VideoHdrRawInfo(&DstRawInfo);
        DstRawWidth = DstRawInfo.RawAreaWidth[0];
        DstRawHeight = DstRawInfo.RawAreaHeight[0];
        DstRawPitch = DstRawWidth<<1; // We only have uncompressed dst raw now
        DstRawSize = (UINT32) DstRawPitch*DstRawHeight;
        AmbaPrint("[ItunerRawEnc] <Get buffer> dst raw(%u %u %u %u)", DstRawWidth, DstRawHeight, DstRawPitch, DstRawSize);

    #ifdef CONFIG_SOC_A12
        AmpUT_GetYuvWorkingBuffer(StillTuningSystemInfo.MainWidth, StillTuningSystemInfo.MainHeight, \
            RawWidth, RawHeight, &YuvMainWidth, &YuvMainHeight);
    #else
        YuvMainWidth = ItunerSystemInfo.MainWidth;
        YuvMainHeight = ItunerSystemInfo.MainHeight;
    #endif
        YuvMainSize = (UINT32) (YuvMainWidth*YuvMainHeight*2);
        AmbaPrint("[ItunerRawEnc] <Get buffer> yuv(%u %u %u)", YuvMainWidth, YuvMainHeight, YuvMainSize);

        YuvScrnWidth = ALIGN_32(StillTuningMgt[StillTuningModeIdx].ScrnWidth);
        YuvScrnHeight = ALIGN_16(StillTuningMgt[StillTuningModeIdx].ScrnHeight);
        YuvScrnSize = (UINT32)(YuvScrnWidth*YuvScrnHeight*2);
        YuvScrnSize += (YuvScrnSize*10)/100;
        AmbaPrint("[ItunerRawEnc] <Get buffer> scrn(%u %u %u)", YuvScrnWidth, YuvScrnHeight, YuvScrnSize);

        YuvThmWidth = ALIGN_32(StillTuningMgt[StillTuningModeIdx].ThmWidth);
        YuvThmHeight = ALIGN_16(StillTuningMgt[StillTuningModeIdx].ThmHeight);
        YuvThmSize = (UINT32)(YuvThmWidth*YuvThmHeight*2);
        YuvThmSize += (YuvThmSize*10)/100;
        AmbaPrint("[ItunerRawEnc] <Get buffer> thm(%u %u %u)", YuvThmWidth, YuvThmHeight, YuvThmSize);

    #ifdef CONFIG_SOC_A12
        if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) Ar = 0;
        else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) Ar = 1;
        QvLcdWidth = ALIGN_32(StillTuningVoutMgt[0][Ar].Width);
        QvLcdHeight = ALIGN_16(StillTuningVoutMgt[0][Ar].Height);
        QvLcdSize = (UINT32) (QvLcdWidth*QvLcdHeight*2);
        QvHdmiWidth = ALIGN_32(StillTuningVoutMgt[1][Ar].Width);
        QvHdmiHeight =ALIGN_16(StillTuningVoutMgt[1][Ar].Height);
        QvHdmiSize = (UINT32) (QvHdmiWidth*QvHdmiHeight*2);
    #endif

        RoiSize = TileNumber*sizeof(AMP_STILLENC_RAW2RAW_ROI_s);
        Raw3aSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);

        Er = AmpUT_StillTuning_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
        if (-1 != Er) {

            /* Memory layout:
             * | -------------------------------------------------------------------------------------|
             * |            |  Raw  |  Dst Raw(opt)  | Main  | Scrn | Thm | Roi | cfa | QvLcd | QvHdmi|
             * |--------------------------------------------------------------------------------------|
             */

            BufAddr = DspWorkAddr + DspWorkSize;
            RawBuffAddr = BufAddr;
            BufAddr += RawSize;
            AmbaPrint("[ItunerRawEnc] rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);

            if (DstRawSize) {
                DstRawBuffAddr = BufAddr;
                BufAddr += DstRawSize;
                AmbaPrint("[ItunerRawEnc] dstRawBuffAddr (0x%08X) (%u)!", DstRawBuffAddr, DstRawSize);
            } else {
                DstRawBuffAddr = NULL;
            }

            YuvBuffAddr = BufAddr;
            BufAddr += YuvMainSize;
            memset(YuvBuffAddr, 0x0, YuvMainSize);
            AmbaCache_Clean(YuvBuffAddr, YuvMainSize);
            AmbaPrint("[ItunerRawEnc] yuvBuffAddr (0x%08X) (%u)!", YuvBuffAddr, YuvMainSize);

            ScrnBuffAddr = BufAddr;
            BufAddr += YuvScrnSize;
            AmbaPrint("[ItunerRawEnc] scrnBuffAddr (0x%08X) (%u)!", ScrnBuffAddr, YuvScrnSize);

            ThmBuffAddr = BufAddr;
            BufAddr += YuvThmSize;
            AmbaPrint("[ItunerRawEnc] thmBuffAddr (0x%08X) (%u)!", ThmBuffAddr, YuvThmSize);

            Raw3ARoiBuffAddr = BufAddr;
            BufAddr += RoiSize;
            AmbaPrint("[ItunerRawEnc] roiBuffAddr (0x%08X) (%u)!", Raw3ARoiBuffAddr, RoiSize);

            Raw3AStatBuffAddr = BufAddr;
            BufAddr += Raw3aSize;
            AmbaPrint("[ItunerRawEnc] roiBuffAddr (0x%08X) (%u)!", Raw3AStatBuffAddr, Raw3aSize);
    #ifdef CONFIG_SOC_A12
            QvLCDBuffAddr = BufAddr;
            BufAddr += QvLcdSize;
            AmbaPrint("[ItunerRawEnc] qvLCDBuffaddr (0x%08X) (%u)!", QvLCDBuffAddr, QvLcdSize);

            QvHDMIBuffAddr = BufAddr;
            BufAddr += QvHdmiSize;
            AmbaPrint("[ItunerRawEnc] qvHDMIBuffaddr (0x%08X) (%u)!", QvHDMIBuffAddr, QvHdmiSize);

            AmbaPrint("[ItunerRawEnc] Buffer end (0x%08X)", BufAddr);
    #endif

            memset(rawBuf, 0x0, sizeof(AMBA_DSP_RAW_BUF_s));
            rawBuf->Compressed = (UINT8) StillTuningSystemInfo.CompressedRaw;
            rawBuf->Window.Width = RawWidth;
            rawBuf->Window.Height = RawHeight;
            rawBuf->Pitch = RawPitch;
            rawBuf->pBaseAddr = (UINT8 *)RawBuffAddr;
            StillRawEncodeNeedPreLoad = 1;
        } else {
            Er = NG;
            AmbaPrintColor(RED,"[StillTuning] Incorrect ituner system info Raw(%u %u %u) Yuv(%u %u)", \
                StillTuningSystemInfo.RawWidth, StillTuningSystemInfo.RawHeight, StillTuningSystemInfo.RawPitch, \
                StillTuningSystemInfo.MainWidth, StillTuningSystemInfo.MainHeight);
        }
    } else {
        Er = NG;
        AmbaPrintColor(RED,"[StillTuning] Already ask for buffer %d", StillRawEncodePreLoadDone);
    }

    return Er;
}

int AmpUt_StillTuningPreLoadDone(void)
{
    int Er = OK;
    if (0 == StillRawEncodeNeedPreLoad) {
        Er = NG;
        AmbaPrintColor(RED,"[ItunerRawEnc] Pre-load is not needed");
    } else if (1 == StillRawEncodeNeedPreLoad) {
        StillRawEncodePreLoadDone = 1;
        AmbaPrint("[ItunerRawEnc] Pre-load is done");
    }
    return Er;
}
int AmpUT_ItunerRawEncode(void)
{
    int Er = OK;
    void *BufAddr = NULL;
    void *TmpBufAddr = NULL;
    UINT8 *StageAddr = NULL, Ar = 0;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT16 DstRawPitch = 0, DstRawWidth = 0, DstRawHeight = 0;
    UINT16 YuvWidth = 0, YuvHeight = 0, ScrnWidth = 0, ScrnHeight = 0, ThmWidth = 0, ThmHeight = 0;
    UINT32 QvLcdSize = 0, QvHdmiSize = 0, YuvSize = 0, TotalScriptSize = 0, TotalStageNum = 0;
    UINT32 ActualFlag = 0;

    AMP_SCRPT_CONFIG_s Scrpt = {0};
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt = NULL;
    AMP_SENC_SCRPT_RAW2YUV_s *Raw2YvuScrpt = NULL;
    AMP_SENC_SCRPT_RAW2RAW_s *Raw2RawScrpt = NULL;
    AMP_SENC_SCRPT_YUV2JPG_s *Yuv2JpgScrpt = NULL;
    ITUNER_VIDEO_HDR_RAW_INFO_s DstRawInfo = {0};

    if (Status != TUNING_STATUS_STILL_LIVEVIEW) {
        AmbaPrint("Error Status, only execute rawenc in LV");
        goto _DONE;
    }

    if (StillTuningRawCaptureRunning) {
        AmbaPrint("Error status");
        goto _DONE;
    }

    StillTuningOutputFlag = 0x2;
    TuningMode = STILL_TUNING_RAWENC;
    AmbaKAL_EventFlagClear(&StillTuningUTTaskFlag, STILL_TUNING_BG_PROCESS_DONE);
    AmbaKAL_EventFlagClear(&StillTuningUTTaskFlag, STILL_TUNING_JPEG_DATA_READY);

    memset(&StillTuningSystemInfo, 0, sizeof(ITUNER_SYSTEM_s));
    AmbaTUNE_Get_SystemInfo(&StillTuningSystemInfo);

    memset(&StillTuningItunerInfo, 0, sizeof(ITUNER_INFO_s));
    AmbaTUNE_Get_ItunerInfo(&StillTuningItunerInfo);

    StillTuningBGProcessing = 1;

    RawPitch = ALIGN_32(StillTuningSystemInfo.RawPitch);
    RawWidth =  StillTuningSystemInfo.RawWidth;
    RawHeight = StillTuningSystemInfo.RawHeight;

    AmbaItuner_Get_VideoHdrRawInfo(&DstRawInfo);
    DstRawWidth = DstRawInfo.RawAreaWidth[0];
    DstRawHeight = DstRawInfo.RawAreaHeight[0];
    DstRawPitch = DstRawWidth<<1; // We only have uncompressed dst raw now

    AmpEnc_StopLiveview(StillTuningPipe, AMP_ENC_FUNC_FLAG_WAIT);

    if (1 == StillRawEncodeNeedPreLoad) {
        if (0 == StillRawEncodePreLoadDone) {
            AmbaPrintColor(RED,"[ItunerRawEnc] Please make sure pre-load is done %d", StillRawEncodePreLoadDone);
        }
    } else {
        AMBA_DSP_RAW_BUF_s RawBuf = {0};
        AmpUT_StillTuningGetRawEncodeBuffer(&RawBuf);
        {   //Load raw file
            Ituner_Ext_File_Param_s Ituner_Ext_File_Param = {0};
            Ituner_Ext_File_Param.Raw_Load_Param.Address = RawBuffAddr;
            Ituner_Ext_File_Param.Raw_Load_Param.Max_Size = RawPitch*RawHeight;
            Ituner_Ext_File_Param.Raw_Load_Param.InputPitch = RawPitch;
            AmbaTUNE_Load_Data(EXT_FILE_RAW, &Ituner_Ext_File_Param);
            AmbaPrint("[ItunerRawEnc] Load Raw Done(0x%X %u)", RawBuffAddr, RawPitch*RawHeight);
            AmbaCache_Clean(Ituner_Ext_File_Param.Raw_Load_Param.Address, Ituner_Ext_File_Param.Raw_Load_Param.Max_Size);
        }
    }

    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        if (OK != AmpUtil_GetAlignedPool(&G_MMPL, &BufAddr, &TmpBufAddr, ScriptSize, 32)) {
            AmbaPrint("[ItunerRawEnc] Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)BufAddr;
            OriScriptAddr = (UINT8*)TmpBufAddr;
            AmbaPrint("[ItunerRawEnc] scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) Ar = 0;
    else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) Ar = 1;

    QvLcdSize = ALIGN_32(StillTuningVoutMgt[0][Ar].Width)*ALIGN_16(StillTuningVoutMgt[0][Ar].Height)*2;
    QvHdmiSize = ALIGN_32(StillTuningVoutMgt[1][Ar].Width)*ALIGN_16(StillTuningVoutMgt[1][Ar].Height)*2;

#ifdef CONFIG_SOC_A12
    AmpUT_GetYuvWorkingBuffer(StillTuningSystemInfo.MainWidth, \
        StillTuningSystemInfo.MainHeight, StillTuningSystemInfo.RawWidth, \
        StillTuningSystemInfo.RawHeight, &YuvWidth, &YuvHeight);
    YuvSize = YuvWidth*YuvHeight*2;
#else
    YuvWidth = StillTuningSystemInfo.MainWidth;
    YuvHeight = StillTuningSystemInfo.MainHeight;
    YuvSize = YuvWidth*YuvHeight*2;
    YuvSize += (YuvSize*10)/100;
#endif

    ScrnWidth = ALIGN_32(StillTuningMgt[StillTuningModeIdx].ScrnWidth);
    ScrnHeight = ALIGN_16(StillTuningMgt[StillTuningModeIdx].ScrnHeight);
    ThmWidth = ALIGN_32(StillTuningMgt[StillTuningModeIdx].ThmWidth);
    ThmHeight = ALIGN_16(StillTuningMgt[StillTuningModeIdx].ThmHeight);

    StageAddr = ScriptAddr;
    GenScrpt = (AMP_SENC_SCRPT_GENCFG_s *)StageAddr;
    memset(GenScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    GenScrpt->Cmd = SENC_GENCFG;
    GenScrpt->RawEncRepeat = 0;
    GenScrpt->RawToCap = 1;
    if (StillTuningItunerInfo.TuningAlgoMode.AlgoMode == 0) GenScrpt->StillProcMode = 2;
    else if (StillTuningItunerInfo.TuningAlgoMode.AlgoMode == 1) GenScrpt->StillProcMode = 1;
    else GenScrpt->StillProcMode = 0;
#ifdef CONFIG_SOC_A12
    GenScrpt->QVConfig.DisableLCDQV = 0;
    GenScrpt->QVConfig.DisableHDMIQV = 0;
    GenScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    GenScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + QvLcdSize/2;
    GenScrpt->QVConfig.LCDWidth = StillTuningVoutMgt[0][Ar].Width;
    GenScrpt->QVConfig.LCDHeight = StillTuningVoutMgt[0][Ar].Height;
    GenScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    GenScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + QvHdmiSize/2;
    GenScrpt->QVConfig.HDMIWidth = StillTuningVoutMgt[1][Ar].Width;
    GenScrpt->QVConfig.HDMIHeight = StillTuningVoutMgt[1][Ar].Height;
#else
    GenScrpt->QVConfig.DisableLCDQV = 1;
    GenScrpt->QVConfig.DisableHDMIQV = 1;
#endif
    QvLcdEnable = (GenScrpt->QVConfig.DisableLCDQV == 1)? 0: 1;
    QvTvEnable = (GenScrpt->QVConfig.DisableHDMIQV == 1)? 0: 1;

    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (GenScrpt->b2LVCfg)? 1: 0;
    GenScrpt->ScrnEnable = 1;
    GenScrpt->ThmEnable = 1;
    GenScrpt->PostProc = &post_ItunerRawEncode_cb;
    GenScrpt->PreProc = &pre_ItunerRawEncode_cb;
    GenScrpt->MainBuf.ColorFmt = AMP_YUV_422;
#ifdef CONFIG_SOC_A12
    GenScrpt->RawDataBits = StillTuningSystemInfo.RawResolution;
    GenScrpt->RawBayerPattern = StillTuningSystemInfo.RawBayer;
    GenScrpt->MainBuf.Width = GenScrpt->MainBuf.Pitch = YuvWidth;
    GenScrpt->MainBuf.Height = YuvHeight;
    GenScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    GenScrpt->MainBuf.ChromaAddr = YuvBuffAddr + YuvSize/2;
    GenScrpt->MainBuf.AOI.X = 0;
    GenScrpt->MainBuf.AOI.Y = 0;
    GenScrpt->MainBuf.AOI.Width = StillTuningSystemInfo.MainWidth;
    GenScrpt->MainBuf.AOI.Height = StillTuningSystemInfo.MainHeight;
#else
    GenScrpt->MainBuf.Width = GenScrpt->MainBuf.Pitch = YuvWidth;
    GenScrpt->MainBuf.Height =  YuvHeight;
    GenScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    GenScrpt->MainBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->MainBuf.AOI.X = 0;
    GenScrpt->MainBuf.AOI.Y = 0;
    GenScrpt->MainBuf.AOI.Width = GenScrpt->MainBuf.Width;
    GenScrpt->MainBuf.AOI.Height = GenScrpt->MainBuf.Height;
#endif
    GenScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ScrnBuf.Width = GenScrpt->ScrnBuf.Pitch = ScrnWidth;
    GenScrpt->ScrnBuf.Height = ScrnHeight;
    GenScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    GenScrpt->ScrnBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->ScrnBuf.AOI.X = 0;
    GenScrpt->ScrnBuf.AOI.Y = 0;
    GenScrpt->ScrnBuf.AOI.Width = StillTuningMgt[StillTuningModeIdx].ScrnWidthAct;
    GenScrpt->ScrnBuf.AOI.Height = StillTuningMgt[StillTuningModeIdx].ScrnHeightAct;
    GenScrpt->ScrnWidth = StillTuningMgt[StillTuningModeIdx].ScrnWidth;
    GenScrpt->ScrnHeight =StillTuningMgt[StillTuningModeIdx].ScrnHeight;

    GenScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ThmBuf.Width = GenScrpt->ThmBuf.Pitch = ThmWidth;
    GenScrpt->ThmBuf.Height = ThmHeight;
    GenScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    GenScrpt->ThmBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->ThmBuf.AOI.X = 0;
    GenScrpt->ThmBuf.AOI.Y = 0;
    GenScrpt->ThmBuf.AOI.Width = StillTuningMgt[StillTuningModeIdx].ThmWidthAct;
    GenScrpt->ThmBuf.AOI.Height = StillTuningMgt[StillTuningModeIdx].ThmHeightAct;
    GenScrpt->ThmWidth = StillTuningMgt[StillTuningModeIdx].ThmWidth;
    GenScrpt->ThmHeight = StillTuningMgt[StillTuningModeIdx].ThmHeight;

    if (1 == StillRawEncodeNeedJpegOutput) {
        extern void AmpUT_initJpegDqt(UINT8 *qTable, int quality);
        extern UINT8 AmpUTJpegQTable[3][128];
        int JpegQuality = (StillTuningSystemInfo.JpgQuality)? StillTuningSystemInfo.JpgQuality: 95;
        AmpUT_initJpegDqt(AmpUTJpegQTable[0], JpegQuality);
        AmpUT_initJpegDqt(AmpUTJpegQTable[1], JpegQuality);
        AmpUT_initJpegDqt(AmpUTJpegQTable[2], JpegQuality);
        GenScrpt->BrcCtrl.Tolerance = 0;
        GenScrpt->BrcCtrl.MaxEncLoop = 0;
        GenScrpt->BrcCtrl.JpgBrcCB = NULL;
        GenScrpt->BrcCtrl.TargetBitRate = 0;
        GenScrpt->BrcCtrl.MainQTAddr = AmpUTJpegQTable[0];
        GenScrpt->BrcCtrl.ThmQTAddr = AmpUTJpegQTable[1];
        GenScrpt->BrcCtrl.ScrnQTAddr = AmpUTJpegQTable[2];
    }

    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum++;
    AmbaPrint("[ItunerRawEnc] Stage_%d 0x%08X", TotalStageNum-1, StageAddr);

    if (1 == StillTuningSystemInfo.EnableRaw2Raw) {
        StageAddr = ScriptAddr + TotalScriptSize;
        Raw2RawScrpt = (AMP_SENC_SCRPT_RAW2RAW_s *)StageAddr;
        Raw2RawScrpt->Cmd = SENC_RAW2RAW;
        Raw2RawScrpt->SrcRawType = (StillTuningSystemInfo.CompressedRaw)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
        Raw2RawScrpt->SrcRawBuf.Buf = RawBuffAddr;
        Raw2RawScrpt->SrcRawBuf.Width = RawWidth;
        Raw2RawScrpt->SrcRawBuf.Height = RawHeight;
        Raw2RawScrpt->SrcRawBuf.Pitch = RawPitch;
        if (DYNAMIC_OB == TUNE_Rule_Lookup_Tuning_Mode_Ext(StillTuningSystemInfo.TuningModeExt)) {
            AMP_STILLENC_RAW2RAW_ROI_s *Roi = (AMP_STILLENC_RAW2RAW_ROI_s *)Raw3ARoiBuffAddr;
            Raw2RawScrpt->TileNumber = 0;
            Roi->RoiColStart = DstRawInfo.Offset.XOffset[0];
            Roi->RoiRowStart = DstRawInfo.Offset.YOffset[0];
            Roi->RoiWidth = DstRawWidth;
            Roi->RoiHeight = DstRawHeight;
            Raw2RawScrpt->DstRawType = AMP_STILLENC_RAW_UNCOMPR; // We only have output uncompressed raw now.
            Raw2RawScrpt->DstRawBuf.Buf = DstRawBuffAddr;
            Raw2RawScrpt->DstRawBuf.Width = DstRawWidth;
            Raw2RawScrpt->DstRawBuf.Height = DstRawHeight;
            Raw2RawScrpt->DstRawBuf.Pitch = DstRawPitch;
            Raw2RawScrpt->TileListAddr = (UINT32)Raw3ARoiBuffAddr;
        } else if (SINGLE_SHOT == TUNE_Rule_Lookup_Tuning_Mode_Ext(StillTuningSystemInfo.TuningModeExt)) {
            Raw2RawScrpt->TileNumber = 1; // We only have full frame case now.
            if (Raw2RawScrpt->TileNumber == 1) {
                AMP_STILLENC_RAW2RAW_ROI_s *Roi = (AMP_STILLENC_RAW2RAW_ROI_s *)Raw3ARoiBuffAddr;
                Roi->RoiColStart = 0;
                Roi->RoiRowStart = 0;
                Roi->RoiWidth = RawWidth - Roi->RoiColStart;
                Roi->RoiHeight = RawHeight - Roi->RoiRowStart;
            } else if (Raw2RawScrpt->TileNumber > 1) {
                //TBD
            }
            Raw2RawScrpt->Raw3AStatAddr = (UINT32)Raw3AStatBuffAddr;
            Raw2RawScrpt->Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
        }
        Raw2RawScrpt->TileListAddr = (UINT32)Raw3ARoiBuffAddr;
        TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s));
        TotalStageNum++;
        AmbaPrint("[ItunerRawEnc] Stage_%d 0x%X", TotalStageNum-1, StageAddr);
    } else {
        StageAddr = ScriptAddr + TotalScriptSize;
        Raw2YvuScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)StageAddr;
        memset(Raw2YvuScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
        Raw2YvuScrpt->Cmd = SENC_RAW2YUV;
        Raw2YvuScrpt->RawType = (StillTuningSystemInfo.CompressedRaw)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
        Raw2YvuScrpt->RawBuf.Buf = RawBuffAddr;
        Raw2YvuScrpt->RawBuf.Width = RawWidth;
        Raw2YvuScrpt->RawBuf.Height = RawHeight;
        Raw2YvuScrpt->RawBuf.Pitch = RawPitch;
        Raw2YvuScrpt->RawBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
        Raw2YvuScrpt->RingBufSize = 0;
        Raw2YvuScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
        Raw2YvuScrpt->YuvRingBufSize = 0;
        TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
        TotalStageNum++;
        AmbaPrint("[ItunerRawEnc] Stage_%d 0x%X", TotalStageNum-1, StageAddr);
        if (1 == StillRawEncodeNeedJpegOutput) {
            StageAddr = ScriptAddr + TotalScriptSize;
            Yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)StageAddr;
            memset(Yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
            Yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
            Yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
            Yuv2JpgScrpt->YuvRingBufSize = 0;
            TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
            AmbaPrint("[ItunerRawEnc] Stage_%u 0x%08X", TotalStageNum, StageAddr);
            TotalStageNum++;
        }
    }

    //script config
    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[ItunerRawEnc] Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    //set still idsp param
    {
        AMBA_DSP_IMG_CTX_INFO_s DestCtx = {0}, SrcCtx = {0};
        AMBA_DSP_IMG_CFG_INFO_s CfgInfo = {0};
        AMBA_ITUNER_PROC_INFO_s ProcInfo = {0};
        AMBA_DSP_IMG_WARP_CALC_INFO_s CalcWarp = {0};
        AMBA_DSP_IMG_OUT_WIN_INFO_s ImgOutputWin = {0};
        AMBA_DSP_IMG_SIZE_INFO_s SizeInfo = {0};

        //Initialize the context of ImageKernel of still
        DestCtx.Pipe = AMBA_DSP_IMG_PIPE_STILL;
        DestCtx.CtxId = 0;
        SrcCtx.CtxId = 0;
    #ifdef CONFIG_SOC_A9
        AmbaDSP_ImgInitCtx(0, 0, &destCtx, &srcCtx);
    #endif
        //Initialize the configure of ImageKernel of Still
        CfgInfo.Pipe = AMBA_DSP_IMG_PIPE_STILL;
        CfgInfo.CfgId = 0;
        AmbaDSP_ImgInitCfg(&CfgInfo, StillTuningItunerInfo.TuningAlgoMode.AlgoMode);

        AmbaPrint("[ItunerRawEnc] AlgoMode(%d %d %d %d %d %d)", \
            StillTuningItunerInfo.TuningAlgoMode.Pipe, \
            StillTuningItunerInfo.TuningAlgoMode.AlgoMode, \
            StillTuningItunerInfo.TuningAlgoMode.FuncMode, \
            StillTuningItunerInfo.TuningAlgoMode.BatchId, \
            StillTuningItunerInfo.TuningAlgoMode.ConfigId, \
            StillTuningItunerInfo.TuningAlgoMode.ContextId);

        ImgOutputWin.MainWinDim.Width = StillTuningSystemInfo.MainWidth;
        ImgOutputWin.MainWinDim.Height = StillTuningSystemInfo.MainHeight;
    #ifdef CONFIG_SOC_A12
        ImgOutputWin.ScreennailDim.Width = StillTuningMgt[StillTuningModeIdx].ScrnWidthAct;
        ImgOutputWin.ScreennailDim.Height = StillTuningMgt[StillTuningModeIdx].ScrnHeightAct;
        ImgOutputWin.ThumbnailDim.Width = StillTuningMgt[StillTuningModeIdx].ThmWidthAct;
        ImgOutputWin.ThumbnailDim.Height = StillTuningMgt[StillTuningModeIdx].ThmHeightAct;
        ImgOutputWin.PrevWinDim[0].Width = StillTuningVoutMgt[0][Ar].Width;
        ImgOutputWin.PrevWinDim[0].Height = StillTuningVoutMgt[0][Ar].Height;
        ImgOutputWin.PrevWinDim[1].Width  = StillTuningVoutMgt[1][Ar].Width;
        ImgOutputWin.PrevWinDim[1].Height = StillTuningVoutMgt[1][Ar].Height;
    #endif
        AmbaDSP_WarpCore_SetOutputWin(&StillTuningItunerInfo.TuningAlgoMode, &ImgOutputWin);

        ProcInfo.HisoBatchId = AMBA_DSP_STILL_HISO_FILTER;
        AmbaTUNE_Execute_IDSP(&StillTuningItunerInfo.TuningAlgoMode, &ProcInfo);
        AmbaDSP_ImgGetWarpCompensation(&StillTuningItunerInfo.TuningAlgoMode, &CalcWarp);

        SizeInfo.WidthIn = ((CalcWarp.ActWinCrop.RightBotX - CalcWarp.ActWinCrop.LeftTopX + 0xFFFF)>>16);
        SizeInfo.HeightIn = ((CalcWarp.ActWinCrop.RightBotY - CalcWarp.ActWinCrop.LeftTopY + 0xFFFF)>>16);
        SizeInfo.WidthMain = StillTuningSystemInfo.MainWidth;
        SizeInfo.HeightMain = StillTuningSystemInfo.MainHeight;
    #ifdef CONFIG_SOC_A12
        SizeInfo.WidthPrevA = StillTuningVoutMgt[0][Ar].Width;
        SizeInfo.HeightPrevA = StillTuningVoutMgt[0][Ar].Height;
        SizeInfo.WidthPrevB = StillTuningVoutMgt[1][Ar].Width;
        SizeInfo.HeightPrevB = StillTuningVoutMgt[1][Ar].Height;
        SizeInfo.WidthScrn = StillTuningMgt[StillTuningModeIdx].ScrnWidthAct;
        SizeInfo.HeightScrn = StillTuningMgt[StillTuningModeIdx].ScrnHeightAct;
    #endif
        AmbaDSP_ImgSetSizeInfo(&StillTuningItunerInfo.TuningAlgoMode, &SizeInfo);
        AmbaDSP_ImgPostExeCfg(&StillTuningItunerInfo.TuningAlgoMode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY);
    }

    AmpEnc_RunScript(StillTuningPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK) {
        AmbaPrint("memFree Fail (scrpt)");
    } else {
        ScriptAddr = NULL;
        OriScriptAddr = NULL;
    }

    if ((0 == StillTuningSystemInfo.EnableRaw2Raw) && (1 == StillRawEncodeNeedJpegOutput)) {
        Er = AmbaKAL_EventFlagTake(&StillTuningUTTaskFlag, STILL_TUNING_JPEG_DATA_READY, \
            AMBA_KAL_AND, &ActualFlag, StillTuningTimeout);
    } else {
        Er = AmbaKAL_EventFlagTake(&StillTuningUTTaskFlag, STILL_TUNING_BG_PROCESS_DONE, \
            AMBA_KAL_AND, &ActualFlag, StillTuningTimeout);
    }

    if (Er != OK) {
        AmbaPrintColor(RED, "[ItunerRawEnc] Execute Script TimeOut!!");
        goto _DONE;
    } else {
        AmbaPrint("[ItunerRawEnc] Wait Execute Script Done!!, current script is finished");
    }

    /* Back to liveview */
    AmpUT_StillTuningUnLockAE();
    AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_LIVEVIEW, 0);
    AmpUT_StillTuning_ChangeResolution(StillTuningModeIdx);
    AmpEnc_StartLiveview(StillTuningPipe, AMP_ENC_FUNC_FLAG_WAIT);

    StillRawEncodeNeedPreLoad = 0;
    StillRawEncodePreLoadDone = 0;

    RawBuffAddr = NULL;
    DstRawBuffAddr = NULL;
    YuvBuffAddr = NULL;
    ScrnBuffAddr = NULL;
    ThmBuffAddr = NULL;
    Raw3ARoiBuffAddr = NULL;
    Raw3AStatBuffAddr = NULL;

_DONE:
    return Er;
}

void AmpUT_StillTuning_UtTask(UINT32 info)
{
    STILLTUNING_UT_MSG_S Msg = {0};

    AmbaPrint("AmpUT_StillTuning_UtTask Start");

    while (1) {
        AmbaKAL_MsgQueueReceive(&TuningUtMsgQueue, (void *)&Msg, AMBA_KAL_WAIT_FOREVER);

        AmbaPrint("[Tuning_UT_task] Rcv 0x%X", Msg.Event);
        switch (Msg.Event) {
            case MSG_STILLTUNING_UT_TIMER_TIMEOUT:
            {
                /* Swith back to VideoSource */
                TuningVoutLcdSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;
                TuningVoutTvSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;

                if (1) {
                    //Send LiveStartCmd instead of issue videoSourceSel, SSP forbid user to select voutSource to vin
                    AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_LIVEVIEW, 1);
                }

                /* reset Timer */
                AmbaKAL_TimerStop(&QvTimerHdlr);
                AmbaKAL_TimerDelete(&QvTimerHdlr);
                QvTimerHdlrID = -1;

                /* reset qvshow buffer */
                QvLcdShowBuffIndex = QvTvShowBuffIndex = 0;
                AmbaPrint("[qvTmr_TO] Done!");
            }
                break;
            case MSG_STILLTUNING_UT_QV_LCD_READY:
            {
                AMBA_DSP_VOUT_DEFAULT_IMG_CONFIG_s DefImage = {0};
                UINT8 *LumaAddr = Msg.Data.yuvInfo.yAddr;
                UINT8 *ChromaAddr = LumaAddr + Msg.Data.yuvInfo.ySize;

                //switch VOUT base on qvConfig
                /* 1. Set Default image as ShowBuffer, TBD, wrap by MW? */
                DefImage.FieldRepeat = 0;
                DefImage.Pitch = Msg.Data.yuvInfo.pitch;
                DefImage.pBaseAddrY = LumaAddr;
                DefImage.pBaseAddrUV = ChromaAddr;
                AmbaDSP_VoutDefaultImgSetup(AMBA_DSP_VOUT_LCD, &DefImage);
                QvLcdShowBuffIndex++;
                QvLcdShowBuffIndex %= 2;

                /* 2. switch VOUT to default image, TBD, wrap by MW? */
                if (TuningVoutLcdSrc != (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_DEFAULT_IMG) {
                    AmbaDSP_VoutVideoSourceSel(AMBA_DSP_VOUT_LCD, AMBA_DSP_VOUT_VIDEO_SRC_DEFAULT_IMG);
                    TuningVoutLcdSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_DEFAULT_IMG;
                }

                /* 3. start timer handler base on qvShowConfig */
                if (QvTimerHdlrID == -1) {
                    QvTimerHdlrID = AmbaKAL_TimerCreate(&QvTimerHdlr, AMBA_KAL_AUTO_START,
                                    &AmpUT_qvTimer_Handler, QvTimerID,
                                    QvShowTimeTable[QvDisplayCfg-1], QvShowTimeTable[QvDisplayCfg-1]);
                    if (QvTimerHdlrID < 0) {
                        AmbaPrintColor(RED,"[Amp_UT] QV tmr handler create fail !!");
                    } else AmbaPrint("[Amp_UT] QV tmr handler %d", QvTimerHdlrID);
                } else {
                    if ((QvTvEnable && QvTvShowBuffIndex == QvLcdShowBuffIndex) || \
                        !QvTvEnable) {
                        /* Timer Handler already exist, reset it */
                        AmbaKAL_TimerStop(&QvTimerHdlr);
                        AmbaKAL_TimerChange(&QvTimerHdlr, QvShowTimeTable[QvDisplayCfg-1], QvShowTimeTable[QvDisplayCfg-1], AMBA_KAL_AUTO_START);
                    }
                }
            }
                break;
            case MSG_STILLTUNING_UT_QV_TV_READY:
            {
                AMBA_DSP_VOUT_DEFAULT_IMG_CONFIG_s DefImage = {0};
                UINT8 *LumaAddr = Msg.Data.yuvInfo.yAddr;
                UINT8 *ChromaAddr = LumaAddr + Msg.Data.yuvInfo.ySize;

                //switch VOUT base on qvConfig
                /* 1. Set Default image as ShowBuffer, TBD, wrap by MW? */
                DefImage.FieldRepeat = 0;
                DefImage.Pitch = Msg.Data.yuvInfo.pitch;
                DefImage.pBaseAddrY = LumaAddr;
                DefImage.pBaseAddrUV = ChromaAddr;
                AmbaDSP_VoutDefaultImgSetup(AMBA_DSP_VOUT_LCD, &DefImage);
                QvTvShowBuffIndex++;
                QvTvShowBuffIndex %= 2;

                /* 2. switch VOUT to default image, TBD, wrap by MW? */
                if (TuningVoutTvSrc != (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_DEFAULT_IMG) {
                    AmbaDSP_VoutVideoSourceSel(AMBA_DSP_VOUT_TV, AMBA_DSP_VOUT_VIDEO_SRC_DEFAULT_IMG);
                    TuningVoutTvSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_DEFAULT_IMG;
                }

                /* 3. start timer handler base on qvShowConfig */
                if (QvTimerHdlrID == -1) {
                    QvTimerHdlrID = AmbaKAL_TimerCreate(&QvTimerHdlr, AMBA_KAL_AUTO_START,
                                    &AmpUT_qvTimer_Handler, QvTimerID,
                                    QvShowTimeTable[QvDisplayCfg-1], QvShowTimeTable[QvDisplayCfg-1]);
                    if (QvTimerHdlrID < 0) {
                        AmbaPrintColor(RED,"[Amp_UT] QV tmr handler create fail !!");
                    } else AmbaPrint("[Amp_UT] QV tmr handler %d", QvTimerHdlrID);
                } else {
                    if ((QvLcdEnable && QvLcdShowBuffIndex == QvTvShowBuffIndex) || \
                        !QvLcdEnable) {
                        /* Timer Handler already exist, reset it */
                        AmbaKAL_TimerStop(&QvTimerHdlr);
                        AmbaKAL_TimerChange(&QvTimerHdlr, QvShowTimeTable[QvDisplayCfg-1], QvShowTimeTable[QvDisplayCfg-1], AMBA_KAL_AUTO_START);
                    }
                }
            }
                break;
            case MSG_STILLTUNING_UT_RAW_CAPTURE_DONE:
            {
                if (AutoBackToLiveview == 0) {
                    UINT32 Flag = 0;
                    AmpUT_StillTuningUnLockAE();
                    AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_LIVEVIEW, 0);
                    AmpUT_StillTuning_ChangeResolution(StillTuningModeIdx);
                    Flag = AMP_ENC_FUNC_FLAG_WAIT;
                    if (QvDisplayCfg) Flag |= AMP_ENC_FUNC_FLAG_NO_SHOW_VIDEO_PLANE;
                    AmpEnc_StartLiveview(StillTuningPipe, Flag);
                    Status = TUNING_STATUS_STILL_LIVEVIEW;
                }

                if (StillTuningOutputFlag & 0x1)
                    AmbaKAL_EventFlagGive(&StillTuningUTTaskFlag, STILL_TUNING_RAW_CAPTURE_DONE);
            }
                break;
            case MSG_STILLTUNING_UT_BG_PROCESS_DONE:
            {
                if ((StillTuningOutputFlag & 0x2) || (StillTuningOutputFlag & 0x4)) {
                    AmbaKAL_EventFlagGive(&StillTuningUTTaskFlag, STILL_TUNING_BG_PROCESS_DONE);
                    AmbaPrintColor(BLUE, "[AMP_UT] STILL_SCRIPT_PROCESS_DONE");
                }
            }
                break;
            default:
                break;
        }

    }
}

/**
 * Generic StillTuning ImgSchdlr callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_StillTuningImgSchdlrCallback(void *hdlr,UINT32 event, void *info)
{
//    static int cfacnt = 0;

    switch (event) {
        case AMBA_IMG_SCHDLR_EVENT_CFA_STAT_READY:
            if (StillTuning3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspCfa_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_RGB_STAT_READY:
            if (StillTuning3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_MAIN_CFA_HIST_READY:
            if (StillTuning3AEnable) {
//                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                //Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_HDR_CFA_HIST_READY:
            if (StillTuning3AEnable) {
//                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                //Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        default:
            AmbaPrint("[%s] Unknown %X info: %x", __func__, event, info);
            break;
    }
    return 0;
}

static int AmpUT_StillTuningVinSwitchCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {

        case AMP_VIN_EVENT_INVALID:
            AmbaPrint("[StillTuning]VinSWCB: AMP_VIN_EVENT_INVALID info: %X", info);

            if (STuningImgSchdlr) AmbaImgSchdlr_Enable(STuningImgSchdlr, 0);
            Amba_Img_VIn_Invalid(hdlr, (UINT32 *)NULL);
            break;
        case AMP_VIN_EVENT_VALID:
            AmbaPrint("[StillTuning]VinSWCB: AMP_VIN_EVENT_VALID info: %X", info);
            if (StillTuning3AEnable) {
                Amba_Img_VIn_Valid(hdlr, (UINT32 *)NULL);
                if (STuningImgSchdlr)
                    AmbaImgSchdlr_Enable(STuningImgSchdlr, 1);
            }
            break;
        case AMP_VIN_EVENT_CHANGED_PRIOR:
            AmbaPrint("[StillTuning]VinSWCB: AMP_VIN_EVENT_CHANGED_PRIOR info: %X", info);
            if (StillTuning3AEnable) {
                Amba_Img_VIn_Changed_Prior(hdlr, (UINT32 *)NULL);
            }
        #ifdef CONFIG_SOC_A12
            if (Status == TUNING_STATUS_STILL_LIVEVIEW || StillTuningBGProcessing) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s Window = {0}, DefWindow = {0};
                UINT8 ArIndex = 0;

                if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                Window.Source = AMP_DISP_ENC;
                Window.SourceDesc.Enc.VinCh = TuningChannel;
                Window.CropArea.Width = 0;
                Window.CropArea.Height = 0;
                Window.CropArea.X = 0;
                Window.CropArea.Y = 0;
                Window.TargetAreaOnPlane.Width = StillTuningVoutMgt[0][ArIndex].Width;
                Window.TargetAreaOnPlane.Height = StillTuningVoutMgt[0][ArIndex].Height;
                Window.TargetAreaOnPlane.X = 00;
                Window.TargetAreaOnPlane.Y = (480 - Window.TargetAreaOnPlane.Height)/2;
                if(AmpUT_Display_GetWindowCfg(0, &DefWindow) != OK) {
                    AmpUT_Display_Window_Create(0, &Window);
                } else {
                    AmpUT_Display_SetWindowCfg(0, &Window);
                }
                if (StillTuningLCDLiveview) {
                    AmpUT_Display_Act_Window(0);
                } else {
                    AmpUT_Display_DeAct_Window(0);
                }
                TuningVoutLcdSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;
            }

            if (Status == TUNING_STATUS_STILL_LIVEVIEW || StillTuningBGProcessing) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s Window = {0}, DefWindow = {0};
                UINT8 ArIndex = 0;

                if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                Window.Source = AMP_DISP_ENC;
                Window.SourceDesc.Enc.VinCh = TuningChannel;
                Window.CropArea.Width = 0;
                Window.CropArea.Height = 0;
                Window.CropArea.X = 0;
                Window.CropArea.Y = 0;
                Window.TargetAreaOnPlane.Width = StillTuningVoutMgt[1][ArIndex].Width;
                Window.TargetAreaOnPlane.Height = StillTuningVoutMgt[1][ArIndex].Height;
                Window.TargetAreaOnPlane.X = (1920 - Window.TargetAreaOnPlane.Width)/2;
                Window.TargetAreaOnPlane.Y = 0;
                if(AmpUT_Display_GetWindowCfg(1, &DefWindow) != OK) {
                    AmpUT_Display_Window_Create(1, &Window);
                } else {
                    AmpUT_Display_SetWindowCfg(1, &Window);
                }
                if (StillTuningTvLiveview) {
                    AmpUT_Display_Act_Window(1);
                } else {
                    AmpUT_Display_DeAct_Window(1);
                }
                TuningVoutTvSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;
            }
        #endif
            break;
        case AMP_VIN_EVENT_CHANGED_POST:
            AmbaPrint("[StillTuning]VinSWCB: AMP_VIN_EVENT_CHANGED_POST info: %X", info);
            if (StillTuning3AEnable) {
                UINT8 IsPhotoLiveView = 0;
                UINT16 PipeMode = IP_EXPERSS_MODE;
                UINT16 AlgoMode = IP_MODE_LISO_VIDEO;
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo;
                AMBA_SENSOR_MODE_ID_u SensorMode = {0};

                SensorMode.Data = (StillTuningEncodeSystem==0)? StillTuningMgt[StillTuningModeIdx].InputMode: StillTuningMgt[StillTuningModeIdx].InputPALMode;
                AmbaSensor_GetModeInfo(TuningChannel, SensorMode, &SensorModeInfo);

                if (STuningImgSchdlr) {
                    AMBA_IMG_SCHDLR_UPDATE_CFG_s SchdlrCfg = {0};

                    AmbaImgSchdlr_GetConfig(STuningImgSchdlr, &SchdlrCfg);
                    if (StillTuningLiveViewProcMode && StillTuningLiveViewAlgoMode) {
                        SchdlrCfg.VideoProcMode = 1;
                        if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                            SchdlrCfg.VideoProcMode |= 0x10;
                        }
                    }
                    AmbaImgSchdlr_SetConfig(STuningImgSchdlr, &SchdlrCfg);  // One MainViewID (not vin) need one scheduler.
                }

            #ifdef CONFIG_SOC_A12
                { // Load IQ params
                    extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
                    UINT8 IsSensorHdrMode;

                    // Inform 3A LV sensor mode is Hdr or not
                    IsSensorHdrMode = (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE)? 1: 0;
                    AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_HDR_ENABLE, 0/*ChNo*/, (UINT32)IsSensorHdrMode, 0);

                    if (StillTuningIsIqParamInit == 0 || IsSensorHdrMode != StillTuningIsHdrIqParam) {
                        App_Image_Init_Iq_Params(0, InputDeviceId);
                        StillTuningIsIqParamInit = 1;
                        StillTuningIsHdrIqParam = IsSensorHdrMode;
                    }
                }
            #endif

                //inform 3A LiveView pipeline
                if (StillTuningLiveViewProcMode && StillTuningLiveViewAlgoMode) {
                    PipeMode = IP_HYBRID_MODE;
                } else {
                    PipeMode = IP_EXPERSS_MODE;
                }
                AmbaImg_Proc_Cmd(MW_IP_SET_PIPE_MODE, 0/*ChNo*/, (UINT32)PipeMode, 0);

                //inform 3A LiveView Algo
                if (StillTuningLiveViewAlgoMode == 0 || StillTuningLiveViewAlgoMode == 1) {
                    AlgoMode = IP_MODE_LISO_VIDEO;
                } else {
                    AlgoMode = IP_MODE_HISO_VIDEO;
                }
                AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_ALGO_MODE, 0/*ChNo*/, (UINT32)AlgoMode, 0);

                //inform 3A LiveView type

                AmbaImg_Proc_Cmd(MW_IP_SET_PHOTO_PREVIEW, (UINT32)&IsPhotoLiveView, 0, 0);
                //inform 3A LiveView info
                {
                    LIVEVIEW_INFO_s LiveViewInfo = {0};
                    AMBA_SENSOR_STATUS_INFO_s SsrStatus = {0};
                    AMBA_SENSOR_INPUT_INFO_s *InputInfo = &SsrStatus.ModeInfo.InputInfo;

                    LiveViewInfo.OverSamplingEnable = 1; //HybridMode use OverSampling
                    LiveViewInfo.MainW = StillTuningMgt[StillTuningModeIdx].MainWidth;
                    LiveViewInfo.MainH = StillTuningMgt[StillTuningModeIdx].MainHeight;
                    if (StillTuningEncodeSystem == 0) {
                        LiveViewInfo.FrameRateInt = StillTuningMgt[StillTuningModeIdx].TimeScale/StillTuningMgt[StillTuningModeIdx].TickPerPicture;
                    } else {
                        LiveViewInfo.FrameRateInt = StillTuningMgt[StillTuningModeIdx].TimeScalePAL/StillTuningMgt[StillTuningModeIdx].TickPerPicturePAL;
                    }
                    LiveViewInfo.FrameRateInt = UT_StillTuningFrameRateIntConvert(LiveViewInfo.FrameRateInt);
                    AmbaSensor_GetStatus(TuningChannel, &SsrStatus);
                    LiveViewInfo.BinningHNum = InputInfo->HSubsample.FactorNum;
                    LiveViewInfo.BinningHDen = InputInfo->HSubsample.FactorDen;
                    LiveViewInfo.BinningVNum = InputInfo->VSubsample.FactorNum;
                    LiveViewInfo.BinningVDen = InputInfo->VSubsample.FactorDen;
                    AmbaImg_Proc_Cmd(MW_IP_SET_LIVEVIEW_INFO, 0/*ChIndex*/, (UINT32)&LiveViewInfo, 0);
                }

                //inform 3A FrameRate info
                {
					UINT32 FrameRate = 0, FrameRatex1000 = 0;

                    if (StillTuningEncodeSystem == 0) {
                        FrameRate = StillTuningMgt[StillTuningModeIdx].TimeScale/StillTuningMgt[StillTuningModeIdx].TickPerPicture;
                        FrameRatex1000 = StillTuningMgt[StillTuningModeIdx].TimeScale*1000/StillTuningMgt[StillTuningModeIdx].TickPerPicture;
                    } else {
                        FrameRate = StillTuningMgt[StillTuningModeIdx].TimeScalePAL/StillTuningMgt[StillTuningModeIdx].TickPerPicturePAL;
                        FrameRatex1000 = StillTuningMgt[StillTuningModeIdx].TimeScalePAL*1000/StillTuningMgt[StillTuningModeIdx].TickPerPicturePAL;
                    }
                    FrameRate = UT_StillTuningFrameRateIntConvert(FrameRate);
                    AmbaImg_Proc_Cmd(MW_IP_SET_FRAME_RATE, 0/*ChIndex*/, FrameRate, FrameRatex1000);
                }

                Amba_Img_VIn_Changed_Post(hdlr, (UINT32 *)NULL);
            }
        #ifdef CONFIG_SOC_A9
            if ((Status == TUNING_STATUS_STILL_INIT || \
                Status == TUNING_STATUS_STILL_LIVEVIEW || \
                StillTuningBGProcessing) && \
                TuningLcdWinHdlr) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s Window = {0};
                UINT8 ArIndex = 0;
                if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;
                Window.Source = AMP_DISP_ENC;
                Window.SourceDesc.Enc.VinCh = TuningChannel;
                Window.CropArea.Width = 0;
                Window.CropArea.Height = 0;
                Window.CropArea.X = 0;
                Window.CropArea.Y = 0;
                Window.TargetAreaOnPlane.Width = StillTuningVoutMgt[0][ArIndex].Width;
                Window.TargetAreaOnPlane.Height = StillTuningVoutMgt[0][ArIndex].Height;
                Window.TargetAreaOnPlane.X = 00;
                Window.TargetAreaOnPlane.Y = (480 - Window.TargetAreaOnPlane.Height)/2;
                AmpDisplay_SetWindowCfg(TuningLcdWinHdlr, &Window);
                if (StillTuningLCDLiveview) {
                    AmpDisplay_SetWindowActivateFlag(TuningLcdWinHdlr, 1);
                } else {
                    AmpDisplay_SetWindowActivateFlag(TuningLcdWinHdlr, 0);
                }
                AmpDisplay_Update(LCDHdlr);
                TuningVoutLcdSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;
            }

            if ((Status == TUNING_STATUS_STILL_INIT || \
                Status == TUNING_STATUS_STILL_LIVEVIEW || \
                StillTuningBGProcessing) && \
                TuningTvWinHdlr) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s Window = {0};
                UINT8 ArIndex = 0;
                if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;
                Window.Source = AMP_DISP_ENC;
                Window.SourceDesc.Enc.VinCh = TuningChannel;
                Window.CropArea.Width = 0;
                Window.CropArea.Height = 0;
                Window.CropArea.X = 0;
                Window.CropArea.Y = 0;
                Window.TargetAreaOnPlane.Width = StillTuningVoutMgt[1][ArIndex].Width;
                Window.TargetAreaOnPlane.Height = StillTuningVoutMgt[1][ArIndex].Height;
                Window.TargetAreaOnPlane.X = (1920 - Window.TargetAreaOnPlane.Width)/2;
                Window.TargetAreaOnPlane.Y = 0;
                AmpDisplay_SetWindowCfg(TuningTvWinHdlr, &Window);
                if (StillTuningTvLiveview) {
                    AmpDisplay_SetWindowActivateFlag(TuningTvWinHdlr, 1);
                } else {
                    AmpDisplay_SetWindowActivateFlag(TuningTvWinHdlr, 0);
                }
                AmpDisplay_Update(TVHdlr);
                TuningVoutTvSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;
            }
        #endif
            break;
        default:
            AmbaPrint("[StillTuning]VinSWCB: Unknown %X info: %x", event, info);
           break;
    }
    return 0;
}

//static int xcnt = 0, ycnt = 0, zcnt = 0; // Just to reduce console print
static int AmpUT_StillTuningVinEventCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {
        case AMP_VIN_EVENT_FRAME_READY:
            //if (xcnt % 30 == 0)
              //AmbaPrint("[StillTuning]VinEVCB: AMP_VIN_EVENT_FRAME_READY info: %X", info);
            //xcnt++;
            break;
        case AMP_VIN_EVENT_FRAME_DROPPED:
            AmbaPrint("[StillTuning]VinEVCB: AMP_VIN_EVENT_FRAME_DROPPED info: %X", info);
            break;
        default:
            AmbaPrint("[StillTuning]VinEVCB: Unknown %X info: %x", event, info);
           break;
    }
    return 0;
}

static int AmpUT_StillTuningCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_DONE:
        {
            // APP can issue start liveview,
            // Next raw capture is allowed from now on
            StillTuningRawCaptureRunning = 0;
            if (AutoBackToLiveview == 0) {
                STILLTUNING_UT_MSG_S msg;
                msg.Event = MSG_STILLTUNING_UT_RAW_CAPTURE_DONE;
                AmbaKAL_MsgQueueSend(&TuningUtMsgQueue, &msg, AMBA_KAL_NO_WAIT);
            }
        }
            break;
        case AMP_ENC_EVENT_BACK_TO_LIVEVIEW:
        {
            // if script.b2lv > 0,
            // this event_CB is invoke when system already back to liveview
        }
            break;
        case AMP_ENC_EVENT_BG_PROCESS_DONE:
        {
            STILLTUNING_UT_MSG_S msg;
            //whole script process done.
            StillTuningBGProcessing = 0;
            msg.Event = MSG_STILLTUNING_UT_BG_PROCESS_DONE;
            AmbaKAL_MsgQueueSend(&TuningUtMsgQueue, &msg, AMBA_KAL_NO_WAIT);
        }
            break;
        case AMP_ENC_EVENT_LIVEVIEW_RAW_READY:
            {
              //AMP_ENC_RAW_INFO_s *ptr = info;
              //AMP_ENC_RAW_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
              //AmbaPrint("EncCB: AMP_ENC_EVENT_LIVEVIEW_RAW_READY addr: %X p:%d %dx%d", inf.RawAddr, inf.RawPitch, inf.RawWidth, inf.RawHeight);
            }
            break;
        case AMP_ENC_EVENT_LIVEVIEW_DCHAN_YUV_READY:
            {
              //AMP_ENC_YUV_INFO_s *ptr = info;
              //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
              //AmbaPrint("[AmpUT_StillTuning] AMP_STILLENC_MSG_LIVEVIEW_DCHAN_YUV_READY addr: %X p:%d %dx%d", inf.yAddr, inf.pitch, inf.width, inf.height);
            }
            break;
        case AMP_ENC_EVENT_LIVEVIEW_FCHAN_YUV_READY:
            {
              //AMP_ENC_YUV_INFO_s *ptr = info;
              //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
              //AmbaPrint("[AmpUT_StillTuning] AMP_STILLENC_MSG_LIVEVIEW_FCHAN_YUV_READY info: %X", info);
            }
            break;
        case AMP_ENC_EVENT_VCAP_YUV_READY:
            //AmbaPrint("[AmpUT_StillTuning]: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VCAP_2ND_YUV_READY:
            //AmbaPrint("[AmpUT_StillTuning]: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_2ND_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VCAP_ME1_Y_READY:
            //AmbaPrint("[AmpUT_StillTuning]: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_ME1_Y_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_HYBRID_YUV_READY:
            //AmbaPrint("[AmpUT_StillTuning]: !!!!!!!!!!! AMP_ENC_EVENT_HYBRID_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_QUICKVIEW_DCHAN_YUV_READY:
        {
            // In A9, app must copy LCD or HDMI qv to their local buffer and ask DISPLAY module to show defIMG.
            // But in A12, we need to switch display to video src as soon as qv_data_rdy to prevent vout buffer full.
            // preview_a buffer is control by uCode in a12, APP is able to access it.
            // preview_b buffer is control by uCode in a12, App is unable to access it.
#ifdef CONFIG_SOC_A12
            AMP_DISP_WINDOW_CFG_s Window = {0};
            UINT8 ArIndex = 0;
            if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
            else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

            if (1) {
                Window.Source = AMP_DISP_ENC;
                Window.SourceDesc.Enc.VinCh = TuningChannel;
                Window.CropArea.Width = 0;
                Window.CropArea.Height = 0;
                Window.CropArea.X = 0;
                Window.CropArea.Y = 0;
                Window.TargetAreaOnPlane.Width = StillTuningVoutMgt[0][ArIndex].Width;
                Window.TargetAreaOnPlane.Height = StillTuningVoutMgt[0][ArIndex].Height;
                Window.TargetAreaOnPlane.X = 0;
                Window.TargetAreaOnPlane.Y = (480 - Window.TargetAreaOnPlane.Height)/2;
                AmpUT_Display_SetWindowCfg(0, &Window);
                AmpUT_Display_Act_Window(0);
                AmbaPrint("[AmpUT_StillEnc] QV_DCHAN_YUV_READY, switch to VDSRC");
            } else {
                AmbaPrint("[AmpUT_StillEnc] ERROR, A12 MUST turn on VDSRC as soon as received QV_DCHAN_YUV_READY");
            }
#else
            AMP_ENC_YUV_INFO_s *ptr = info;
            //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
            AmbaPrint("[AmpUT_StillTuning] AMP_ENC_EVENT_QUICKVIEW_DCHAN_YUV_READY info: 0x%X (%d %d %d) %u qv %d", ptr->yAddr, ptr->pitch, ptr->width, ptr->height, ptr->ySize, QvDisplayCfg);
            AmbaKAL_TaskSleep(3); //temp sloution to wait memory sync

            #if 0
            {
                char fn[32], fn1[32];
                UINT8 *LumaAddr, *ChromaAddr;
                UINT16 Pitch;
                AMP_CFS_FILE_s *y = NULL;
                AMP_CFS_FILE_s *uv = NULL;
                char mdASCII[3] = {'w','+','\0'};
                AMP_AREA_s Aoi;

                sprintf(fn,"%s:\\Dchan.y", DefaultSlot);
                sprintf(fn1,"%s:\\Dchan.uv", DefaultSlot);
                LumaAddr = ptr->yAddr;
                ChromaAddr = ptr->uvAddr;
                Pitch = ptr->pitch;
                Aoi.Width = ptr->width;
                Aoi.Height = ptr->height;

                AmbaPrint("[Amp_UT] Dump DCHAN YUV (%s %s) to (0x%X 0x%X) %d %d %d Start!", \
                    fn, fn1, LumaAddr, ChromaAddr, Pitch, Aoi.Width, Aoi.Height);
                y = UT_StillTunningEncodefopen((const char *)fn, (const char *)mdASCII);
                AmpUT_StillTuningDumpAoi(LumaAddr, Pitch, Aoi, (void *)y);
                UT_StillTunningEncodefsync((void *)y);
                UT_StillTunningEncodefclose((void *)y);

                uv = UT_StillTunningEncodefopen((const char *)fn1, (const char *)mdASCII);
                AmpUT_StillTuningDumpAoi(ChromaAddr, Pitch, Aoi, (void *)uv);
                UT_StillTunningEncodefsync((void *)y);
                UT_StillTunningEncodefclose((void *)uv);
                AmbaPrint("[Amp_UT] Dump DCHAN YUV Done!");
            }
            #endif

            if (QvDisplayCfg != 0) {
                STILLTUNING_UT_MSG_S msg;

                if (QvLcdShowBuffIndex == 0) {
                    //Even buffer is using
                    memcpy(QvLcdShowBuffAddrOdd, ptr->yAddr, ptr->ySize*2);
                    AmbaCache_Clean((void*)QvLcdShowBuffAddrOdd, ptr->ySize*2);
                } else {
                    //Odd buffer is using
                    memcpy(QvLcdShowBuffAddrEven, ptr->yAddr, ptr->ySize*2);
                    AmbaCache_Clean((void*)QvLcdShowBuffAddrEven, ptr->ySize*2);
                }

                msg.Event = MSG_STILLTUNING_UT_QV_LCD_READY;
                msg.Data.yuvInfo.yAddr = (QvLcdShowBuffIndex == 0)? QvLcdShowBuffAddrOdd: QvLcdShowBuffAddrEven;
                msg.Data.yuvInfo.ySize = ptr->ySize;
                msg.Data.yuvInfo.pitch = ptr->pitch;
                msg.Data.yuvInfo.width = ptr->width;
                msg.Data.yuvInfo.height = ptr->height;
                AmbaKAL_MsgQueueSend(&TuningUtMsgQueue, &msg, AMBA_KAL_NO_WAIT);
            }
#endif
        }
            break;
        case AMP_ENC_EVENT_QUICKVIEW_FCHAN_YUV_READY:
        {
            // In A9, app must copy LCD or HDMI qv to their local buffer and ask DISPLAY module to show defIMG.
            // But in A12, we need to switch display to video src as soon as qv_data_rdy to prevent vout buffer full.
            // preview_a buffer is control by uCode in a12, APP is able to access it.
            // preview_b buffer is control by uCode in a12, App is unable to access it.
#ifdef CONFIG_SOC_A12
            AMP_DISP_WINDOW_CFG_s Window = {0};
            UINT8 ArIndex = 0;
            if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
            else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

            if (1) {
                Window.Source = AMP_DISP_ENC;
                Window.SourceDesc.Enc.VinCh = TuningChannel;
                Window.CropArea.Width = 0;
                Window.CropArea.Height = 0;
                Window.CropArea.X = 0;
                Window.CropArea.Y = 0;
                Window.TargetAreaOnPlane.Width = StillTuningVoutMgt[1][ArIndex].Width;
                Window.TargetAreaOnPlane.Height = StillTuningVoutMgt[1][ArIndex].Height;
                Window.TargetAreaOnPlane.X = (1920 - Window.TargetAreaOnPlane.Width)/2;
                Window.TargetAreaOnPlane.Y = 0;
                AmpUT_Display_SetWindowCfg(1, &Window);
                AmpUT_Display_Act_Window(1);
                AmbaPrint("[AmpUT_StillEnc] QV_FCHAN_YUV_READY, switch to VDSRC");

            } else {
                AmbaPrint("[AmpUT_StillEnc] ERROR, A12 MUST turn on VDSRC as soon as received QV_FCHAN_YUV_READY");
            }
#else
            AMP_ENC_YUV_INFO_s *ptr = info;
            //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
            AmbaPrint("[AmpUT_StillTuning] AMP_ENC_EVENT_QUICKVIEW_FCHAN_YUV_READY info: 0x%X (%d %d %d) %u", ptr->yAddr, ptr->pitch, ptr->width, ptr->height, ptr->ySize);
            AmbaKAL_TaskSleep(3); //temp sloution to wait memory sync

            #if 0
            {
                char fn[32], fn1[32];
                UINT8 *LumaAddr, *ChromaAddr;
                UINT16 Pitch;
                AMP_CFS_FILE_s *y = NULL;
                AMP_CFS_FILE_s *uv = NULL;
                char mdASCII[3] = {'w','+','\0'};
                AMP_AREA_s Aoi;

                sprintf(fn,"%s:\\Fchan.y", DefaultSlot);
                sprintf(fn1,"%s:\\Fchan.uv", DefaultSlot);
                LumaAddr = ptr->yAddr;
                ChromaAddr = ptr->uvAddr;
                Pitch = ptr->pitch;
                Aoi.Width = ptr->width;
                Aoi.Height = ptr->height;

                AmbaPrint("[Amp_UT] Dump FCHAN YUV (%s %s) to (0x%X 0x%X) %d %d %d Start!", \
                    fn, fn1, LumaAddr, ChromaAddr, Pitch, Aoi.Width, Aoi.Height);
                y = UT_StillTunningEncodefopen((const char *)fn, (const char *)mdASCII);
                AmpUT_StillTuningDumpAoi(LumaAddr, Pitch, Aoi, (void *)y);
                UT_StillTunningEncodefsync((void *)y);
                UT_StillTunningEncodefclose((void *)y);

                uv = UT_StillTunningEncodefopen((const char *)fn1, (const char *)mdASCII);
                AmpUT_StillTuningDumpAoi(ChromaAddr, Pitch, Aoi, (void *)uv);
                UT_StillTunningEncodefsync((void *)uv);
                UT_StillTunningEncodefclose((void *)uv);
                AmbaPrint("[Amp_UT] Dump FCHAN YUV Done!");
            }
            #endif

            if (QvDisplayCfg != 0) {
                //TBD
                #if 0
                    AmbaPrint("[Amp_UT] Copy FCHAN YUV Start!");
                    if (QvTvShowBuffIndex == 0) {
                        //Even buffer is using
                        memcpy(QvTvShowBuffAddrOdd, ptr->yAddr, ptr->ySize*2);
                    } else {
                        //Odd buffer is using
                        memcpy(QvTvShowBuffAddrEven, ptr->yAddr, ptr->ySize*2);
                    }
                    AmbaPrint("[Amp_UT] Copy FCHAN YUV Done!");
                #endif
            }
#endif
        }
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("[AmpUT_StillTuning] ~~~ AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD ~~~");
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("[AmpUT_StillTuning] ~~~ AMP_ENC_EVENT_DATA_OVERRUN ~~~");
            break;
        case AMP_ENC_EVENT_DESC_OVERRUN:
            AmbaPrint("[AmpUT_StillTuning] ~~~ AMP_ENC_EVENT_DESC_OVERRUN ~~~");
            break;
        default:
            AmbaPrint("[AmpUT_StillTuning] Unknown %X info: %x", event, info);
            break;
    }
    return 0;
}

static int AmpUT_StillTuningPipeCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
        {
            AMP_ENC_STATE_CHANGED_INFO_s *inf = info;

            AmbaPrint("[StillTuning]PipeCB: AMP_ENC_EVENT_STATE_CHANGED newState %X", inf->newState);
        }
        break;
    }
    return 0;

}
static int AmpUT_StillTuning_DisplayStart(void)
{
    AMP_DISP_WINDOW_CFG_s window;
    UINT8 ArIndex = 0;

    if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillTuningMgt[StillTuningModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /** Step 1: Display config & window config */
//    if (AmpUT_Display_Init() == NG) {
//        return NG;
//    }

    // Creat LCD Window
    if (1) {
        memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
        window.Source = AMP_DISP_ENC;
        window.SourceDesc.Enc.VinCh = TuningChannel;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;
        window.TargetAreaOnPlane.Width = StillTuningVoutMgt[0][ArIndex].Width;
        window.TargetAreaOnPlane.Height = StillTuningVoutMgt[0][ArIndex].Height;
        window.TargetAreaOnPlane.X = 0;
        window.TargetAreaOnPlane.Y = (480-window.TargetAreaOnPlane.Height)/2;
        if (AmpUT_Display_Window_Create(0, &window) == NG) {
            return NG;
        }
    }

    // Creat TV Window
    if (1) {
        memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
        window.Source = AMP_DISP_ENC;
        window.SourceDesc.Enc.VinCh = TuningChannel;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;
        window.TargetAreaOnPlane.Width = StillTuningVoutMgt[1][ArIndex].Width;
        window.TargetAreaOnPlane.Height = StillTuningVoutMgt[1][ArIndex].Height;
        window.TargetAreaOnPlane.X = (1920-window.TargetAreaOnPlane.Width)/2;
        window.TargetAreaOnPlane.Y = 0;
        if (AmpUT_Display_Window_Create(1, &window) == NG) {
            return NG;
        }
    }

    /** Step 2: Setup device */
    // Setup LCD & TV
    if (StillTuningLCDLiveview) {
        AmpUT_Display_Start(0);
    } else {
        AmpUT_Display_Stop(0);
    }
    if (StillTuningTvLiveview) {
        AmpUT_Display_Start(1);
    } else {
        AmpUT_Display_Stop(1);
    }

    // Active Window 7002 cmd
    if (StillTuningLCDLiveview) {
        AmpUT_Display_Act_Window(0);
    } else {
        AmpUT_Display_DeAct_Window(0);
    }
    if (StillTuningTvLiveview) {
        AmpUT_Display_Act_Window(1);
    } else {
        AmpUT_Display_DeAct_Window(1);
    }

    TuningVoutLcdSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;
    TuningVoutTvSrc = (AMBA_DSP_VOUT_VIDEO_SRC_e)VOUT_SRC_VIDEO_CAPTURE;

    return 0;
}

int AmpUT_StillTuning_Init(int sensorID, int LcdID)
{
    int er = OK;
    void *TmpbufRaw = NULL;
    if (StillTuningCodecInit) {
        AmbaPrint("StillEnc already init");
        return er;
    }

    //imgproc initialization

    // Register LCD driver
    InputDeviceId = sensorID;
//    if (LcdID >= 0) {
//        if (LcdID == 0) {
//        #ifdef CONFIG_LCD_WDF9648W
//            AmbaPrint("Hook Wdf9648w LCD");
//            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_WdF9648wObj);
//        #else
//            AmbaPrint("Please enable Wdf9648w LCD");
//        #endif
//        }
//    }

    // Register sensor driver
    if (sensorID >= 0) {
        memset(&TuningChannel, 0x0, sizeof(AMBA_DSP_CHANNEL_ID_u));
        TuningChannel.Bits.VinID = 0;
        TuningChannel.Bits.SensorID = 0x1;
        StillTuningMgt = RegisterMWUT_Sensor_Driver(TuningChannel, sensorID);
        {
        #ifdef CONFIG_SOC_A12 // Temporary disable for A9
            extern int App_Image_Init(UINT32 ChCount, int sensorID);
            App_Image_Init(1, sensorID);
        #endif
        }
    }

    // Create simple muxer semophore
    if (AmbaKAL_SemCreate(&StillTuningSem, 0) != OK) {
        AmbaPrint("StillEnc UnitTest: Semaphore creation failed");
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_StillTuningMuxStack, (void **)&TmpbufRaw, STILL_TUNING_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    // Create simple muxer task
    if (AmbaKAL_TaskCreate(&StillTuningMuxTask, "Still Tuning UnitTest Muxing Task", 10, \
         AmpUT_StillTuning_MuxTask, 0x0, AmpUT_StillTuningMuxStack, STILL_TUNING_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("StillTuning UnitTest: Muxer task creation failed");
    }

    // Simple UnitTest Task
    {
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_StillTuningUtStack, (void **)&TmpbufRaw, STILL_TUNING_UT_TASK_STACK_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for UnitTest stack!!");
        }

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_StillTuningUtMsgQPool, (void **)&TmpbufRaw, (sizeof(STILLTUNING_UT_MSG_S)*MAX_AMPUT_STILLTUNING_UT_MSG_NUM), 32);
        if (er != OK) {
            AmbaPrint("Out of memory for UnitTestMsgQPool!!");
        }

        // Create mQueue
        er = AmbaKAL_MsgQueueCreate(&TuningUtMsgQueue, AmpUT_StillTuningUtMsgQPool, sizeof(STILLTUNING_UT_MSG_S), MAX_AMPUT_STILLTUNING_UT_MSG_NUM);
        if (er != OK) {
            AmbaPrint("UnitTest MsgQPoll create fail");
        }

        // Create simple still_UT task
        if (AmbaKAL_TaskCreate(&StillTuningUtTask, "Still Encoder UnitTest Task", 50, \
             AmpUT_StillTuning_UtTask, 0x0, AmpUT_StillTuningUtStack, STILL_TUNING_UT_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
             AmbaPrint("UnitTest task creation failed");
        }

        // Create Event flag
        er = AmbaKAL_EventFlagCreate(&StillTuningUTTaskFlag);
        if (er != OK)
            AmbaPrint("UnitTest Flag create fail");
    }

    // Create Still encoder object
    {
        AMP_STILLENC_INIT_CFG_s encInitCfg;

        // Init STILLENC module
        AmpStillEnc_GetInitDefaultCfg(&encInitCfg);
        if (StillTuningWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillTuningWorkBuf, (void **)&TmpbufRaw, encInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for stillmain!!");
            }
        }
        encInitCfg.MemoryPoolAddr = StillTuningWorkBuf;
        AmpStillEnc_Init(&encInitCfg);
    }

    if (AmpUT_Display_Init() == NG) {
        return NG;
    }

    StillTuningCodecInit = 1;

    return 0;
}

int AmpUT_StillTuning_LiveviewStart(UINT32 encID)
{
    int Er = OK;
    void *TmpbufRaw = NULL;
    if (StillTuningVinA) {
        if (STuningImgSchdlr == NULL) {
            AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg = {0};
            AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);
            ImgSchdlrCfg.MainViewID = 0; //single channle have one MainView
            ImgSchdlrCfg.Channel = TuningChannel;
            ImgSchdlrCfg.Vin = StillTuningVinA;
            ImgSchdlrCfg.cbEvent = AmpUT_StillTuningImgSchdlrCallback;
            if (StillTuningLiveViewProcMode && StillTuningLiveViewAlgoMode) {
                ImgSchdlrCfg.VideoProcMode = 1;
            }
            AmbaImgSchdlr_Create(&ImgSchdlrCfg, &STuningImgSchdlr);  // One sensor (not vin) need one scheduler.
        }

        AmpUT_StillTuning_ChangeResolution(encID);
        AmpEnc_StartLiveview(StillTuningPipe, AMP_ENC_FUNC_FLAG_WAIT);
        return 0;
    }

    StillTuningModeIdx = encID;

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Liveview at %s", MWUT_GetInputVideoModeName(encID, StillTuningEncodeSystem));
    AmbaPrint(" =========================================================");

    // Vin module init
    if (1 /* Vin not create yet */) {
        AMP_VIN_INIT_CFG_s vinInitCfg;

       // Init VIN module
        AmpVin_GetInitDefaultCfg(&vinInitCfg);
        if (StillTuningVinWorkBuf == NULL) {
            Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillTuningVinWorkBuf, (void **)&TmpbufRaw, vinInitCfg.MemoryPoolSize, 32);
            if (Er != OK) {
                AmbaPrint("Out of memory for vin!!");
            }
        }
        vinInitCfg.MemoryPoolAddr = StillTuningVinWorkBuf;
        AmpVin_Init(&vinInitCfg);
    }

    //ImgSchdlr module init
    {
        AMBA_IMG_SCHDLR_INIT_CFG_s IsInitCfg = {0};
        UINT32 MainViewNumber = 1;
        UINT32 ISPoolSize = 0;
        AMBA_DSP_IMG_PIPE_e Pipe = AMBA_DSP_IMG_PIPE_VIDEO;

        AmbaImgSchdlr_GetInitDefaultCfg(&IsInitCfg);
        IsInitCfg.MainViewNum = MainViewNumber;
        AmbaImgSchdlr_QueryMemsize(MainViewNumber, &ISPoolSize);

#ifdef CONFIG_SOC_A9
        IsInitCfg.MemoryPoolSize = ISPoolSize;
#else
        IsInitCfg.MemoryPoolSize = ISPoolSize + IsInitCfg.MsgTaskStackSize;
#endif

        if (StillTuningImgSchdlrWorkBuf == NULL) {
            Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillTuningImgSchdlrWorkBuf, (void **)&TmpbufRaw, IsInitCfg.MemoryPoolSize + 32, 32);
            if (Er != OK) {
                AmbaPrint("Out of memory for imgschdlr!!");
            } else StillTuningImgSchdlrWorkBuf = (UINT8 *)ALIGN_32((UINT32)StillTuningImgSchdlrWorkBuf);
        }

        IsInitCfg.IsoCfgNum = AmpResource_GetIKIsoConfigNumber(Pipe);
        IsInitCfg.MemoryPoolAddr = StillTuningImgSchdlrWorkBuf;
        AmbaImgSchdlr_Init(&IsInitCfg);
    }

    // Create Vin instance
    if (StillTuningVinA == NULL) {
        AMBA_SENSOR_MODE_INFO_s VinInfo = {0};
        AMP_VIN_HDLR_CFG_s VinCfg = {0};
        AMP_VIN_LAYOUT_CFG_s Layout = {0};
        AMBA_SENSOR_MODE_ID_u SensorMode = {0};

        SensorMode.Data = StillTuningMgt[encID].InputMode;
        AmbaSensor_GetModeInfo(TuningChannel, SensorMode, &VinInfo);

        // Create VIN instance
        AmpVin_GetDefaultCfg(&VinCfg);
        VinCfg.Mode = SensorMode;
        if (StillTuningRotate == AMP_ROTATE_180_HORZ_FLIP) VinCfg.Mode.Bits.VerticalFlip = 1;
        else VinCfg.Mode.Bits.VerticalFlip = 0;
        VinCfg.Channel = TuningChannel;
        VinCfg.LayoutNumber = 1;
        if (StillTuningMultiChan) {
            VinCfg.HwCaptureWindow.Width = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.Width;
            VinCfg.HwCaptureWindow.Height = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.Height;
            VinCfg.HwCaptureWindow.X = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.X;
            VinCfg.HwCaptureWindow.Y = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.Y;
            Layout.Width = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainWidth;
            Layout.Height = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainHeight;
        } else {
            VinCfg.HwCaptureWindow.Width = StillTuningMgt[encID].CaptureWidth;
            VinCfg.HwCaptureWindow.Height = StillTuningMgt[encID].CaptureHeight;
            VinCfg.HwCaptureWindow.X = VinInfo.OutputInfo.RecordingPixels.StartX + (((VinInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
            VinCfg.HwCaptureWindow.Y = (VinInfo.OutputInfo.RecordingPixels.StartY + ((VinInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;
            Layout.Width = StillTuningMgt[encID].MainWidth;
            Layout.Height = StillTuningMgt[encID].MainHeight;
        }

        Layout.EnableSourceArea = 0; // Get all capture window to main
        Layout.DzoomFactorX = 1<<16;
        Layout.DzoomFactorY = 1<<16;
        Layout.DzoomOffsetX = 0;
        Layout.DzoomOffsetY = 0;
        VinCfg.Layout = &Layout;
        VinCfg.cbEvent = AmpUT_StillTuningVinEventCallback;
        VinCfg.cbSwitch = AmpUT_StillTuningVinSwitchCallback;
        AmbaPrint("Vin creat %d %d %d", VinCfg.Mode, Layout.Width, Layout.Height);

        AmpVin_Create(&VinCfg, &StillTuningVinA);
    }

    // Create ImgSchdlr instance
    {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg = {0};

        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

        ImgSchdlrCfg.MainViewID = 0; //single channle have one MainView
        ImgSchdlrCfg.Channel = TuningChannel;
        ImgSchdlrCfg.Vin = StillTuningVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_StillTuningImgSchdlrCallback;
        if (StillTuningLiveViewProcMode && StillTuningLiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &STuningImgSchdlr);  // One sensor (not vin) need one scheduler.
    }

    // Create Still encoder object
    if (StillTuningPri == NULL) {
        AMP_STILLENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s Elayer = {0};

        EncCfg.MainLayout.Layer = &Elayer;
        AmpStillEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_StillTuningCallback;

        // Assign main layout
        if (StillTuningMultiChan) {
            Elayer.SourceType = AMP_ENC_SOURCE_VIN;
            Elayer.Source = StillTuningVinA;
            Elayer.SourceLayoutId = 0;
            Elayer.EnableSourceArea = 1;
            Elayer.EnableTargetArea = 1;
            Elayer.SourceArea = MultiChanMainWindow[0].MainCapWin;
            Elayer.TargetArea.Width = MultiChanMainWindow[0].MainWidth;
            Elayer.TargetArea.Height = MultiChanMainWindow[0].MainHeight;

            EncCfg.MainLayout.LayerNumber = 1;
            EncCfg.MainLayout.Width = MultiChanMainWindow[0].MainWidth;
            EncCfg.MainLayout.Height = MultiChanMainWindow[0].MainHeight;
        } else {
            Elayer.SourceType = AMP_ENC_SOURCE_VIN;
            Elayer.Source = StillTuningVinA;
            Elayer.SourceLayoutId = 0;
            Elayer.EnableSourceArea = 0;  // No source cropping
            Elayer.EnableTargetArea = 0;  // No target pip

            EncCfg.MainLayout.Width = StillTuningMgt[encID].MainWidth;
            EncCfg.MainLayout.Height = StillTuningMgt[encID].MainHeight;
        }
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = StillTuningMgt[encID].Interlace;
        EncCfg.MainTimeScale = StillTuningMgt[encID].TimeScale;
        EncCfg.MainTickPerPicture = StillTuningMgt[encID].TickPerPicture;
        {
            UINT8 *DspWorkAddr = NULL;
            UINT32 DspWorkSize = 0;
            AmpUT_StillTuning_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
            EncCfg.DspWorkBufAddr = DspWorkAddr;
            EncCfg.DspWorkBufSize = DspWorkSize;
        }

        if (StillTuningLiveViewProcMode == 0) {
            if (StillTuningMgt[encID].StillMainWidth > 1920 || \
                StillTuningMgt[encID].StillCaptureWidth > 1920)
                StillTuningLiveViewOSMode = 1;
        }

        EncCfg.LiveViewProcMode = StillTuningLiveViewProcMode;
        EncCfg.LiveViewAlgoMode = StillTuningLiveViewAlgoMode;
        EncCfg.LiveViewOSMode = StillTuningLiveViewOSMode;

        // Assign bitstream/descriptor buffer
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillTuningBitsBuf, (void **)&TmpbufRaw, STILL_TUNING_BISFIFO_SIZE, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }
        EncCfg.BitsBufCfg.BitsBufAddr = StillTuningBitsBuf;
        EncCfg.BitsBufCfg.BitsBufSize = STILL_TUNING_BISFIFO_SIZE;

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillTuningDescBuf, (void **)&TmpbufRaw, STILL_TUNING_DESC_SIZE, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for DescFifo!!");
        }
        EncCfg.BitsBufCfg.DescBufAddr = StillTuningDescBuf;
        EncCfg.BitsBufCfg.DescBufSize = STILL_TUNING_DESC_SIZE;
        EncCfg.BitsBufCfg.BitsRunoutThreshold = STILL_TUNING_BISFIFO_SIZE - (4*1024*1024);
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", \
            StillTuningBitsBuf, STILL_TUNING_BISFIFO_SIZE, StillTuningDescBuf, STILL_TUNING_DESC_SIZE);

    #ifdef CONFIG_SOC_A9
        EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    #else
        EncCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
        EncCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_POWERSAVING;
        EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
        EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
        EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
    #endif

        AmbaPrint("Enc creat %d %d %d %d", EncCfg.MainLayout.Width, EncCfg.MainLayout.Height, \
        EncCfg.MainTimeScale, EncCfg.MainTickPerPicture);
        AmpStillEnc_Create(&EncCfg, &StillTuningPri);

        // create a virtual fifo
        if (StillTuningVirtualFifoHdlr == NULL) {
            AMP_FIFO_CFG_s FifoDefCfg = {0};
            AmpFifo_GetDefaultCfg(&FifoDefCfg);
            FifoDefCfg.hCodec = StillTuningPri;
            FifoDefCfg.IsVirtual = 1;
            FifoDefCfg.NumEntries = 1024;
            FifoDefCfg.cbEvent = AmpUT_StillTuning_FifoCB;
            AmpFifo_Create(&FifoDefCfg, &StillTuningVirtualFifoHdlr);
        }
    } else {
        //May have to update VIN
        AMP_VIN_RUNTIME_CFG_s VinCfg = {0};
        AMP_STILLENC_MAIN_CFG_s MainCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s Newlayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
        AMBA_SENSOR_MODE_INFO_s VinInfo = {0};
        AMP_VIN_LAYOUT_CFG_s Layout = {0};
        AMBA_SENSOR_MODE_ID_u SensorMode = {0};

        SensorMode.Data = StillTuningMgt[encID].InputMode;
        AmbaSensor_GetModeInfo(TuningChannel, SensorMode, &VinInfo);

        VinCfg.Hdlr = StillTuningVinA;
        VinCfg.Mode = SensorMode;
        VinCfg.LayoutNumber = 1;
        VinCfg.HwCaptureWindow.Width = StillTuningMgt[encID].CaptureWidth;
        VinCfg.HwCaptureWindow.Height = StillTuningMgt[encID].CaptureHeight;
        VinCfg.HwCaptureWindow.X = VinInfo.OutputInfo.RecordingPixels.StartX + (((VinInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        VinCfg.HwCaptureWindow.Y = (VinInfo.OutputInfo.RecordingPixels.StartY + ((VinInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;
        Layout.Width = StillTuningMgt[encID].MainWidth;
        Layout.Height = StillTuningMgt[encID].MainHeight;
        Layout.EnableSourceArea = 0; // Get all capture window to main
        Layout.DzoomFactorX = 1<<16;
        Layout.DzoomFactorY = 1<<16;
        Layout.DzoomOffsetX = 0;
        Layout.DzoomOffsetY = 0;
        VinCfg.Layout = &Layout;

        MainCfg.Hdlr = StillTuningPri;
        MainCfg.MainLayout.LayerNumber = 1;
        MainCfg.MainLayout.Layer = &Newlayer;
        MainCfg.MainLayout.Width = StillTuningMgt[encID].MainWidth;
        MainCfg.MainLayout.Height = StillTuningMgt[encID].MainHeight;
        MainCfg.Interlace = StillTuningMgt[encID].Interlace;
        MainCfg.MainTickPerPicture = StillTuningMgt[encID].TickPerPicture;
        MainCfg.MainTimeScale = StillTuningMgt[encID].TimeScale;
        {
            UINT8 *DspWorkAddr = NULL;
            UINT32 DspWorkSize = 0;
            AmpUT_StillTuning_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
            MainCfg.DspWorkBufAddr = DspWorkAddr;
            MainCfg.DspWorkBufSize = DspWorkSize;
        }
        Newlayer.EnableSourceArea = 0;
        Newlayer.EnableTargetArea = 0;
        Newlayer.LayerId = 0;
        Newlayer.SourceType = AMP_ENC_SOURCE_VIN;
        Newlayer.Source = StillTuningVinA;
        Newlayer.SourceLayoutId = 0;

        if (StillTuningLiveViewProcMode == 0) {
            if (StillTuningMgt[encID].StillMainWidth > 1920 || \
                StillTuningMgt[encID].StillCaptureWidth > 1920)
                StillTuningLiveViewOSMode = 1;
        }

        MainCfg.LiveViewProcMode = StillTuningLiveViewProcMode;
        MainCfg.LiveViewAlgoMode = StillTuningLiveViewAlgoMode;
        MainCfg.LiveViewOSMode = StillTuningLiveViewOSMode;

        AmpStillEnc_UpdateVinMain(1, &VinCfg, &MainCfg);
    }

    //MultiChan dualVout will have preformance issue
    if (StillTuningMultiChan) {
        AmbaPrint("Turn OFF LCD");
        StillTuningLCDLiveview = 0;
    } else {
        AmbaPrint("Turn ON LCD");
        StillTuningLCDLiveview = 1;
    }

    // Init Display
    AmpUT_StillTuning_DisplayStart();

    // Register pipeline
    if (StillTuningPipe == NULL) {
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};
        // Register pipeline
        AmpEnc_GetDefaultCfg(&PipeCfg);
        PipeCfg.encoder[0] = StillTuningPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_StillTuningPipeCallback;
        PipeCfg.type = AMP_ENC_STILL_PIPE;
        AmpEnc_Create(&PipeCfg, &StillTuningPipe);

        AmpEnc_Add(StillTuningPipe);
    }

    AmpEnc_StartLiveview(StillTuningPipe, AMP_ENC_FUNC_FLAG_WAIT);
    Status = TUNING_STATUS_STILL_LIVEVIEW;

    return 0;
}

int AmpUT_StillTuning_ChangeResolution(UINT32 encID)
{
    AMP_VIN_RUNTIME_CFG_s vinCfg = {0};
    AMP_STILLENC_MAIN_CFG_s mainCfg = {0};
    AMP_VIDEOENC_LAYER_DESC_s newlayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
    AMBA_SENSOR_MODE_INFO_s vinInfo;
    AMP_VIN_LAYOUT_CFG_s layout;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};

    StillTuningModeIdx = encID;

    AmbaPrint(" =========================================================");
    AmbaPrint(" AmbaUnitTest: Change Resolution to %s", MWUT_GetInputVideoModeName(encID, StillTuningEncodeSystem));
    AmbaPrint(" =========================================================");

    memset(&layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s));
    SensorMode.Data = StillTuningMgt[encID].InputMode;
    AmbaSensor_GetModeInfo(TuningChannel, SensorMode, &vinInfo);

    vinCfg.Hdlr = StillTuningVinA;
    vinCfg.Mode = SensorMode;
    if (StillTuningRotate == AMP_ROTATE_180_HORZ_FLIP) vinCfg.Mode.Bits.VerticalFlip = 1;
    else vinCfg.Mode.Bits.VerticalFlip = 0;
    vinCfg.LayoutNumber = 1;

    if (StillTuningMultiChan) {
        vinCfg.HwCaptureWindow.Width = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.Width;
        vinCfg.HwCaptureWindow.Height = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.Height;
        vinCfg.HwCaptureWindow.X = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.X;
        vinCfg.HwCaptureWindow.Y = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainCapWin.Y;
        layout.Width = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainWidth;
        layout.Height = MultiChanMainWindow[StillTuningMultiChanMainViewID].MainHeight;
    } else {
        vinCfg.HwCaptureWindow.Width = StillTuningMgt[encID].CaptureWidth;
        vinCfg.HwCaptureWindow.Height = StillTuningMgt[encID].CaptureHeight;
        vinCfg.HwCaptureWindow.X = vinInfo.OutputInfo.RecordingPixels.StartX + (((vinInfo.OutputInfo.RecordingPixels.Width - vinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        vinCfg.HwCaptureWindow.Y = (vinInfo.OutputInfo.RecordingPixels.StartY + ((vinInfo.OutputInfo.RecordingPixels.Height - vinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;
        layout.Width = StillTuningMgt[encID].MainWidth;
        layout.Height = StillTuningMgt[encID].MainHeight;
    }

    layout.EnableSourceArea = 0; // Get all capture window to main
    layout.DzoomFactorX = 1<<16;
    layout.DzoomFactorY = 1<<16;
    layout.DzoomOffsetX = 0;
    layout.DzoomOffsetY = 0;
    vinCfg.Layout = &layout;

    mainCfg.Hdlr = StillTuningPri;
    mainCfg.MainLayout.LayerNumber = 1;
    mainCfg.MainLayout.Layer = &newlayer;
    mainCfg.MainLayout.Width = StillTuningMgt[encID].MainWidth;
    mainCfg.MainLayout.Height = StillTuningMgt[encID].MainHeight;
    mainCfg.Interlace = StillTuningMgt[encID].Interlace;
    mainCfg.MainTickPerPicture = StillTuningMgt[encID].TickPerPicture;
    mainCfg.MainTimeScale = StillTuningMgt[encID].TimeScale;
    if (StillTuningMultiChan) {
        newlayer.EnableSourceArea = 1;
        newlayer.EnableTargetArea = 1;
        newlayer.SourceArea = MultiChanMainWindow[0].MainCapWin;
        newlayer.TargetArea.Width = MultiChanMainWindow[0].MainWidth;
        newlayer.TargetArea.Height = MultiChanMainWindow[0].MainHeight;

        mainCfg.MainLayout.Width = MultiChanMainWindow[0].MainWidth;
        mainCfg.MainLayout.Height = MultiChanMainWindow[0].MainHeight;
    } else {
        newlayer.EnableSourceArea = 0;
        newlayer.EnableTargetArea = 0;
        mainCfg.MainLayout.Width = StillTuningMgt[encID].MainWidth;
        mainCfg.MainLayout.Height = StillTuningMgt[encID].MainHeight;
    }

#ifdef CONFIG_SOC_A9
    mainCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    mainCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    mainCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
#else
    mainCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
    mainCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
    mainCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_POWERSAVING;
    mainCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
    mainCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
    mainCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
#endif

    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        AmpUT_StillTuning_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        mainCfg.DspWorkBufAddr = dspWorkAddr;
        mainCfg.DspWorkBufSize = dspWorkSize;
    }

    if (StillTuningLiveViewProcMode == 0) {
        if (StillTuningMgt[encID].StillMainWidth > 1920 || \
            StillTuningMgt[encID].StillCaptureWidth > 1920)
            StillTuningLiveViewOSMode = 1;
    }

    mainCfg.LiveViewProcMode = StillTuningLiveViewProcMode;
    mainCfg.LiveViewAlgoMode = StillTuningLiveViewAlgoMode;
    mainCfg.LiveViewOSMode = StillTuningLiveViewOSMode;

    newlayer.LayerId = 0;
    newlayer.SourceType = AMP_ENC_SOURCE_VIN;
    newlayer.Source = StillTuningVinA;
    newlayer.SourceLayoutId = 0;

    AmpStillEnc_ConfigVinMain(1, &vinCfg, &mainCfg);

    return 0;
}

UINT32 CalibRawPP(AMP_STILLENC_POSTP_INFO_s *PostpInfo)
{
    char fn[64];
    AMP_CFS_FILE_s *raw = NULL;

    char mdASCII[3] = {'w','+','\0'};

    AmbaPrint("[CalibRawPP] Raw 0x%X, p-%d, w-%d, h-%d", \
        PostpInfo->media.RawInfo.RawAddr, \
        PostpInfo->media.RawInfo.RawPitch, \
        PostpInfo->media.RawInfo.RawWidth, \
        PostpInfo->media.RawInfo.RawHeight);

    //raw ready, dump it
    sprintf(fn,"%s:\\StillRawCap.RAW", DefaultSlot);

    raw = UT_StillTunningEncodefopen((const char *)fn, (const char *)mdASCII);
    AmbaPrint("[Amp_UT]Dump Raw %s Start!", fn);
    UT_StillTunningEncodefwrite((const void *)PostpInfo->media.RawInfo.RawAddr, 1, PostpInfo->media.RawInfo.RawPitch*PostpInfo->media.RawInfo.RawHeight, (void *)raw);
    UT_StillTunningEncodefsync((void *)raw);
    UT_StillTunningEncodefclose((void *)raw);

    if (StillTuningDumpItuner == 1) {
        char TuningModeExt[32] = "SINGLE_SHOT";
        char ItunerFileName[64];
        snprintf(ItunerFileName, sizeof(ItunerFileName), "%s:\\A12_StillRawCap.txt", DefaultSlot);
        AmpUT_StillTuningDumpItunerFile(ItunerFileName, TuningModeExt, fn);
    }

    return 0;
}

/* Inform 3A to lock AE/AWB before capture */
UINT32 AmpUT_StillTuningAAALock(void)
{
    UINT8 CurrMode = IP_PREVIEW_MODE;
    UINT8 NextMode = IP_CAPTURE_MODE;
    UINT32 ChNo = 0;
    AMBA_3A_OP_INFO_s AaaOpInfo = {0};
    AMBA_3A_STATUS_s VideoStatus = {0};
    AMBA_3A_STATUS_s StillStatus = {0};

    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
    AmbaImg_Proc_Cmd(MW_IP_SET_MODE, (UINT32)&CurrMode, (UINT32)&NextMode, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, ChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);

    if (0/*CaptureMode == STILL_CAPTURE_AEB*/) {
        AmbaImg_Proc_Cmd(MW_IP_SET_CAP_FORMAT, ChNo, IMG_CAP_AEB, 0);
    } else {
        AmbaImg_Proc_Cmd(MW_IP_SET_CAP_FORMAT, ChNo, IMG_CAP_NORMAL, 0);
    }

    // Wait AE lock
    if (AaaOpInfo.AeOp == ENABLE) {
        static UINT16 StillTuningAeWaitCnt = 666;
        while (StillStatus.Ae != AMBA_LOCK) {
            AmbaKAL_TaskSleep(3);
            AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, ChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
            StillTuningAeWaitCnt--;
            if (StillTuningAeWaitCnt == 0) {
                AmbaPrintColor(GREEN, "[UT_Still] Ae lock?");
                StillTuningAeWaitCnt = 666;
            }
        }
    } else if (AaaOpInfo.AeOp == DISABLE) {
        AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, ChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
        VideoStatus.Ae = AMBA_LOCK;
        StillStatus.Ae = AMBA_LOCK;
        AmbaImg_Proc_Cmd(MW_IP_SET_AE_STATUS, ChNo, (UINT32)VideoStatus.Ae, (UINT32)StillStatus.Ae);
    }

    // Wait AWB lock
    if (AaaOpInfo.AwbOp == ENABLE) {
        static UINT16 StillTuningAwbWaitCnt = 666;
        while (StillStatus.Awb != AMBA_LOCK) {
            AmbaKAL_TaskSleep(3);
            AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, ChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
            StillTuningAwbWaitCnt--;
            if (StillTuningAwbWaitCnt == 0) {
                AmbaPrintColor(GREEN, "[UT_Still] Awb lock?");
                StillTuningAwbWaitCnt = 666;
            }
        }
    } else if (AaaOpInfo.AwbOp == DISABLE) {
        AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, ChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
        VideoStatus.Awb = AMBA_LOCK;
        StillStatus.Awb = AMBA_LOCK;
        AmbaImg_Proc_Cmd(MW_IP_SET_AWB_STATUS, ChNo, (UINT32)VideoStatus.Awb, (UINT32)StillStatus.Awb);
    }

    return OK;
}

/* Inform 3A to unlock AE/AWB before b2lv */
UINT32 AmpUT_StillTuningUnLockAE(void)
{
    UINT8 CurrMode = IP_PREVIEW_MODE;
    UINT8 NextMode = IP_PREVIEW_MODE;
    UINT32 ChNo = 0;
    AMBA_3A_STATUS_s VideoStatus = {0};
    AMBA_3A_STATUS_s StillStatus = {0};

    AmbaImg_Proc_Cmd(MW_IP_SET_MODE, (UINT32)&CurrMode, (UINT32)&NextMode, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, ChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);

    VideoStatus.Ae = AMBA_IDLE;
    StillStatus.Ae = AMBA_IDLE;
    AmbaImg_Proc_Cmd(MW_IP_SET_AE_STATUS, ChNo, (UINT32)VideoStatus.Ae, (UINT32)StillStatus.Ae);

    VideoStatus.Awb = AMBA_IDLE;
    StillStatus.Awb = AMBA_IDLE;
    AmbaImg_Proc_Cmd(MW_IP_SET_AWB_STATUS, ChNo, (UINT32)VideoStatus.Awb, (UINT32)StillStatus.Awb);

    return OK;
}

static int AmpUT_StillTuning_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *NumFrames = info;

    //AmbaPrint("AmpUT_StillEnc_FifoCB on Event: 0x%x 0x%x", event, *NumFrames);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for(i=0; i<*NumFrames; i++) {
            AmbaKAL_SemGive(&StillTuningSem);
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            AmbaKAL_SemGive(&StillTuningSem);
    }

    return 0;
}

void AmpUT_StillTuning_MuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc = NULL;
    UINT8 *BitsLimit = NULL;
    static UINT32 StillTuningPrevSegNum = 0;
    static UINT8 StillTuningJpegGroupCnt = 0;

    AmbaPrint("AmpUT_StillEnc_MuxTask Start");

    while (1) {
        int Er = 0;
        AmbaKAL_SemTake(&StillTuningSem, AMBA_KAL_WAIT_FOREVER);
        Er = AmpFifo_PeekEntry(StillTuningVirtualFifoHdlr, &Desc, 0);
        if (0 == Er) {
            AmbaPrint("Muxer PeekEntry: size:%5d@0x%08X Ft(%d) Seg(%u)", Desc->Size, Desc->StartAddr, Desc->Type, Desc->SeqNum);
        } else {
            while (0 != Er) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(StillTuningVirtualFifoHdlr, &Desc, 0);
            }
        }

        if (AMP_FIFO_MARK_EOS == Desc->Size) {
            AmbaPrint("Muxer met EOS");
        } else {
            char FileName[MAX_FILENAME_LENGTH];
            BitsLimit = (UINT8 *)StillTuningBitsBuf + STILL_TUNING_BISFIFO_SIZE - 1;
            if (StillTuningPrevSegNum != Desc->SeqNum) {
                StillTuningPicCnt++;
                StillTuningPrevSegNum = Desc->SeqNum;
            }

            if (AMP_FIFO_TYPE_JPEG_FRAME == Desc->Type) {
                Ituner_Ext_File_Param_s Ext_File_Param = {0};
                sprintf(FileName,"%s:\\%04u_m.jpg", DefaultSlot, StillTuningPicCnt);
                if (1/*Ituner need jpg*/) {
                    if (Desc->StartAddr + Desc->Size - 1 <= BitsLimit) {
                        memset(&Ext_File_Param, 0x0, sizeof(Ituner_Ext_File_Param_s));
                        Ext_File_Param.JPG_Save_Param.Address = Desc->StartAddr;
                        Ext_File_Param.JPG_Save_Param.Size = Desc->Size;
                        Ext_File_Param.JPG_Save_Param.Target_File_Path = FileName;
                        AmbaTUNE_Save_Data(EXT_FILE_JPG, &Ext_File_Param);
                    } else {
                        AmbaPrintColor(RED,"Bitsteam over-write");
                    }
                }
            } else if (AMP_FIFO_TYPE_THUMBNAIL_FRAME == Desc->Type) {
                sprintf(FileName, "%s:\\%04u_t.jpg", DefaultSlot, StillTuningPicCnt);
            } else if (AMP_FIFO_TYPE_SCREENNAIL_FRAME == Desc->Type) {
                sprintf(FileName, "%s:\\%04u_s.jpg", DefaultSlot, StillTuningPicCnt);
            }

            if (!(StillTuningDumpSkip & STILL_TUNING_SKIP_JPG)) {
                char MdASCII[3] = {'w','+','\0'};
                StillTuningOutputFile = UT_StillTunningEncodefopen((const char *)FileName, (const char *)MdASCII);
                AmbaPrint("Write: 0x%x sz %d limit %X", Desc->StartAddr, Desc->Size, BitsLimit);
                if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                    UT_StillTunningEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)StillTuningOutputFile);
                }else{
                    UT_StillTunningEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)StillTuningOutputFile);
                    UT_StillTunningEncodefwrite((const void *)BitsLimit, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)StillTuningOutputFile);
                }
                UT_StillTunningEncodefsync((void *)StillTuningOutputFile);
                UT_StillTunningEncodefclose((void *)StillTuningOutputFile);
            }
            StillTuningJpegGroupCnt++;
            StillTuningJpegGroupCnt %= 3;
            if (StillTuningJpegGroupCnt == 0) {
                if (StillTuningOutputFlag & 0x2) {
                    AmbaKAL_EventFlagGive(&StillTuningUTTaskFlag, STILL_TUNING_JPEG_DATA_READY);
                }
            }
            AmbaKAL_TaskSleep(1);
            AmpFifo_RemoveEntry(StillTuningVirtualFifoHdlr, 1);
        }
    }
}

int AmpUT_StillTuningTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_StillTuningTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        if (argc < 3) {
            UINT8 i = 0;
            AmbaPrint("Usage: t stilltuning init [sensorID][LcdID]");
            AmbaPrint("               sensorID:");
            for (i = 0; i<INPUT_DEVICE_NUM; i++) {
                AmbaPrint("                        %2d -- %s", i, MWUT_GetInputDeviceName(i));
            }
            AmbaPrint("               LcdID: 0 -- WDF9648W");
            return -1;
        } else {
            int SensorID = atoi(argv[2]);
            int LcdID = 0;
            if (argc == 4) {
                LcdID = atoi(argv[3]);
            }
            AmpUT_StillTuning_Init(SensorID, LcdID);
            Status = TUNING_STATUS_STILL_INIT;
            AmbaPrintColor(BLUE, "[AMP_UT] Init done %d", SensorID);
        }
    } else if (strcmp(argv[1],"mode") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stilltuning mode [tuningMode]");
            AmbaPrint("             tuningMode: tuning mode");
            AmbaPrint("              0x0: raw capture");
            AmbaPrint("              0x1: raw encode");
            return -1;
        } else {
            TuningMode = (STILL_TUNING_MODE_e)atoi(argv[2]);
            AmbaPrint("[StillTuning] TuningMode %d", TuningMode);
        }
    } else if (strcmp(argv[1],"lvproc") == 0 || strcmp(argv[1],"liveviewproc") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stilltuning liveviewproc [proc][Algo/OS]");
            AmbaPrint("             proc: process mode");
            AmbaPrint("                   0 : Express Mode");
            AmbaPrint("                   1 : Hybrid Mode");
            AmbaPrint("             Algo: Algo mode in HybridMode");
            AmbaPrint("                   0 : Fast");
            AmbaPrint("                   1 : Liso");
            AmbaPrint("                   2 : Hiso");
            AmbaPrint("             OS: OverSampling mode in ExpressMode");
            AmbaPrint("                   0 : Disable");
            AmbaPrint("                   1 : Enable");
            return -1;
        } else {
            UINT8 Proc = atoi(argv[2]);
            UINT8 Aux = atoi(argv[3]);
            StillTuningLiveViewProcMode = Proc;
            if (Proc == 0) {
                StillTuningLiveViewOSMode = Aux;
                StillTuningLiveViewAlgoMode = 0;
            } else {
                StillTuningLiveViewAlgoMode = Aux;
                StillTuningLiveViewOSMode = 0;
            }

            if (StillTuningLiveViewProcMode == 0) {
                AmbaPrint("Set LvProc = %s_%s", "Express", Aux?"OS":"Basic");
            } else {
                AmbaPrint("Set LvProc = %s_%s", "Hybrid",
                    (Aux==0)? "Fast": ((Aux==1)? "LISO": "HISO"));
            }
        }
    } else if (strcmp(argv[1],"lvst") == 0 || strcmp(argv[1],"liveviewstart") == 0) {
        if (Status != TUNING_STATUS_STILL_INIT)
            return -1;

        if (argc < 3) {
            AmbaPrint("Usage: t stilltuning liveviewstart [encID][mainViewID]");
            AmbaPrint("             encID: encode ID, IMX122(B5) only have accept encID = 1 now");
            AmbaPrint("        mainviewID: main view ID, only applicable in IMX122(B5)");
            return -1;
        } else {
            UINT32 SensorModeId = atoi(argv[2]);
            if (argc < 4) StillTuningMultiChanMainViewID = 0;
            else {
                StillTuningMultiChanMainViewID = atoi(argv[3]);
                //should go MultiChan raw-capture
                TuningMode = STILL_TUNING_RAWCAP;
            }

            if (StillTuningMgt[SensorModeId].ForbidMode && (StillTuningMultiChan == 0)) {
                AmbaPrintColor(RED, "Not Support this Mode (%u)", SensorModeId);
                return -1;
            }

            AmpUT_StillTuning_LiveviewStart(SensorModeId);
            Status = TUNING_STATUS_STILL_LIVEVIEW;
            AmbaPrintColor(BLUE, "[AMP_UT] LVST %d", SensorModeId);
        }
    } else if (strcmp(argv[1],"lvsp") == 0 || strcmp(argv[1],"liveviewstop") == 0) {
        if (Status != TUNING_STATUS_STILL_LIVEVIEW)
            return -1;
        AmpUT_StillTuning_LiveviewStop();
        Status = TUNING_STATUS_STILL_INIT;
        AmbaPrintColor(BLUE, "[AMP_UT] LVSP");
    } else if (strcmp(argv[1],"chg") == 0 || strcmp(argv[1],"modechange") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc modechange [encID][mainViewID]");
            AmbaPrint("             encID: encode ID, IMX122(B5) only have accept encID = 1 now");
            AmbaPrint("        mainviewID: main view ID, only applicable in IMX122(B5)");
            return -1;
        } else {
            UINT32 NewSensorModeID = atoi(argv[2]);
            if (argc < 4) StillTuningMultiChanMainViewID = 0;
            else StillTuningMultiChanMainViewID = atoi(argv[3]);
            AmpUT_StillTuning_ChangeResolution(NewSensorModeID);
            AmbaPrintColor(BLUE,"[AMP_UT] Mode change done %d", NewSensorModeID);
        }
    } else if ((strcmp(argv[1],"3a") == 0)) {
        UINT8 Is3aEnable = atoi(argv[2]);
        UINT8 i;
        UINT32 ChannelCount = 0;
        AMBA_3A_OP_INFO_s AaaOpInfo = {0};

        AmbaImg_Proc_Cmd(MW_IP_GET_TOTAL_CH_COUNT, (UINT32)&ChannelCount,0, 0);

        if (Is3aEnable == 0) {
            StillTuning3AEnable = 0;
            AaaOpInfo.AeOp = DISABLE;
            AaaOpInfo.AfOp = DISABLE;
            AaaOpInfo.AwbOp = DISABLE;
            AaaOpInfo.AdjOp = DISABLE;
        } else if (Is3aEnable == 1) {
            StillTuning3AEnable = 1;
            AaaOpInfo.AeOp = ENABLE;
            AaaOpInfo.AfOp = ENABLE;
            AaaOpInfo.AwbOp = ENABLE;
            AaaOpInfo.AdjOp = ENABLE;
        }

        for (i=0; i<ChannelCount; i++) {
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, i, (UINT32)&AaaOpInfo, 0);
        }

        for (i=0; i <ChannelCount; i++) {
            AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, i, (UINT32)&AaaOpInfo, 0);
            AmbaPrint("ChNo[%u] 3A %s: ae:%u af:%u awb:%u adj:%u", i, StillTuning3AEnable? "Enable": "Disable", \
                AaaOpInfo.AeOp, AaaOpInfo.AfOp, AaaOpInfo.AwbOp, AaaOpInfo.AdjOp);
        }
    } else if ((strcmp(argv[1],"timeout") == 0)) {
        UINT32 timeout = atoi(argv[2]);
        AmbaPrint("StillTuning set new timeout value %d -> %d ", StillTuningTimeout, timeout);
        StillTuningTimeout = timeout;
    } else if (strcmp(argv[1], "debug") == 0) {
    #ifdef CONFIG_SOC_A9
        // TH_0 ~ TH_7
        extern int AmbaDSP_SetDebugThread(UINT32 ThreadMask);
        UINT32 thrdMask = atoi(argv[2]);
        AmbaDSP_SetDebugThread(thrdMask);
    #else
        extern int AmbaDSP_CmdSetDebugLevel(UINT8 Module, UINT8 Level, UINT8 Mask);
        UINT8 Module = atoi(argv[2]);
        UINT8 Level = atoi(argv[3]);
        UINT8 Mask = atoi(argv[4]);
        AmbaDSP_CmdSetDebugLevel(Module, Level, Mask);
    #endif
    } else if (strcmp(argv[1],"debuglvl") == 0) {
    #ifdef CONFIG_SOC_A9
        // module ??
        // Mask 0~0xFF
        // addorset: set = 0
        extern int AmbaDSP_SetDebugLevel(UINT8 Module, UINT32 DebugMask, UINT8 AddOrSet);
        UINT32 mod = atoi(argv[2]);
        UINT32 msk = atoi(argv[3]);
        UINT32 add = atoi(argv[4]);
        AmbaDSP_SetDebugLevel(mod, msk, add);
        #endif
    } else if (strcmp(argv[1],"del") == 0) {
        AmpStillEnc_Delete(StillTuningPri);
        StillTuningPri = NULL;
    } else if (strcmp(argv[1],"rotate") == 0 || strcmp(argv[1],"rot") == 0) {
        if (argc != 3) {
            AmbaPrint("Usage: t stilltuning rotation [type] ");
            AmbaPrint("              type: Predefined rotation value");
            AmbaPrint("                    0 - No rotation ");
            AmbaPrint("                    1 - Horizontal flip");
            AmbaPrint("                    2 - 90 clockwise");
            AmbaPrint("                    3 - 90 clockwise then verical flip");
            AmbaPrint("                    4 - 180 clockwise");
            AmbaPrint("                    5 - 180 clockwise then horizontal flip");
            AmbaPrint("                    6 - 270 clockwise");
            AmbaPrint("                    7 - 270 clockwise then vertical flip");
            AmbaPrint(" Temp only support Type_5 via sensor VFlip reg");
            return -1;
        }
        StillTuningRotate = (AMP_ROTATION_e)atoi(argv[2]);
        if (StillTuningRotate == AMP_ROTATE_180_HORZ_FLIP || StillTuningRotate == AMP_ROTATE_0) {
            AmbaPrint("Rotation %d ", StillTuningRotate);
        } else {
            AmbaPrint("Rotation type Not Support");
            StillTuningRotate = AMP_ROTATE_0;
        }
    } else if (strcmp(argv[1],"system") == 0) {
        UINT8 v1 = atoi(argv[2]);

        AmbaPrint("Encode System is %s", v1?"PAL": "NTSC");
        StillTuningEncodeSystem = v1;
    } else if (strcmp(argv[1],"perform") == 0) {
        UINT16 Idsp = atoi(argv[2]);
        UINT16 Core = atoi(argv[3]);
        UINT16 Cortex = atoi(argv[4]);
        AmbaPrint("Idsp %dMhz, Core %dMhz, Cortex %dMhz", Idsp, Core, Cortex);
        CustomIdspFreq = Idsp;
        CustomCoreFreq = Core;
        CustomCortexFreq = Cortex;
    } else if (strcmp(argv[1],"maxperform") == 0) {
        UINT16 MaxIdsp = atoi(argv[2]);
        UINT16 MaxCore = atoi(argv[3]);
        UINT16 MaxCortex = atoi(argv[4]);
        AmbaPrint("MaxIdsp %dMhz, MaxCore %dMhz, MaxCortex %dMhz", MaxIdsp, MaxCore, MaxCortex);
        CustomMaxIdspFreq = MaxIdsp;
        CustomMaxCoreFreq = MaxCore;
        CustomMaxCortexFreq = MaxCortex;
    } else if (strcmp(argv[1],"rawtest") == 0) {
        extern int AmpUT_ItunerRawCapture(UINT8 OutputFlag, AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s ItunerRawCapCtrl);
        extern UINT32 CalibRawPP(AMP_STILLENC_POSTP_INFO_s *PostpInfo);
        static AMP_STILLENC_POSTP_s pp_RawReady_cb = {.Process = CalibRawPP};
        UINT8 flag = 0x1;
        AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s ItunerRawCapCtrl = {
            .RawBufSource = 0,
            .ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING,
            .SensorMode = {
                .Bits = {
                    .Mode = 0, //TBD
                    .VerticalFlip = 1,
                }
            },
        };
        ItunerRawCapCtrl.RawBuff.PostProc = &pp_RawReady_cb;

        AmpUT_ItunerRawCapture(flag, ItunerRawCapCtrl);
    } else if (strcmp(argv[1],"rawenc") == 0) {
        TUNE_Initial_Config_s InitialConfig;
        TUNE_Load_Param_s LoadParam;
        memset(&InitialConfig, 0x0, sizeof(TUNE_Initial_Config_s));
        InitialConfig.Text.pBytePool = &G_MMPL;
        AmbaTUNE_Change_Parser_Mode(TEXT_TUNE);
        if (0 != AmbaTUNE_Init(&InitialConfig)) {
           AmbaPrintColor(RED, "Call AmbaTUNE_Init() Fail");
        }

        memset(&LoadParam, 0x0, sizeof(TUNE_Load_Param_s));
        LoadParam.Text.FilePath = argv[2];
        if (0 != AmbaTUNE_Load_IDSP(&LoadParam)) {
            AmbaPrintColor(RED, "Call AmbaTUNE_Load_IDSP(%s) Fail", argv[2]);
        }
        {
            AMBA_DSP_RAW_BUF_s RawBuf = {0};
            Ituner_Ext_File_Param_s Ituner_Ext_File_Param = {0};
            AmpUT_StillTuningGetRawEncodeBuffer(&RawBuf);
            Ituner_Ext_File_Param.Raw_Load_Param.Address = (void *)RawBuf.pBaseAddr;
            Ituner_Ext_File_Param.Raw_Load_Param.Max_Size = (UINT32)RawBuf.Pitch*RawBuf.Window.Height;
            Ituner_Ext_File_Param.Raw_Load_Param.InputPitch = RawBuf.Pitch;
            AmbaTUNE_Load_Data(EXT_FILE_RAW, &Ituner_Ext_File_Param);
            AmbaCache_Clean(Ituner_Ext_File_Param.Raw_Load_Param.Address, Ituner_Ext_File_Param.Raw_Load_Param.Max_Size);
            AmpUt_StillTuningPreLoadDone();
            AmpUT_ItunerRawEncode();
        }
    } else if (strcmp(argv[1],"suspend") == 0) {
        DspSuspendEnable = atoi(argv[2]);
        AmbaPrint("%s Dsp suspend in LiveveiwStop !", DspSuspendEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"wirelessmode") == 0) {
        WirelessModeEnable = atoi(argv[2]);
        AmbaPrint("%s Enter Wireless mode in LiveveiwStop !", WirelessModeEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"dumpskip") == 0) {
        StillTuningDumpSkip = atoi(argv[2]);
        AmbaPrint("DumpSkipFlag is 0x%X", StillTuningDumpSkip);
    } else if (strcmp(argv[1],"dumpituner") == 0) {
        StillTuningDumpItuner = atoi(argv[2])? 1: 0;
        AmbaPrint("Dump ituner %u", StillTuningDumpItuner);
    } else if (strcmp(argv[1],"needjpeg") == 0) {
        StillRawEncodeNeedJpegOutput = atoi(argv[2]);
        AmbaPrint("Still raw encode need jpeg %u", StillRawEncodeNeedJpegOutput);
    } else {
        AmbaPrint("Usage: t stilltuning [cmd]");
        AmbaPrint("       init: init codec");
        AmbaPrint("       mode: tuning mode");
        AmbaPrint("       liveviewstart: start Liveview");
        AmbaPrint("       liveviewstop: stop Liveview");
        AmbaPrint("       modechange: change LiveView mode");
    }
    return 0;
}

int AmpUT_StillTuningTestAdd(void)
{
    AmbaPrint("Adding AmpUT_StillTuning");
    // hook command
    AmbaTest_RegisterCommand("st", AmpUT_StillTuningTest);
    AmbaTest_RegisterCommand("stilltuning", AmpUT_StillTuningTest);

    if (0) {
        AmpUT_StillTuning_Init(0, 0);
        Status = TUNING_STATUS_STILL_INIT;
    }
    if (0) {
        AmpUT_StillTuning_LiveviewStart(0);
    }

    return AMP_OK;
}
