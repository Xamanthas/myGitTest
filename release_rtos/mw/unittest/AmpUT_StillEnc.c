 /**
  * @file src/app/sample/unittest/AmpUT_StillEnc.c
  *
  * still encode unit test
  *
  * History:
  *    2013/05/27 - [Edgar Lai] created file
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
#include "AmbaDSP_StillCapture.h"
#include <scheduler.h>
#include "AmbaCache.h"
#include <img/ImgDzoom.h>
#include "AmbaTUNE_HdlrManager.h"
#include <util.h>
#include <cfs/AmpCfs.h>
#include <AmbaSensor.h>
#include <AmbaCalibInfo.h>
#include "AmbaLCD.h"
#include "AmpUT_Display.h"

#include <AmbaImg_Proc.h>
#include <AmbaImg_Impl_Cmd.h>
#include <AmbaImg_VIn_Handler.h>
#include <AmbaImg_VDsp_Handler.h>
#include <AmbaImg_Adjustment_Def.h>
#ifdef CONFIG_SOC_A9
#include <AmbaImg_AeAwb.h>
#else
#include <AmbaImg_Adjustment_A12.h>
#endif
#include <AmbaImg_AaaDef.h>
#include "extSrc/sample_aaa/AmbaSample_AeAwbAdj.h"
#include <AmbaImg_Exif.h>

#include <AmbaCalib_Utility.h>
#include "extSrc/sample_calib/AmbaUTCalibMgr.h"

#ifdef CONFIG_SOC_A12
#include "AmbaUTCalibMgr.h"
#endif

static char DefaultSlot[] = "C";


void *UT_StillEncodefopen(const char *pFileName, const char *pMode)
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

int UT_StillEncodefclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_StillEncodefread(void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fread(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_StillEncodefwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_StillEncodefsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

UINT32 UT_StillFrameRateIntConvert(UINT32 OriFrameRate)
{
    UINT32 FrameRateInt = OriFrameRate;

    if (OriFrameRate==29 || OriFrameRate==59 || OriFrameRate==119 || OriFrameRate==239) {
        FrameRateInt++;
    }

    return FrameRateInt;
}


#define _STILL_BUFFER_FROM_DSP_

//static AMBA_DSP_EVENT_CFA_3A_DATA_s cfa3a;
//static AMBA_DSP_EVENT_RGB_3A_DATA_s rgb3a;

// global var for StillEnc
/* still codec flags */
static UINT8 StillCodecInit = 0;
static UINT8 StillEnc3AEnable = 1;
static UINT8 StillRawCaptureRunning = 0;
static UINT8 StillBGProcessing = 0;
static UINT8 AutoBackToLiveview = 0;
static UINT8 TVLiveViewEnable = 1;
static UINT8 StillPivProcess = 0;
static AMP_ROTATION_e StillEncRotate = AMP_ROTATE_0;
static UINT8 StillLiveViewProcMode = 0;             // LiveView Process Mode, 0: Express 1:Hybrid
static UINT8 StillLiveViewAlgoMode = 0;             // LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
static UINT8 StillLiveViewOSMode = 0;               // LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 StillEncodeSystem = 0;                 // Encode System, 0:NTSC, 1: PAL
static UINT8 StillIsScapCTimeLapse = 0;             // Flag to indicate whether it is time-lapse scapc
static UINT32 StillContSCapShotCount = 0;
static UINT32 StillIsCapProfEnable = 0;
static UINT8 StillOBModeEnable = 0;

#define INIT_DZOOM_FACTOR   (1<<16)

static UINT8 StillEncIsIqParamInit = 0;
static UINT8 StillEncIsHdrIqParam = 0;

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
static UINT32 StillEncCusomtDspWorkSize = 0;


#ifdef CONFIG_SOC_A12
static UINT8 IsTurnOnLcdQV = 0;                     // A12 always ask for LCD qv
static UINT8 IsTurnOnHdmiQV = 0;                     // A12 always ask for LCD qv
#endif
/* still codec HDLRs */
static AMP_VIN_HDLR_s *StillEncVinA = NULL;
static AMP_STLENC_HDLR_s *StillEncPri = NULL;
AMP_ENC_PIPE_HDLR_s *StillEncPipe = NULL;
static UINT8 *StillVinWorkBuf = NULL;               // Vin module working buffer
static UINT8 *StillImgSchdlrWorkBuf = NULL;         // Img scheduler working buffer
static UINT8 *StillEncWorkBuf = NULL;               // StillEnc working buffer
static AMBA_DSP_CHANNEL_ID_u EncChannel;
#define STATUS_STILL_IDLE                1
#define STATUS_STILL_INIT                2
#define STATUS_STILL_LIVEVIEW            3
#define STATUS_STILL_RAWCAPTURE          4
#define STATUS_STILL_RAWENCODE           5
static UINT8 Status = STATUS_STILL_IDLE;
#ifdef CONFIG_LCD_WDF9648W
extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;
#endif

// Image scheduler
AMBA_IMG_SCHDLR_HDLR_s *SencImgSchdlr = NULL;

/* Single capture buffers */
static UINT8 *ScriptAddr = NULL;
static UINT8 *RawBuffAddr = NULL;
static UINT8 *YuvBuffAddr = NULL;
static UINT8 *ScrnBuffAddr = NULL;
static UINT8 *ThmBuffAddr = NULL;
static UINT8 *QvLCDBuffAddr = NULL;
static UINT8 *QvHDMIBuffAddr = NULL;
static UINT8 *DstRawBuffAddr = NULL;
static UINT8 *Raw3ARoiBuffAddr = NULL;
static UINT8 *Raw3AStatBuffAddr = NULL;
static UINT8 *OriScriptAddr = NULL;
static UINT8 *OriRawBuffAddr = NULL;
static UINT8 *OriScrnBuffAddr = NULL;
static UINT8 *OriThmBuffAddr = NULL;
#ifndef _STILL_BUFFER_FROM_DSP_
static UINT8 *OriYuvBuffAddr = NULL;
static UINT8 *OriQvLCDBuffAddr = NULL;
static UINT8 *OriQvHDMIBuffAddr = NULL;
#endif
static UINT8 *OriDstRawBuffAddr = NULL;
static UINT8 *OriRaw3ARoiBuffAddr = NULL;
static UINT8 *OriRaw3AStatBuffAddr = NULL;


typedef enum _STILL_CAPTURE_MODE_e_ {
    STILL_CAPTURE_NONE,
    STILL_CAPTURE_RAW2RAW,
    STILL_CAPTURE_RAW2YUV,
    STILL_CAPTURE_YUV2JPG,
    STILL_CAPTURE_SINGLE_SHOT,
    STILL_CAPTURE_SINGLE_SHOT_CONT,
    STILL_CAPTURE_BURST_SHOT,
    STILL_CAPTURE_BURST_SHOT_CONT,
    STILL_CAPTURE_BURST_SHOT_CONT_WB,
    STILL_CAPTURE_AEB,
    STILL_CAPTURE_PRE_CAPTURE,
    STILL_CAPTURE_UNKNOWN = 0xFF
} STILL_CAPTURE_MODE_e;
static STILL_CAPTURE_MODE_e CaptureMode = STILL_CAPTURE_NONE;
static AMP_RAW_CAPTURE_PARAM_s ScapCont;

#ifdef CONFIG_SOC_A12
static UINT32 Force3PassLiso = 0; // MISO
#endif
static UINT32 G_iso = 2;
static UINT16 G_rawCapCnt = 0;
static UINT16 G_capcnt = 1;
static UINT8 G_raw_cmpr = 0;
static UINT8 G_pivRaw2YuvRestoreWarp = 0;
#ifdef CONFIG_SOC_A12
static UINT8 QvDisplayCfg = 1;
static UINT8 QvLcdEnable = 1;
#else
static UINT8 QvDisplayCfg = 0;
static UINT8 QvLcdEnable = 0;
#endif
static UINT8 QvTvEnable = 0;
static UINT8 QvLcdShowBuffIndex = 0;
static UINT8 QvTvShowBuffIndex = 0;
static UINT8 *QvLcdShowBuffAddrOdd = NULL;
static UINT8 *QvLcdShowBuffAddrEven = NULL;
static UINT8 *QvTvShowBuffAddrOdd = NULL;
static UINT8 *QvTvShowBuffAddrEven = NULL;
static UINT8 StillEncDumpItuner = 0;   // Dump ituner text file or not
static UINT8 StillDumpSkip = 0;     // Encode Skip dump file, write data to SD card or just lies to fifo that it has muxed pending data
#define STILL_ENC_SKIP_JPG   (0x1)
#define STILL_ENC_SKIP_YUV   (0x1<<1)
#define STILL_ENC_SKIP_RAW   (0x1<<2)
#define STILL_ENC_SKIP_CFA   (0x1<<3)

#define DEFAULT_STAMP_MAIN_WIDTH (512)
#define DEFAULT_STAMP_MAIN_HEIGHT (200)
#define DEFAULT_STAMP_SCRN_WIDTH (256)
#define DEFAULT_STAMP_SCRN_HEIGHT (100)
#define DEFAULT_STAMP_THM_WIDTH (64)
#define DEFAULT_STAMP_THM_HEIGHT (50)
static UINT8 IsStillStampEnable = 0;
static UINT8 StillStampMainY[DEFAULT_STAMP_MAIN_WIDTH*DEFAULT_STAMP_MAIN_HEIGHT];  // Main Y buffer for stamp test
static UINT8 StillStampMainUV[DEFAULT_STAMP_MAIN_WIDTH*DEFAULT_STAMP_MAIN_HEIGHT]; // Main UV buffer for stamp test
static UINT8 StillStampScrnY[DEFAULT_STAMP_SCRN_WIDTH*DEFAULT_STAMP_SCRN_HEIGHT];  // Scrn Y buffer for stamp test
static UINT8 StillStampScrnUV[DEFAULT_STAMP_SCRN_WIDTH*DEFAULT_STAMP_SCRN_HEIGHT]; // Scrn UV buffer for stamp test
static UINT8 StillStampThmY[DEFAULT_STAMP_THM_WIDTH*DEFAULT_STAMP_THM_HEIGHT];     // Thm Y buffer for stamp test
static UINT8 StillStampThmUV[DEFAULT_STAMP_THM_WIDTH*DEFAULT_STAMP_THM_HEIGHT];    // Thm UV buffer for stamp test
static UINT16 StillCustomMainWidth = 0;
static UINT16 StillCustomMainHeight = 0;

/* Default unittest settings */
#define DEFAULT_STILL_ISO_MODE (1)

#define DEFAULT_STILL_RAW_COMPRESSED_MODE (1)
#define DEFAULT_STILL_JPEG_TARGET_SIZE (0)
#define DEFAULT_STILL_JPEG_ENCODE_LOOP (0)

/* QuickView show Time Table */
#define MAX_QV_SHOW_TIME_ENTRY  (4)
static int QvShowTimeTable[MAX_QV_SHOW_TIME_ENTRY] = {
    1000,       /* 1 second */
    3000,       /* 3 second */
    5000,       /* 5 second */
    0xFFFFFFFF, /* Forever */
};
static AMBA_KAL_TIMER_t QvTimerHdlr;
static AMBA_KAL_TIMER_t ScapCTimerHdlr;
static AMBA_KAL_TIMER_t CaptureButtonTimerHdlr;
static AMBA_KAL_TIMER_t ForceStopTimerHdlr;
static int QvTimerHdlrID = -1;
static int ScapCTimerHdlrID = -1;
static int CaptureButtonTimerHdlrID = -1;
static int ForceStopTimerHdlrID = -1;
static int CaptureButtonPressed = 0;
static UINT32 QvTimerID = 0;
typedef enum _AMPUT_STILL_VOUT_SRC_e_ {
    VOUT_SRC_DEFAULT_IMG = 0,    /* Default Image */
    VOUT_SRC_BACK_COLOR,         /* Mixer Background Color */
    VOUT_SRC_VIDEO_CAPTURE,      /* Video Plane */
} AMPUT_STILL_VOUT_SRC_e;
#if 0 //20131113 Chester: Since nobody refer to VoutLcdSrc and VoutTvSrc,
      // temp remove to avoid compile warning message
static AMP_DISP_WINDOW_SRC_e VoutLcdSrc = AMP_DISP_ENC;
static AMP_DISP_WINDOW_SRC_e VoutTvSrc = AMP_DISP_ENC;
#endif

static AMP_IMG_DZOOM_TABLE_s StillDzoomTable;    // Digital zoom table
static AMP_IMG_DZOOM_HDLR_s *StillDzoomHdlr;     // Digital zoom instance
#define STILL_DZOOM_10X_TOTAL_STEP    (487)       /** 414+73 */
#define STILL_DZOOM_10X_MAX_RATIO     (654542)    /** 9.987518<<16 */
#define STILL_ZF_1X    (0x10000)

#if 1
    const UINT16 TileNumber = 1;
#else
    const UINT16 TileNumber = 3;
#endif

/* multi capture buffers */
//static UINT8 *m_rawBuffaddr = NULL;
//static UINT8 *m_yuvBuffaddr = NULL;

#define AMPUT_STILLENC_MAX_ENC_ID   3

static INPUT_ENC_MGT_s *StillEncMgt = NULL;
static INT8 StillSensorIdx = 0;
static UINT8 StillEncModeIdx = 0;

#define AMPUT_STILLENC_MAX_VOUT_ID   2
static struct {
    UINT16 OffsetX;
    UINT16 OffsetY;
    UINT16 Width;
    UINT16 Height;
} StillVoutMgt[AMPUT_STILLENC_MAX_VOUT_ID][AMPUT_STILLENC_MAX_ENC_ID] = {
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

/*
 * JPEG QTable
 */
static UINT8 StdJpegQTable[128] = {
    0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E,
    0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28,
    0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25,
    0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33,
    0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44,
    0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57,
    0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71,
    0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63,
    0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A,
    0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63
};

//#pragma data_alignment = 32
UINT8 AmpUTJpegQTable[3][128] = {
    [0] = { //Main
        0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E,
        0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28,
        0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25,
        0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33,
        0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44,
        0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57,
        0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71,
        0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63,
        0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A,
        0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63
    },
    [1] = { //Thm
        0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E,
        0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28,
        0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25,
        0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33,
        0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44,
        0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57,
        0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71,
        0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63,
        0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A,
        0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63
    },
    [2] = { //Scrn
        0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E,
        0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28,
        0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25,
        0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33,
        0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44,
        0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57,
        0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71,
        0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63,
        0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A,
        0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
        0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63
    }
};

#ifdef CONFIG_SOC_A9
// Window handler
static AMP_DISP_WINDOW_HDLR_s *EncLcdWinHdlr = NULL;
static AMP_DISP_WINDOW_HDLR_s *EncTvWinHdlr = NULL;
#endif

/* muxer */
#define STILL_BISFIFO_SIZE 16*1024*1024
static UINT8 *BitsBuf;
#define STILL_DESC_SIZE 32*128
static UINT8 *DescBuf;
static AMP_FIFO_HDLR_s *StillEncVirtualFifoHdlr = NULL;
static AMBA_KAL_TASK_t StillEncMuxTask = {0};
#define STILL_ENC_MUX_TASK_STACK_SIZE   (8192)
static AMBA_KAL_SEM_t StillEncSem = {0};

static AMBA_FS_FILE *OutputFile = NULL;

static char *AmpUT_StillEncMuxStack;

/* Simple still_ut */
static AMBA_KAL_TASK_t StillEncUtTask = {0};
#define STILL_ENC_MUX_UT_STACK_SIZE (0x4000)
static char *AmpUT_StillEncUtStack;
static AMBA_KAL_MSG_QUEUE_t UtMsgQueue;
#define MAX_STILL_UT_MSG_NUM  64
typedef struct _STILLENC_UT_MSG_S_ {
    UINT32 Event;
    union {
        UINT32 DataInfo[2];
        AMP_ENC_YUV_INFO_s yuvInfo;
    } Data;
} STILLENC_UT_MSG_S;
static STILLENC_UT_MSG_S *AmpUT_StillEncUtMsgQPool = NULL;

//define UT message
#define MSG_STILLENC_UT_QV_TIMER_TIMEOUT            (0x0000001)
#define MSG_STILLENC_UT_QV_LCD_READY                (0x0000002)
#define MSG_STILLENC_UT_QV_TV_READY                 (0x0000003)
#define MSG_STILLENC_UT_RAW_CAPTURE_DONE            (0x0000004)
#define MSG_STILLENC_UT_SCAPC_TIMER_TRIGGER_START   (0x0000005)
#define MSG_STILLENC_UT_BG_PROCESS_DONE             (0x0000006)
#define MSG_STILLENC_UT_RAW_CAPTURE_START           (0x0000007)

static UINT16 RawFno = 1;
static UINT16 YuvFno = 1;
static UINT16 JpgFno = 1;
static UINT16 CfaFno = 1;

extern UINT32 AmpUT_JpegBRCPredictCB(UINT16 targetBpp, UINT32 stillProc, UINT8* predictQ, UINT8 *curveNum, UINT32 *curveAddr);


/*  Function prototype*/
int AmpUT_StillEnc_Init(int sensorID, int LcdID);
int AmpUT_StillEnc_ChangeResolution(UINT32 encID);
void AmpUT_StillEnc_MuxTask(UINT32 info);
UINT32 AmpUT_StillEncControlStillAdj(UINT32 aeIdx);
UINT32 AmpUT_StillEncIdspParamSetup(UINT8 aeIdx, UINT16 rawWidth, UINT16 rawHeight);
UINT32 AmpUT_StillEncPostIdspParamSetup(UINT8 aeIdx);
UINT32 AmpUT_SensorPrep(AMBA_DSP_CHANNEL_ID_u vinChan, UINT32 exposureIdx);
UINT32 AmpUT_StillEncRawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl);
UINT32 AmpUT_StillEncMultiRawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl);
UINT32 AmpUT_StillEncRaw2RawIdspCfgCB(UINT16 index);
UINT32 AmpUT_StillEncAAALock(void);
UINT32 AmpUT_StillEncAAAUnlock(void);
UINT32 AmpUT_StillEncSetShotCount(UINT32 shotCount);
UINT32 AmpUT_StillEncPostWBCalculation(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s *cfaStat);
UINT32 AmpUT_StillEncAttachOBScript(AMP_SENC_SCRPT_RAW2RAW_s *raw2rawScript, UINT8 encID, UINT8 cmpr);
UINT32 AmpUT_StillEncSetJpegBrc(AMP_SENC_SCRPT_GENCFG_s *genScript, UINT32 targetSize, UINT8 encodeLp);

UINT32 AmpUT_RawCapPreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_Raw2RawPreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_PivRaw2YuvPreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_SingleCapturePreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_SingleCaptureContPreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_BurstCapturePreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_BurstCaptureContPreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_BurstCaptureContWBPreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_AEBPreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_PBCapturePreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_PreCapturePreCB(AMP_STILLENC_PREP_INFO_s *info);

UINT32 AmpUT_RawCapPostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_Raw2RawPostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_PivRaw2YuvPostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_SingleCapturePostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_SingleCaptureContPostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_BurstCapturePostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_BurstCaptureContPostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_BurstCaptureContWBPostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_AEBPostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_PBCapturePostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_PreCapturePostCB(AMP_STILLENC_POSTP_INFO_s *info);

static int AmpUT_StillEncCallback(void *hdlr,UINT32 event, void *info);
static int AmpUT_StillEnc_FifoCB(void *hdlr, UINT32 event, void* info);
static int AmpUT_StillEncPipeCallback(void *hdlr,UINT32 event, void *info);

/* Still codec pre-callback */
static AMP_STILLENC_PREP_s PreRawCapCB = {.Process = AmpUT_RawCapPreCB};
static AMP_STILLENC_PREP_s PreRaw2RawCB = {.Process = AmpUT_Raw2RawPreCB};
static AMP_STILLENC_PREP_s PrePivRaw2YuvCB = {.Process = AmpUT_PivRaw2YuvPreCB};
static AMP_STILLENC_PREP_s PreSingleCapCB = {.Process = AmpUT_SingleCapturePreCB};
static AMP_STILLENC_PREP_s PreSingleCapContCB = {.Process = AmpUT_SingleCaptureContPreCB};
static AMP_STILLENC_PREP_s PreBurstCapCB = {.Process = AmpUT_BurstCapturePreCB};
static AMP_STILLENC_PREP_s PreBurstCapContCB = {.Process = AmpUT_BurstCaptureContPreCB};
static AMP_STILLENC_PREP_s PreBurstCapContWBCB = {.Process = AmpUT_BurstCaptureContWBPreCB};
static AMP_STILLENC_PREP_s PreAEBCB = {.Process = AmpUT_AEBPreCB};
static AMP_STILLENC_PREP_s PrePbCaptureCB = {.Process = AmpUT_PBCapturePreCB};
static AMP_STILLENC_PREP_s PrePreCaptureCB = {.Process = AmpUT_PreCapturePreCB};

/* Still codec post-callback */
static AMP_STILLENC_POSTP_s PostRawCapCB = {.Process = AmpUT_RawCapPostCB};
static AMP_STILLENC_POSTP_s PostRaw2RawCB = {.Process = AmpUT_Raw2RawPostCB};
static AMP_STILLENC_POSTP_s PostPivRaw2YuvCB = {.Process = AmpUT_PivRaw2YuvPostCB};
static AMP_STILLENC_POSTP_s PostSingleCapCB = {.Process = AmpUT_SingleCapturePostCB};
static AMP_STILLENC_POSTP_s PostSingleCapContCB = {.Process = AmpUT_SingleCaptureContPostCB};
static AMP_STILLENC_POSTP_s PostBurstCapCB = {.Process = AmpUT_BurstCapturePostCB};
static AMP_STILLENC_POSTP_s PostBurstCapContCB = {.Process = AmpUT_BurstCaptureContPostCB};
static AMP_STILLENC_POSTP_s PostBurstCapContWBCB = {.Process = AmpUT_BurstCaptureContWBPostCB};
static AMP_STILLENC_POSTP_s PostAEBCB = {.Process = AmpUT_AEBPostCB};
static AMP_STILLENC_POSTP_s PostPbCaptureCB = {.Process = AmpUT_PBCapturePostCB};
static AMP_STILLENC_POSTP_s PostPreCaptureCB = {.Process = AmpUT_PreCapturePostCB};

int AmpUT_StillEncodeDumpItunerFile(char *pItunerFileName, char *pTuningModeExt, char *pRawPath)
{
    UINT32 ImgIpChNo = 0, AeIdx = 0;
    ITUNER_SYSTEM_s ItunerSysInfo = {0};
    AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
    AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
    AMBA_AE_INFO_s StillAeInfo[MAX_AEB_NUM];
    ITUNER_AE_INFO_s ItunerAeInfo = {0};

    snprintf(ItunerSysInfo.ChipRev, sizeof(ItunerSysInfo.ChipRev), "A12");
    snprintf(ItunerSysInfo.TuningModeExt, sizeof(ItunerSysInfo.TuningModeExt), pTuningModeExt);
    memcpy(ItunerSysInfo.RawPath, pRawPath, sizeof(ItunerSysInfo.RawPath));

    AmbaSensor_GetStatus(EncChannel, &SensorStatus);
    ItunerSysInfo.RawBayer = SensorStatus.ModeInfo.OutputInfo.CfaPattern;
    ItunerSysInfo.RawResolution = SensorStatus.ModeInfo.OutputInfo.NumDataBits;
    ItunerSysInfo.HSubSampleFactorDen = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorDen;
    ItunerSysInfo.HSubSampleFactorNum = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorNum;
    ItunerSysInfo.VSubSampleFactorDen = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorDen;
    ItunerSysInfo.VSubSampleFactorNum = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorNum;
    ItunerSysInfo.CompressedRaw = G_raw_cmpr;
    if (StillOBModeEnable == 1) {
        ItunerSysInfo.RawWidth = SensorStatus.ModeInfo.OutputInfo.OutputWidth;
        ItunerSysInfo.RawHeight = SensorStatus.ModeInfo.OutputInfo.OutputHeight;
        ItunerSysInfo.RawStartX = (SensorStatus.ModeInfo.OutputInfo.OutputWidth - ItunerSysInfo.RawWidth) & 0xFFFE;
        ItunerSysInfo.RawStartY = (SensorStatus.ModeInfo.OutputInfo.OutputHeight - ItunerSysInfo.RawHeight) & 0xFFFE;
    } else {
        ItunerSysInfo.RawWidth = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        ItunerSysInfo.RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        ItunerSysInfo.RawStartX = (SensorStatus.ModeInfo.OutputInfo.RecordingPixels.StartX + ((SensorStatus.ModeInfo.OutputInfo.RecordingPixels.Width - ItunerSysInfo.RawWidth)/2)) & 0xFFFE;
        ItunerSysInfo.RawStartY = (SensorStatus.ModeInfo.OutputInfo.RecordingPixels.StartY + ((SensorStatus.ModeInfo.OutputInfo.RecordingPixels.Height - ItunerSysInfo.RawHeight)/2)) & 0xFFFE;
    }
    ItunerSysInfo.RawPitch = ALIGN_32((G_raw_cmpr) ? (AMP_COMPRESSED_RAW_WIDTH(ItunerSysInfo.RawWidth)) : (ItunerSysInfo.RawWidth<<1));
    ItunerSysInfo.MainWidth = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[StillEncModeIdx].StillMainWidth;
    ItunerSysInfo.MainHeight = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[StillEncModeIdx].StillMainHeight;
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
    ImgMode.AlgoMode = (G_iso == 0)? AMBA_DSP_IMG_ALGO_MODE_HISO: ((G_iso == 1)? AMBA_DSP_IMG_ALGO_MODE_LISO: AMBA_DSP_IMG_ALGO_MODE_FAST);
    ImgMode.ConfigId = 0;
    ImgMode.ContextId = 0;

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

int AmpUT_DumpAoi(UINT8 *Addr, UINT16 Pitch, AMP_AREA_s Aoi, void *Fp)
{
    UINT8 *StartAddress = Addr;
    UINT16 i;
    StartAddress += (Aoi.X + Aoi.Y*Pitch);
    for (i=0; i<Aoi.Height; i++) {
        UT_StillEncodefwrite((const void *)StartAddress, 1, Aoi.Width, Fp);
        StartAddress += Pitch;
    }

    return OK;
}

int AmpUT_DrawStillStamp(UINT8 type, UINT8 *yAddr, UINT8 *uvAddr, AMP_AREA_s aoi, UINT16 pitch)
{
    const int OffsetX = 5;
    const int OffsetY = 6;
    int i;
    UINT32 StartX, StartY;
    UINT8 *StartAddrY, *StartAddrUV;

    switch (type) {
    case 0: //Main
    {
        StartX = (aoi.Width>>1) + OffsetX - (DEFAULT_STAMP_MAIN_WIDTH>>1);
        StartY = (aoi.Height>>1) + OffsetY - (DEFAULT_STAMP_MAIN_HEIGHT>>1);
        for (i=0; i<DEFAULT_STAMP_MAIN_HEIGHT; i++) {
            StartAddrY = yAddr + aoi.X + aoi.Y*pitch + (i+StartY)*pitch;
            StartAddrUV = uvAddr + aoi.X + aoi.Y*pitch + (i+StartY)*pitch;
            memcpy(StartAddrY+StartX, &StillStampMainY[i*DEFAULT_STAMP_MAIN_WIDTH], DEFAULT_STAMP_MAIN_WIDTH);
            memcpy(StartAddrUV+StartX, &StillStampMainUV[i*DEFAULT_STAMP_MAIN_WIDTH], DEFAULT_STAMP_MAIN_WIDTH);
        }
    }
        break;
    case 1: // Scrn
    {
        StartX = (aoi.Width>>1) + OffsetX - (DEFAULT_STAMP_SCRN_WIDTH>>1);
        StartY = (aoi.Height>>1) + OffsetY - (DEFAULT_STAMP_SCRN_HEIGHT>>1);
        for (i=0; i<DEFAULT_STAMP_SCRN_HEIGHT; i++) {
            StartAddrY = yAddr + aoi.X + aoi.Y*pitch + (i+StartY)*pitch;
            StartAddrUV = uvAddr + aoi.X + aoi.Y*pitch + (i+StartY)*pitch;
            memcpy(StartAddrY+StartX, &StillStampScrnY[i*DEFAULT_STAMP_SCRN_WIDTH], DEFAULT_STAMP_SCRN_WIDTH);
            memcpy(StartAddrUV+StartX, &StillStampScrnUV[i*DEFAULT_STAMP_SCRN_WIDTH], DEFAULT_STAMP_SCRN_WIDTH);
        }
    }
        break;
    case 2:
    {
        StartX = (aoi.Width>>1) + OffsetX - (DEFAULT_STAMP_THM_WIDTH>>1);
        StartY = (aoi.Height>>1) + OffsetY - (DEFAULT_STAMP_THM_HEIGHT>>1);
        for (i=0; i<DEFAULT_STAMP_THM_HEIGHT; i++) {
            StartAddrY = yAddr + aoi.X + aoi.Y*pitch + (i+StartY)*pitch;
            StartAddrUV = uvAddr + aoi.X + aoi.Y*pitch + (i+StartY)*pitch;
            memcpy(StartAddrY+StartX, &StillStampThmY[i*DEFAULT_STAMP_THM_WIDTH], DEFAULT_STAMP_THM_WIDTH);
            memcpy(StartAddrUV+StartX, &StillStampThmUV[i*DEFAULT_STAMP_THM_WIDTH], DEFAULT_STAMP_THM_WIDTH);
        }
    }
        break;
    default:
        break;
    }


    return OK;
}

/* Initial JPEG DQT */
void AmpUT_initJpegDqt(UINT8 *qTable, int quality)
{
    int i, scale, temp;

    /** for jpeg brc; return the quantization table*/
    if (quality == -1) {
        memcpy(qTable, StdJpegQTable, 128);
        return;
    }

    if (quality <= 0) {
        scale = 5000;
    } else if (quality >= 100) {
        scale = 0;
    } else if (quality < 50) {
        scale = (5000/quality);
    } else {
        scale = (200-quality*2);
    }

    for (i=0; i<128; i++) {
        temp = ((long) StdJpegQTable[i] * scale + 50L) / 100L;
        /* limit the values to the valid range */
        if (temp <= 0L) temp = 1L;
        if (temp > 255L) temp = 255L; /* max quantizer needed for baseline */
        qTable[i] = temp;
    }
}

int AmpUT_StillEnc_LiveviewStop(void)
{
    UINT32 Flag = AMP_ENC_FUNC_FLAG_WAIT;

    if (Status != STATUS_STILL_LIVEVIEW) {
        AmbaPrint("Error status %d",Status);
        return -1;
    }

    AmpImgDzoom_StopDzoom(StillDzoomHdlr);
    if (SencImgSchdlr) {
        AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    }
    Amba_Img_VIn_Invalid(StillEncPri, (UINT32 *)NULL);

    if (WirelessModeEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        DspSuspendEnable = 0;
    } else if (DspSuspendEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        WirelessModeEnable = 0;
    }
    AmpEnc_StopLiveview(StillEncPipe, Flag);

    AmpEnc_Delete(StillEncPipe); StillEncPipe = NULL;
    AmpVin_Delete(StillEncVinA); StillEncVinA = NULL;
    AmbaImgSchdlr_Delete(SencImgSchdlr);
    AmpStillEnc_Delete(StillEncPri); StillEncPri = NULL;

    if (AmpResource_GetVoutSeamlessEnable() == 0) {
        AmpUT_Display_Stop(0);
    }
    if (TVLiveViewEnable) {
        AmpUT_Display_Stop(1);
    }

    if (AmpResource_GetVoutSeamlessEnable() == 0) {
        AmpUT_Display_Window_Delete(0);
    }
    if (TVLiveViewEnable) {
        AmpUT_Display_Window_Delete(1);
    }

    if (StillEncVirtualFifoHdlr) {
        AmpFifo_Delete(StillEncVirtualFifoHdlr);
        StillEncVirtualFifoHdlr = NULL;
    }

    AmpImgDzoom_Delete(StillDzoomHdlr);

    return 0;
}

/**
 * UnitTest: Set WB Gain
 *
 * @return 0 - success, -1 - fail
 */
static int AmpUT_Img3ASetWbGain(AMBA_DSP_IMG_MODE_CFG_s Mode, UINT32 GlobalGain, AMBA_DSP_IMG_WB_GAIN_s *pWbGain)
{
    UINT32 dGain = WB_DGAIN_UNIT;
    AMBA_DSP_IMG_WB_GAIN_s wbGainTmp;

    if (pWbGain->GainR == 0 || pWbGain->GainG == 0 || pWbGain->GainB == 0) {
        AmbaPrint("incorrect wb gain!");
        return -1;
    }
    if (dGain < (WB_DGAIN_UNIT * 7 / 8)) {
        AmbaPrint("incorrect Dgain!, reset to 1x");
        dGain = WB_DGAIN_UNIT;
    }
    //GlobalGain = G_aaa_param.default_ae_algoinfo.def_setting.global_dgain;
    if (GlobalGain < WB_DGAIN_UNIT) {
        AmbaPrint("incorrect global_dgain!, reset to 1x");
        GlobalGain = WB_DGAIN_UNIT;
    }

    // User should guareteen that no overflow will occur here...
    wbGainTmp = *pWbGain;
    dGain = (dGain * GlobalGain) / WB_DGAIN_UNIT;
    wbGainTmp.GainR = pWbGain->GainR * dGain / WB_DGAIN_UNIT;
    wbGainTmp.GainG = pWbGain->GainG * dGain / WB_DGAIN_UNIT;
    wbGainTmp.GainB = pWbGain->GainB * dGain / WB_DGAIN_UNIT;

    return AmbaDSP_ImgSetWbGain(&Mode, &wbGainTmp);
}

/**
 * UnitTest: Fake and default iso config
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_IsoConfigSet(AMBA_DSP_IMG_ALGO_MODE_e isoMode)
{
    extern STILL_ISO_CONFIG_s AmpUT_StillIsoConfig;
    extern STILL_HISO_NORMAL_CONFIG_s AmpUT_StillHisoNormalConfig;
    extern STILL_HISO_CONFIG_s AmpUT_StillHisoConfig;
    extern const UINT8 StillCc3DConfig[];
    extern const UINT8 StillCcRegConfig[];
    //extern const UINT8 StillSecCcConfig[];
#ifdef CONFIG_SOC_A9
    extern UINT8 AmpUT_StillThreeDTable[];
    extern UINT8 AmpUT_StillHisoThreeDTable[];
#endif
    AMBA_DSP_IMG_MODE_CFG_s imgMode;
    AMBA_DSP_IMG_CFG_INFO_s cfgInfo;
    AMBA_DSP_IMG_CTX_INFO_s destCtx, srcCtx;

    /* Initialize the context of ImageKernel of still */
    destCtx.Pipe = AMBA_DSP_IMG_PIPE_STILL;
    destCtx.CtxId = 0;
    srcCtx.CtxId = 0;
    AmbaDSP_ImgInitCtx(0, 0, &destCtx, &srcCtx);
    AmbaPrint("%s: AmbaDSP_ImgInitCtx done", __FUNCTION__);

    /* Initialize the configure of ImageKernel of Still */
    cfgInfo.Pipe = AMBA_DSP_IMG_PIPE_STILL;
    cfgInfo.CfgId = 0;
    if (isoMode == AMBA_DSP_IMG_ALGO_MODE_LISO)
        AmbaDSP_ImgInitCfg(&cfgInfo, AMBA_DSP_IMG_ALGO_MODE_LISO);
    else
        AmbaDSP_ImgInitCfg(&cfgInfo, AMBA_DSP_IMG_ALGO_MODE_HISO);
    AmbaPrint("%s: AmbaDSP_ImgInitCfg done", __FUNCTION__);

    /* Setup mode of ImageKernel */
    /*
     * batch rule for normal filter:
     * LISO : batch_id = LISO
     * HISO : batch_id = LISO, some idsp_cmds need to issue again with batch_id = Hiso
    */
    memset(&imgMode, 0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
    imgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
    if (isoMode == AMBA_DSP_IMG_ALGO_MODE_LISO) {
        imgMode.AlgoMode  = AMBA_DSP_IMG_ALGO_MODE_LISO;
    } else {
        imgMode.AlgoMode  = AMBA_DSP_IMG_ALGO_MODE_HISO;
    }
    imgMode.BatchId = (G_iso == 0)? AMBA_DSP_STILL_HISO_FILTER:((G_iso == 1)? AMBA_DSP_STILL_LISO_FILTER: AMBA_DSP_VIDEO_FILTER);
    imgMode.ContextId = 0;
    imgMode.ConfigId  = 0;

    /* Setup related windows size */
    {
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.VinSensorGeo.Width    = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.VinSensorGeo.Height   = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.MainWinDim.Width      = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[StillEncModeIdx].StillMainWidth;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.MainWinDim.Height     = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[StillEncModeIdx].StillMainHeight;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.R2rOutWinDim.Width    = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.R2rOutWinDim.Height   = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.DmyWinGeo.Width       = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.DmyWinGeo.Height      = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.CfaWinDim.Width       = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.CfaWinDim.Height      = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.ActWinCrop.RightBotX  = StillEncMgt[StillEncModeIdx].StillCaptureWidth << 16;
        AmpUT_StillIsoConfig.WarpCalcInfo.Data.ActWinCrop.RightBotY  = StillEncMgt[StillEncModeIdx].StillCaptureHeight << 16;
    }

    /* Prepare filters, same as Amba_Img_Set_Still_Pipe_Ctrl_Params() */
    if (AmpUT_StillIsoConfig.AeStatInfo.Enable)
        AmbaDSP_Img3aSetAeStatInfo(&imgMode, &(AmpUT_StillIsoConfig.AeStatInfo.Data));

    if (AmpUT_StillIsoConfig.AfStatInfo.Enable)
        AmbaDSP_Img3aSetAfStatInfo(&imgMode, &(AmpUT_StillIsoConfig.AfStatInfo.Data));

    if (AmpUT_StillIsoConfig.AwbStatInfo.Enable)
        AmbaDSP_Img3aSetAwbStatInfo(&imgMode, &(AmpUT_StillIsoConfig.AwbStatInfo.Data));
#ifdef CONFIG_SOC_A12
    {
        // 20140911 Chester: this part is for testing, should be removed someday
        AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
        AMBA_DSP_IMG_SENSOR_INFO_s SensorInfo = {0};
        AmbaSensor_GetStatus(EncChannel, &SensorStatus);
        SensorInfo.SensorPattern = (UINT8)SensorStatus.ModeInfo.OutputInfo.CfaPattern;
        AmbaDSP_ImgSetVinSensorInfo(&imgMode, &SensorInfo);
    }
#else
    if (AmpUT_StillIsoConfig.SensorInfo.Enable)
        AmbaDSP_ImgSetVinSensorInfo(&imgMode, &(AmpUT_StillIsoConfig.SensorInfo.Data));
#endif
    if (AmpUT_StillIsoConfig.SbpCorr.Enable)
        AmbaDSP_ImgSetStaticBadPixelCorrection(&imgMode, &(AmpUT_StillIsoConfig.SbpCorr.Data));

    if (AmpUT_StillIsoConfig.VignetteCalcInfo.Enable) {
        AmbaDSP_ImgCalcVignetteCompensation(&imgMode, &(AmpUT_StillIsoConfig.VignetteCalcInfo.Data));
        AmbaDSP_ImgSetVignetteCompensation(&imgMode);
    }

    if (AmpUT_StillIsoConfig.CACalInfo.Enable) {
        AmbaDSP_ImgCalcCawarpCompensation(&imgMode, &(AmpUT_StillIsoConfig.CACalInfo.Data));
        AmbaDSP_ImgSetCawarpCompensation(&imgMode);
    }

    if (isoMode == AMBA_DSP_IMG_ALGO_MODE_LISO) {
        //LISO normal filter
        if (AmpUT_StillIsoConfig.StaticBlackLevelInfo.Enable)
            AmbaDSP_ImgSetStaticBlackLevel(&imgMode, &(AmpUT_StillIsoConfig.StaticBlackLevelInfo.Data));

//        if (AmpUT_StillIsoConfig.VerticalFlipCorrectionInfo.Enable)
//          AmbaDSP_ImgSetDynamicBadPixelCorrection(&imgMode, &(AmpUT_StillIsoConfig.VerticalFlipCorrectionInfo.Data));

        if (AmpUT_StillIsoConfig.CfaLeakageFilterInfo.Enable)
            AmbaDSP_ImgSetCfaLeakageFilter(&imgMode, &(AmpUT_StillIsoConfig.CfaLeakageFilterInfo.Data));

        if (AmpUT_StillIsoConfig.CfaNoiseFilterInfo.Enable)
            AmbaDSP_ImgSetCfaNoiseFilter(&imgMode, &(AmpUT_StillIsoConfig.CfaNoiseFilterInfo.Data));

        if (AmpUT_StillIsoConfig.AntiAliasingInfo.Enable)
            AmbaDSP_ImgSetAntiAliasing(&imgMode, &AmpUT_StillIsoConfig.AntiAliasingInfo.Data);

        if (AmpUT_StillIsoConfig.WbGainInfo.Enable)
            AmpUT_Img3ASetWbGain(imgMode, AmpUT_StillIsoConfig.WbGainInfo.GlobalGain, &(AmpUT_StillIsoConfig.WbGainInfo.Data));

        if (AmpUT_StillIsoConfig.DgainSaturationLevelInfo.Enable)
            AmbaDSP_ImgSetDgainSaturationLevel(&imgMode, &(AmpUT_StillIsoConfig.DgainSaturationLevelInfo.Data));

        if (AmpUT_StillIsoConfig.LocalExposureInfo.Enable)
            AmbaDSP_ImgSetLocalExposure(&imgMode, &(AmpUT_StillIsoConfig.LocalExposureInfo.Data));

        if (AmpUT_StillIsoConfig.ColorCorrectionRegInfo.Enable) {
            AmpUT_StillIsoConfig.ColorCorrectionRegInfo.Data.RegSettingAddr = (UINT32)StillCcRegConfig;
            AmbaDSP_ImgSetColorCorrectionReg(&imgMode, &(AmpUT_StillIsoConfig.ColorCorrectionRegInfo.Data));
        }

        if (AmpUT_StillIsoConfig.ColorCorrectionInfo.Enable) {
            AmpUT_StillIsoConfig.ColorCorrectionInfo.Data.MatrixThreeDTableAddr = (UINT32)StillCc3DConfig;
            //AmpUT_StillIsoConfig.ColorCorrectionInfo.Data.SecCcAddr = (UINT32)StillSecCcConfig;
            AmbaDSP_ImgSetColorCorrection(&imgMode, &(AmpUT_StillIsoConfig.ColorCorrectionInfo.Data));
        }

        if (AmpUT_StillIsoConfig.ToneCurveInfo.Enable)
            AmbaDSP_ImgSetToneCurve(&imgMode, &(AmpUT_StillIsoConfig.ToneCurveInfo.Data));

        if (AmpUT_StillIsoConfig.Rgb2YuvInfo.Enable)
            AmbaDSP_ImgSetRgbToYuvMatrix(&imgMode, &(AmpUT_StillIsoConfig.Rgb2YuvInfo.Data));

        if (AmpUT_StillIsoConfig.ChromaScaleInfo.Enable)
            AmbaDSP_ImgSetChromaScale(&imgMode, &(AmpUT_StillIsoConfig.ChromaScaleInfo.Data));

        if (AmpUT_StillIsoConfig.ChromaMedianFilterInfor.Enable)
            AmbaDSP_ImgSetChromaMedianFilter(&imgMode, &(AmpUT_StillIsoConfig.ChromaMedianFilterInfor.Data));

        if (AmpUT_StillIsoConfig.DemosaicInfo.Enable)
            AmbaDSP_ImgSetDemosaic(&imgMode, &(AmpUT_StillIsoConfig.DemosaicInfo.Data));

        if (AmpUT_StillIsoConfig.GbGrMismatch.Enable)
            AmbaDSP_ImgSetGbGrMismatch(&imgMode, &(AmpUT_StillIsoConfig.GbGrMismatch.Data));

    #ifdef CONFIG_SOC_A9
        if (AmpUT_StillIsoConfig.SharpenAOrSpatialFilterInfo.Enable)
            AmbaDSP_ImgSet1stLumaProcessingMode(&imgMode, AmpUT_StillIsoConfig.SharpenAOrSpatialFilterInfo.Data);

        if (AmpUT_StillIsoConfig.CdnrInfo.Enable)
            AmbaDSP_ImgSetColorDependentNoiseReduction(&imgMode, &(AmpUT_StillIsoConfig.CdnrInfo.Data));

        if (AmpUT_StillIsoConfig.AsfInfo.Enable) {
            AmpUT_StillIsoConfig.AsfInfo.Data.Adapt.ThreeD.pTable = AmpUT_StillThreeDTable;
            AmbaDSP_ImgSetAdvanceSpatialFilter(&imgMode, &(AmpUT_StillIsoConfig.AsfInfo.Data));
        }
    #endif

        if (AmpUT_StillIsoConfig.ChromaFilterInfo.Enable)
            AmbaDSP_ImgSetChromaFilter(&imgMode, &(AmpUT_StillIsoConfig.ChromaFilterInfo.Data));

        //LISO only
    #ifdef CONFIG_SOC_A9
        if (AmpUT_StillIsoConfig.SharpenBoth.Enable) {
            AmpUT_StillIsoConfig.SharpenBoth.Data.ThreeD.pTable = AmpUT_StillThreeDTable;
            AmbaDSP_ImgSetFinalSharpenNoiseBoth(&imgMode, &(AmpUT_StillIsoConfig.SharpenBoth.Data));
        }
    #endif

        if (AmpUT_StillIsoConfig.SharpenNoise.Enable)
            AmbaDSP_ImgSetFinalSharpenNoiseNoise(&imgMode, &(AmpUT_StillIsoConfig.SharpenNoise.Data));

        if (AmpUT_StillIsoConfig.SharpenBFir.Enable)
            AmbaDSP_ImgSetFinalSharpenNoiseSharpenFir(&imgMode, &(AmpUT_StillIsoConfig.SharpenBFir.Data));

        if (AmpUT_StillIsoConfig.SharpenBCoringInfo.Enable)
            AmbaDSP_ImgSetFinalSharpenNoiseSharpenCoring(&imgMode, &(AmpUT_StillIsoConfig.SharpenBCoringInfo.Data));

        if (AmpUT_StillIsoConfig.SharpBLevelOverallInfo.Enable)
            AmbaDSP_ImgSetFinalSharpenNoiseSharpenCoringIndexScale(&imgMode, &(AmpUT_StillIsoConfig.SharpBLevelOverallInfo.Data));

        if (AmpUT_StillIsoConfig.SharpBLevelMinInfo.Enable)
            AmbaDSP_ImgSetFinalSharpenNoiseSharpenMinCoringResult(&imgMode, &(AmpUT_StillIsoConfig.SharpBLevelMinInfo.Data));

        if (AmpUT_StillIsoConfig.SharpenBScaleCoring.Enable)
            AmbaDSP_ImgSetFinalSharpenNoiseSharpenScaleCoring(&imgMode, &(AmpUT_StillIsoConfig.SharpenBScaleCoring.Data));
    } else {
        AMBA_DSP_IMG_MODE_CFG_s modeHisoBatch;
        //HISO normal filter
        if (AmpUT_StillHisoNormalConfig.StaticBlackLevelInfo.Enable)
            AmbaDSP_ImgSetStaticBlackLevel(&imgMode, &(AmpUT_StillHisoNormalConfig.StaticBlackLevelInfo.Data));

//        if (AmpUT_StillHisoNormalConfig.VerticalFlipCorrectionInfo.Enable)
//            AmbaDSP_ImgSetDynamicBadPixelCorrection(&imgMode, &(AmpUT_StillHisoNormalConfig.VerticalFlipCorrectionInfo.Data));

        if (AmpUT_StillHisoNormalConfig.CfaLeakageFilterInfo.Enable)
            AmbaDSP_ImgSetCfaLeakageFilter(&imgMode, &(AmpUT_StillHisoNormalConfig.CfaLeakageFilterInfo.Data));

        if (AmpUT_StillHisoNormalConfig.CfaNoiseFilterInfo.Enable)
            AmbaDSP_ImgSetCfaNoiseFilter(&imgMode, &(AmpUT_StillHisoNormalConfig.CfaNoiseFilterInfo.Data));

        if (AmpUT_StillHisoNormalConfig.AntiAliasingInfo.Enable)
            AmbaDSP_ImgSetAntiAliasing(&imgMode, &AmpUT_StillHisoNormalConfig.AntiAliasingInfo.Data);

        if (AmpUT_StillHisoNormalConfig.WbGainInfo.Enable)
            AmpUT_Img3ASetWbGain(imgMode, AmpUT_StillHisoNormalConfig.WbGainInfo.GlobalGain, &(AmpUT_StillHisoNormalConfig.WbGainInfo.Data));

        if (AmpUT_StillHisoNormalConfig.DgainSaturationLevelInfo.Enable)
            AmbaDSP_ImgSetDgainSaturationLevel(&imgMode, &(AmpUT_StillHisoNormalConfig.DgainSaturationLevelInfo.Data));

        if (AmpUT_StillHisoNormalConfig.LocalExposureInfo.Enable)
            AmbaDSP_ImgSetLocalExposure(&imgMode, &(AmpUT_StillHisoNormalConfig.LocalExposureInfo.Data));

        if (AmpUT_StillHisoNormalConfig.ColorCorrectionRegInfo.Enable) {
            AmpUT_StillHisoNormalConfig.ColorCorrectionRegInfo.Data.RegSettingAddr = (UINT32)StillCcRegConfig;
            AmbaDSP_ImgSetColorCorrectionReg(&imgMode, &(AmpUT_StillHisoNormalConfig.ColorCorrectionRegInfo.Data));
        }

        if (AmpUT_StillHisoNormalConfig.ColorCorrectionInfo.Enable) {
            AmpUT_StillHisoNormalConfig.ColorCorrectionInfo.Data.MatrixThreeDTableAddr = (UINT32)StillCc3DConfig;
            //AmpUT_StillHisoNormalConfig.ColorCorrectionInfo.Data.SecCcAddr = (UINT32)StillSecCcConfig;
            AmbaDSP_ImgSetColorCorrection(&imgMode, &(AmpUT_StillHisoNormalConfig.ColorCorrectionInfo.Data));
        }

        if (AmpUT_StillHisoNormalConfig.ToneCurveInfo.Enable)
            AmbaDSP_ImgSetToneCurve(&imgMode, &(AmpUT_StillHisoNormalConfig.ToneCurveInfo.Data));

        if (AmpUT_StillHisoNormalConfig.Rgb2YuvInfo.Enable)
            AmbaDSP_ImgSetRgbToYuvMatrix(&imgMode, &(AmpUT_StillHisoNormalConfig.Rgb2YuvInfo.Data));

        if (AmpUT_StillHisoNormalConfig.ChromaScaleInfo.Enable)
            AmbaDSP_ImgSetChromaScale(&imgMode, &(AmpUT_StillHisoNormalConfig.ChromaScaleInfo.Data));

        if (AmpUT_StillHisoNormalConfig.ChromaMedianFilterInfor.Enable)
            AmbaDSP_ImgSetChromaMedianFilter(&imgMode, &(AmpUT_StillHisoNormalConfig.ChromaMedianFilterInfor.Data));

        if (AmpUT_StillHisoNormalConfig.DemosaicInfo.Enable)
            AmbaDSP_ImgSetDemosaic(&imgMode, &(AmpUT_StillHisoNormalConfig.DemosaicInfo.Data));

        if (AmpUT_StillHisoNormalConfig.GbGrMismatch.Enable)
            AmbaDSP_ImgSetGbGrMismatch(&imgMode, &(AmpUT_StillHisoNormalConfig.GbGrMismatch.Data));

    #ifdef CONFIG_SOC_A9
        if (AmpUT_StillHisoNormalConfig.SharpenAOrSpatialFilterInfo.Enable)
            AmbaDSP_ImgSet1stLumaProcessingMode(&imgMode, AmpUT_StillHisoNormalConfig.SharpenAOrSpatialFilterInfo.Data);

        if (AmpUT_StillHisoNormalConfig.CdnrInfo.Enable)
            AmbaDSP_ImgSetColorDependentNoiseReduction(&imgMode, &(AmpUT_StillHisoNormalConfig.CdnrInfo.Data));

        if (AmpUT_StillHisoNormalConfig.AsfInfo.Enable) {
            AmpUT_StillHisoNormalConfig.AsfInfo.Data.Adapt.ThreeD.pTable = AmpUT_StillThreeDTable;
            AmbaDSP_ImgSetAdvanceSpatialFilter(&imgMode, &(AmpUT_StillHisoNormalConfig.AsfInfo.Data));
        }
    #endif

        if (AmpUT_StillHisoNormalConfig.ChromaFilterInfo.Enable)
            AmbaDSP_ImgSetChromaFilter(&imgMode, &(AmpUT_StillHisoNormalConfig.ChromaFilterInfo.Data));

        //HISO only
        if (AmpUT_StillHisoConfig.AntiAliasing.Enable)
            AmbaDSP_ImgSetHighIsoAntiAliasing(&imgMode, &AmpUT_StillHisoConfig.AntiAliasing.Data);

        if (AmpUT_StillHisoConfig.CfaLeakageFilter.Enable)
            AmbaDSP_ImgSetHighIsoCfaLeakageFilter(&imgMode, &(AmpUT_StillHisoConfig.CfaLeakageFilter.Data));

        if (AmpUT_StillHisoConfig.DynamicBadPixelCorrection.Enable)
            AmbaDSP_ImgSetHighIsoDynamicBadPixelCorrection(&imgMode, &(AmpUT_StillHisoConfig.DynamicBadPixelCorrection.Data));

        if (AmpUT_StillHisoConfig.CfaNoiseFilter.Enable)
            AmbaDSP_ImgSetHighIsoCfaNoiseFilter(&imgMode, &(AmpUT_StillHisoConfig.CfaNoiseFilter.Data));

        if (AmpUT_StillHisoConfig.GbGrMismatch.Enable)
            AmbaDSP_ImgSetHighIsoGbGrMismatch(&imgMode, &(AmpUT_StillHisoConfig.GbGrMismatch.Data));

        if (AmpUT_StillHisoConfig.Demosaic.Enable)
            AmbaDSP_ImgSetHighIsoDemosaic(&imgMode, &(AmpUT_StillHisoConfig.Demosaic.Data));

        if (AmpUT_StillHisoConfig.ChromaMedianFilter.Enable)
            AmbaDSP_ImgSetHighIsoChromaMedianFilter(&imgMode, &(AmpUT_StillHisoConfig.ChromaMedianFilter.Data));

        if (AmpUT_StillHisoConfig.ColorDependentNoiseReduction.Enable) {
        #ifdef CONFIG_SOC_A9
            AmbaDSP_ImgSetHighIsoColorDependentNoiseReduction(&imgMode, &(AmpUT_StillHisoConfig.ColorDependentNoiseReduction.Data));
        #endif
        }

        if (AmpUT_StillHisoConfig.DeferColorCorrection.Enable)
            AmbaDSP_ImgSetHighIsoDeferColorCorrection(&imgMode, &(AmpUT_StillHisoConfig.DeferColorCorrection.Data));

        //Hiso Asf
    #ifdef CONFIG_SOC_A9
        if (AmpUT_StillHisoConfig.AdvanceSpatialFilter.Enable) {
            AmpUT_StillHisoConfig.AdvanceSpatialFilter.Data.Adapt.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoAdvanceSpatialFilter(&imgMode, &(AmpUT_StillHisoConfig.AdvanceSpatialFilter.Data));
        }

        if (AmpUT_StillHisoConfig.HighAdvanceSpatialFilter.Enable) {
            AmpUT_StillHisoConfig.HighAdvanceSpatialFilter.Data.Adapt.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoHighAdvanceSpatialFilter(&imgMode, &(AmpUT_StillHisoConfig.HighAdvanceSpatialFilter.Data));
        }

        if (AmpUT_StillHisoConfig.LowAdvanceSpatialFilter.Enable) {
            AmpUT_StillHisoConfig.LowAdvanceSpatialFilter.Data.Adapt.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoLowAdvanceSpatialFilter(&imgMode, &(AmpUT_StillHisoConfig.LowAdvanceSpatialFilter.Data));
        }

        if (AmpUT_StillHisoConfig.Med1AdvanceSpatialFilter.Enable) {
            AmpUT_StillHisoConfig.Med1AdvanceSpatialFilter.Data.Adapt.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoMed1AdvanceSpatialFilter(&imgMode, &(AmpUT_StillHisoConfig.Med1AdvanceSpatialFilter.Data));
        }

        if (AmpUT_StillHisoConfig.Med2AdvanceSpatialFilter.Enable) {
            AmpUT_StillHisoConfig.Med2AdvanceSpatialFilter.Data.Adapt.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoMed2AdvanceSpatialFilter(&imgMode, &(AmpUT_StillHisoConfig.Med2AdvanceSpatialFilter.Data));
        }

        if (AmpUT_StillHisoConfig.Li2ndAdvanceSpatialFilter.Enable) {
            AmpUT_StillHisoConfig.Li2ndAdvanceSpatialFilter.Data.Adapt.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoLi2ndAdvanceSpatialFilter(&imgMode, &(AmpUT_StillHisoConfig.Li2ndAdvanceSpatialFilter.Data));
        }

        if (AmpUT_StillHisoConfig.ChromaAdvanceSpatialFilter.Enable) {
          // JH  AmpUT_StillHisoConfig.ChromaAdvanceSpatialFilter.Data.Adapt.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoChromaAdvanceSpatialFilter(&imgMode, &(AmpUT_StillHisoConfig.ChromaAdvanceSpatialFilter.Data));
        }

        //Hiso Sharpen hi_high_
        if (AmpUT_StillHisoConfig.HighSharpenNoiseBoth.Enable) {
            AmpUT_StillHisoConfig.HighSharpenNoiseBoth.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseBoth(&imgMode, &(AmpUT_StillHisoConfig.HighSharpenNoiseBoth.Data));
        }
    #endif

        if (AmpUT_StillHisoConfig.HighSharpenNoiseNoise.Enable)
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseNoise(&imgMode, &(AmpUT_StillHisoConfig.HighSharpenNoiseNoise.Data));

        if (AmpUT_StillHisoConfig.HighSharpenNoiseSharpenFir.Enable)
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenFir(&imgMode, &(AmpUT_StillHisoConfig.HighSharpenNoiseSharpenFir.Data));

        if (AmpUT_StillHisoConfig.HighSharpenNoiseSharpenCoring.Enable)
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenCoring(&imgMode, &(AmpUT_StillHisoConfig.HighSharpenNoiseSharpenCoring.Data));

        if (AmpUT_StillHisoConfig.HighSharpenNoiseSharpenCoringIndexScale.Enable)
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenCoringIndexScale(&imgMode, &(AmpUT_StillHisoConfig.HighSharpenNoiseSharpenCoringIndexScale.Data));

        if (AmpUT_StillHisoConfig.HighSharpenNoiseSharpenMinCoringResult.Enable)
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenMinCoringResult(&imgMode, &(AmpUT_StillHisoConfig.HighSharpenNoiseSharpenMinCoringResult.Data));

        if (AmpUT_StillHisoConfig.HighSharpenNoiseSharpenScaleCoring.Enable)
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenScaleCoring(&imgMode, &(AmpUT_StillHisoConfig.HighSharpenNoiseSharpenScaleCoring.Data));

        //Hiso Sharpen hi_med_
    #ifdef CONFIG_SOC_A9
        if (AmpUT_StillHisoConfig.MedSharpenNoiseBoth.Enable) {
            AmpUT_StillHisoConfig.MedSharpenNoiseBoth.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseBoth(&imgMode, &(AmpUT_StillHisoConfig.MedSharpenNoiseBoth.Data));
        }
    #endif

        if (AmpUT_StillHisoConfig.MedSharpenNoiseNoise.Enable)
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseNoise(&imgMode, &(AmpUT_StillHisoConfig.MedSharpenNoiseNoise.Data));

        if (AmpUT_StillHisoConfig.MedSharpenNoiseSharpenFir.Enable)
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenFir(&imgMode, &(AmpUT_StillHisoConfig.MedSharpenNoiseSharpenFir.Data));

        if (AmpUT_StillHisoConfig.MedSharpenNoiseSharpenCoring.Enable)
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenCoring(&imgMode, &(AmpUT_StillHisoConfig.MedSharpenNoiseSharpenCoring.Data));

        if (AmpUT_StillHisoConfig.MedSharpenNoiseSharpenCoringIndexScale.Enable)
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenCoringIndexScale(&imgMode, &(AmpUT_StillHisoConfig.MedSharpenNoiseSharpenCoringIndexScale.Data));

        if (AmpUT_StillHisoConfig.MedSharpenNoiseSharpenMinCoringResult.Enable)
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenMinCoringResult(&imgMode, &(AmpUT_StillHisoConfig.MedSharpenNoiseSharpenMinCoringResult.Data));

        if (AmpUT_StillHisoConfig.MedSharpenNoiseSharpenScaleCoring.Enable)
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenScaleCoring(&imgMode, &(AmpUT_StillHisoConfig.MedSharpenNoiseSharpenScaleCoring.Data));

        //Hiso Sharpen hili_
    #ifdef CONFIG_SOC_A9
        if (AmpUT_StillHisoConfig.Liso1SharpenNoiseBoth.Enable) {
            AmpUT_StillHisoConfig.Liso1SharpenNoiseBoth.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseBoth(&imgMode, &(AmpUT_StillHisoConfig.Liso1SharpenNoiseBoth.Data));
        }

        if (AmpUT_StillHisoConfig.Liso1SharpenNoiseNoise.Enable)
            AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseNoise(&imgMode, &(AmpUT_StillHisoConfig.Liso1SharpenNoiseNoise.Data));

        if (AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenFir.Enable)
            AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenFir(&imgMode, &(AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenFir.Data));

        if (AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenCoring.Enable)
            AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenCoring(&imgMode, &(AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenCoring.Data));

        if (AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenCoringIndexScale.Enable)
            AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenCoringIndexScale(&imgMode, &(AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenCoringIndexScale.Data));

        if (AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenMinCoringResult.Enable)
            AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenMinCoringResult(&imgMode, &(AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenMinCoringResult.Data));

        if (AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenScaleCoring.Enable)
            AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenScaleCoring(&imgMode, &(AmpUT_StillHisoConfig.Liso1SharpenNoiseSharpenScaleCoring.Data));
    #endif


        //Hiso Sharpen li_2nd_
    #ifdef CONFIG_SOC_A9
        if (AmpUT_StillHisoConfig.Liso2SharpenNoiseBoth.Enable) {
            AmpUT_StillHisoConfig.Liso2SharpenNoiseBoth.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseBoth(&imgMode, &(AmpUT_StillHisoConfig.Liso2SharpenNoiseBoth.Data));
        }

        if (AmpUT_StillHisoConfig.Liso2SharpenNoiseNoise.Enable)
            AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseNoise(&imgMode, &(AmpUT_StillHisoConfig.Liso2SharpenNoiseNoise.Data));

        if (AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenFir.Enable)
            AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenFir(&imgMode, &(AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenFir.Data));

        if (AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenCoring.Enable)
            AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenCoring(&imgMode, &(AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenCoring.Data));

        if (AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenCoringIndexScale.Enable)
            AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenCoringIndexScale(&imgMode, &(AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenCoringIndexScale.Data));

        if (AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenMinCoringResult.Enable)
            AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenMinCoringResult(&imgMode, &(AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenMinCoringResult.Data));

        if (AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenScaleCoring.Enable)
            AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenScaleCoring(&imgMode, &(AmpUT_StillHisoConfig.Liso2SharpenNoiseSharpenScaleCoring.Data));
    #endif


        //Chroma
        if (AmpUT_StillHisoConfig.ChromaFilterHigh.Enable)
            AmbaDSP_ImgHighIsoSetChromaFilterHigh(&imgMode, &(AmpUT_StillHisoConfig.ChromaFilterHigh.Data));

        if (AmpUT_StillHisoConfig.ChromaFilterLowVeryLow.Enable)
            AmbaDSP_ImgHighIsoSetChromaFilterLowVeryLow(&imgMode, &(AmpUT_StillHisoConfig.ChromaFilterLowVeryLow.Data));

        if (AmpUT_StillHisoConfig.ChromaFilterPre.Enable)
            AmbaDSP_ImgHighIsoSetChromaFilterPre(&imgMode, &(AmpUT_StillHisoConfig.ChromaFilterPre.Data));

        if (AmpUT_StillHisoConfig.ChromaFilterMed.Enable)
            AmbaDSP_ImgHighIsoSetChromaFilterMed(&imgMode, &(AmpUT_StillHisoConfig.ChromaFilterMed.Data));

        if (AmpUT_StillHisoConfig.ChromaFilterLow.Enable)
            AmbaDSP_ImgHighIsoSetChromaFilterLow(&imgMode, &(AmpUT_StillHisoConfig.ChromaFilterLow.Data));

        if (AmpUT_StillHisoConfig.ChromaFilterVeryLow.Enable)
            AmbaDSP_ImgHighIsoSetChromaFilterVeryLow(&imgMode, &(AmpUT_StillHisoConfig.ChromaFilterVeryLow.Data));

        //Combine
    #ifdef CONFIG_SOC_A9
        if (AmpUT_StillHisoConfig.ChromaFilterMedCombine.Enable) {
            AmpUT_StillHisoConfig.ChromaFilterMedCombine.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgHighIsoSetChromaFilterMedCombine(&imgMode, &(AmpUT_StillHisoConfig.ChromaFilterMedCombine.Data));
        }

        if (AmpUT_StillHisoConfig.ChromaFilterLowCombine.Enable) {
            AmpUT_StillHisoConfig.ChromaFilterLowCombine.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgHighIsoSetChromaFilterLowCombine(&imgMode, &(AmpUT_StillHisoConfig.ChromaFilterLowCombine.Data));
        }

        if (AmpUT_StillHisoConfig.ChromaFilterVeryLowCombine.Enable) {
            AmpUT_StillHisoConfig.ChromaFilterVeryLowCombine.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgHighIsoSetChromaFilterVeryLowCombine(&imgMode, &(AmpUT_StillHisoConfig.ChromaFilterVeryLowCombine.Data));
        }

        if (AmpUT_StillHisoConfig.LumaNoiseCombine.Enable) {
            AmpUT_StillHisoConfig.LumaNoiseCombine.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgHighIsoSetLumaNoiseCombine(&imgMode, &(AmpUT_StillHisoConfig.LumaNoiseCombine.Data));
        }

        if (AmpUT_StillHisoConfig.LowASFCombine.Enable) {
            AmpUT_StillHisoConfig.LowASFCombine.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgHighIsoSetLowASFCombine(&imgMode, &(AmpUT_StillHisoConfig.LowASFCombine.Data));
        }

        if (AmpUT_StillHisoConfig.HighIsoCombine.Enable) {
            AmpUT_StillHisoConfig.HighIsoCombine.Data.ThreeD.pTable = AmpUT_StillHisoThreeDTable;
            AmbaDSP_ImgHighIsoSetHighIsoCombine(&imgMode, &(AmpUT_StillHisoConfig.HighIsoCombine.Data));
        }

        if (AmpUT_StillHisoConfig.HighIsoFreqRecover.Enable)
            AmbaDSP_ImgHighIsoSetHighIsoFreqRecover(&imgMode, &(AmpUT_StillHisoConfig.HighIsoFreqRecover.Data));

        if (AmpUT_StillHisoConfig.HighIsoLumaBlend.Enable) {
            AmbaDSP_ImgHighIsoSetHighIsoLumaBlend(&imgMode, &(AmpUT_StillHisoConfig.HighIsoLumaBlend.Data));

            if (AmpUT_StillHisoConfig.HighIsoLumaBlend.Data.Enable) {
                if (AmpUT_StillHisoConfig.HighIsoBlendLumaLevel.Enable)
                    AmbaDSP_ImgHighIsoSetHighIsoBlendLumaLevel(&imgMode, &(AmpUT_StillHisoConfig.HighIsoBlendLumaLevel.Data));
            }
        }
    #endif


        //to hiso batch
        modeHisoBatch = imgMode;
        modeHisoBatch.BatchId = AMBA_DSP_STILL_HISO_FILTER;

        AmbaDSP_ImgSetStaticBadPixelCorrection(&modeHisoBatch, &(AmpUT_StillIsoConfig.SbpCorr.Data));
        AmbaDSP_ImgSetVignetteCompensation(&modeHisoBatch);
        AmbaDSP_ImgSetWarpCompensation(&modeHisoBatch);
        AmbaDSP_ImgSetCawarpCompensation(&modeHisoBatch);
    }

    return OK;
}

/**
 * UnitTest: Set still WB gain
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_SetStillWB(UINT32 chipNo, AMBA_DSP_IMG_MODE_CFG_s *imgMode)
{
    AMBA_DSP_IMG_WB_GAIN_s StillWbGain = {0};
    AMBA_AE_INFO_s StillAeInfo[MAX_AEB_NUM];
    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, chipNo, IP_MODE_STILL, (UINT32)StillAeInfo);
    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, chipNo, IP_MODE_STILL, (UINT32)&StillWbGain);
    StillWbGain.AeGain = StillAeInfo[0].Dgain;
    AmbaPrintColor(GREEN,"[UT] StillWbGain: GainR(%u) GainG(%u) GainB(%u) AEGain(%u) GlobalD(%u)", \
        StillWbGain.GainR, StillWbGain.GainG, StillWbGain.GainB, StillWbGain.AeGain, StillWbGain.GlobalDGain);
    return AmbaDSP_ImgSetWbGain(imgMode, &StillWbGain);
}

int AmpUT_SetStillSensorInfo(AMBA_DSP_IMG_MODE_CFG_s *imgMode)
{
    AMBA_DSP_IMG_SENSOR_INFO_s SensorInfo = {0};
    AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
    AmbaSensor_GetStatus(EncChannel, &SensorStatus);
    SensorInfo.SensorPattern = (UINT8)SensorStatus.ModeInfo.OutputInfo.CfaPattern;
    return AmbaDSP_ImgSetVinSensorInfo(imgMode, &SensorInfo);
}

#ifdef CONFIG_SOC_A12
/**
 * UnitTest: Set Active window when OB mode is enable
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_SetStillActiveVinWindow(AMBA_DSP_IMG_MODE_CFG_s *imgMode)
{
    AMBA_DSP_IMG_WIN_GEOMETRY_s VinActiveWindow = {0};
    AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};
    UINT16 RawCapWidth, RawCapHeight, RecordingWidth, RecordingHeight;
    UINT16 OffsetX, OffsetY;

    SensorMode.Data = StillEncMgt[StillEncModeIdx].InputStillMode;
    AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);

    RawCapWidth = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
    RawCapHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
    RecordingWidth = SensorModeInfo.OutputInfo.RecordingPixels.Width;
    RecordingHeight = SensorModeInfo.OutputInfo.RecordingPixels.Height;
    OffsetX = SensorModeInfo.OutputInfo.RecordingPixels.StartX;
    OffsetY = SensorModeInfo.OutputInfo.RecordingPixels.StartY;

    VinActiveWindow.Width = RawCapWidth;
    VinActiveWindow.Height = RawCapHeight;
    VinActiveWindow.StartX = (OffsetX + (RecordingWidth - RawCapWidth)/2)&0xFFFE;
    VinActiveWindow.StartY = (OffsetY + (RecordingHeight - RawCapHeight)/2)&0xFFFE;

    return AmbaDSP_WarpCore_SetVinActiveWin(imgMode, &VinActiveWindow);
}
#endif

UINT32 AmpUT_StillEncControlStillAdj(UINT32 aeIdx)
{
    UINT32 ImgIpChNo = 0;
    AMBA_DSP_IMG_ALGO_MODE_e AlgoMode = 0;
    AMBA_DSP_IMG_CTX_INFO_s DestCtx = {0}, SrcCtx = {0};
    AMBA_DSP_IMG_CFG_INFO_s CfgInfo = {0};
    AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};

    AlgoMode = (G_iso == 0)? AMBA_DSP_IMG_ALGO_MODE_HISO:((G_iso == 1)? AMBA_DSP_IMG_ALGO_MODE_LISO: AMBA_DSP_IMG_ALGO_MODE_FAST);

    if (0/*Fake Iso config, use for verification*/) {
        AmpUT_IsoConfigSet(AlgoMode);
    } else {
        /* Init Ctx */
        DestCtx.Pipe = AMBA_DSP_IMG_PIPE_STILL;
        DestCtx.CtxId = 0;
        SrcCtx.CtxId = 0;
        AmbaDSP_ImgInitCtx(0, 0, &DestCtx, &SrcCtx);

        /* Init Cfg */
        CfgInfo.Pipe = AMBA_DSP_IMG_PIPE_STILL;
        CfgInfo.CfgId = 0;
        AmbaDSP_ImgInitCfg(&CfgInfo, AlgoMode);

        ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
        ImgMode.AlgoMode = AlgoMode;
        ImgMode.BatchId = (G_iso == 0)? AMBA_DSP_STILL_HISO_FILTER:((G_iso == 1)? AMBA_DSP_STILL_LISO_FILTER: AMBA_DSP_VIDEO_FILTER);
        ImgMode.ContextId = 0;
        ImgMode.ConfigId  = 0;

    #ifdef CONFIG_SOC_A12
        if (AlgoMode == AMBA_DSP_IMG_ALGO_MODE_LISO) {
            /* In A12, MISO is regard as 3-pass LISO. We need to raise certain flag for stillAdj in order
             * to get correct still_process_mode for SSP.
             */
            if (Force3PassLiso == 1) {
                UINT32 Addr, i;
                ADJ_STILL_FAST_LISO_PARAM_S *AmbaAdjStillLIsoTmp;
                AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD, ImgIpChNo, IQ_PARAMS_STILL_LISO_ADJ, (UINT32)&Addr);
                AmbaAdjStillLIsoTmp = (ADJ_STILL_FAST_LISO_PARAM_S *)Addr;
                for(i = 0;i<ADJ_NF_TABLE_COUNT;i++){
                    AmbaAdjStillLIsoTmp->LiProcessingSelect[i].Value[0] = 1; //0:1-Pass, 1:3-Passes, 2:2-passes
                }
            } else if (Force3PassLiso == 0) {
                UINT32 Addr, i;
                ADJ_STILL_FAST_LISO_PARAM_S *AmbaAdjStillLIsoTmp;
                AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_PARAMS_ADD, ImgIpChNo, IQ_PARAMS_STILL_LISO_ADJ, (UINT32)&Addr);
                AmbaAdjStillLIsoTmp = (ADJ_STILL_FAST_LISO_PARAM_S *)Addr;
                for(i = 0;i<ADJ_NF_TABLE_COUNT;i++){
                    AmbaAdjStillLIsoTmp->LiProcessingSelect[i].Value[0] = 0; //0:1-Pass, 1:3-Passes, 2:2-passes
                }
            }
        }
    #endif
        AmbaSample_AdjStillControl(ImgIpChNo, aeIdx, &ImgMode, G_iso);
    }

    AmpUT_SetStillWB(ImgIpChNo, &ImgMode);
    AmpUT_SetStillSensorInfo(&ImgMode);

    return 0;
}

/**
 * UnitTest: Setup still Idsp parameters before doing any still process
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_StillEncIdspParamSetup(UINT8 aeIdx, UINT16 rawWidth, UINT16 rawHeight)
{
    UINT8 ArIndex = 0;
    UINT16 CapW = 0, CapH = 0, EncW = 0, EncH = 0, OffsetX = 0, OffsetY = 0;

    AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
    AMBA_DSP_IMG_SIZE_INFO_s SizeInfo = {0};
    AMBA_DSP_IMG_WARP_CALC_INFO_s CalcWarp = {0};
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s VinGeo = {0};
    AMBA_DSP_IMG_OUT_WIN_INFO_s ImgOutputWin = {0};
    AMBA_DSP_IMG_DZOOM_INFO_s ImgDzoom = {0};
    AMP_IMG_DZOOM_STATUS_s ImgDzoomStatus = {0};
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};
    AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
    AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
    AMBA_DSP_IMG_SENSOR_FRAME_TIMING_s ImgSensorFrameTiming;
    AMBA_SENSOR_AREA_INFO_s *RecPixel = &SensorStatus.ModeInfo.OutputInfo.RecordingPixels;
    AMBA_SENSOR_INPUT_INFO_s *InputInfo = &SensorStatus.ModeInfo.InputInfo;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /* Still Adj control */
    AmpUT_StillEncControlStillAdj(aeIdx);

    ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
    if (G_iso == 0) {
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
        ImgMode.BatchId = AMBA_DSP_STILL_HISO_FILTER;
    } else if (G_iso == 1) {
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
        ImgMode.BatchId = AMBA_DSP_STILL_LISO_FILTER;
    } else {
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
        ImgMode.BatchId = AMBA_DSP_VIDEO_FILTER;
    }
    ImgMode.ContextId = 0;
    ImgMode.ConfigId  = 0;

    SensorMode.Data = StillEncMgt[StillEncModeIdx].InputStillMode;
    AmbaSensor_GetStatus(EncChannel, &SensorStatus);
    AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);

    CapW = rawWidth;
    CapH = rawHeight;
#ifdef CONFIG_SOC_A12
    AmpUT_SetStillActiveVinWindow(&ImgMode);
#endif

    EncW = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[StillEncModeIdx].StillMainWidth;
    EncH = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[StillEncModeIdx].StillMainHeight;

    if ((CapW > RecPixel->Width) || (CapH > RecPixel->Height)) { // Vin capture window size > sensor recording region means OB is enable
        OffsetX = (((SensorStatus.ModeInfo.OutputInfo.OutputWidth - CapW)/2)&0xFFF8)* \
                InputInfo->HSubsample.FactorDen/InputInfo->HSubsample.FactorNum;
        OffsetY = (((SensorStatus.ModeInfo.OutputInfo.OutputHeight - CapH)/2)&0xFFFE)* \
                InputInfo->HSubsample.FactorDen/InputInfo->HSubsample.FactorNum;
        VinGeo.StartX = (InputInfo->PhotodiodeArray.StartX - RecPixel->StartX) + OffsetX;
        VinGeo.StartY = (InputInfo->PhotodiodeArray.StartY - RecPixel->StartY) + OffsetY;
    } else {
        OffsetX = (((RecPixel->Width - CapW)/2)&0xFFF8)*InputInfo->HSubsample.FactorDen/InputInfo->HSubsample.FactorNum;
        OffsetY = (((RecPixel->Height - CapH)/2)&0xFFFE)*InputInfo->HSubsample.FactorDen/InputInfo->HSubsample.FactorNum;
        VinGeo.StartX = InputInfo->PhotodiodeArray.StartX + OffsetX;
        VinGeo.StartY = InputInfo->PhotodiodeArray.StartY + OffsetY;
    }
    VinGeo.Width = CapW;
    VinGeo.Height = CapH;
    VinGeo.HSubSample.FactorDen = InputInfo->HSubsample.FactorDen;
    VinGeo.HSubSample.FactorNum = InputInfo->HSubsample.FactorNum;
    VinGeo.VSubSample.FactorDen = InputInfo->VSubsample.FactorDen;
    VinGeo.VSubSample.FactorNum = InputInfo->VSubsample.FactorNum;
    AmbaDSP_WarpCore_SetVinSensorGeo(&ImgMode, &VinGeo);

    ImgSensorFrameTiming.TimeScale = SensorStatus.ModeInfo.FrameTime.FrameRate.TimeScale;
    ImgSensorFrameTiming.NumUnitsInTick = SensorStatus.ModeInfo.FrameTime.FrameRate.NumUnitsInTick;
    ImgSensorFrameTiming.FrameLengthLines = SensorStatus.ModeInfo.FrameLengthLines;
    AmbaDSP_WarpCore_SetSensorFrameTiming(&ImgMode, &ImgSensorFrameTiming);

    ImgOutputWin.MainWinDim.Width  = EncW;
    ImgOutputWin.MainWinDim.Height = EncH;
#ifdef CONFIG_SOC_A12
    ImgOutputWin.ScreennailDim.Width  = StillEncMgt[StillEncModeIdx].ScrnWidthAct;
    ImgOutputWin.ScreennailDim.Height = StillEncMgt[StillEncModeIdx].ScrnHeightAct;
    ImgOutputWin.ThumbnailDim.Width  = StillEncMgt[StillEncModeIdx].ThmWidthAct;
    ImgOutputWin.ThumbnailDim.Height = StillEncMgt[StillEncModeIdx].ThmHeightAct;
    ImgOutputWin.PrevWinDim[0].Width  = StillVoutMgt[0][ArIndex].Width;
    ImgOutputWin.PrevWinDim[0].Height = StillVoutMgt[0][ArIndex].Height;
    ImgOutputWin.PrevWinDim[1].Width  = StillVoutMgt[1][ArIndex].Width;
    ImgOutputWin.PrevWinDim[1].Height = StillVoutMgt[1][ArIndex].Height;
#endif
    AmbaDSP_WarpCore_SetOutputWin(&ImgMode, &ImgOutputWin);

    AmpImgDzoom_GetDzoomStatus(StillDzoomHdlr, &ImgDzoomStatus);

    ImgDzoom.ZoomX = ImgDzoomStatus.Factor;
    ImgDzoom.ZoomY = ImgDzoomStatus.Factor;
    ImgDzoom.ShiftX = ImgDzoomStatus.ShiftX;
    ImgDzoom.ShiftY = ImgDzoomStatus.ShiftY;
    AmbaDSP_WarpCore_SetDzoomFactor(&ImgMode, &ImgDzoom);

    AmbaDSP_WarpCore_CalcDspWarp(&ImgMode, 0);
    AmbaDSP_WarpCore_CalcDspCawarp(&ImgMode, 0);

    AmbaDSP_WarpCore_SetDspWarp(&ImgMode);
    AmbaDSP_WarpCore_SetDspCawarp(&ImgMode);

    if (AmbaDSP_ImgGetWarpCompensation(&ImgMode, &CalcWarp) != OK)
        AmbaPrintColor(RED, "%s: Get Warp Compensation fail!!", __func__);

    SizeInfo.WidthIn = ((CalcWarp.ActWinCrop.RightBotX - CalcWarp.ActWinCrop.LeftTopX + 0xFFFF)>>16);
    SizeInfo.HeightIn = ((CalcWarp.ActWinCrop.RightBotY - CalcWarp.ActWinCrop.LeftTopY + 0xFFFF)>>16);
    SizeInfo.WidthMain = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[StillEncModeIdx].StillMainWidth;
    SizeInfo.HeightMain = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[StillEncModeIdx].StillMainHeight;
    SizeInfo.WidthPrevA = StillVoutMgt[0][ArIndex].Width;
    SizeInfo.HeightPrevA = StillVoutMgt[0][ArIndex].Height;
    SizeInfo.WidthPrevB = StillVoutMgt[1][ArIndex].Width;
    SizeInfo.HeightPrevB = StillVoutMgt[1][ArIndex].Height;
    SizeInfo.WidthScrn = StillEncMgt[StillEncModeIdx].ScrnWidthAct;
    SizeInfo.HeightScrn = StillEncMgt[StillEncModeIdx].ScrnHeightAct;

    AmbaDSP_ImgSetSizeInfo(&ImgMode, &SizeInfo);
    AmbaDSP_ImgPostExeCfg(&ImgMode, (AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e)0);
    AmbaPrint("Still Idsp parameters setup done!!!");

    return 0;
}

/**
 * UnitTest: Still post IDSP parameters setup should be invoke after post WB calculation
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_StillEncPostIdspParamSetup(UINT8 aeIdx)
{
    UINT32 ImgChipNo = 0;
    AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
    AMBA_AE_INFO_s StillAeInfo[MAX_AEB_NUM];
    AMBA_DSP_IMG_WB_GAIN_s WbGain = {0};

    ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
    if (G_iso == 0) {
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
        ImgMode.BatchId = AMBA_DSP_STILL_HISO_FILTER;
    } else if (G_iso == 1) {
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
        ImgMode.BatchId = AMBA_DSP_STILL_LISO_FILTER;
    } else {
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
        ImgMode.BatchId = AMBA_DSP_VIDEO_FILTER;
    }
    ImgMode.ContextId = 0;
    ImgMode.ConfigId = 0;
    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, ImgChipNo, IP_MODE_STILL, (UINT32)StillAeInfo);
    AmbaImg_Proc_Cmd(MW_IP_GET_WB_GAIN, ImgChipNo, (UINT32)&WbGain, 0);

#ifdef CONFIG_SOC_A12
    if (0/*1 == StillOBModeEnable*/) { // After MW_IP_COMPUTE_STILL_OB, 3A need to compute adj again to apply new black level.
        AmbaSample_AdjStillControl(ImgChipNo, aeIdx, &ImgMode, G_iso);
    }
#endif

    if (0 == StillOBModeEnable) { //TBD
        AmbaImg_Proc_Cmd(MW_IP_SET_STILL_PIPE_CTRL_PARAMS, ImgChipNo, (UINT32)&ImgMode , 0);
    }

    AmpUT_SetStillWB(ImgChipNo, &ImgMode);
    AmpUT_SetStillSensorInfo(&ImgMode);
    AmbaDSP_ImgPostExeCfg(&ImgMode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY);
    AmbaPrint("[UT_StillEnc_PostIdspParamSetup] Done!!!");
    return OK;
}

/**
 * UnitTest: Calculate dsp working address and size base on current capture mode
 *
 * @param [in] return dsp reserved working area address
 * @param [in] return dsp reserved working area size
 *
 * @return 0 - success, -1 - fail
 */
static UINT32 AmpUT_StillEnc_DspWork_Calculate(UINT8 **addr, UINT32 *size)
{
    extern UINT8 *DspWorkAreaResvStart;
    extern UINT32 DspWorkAreaResvSize;
    int rval = 0;
    UINT32 TotalSize = 0, MaxResvSize = 0;
    UINT8 ArIndex = 0;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    MaxResvSize = DspWorkAreaResvSize - (130<<20);

    if (CaptureMode == STILL_CAPTURE_NONE) {
        //LiveView only
        (*addr) = DspWorkAreaResvStart;
        (*size) = DspWorkAreaResvSize;
    } else if (CaptureMode == STILL_CAPTURE_YUV2JPG) {
        //allocate YUV buffer
        UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
        UINT32 YuvSize = 0, ScrnSize = 0, ThmSize = 0;

        //FastMode need 16_align enc_height
        if (G_iso == 2) {
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ScrnSize = ScrnW*ScrnH*2;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
            ThmSize = ThmW*ThmH*2;
        } else {
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            YuvSize += (YuvSize*10)/100;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ScrnSize = ScrnW*ScrnH*2;
            ScrnSize += (ScrnSize*10)/100;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
            ThmSize = ThmW*ThmH*2;
            ThmSize += (ThmSize*10)/100;
        }

        TotalSize = YuvSize*1 + ScrnSize*1 + ThmSize*1;
        if (TotalSize > MaxResvSize) {
            AmbaPrint("[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, MaxResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            rval = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }
    } else if (CaptureMode == STILL_CAPTURE_SINGLE_SHOT || \
                CaptureMode == STILL_CAPTURE_RAW2YUV) {
        //allocate (Raw + YUV) buffer
        UINT16 RawPitch = 0, RawHeight = 0, RawWidth = 0;
        UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
        UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
        UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
        UINT32 QvLCDSize = 0, QvHDMISize = 0;
        UINT32 RoiSize = 0, Raw3ASize = 0;

    #ifdef CONFIG_SOC_A12
        if (1 == StillOBModeEnable) {
            AMBA_SENSOR_MODE_ID_u SensorMode = {0};
            AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
            SensorMode.Data = StillEncMgt[StillEncModeIdx].InputStillMode;
            AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);
            RawWidth = SensorModeInfo.OutputInfo.OutputWidth;
            RawHeight = SensorModeInfo.OutputInfo.OutputHeight;

            RoiSize = TileNumber*sizeof(AMP_STILLENC_RAW2RAW_ROI_s);
            Raw3ASize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
        } else {
            RawWidth =  StillEncMgt[StillEncModeIdx].StillCaptureWidth;
            RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        }
    #else
        RawWidth =  StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
    #endif
        RawPitch = ALIGN_32((G_raw_cmpr) ? AMP_COMPRESSED_RAW_WIDTH(RawWidth) : RawWidth*2);
        RawSize = RawPitch*RawHeight;

        //FastMode need 16_align enc_height
        if (G_iso == 2) {
        #ifdef CONFIG_SOC_A12
            UINT16 OriRawWidth = 0;
            YuvWidth = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[StillEncModeIdx].StillMainHeight;
            OriRawWidth = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, OriRawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32((StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16((StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ThmSize = ThmW*ThmH*2;
        } else {
        #ifdef CONFIG_SOC_A12
            YuvWidth = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32((StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16((StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            YuvSize += (YuvSize*10)/100;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ScrnSize += (ScrnSize*10)/100;
            ThmSize = ThmW*ThmH*2;
            ThmSize += (ThmSize*10)/100;
        }

        /* QV need 16_Align */
    #ifdef CONFIG_SOC_A12
        QvLCDW = StillVoutMgt[0][ArIndex].Width;
        QvLCDH = StillVoutMgt[0][ArIndex].Height;
        QvHDMIW = StillVoutMgt[1][ArIndex].Width;
        QvHDMIH = StillVoutMgt[1][ArIndex].Height;
    #else
        QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
        QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
        QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
        QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
    #endif
        QvLCDSize = QvLCDW*QvLCDH*2;
        QvLCDSize += (QvLCDSize*10)/100;
        QvHDMISize = QvHDMIW*QvHDMIH*2;
        QvHDMISize += (QvHDMISize*10)/100;

        if (CaptureMode == STILL_CAPTURE_RAW2YUV) QvLCDSize = QvHDMISize = 0;

        TotalSize = RawSize*1 + YuvSize*1 + ScrnSize*1 + ThmSize*1 + QvLCDSize*1 + QvHDMISize*1 + RoiSize + Raw3ASize;
        if (TotalSize > MaxResvSize) {
            AmbaPrint("[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, MaxResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            rval = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }
    } else if (CaptureMode == STILL_CAPTURE_BURST_SHOT) {
        //allocate (Raw*CapCnt + YUV) buffer
        UINT16 RawPitch = 0, RawHeight = 0, RawWidth = 0;
        UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
        UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
        UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
        UINT32 QvLCDSize = 0, QvHDMISize = 0;

        RawWidth = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        RawPitch = ALIGN_32((G_raw_cmpr) ? AMP_COMPRESSED_RAW_WIDTH(RawWidth) : RawWidth*2);
        RawSize = RawPitch*RawHeight;

        //FastMode need 16_align enc_height
        if (G_iso == 2) {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ThmSize = ThmW*ThmH*2;
        } else {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            YuvSize += (YuvSize*10)/100;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ScrnSize += (ScrnSize*10)/100;
            ThmSize = ThmW*ThmH*2;
            ThmSize += (ThmSize*10)/100;
        }
    #ifdef CONFIG_SOC_A12
        QvLCDW = StillVoutMgt[0][ArIndex].Width;
        QvLCDH = StillVoutMgt[0][ArIndex].Height;
        QvHDMIW = StillVoutMgt[1][ArIndex].Width;
        QvHDMIH = StillVoutMgt[1][ArIndex].Height;
    #else
        /* QV need 16_Align */
        QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
        QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
        QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
        QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
    #endif
        QvLCDSize = QvLCDW*QvLCDH*2;
        QvLCDSize += (QvLCDSize*10)/100;
        QvHDMISize = QvHDMIW*QvHDMIH*2;
        QvHDMISize += (QvHDMISize*10)/100;

        TotalSize = RawSize*G_capcnt + YuvSize*1 + ScrnSize*1 + ThmSize*1 + QvLCDSize*1 + QvHDMISize*1;
        if (TotalSize > MaxResvSize) {
            AmbaPrint("[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, MaxResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            rval = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }
    } else if (CaptureMode == STILL_CAPTURE_SINGLE_SHOT_CONT) {
        //allocate (Raw + Cfa + YUV) buffer
        //allocate (Raw + YUV) buffer
        UINT16 RawPitch = 0, RawHeight = 0, RawWidth =0;
        UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
        UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
        UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
        UINT32 QvLCDSize = 0, QvHDMISize = 0;
        UINT32 RoiSize = 0, Raw3AStatSize = 0;

        RawWidth = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        RawPitch = ALIGN_32((G_raw_cmpr) ? AMP_COMPRESSED_RAW_WIDTH(RawWidth) : RawWidth*2);
        RawSize = RawPitch*RawHeight;

        //FastMode need 16_align enc_height
        if (G_iso == 2) {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ThmSize = ThmW*ThmH*2;
        } else {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            YuvSize += (YuvSize*10)/100;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ScrnSize += (ScrnSize*10)/100;
            ThmSize = ThmW*ThmH*2;
            ThmSize += (ThmSize*10)/100;
        }
    #ifdef CONFIG_SOC_A12
        QvLCDW = StillVoutMgt[0][ArIndex].Width;
        QvLCDH = StillVoutMgt[0][ArIndex].Height;
        QvHDMIW = StillVoutMgt[1][ArIndex].Width;
        QvHDMIH = StillVoutMgt[1][ArIndex].Height;
    #else
        /* QV need 16_Align */
        QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
        QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
        QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
        QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
    #endif
        QvLCDSize = QvLCDW*QvLCDH*2;
        QvLCDSize += (QvLCDSize*10)/100;
        QvHDMISize = QvHDMIW*QvHDMIH*2;
        QvHDMISize += (QvHDMISize*10)/100;

        /* One tile need one RoiSize */
        RoiSize = TileNumber*sizeof(AMP_STILLENC_RAW2RAW_ROI_s);
        Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);

        TotalSize = RawSize*1 + YuvSize*1 + ScrnSize*1 + ThmSize*1 + QvLCDSize*1 + QvHDMISize*1 + RoiSize*TileNumber + Raw3AStatSize;
        if (TotalSize > MaxResvSize) {
            AmbaPrint("[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, MaxResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            rval = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }
    } else if (CaptureMode == STILL_CAPTURE_BURST_SHOT_CONT) {
        //allocate (Raw*6 + YUV) buffer
        UINT16 RawPitch = 0, RawHeight = 0, RawWidth = 0;
        UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
        UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
        UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
        UINT32 QvLCDSize = 0, QvHDMISize = 0;

        RawWidth = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        RawPitch = ALIGN_32((G_raw_cmpr)? AMP_COMPRESSED_RAW_WIDTH(RawWidth): RawWidth*2);
        RawSize = RawPitch*RawHeight;

        //FastMode need 16_align enc_height
        if (G_iso == 2) {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ThmSize = ThmW*ThmH*2;
        } else {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            YuvSize += (YuvSize*10)/100;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ScrnSize += (ScrnSize*10)/100;
            ThmSize = ThmW*ThmH*2;
            ThmSize += (ThmSize*10)/100;
        }

    #ifdef CONFIG_SOC_A12
        QvLCDW = StillVoutMgt[0][ArIndex].Width;
        QvLCDH = StillVoutMgt[0][ArIndex].Height;
        QvHDMIW = StillVoutMgt[1][ArIndex].Width;
        QvHDMIH = StillVoutMgt[1][ArIndex].Height;
    #else
        /* QV need 16_Align */
        QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
        QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
        QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
        QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
    #endif
        QvLCDSize = QvLCDW*QvLCDH*2;
        QvLCDSize += (QvLCDSize*10)/100;
        QvHDMISize = QvHDMIW*QvHDMIH*2;
        QvHDMISize += (QvHDMISize*10)/100;

        TotalSize = RawSize*6 + YuvSize*1 + ScrnSize*1 + ThmSize*1 + QvLCDSize*1 + QvHDMISize*1;
        if (TotalSize > MaxResvSize) {
            AmbaPrint("[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, MaxResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            rval = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }
    } else if (CaptureMode == STILL_CAPTURE_BURST_SHOT_CONT_WB) {
        //allocate (Raw*6 + YUV) buffer
        UINT16 RawPitch = 0, RawHeight = 0, RawWidth = 0;
        UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
        UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
        UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
        UINT32 QvLCDSize = 0, QvHDMISize = 0;
        UINT32 RoiSize = 0, Raw3AStatSize = 0;

        RawWidth =  StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        RawPitch = ALIGN_32((G_raw_cmpr) ? AMP_COMPRESSED_RAW_WIDTH(RawWidth) : RawWidth*2);
        RawSize = RawPitch*RawHeight;

        //FastMode need 16_align enc_height
        if (G_iso == 2) {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ThmSize = ThmW*ThmH*2;
        } else {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            YuvSize += (YuvSize*10)/100;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ScrnSize += (ScrnSize*10)/100;
            ThmSize = ThmW*ThmH*2;
            ThmSize += (ThmSize*10)/100;
        }

    #ifdef CONFIG_SOC_A12
        QvLCDW = StillVoutMgt[0][ArIndex].Width;
        QvLCDH = StillVoutMgt[0][ArIndex].Height;
        QvHDMIW = StillVoutMgt[1][ArIndex].Width;
        QvHDMIH = StillVoutMgt[1][ArIndex].Height;
    #else
        /* QV need 16_Align */
        QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
        QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
        QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
        QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
    #endif
        QvLCDSize = QvLCDW*QvLCDH*2;
        QvLCDSize += (QvLCDSize*10)/100;
        QvHDMISize = QvHDMIW*QvHDMIH*2;
        QvHDMISize += (QvHDMISize*10)/100;

        /* One tile need one RoiSize */
        RoiSize = TileNumber*sizeof(AMP_STILLENC_RAW2RAW_ROI_s);
        Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);

        TotalSize = RawSize*6 + YuvSize*1 + ScrnSize*1 + ThmSize*1 + QvLCDSize*1 + QvHDMISize*1 + RoiSize*TileNumber + Raw3AStatSize;
        if (TotalSize > MaxResvSize) {
            AmbaPrint("[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, MaxResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            rval = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }
    } else if (CaptureMode == STILL_CAPTURE_AEB) {
        //allocate (Raw*3 + YUV) buffer
        UINT16 RawPitch = 0, RawHeight = 0, RawWidth = 0;
        UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
        UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
        UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
        UINT32 QvLCDSize = 0, QvHDMISize = 0;

        RawWidth =  StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        RawPitch = ALIGN_32((G_raw_cmpr) ? AMP_COMPRESSED_RAW_WIDTH(RawWidth) : RawWidth*2);
        RawSize = RawPitch*RawHeight;

        //FastMode need 16_align enc_height
        if (G_iso == 2) {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ThmSize = ThmW*ThmH*2;
        } else {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            YuvSize += (YuvSize*10)/100;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ScrnSize += (ScrnSize*10)/100;
            ThmSize = ThmW*ThmH*2;
            ThmSize += (ThmSize*10)/100;
        }

    #ifdef CONFIG_SOC_A12
        QvLCDW = StillVoutMgt[0][ArIndex].Width;
        QvLCDH = StillVoutMgt[0][ArIndex].Height;
        QvHDMIW = StillVoutMgt[1][ArIndex].Width;
        QvHDMIH = StillVoutMgt[1][ArIndex].Height;
    #else
        /* QV need 16_Align */
        QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
        QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
        QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
        QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
    #endif
        QvLCDSize = QvLCDW*QvLCDH*2;
        QvLCDSize += (QvLCDSize*10)/100;
        QvHDMISize = QvHDMIW*QvHDMIH*2;
        QvHDMISize += (QvHDMISize*10)/100;

        TotalSize = RawSize*3+ YuvSize*1 + ScrnSize*1 + ThmSize*1 + QvLCDSize*1 + QvHDMISize*1;
        if (TotalSize > MaxResvSize) {
            AmbaPrint("[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, MaxResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            rval = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }
    } else if (CaptureMode == STILL_CAPTURE_PRE_CAPTURE) {
        //allocate (Raw*5 + YUV) buffer
        UINT16 RawPitch = 0, RawHeight = 0, RawWidth = 0;
        UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
        UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
        UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
        UINT32 QvLCDSize = 0, QvHDMISize = 0;

        RawWidth =  StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        RawPitch = ALIGN_32((G_raw_cmpr) ? AMP_COMPRESSED_RAW_WIDTH(RawWidth) : RawWidth*2);
        RawSize = RawPitch*RawHeight;

        //FastMode need 16_align enc_height
        if (G_iso == 2) {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ThmSize = ThmW*ThmH*2;
        } else {
        #ifdef CONFIG_SOC_A12
            YuvWidth = StillEncMgt[StillEncModeIdx].StillMainWidth;
            YuvHeight = StillEncMgt[StillEncModeIdx].StillMainHeight;
            AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = StillEncMgt[StillEncModeIdx].ScrnHeight;
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = StillEncMgt[StillEncModeIdx].ThmHeight;
        #else
            //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
            YuvWidth = ALIGN_32(StillEncMgt[StillEncModeIdx].StillMainWidth);
            YuvHeight = ALIGN_16(StillEncMgt[StillEncModeIdx].StillMainHeight);
            YuvSize = YuvWidth*YuvHeight*2;
            YuvSize += (YuvSize*10)/100;
            ScrnW = ALIGN_32(StillEncMgt[StillEncModeIdx].ScrnWidth);
            ScrnH = ALIGN_16(StillEncMgt[StillEncModeIdx].ScrnHeight);
            ThmW = ALIGN_32(StillEncMgt[StillEncModeIdx].ThmWidth);
            ThmH = ALIGN_16(StillEncMgt[StillEncModeIdx].ThmHeight);
        #endif
            ScrnSize = ScrnW*ScrnH*2;
            ScrnSize += (ScrnSize*10)/100;
            ThmSize = ThmW*ThmH*2;
            ThmSize += (ThmSize*10)/100;
        }

    #ifdef CONFIG_SOC_A12
        QvLCDW = StillVoutMgt[0][ArIndex].Width;
        QvLCDH = StillVoutMgt[0][ArIndex].Height;
        QvHDMIW = StillVoutMgt[1][ArIndex].Width;
        QvHDMIH = StillVoutMgt[1][ArIndex].Height;
    #else
        /* QV need 16_Align */
        QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
        QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
        QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
        QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
    #endif
        QvLCDSize = QvLCDW*QvLCDH*2;
        QvLCDSize += (QvLCDSize*10)/100;
        QvHDMISize = QvHDMIW*QvHDMIH*2;
        QvHDMISize += (QvHDMISize*10)/100;

        TotalSize = RawSize*5+ YuvSize*1 + ScrnSize*1 + ThmSize*1 + QvLCDSize*1 + QvHDMISize*1;
        if (TotalSize > MaxResvSize) {
            AmbaPrint("[DspWork_Calculate] Memory shortage (%u %u)", TotalSize, MaxResvSize);
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize;
            rval = -1;
        } else {
            (*addr) = DspWorkAreaResvStart;
            (*size) = DspWorkAreaResvSize - TotalSize;
        }

    }

    if (StillEncCusomtDspWorkSize) {
        *size = StillEncCusomtDspWorkSize<<20;
    }

    AmbaPrint("[DspWork_Calculate] Addr 0x%X, Sz %u, req %d", *addr, *size, TotalSize);
    return rval;
}

/* qv timer handler */
static void AmpUT_qvTimer_Handler(UINT32 exinf)
{
    STILLENC_UT_MSG_S msg;

    /* TimeUP!! */
    msg.Event = MSG_STILLENC_UT_QV_TIMER_TIMEOUT;
    msg.Data.DataInfo[0] = exinf; //TimerID
    AmbaKAL_MsgQueueSend(&UtMsgQueue, &msg, AMBA_KAL_NO_WAIT);
}

/* Continuous single capture timer handler */
static void AmpUT_ScapCTimer_Handler(UINT32 scapCont)
{
    STILLENC_UT_MSG_S msg;

    /* TimeUP!! */
    msg.Event = MSG_STILLENC_UT_SCAPC_TIMER_TRIGGER_START;
    msg.Data.DataInfo[0] = scapCont; // scapCont capture parameters
    AmbaPrint("[scapCTmr_Hdlr]Snd 0x%X (%d)!", msg.Event, msg.Data.DataInfo[0]);
    AmbaKAL_MsgQueueSend(&UtMsgQueue, &msg, AMBA_KAL_NO_WAIT);
}

/* Timer handler that use to simulate button release */
static void AmpUT_CaptureButtonTimer_Handler(UINT32 scriptID)
{
    /* TimeUP!! stop running script */
    UINT32 stopRule;

    switch (CaptureMode) {
    case STILL_CAPTURE_BURST_SHOT_CONT:
    case STILL_CAPTURE_BURST_SHOT_CONT_WB:
        stopRule = AMP_ENC_SCRPT_STOP_CAPTURE_RAW;
        AmpEnc_StopRunScript(StillEncPipe, stopRule, scriptID);
        break;
    case STILL_CAPTURE_PRE_CAPTURE:
        stopRule = AMP_ENC_SCRPT_STOP_PRE_CAPTURE;
        AmpEnc_StopRunScript(StillEncPipe, stopRule, scriptID);
        break;
    default:
        break;
    }
    CaptureButtonPressed = 0;
}

/* Timer handler that use to simulate app inform force stop script running */
static void AmpUT_ForceStopTimer_Handler(UINT32 scriptID)
{
    /* TimeUP!! stop running script */
    UINT32 stopRule = AMP_ENC_SCRPT_STOP_RIGHT_AWAY;
    AmpEnc_StopRunScript(StillEncPipe, stopRule, scriptID);
}

/**
 * UnitTest: QuickView show buffer allocation
 *
 * @param [in] LCD qv buffer size
 * @param [in] HDMI qv buffer size
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_qvShowBufferAllocate(UINT32 qvLCDSize, UINT32 qvHDMISize)
{
    int er = NG;
    void *tempPtr = NULL;
    void *TempRawPtr = NULL;

    if (QvLcdShowBuffAddrOdd == NULL) {
        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvLCDSize, 32);
        if (er != OK) {
            AmbaPrint("Cache alloc yuv_lcd_odd fail (%u)!", qvLCDSize);
        } else {
            QvLcdShowBuffAddrOdd = (UINT8*)tempPtr;
            AmbaPrint("qvLCDBuffaddrOdd (0x%08X)!", QvLcdShowBuffAddrOdd);
        }
    }
    if (QvLcdShowBuffAddrEven == NULL) {
        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvLCDSize, 32);
        if (er != OK) {
            AmbaPrint("Cache alloc yuv_lcd_even fail (%u)!", qvLCDSize);
        } else {
            QvLcdShowBuffAddrEven = (UINT8*)tempPtr;
            AmbaPrint("qvLCDBuffaddrEven (0x%08X)!", QvLcdShowBuffAddrEven);
        }
    }

    if (QvTvShowBuffAddrOdd == NULL) {
        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvHDMISize, 32);
        if (er != OK) {
            AmbaPrint("Cache alloc yuv_tv_odd fail (%u)!", qvHDMISize);
        } else {
            QvTvShowBuffAddrOdd = (UINT8*)tempPtr;
            AmbaPrint("qvTVBuffaddrOdd (0x%08X)!", QvTvShowBuffAddrOdd);
        }
    }
    if (QvTvShowBuffAddrEven == NULL) {
        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvHDMISize, 32);
        if (er != OK) {
            AmbaPrint("Cache alloc yuv_tv_even fail (%u)!", qvHDMISize);
        } else {
            QvTvShowBuffAddrEven = (UINT8*)tempPtr;
            AmbaPrint("qvTVBuffaddrEvev (0x%08X)!", QvTvShowBuffAddrEven);
        }
    }

    return er;
}

/**
 * UnitTest: Raw2yuv PreProc Callback
 * Stage1: YUV2YUV  -> Nothing to do
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_PBCapturePreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    // Nothing to do
    return 0;
}

/**
 * UnitTest: PB capture PostProc Callback
 * Stage1: YUV2YUV  -> Dump yuv
 * @param [in] info postproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_PBCapturePostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    static UINT8 yuvFlag = 0;

    if (info->StageCnt == 1) {
        char fn[MAX_FILENAME_LENGTH];
        char fn1[MAX_FILENAME_LENGTH];
        UINT8 *lumaAddr, *chromaAddr;
        UINT16 pitch = 0, width = 0, height = 0;
        AMP_CFS_FILE_s *y = NULL;
        AMP_CFS_FILE_s *uv = NULL;

        char mdASCII[3] = {'w','+','\0'};
        UINT8 dumpSkip = 0;

        if (1/* TBD */) {
            dumpSkip = 1;
        }

        if (dumpSkip == 0) {
            AMP_AREA_s Aoi;
            Aoi.X = Aoi.Y = 0;
            if (info->media.YuvInfo.ThmLumaAddr) {
                sprintf(fn, "%s:\\%04d_t.y", DefaultSlot, YuvFno);
                sprintf(fn1, "%s:\\%04d_t.uv", DefaultSlot, YuvFno);
                lumaAddr = info->media.YuvInfo.ThmLumaAddr;
                chromaAddr = info->media.YuvInfo.ThmChromaAddr;
                pitch = info->media.YuvInfo.ThmPitch;
                width = info->media.YuvInfo.ThmWidth;
                height = info->media.YuvInfo.ThmHeight;
            } else if (info->media.YuvInfo.ScrnLumaAddr) {
                sprintf(fn, "%s:\\%04d_s.y", DefaultSlot, YuvFno);
                sprintf(fn1, "%s:\\%04d_s.uv", DefaultSlot, YuvFno);
                lumaAddr = info->media.YuvInfo.ScrnLumaAddr;
                chromaAddr = info->media.YuvInfo.ScrnChromaAddr;
                pitch = info->media.YuvInfo.ScrnPitch;
                width = info->media.YuvInfo.ScrnWidth;
                height = info->media.YuvInfo.ScrnHeight;
            } else if (info->media.YuvInfo.LumaAddr) {
                sprintf(fn, "%s:\\%04d_m.y", DefaultSlot, YuvFno);
                sprintf(fn1, "%s:\\%04d_m.uv", DefaultSlot, YuvFno);
                lumaAddr = info->media.YuvInfo.LumaAddr;
                chromaAddr = info->media.YuvInfo.ChromaAddr;
                pitch = info->media.YuvInfo.Pitch;
                width = info->media.YuvInfo.Width;
                height = info->media.YuvInfo.Height;
            }

            Aoi.Width = width;
            Aoi.Height = height;
            AmbaPrint("[Amp_UT] Dump YUV(%s) (0x%X 0x%X) %d %d %d Start!", \
                (info->media.YuvInfo.DataFormat)?"422":"420", \
                lumaAddr, chromaAddr, pitch, width, height);
            if (!(StillDumpSkip & STILL_ENC_SKIP_YUV)) {
                y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
                AmpUT_DumpAoi(lumaAddr, pitch, Aoi, (void *)y);
                UT_StillEncodefsync((void *)y);
                UT_StillEncodefclose((void *)y);

                uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
                AmpUT_DumpAoi(chromaAddr, pitch, Aoi, (void *)uv);
                UT_StillEncodefsync((void *)uv);
                UT_StillEncodefclose((void *)uv);
            }

            //release yuv buffers
            if (info->media.YuvInfo.ThmLumaAddr) {
                yuvFlag |= 0x4;
            } else if (info->media.YuvInfo.ScrnLumaAddr) {
                yuvFlag |= 0x2;
            } else if (info->media.YuvInfo.LumaAddr) {
                yuvFlag |= 0x1;
            }
        } else {
            yuvFlag |= 0x6;
        }

        if (yuvFlag == 0x6) {
            YuvFno++;
        }
    } else {
        //nothing to do
    }

    return 0;
}

/**
 * UnitTest: simple PB capture(ie yuv2yuv for scrn/thm + yuv2jpeg)
 *
 * @param [in] pivCtrl PIV control information
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_StillEnc_PBCapture(AMP_STILLENC_PB_CAPTURE_CTRL_s PBCapCtrl)
{
    int er;
    void *tempPtr;
    void *TempRawPtr;
    UINT32 yuvSize = 0, scrnSize = 0, thmSize = 0;
    UINT8 *stageAddr = NULL;
    UINT32 totalScriptSize = 0, totalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *genScrpt;
    AMP_SENC_SCRPT_YUV2YUV_s *Yuv2YuvScrpt;
    AMP_SENC_SCRPT_YUV2JPG_s *yuv2JpgScrpt;
    AMP_SCRPT_CONFIG_s scrpt;

    /* Phase I */
    /* check still codec status */
    if (StillCodecInit == 0) { //not init yet
        int er = OK;

        if (StillCodecInit) {
            AmbaPrint("StillEnc already init");
            return er;
        }

        // Create simple muxer semophore
        if (AmbaKAL_SemCreate(&StillEncSem, 0) != OK) {
            AmbaPrint("StillEnc UnitTest: Semaphore creation failed");
        }

        // Init VIN module
        if (StillVinWorkBuf == NULL) {
            AMP_VIN_INIT_CFG_s vinInitCfg;

            AmpVin_GetInitDefaultCfg(&vinInitCfg);
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillVinWorkBuf, (void **)&TempRawPtr, vinInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for vin!!");
            }
        }

        // Create simple muxer task
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_StillEncMuxStack, (void **)&TempRawPtr, STILL_ENC_MUX_TASK_STACK_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for muxer stack!!");
        }
        if (AmbaKAL_TaskCreate(&StillEncMuxTask, "Still Encoder UnitTest Muxing Task", 10, \
             AmpUT_StillEnc_MuxTask, 0x0, AmpUT_StillEncMuxStack, STILL_ENC_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
             AmbaPrint("StillEnc UnitTest: Muxer task creation failed");
        }

        // Create Still encoder object
        {
            AMP_STILLENC_INIT_CFG_s encInitCfg;

            // Init STILLENC module
            AmpStillEnc_GetInitDefaultCfg(&encInitCfg);
            if (StillEncWorkBuf == NULL) {
                er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillEncWorkBuf, (void **)&TempRawPtr, encInitCfg.MemoryPoolSize, 32);
                if (er != OK) {
                    AmbaPrint("Out of memory for stillmain!!");
                }
            }
            encInitCfg.MemoryPoolAddr = StillEncWorkBuf;
            AmpStillEnc_Init(&encInitCfg);
        }
        StillCodecInit = 1;
    }

    if (StillEncPri == NULL) { //no codec be create
        AMP_STILLENC_HDLR_CFG_s encCfg;
        AMP_VIDEOENC_LAYER_DESC_s layer;

        memset(&encCfg, 0x0, sizeof(AMP_STILLENC_HDLR_CFG_s));
        memset(&layer, 0x0, sizeof(AMP_VIDEOENC_LAYER_DESC_s));
        encCfg.MainLayout.Layer = &layer;
        AmpStillEnc_GetDefaultCfg(&encCfg);

        // Assign callback
        encCfg.cbEvent = AmpUT_StillEncCallback;

        // Assign bitstream/descriptor buffer
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&BitsBuf, (void **)&TempRawPtr, STILL_BISFIFO_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }
        encCfg.BitsBufCfg.BitsBufAddr = BitsBuf;
        encCfg.BitsBufCfg.BitsBufSize = STILL_BISFIFO_SIZE;

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&DescBuf, (void **)&TempRawPtr, STILL_DESC_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for DescFifo!!");
        }
        encCfg.BitsBufCfg.DescBufAddr = DescBuf;
        encCfg.BitsBufCfg.DescBufSize = STILL_DESC_SIZE;
        encCfg.BitsBufCfg.BitsRunoutThreshold = STILL_BISFIFO_SIZE - 4*1024*1024; // leave 4MB
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", BitsBuf, STILL_BISFIFO_SIZE, DescBuf, STILL_DESC_SIZE);
        AmpStillEnc_Create(&encCfg, &StillEncPri);

        // create a virtual fifo
        if (StillEncVirtualFifoHdlr == NULL) {
            AMP_FIFO_CFG_s fifoDefCfg;

            AmpFifo_GetDefaultCfg(&fifoDefCfg);
            fifoDefCfg.hCodec = StillEncPri;
            fifoDefCfg.IsVirtual = 1;
            fifoDefCfg.NumEntries = 1024;
            fifoDefCfg.cbEvent = AmpUT_StillEnc_FifoCB;
            AmpFifo_Create(&fifoDefCfg, &StillEncVirtualFifoHdlr);
        }
    }

    if (StillEncPipe == NULL) { //no pipeline be create
        AMP_ENC_PIPE_CFG_s pipeCfg;

        AmpEnc_GetDefaultCfg(&pipeCfg);
        pipeCfg.encoder[0] = StillEncPri;
        pipeCfg.numEncoder = 1;
        pipeCfg.cbEvent = AmpUT_StillEncPipeCallback;
        pipeCfg.type = AMP_ENC_STILL_PIPE;
        AmpEnc_Create(&pipeCfg, &StillEncPipe);

        AmpEnc_Add(StillEncPipe);
    }

    /* Phase II */
    /* fill script and run */
    if (StillRawCaptureRunning) {
        AmbaPrint("Error status");
        goto _DONE;
    }

    /* Step1. calc yuv buffer memory */

    //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
    yuvSize = PBCapCtrl.MainBuf.Width*PBCapCtrl.MainBuf.Height*2;
    AmbaPrint("[UT_PBCapture]yuv(%u %u %u)!", PBCapCtrl.MainBuf.Width, PBCapCtrl.MainBuf.Height, yuvSize);
    scrnSize = PBCapCtrl.ScrnWidth*PBCapCtrl.ScrnHeight*2;
    scrnSize += (scrnSize*10)/100;
    thmSize = PBCapCtrl.ThmWidth*PBCapCtrl.ThmHeight*2;
    thmSize += (thmSize*10)/100;
    AmbaPrint("[UT_PBCapture]scrn(%d %d %u) thm(%d %d %u)!", \
        PBCapCtrl.ScrnWidth, PBCapCtrl.ScrnHeight, scrnSize, \
        PBCapCtrl.ThmWidth, PBCapCtrl.ThmHeight, thmSize);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, (void **)&TempRawPtr, scrnSize*1, 32);
    if (er != OK) {
        AmbaPrint("[UT_PBCapture]Cache alloc yuv_scrn fail (%u)!", scrnSize*1);
    } else {
        ScrnBuffAddr = (UINT8*)tempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_PBCapture]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, (void **)&TempRawPtr, thmSize*1, 32);
    if (er != OK) {
        AmbaPrint("[UT_PBCapture]Cache alloc yuv_thm fail (%u)!", thmSize*1);
    } else {
        ThmBuffAddr = (UINT8*)tempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_PBCapture]thmBuffAddr (0x%08X)!", ThmBuffAddr);
    }
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, (void **)&TempRawPtr, ScriptSize, 32);
        if (er != OK) {
            AmbaPrint("[UT_PBCapture]Cache alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)tempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_PBCapture]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    stageAddr = ScriptAddr;
    genScrpt = (AMP_SENC_SCRPT_GENCFG_s *)stageAddr;
    memset(genScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    genScrpt->Cmd = SENC_GENCFG;
    genScrpt->RawEncRepeat = 0;
    genScrpt->RawToCap = 1;
    genScrpt->StillProcMode = 0; //dont care
#ifdef CONFIG_SOC_A12
    genScrpt->EncRotateFlip = StillEncRotate;
#endif
    genScrpt->QVConfig.DisableLCDQV = 1;
    genScrpt->QVConfig.DisableHDMIQV = 1;
    QvLcdEnable = 0;
    QvTvEnable = 0;
    genScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (genScrpt->b2LVCfg)? 1: 0;
    genScrpt->ScrnEnable = 1;
    genScrpt->ThmEnable = 1;
    genScrpt->PostProc = &PostPbCaptureCB;
    genScrpt->PreProc = &PrePbCaptureCB;
    genScrpt->MainBuf = PBCapCtrl.MainBuf;
    genScrpt->ScrnBuf.ColorFmt = PBCapCtrl.MainBuf.ColorFmt;
    genScrpt->ScrnBuf.Width = genScrpt->ScrnBuf.Pitch = PBCapCtrl.ScrnWidth;
    genScrpt->ScrnBuf.Height = PBCapCtrl.ScrnHeight;
    genScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    genScrpt->ScrnBuf.ChromaAddr = 0;
    genScrpt->ScrnBuf.AOI.X = 0;
    genScrpt->ScrnBuf.AOI.Y = 0;
    genScrpt->ScrnBuf.AOI.Width = PBCapCtrl.ScrnActiveWidth;
    genScrpt->ScrnBuf.AOI.Height = PBCapCtrl.ScrnActiveHeight;
    genScrpt->ScrnWidth = PBCapCtrl.ScrnWidth;
    genScrpt->ScrnHeight = PBCapCtrl.ScrnHeight;
    genScrpt->ThmBuf.ColorFmt = PBCapCtrl.MainBuf.ColorFmt;
    genScrpt->ThmBuf.Width = genScrpt->ThmBuf.Pitch = PBCapCtrl.ThmWidth;
    genScrpt->ThmBuf.Height = PBCapCtrl.ThmHeight;
    genScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    genScrpt->ThmBuf.ChromaAddr = 0;
    genScrpt->ThmBuf.AOI.X = 0;
    genScrpt->ThmBuf.AOI.Y = 0;
    genScrpt->ThmBuf.AOI.Width = PBCapCtrl.ThmActiveWidth;
    genScrpt->ThmBuf.AOI.Height = PBCapCtrl.ThmActiveHeight;
    genScrpt->ThmWidth = PBCapCtrl.ThmWidth;
    genScrpt->ThmHeight = PBCapCtrl.ThmHeight;

    genScrpt->BrcCtrl.Tolerance = 10;
    genScrpt->BrcCtrl.MaxEncLoop = 0;
    genScrpt->BrcCtrl.JpgBrcCB = NULL;
    genScrpt->BrcCtrl.TargetBitRate = 0;
    genScrpt->BrcCtrl.MainQTAddr = PBCapCtrl.MainQTAddr;
    genScrpt->BrcCtrl.ThmQTAddr = PBCapCtrl.ThmQTAddr;
    genScrpt->BrcCtrl.ScrnQTAddr = PBCapCtrl.ScrnQTAddr;

    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    totalStageNum ++;
    AmbaPrint("[UT_PBCapture]Stage_0 0x%08X", stageAddr);

    //yuv2yuv config
    stageAddr = ScriptAddr + totalScriptSize;
    Yuv2YuvScrpt = (AMP_SENC_SCRPT_YUV2YUV_s *)stageAddr;
    memset(Yuv2YuvScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2YUV_s));
    Yuv2YuvScrpt->Cmd = SENC_YUV2YUV;
    Yuv2YuvScrpt->SrcMainBuf = PBCapCtrl.MainBuf;
    Yuv2YuvScrpt->ScrnBuf.ColorFmt = PBCapCtrl.MainBuf.ColorFmt;
    Yuv2YuvScrpt->ScrnBuf.Width = PBCapCtrl.ScrnWidth;
    Yuv2YuvScrpt->ScrnBuf.Height = PBCapCtrl.ScrnHeight;
    Yuv2YuvScrpt->ScrnBuf.Pitch = PBCapCtrl.ScrnWidth;
    Yuv2YuvScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    Yuv2YuvScrpt->ScrnBuf.ChromaAddr = 0; //just behind luma
    Yuv2YuvScrpt->ScrnBuf.AOI.X = 0;
    Yuv2YuvScrpt->ScrnBuf.AOI.Y = 0;
    Yuv2YuvScrpt->ScrnBuf.AOI.Width = PBCapCtrl.ScrnActiveWidth;
    Yuv2YuvScrpt->ScrnBuf.AOI.Height = PBCapCtrl.ScrnActiveHeight;
    Yuv2YuvScrpt->ScrnWidth = PBCapCtrl.ScrnWidth;
    Yuv2YuvScrpt->ScrnHeight = PBCapCtrl.ScrnHeight;
    Yuv2YuvScrpt->ThmBuf.ColorFmt = PBCapCtrl.MainBuf.ColorFmt;
    Yuv2YuvScrpt->ThmBuf.Width = PBCapCtrl.ThmWidth;
    Yuv2YuvScrpt->ThmBuf.Height = PBCapCtrl.ThmHeight;
    Yuv2YuvScrpt->ThmBuf.Pitch = PBCapCtrl.ThmWidth;
    Yuv2YuvScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    Yuv2YuvScrpt->ThmBuf.ChromaAddr = 0; //just behind luma
    Yuv2YuvScrpt->ThmBuf.AOI.X = 0;
    Yuv2YuvScrpt->ThmBuf.AOI.Y = 0;
    Yuv2YuvScrpt->ThmBuf.AOI.Width = PBCapCtrl.ThmActiveWidth;
    Yuv2YuvScrpt->ThmBuf.AOI.Height = PBCapCtrl.ThmActiveHeight;
    Yuv2YuvScrpt->ThmWidth = PBCapCtrl.ThmWidth;
    Yuv2YuvScrpt->ThmHeight = PBCapCtrl.ThmHeight;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2YUV_s));
    totalStageNum ++;
    AmbaPrint("[UT_PBCapture]Stage_1 0x%X", stageAddr);

    //yuv2jpg config
    stageAddr = ScriptAddr + totalScriptSize;
    yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)stageAddr;
    memset(yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    yuv2JpgScrpt->YuvRingBufSize = 0;

    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    totalStageNum ++;
    AmbaPrint("[UT_PBCapture]Stage_2 0x%X", stageAddr);

    //script config
    scrpt.mode = AMP_SCRPT_MODE_STILL;
    scrpt.StepPreproc = NULL;
    scrpt.StepPostproc = NULL;
    scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    scrpt.ScriptTotalSize = totalScriptSize;
    scrpt.ScriptStageNum = totalStageNum;
    AmbaPrint("[UT_PBCapture]Scrpt addr 0x%X, Sz %uByte, stg %d", scrpt.ScriptStartAddr, scrpt.ScriptTotalSize, scrpt.ScriptStageNum);

    StillBGProcessing = 1;
    StillPivProcess = 0;

    /* Step4. execute script */
    AmpEnc_RunScript(StillEncPipe, &scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step4. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");

_DONE:

    return 0;
}

/**
 * UnitTest: Raw2yuv PreProc Callback
 * Stage1: RAW2YUV  -> setup r2y idsp cfg
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_PivRaw2YuvPreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 1) {
        UINT16 RawWidth = 0, RawHeight = 0;
        if (StillOBModeEnable == 1) {
            AMBA_SENSOR_MODE_ID_u SensorMode = {0};
            AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
            SensorMode.Data = StillEncMgt[StillEncModeIdx].InputStillMode;
            AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);
            RawWidth = SensorModeInfo.OutputInfo.OutputWidth;
            RawHeight = SensorModeInfo.OutputInfo.OutputHeight;
        } else {
            RawWidth =  StillEncMgt[StillEncModeIdx].StillCaptureWidth;
            RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        }
        G_pivRaw2YuvRestoreWarp = 0;
        AmpUT_StillEncIdspParamSetup(info->AeIdx, RawWidth, RawHeight);
    }
    return 0;
}

/**
 * UnitTest: simple raw to yuv PostProc Callback
 * Stage1: RAW2YUV  -> dump yuv
 * @param [in] info postproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_PivRaw2YuvPostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    if (info->StageCnt == 1) {
        char fn[MAX_FILENAME_LENGTH];
        char fn1[MAX_FILENAME_LENGTH];
        UINT8 *lumaAddr = NULL, *chromaAddr = NULL;
        UINT16 pitch = 0, width = 0, height = 0;
        AMP_CFS_FILE_s *y = NULL;
        AMP_CFS_FILE_s *uv = NULL;
        char mdASCII[3] = {'w','+','\0'};
        AMP_AREA_s Aoi = {0};
        UINT8 type = 0;
        if (G_pivRaw2YuvRestoreWarp == 0 && G_iso == 2) {
            //In Piv_r2y @ FastMode, since we use IK_VideoBatch, we need to restore warp setting for LV
            AMBA_DSP_IMG_MODE_CFG_s imgMode = {0};
            AMBA_DSP_IMG_SIZE_INFO_s sizeInfo = {0};
            AMBA_SENSOR_STATUS_INFO_s sensorStatus = {0};
            AMBA_SENSOR_INPUT_INFO_s *inputInfo = &sensorStatus.ModeInfo.InputInfo;
            AMBA_SENSOR_AREA_INFO_s *recPixel = &sensorStatus.ModeInfo.OutputInfo.RecordingPixels;
            AMBA_DSP_IMG_WARP_CALC_INFO_s calcWarp = {0};
            double zoomRatio = 0;
            AMP_AREA_s hwCaptureWindow = {0};
            UINT16 encW = 0, encH = 0;
            AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s vinGeo = {0};
            AMBA_DSP_IMG_OUT_WIN_INFO_s imgOutputWin = {0};
            AMBA_DSP_IMG_DZOOM_INFO_s imgDzoom = {0};
            UINT16 offsetX = 0;
            UINT8 ArIndex = 0;

            if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
            else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

            imgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
            imgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
            imgMode.BatchId = (G_iso == 0)? AMBA_DSP_STILL_HISO_FILTER:((G_iso == 1)? AMBA_DSP_STILL_LISO_FILTER: AMBA_DSP_VIDEO_FILTER);

            imgMode.ContextId = 0;
            imgMode.ConfigId  = 0;

            /* Dzoom */
            encW = StillEncMgt[StillEncModeIdx].MainWidth;
            encH = StillEncMgt[StillEncModeIdx].MainHeight;

            AmbaSensor_GetStatus(EncChannel, &sensorStatus);
            hwCaptureWindow.Width = StillEncMgt[StillEncModeIdx].CaptureWidth;
            hwCaptureWindow.Height = StillEncMgt[StillEncModeIdx].CaptureHeight;
            hwCaptureWindow.X = recPixel->StartX + (((recPixel->Width - hwCaptureWindow.Width)/2)&0xFFF8);
            hwCaptureWindow.Y = (recPixel->StartY + ((recPixel->Height - hwCaptureWindow.Height)/2)) & 0xFFFE;

            offsetX = (hwCaptureWindow.X - recPixel->StartX)*inputInfo->HSubsample.FactorDen/inputInfo->HSubsample.FactorNum;
            vinGeo.StartX = inputInfo->PhotodiodeArray.StartX + offsetX;
            vinGeo.StartY = inputInfo->PhotodiodeArray.StartY +
                (hwCaptureWindow.Y - recPixel->StartY)*inputInfo->VSubsample.FactorDen/inputInfo->VSubsample.FactorNum;
            vinGeo.Width = hwCaptureWindow.Width;
            vinGeo.Height = hwCaptureWindow.Height;
            vinGeo.HSubSample.FactorDen = inputInfo->HSubsample.FactorDen;
            vinGeo.HSubSample.FactorNum = inputInfo->HSubsample.FactorNum;
            vinGeo.VSubSample.FactorDen = inputInfo->VSubsample.FactorDen;
            vinGeo.VSubSample.FactorNum = inputInfo->VSubsample.FactorNum;
            AmbaDSP_WarpCore_SetVinSensorGeo(&imgMode, &vinGeo);

            imgOutputWin.MainWinDim.Width  = encW;
            imgOutputWin.MainWinDim.Height = encH;
        #ifdef CONFIG_SOC_A12
            imgOutputWin.ScreennailDim.Width  = StillEncMgt[StillEncModeIdx].ScrnWidthAct;
            imgOutputWin.ScreennailDim.Height = StillEncMgt[StillEncModeIdx].ScrnHeightAct;
            imgOutputWin.ThumbnailDim.Width  = StillEncMgt[StillEncModeIdx].ThmWidthAct;
            imgOutputWin.ThumbnailDim.Height = StillEncMgt[StillEncModeIdx].ThmHeightAct;
            imgOutputWin.PrevWinDim[0].Width  = StillVoutMgt[0][ArIndex].Width;
            imgOutputWin.PrevWinDim[0].Height = StillVoutMgt[0][ArIndex].Height;
            imgOutputWin.PrevWinDim[1].Width  = StillVoutMgt[1][ArIndex].Width;
            imgOutputWin.PrevWinDim[1].Height = StillVoutMgt[1][ArIndex].Height;
        #endif
            AmbaDSP_WarpCore_SetOutputWin(&imgMode, &imgOutputWin);

            /* Dzoom don't care, TBD */
            zoomRatio = (double) 100 / 100;

            imgDzoom.ZoomX = (UINT32)(zoomRatio * 65536);
            imgDzoom.ZoomY = (UINT32)(zoomRatio * 65536);
            imgDzoom.ShiftX = 0;
            imgDzoom.ShiftY = 0;
            AmbaDSP_WarpCore_SetDzoomFactor(&imgMode, &imgDzoom);

            AmbaDSP_WarpCore_CalcDspWarp(&imgMode, 0);
            AmbaDSP_WarpCore_CalcDspCawarp(&imgMode, 0);

            AmbaDSP_WarpCore_SetDspWarp(&imgMode);
            AmbaDSP_WarpCore_SetDspCawarp(&imgMode);

            if (AmbaDSP_ImgGetWarpCompensation(&imgMode, &calcWarp) != OK)
                AmbaPrintColor(RED, "[UT_StillEnc_PivRaw2Yuv_PostP] Get Warp Compensation fail!!");

            sizeInfo.WidthIn     = ((calcWarp.ActWinCrop.RightBotX - calcWarp.ActWinCrop.LeftTopX + 0xFFFF)>>16);
            sizeInfo.HeightIn    = ((calcWarp.ActWinCrop.RightBotY - calcWarp.ActWinCrop.LeftTopY + 0xFFFF)>>16);
            sizeInfo.WidthMain   = StillEncMgt[StillEncModeIdx].MainWidth;
            sizeInfo.HeightMain  = StillEncMgt[StillEncModeIdx].MainHeight;
            sizeInfo.WidthPrevA = StillVoutMgt[0][ArIndex].Width;
            sizeInfo.HeightPrevA = StillVoutMgt[0][ArIndex].Height;
            sizeInfo.WidthPrevB = StillVoutMgt[1][ArIndex].Width;
            sizeInfo.HeightPrevB = StillVoutMgt[1][ArIndex].Height;
            sizeInfo.WidthScrn = StillEncMgt[StillEncModeIdx].ScrnWidthAct;
            sizeInfo.HeightScrn = StillEncMgt[StillEncModeIdx].ScrnHeightAct;

            AmbaDSP_ImgSetSizeInfo(&imgMode, &sizeInfo);
            AmbaDSP_ImgPostExeCfg(&imgMode, (AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e)0);
            AmbaPrint("[UT_StillEnc_PivRaw2Yuv_PostP] Done!!!");
            G_pivRaw2YuvRestoreWarp = 1;
        }

        if (info->media.YuvInfo.ThmLumaAddr) {
            sprintf(fn, "%s:\\%04d_t.y", DefaultSlot, YuvFno);
            sprintf(fn1, "%s:\\%04d_t.uv", DefaultSlot, YuvFno);
            lumaAddr = info->media.YuvInfo.ThmLumaAddr;
            chromaAddr = info->media.YuvInfo.ThmChromaAddr;
            pitch = info->media.YuvInfo.ThmPitch;
            width = info->media.YuvInfo.ThmWidth;
            height = StillEncMgt[StillEncModeIdx].ThmHeight; // Thm align to 16, user need to decide height by their own
            type = 2;
        } else if (info->media.YuvInfo.ScrnLumaAddr) {
            sprintf(fn, "%s:\\%04d_s.y", DefaultSlot, YuvFno);
            sprintf(fn1, "%s:\\%04d_s.uv", DefaultSlot, YuvFno);
            lumaAddr = info->media.YuvInfo.ScrnLumaAddr;
            chromaAddr = info->media.YuvInfo.ScrnChromaAddr;
            pitch = info->media.YuvInfo.ScrnPitch;
            width = info->media.YuvInfo.ScrnWidth;
            height = info->media.YuvInfo.ScrnHeight;
            type = 1;
        } else if (info->media.YuvInfo.LumaAddr) {
            sprintf(fn, "%s:\\%04d_m.y", DefaultSlot, YuvFno);
            sprintf(fn1, "%s:\\%04d_m.uv", DefaultSlot, YuvFno);
            lumaAddr = info->media.YuvInfo.LumaAddr;
            chromaAddr = info->media.YuvInfo.ChromaAddr;
            pitch = info->media.YuvInfo.Pitch;
            width = info->media.YuvInfo.Width;
            height = info->media.YuvInfo.Height;
            type = 0;
        }

        if (IsStillStampEnable == 1) {
            AMP_AREA_s Aoi = {0};

            Aoi.X = 0;
            Aoi.Y = 0;
            Aoi.Width = width;
            Aoi.Height = height;
            AmpUT_DrawStillStamp(type, lumaAddr, chromaAddr, Aoi, pitch);
        }

        Aoi.X = Aoi.Y = 0;
        Aoi.Width = width;
        Aoi.Height = height;
        AmbaPrint("[Amp_UT] Dump YUV(%s) (0x%X 0x%X) %d %d %d Start!", \
            (info->media.YuvInfo.DataFormat)?"422":"420", \
            lumaAddr, chromaAddr, pitch, width, height);
        if (!(StillDumpSkip & STILL_ENC_SKIP_YUV)) {
            y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
            AmpUT_DumpAoi(lumaAddr, pitch, Aoi, (void *)y);
            UT_StillEncodefsync((void *)y);
            UT_StillEncodefclose((void *)y);

            uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
            AmpUT_DumpAoi(chromaAddr, pitch, Aoi, (void *)uv);
            UT_StillEncodefsync((void *)uv);
            UT_StillEncodefclose((void *)uv);
        }

        //release yuv buffers
        if (info->media.YuvInfo.ThmLumaAddr) {
            if (ThmBuffAddr) {
            #ifndef _STILL_BUFFER_FROM_DSP_
                if (AmbaKAL_BytePoolFree((void *)OriThmBuffAddr) != OK)
                    AmbaPrint("[Amp_UT] memFree Fail thm!");
            #endif
                ThmBuffAddr = NULL;
            }
        } else if (info->media.YuvInfo.ScrnLumaAddr) {
            if (ScrnBuffAddr) {
            #ifndef _STILL_BUFFER_FROM_DSP_
                if (AmbaKAL_BytePoolFree((void *)OriScrnBuffAddr) != OK)
                    AmbaPrint("[Amp_UT] memFree Fail scrn!");
            #endif
                ScrnBuffAddr = NULL;
            }
        } else if (info->media.YuvInfo.LumaAddr) {
            if (YuvBuffAddr) {
            #ifndef _STILL_BUFFER_FROM_DSP_
                if (AmbaKAL_BytePoolFree((void *)OriYuvBuffAddr) != OK)
                    AmbaPrint("[Amp_UT] memFree Fail yuv!");
            #endif
                YuvBuffAddr = NULL;
            }
        }

        if (ThmBuffAddr == NULL && \
            ScrnBuffAddr == NULL && \
            YuvBuffAddr == NULL) {
            //release raw buffers
            if (RawBuffAddr) {
            #ifndef _STILL_BUFFER_FROM_DSP_
                if (AmbaKAL_BytePoolFree((void *)OriRawBuffAddr) != OK)
                    AmbaPrint("[Amp_UT] memFree Fail raw!");
            #endif
                RawBuffAddr = NULL;
            }

            YuvFno++;
        }
    } else {
        //nothing to do
    }

    return 0;
}

/**
 * UnitTest: Raw capture PreProc Callback
 * Stage1: RAWCAP  -> Nothing to do
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_RawCapPreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    // Nothing to do
    return 0;
}

/**
 * UnitTest: single raw capture PostProc Callback
 * Stage1: RAWCAP  -> Dump raw
 * @param [in] info postproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_RawCapPostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    if (info->StageCnt == 1) {
        char fn[MAX_FILENAME_LENGTH];
        AMBA_FS_FILE *raw = NULL;
        char mdASCII[3] = {'w','+','\0'};

        //raw ready, dump it
        sprintf(fn, "%s:\\%04d.RAW", DefaultSlot, RawFno);

        if (!(StillDumpSkip & STILL_ENC_SKIP_RAW)) {
            raw = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
            AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d %d to %s Start!", \
                info->media.RawInfo.RawAddr, \
                info->media.RawInfo.RawPitch, \
                info->media.RawInfo.RawWidth, \
                info->media.RawInfo.RawHeight, fn);
            UT_StillEncodefwrite((const void *)info->media.RawInfo.RawAddr, 1, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight, (void *)raw);
            UT_StillEncodefsync((void *)raw);
            UT_StillEncodefclose((void *)raw);
        }

        if (StillEncDumpItuner == 1) {
            char TuningModeExt[32] = "SINGLE_SHOT";
            char ItunerFileName[64];
            snprintf(ItunerFileName, sizeof(ItunerFileName), "%s:\\A12_%04d.txt", DefaultSlot, RawFno);
            AmpUT_StillEncodeDumpItunerFile(ItunerFileName, TuningModeExt, fn);
        }

        RawFno++;

        //release raw buffer
        //AmbaPrint("[Amp_UT] Raw(0x%08X) memFree", RawBuffAddr);
        if (RawBuffAddr) {
            if (AmbaKAL_BytePoolFree((void *)OriRawBuffAddr) != OK)
                AmbaPrint("[Amp_UT] memFree Fail raw!");
            RawBuffAddr = NULL;
        }
    } else {
        //nothing to do
    }

    return 0;
}

/**
 * UnitTest: Single Capture PreProc Callback
 * Stage1: RAWCAP  -> Nothing to do
 * Stage2: RAW2YUV  -> setup r2y idsp cfg
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_SingleCapturePreCB(AMP_STILLENC_PREP_INFO_s *info)
{
   if (StillOBModeEnable == 1) {
        if (info->StageCnt == 2) {
            UINT32 ImgChipNo = 0;
            AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
            ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
            ImgMode.AlgoMode = (G_iso == 0)? AMBA_DSP_IMG_ALGO_MODE_HISO: ((G_iso == 1)? AMBA_DSP_IMG_ALGO_MODE_LISO: AMBA_DSP_IMG_ALGO_MODE_FAST);
            ImgMode.BatchId = (G_iso == 0)? AMBA_DSP_STILL_HISO_FILTER: ((G_iso == 1)? AMBA_DSP_STILL_LISO_FILTER: AMBA_DSP_VIDEO_FILTER);
            ImgMode.ContextId = 0;
            ImgMode.ConfigId = 0;
            // After MW_IP_COMPUTE_STILL_OB, 3A need to compute adj again to apply new black level.
            AmbaSample_AdjStillControl(ImgChipNo, 0, &ImgMode, G_iso);
            AmpUT_StillEncRaw2RawIdspCfgCB(info->CfaIndex);
        } else if (info->StageCnt == 3) {
    #ifdef CONFIG_SOC_A12
        AmpUT_StillEncPostIdspParamSetup(info->AeIdx);
    #else
        AmpUT_StillEncIdspParamSetup(info->AeIdx);
    #endif
        }
    } else {
        if (info->StageCnt == 3) {
            AmbaPrint("[SingleCapture]PreP SerialNum %d", info->JpegSerialNumber);
        }
    }

    return 0;
}

/**
 * UnitTest: single capture PostProc Callback
 * Stage1: RAWCAP  -> Dump raw
 * Stage2: RAW2YUV -> Dump yuv
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_SingleCapturePostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    static UINT8 yuvFlag = 0;
    UINT8 dump_file = 0/*1*/;
    if (StillOBModeEnable == 1) {
        if (info->StageCnt == 1) {
            char fn[MAX_FILENAME_LENGTH];
            char mdASCII[3] = {'w','+','\0'};
            AMBA_FS_FILE *raw = NULL;

            yuvFlag = 0;
            if (dump_file) {
                //raw ready, dump it
                sprintf(fn, "%s:\\%04d.RAW", DefaultSlot, RawFno);

                raw = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
                AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d %d  to %s Start!", \
                    info->media.RawInfo.RawAddr, \
                    info->media.RawInfo.RawPitch, \
                    info->media.RawInfo.RawWidth, \
                    info->media.RawInfo.RawHeight, fn);
                UT_StillEncodefwrite((const void *)info->media.RawInfo.RawAddr, 1, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight, (void *)raw);
                UT_StillEncodefsync((void *)raw);
                UT_StillEncodefclose((void *)raw);
                RawFno++;
            }
        } else if (info->StageCnt == 2) {
            AmpUT_StillEncPostWBCalculation(info->media.CfaStatInfo);
        } else if (info->StageCnt == 3) {
            char fn[MAX_FILENAME_LENGTH];
            char fn1[MAX_FILENAME_LENGTH];
            UINT8 *lumaAddr = NULL, *chromaAddr = NULL;
            UINT16 pitch = 0, width = 0, height = 0;
            UINT8 type = 0;
            AMP_CFS_FILE_s *y = NULL;
            AMP_CFS_FILE_s *uv = NULL;
            char mdASCII[3] = {'w','+','\0'};

            if (info->media.YuvInfo.ThmLumaAddr) {
                sprintf(fn,"%s:\\%04d_t.y", DefaultSlot, YuvFno);
                sprintf(fn1,"%s:\\%04d_t.uv", DefaultSlot, YuvFno);
                lumaAddr = info->media.YuvInfo.ThmLumaAddr;
                chromaAddr = info->media.YuvInfo.ThmChromaAddr;
                pitch = info->media.YuvInfo.ThmPitch;
                width = info->media.YuvInfo.ThmWidth;
                height = StillEncMgt[StillEncModeIdx].ThmHeight; // Thm align to 16, user need to decide height by their own
                type = 2;
            } else if (info->media.YuvInfo.ScrnLumaAddr) {
                sprintf(fn,"%s:\\%04d_s.y", DefaultSlot, YuvFno);
                sprintf(fn1,"%s:\\%04d_s.uv", DefaultSlot, YuvFno);
                lumaAddr = info->media.YuvInfo.ScrnLumaAddr;
                chromaAddr = info->media.YuvInfo.ScrnChromaAddr;
                pitch = info->media.YuvInfo.ScrnPitch;
                width = info->media.YuvInfo.ScrnWidth;
                height = info->media.YuvInfo.ScrnHeight;
                type = 1;
            } else if (info->media.YuvInfo.LumaAddr) {
                sprintf(fn,"%s:\\%04d_m.y", DefaultSlot, YuvFno);
                sprintf(fn1,"%s:\\%04d_m.uv", DefaultSlot, YuvFno);
                lumaAddr = info->media.YuvInfo.LumaAddr;
                chromaAddr = info->media.YuvInfo.ChromaAddr;
                pitch = info->media.YuvInfo.Pitch;
                width = info->media.YuvInfo.Width;
                height = info->media.YuvInfo.Height;
                type = 0;
            }

            if (IsStillStampEnable == 1) {
                AMP_AREA_s Aoi = {0};
                Aoi.X = 0;
                Aoi.Y = 0;
                Aoi.Width = width;
                Aoi.Height = height;
                AmpUT_DrawStillStamp(type, lumaAddr, chromaAddr, Aoi, pitch);
            }

            if (dump_file) {
                AMP_AREA_s Aoi = {0};
                Aoi.X = Aoi.Y = 0;
                Aoi.Width = width;
                Aoi.Height = height;

                AmbaPrint("[Amp_UT] Dump YUV(%s) (0x%X 0x%X) %d %d %d Start!", \
                    (info->media.YuvInfo.DataFormat)?"422":"420", \
                    lumaAddr, chromaAddr, pitch, width, height);
                y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
                AmpUT_DumpAoi(lumaAddr, pitch, Aoi, (void *)y);
                UT_StillEncodefsync((void *)y);
                UT_StillEncodefclose((void *)y);

                uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
                AmpUT_DumpAoi(chromaAddr, pitch, Aoi, (void *)uv);
                UT_StillEncodefsync((void *)uv);
                UT_StillEncodefclose((void *)uv);
                //AmbaPrint("[Amp_UT] Dump YUV Done!");
            }

            //release yuv buffers
            if (info->media.YuvInfo.ThmLumaAddr) {
                yuvFlag |= 0x4;
            } else if (info->media.YuvInfo.ScrnLumaAddr) {
                yuvFlag |= 0x2;
            } else if (info->media.YuvInfo.LumaAddr) {
                yuvFlag |= 0x1;
            }

            if (yuvFlag == 0x7) {
                if (StillEncDumpItuner == 1) {
                    char TuningModeExt[32] = "SINGLE_SHOT";
                    char RawPath[64];
                    char ItunerFileName[64];
                    snprintf(RawPath, sizeof(RawPath), "%s:\\%04d.RAW", DefaultSlot, RawFno);
                    snprintf(ItunerFileName, sizeof(ItunerFileName), "%s:\\A12_%04d.txt", DefaultSlot, YuvFno);
                    AmpUT_StillEncodeDumpItunerFile(ItunerFileName, TuningModeExt, RawPath);
                }

                //reset yuv flag
                yuvFlag = 0x0;
                //release raw buffers
                if (RawBuffAddr) {
                #ifndef _STILL_BUFFER_FROM_DSP_
                    if (AmbaKAL_BytePoolFree((void *)OriRawBuffAddr) != OK)
                        AmbaPrint("[Amp_UT] memFree Fail raw!");
                #endif
                    RawBuffAddr = NULL;
                }

                if (dump_file) {
                    YuvFno++;
                }
            }
        } else {
            //nothing to do
        }
    } else {
        if (info->StageCnt == 1) {
            char fn[MAX_FILENAME_LENGTH];
            char mdASCII[3] = {'w','+','\0'};
            AMP_CFS_FILE_s *raw = NULL;

            yuvFlag = 0;
            if (dump_file) {
                //raw ready, dump it
                sprintf(fn, "%s:\\%04d.RAW", DefaultSlot, RawFno);

                raw = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
                AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d %d  to %s Start!", \
                    info->media.RawInfo.RawAddr, \
                    info->media.RawInfo.RawPitch, \
                    info->media.RawInfo.RawWidth, \
                    info->media.RawInfo.RawHeight, fn);
                UT_StillEncodefwrite((const void *)info->media.RawInfo.RawAddr, 1, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight, (void *)raw);
                UT_StillEncodefsync((void *)raw);
                UT_StillEncodefclose((void *)raw);
                RawFno++;
            }
        } else if (info->StageCnt == 2) {
            char fn[MAX_FILENAME_LENGTH];
            char fn1[MAX_FILENAME_LENGTH];
            UINT8 *lumaAddr = NULL, *chromaAddr = NULL;
            UINT16 pitch = 0, width = 0, height = 0;
            UINT8 type = 0;
            AMP_CFS_FILE_s *y = NULL;
            AMP_CFS_FILE_s *uv = NULL;

            char mdASCII[3] = {'w','+','\0'};

            if (info->media.YuvInfo.ThmLumaAddr) {
                sprintf(fn, "%s:\\%04d_t.y", DefaultSlot, YuvFno);
                sprintf(fn1, "%s:\\%04d_t.uv", DefaultSlot, YuvFno);
                lumaAddr = info->media.YuvInfo.ThmLumaAddr;
                chromaAddr = info->media.YuvInfo.ThmChromaAddr;
                pitch = info->media.YuvInfo.ThmPitch;
                width = info->media.YuvInfo.ThmWidth;
                height = StillEncMgt[StillEncModeIdx].ThmHeight; // Thm align to 16, user need to decide height by their own
                type = 2;
            } else if (info->media.YuvInfo.ScrnLumaAddr) {
                sprintf(fn, "%s:\\%04d_s.y", DefaultSlot, YuvFno);
                sprintf(fn1, "%s:\\%04d_s.uv", DefaultSlot, YuvFno);
                lumaAddr = info->media.YuvInfo.ScrnLumaAddr;
                chromaAddr = info->media.YuvInfo.ScrnChromaAddr;
                pitch = info->media.YuvInfo.ScrnPitch;
                width = info->media.YuvInfo.ScrnWidth;
                height = info->media.YuvInfo.ScrnHeight;
                type = 1;
            } else if (info->media.YuvInfo.LumaAddr) {
                sprintf(fn, "%s:\\%04d_m.y", DefaultSlot, YuvFno);
                sprintf(fn1, "%s:\\%04d_m.uv", DefaultSlot, YuvFno);
                lumaAddr = info->media.YuvInfo.LumaAddr;
                chromaAddr = info->media.YuvInfo.ChromaAddr;
                pitch = info->media.YuvInfo.Pitch;
                width = info->media.YuvInfo.Width;
                height = info->media.YuvInfo.Height;
                type = 0;
            }

            if (IsStillStampEnable == 1) {
                AMP_AREA_s Aoi = {0};
                Aoi.X = 0;
                Aoi.Y = 0;
                Aoi.Width = width;
                Aoi.Height = height;
                AmpUT_DrawStillStamp(type, lumaAddr, chromaAddr, Aoi, pitch);
            }

            if (dump_file) {
                AMP_AREA_s Aoi = {0};
                Aoi.X = Aoi.Y = 0;
                Aoi.Width = width;
                Aoi.Height = height;

                AmbaPrint("[Amp_UT] Dump YUV(%s) (0x%X 0x%X) %d %d %d Start!", \
                    (info->media.YuvInfo.DataFormat)?"422":"420", \
                    lumaAddr, chromaAddr, pitch, width, height);
                y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
                AmpUT_DumpAoi(lumaAddr, pitch, Aoi, (void *)y);
                UT_StillEncodefsync((void *)y);
                UT_StillEncodefclose((void *)y);

                uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
                AmpUT_DumpAoi(chromaAddr, pitch, Aoi, (void *)uv);
                UT_StillEncodefsync((void *)uv);
                UT_StillEncodefclose((void *)uv);
                //AmbaPrint("[Amp_UT] Dump YUV Done!");
            }

            //release yuv buffers
            if (info->media.YuvInfo.ThmLumaAddr) {
                yuvFlag |= 0x4;
            } else if (info->media.YuvInfo.ScrnLumaAddr) {
                yuvFlag |= 0x2;
            } else if (info->media.YuvInfo.LumaAddr) {
                yuvFlag |= 0x1;
            }

            if (yuvFlag == 0x7) {
                if (StillEncDumpItuner == 1) {
                    char TuningModeExt[32] = "SINGLE_SHOT";
                    char RawPath[64];
                    char ItunerFileName[64];

                    snprintf(RawPath, sizeof(RawPath), "%s:\\%04d.RAW", DefaultSlot, RawFno);
                    snprintf(ItunerFileName, sizeof(ItunerFileName), "%s:\\A12_%04d.txt", DefaultSlot, YuvFno);
                    AmpUT_StillEncodeDumpItunerFile(ItunerFileName, TuningModeExt, RawPath);
                }

                //reset yuv flag
                yuvFlag = 0x0;
                //release raw buffers
                if (RawBuffAddr) {
                #ifndef _STILL_BUFFER_FROM_DSP_
                    if (AmbaKAL_BytePoolFree((void *)OriRawBuffAddr) != OK)
                        AmbaPrint("[Amp_UT] memFree Fail raw!");
                #endif
                    RawBuffAddr = NULL;
                }

                if (dump_file) {
                    YuvFno++;
                }
            }
        } else {
            //nothing to do
        }
    }
    return 0;
}

/**
 * UnitTest: Burst Capture PreProc Callback
 * Stage1: RAWCAP  -> Nothing to do
 * Stage2: RAW2YUV -> Nothing to do
 * Stage3: YUV2JPG -> Provide JpegSerialNumber to App
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_BurstCapturePreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 2) {
    #ifdef CONFIG_SOC_A9
        AmpUT_StillEncIdspParamSetup(info->AeIdx, StillEncMgt[StillEncModeIdx].StillCaptureWidth, StillEncMgt[StillEncModeIdx].StillCaptureHeight);
    #endif
    } if (info->StageCnt == 3) {
        AmbaPrint("[BurstCapture]PreP SerialNum %d", info->JpegSerialNumber);
    }
    return 0;
}

/**
 * UnitTest: single burst PostProc Callback
 * Stage1: RAWCAP  -> Dump raw
 * Stage2: RAW2YUV -> Dump yuv
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_BurstCapturePostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    static UINT8 yuvFlag = 0;
    static UINT8 captureCount =0;

    if (info->StageCnt == 2) {

        //release yuv buffers
        if (info->media.YuvInfo.ThmLumaAddr) {
            yuvFlag |= 0x4;
        } else if (info->media.YuvInfo.ScrnLumaAddr) {
            yuvFlag |= 0x2;
        } else if (info->media.YuvInfo.LumaAddr) {
            yuvFlag |= 0x1;
        }

        if (yuvFlag == 0x7) {
            //reset yuv flag
            yuvFlag = 0x0;
            captureCount+=1;
            if (captureCount == G_capcnt) {
                //release raw buffers
                if (RawBuffAddr) {
                #ifndef _STILL_BUFFER_FROM_DSP_
                    if (AmbaKAL_BytePoolFree((void *)OriRawBuffAddr) != OK)
                        AmbaPrint("[Amp_UT] memFree Fail raw!");
                #endif
                    RawBuffAddr = NULL;
                }
            }
            YuvFno++;
        }
    } else {
        //nothing to do
    }

    return 0;
}

/**
 * UnitTest: Continuous Burst Capture Preproc Callback

 * Stage1: RAWCAP  -> Nothing to do
 * Stage2: RAW2YUV -> Nothing to do
 * Stage3: YUV2JPG -> Nothing to do
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_BurstCaptureContPreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 2) {
    #ifdef CONFIG_SOC_A9
        AmpUT_StillEncIdspParamSetup(info->AeIdx, StillEncMgt[StillEncModeIdx].StillCaptureWidth, StillEncMgt[StillEncModeIdx].StillCaptureHeight);
    #endif
    } else if (info->StageCnt == 3) {
        AmbaPrint("[BurstCaptureCont]PreP SerialNum %d", info->JpegSerialNumber);
    }
    return 0;
}

/**
 * UnitTest: continous burst PostProc Callback
 * Stage1: RAWCAP  -> Dump raw
 * Stage2: RAW2YUV -> Dump yuv
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_BurstCaptureContPostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    static UINT8 yuvFlag = 0;
    static UINT8 captureCount =0;

    if (info->StageCnt == 2) {

        //release yuv buffers
        if (info->media.YuvInfo.ThmLumaAddr) {
            yuvFlag |= 0x4;
        } else if (info->media.YuvInfo.ScrnLumaAddr) {
            yuvFlag |= 0x2;
        } else if (info->media.YuvInfo.LumaAddr) {
            yuvFlag |= 0x1;
        }

        if (yuvFlag == 0x7) {
            //reset yuv flag
            yuvFlag = 0x0;
            captureCount+=1;
            if (captureCount == G_capcnt) {
                //release raw buffers
                if (RawBuffAddr) {
                #ifndef _STILL_BUFFER_FROM_DSP_
                    if (AmbaKAL_BytePoolFree((void *)OriRawBuffAddr) != OK)
                        AmbaPrint("[Amp_UT] memFree Fail raw!");
                #endif
                    RawBuffAddr = NULL;
                }
            }
            YuvFno++;
        }
    } else {
        //nothing to do
    }

    return 0;
}

/**
 * UnitTest: Continuous Burst Capture WB PreProc callback
 * Stage1: RAWCAP  -> Nothing to do
 * Stage2: RAW2RAW -> Nothing to do
 * Stage3: RAW2YUV -> Nothing to do
 * Stage4: YUV2JPG -> Nothing to do
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_BurstCaptureContWBPreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 2) {
        AmpUT_StillEncRaw2RawIdspCfgCB(info->CfaIndex);
    } else if (info->StageCnt == 3) {
    #ifdef CONFIG_SOC_A9
        AmpUT_StillEncIdspParamSetup(info->AeIdx, StillEncMgt[StillEncModeIdx].StillCaptureWidth, StillEncMgt[StillEncModeIdx].StillCaptureHeight);
    #endif
    } else if (info->StageCnt == 4) {
        AmbaPrint("[BurstCaptureContWB]PreP SerialNum %d", info->JpegSerialNumber);
    }
    return 0;
}

/**
 * UnitTest: continuous burst PostProc Callback
 * Stage1: RAWCAP  -> Nothing to do
 * Stage2: RAW2RAW -> Parse raw CFA stats
 * Stage3: RAW2YUV -> Nothing to do
 * Stage4: YUV2JPG -> Nothing to do
 * @param [in] info postproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_BurstCaptureContWBPostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    if (info->StageCnt == 2) {
        AmbaPrint("[BurstContWB] raw2RawPPCB: cfa index %d cfa addr 0x%X", info->CfaIndex, info->media.CfaStatInfo);
    } else {
        //nothing to do
    }

    return 0;
}

/**
 * UnitTest: AEB PreProc callback
 * Stage1: RAWCAP  -> Nothing to do
 * Stage2: RAW2YUV -> Nothing to do
 * Stage3: YUV2JPG -> Nothing to do
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_AEBPreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 2) {
    #ifdef CONFIG_SOC_A9
        AmpUT_StillEncIdspParamSetup(info->AeIdx, StillEncMgt[StillEncModeIdx].StillCaptureWidth, StillEncMgt[StillEncModeIdx].StillCaptureHeight);
    #endif
    } else if (info->StageCnt == 3) {
        AmbaPrint("[AEB]PreP SerialNum %d", info->JpegSerialNumber);
    }
    return 0;
}

/**
 * UnitTest: AEB PostProc Callback
 * Stage1: RAWCAP  -> Nothing to do
 * Stage3: RAW2YUV -> Nothing to do
 * Stage4: YUV2JPG -> Nothing to do
 * @param [in] info postproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_AEBPostCB(AMP_STILLENC_POSTP_INFO_s *info)
{

    return 0;
}

/**
 * UnitTest: continuous single capture PreProc Callback
 * Stage1: RAWCAP  -> Nothing to do
 * Stage2: RAW2RAW -> setup raw2raw idsp cfg
 * Stage3: RAW2YUV -> setup raw2yuv idsp cfg
 *
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_SingleCaptureContPreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 1) {
    } else if (info->StageCnt == 2) {
        AmpUT_StillEncRaw2RawIdspCfgCB(info->CfaIndex);
    } else if (info->StageCnt == 3) {
    #ifdef CONFIG_SOC_A12
        AmpUT_StillEncPostIdspParamSetup(info->AeIdx);
    #else
        AmpUT_StillEncIdspParamSetup(info->AeIdx);
    #endif
    }
    return 0;
}

/**
 * UnitTest: continuous single capture PostProc Callback
 * Stage1: RAWCAP  -> Dump raw
 * Stage2: RAW2RAW -> Parse and Dump cfa stat
 * Stage3: RAW2YUV -> Dump yuv
 *
 * @param [in] info postproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_SingleCaptureContPostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    static UINT8 yuvFlag = 0;
    if (0/*info->StageCnt == 1*/) {
        char fn[MAX_FILENAME_LENGTH];
        AMP_CFS_FILE_s *raw = NULL;
        char mdASCII[3] = {'w','+','\0'};

        yuvFlag = 0;
        //raw ready, dump it
        sprintf(fn, "%s:\\%04d.RAW", DefaultSlot, RawFno);

        raw = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
        AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d %d  to %s Start!", \
            info->media.RawInfo.RawAddr, \
            info->media.RawInfo.RawPitch, \
            info->media.RawInfo.RawWidth, \
            info->media.RawInfo.RawHeight, fn);
        UT_StillEncodefwrite((const void *)info->media.RawInfo.RawAddr, 1, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight, (void *)raw);
        UT_StillEncodefsync((void *)raw);
        UT_StillEncodefclose((void *)raw);
        RawFno++;
    } else if (info->StageCnt == 2) {
#ifdef CONFIG_SOC_A12
        AmpUT_StillEncPostWBCalculation(info->media.CfaStatInfo);
#endif
        AmbaPrint("[Amp_UT] raw2RawPPCB: cfa index %d cfa addr 0x%X", info->CfaIndex, info->media.CfaStatInfo);
        if (info->CfaIndex == TileNumber-1) {
            // whole CFA done, release buffer
            if (Raw3ARoiBuffAddr) {
            #ifndef _STILL_BUFFER_FROM_DSP_
                if (AmbaKAL_BytePoolFree((void *)OriRaw3ARoiBuffAddr) != OK)
                    AmbaPrint("[Amp_UT] memFree Fail raw3AROI!");
            #endif
                Raw3ARoiBuffAddr = NULL;

            }
            if (Raw3AStatBuffAddr) {
            #ifndef _STILL_BUFFER_FROM_DSP_
                if (AmbaKAL_BytePoolFree((void *)OriRaw3AStatBuffAddr) != OK)
                    AmbaPrint("[Amp_UT] memFree Fail raw3AStat!");
            #endif
                Raw3AStatBuffAddr = NULL;
            }
        }

    } else if (info->StageCnt == 3) {
        char fn[MAX_FILENAME_LENGTH];
        char fn1[MAX_FILENAME_LENGTH];
        UINT8 *lumaAddr = NULL, *chromaAddr = NULL;
        UINT16 pitch = 0, width = 0, height = 0;

        if (info->media.YuvInfo.ThmLumaAddr) {
            sprintf(fn, "%s:\\%04d_t.y", DefaultSlot, YuvFno);
            sprintf(fn1, "%s:\\%04d_t.uv", DefaultSlot, YuvFno);
            lumaAddr = info->media.YuvInfo.ThmLumaAddr;
            chromaAddr = info->media.YuvInfo.ThmChromaAddr;
            pitch = info->media.YuvInfo.ThmPitch;
            width = info->media.YuvInfo.ThmWidth;
            height = info->media.YuvInfo.ThmHeight;
        } else if (info->media.YuvInfo.ScrnLumaAddr) {
            sprintf(fn, "%s:\\%04d_s.y", DefaultSlot, YuvFno);
            sprintf(fn1, "%s:\\%04d_s.uv", DefaultSlot, YuvFno);
            lumaAddr = info->media.YuvInfo.ScrnLumaAddr;
            chromaAddr = info->media.YuvInfo.ScrnChromaAddr;
            pitch = info->media.YuvInfo.ScrnPitch;
            width = info->media.YuvInfo.ScrnWidth;
            height = info->media.YuvInfo.ScrnHeight;
        } else if (info->media.YuvInfo.LumaAddr) {
            sprintf(fn, "%s:\\%04d_m.y", DefaultSlot, YuvFno);
            sprintf(fn1, "%s:\\%04d_m.uv", DefaultSlot, YuvFno);
            lumaAddr = info->media.YuvInfo.LumaAddr;
            chromaAddr = info->media.YuvInfo.ChromaAddr;
            pitch = info->media.YuvInfo.Pitch;
            width = info->media.YuvInfo.Width;
            height = info->media.YuvInfo.Height;
        }

        AmbaPrint("[Amp_UT] Dump YUV(%s) (0x%X 0x%X) %d %d %d Start!", \
            (info->media.YuvInfo.DataFormat)?"422":"420", \
            lumaAddr, chromaAddr, pitch, width, height);
#if 0
        {
            AMP_CFS_FILE_s *y = NULL;
            AMP_CFS_FILE_s *uv = NULL;

            y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
            UT_StillEncodefwrite((const void *)lumaAddr, 1, pitch*height, (void *)y);
            UT_StillEncodefsync((void *)y);
            UT_StillEncodefclose((void *)y);

            uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
            UT_StillEncodefwrite((const void *)chromaAddr, 1, pitch*height, (void *)uv);
            UT_StillEncodefsync((void *)uv);
            UT_StillEncodefclose((void *)uv);
        }
#endif
        //release yuv buffers
        if (info->media.YuvInfo.ThmLumaAddr) {
            yuvFlag |= 0x4;
        } else if (info->media.YuvInfo.ScrnLumaAddr) {
            yuvFlag |= 0x2;
        } else if (info->media.YuvInfo.LumaAddr) {
            yuvFlag |= 0x1;
        }

        if (yuvFlag == 0x7) {
            //reset yuv flag
            yuvFlag = 0x0;
            //release raw buffers
            if (RawBuffAddr) {
            #ifndef _STILL_BUFFER_FROM_DSP_
                if (AmbaKAL_BytePoolFree((void *)OriRawBuffAddr) != OK)
                    AmbaPrint("[Amp_UT] memFree Fail raw!");
            #endif
                RawBuffAddr = NULL;
            }

            YuvFno++;
        }
    } else {
        //nothing to do
    }

    return 0;
}

/**
 * UnitTest: Pre-Capture Preproc Callback

 * Stage1: RAWCAP  -> Nothing to do
 * Stage2: RAW2YUV -> Setup Idsp Param
 * Stage3: YUV2JPG -> Nothing to do
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_PreCapturePreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 2) {
    #ifdef CONFIG_SOC_A9
        AmpUT_StillEncIdspParamSetup(info->AeIdx, StillEncMgt[StillEncModeIdx].StillCaptureWidth, StillEncMgt[StillEncModeIdx].StillCaptureHeight);
    #endif
    } else if (info->StageCnt == 3) {
        AmbaPrint("[BurstCaptureCont]PreP SerialNum %d", info->JpegSerialNumber);
    }
    return 0;
}

/**
 * UnitTest: Pre-Capture PostProc Callback
 * Stage1: RAWCAP  -> Nothing to do
 * Stage3: RAW2YUV -> Nothing to do
 * Stage4: YUV2JPG -> Nothing to do
 * @param [in] info postproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_PreCapturePostCB(AMP_STILLENC_POSTP_INFO_s *info)
{

    return 0;
}

/**
 * UnitTest: simple Yuv2Jpeg
 *
 * @param [in] inputID group ID of yuv set(main/scrn/thm)
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_pivYuv2Jpeg(UINT32 inputID, UINT8 encID, UINT32 iso, UINT32 targetSize, UINT8 encodeLoop)
{
    int er = OK;
    void *tempPtr;
    void *TempRawPtr;
    UINT16 yuvWidth = 0, yuvHeight = 0;
    UINT32 yuvSize = 0, scrnSize = 0, thmSize = 0;
    UINT8 *stageAddr = NULL;
    UINT32 totalScriptSize = 0, totalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *genScrpt;
    AMP_SENC_SCRPT_YUV2JPG_s *yuv2JpgScrpt;
    AMP_SCRPT_CONFIG_s scrpt;

    if (StillRawCaptureRunning) {
        AmbaPrint("Error status");
        goto _DONE;
    }

    StillEncModeIdx = encID;

    /* Step1. calc yuv buffer memory */
    //FastMode need 16_align enc_height
    if (iso == 2) {
        yuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
        yuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
        yuvSize = yuvWidth*yuvHeight*2;
        AmbaPrint("[UT_pivYuv2Jpeg]yuv(%u %u %u)!", StillEncMgt[encID].StillMainWidth, yuvHeight, yuvSize);
        scrnSize = StillEncMgt[encID].ScrnWidth*StillEncMgt[encID].ScrnHeight*2;
        thmSize = StillEncMgt[encID].ThmWidth*StillEncMgt[encID].ThmHeight*2;
        AmbaPrint("[UT_pivYuv2Jpeg]scrn(%u) thm(%u)!", scrnSize, thmSize);
    } else {
        yuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
        yuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
        yuvSize = yuvWidth*yuvHeight*2;
        AmbaPrint("[UT_pivYuv2Jpeg]yuv(%u %u %u)!", StillEncMgt[encID].StillMainWidth, yuvSize, yuvSize);
        scrnSize = StillEncMgt[encID].ScrnWidth*ALIGN_16(StillEncMgt[encID].ScrnHeight)*2;
        thmSize = StillEncMgt[encID].ThmWidth*ALIGN_16(StillEncMgt[encID].ThmHeight)*2;
        AmbaPrint("[UT_pivYuv2Jpeg]scrn(%u) thm(%u)!", scrnSize, thmSize);
    }

    /* Step2. allocate yuv/scrn/thm buffer address, script address */
#ifdef _STILL_BUFFER_FROM_DSP_
    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        UINT8 *bufAddr;
        int rt = 0;

        rt = AmpUT_StillEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        if (rt == -1) goto _DONE;

        bufAddr = dspWorkAddr + dspWorkSize;
        YuvBuffAddr = bufAddr;
        bufAddr += yuvSize;
        AmbaPrint("[UT_pivYuv2Jpeg]yuvBuffAddr (0x%08X)!", YuvBuffAddr);

        ScrnBuffAddr = bufAddr;
        bufAddr += scrnSize;
        AmbaPrint("[UT_pivYuv2Jpeg]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);

        ThmBuffAddr = bufAddr;
        bufAddr += thmSize;
        AmbaPrint("[UT_pivYuv2Jpeg]thmBuffAddr (0x%08X)!", ThmBuffAddr);
    }
#else
    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, yuvSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_pivYuv2Jpeg]Cache_DDR alloc yuv fail (%u)!", yuvSize);
    } else {
        YuvBuffAddr = (UINT8*)tempPtr;
        OriYuvBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_pivYuv2Jpeg]yuvBuffAddr (0x%08X)!", YuvBuffAddr);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, scrnSize*1, 32);
    if (er != OK) {
        AmbaPrint("[UT_pivYuv2Jpeg]Cache_DDR alloc yuv fail (%u)!", scrnSize*1);
    } else {
        ScrnBuffAddr = (UINT8*)tempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_pivYuv2Jpeg]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, thmSize*1, 32);
    if (er != OK) {
        AmbaPrint("[UT_pivYuv2Jpeg]Cache_DDR alloc yuv fail (%u)!", thmSize*1);
    } else {
        ThmBuffAddr = (UINT8*)tempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_pivYuv2Jpeg]thmBuffAddr (0x%08X)!", ThmBuffAddr);
    }
#endif

    //read yuv_main buffer
    {
        char fn[32];
        char fn1[32];
        AMP_CFS_FILE_s *y = NULL;
        AMP_CFS_FILE_s *uv = NULL;

        char mdASCII[3] = {'r','+','\0'};

        sprintf(fn,"%s:\\%04d_m.y", DefaultSlot, (int)inputID);
        sprintf(fn1,"%s:\\%04d_m.uv", DefaultSlot, (int)inputID);

        y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
        AmbaPrint("[UT_pivYuv2Jpeg]Read y 0x%X %d %d from %s Start!", \
                YuvBuffAddr, StillEncMgt[encID].StillMainWidth, yuvHeight, fn);
        UT_StillEncodefread((void *)YuvBuffAddr, 1, (yuvSize>>1), (void *)y);
        UT_StillEncodefclose((void *)y);
        AmbaCache_Clean((void *)YuvBuffAddr, (yuvSize>>1));

        uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
        AmbaPrint("[UT_pivYuv2Jpeg]Read uv 0x%X %d %d from %s Start!", \
                YuvBuffAddr+(yuvSize>>1), StillEncMgt[encID].StillMainWidth, yuvHeight, fn1);
        UT_StillEncodefread((void *)YuvBuffAddr+(yuvSize>>1), 1, (yuvSize>>1), (void *)uv);
        UT_StillEncodefclose((void *)uv);
        AmbaCache_Clean((void *)YuvBuffAddr+(yuvSize>>1), (yuvSize>>1));

        AmbaPrint("[UT_pivYuv2Jpeg]Read Yuv Done!");
    }

    //read yuv_scrn buffer
    {
        char fn[32];
        char fn1[32];
        AMP_CFS_FILE_s *y = NULL;
        AMP_CFS_FILE_s *uv = NULL;

        char mdASCII[3] = {'r','+','\0'};

        sprintf(fn,"%s:\\%04d_s.y", DefaultSlot, (int)inputID);
        sprintf(fn1,"%s:\\%04d_s.uv", DefaultSlot, (int)inputID);

        y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
        AmbaPrint("[UT_pivYuv2Jpeg]Read y 0x%X %d %d from %s Start!", \
            ScrnBuffAddr, StillEncMgt[encID].ScrnWidth, StillEncMgt[encID].ScrnHeight, fn);
        UT_StillEncodefread((void *)ScrnBuffAddr, 1, (yuvSize>>1), (void *)y);
        UT_StillEncodefclose((void *)y);
        AmbaCache_Clean((void *)ScrnBuffAddr, (scrnSize>>1));

        uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
        AmbaPrint("[UT_pivYuv2Jpeg]Read uv 0x%X %d %d from %s Start!", \
                ScrnBuffAddr+(scrnSize>>1), StillEncMgt[encID].ScrnWidth, StillEncMgt[encID].ScrnHeight, fn1);
        UT_StillEncodefread((void *)ScrnBuffAddr+(scrnSize>>1), 1, (scrnSize>>1), (void *)uv);
        UT_StillEncodefclose((void *)uv);
        AmbaCache_Clean((void *)ScrnBuffAddr+(scrnSize>>1), (scrnSize>>1));

        AmbaPrint("[UT_pivYuv2Jpeg]Read Scrn Done!");
    }

    //read yuv_thm buffer
    {
        char fn[32];
        char fn1[32];
        AMP_CFS_FILE_s *y = NULL;
        AMP_CFS_FILE_s *uv = NULL;
        char mdASCII[3] = {'r','+','\0'};

        sprintf(fn,"%s:\\%04d_t.y", DefaultSlot, (int)inputID);
        sprintf(fn1,"%s:\\%04d_t.uv", DefaultSlot, (int)inputID);

        y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
        AmbaPrint("[UT_pivYuv2Jpeg]Read y 0x%X %d %d from %s Start!", \
                ThmBuffAddr, StillEncMgt[encID].ThmWidth, StillEncMgt[encID].ThmHeight, fn);
        UT_StillEncodefread((void *)ThmBuffAddr, 1, (thmSize>>1), (void *)y);
        UT_StillEncodefclose((void *)y);
        AmbaCache_Clean((void *)ThmBuffAddr, (thmSize>>1));

        uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
        AmbaPrint("[UT_pivYuv2Jpeg]Read uv 0x%X %d %d from %s Start!", \
            ThmBuffAddr+(thmSize>>1), StillEncMgt[encID].ThmWidth, StillEncMgt[encID].ThmHeight, fn1);
        UT_StillEncodefread((void *)ThmBuffAddr+(thmSize>>1), 1, (thmSize>>1), (void *)uv);
        UT_StillEncodefclose((void *)uv);
        AmbaCache_Clean((void *)ThmBuffAddr+(thmSize>>1), (thmSize>>1));

        AmbaPrint("[UT_pivYuv2Jpeg]Read thmb Done!");
    }
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, ScriptSize, 32);
        if (er != OK) {
            AmbaPrint("[UT_pivYuv2Jpeg]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)tempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_pivYuv2Jpeg]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    stageAddr = ScriptAddr;
    genScrpt = (AMP_SENC_SCRPT_GENCFG_s *)stageAddr;
    memset(genScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    genScrpt->Cmd = SENC_GENCFG;
    genScrpt->RawEncRepeat = 0;
    genScrpt->RawToCap = 1;
    genScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    genScrpt->EncRotateFlip = StillEncRotate;
#endif
    genScrpt->QVConfig.DisableLCDQV = genScrpt->QVConfig.DisableHDMIQV = 1;
    QvLcdEnable = 0;
    QvTvEnable = 0;
    genScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (genScrpt->b2LVCfg)? 1: 0;
    genScrpt->ScrnEnable = 1;
    genScrpt->ThmEnable = 1;
    genScrpt->PostProc = NULL;

    genScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    genScrpt->MainBuf.Width = genScrpt->MainBuf.Pitch = yuvWidth;
    genScrpt->MainBuf.Height = yuvHeight;
    genScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    genScrpt->MainBuf.ChromaAddr = 0; // Behind Luma
    genScrpt->MainBuf.AOI.X = 0;
    genScrpt->MainBuf.AOI.Y = 0;
    genScrpt->MainBuf.AOI.Width = StillEncMgt[encID].StillMainWidth;
    genScrpt->MainBuf.AOI.Height = StillEncMgt[encID].StillMainHeight;
    genScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ScrnBuf.Width = StillEncMgt[encID].ScrnWidth;
    genScrpt->ScrnBuf.Height = StillEncMgt[encID].ScrnHeight;
    genScrpt->ScrnBuf.Pitch = genScrpt->ScrnBuf.Width;
    genScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    genScrpt->ScrnBuf.ChromaAddr = 0; //just behind luma
    genScrpt->ScrnBuf.AOI.X = 0;
    genScrpt->ScrnBuf.AOI.Y = 0;
    genScrpt->ScrnBuf.AOI.Width = StillEncMgt[encID].ScrnWidthAct;
    genScrpt->ScrnBuf.AOI.Height = StillEncMgt[encID].ScrnHeightAct;
    genScrpt->ScrnWidth = StillEncMgt[encID].ScrnWidth;
    genScrpt->ScrnHeight = StillEncMgt[encID].ScrnHeight;
    genScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ThmBuf.Width = StillEncMgt[encID].ThmWidth;
    genScrpt->ThmBuf.Height = StillEncMgt[encID].ThmHeight;
    genScrpt->ThmBuf.Pitch = genScrpt->ThmBuf.Width;
    genScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    genScrpt->ThmBuf.ChromaAddr = 0; //just behind luma
    genScrpt->ThmBuf.AOI.X = 0;
    genScrpt->ThmBuf.AOI.Y = 0;
    genScrpt->ThmBuf.AOI.Width = StillEncMgt[encID].ThmWidthAct;
    genScrpt->ThmBuf.AOI.Height = StillEncMgt[encID].ThmHeightAct;
    genScrpt->ThmWidth = StillEncMgt[encID].ThmWidth;
    genScrpt->ThmHeight = StillEncMgt[encID].ThmHeight;
    AmpUT_StillEncSetJpegBrc(genScrpt, targetSize, encodeLoop);

    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    totalStageNum ++;
    AmbaPrint("[UT_pivYuv2Jpeg]Stage_0 0x%08X", stageAddr);

    //yuv2jpg config
    stageAddr = ScriptAddr + totalScriptSize;
    yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)stageAddr;
    memset(yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    yuv2JpgScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    totalStageNum ++;
    AmbaPrint("[UT_pivYuv2Jpeg]Stage_1 0x%X", stageAddr);

    //script config
    scrpt.mode = AMP_SCRPT_MODE_STILL;
    scrpt.StepPreproc = NULL;
    scrpt.StepPostproc = NULL;
    scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    scrpt.ScriptTotalSize = totalScriptSize;
    scrpt.ScriptStageNum = totalStageNum;
    AmbaPrint("[UT_pivYuv2Jpeg]Scrpt addr 0x%X, Sz %uByte, stg %d", scrpt.ScriptStartAddr, scrpt.ScriptTotalSize, scrpt.ScriptStageNum);

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    AmpEnc_StopLiveview(StillEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
    Status = STATUS_STILL_RAWENCODE;
    StillBGProcessing = 1;
    StillPivProcess = 1;

    /* Step4. execute script */
    AmpEnc_RunScript(StillEncPipe, &scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step4. release script */
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

_DONE:

    return er;
}

/**
 * UnitTest: simple raw to yuv
 *
 * @param [in] inputID ID of raw
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] bits raw data bits
 * @param [in] bayer raw bayer pattern
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_pivRaw2Yuv(UINT32 inputID, UINT8 encID, UINT32 iso, UINT8 cmpr, UINT8 bits, UINT8 bayer)
{
    int Er = OK;
    void *TempPtr = NULL;
    void *TempRawPtr = NULL;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
    UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
    UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
    UINT32 QvLCDSize = 0, QvHDMISize = 0;
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0;
    AMP_SCRPT_CONFIG_s Scrpt = {0};
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt = NULL;
    AMP_SENC_SCRPT_RAW2YUV_s *Raw2YuvScrpt = NULL;
    UINT8 ArIndex = 0;

    /* Phase I */
    /* check still codec status */
    if (StillCodecInit == 0) { //not init yet
        AmpUT_StillEnc_Init(-1, 0);
    }

    if (StillEncPri == NULL) { //no codec be create
        AMP_STILLENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s Layer = {0};

        EncCfg.MainLayout.Layer = &Layer;
        AmpStillEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_StillEncCallback;

        // Assign bitstream/descriptor buffer
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&BitsBuf, (void **)&TempRawPtr, STILL_BISFIFO_SIZE, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }
        EncCfg.BitsBufCfg.BitsBufAddr = BitsBuf;
        EncCfg.BitsBufCfg.BitsBufSize = STILL_BISFIFO_SIZE;

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&DescBuf, (void **)&TempRawPtr, STILL_DESC_SIZE, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for DescFifo!!");
        }
        EncCfg.BitsBufCfg.DescBufAddr = DescBuf;
        EncCfg.BitsBufCfg.DescBufSize = STILL_DESC_SIZE;
        EncCfg.BitsBufCfg.BitsRunoutThreshold = STILL_BISFIFO_SIZE - 4*1024*1024; // leave 4MB
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", BitsBuf, STILL_BISFIFO_SIZE, DescBuf, STILL_DESC_SIZE);
        AmpStillEnc_Create(&EncCfg, &StillEncPri);

        // create a virtual fifo
        if (StillEncVirtualFifoHdlr == NULL) {
            AMP_FIFO_CFG_s FifoDefCfg = {0};

            AmpFifo_GetDefaultCfg(&FifoDefCfg);
            FifoDefCfg.hCodec = StillEncPri;
            FifoDefCfg.IsVirtual = 1;
            FifoDefCfg.NumEntries = 1024;
            FifoDefCfg.cbEvent = AmpUT_StillEnc_FifoCB;
            AmpFifo_Create(&FifoDefCfg, &StillEncVirtualFifoHdlr);
        }
    }

    if (StillEncPipe == NULL) { //no pipeline be create
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};

        AmpEnc_GetDefaultCfg(&PipeCfg);
        PipeCfg.encoder[0] = StillEncPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_StillEncPipeCallback;
        PipeCfg.type = AMP_ENC_STILL_PIPE;
        AmpEnc_Create(&PipeCfg, &StillEncPipe);

        AmpEnc_Add(StillEncPipe);
    }

    G_raw_cmpr = cmpr;
    StillEncModeIdx = encID;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /* Phase II */
    /* fill script and run */
    if (StillRawCaptureRunning) {
        AmbaPrint("Error status");
        goto _DONE;
    }

    /* Step1. calc raw and yuv buffer memory */
    if (StillOBModeEnable == 1) {
        AMBA_SENSOR_MODE_ID_u SensorMode = {0};
        AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
        SensorMode.Data = StillEncMgt[encID].InputStillMode;
        AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);
        RawWidth = SensorModeInfo.OutputInfo.OutputWidth;
        RawHeight = SensorModeInfo.OutputInfo.OutputHeight;
    } else {
        RawWidth =  StillEncMgt[encID].StillCaptureWidth;
        RawHeight = StillEncMgt[encID].StillCaptureHeight;
    }
    RawPitch = ALIGN_32((cmpr)? AMP_COMPRESSED_RAW_WIDTH(RawWidth): (RawWidth<<1));
    RawSize = RawPitch*RawHeight;
    AmbaPrint("[UT_pivRaw2Yuv]raw(%u %u %u)", RawPitch, RawWidth, RawHeight);

    //FastMode need 16_align enc_height
    if (iso == 2) {
    #ifdef CONFIG_SOC_A9
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32((StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth);
        YuvHeight = ALIGN_16((StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
    #else
        YuvWidth = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth;
        YuvHeight = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight;
        AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = StillEncMgt[encID].ScrnHeight;
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = StillEncMgt[encID].ThmHeight;
    #endif
        AmbaPrint("[UT_pivRaw2Yuv]yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);
        ScrnSize = ScrnW*ScrnH*2;
        ThmSize = ThmW*ThmH*2;
        AmbaPrint("[UT_pivRaw2Yuv]scrn(%d %d %u) thm(%d %d %u)!", ScrnW, ScrnH, ScrnSize, ThmW, ThmH, ThmSize);
    } else {
    #ifdef CONFIG_SOC_A9
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32((StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth);
        YuvHeight = ALIGN_16((StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = StillEncMgt[encID].ScrnHeight;
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = StillEncMgt[encID].ThmHeight;
    #else
        YuvWidth = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth;
        YuvHeight = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight;
        AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = StillEncMgt[encID].ScrnHeight;
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = StillEncMgt[encID].ThmHeight;
    #endif
        YuvSize += (YuvSize*10)/100;
        AmbaPrint("[UT_pivRaw2Yuv]yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);

        ScrnSize = ScrnW*ScrnH*2;
        ScrnSize += (ScrnSize*10)/100;
        ThmSize = ThmW*ThmH*2;
        ThmSize += (ThmSize*10)/100;
        AmbaPrint("[UT_pivRaw2Yuv]scrn(%d %d %u) thm(%d %d %u)!", ScrnW, ScrnH, ScrnSize, ThmW, ThmH, ThmSize);
    }


#ifdef CONFIG_SOC_A12
    QvLCDW = StillVoutMgt[0][ArIndex].Width;
    QvLCDH = StillVoutMgt[0][ArIndex].Height;
    QvHDMIW = StillVoutMgt[1][ArIndex].Width;
    QvHDMIH = StillVoutMgt[1][ArIndex].Height;
#else
    /* QV need Alignment */
    QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
    QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
    QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
    QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
#endif
    QvLCDSize = QvLCDW*QvLCDH*2;
    QvLCDSize += (QvLCDSize*15)/100;
    QvHDMISize = QvHDMIW*QvHDMIH*2;
    QvHDMISize += (QvHDMISize*15)/100;
    AmbaPrint("[UT_pivRaw2Yuv]qvLCD(%u) qvHDMI(%u)!", QvLCDSize, QvHDMISize);


    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    #if 1
    /* allocate for QV show buffer before working buffer */
    AmpUT_qvShowBufferAllocate(QvLCDSize, QvHDMISize);
    #endif

#ifdef _STILL_BUFFER_FROM_DSP_
    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        UINT8 *bufAddr;
        int rt = 0;

        rt = AmpUT_StillEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        if (rt == -1) goto _DONE;

        bufAddr = dspWorkAddr + dspWorkSize;
        RawBuffAddr = bufAddr;
        bufAddr += RawSize;
        AmbaPrint("[UT_pivRaw2Yuv]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);

        YuvBuffAddr = bufAddr;
        bufAddr += YuvSize;
        AmbaPrint("[UT_pivRaw2Yuv]yuvBuffAddr (0x%08X)!", YuvBuffAddr);

        ScrnBuffAddr = bufAddr;
        bufAddr += ScrnSize;
        AmbaPrint("[UT_pivRaw2Yuv]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);

        ThmBuffAddr = bufAddr;
        bufAddr += ThmSize;
        AmbaPrint("[UT_pivRaw2Yuv]thmBuffAddr (0x%08X)!", ThmBuffAddr);

        QvLCDBuffAddr = bufAddr;
        bufAddr += QvLCDSize;
        AmbaPrint("[UT_pivRaw2Yuv]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);

        QvHDMIBuffAddr = bufAddr;
        AmbaPrint("[UT_pivRaw2Yuv]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#else
    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, RawSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_pivRaw2Yuv]Cache_DDR alloc raw fail (%u)!", RawSize);
    } else {
        RawBuffAddr = (UINT8*)tempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_pivRaw2Yuv]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, YuvSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_pivRaw2Yuv]Cache_DDR alloc yuv fail (%u)!", YuvSize);
    } else {
        YuvBuffAddr = (UINT8*)tempPtr;
        OriYuvBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_pivRaw2Yuv]yuvBuffAddr (0x%08X)!", YuvBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ScrnSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_pivRaw2Yuv]Cache_DDR alloc scrn fail (%u)!", ScrnSize*1);
    } else {
        ScrnBuffAddr = (UINT8*)tempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_pivRaw2Yuv]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ThmSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_pivRaw2Yuv]Cache_DDR alloc thm fail (%u)!", ThmSize*1);
    } else {
        ThmBuffAddr = (UINT8*)TempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_pivRaw2Yuv]thmBuffAddr (0x%08X)!", ThmBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, QvLCDSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCapture]Cache_DDR alloc yuv_lcd fail (%u)!", QvLCDSize*1);
    } else {
        QvLCDBuffAddr = (UINT8*)TempPtr;
        OriQvLCDBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCapture]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, QvHDMISize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCapture]Cache_DDR alloc yuv_hdmi fail (%u)!", QvHDMISize*1);
    } else {
        QvHDMIBuffAddr = (UINT8*)TempPtr;
        OriQvHDMIBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCapture]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#endif
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ScriptSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_pivRaw2Yuv]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)TempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_pivRaw2Yuv]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    //read raw buffer
    {
        char fn[32];
        AMP_CFS_FILE_s *raw = NULL;
        char mdASCII[3] = {'r','+','\0'};

        sprintf(fn,"%s:\\%04d.raw", DefaultSlot, (int)inputID);

        raw = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
        AmbaPrint("[UT_pivRaw2Yuv]Read raw 0x%X %d %d from %s Start!", RawBuffAddr, RawPitch, RawHeight, fn);
        UT_StillEncodefread((void *)RawBuffAddr, 1, RawPitch*RawHeight, (void *)raw);
        UT_StillEncodefclose((void *)raw);
        AmbaCache_Clean((void *)RawBuffAddr, RawPitch*RawHeight);
        AmbaPrint("[UT_pivRaw2Yuv]Read Raw Done!");
    }

    /* Step3. fill script */
    //general config
    StageAddr = ScriptAddr;
    GenScrpt = (AMP_SENC_SCRPT_GENCFG_s *)StageAddr;
    memset(GenScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    GenScrpt->Cmd = SENC_GENCFG;
    GenScrpt->RawEncRepeat = 0;
    GenScrpt->RawToCap = 1;
    GenScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    GenScrpt->EncRotateFlip = StillEncRotate;
#endif
    GenScrpt->QVConfig.DisableLCDQV = (QvDisplayCfg == 0)? 1: 0;
    GenScrpt->QVConfig.DisableHDMIQV = (QvDisplayCfg == 0)? 1: ((TVLiveViewEnable)? 0: 1);
    QvLcdEnable = (QvDisplayCfg == 0)? 0: 1;
    QvTvEnable = (QvDisplayCfg == 0)? 0: ((TVLiveViewEnable)? 1: 0);
    GenScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    GenScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + QvLCDSize/2;
    GenScrpt->QVConfig.LCDWidth = StillVoutMgt[0][ArIndex].Width;
    GenScrpt->QVConfig.LCDHeight = StillVoutMgt[0][ArIndex].Height;
    GenScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    GenScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + QvHDMISize/2;
    GenScrpt->QVConfig.HDMIWidth = StillVoutMgt[1][ArIndex].Width;
    GenScrpt->QVConfig.HDMIHeight = StillVoutMgt[1][ArIndex].Height;
    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (GenScrpt->b2LVCfg)? 1: 0;
    GenScrpt->ScrnEnable = 1;
    GenScrpt->ThmEnable = 1;
    GenScrpt->PostProc = &PostPivRaw2YuvCB;
    GenScrpt->PreProc = &PrePivRaw2YuvCB;

    GenScrpt->RawDataBits = bits;
    GenScrpt->RawBayerPattern = bayer;
    GenScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->MainBuf.Width = GenScrpt->MainBuf.Pitch = YuvWidth;
    GenScrpt->MainBuf.Height = YuvHeight;
    GenScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    GenScrpt->MainBuf.ChromaAddr = GenScrpt->MainBuf.LumaAddr + YuvSize/2;
    GenScrpt->MainBuf.AOI.X = 0;
    GenScrpt->MainBuf.AOI.Y = 0;
    GenScrpt->MainBuf.AOI.Width = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth;
    GenScrpt->MainBuf.AOI.Height =(StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight;

    GenScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ScrnBuf.Width = GenScrpt->ScrnBuf.Pitch = ScrnW;
    GenScrpt->ScrnBuf.Height = ScrnH;
    GenScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    GenScrpt->ScrnBuf.ChromaAddr = GenScrpt->ScrnBuf.LumaAddr + ScrnSize/2;
    GenScrpt->ScrnBuf.AOI.X = 0;
    GenScrpt->ScrnBuf.AOI.Y = 0;
    GenScrpt->ScrnBuf.AOI.Width = StillEncMgt[encID].ScrnWidthAct;
    GenScrpt->ScrnBuf.AOI.Height = StillEncMgt[encID].ScrnHeightAct;
    GenScrpt->ScrnWidth = StillEncMgt[encID].ScrnWidth;
    GenScrpt->ScrnHeight = StillEncMgt[encID].ScrnHeight;

    GenScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ThmBuf.Width = GenScrpt->ThmBuf.Pitch = ThmW;
    GenScrpt->ThmBuf.Height = ThmH;
    GenScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    GenScrpt->ThmBuf.ChromaAddr = GenScrpt->ThmBuf.LumaAddr + ThmSize/2;
    GenScrpt->ThmBuf.AOI.X = 0;
    GenScrpt->ThmBuf.AOI.Y = 0;
    GenScrpt->ThmBuf.AOI.Width = StillEncMgt[encID].ThmWidthAct;
    GenScrpt->ThmBuf.AOI.Height = StillEncMgt[encID].ThmHeightAct;
    GenScrpt->ThmWidth = StillEncMgt[encID].ThmWidth;
    GenScrpt->ThmHeight = StillEncMgt[encID].ThmHeight;

    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_pivRaw2Yuv]Stage_0 0x%08X", StageAddr);

    //raw2yuv config
    StageAddr = ScriptAddr + TotalScriptSize;
    Raw2YuvScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)StageAddr;
    memset(Raw2YuvScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    Raw2YuvScrpt->Cmd = SENC_RAW2YUV;
    Raw2YuvScrpt->RawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    Raw2YuvScrpt->RawBuf.Buf = RawBuffAddr;
    Raw2YuvScrpt->RawBuf.Width = RawWidth;
    Raw2YuvScrpt->RawBuf.Height = RawHeight;
    Raw2YuvScrpt->RawBuf.Pitch = RawPitch;
    Raw2YuvScrpt->RawBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    Raw2YuvScrpt->RingBufSize = 0;
    Raw2YuvScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    Raw2YuvScrpt->YuvRingBufSize = 0;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    TotalStageNum ++;
    AmbaPrint("[UT_pivRaw2Yuv]Stage_1 0x%X", StageAddr);

    //script config
    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[UT_pivRaw2Yuv]Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    AmpEnc_StopLiveview(StillEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
    Status = STATUS_STILL_RAWENCODE;
    StillBGProcessing = 1;
    StillPivProcess = 1;

    /* Step5. execute script */
    AmpEnc_RunScript(StillEncPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step6. release script */
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

_DONE:
    return Er;
}

/**
 * UnitTest: simple raw capture
 *
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_rawcap(UINT8 encID, UINT32 iso, UINT8 cmpr)
{
    int Er = OK;
    void *TempPtr = NULL;
    void *TempRawPtr = NULL;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT32 RawSize = 0;
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0;
    AMP_SCRPT_CONFIG_s Scrpt = {0};
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt;
    AMP_SENC_SCRPT_RAWCAP_s *RawCapScrpt;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};

    if (StillRawCaptureRunning) {
        AmbaPrint("Error status");
        goto _DONE;
    }

    /* Step1. calc raw buffer memory */
    if (StillOBModeEnable == 1) {
        AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
        SensorMode.Data = StillEncMgt[encID].InputStillMode;
        AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);
        RawWidth = SensorModeInfo.OutputInfo.OutputWidth;
        RawHeight = SensorModeInfo.OutputInfo.OutputHeight;
    } else {
        RawWidth = StillEncMgt[encID].StillCaptureWidth;
        RawHeight = StillEncMgt[encID].StillCaptureHeight;
    }
    RawPitch = (cmpr)? AMP_COMPRESSED_RAW_WIDTH(RawWidth) :RawWidth*2;
    RawPitch = ALIGN_32(RawPitch);
    RawSize = RawPitch*RawHeight;
    AmbaPrint("[UT_rawcap]raw(%u %u %u)", RawPitch, RawWidth, RawHeight);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, RawSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_rawcap]C_DDR alloc raw fail (%u)!", RawSize);
    } else {
        RawBuffAddr = (UINT8*)TempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_rawcap]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);
    }
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ScriptSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_rawcap]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)TempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_rawcap]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
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
    GenScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    GenScrpt->EncRotateFlip = StillEncRotate;
#endif
    GenScrpt->QVConfig.DisableLCDQV = 1;
    GenScrpt->QVConfig.DisableHDMIQV = 1;
    QvLcdEnable = 0;
    QvTvEnable = 0;
    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (GenScrpt->b2LVCfg)? 1: 0;
    GenScrpt->ScrnEnable = 1;
    GenScrpt->ThmEnable = 1;
    GenScrpt->PostProc = &PostRawCapCB;
    GenScrpt->PreProc = &PreRawCapCB;
#ifdef CONFIG_SOC_A12
    GenScrpt->MainBuf.AOI.Width = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth; // sanity check for A12 HISO
    GenScrpt->MainBuf.AOI.Height = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight;
#endif
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum++;
    AmbaPrint("[UT_rawcap]Stage_0 0x%08X, PP 0x%08X", StageAddr, GenScrpt->PostProc);

    //raw cap config
    StageAddr = ScriptAddr + TotalScriptSize;
    RawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)StageAddr;
    memset(RawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    RawCapScrpt->Cmd = SENC_RAWCAP;
#ifdef CONFIG_SOC_A12
    RawCapScrpt->EnableOB = (StillOBModeEnable == 1)?1:0;
#endif
    RawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    RawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    SensorMode.Data = StillEncMgt[encID].InputStillMode;
    RawCapScrpt->SensorMode = SensorMode;
#ifdef CONFIG_SOC_A9
    RawCapScrpt->SensorMode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
#endif
    RawCapScrpt->FvRawCapArea.VcapWidth = RawWidth;
    RawCapScrpt->FvRawCapArea.VcapHeight = RawHeight;
    RawCapScrpt->FvRawCapArea.EffectArea.X = 0;
    RawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    RawCapScrpt->FvRawCapArea.EffectArea.Width = RawCapScrpt->FvRawCapArea.VcapWidth;
    RawCapScrpt->FvRawCapArea.EffectArea.Height = RawCapScrpt->FvRawCapArea.VcapHeight;
    RawCapScrpt->FvRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    RawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    RawCapScrpt->FvRawBuf.Width = RawWidth;
    RawCapScrpt->FvRawBuf.Height = RawHeight;
    RawCapScrpt->FvRawBuf.Pitch = RawPitch;
    RawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    RawCapScrpt->FvRingBufSize = RawSize*1;
    RawCapScrpt->CapCB.RawCapCB = AmpUT_StillEncRawCapCB;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    TotalStageNum ++;
    AmbaPrint("[UT_rawcap]Stage_1 0x%X", StageAddr);
    //script config
    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[UT_rawcap]Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    StillRawCaptureRunning = 1;
    Status = STATUS_STILL_RAWCAPTURE;
    StillBGProcessing = 1;
    StillPivProcess = 0;

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    /* Step4. execute script */
    AmpEnc_RunScript(StillEncPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step5. release script */
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

_DONE:
    return Er;
}

/**
 * UnitTest: simple capture(ie rawcap + raw2yuv + yuv2jpeg)
 *
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_SingleCapture(UINT32 encID, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop)
{
    int Er = 0;
    void *TempPtr = NULL, *TempRawPtr = NULL;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
    UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
    UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
    UINT32 QvLCDSize = 0, QvHDMISize = 0;
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0;
    AMP_SCRPT_CONFIG_s Scrpt = {0};
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt;
    AMP_SENC_SCRPT_RAWCAP_s *RawCapScrpt;
    AMP_SENC_SCRPT_RAW2YUV_s *Raw2YuvScrpt;
    AMP_SENC_SCRPT_YUV2JPG_s *Yuv2JpgScrpt;
    UINT8 ArIndex = 0;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};

    if (StillRawCaptureRunning) {
        AmbaPrint("Error status %d", Status);
        goto _DONE;
    }

    G_raw_cmpr = cmpr;
    StillEncModeIdx = encID;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /* Step1. calc raw and yuv buffer memory */
    if (StillOBModeEnable == 1) {
        AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
        SensorMode.Data = StillEncMgt[encID].InputStillMode;
        AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);
        RawWidth = SensorModeInfo.OutputInfo.OutputWidth;
        RawHeight = SensorModeInfo.OutputInfo.OutputHeight;
    } else {
        RawWidth = StillEncMgt[encID].StillCaptureWidth;
        RawHeight = StillEncMgt[encID].StillCaptureHeight;
    }
    RawPitch = (cmpr)? AMP_COMPRESSED_RAW_WIDTH(RawWidth) :RawWidth*2;
    RawSize = RawPitch*RawHeight;
    AmbaPrint("[UT_singleCapture]raw(%u %u %u)", RawPitch, RawWidth, RawHeight);

    //FastMode need 16_align enc_height
    if (iso == 2) {
    #ifdef CONFIG_SOC_A9
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32((StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth);
        YuvHeight = ALIGN_16((StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight);
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
    #else //A12 only need 32Align for Scrn/Thmb Width
        YuvWidth = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth;
        YuvHeight = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight;
        AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);

        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = StillEncMgt[encID].ScrnHeight;
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = StillEncMgt[encID].ThmHeight;
    #endif
        YuvSize = YuvWidth*YuvHeight*2;
        ScrnSize = ScrnW*ScrnH*2;
        ThmSize = ThmW*ThmH*2;
        AmbaPrint("[UT_singleCapture]Yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);
        AmbaPrint("[UT_singleCapture]Scrn(%d %d %u) Thm(%d %d %u)!", ScrnW, ScrnH, ScrnSize, ThmW, ThmH, ThmSize);
    } else {
    #ifdef CONFIG_SOC_A9
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
        YuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        YuvSize += (YuvSize*10)/100;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
    #else //A12 only need 32Align for Scrn/Thmb Width
        YuvWidth = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth;
        YuvHeight = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight;
        AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = StillEncMgt[encID].ScrnHeight;
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = StillEncMgt[encID].ThmHeight;
    #endif
        ScrnSize = ScrnW*ScrnH*2;
        ScrnSize += (ScrnSize*10)/100;
        ThmSize = ThmW*ThmH*2;
        ThmSize += (ThmSize*10)/100;
        AmbaPrint("[UT_singleCapture]yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);
        AmbaPrint("[UT_singleCapture]scrn(%d %d %u) thm(%d %d %u)!", ScrnW, ScrnH, ScrnSize, ThmW, ThmH, ThmSize);
    }

#ifdef CONFIG_SOC_A9
    QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
    QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
    QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
    QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
#else
    /* QV need Alignment */
    QvLCDW = StillVoutMgt[0][ArIndex].Width;
    QvLCDH = StillVoutMgt[0][ArIndex].Height;
    QvHDMIW = StillVoutMgt[1][ArIndex].Width;
    QvHDMIH = StillVoutMgt[1][ArIndex].Height;
#endif
    QvLCDSize = QvLCDW*QvLCDH*2;
    QvLCDSize += (QvLCDSize*15)/100;
    QvHDMISize = QvHDMIW*QvHDMIH*2;
    QvHDMISize += (QvHDMISize*15)/100;
    AmbaPrint("[UT_singleCapture]qvLCD(%u) qvHDMI(%u)!", QvLCDSize, QvHDMISize);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    /* allocate for QV show buffer before working buffer */
    AmpUT_qvShowBufferAllocate(QvLCDSize, QvHDMISize);

#ifdef _STILL_BUFFER_FROM_DSP_
    {
        UINT8 *DspWorkAddr;
        UINT32 DspWorkSize;
        UINT8 *BufAddr;
        UINT32 RoiSize = TileNumber*sizeof(AMP_STILLENC_RAW2RAW_ROI_s);
        UINT32 Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
        int Rt = 0;

        Rt = AmpUT_StillEnc_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
        if (Rt == -1) goto _DONE;

        BufAddr = DspWorkAddr + DspWorkSize;
        RawBuffAddr = BufAddr;
        BufAddr += RawSize;
        AmbaPrint("[UT_singleCapture]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);

        YuvBuffAddr = BufAddr;
        BufAddr += YuvSize;
        AmbaPrint("[UT_singleCapture]yuvBuffAddr (0x%08X)!", YuvBuffAddr);

        ScrnBuffAddr = BufAddr;
        BufAddr += ScrnSize;
        AmbaPrint("[UT_singleCapture]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);

        ThmBuffAddr = BufAddr;
        BufAddr += ThmSize;
        AmbaPrint("[UT_singleCapture]thmBuffAddr (0x%08X)!", ThmBuffAddr);

        QvLCDBuffAddr = BufAddr;
        BufAddr += QvLCDSize;
        AmbaPrint("[UT_singleCapture]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);

        QvHDMIBuffAddr = BufAddr;
        BufAddr += QvHDMISize;
        AmbaPrint("[UT_singleCapture]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);

        if (StillOBModeEnable == 1) {
            Raw3ARoiBuffAddr = BufAddr;
            BufAddr += RoiSize;
            AmbaPrint("[UT_singleCapture]raw3ARoiBuffAddr (0x%08X) (%u)!", Raw3ARoiBuffAddr, RoiSize);

            Raw3AStatBuffAddr = BufAddr;
            BufAddr += Raw3AStatSize;
            AmbaPrint("[UT_singleCapture]raw3AStatBuffAddr (0x%08X) (%u)!", Raw3AStatBuffAddr, Raw3AStatSize);
        }
    }
#else
    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, RawSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCapture]Cache_DDR alloc raw fail (%u)!", RawSize);
    } else {
        RawBuffAddr = (UINT8*)TempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCapture]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, YuvSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCapture]Cache_DDR alloc yuv_main fail (%u)!", YuvSize);
    } else {
        YuvBuffAddr = (UINT8*)TempPtr;
        OriYuvBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCapture]yuvBuffAddr (0x%08X)!", YuvBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ScrnSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCapture]Cache_DDR alloc yuv_scrn fail (%u)!", ScrnSize*1);
    } else {
        ScrnBuffAddr = (UINT8*)TempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCapture]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ThmSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCapture]Cache_DDR alloc yuv_thm fail (%u)!", ThmSize*1);
    } else {
        ThmBuffAddr = (UINT8*)TempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCapture]thmBuffAddr (0x%08X)!", ThmBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, QvLCDSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCapture]Cache_DDR alloc yuv_lcd fail (%u)!", QvLCDSize*1);
    } else {
        QvLCDBuffAddr = (UINT8*)TempPtr;
        OriQvLCDBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCapture]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, QvHDMISize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCapture]Cache_DDR alloc yuv_hdmi fail (%u)!", QvHDMISize*1);
    } else {
        QvHDMIBuffAddr = (UINT8*)TempPtr;
        OriQvHDMIBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCapture]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }

    if (StillOBModeEnable == 1) {
        UINT32 RoiSize = TileNumber*sizeof(AMP_STILLENC_RAW2RAW_ROI_s);
        UINT32 Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, RoiSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_singleCapture]Cache_DDR alloc roi buffer fail (%u)!", RoiSize);
        } else {
            Raw3ARoiBuffAddr = (UINT8*)TempPtr;
            OriRaw3ARoiBuffAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_singleCapture]raw3ARoiBuffAddr (0x%08X) (%u)!", Raw3ARoiBuffAddr, RoiSize);
        }

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, Raw3AStatSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_singleCapture]Cache_DDR alloc 3A stat buffer fail (%u)!", Raw3AStatSize);
        } else {
            Raw3AStatBuffAddr = (UINT8*)TempPtr;
            OriRaw3AStatBuffAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_singleCapture]raw3AStatBuffAddr (0x%08X) (%u)!", Raw3AStatBuffAddr, Raw3AStatSize);
        }
    }

#endif

    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ScriptSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_singleCapture]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)TempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_singleCapture]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    StageAddr = ScriptAddr;
    GenScrpt = (AMP_SENC_SCRPT_GENCFG_s *)StageAddr;
    memset(GenScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    GenScrpt->Cmd = SENC_GENCFG;
    GenScrpt->RawEncRepeat = 0;
    GenScrpt->RawToCap = G_capcnt;
    GenScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    GenScrpt->EncRotateFlip = StillEncRotate;
#endif
    GenScrpt->CapProfEnable = (StillIsCapProfEnable == 1)?1: 0;
    GenScrpt->CapProfShow = (StillIsCapProfEnable == 1)?1: 0;
    GenScrpt->QVConfig.DisableLCDQV = (QvDisplayCfg == 0)? 1: 0;
    GenScrpt->QVConfig.DisableHDMIQV = (QvDisplayCfg == 0)? 1: ((TVLiveViewEnable)? 0: 1);
    QvLcdEnable = (QvDisplayCfg == 0)? 0: 1;
    QvTvEnable = (QvDisplayCfg == 0)? 0: ((TVLiveViewEnable)? 1: 0);

    GenScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    GenScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + QvLCDSize/2;
    GenScrpt->QVConfig.LCDWidth = StillVoutMgt[0][ArIndex].Width;
    GenScrpt->QVConfig.LCDHeight = StillVoutMgt[0][ArIndex].Height;
    GenScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    GenScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + QvHDMISize/2;
    GenScrpt->QVConfig.HDMIWidth = StillVoutMgt[1][ArIndex].Width;
    GenScrpt->QVConfig.HDMIHeight = StillVoutMgt[1][ArIndex].Height;
    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (GenScrpt->b2LVCfg)? 1: 0;
    GenScrpt->ScrnEnable = 1;
    GenScrpt->ThmEnable = 1;
    GenScrpt->PostProc = &PostSingleCapCB;
    GenScrpt->PreProc = &PreSingleCapCB;

    GenScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->MainBuf.Width = GenScrpt->MainBuf.Pitch = YuvWidth;
    GenScrpt->MainBuf.Height = YuvHeight;
    GenScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    GenScrpt->MainBuf.ChromaAddr = GenScrpt->MainBuf.LumaAddr + YuvSize/2;
    GenScrpt->MainBuf.AOI.X = 0;
    GenScrpt->MainBuf.AOI.Y = 0;
    GenScrpt->MainBuf.AOI.Width = (StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[encID].StillMainWidth;
    GenScrpt->MainBuf.AOI.Height = (StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[encID].StillMainHeight;

    GenScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ScrnBuf.Width = GenScrpt->ScrnBuf.Pitch = ScrnW;
    GenScrpt->ScrnBuf.Height = ScrnH;
    GenScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    GenScrpt->ScrnBuf.ChromaAddr = GenScrpt->ScrnBuf.LumaAddr + ScrnSize/2;
    GenScrpt->ScrnBuf.AOI.X = 0;
    GenScrpt->ScrnBuf.AOI.Y = 0;
    GenScrpt->ScrnBuf.AOI.Width = StillEncMgt[encID].ScrnWidthAct;
    GenScrpt->ScrnBuf.AOI.Height = StillEncMgt[encID].ScrnHeightAct;
    GenScrpt->ScrnWidth = StillEncMgt[encID].ScrnWidth;
    GenScrpt->ScrnHeight = StillEncMgt[encID].ScrnHeight;

    GenScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ThmBuf.Width = GenScrpt->ThmBuf.Pitch = ThmW;
    GenScrpt->ThmBuf.Height = ThmH;
    GenScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    GenScrpt->ThmBuf.ChromaAddr = GenScrpt->ThmBuf.ChromaAddr + ThmSize/2; // Behind Luma
    GenScrpt->ThmBuf.AOI.X = 0;
    GenScrpt->ThmBuf.AOI.Y = 0;
    GenScrpt->ThmBuf.AOI.Width = StillEncMgt[encID].ThmWidthAct;
    GenScrpt->ThmBuf.AOI.Height = StillEncMgt[encID].ThmHeightAct;
    GenScrpt->ThmWidth = StillEncMgt[encID].ThmWidth;
    GenScrpt->ThmHeight = StillEncMgt[encID].ThmHeight;
    AmpUT_StillEncSetJpegBrc(GenScrpt, targetSize, encodeLoop);

    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    AmbaPrint("[UT_singleCapture]Stage_%u 0x%08X", TotalStageNum, StageAddr);
    TotalStageNum++;

    //raw cap config
    StageAddr = ScriptAddr + TotalScriptSize;
    RawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)StageAddr;
    memset(RawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    RawCapScrpt->Cmd = SENC_RAWCAP;
#ifdef CONFIG_SOC_A12
    RawCapScrpt->EnableOB = (StillOBModeEnable == 1)?1:0;
#endif
    RawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    RawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    SensorMode.Data = StillEncMgt[encID].InputStillMode;
    RawCapScrpt->SensorMode = SensorMode;
#ifdef CONFIG_SOC_A9
    if (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) RawCapScrpt->SensorMode.Bits.VerticalFlip = 1;
    else RawCapScrpt->SensorMode.Bits.VerticalFlip = 0;
#endif
    RawCapScrpt->FvRawCapArea.VcapWidth = RawWidth;
    RawCapScrpt->FvRawCapArea.VcapHeight = RawHeight;
    RawCapScrpt->FvRawCapArea.EffectArea.X = RawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    RawCapScrpt->FvRawCapArea.EffectArea.Width = RawCapScrpt->FvRawCapArea.VcapWidth;
    RawCapScrpt->FvRawCapArea.EffectArea.Height = RawCapScrpt->FvRawCapArea.VcapHeight;
    RawCapScrpt->FvRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    RawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    RawCapScrpt->FvRawBuf.Width = RawWidth;
    RawCapScrpt->FvRawBuf.Height = RawHeight;
    RawCapScrpt->FvRawBuf.Pitch = RawPitch;
    RawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    RawCapScrpt->FvRingBufSize = RawSize*G_capcnt;
    RawCapScrpt->CapCB.RawCapCB = AmpUT_StillEncRawCapCB;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    AmbaPrint("[UT_singleCapture]Stage_%u 0x%08X", TotalStageNum, StageAddr);
    TotalStageNum++;
#ifdef CONFIG_SOC_A12
    if (RawCapScrpt->EnableOB == 1) {
        StageAddr = ScriptAddr + TotalScriptSize;
        AmpUT_StillEncAttachOBScript((AMP_SENC_SCRPT_RAW2RAW_s *) StageAddr, encID, cmpr);
        TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s));
        AmbaPrint("[UT_singleCapture]Stage_%u 0x%08X", TotalStageNum, StageAddr);
        TotalStageNum++;
    }
#endif
    //raw2yuv config
    StageAddr = ScriptAddr + TotalScriptSize;
    Raw2YuvScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)StageAddr;
    memset(Raw2YuvScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    Raw2YuvScrpt->Cmd = SENC_RAW2YUV;
    Raw2YuvScrpt->RawType = RawCapScrpt->FvRawType;
    Raw2YuvScrpt->RawBuf.Buf = RawCapScrpt->FvRawBuf.Buf;
    Raw2YuvScrpt->RawBuf.Width = RawCapScrpt->FvRawBuf.Width;
    Raw2YuvScrpt->RawBuf.Height = RawCapScrpt->FvRawBuf.Height;
    Raw2YuvScrpt->RawBuf.Pitch = RawCapScrpt->FvRawBuf.Pitch;
    Raw2YuvScrpt->RawBufRule = RawCapScrpt->FvBufRule;
    Raw2YuvScrpt->RingBufSize = 0;
    Raw2YuvScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    Raw2YuvScrpt->YuvRingBufSize = 0;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    AmbaPrint("[UT_singleCapture]Stage_%u 0x%08X", TotalStageNum, StageAddr);
    TotalStageNum++;

    StageAddr = ScriptAddr + TotalScriptSize;
    Yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)StageAddr;
    memset(Yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    Yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    Yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    Yuv2JpgScrpt->YuvRingBufSize = 0;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    AmbaPrint("[UT_singleCapture]Stage_%u 0x%08X", TotalStageNum, StageAddr);
    TotalStageNum ++;

    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[UT_singleCapture]Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    StillRawCaptureRunning = 1;
    Status = STATUS_STILL_RAWCAPTURE;
    StillBGProcessing = 1;
    StillPivProcess = 0;

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    /* Step4. execute script */
    AmpEnc_RunScript(StillEncPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step4. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");

_DONE:
    return 0;
}

/**
 * UnitTest: continuous single capture (ie: repeat (rawcap + raw2yuv + yuv2jpeg))
 *
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_singleCaptureCont(UINT32 encID, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop)
{
    int Er = 0;
    void *TempPtr = NULL, *TempRawPtr = NULL;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
    UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
    UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
    UINT32 QvLCDSize = 0, QvHDMISize = 0, RoiSize = 0, Raw3AStatSize = 0;
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0;
    AMP_SCRPT_CONFIG_s Scrpt = {0};
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt = NULL;
    AMP_SENC_SCRPT_RAWCAP_s *RawCapScrpt = NULL;
    AMP_SENC_SCRPT_RAW2RAW_s *Raw2RawScrpt = NULL;
    AMP_SENC_SCRPT_RAW2YUV_s *Raw2YuvScrpt = NULL;
    AMP_SENC_SCRPT_YUV2JPG_s *Yuv2JpgScrpt = NULL;
    UINT8 ArIndex = 0;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};

    if (StillRawCaptureRunning) {
        AmbaPrintColor(RED, "Still capture is running %u", Status);
        goto _DONE;
    }

    G_raw_cmpr = cmpr;
    StillEncModeIdx = encID;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /* Step1. calc raw and yuv buffer memory */
    RawWidth =  StillEncMgt[encID].StillCaptureWidth;
    RawHeight = StillEncMgt[encID].StillCaptureHeight;
    RawPitch = ALIGN_32((cmpr)? AMP_COMPRESSED_RAW_WIDTH(RawWidth): RawWidth*2);
    RawSize = RawPitch*RawHeight;
    AmbaPrint("[UT_singleCaptureCont]raw(%u %u %u)", RawPitch, RawWidth, RawHeight);

    //FastMode need 16_align enc_height
    if (iso == 2) {
    #ifdef CONFIG_SOC_A9
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
        YuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
    #else
        YuvWidth = StillEncMgt[encID].StillMainWidth;
        YuvHeight = StillEncMgt[encID].StillMainHeight;
        AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = StillEncMgt[encID].ScrnHeight;
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = StillEncMgt[encID].ThmHeight;
    #endif
        AmbaPrint("[UT_singleCaptureCont]yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);
        ScrnSize = ScrnW*ScrnH*2;
        ThmSize = ThmW*ThmH*2;
        AmbaPrint("[UT_singleCaptureCont]scrn(%d %d %u) thm(%d %d %u)!", ScrnW, ScrnH, ScrnSize, ThmW, ThmH, ThmSize);
    } else {
    #ifdef CONFIG_SOC_A9
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
        YuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        YuvSize += (YuvSize*10)/100;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
    #else
        YuvWidth = StillEncMgt[encID].StillMainWidth;
        YuvHeight = StillEncMgt[encID].StillMainHeight;
        AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        ScrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
        ScrnH = StillEncMgt[encID].ScrnHeight;
        ThmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
        ThmH = StillEncMgt[encID].ThmHeight;
    #endif
        AmbaPrint("[UT_singleCaptureCont]yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);
        ScrnSize = ScrnW*ScrnH*2;
        ScrnSize += (ScrnSize*10)/100;
        ThmSize = ThmW*ThmH*2;
        ThmSize += (ThmSize*10)/100;
        AmbaPrint("[UT_singleCaptureCont]scrn(%d %d %u) thm(%d %d %u)!", ScrnW, ScrnH, ScrnSize, ThmW, ThmH, ThmSize);
    }

#ifdef CONFIG_SOC_A12
    QvLCDW = StillVoutMgt[0][ArIndex].Width;
    QvLCDH = StillVoutMgt[0][ArIndex].Height;
    QvHDMIW = StillVoutMgt[1][ArIndex].Width;
    QvHDMIH = StillVoutMgt[1][ArIndex].Height;
#else
    /* QV need Alignment */
    QvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
    QvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
    QvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
    QvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
#endif
    QvLCDSize = QvLCDW*QvLCDH*2;
    QvLCDSize += (QvLCDSize*15)/100;
    QvHDMISize = QvHDMIW*QvHDMIH*2;
    QvHDMISize += (QvHDMISize*15)/100;
    AmbaPrint("[UT_singleCaptureCont]qvLCD(%u) qvHDMI(%u)!", QvLCDSize, QvHDMISize);

    RoiSize = TileNumber*sizeof(AMP_STILLENC_RAW2RAW_ROI_s);
    Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    /* allocate for QV show buffer before working buffer */
    AmpUT_qvShowBufferAllocate(QvLCDSize, QvHDMISize);

#ifdef _STILL_BUFFER_FROM_DSP_
    {
        UINT8 *DspWorkAddr = NULL;
        UINT32 DspWorkSize = 0;
        UINT8 *BufAddr = NULL;
        int Rt = 0;

        Rt = AmpUT_StillEnc_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
        if (Rt == -1) goto _DONE;

        BufAddr = DspWorkAddr + DspWorkSize;
        RawBuffAddr = BufAddr;
        BufAddr += RawSize;
        AmbaPrint("[UT_singleCaptureCont]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);

        Raw3ARoiBuffAddr = BufAddr;
        BufAddr += RoiSize;
        AmbaPrint("UT_singleCaptureCont]raw3ARoiBuffAddr (0x%08X) (%u)!", Raw3ARoiBuffAddr, RoiSize);

        Raw3AStatBuffAddr = BufAddr;
        BufAddr += Raw3AStatSize;
        AmbaPrint("UT_singleCaptureCont]raw3AStatBuffAddr (0x%08X) (%u)!", Raw3AStatBuffAddr, Raw3AStatSize);

        YuvBuffAddr = BufAddr;
        BufAddr += YuvSize;
        AmbaPrint("[UT_singleCaptureCont]yuvBuffAddr (0x%08X)!", YuvBuffAddr);

        ScrnBuffAddr = BufAddr;
        BufAddr += ScrnSize;
        AmbaPrint("[UT_singleCaptureCont]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);

        ThmBuffAddr = BufAddr;
        BufAddr += ThmSize;
        AmbaPrint("[UT_singleCaptureCont]thmBuffAddr (0x%08X)!", ThmBuffAddr);

        QvLCDBuffAddr = BufAddr;
        BufAddr += QvLCDSize;
        AmbaPrint("[UT_singleCaptureCont]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);

        QvHDMIBuffAddr = BufAddr;
        AmbaPrint("[UT_singleCaptureCont]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#else
    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, RawSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCaptureCont]Cache_DDR alloc raw fail (%u)!", RawSize);
    } else {
        RawBuffAddr = (UINT8*)TempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCaptureCont]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, RoiSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCaptureCont]Cache_DDR alloc raw fail (%u)!", RoiSize);
    } else {
        Raw3ARoiBuffAddr = (UINT8*)TempPtr;
        OriRaw3ARoiBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCaptureCont]Raw3ARoiBuffAddr (0x%08X) (%u)!", Raw3ARoiBuffAddr, RoiSize);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, Raw3AStatSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCaptureCont]Cache_DDR alloc raw 3A stat fail (%u)!", Raw3AStatSize);
    } else {
        Raw3ARoiBuffAddr = (UINT8*)TempPtr;
        OriRaw3ARoiBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCaptureCont]raw3AStatBuffAddr (0x%08X) (%u)!", Raw3AStatBuffAddr, Raw3AStatSize);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, YuvSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCaptureCont]Cache_DDR alloc yuv_main fail (%u)!", YuvSize);
    } else {
        YuvBuffAddr = (UINT8*)TempPtr;
        OriYuvBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCaptureCont]yuvBuffAddr (0x%08X)!", YuvBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ScrnSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCaptureCont]Cache_DDR alloc yuv_scrn fail (%u)!", ScrnSize*1);
    } else {
        ScrnBuffAddr = (UINT8*)TempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCaptureCont]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ThmSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCaptureCont]Cache_DDR alloc yuv_thm fail (%u)!", ThmSize*1);
    } else {
        ThmBuffAddr = (UINT8*)TempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCaptureCont]thmBuffAddr (0x%08X)!", ThmBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, QvLCDSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCaptureCont]Cache_DDR alloc yuv_lcd fail (%u)!", QvLCDSize*1);
    } else {
        QvLCDBuffAddr = (UINT8*)TempPtr;
        OriQvLCDBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCaptureCont]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, QvHDMISize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_singleCaptureCont]Cache_DDR alloc yuv_hdmi fail (%u)!", QvHDMISize*1);
    } else {
        QvHDMIBuffAddr = (UINT8*)TempPtr;
        OriQvHDMIBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_singleCaptureCont]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#endif
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ScriptSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_singleCaptureCont]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)TempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_singleCaptureCont]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
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
    GenScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    GenScrpt->EncRotateFlip = StillEncRotate;
#endif
    GenScrpt->CapProfEnable = (StillIsCapProfEnable == 1)?1: 0;
    GenScrpt->CapProfShow = (StillIsCapProfEnable == 1)?1: 0;
    GenScrpt->QVConfig.DisableLCDQV = (QvDisplayCfg == 0)? 1: 0;
    GenScrpt->QVConfig.DisableHDMIQV = (QvDisplayCfg == 0)? 1: ((TVLiveViewEnable)? 0: 1);
    QvLcdEnable = (QvDisplayCfg == 0)? 0: 1;
    QvTvEnable = (QvDisplayCfg == 0)? 0: ((TVLiveViewEnable)? 1: 0);
    GenScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    GenScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + QvLCDSize/2;
    GenScrpt->QVConfig.LCDWidth = StillVoutMgt[0][ArIndex].Width;
    GenScrpt->QVConfig.LCDHeight = StillVoutMgt[0][ArIndex].Height;
    GenScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    GenScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + QvHDMISize/2;
    GenScrpt->QVConfig.HDMIWidth = StillVoutMgt[1][ArIndex].Width;
    GenScrpt->QVConfig.HDMIHeight = StillVoutMgt[1][ArIndex].Height;
    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (GenScrpt->b2LVCfg)? 1: 0;
    GenScrpt->ScrnEnable = 1;
    GenScrpt->ThmEnable = 1;
    GenScrpt->PostProc = &PostSingleCapContCB;
    GenScrpt->PreProc = &PreSingleCapContCB;

    GenScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->MainBuf.Width = GenScrpt->MainBuf.Pitch = YuvWidth;
    GenScrpt->MainBuf.Height = YuvHeight;
    GenScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    GenScrpt->MainBuf.ChromaAddr = GenScrpt->MainBuf.LumaAddr + YuvSize/2;
    GenScrpt->MainBuf.AOI.X = 0;
    GenScrpt->MainBuf.AOI.Y = 0;
    GenScrpt->MainBuf.AOI.Width = StillEncMgt[encID].StillMainWidth;
    GenScrpt->MainBuf.AOI.Height = StillEncMgt[encID].StillMainHeight;

    GenScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ScrnBuf.Width = GenScrpt->ScrnBuf.Pitch = ScrnW;
    GenScrpt->ScrnBuf.Height = ScrnH;
    GenScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    GenScrpt->ScrnBuf.ChromaAddr = GenScrpt->ScrnBuf.LumaAddr + ScrnSize/2;
    GenScrpt->ScrnBuf.AOI.X = 0;
    GenScrpt->ScrnBuf.AOI.Y = 0;
    GenScrpt->ScrnBuf.AOI.Width = StillEncMgt[encID].ScrnWidthAct;
    GenScrpt->ScrnBuf.AOI.Height = StillEncMgt[encID].ScrnHeightAct;
    GenScrpt->ScrnWidth = StillEncMgt[encID].ScrnWidth;
    GenScrpt->ScrnHeight = StillEncMgt[encID].ScrnHeight;

    GenScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ThmBuf.Width = GenScrpt->ThmBuf.Pitch = ThmW;
    GenScrpt->ThmBuf.Height = ThmH;
    GenScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    GenScrpt->ThmBuf.ChromaAddr = GenScrpt->ThmBuf.LumaAddr + ThmSize/2;
    GenScrpt->ThmBuf.AOI.X = 0;
    GenScrpt->ThmBuf.AOI.Y = 0;
    GenScrpt->ThmBuf.AOI.Width = StillEncMgt[encID].ThmWidthAct;
    GenScrpt->ThmBuf.AOI.Height = StillEncMgt[encID].ThmHeightAct;
    GenScrpt->ThmWidth = StillEncMgt[encID].ThmWidth;
    GenScrpt->ThmHeight = StillEncMgt[encID].ThmHeight;
    AmpUT_StillEncSetJpegBrc(GenScrpt, targetSize, encodeLoop);

    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_singleCaptureCont]Stage_0 0x%08X", StageAddr);

    //raw cap config
    StageAddr = ScriptAddr + TotalScriptSize;
    RawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)StageAddr;
    memset(RawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    RawCapScrpt->Cmd = SENC_RAWCAP;
    RawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    RawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    SensorMode.Data = StillEncMgt[encID].InputStillMode;
    RawCapScrpt->SensorMode = SensorMode;
#ifdef CONFIG_SOC_A9
    RawCapScrpt->SensorMode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
#endif
    RawCapScrpt->FvRawCapArea.VcapWidth = StillEncMgt[encID].StillCaptureWidth;
    RawCapScrpt->FvRawCapArea.VcapHeight = StillEncMgt[encID].StillCaptureHeight;
    RawCapScrpt->FvRawCapArea.EffectArea.X = RawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    RawCapScrpt->FvRawCapArea.EffectArea.Width = RawCapScrpt->FvRawCapArea.VcapWidth;
    RawCapScrpt->FvRawCapArea.EffectArea.Height = RawCapScrpt->FvRawCapArea.VcapHeight;
    RawCapScrpt->FvRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    RawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    RawCapScrpt->FvRawBuf.Width = RawWidth;
    RawCapScrpt->FvRawBuf.Height = RawHeight;
    RawCapScrpt->FvRawBuf.Pitch = RawPitch;
    RawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    RawCapScrpt->FvRingBufSize = RawSize*1;
    RawCapScrpt->CapCB.RawCapCB = AmpUT_StillEncRawCapCB;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    TotalStageNum ++;
    AmbaPrint("[UT_singleCaptureCont]Stage_1 0x%X", StageAddr);

    //raw2raw config
    StageAddr = ScriptAddr + TotalScriptSize;
    Raw2RawScrpt = (AMP_SENC_SCRPT_RAW2RAW_s *)StageAddr;
    memset(Raw2RawScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2RAW_s));
    Raw2RawScrpt->Cmd = SENC_RAW2RAW;
    Raw2RawScrpt->SrcRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    Raw2RawScrpt->SrcRawBuf.Buf = RawBuffAddr;
    Raw2RawScrpt->SrcRawBuf.Width = RawWidth;
    Raw2RawScrpt->SrcRawBuf.Height = RawHeight;
    Raw2RawScrpt->SrcRawBuf.Pitch = RawPitch;
    Raw2RawScrpt->TileNumber = TileNumber;
    if (TileNumber == 1) { //full frame
        AMP_STILLENC_RAW2RAW_ROI_s *roi = (AMP_STILLENC_RAW2RAW_ROI_s *)Raw3ARoiBuffAddr;
        roi->RoiColStart = 0;
        roi->RoiRowStart = 0;
        roi->RoiWidth = RawWidth;
        roi->RoiHeight = RawHeight;
    } else if (TileNumber == 3) {
        AMP_STILLENC_RAW2RAW_ROI_s *roi = (AMP_STILLENC_RAW2RAW_ROI_s *)Raw3ARoiBuffAddr;
        roi[0].RoiColStart = 64;
        roi[0].RoiRowStart = 64;
        roi[0].RoiWidth = 128;
        roi[0].RoiHeight = 128;

        roi[1].RoiColStart = 2368;
        roi[1].RoiRowStart = 64;
        roi[1].RoiWidth = 512;
        roi[1].RoiHeight = 512;

        roi[2].RoiColStart = 128;
        roi[2].RoiRowStart = 128;
        roi[2].RoiWidth = 256;
        roi[2].RoiHeight = 256;
    }
    Raw2RawScrpt->TileListAddr = (UINT32)Raw3ARoiBuffAddr;
    Raw2RawScrpt->Raw3AStatAddr = (UINT32)Raw3AStatBuffAddr;
    Raw2RawScrpt->Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s));
    TotalStageNum ++;
    AmbaPrint("[UT_singleCaptureCont]Stage_2 0x%X", StageAddr);

    //raw2yuv config
    StageAddr = ScriptAddr + TotalScriptSize;
    Raw2YuvScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)StageAddr;
    memset(Raw2YuvScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    Raw2YuvScrpt->Cmd = SENC_RAW2YUV;
    Raw2YuvScrpt->RawType = RawCapScrpt->FvRawType;
    Raw2YuvScrpt->RawBuf.Buf = RawCapScrpt->FvRawBuf.Buf;
    Raw2YuvScrpt->RawBuf.Width = RawCapScrpt->FvRawBuf.Width;
    Raw2YuvScrpt->RawBuf.Height = RawCapScrpt->FvRawBuf.Height;
    Raw2YuvScrpt->RawBuf.Pitch = RawCapScrpt->FvRawBuf.Pitch;
    Raw2YuvScrpt->RawBufRule = RawCapScrpt->FvBufRule;
    Raw2YuvScrpt->RingBufSize = 0;
    Raw2YuvScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    Raw2YuvScrpt->YuvRingBufSize = 0;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    TotalStageNum ++;
    AmbaPrint("[UT_singleCaptureCont]Stage_3 0x%X", StageAddr);

    //yuv2jpg config
    StageAddr = ScriptAddr + TotalScriptSize;
    Yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)StageAddr;
    memset(Yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    Yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    Yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    Yuv2JpgScrpt->YuvRingBufSize = 0;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_singleCaptureCont]Stage_4 0x%X", StageAddr);

    //script config
    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[UT_singleCaptureCont]Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    StillRawCaptureRunning = 1;
    Status = STATUS_STILL_RAWCAPTURE;
    StillBGProcessing = 1;
    StillPivProcess = 0;

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    /* Step4. execute script */
    AmpEnc_RunScript(StillEncPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step4. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");

_DONE:
    return 0;
}

/**
 * UnitTest: burst capture(ie rawcap*capcnt + (raw2yuv+yuv2jpeg)*capcnt)
 *
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_burstCapture(UINT32 encID, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop, UINT8 capcnt)
{
    int er, scriptID;
    void *tempPtr;
    void *TempRawPtr;
    UINT16 rawPitch = 0, rawWidth = 0, rawHeight = 0;
    UINT16 yuvWidth = 0, yuvHeight = 0, scrnW = 0, scrnH = 0, thmW = 0, thmH = 0;
    UINT32 rawSize = 0, yuvSize = 0, scrnSize = 0, thmSize = 0;
    UINT16 qvLCDW = 0, qvLCDH = 0, qvHDMIW = 0, qvHDMIH = 0;
    UINT32 qvLCDSize = 0, qvHDMISize = 0;
    UINT8 *stageAddr = NULL;
    UINT32 totalScriptSize = 0, totalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *genScrpt;
    AMP_SENC_SCRPT_RAWCAP_s *rawCapScrpt;
    AMP_SENC_SCRPT_RAW2YUV_s *raw2YuvScrpt;
    AMP_SENC_SCRPT_YUV2JPG_s *yuv2JpgScrpt;
    AMP_SCRPT_CONFIG_s scrpt;
    UINT8 ArIndex = 0;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};

    if (StillRawCaptureRunning) {
        AmbaPrint("Error status %d", Status);
        goto _DONE;
    }

    G_raw_cmpr = cmpr;
    StillEncModeIdx = encID;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /* Step1. calc single raw and yuv buffer memory */
    rawPitch = (cmpr)? \
        AMP_COMPRESSED_RAW_WIDTH(StillEncMgt[encID].StillCaptureWidth): \
        StillEncMgt[encID].StillCaptureWidth*2;
    rawPitch = ALIGN_32(rawPitch);
    rawWidth =  StillEncMgt[encID].StillCaptureWidth;
    rawHeight = StillEncMgt[encID].StillCaptureHeight;
    rawSize = rawPitch*rawHeight;
    AmbaPrint("[UT_burstCapture]raw(%u %u %u) capcnt(%d)", rawPitch, rawWidth, rawHeight, capcnt);

#ifdef CONFIG_SOC_A12
    yuvWidth = StillEncMgt[encID].StillMainWidth;
    yuvHeight = StillEncMgt[encID].StillMainHeight;
    AmpUT_GetYuvWorkingBuffer(yuvWidth, yuvHeight, rawWidth, rawHeight, &yuvWidth, &yuvHeight);
    yuvSize = yuvWidth*yuvHeight*2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = StillEncMgt[encID].ScrnHeight;
    scrnSize = scrnW*scrnH*2;
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = StillEncMgt[encID].ThmHeight;
    thmSize = thmW*thmH*2;
#else
    //DSP lib need 32Align for Width and 16_Align for height in buffer allocation
    yuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
    yuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
    yuvSize = yuvWidth*yuvHeight*2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
    scrnSize = scrnW*scrnH*2;
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
    thmSize = thmW*thmH*2;
#endif
    AmbaPrint("[UT_burstCapture]yuv(%u %u %u)!", yuvWidth, yuvHeight, yuvSize);


    if (iso != 2) {
#ifdef CONFIG_SOC_A9
        yuvSize += (yuvSize*10)/100;
#endif
        scrnSize += (scrnSize*10)/100;
        thmSize += (thmSize*10)/100;
    }

#ifdef CONFIG_SOC_A12
    qvLCDW = StillVoutMgt[0][ArIndex].Width;
    qvLCDH = StillVoutMgt[0][ArIndex].Height;
    qvHDMIW = StillVoutMgt[1][ArIndex].Width;
    qvHDMIH = StillVoutMgt[1][ArIndex].Height;
#else
    /* QV need Alignment */
    qvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
    qvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
    qvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
    qvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
#endif
    qvLCDSize = qvLCDW*qvLCDH*2;
    qvLCDSize += (qvLCDSize*15)/100;
    qvHDMISize = qvHDMIW*qvHDMIH*2;
    qvHDMISize += (qvHDMISize*15)/100;
    AmbaPrint("[UT_burstCapture]qvLCD(%u) qvHDMI(%u)!", qvLCDSize, qvHDMISize);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    /* allocate for QV show buffer before working buffer */
    AmpUT_qvShowBufferAllocate(qvLCDSize, qvHDMISize);

#ifdef _STILL_BUFFER_FROM_DSP_
    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        UINT8 *bufAddr;
        int rt = 0;

       rt = AmpUT_StillEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        if (rt == -1) goto _DONE;
            bufAddr = dspWorkAddr + dspWorkSize;
        RawBuffAddr = bufAddr;
        bufAddr += rawSize*capcnt;
        AmbaPrint("[UT_burstCapture]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, rawSize);

        YuvBuffAddr = bufAddr;
        bufAddr += yuvSize;
        AmbaPrint("[UT_burstCapture]yuvBuffAddr (0x%08X)!", YuvBuffAddr);

        ScrnBuffAddr = bufAddr;
        bufAddr += scrnSize;
        AmbaPrint("[UT_burstCapture]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);

        ThmBuffAddr = bufAddr;
        bufAddr += thmSize;
        AmbaPrint("[UT_burstCapture]thmBuffAddr (0x%08X)!", ThmBuffAddr);

        QvLCDBuffAddr = bufAddr;
        bufAddr += qvLCDSize;
        AmbaPrint("[UT_burstCapture]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);

        QvHDMIBuffAddr = bufAddr;
        bufAddr += qvHDMISize;
        AmbaPrint("[UT_burstCapture]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#else
    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, &TempRawPtr, rawSize*capcnt, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCapture]Cache_DDR alloc raw buffer fail (%u*%d=%u)!",rawSize,capcnt,rawSize*capcnt);
    } else {
        RawBuffAddr = (UINT8*)tempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCapture]rawBuffAddr (0x%08X) (%u*%d=%u)!", RawBuffAddr,rawSize,capcnt,rawSize*capcnt);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, yuvSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCapture]Cache_DDR alloc yuv_main fail (%u)!", yuvSize);
    } else {
        YuvBuffAddr = (UINT8*)tempPtr;
        OriYuvBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCapture]yuvBuffAddr (0x%08X) (%u)!", YuvBuffAddr,yuvSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, scrnSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCapture]Cache_DDR alloc yuv_scrn fail (%u)!",scrnSize);
    } else {
        ScrnBuffAddr = (UINT8*)tempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCapture]scrnBuffAddr (0x%08X) (%u)!", ScrnBuffAddr,scrnSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, thmSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCapture]Cache_DDR alloc yuv_thm fail (%u)!",thmSize);
    } else {
        ThmBuffAddr = (UINT8*)tempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCapture]thmBuffAddr (0x%08X) (%u)!", ThmBuffAddr,thmSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvLCDSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCapture]Cache_DDR alloc yuv_lcd fail (%u)!", qvLCDSize);
    } else {
        QvLCDBuffAddr = (UINT8*)tempPtr;
        OriQvLCDBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCapture]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvHDMISize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCapture]Cache_DDR alloc yuv_hdmi fail (%u)!", qvHDMISize);
    } else {
        QvHDMIBuffAddr = (UINT8*)tempPtr;
        OriQvHDMIBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCapture]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#endif
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, ScriptSize, 32);
        if (er != OK) {
            AmbaPrint("[UT_burstCapture]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)tempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_burstCapture]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    stageAddr = ScriptAddr;
    genScrpt = (AMP_SENC_SCRPT_GENCFG_s *)stageAddr;
    memset(genScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    genScrpt->Cmd = SENC_GENCFG;
    genScrpt->RawEncRepeat = 1;      // More than one raw, need to do rawenc repeatedly.
    genScrpt->RawEncRepeatStage = 2; // Repeat from R2Y
    genScrpt->RawToCap = capcnt;
    genScrpt->CapProfEnable = (StillIsCapProfEnable == 1)?1: 0;
    genScrpt->CapProfShow = (StillIsCapProfEnable == 1)?1: 0;
    genScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    genScrpt->EncRotateFlip = StillEncRotate;
#endif
    genScrpt->QVConfig.DisableLCDQV = (QvDisplayCfg == 0)? 1: 0;
    genScrpt->QVConfig.DisableHDMIQV = (QvDisplayCfg == 0)? 1: ((TVLiveViewEnable)? 0: 1);
    QvLcdEnable = (QvDisplayCfg == 0)? 0: 1;
    QvTvEnable = (QvDisplayCfg == 0)? 0: ((TVLiveViewEnable)? 1: 0);
    genScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    genScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + qvLCDSize/2;
    genScrpt->QVConfig.LCDWidth = StillVoutMgt[0][ArIndex].Width;
    genScrpt->QVConfig.LCDHeight = StillVoutMgt[0][ArIndex].Height;
    genScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    genScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + qvHDMISize/2;
    genScrpt->QVConfig.HDMIWidth = StillVoutMgt[1][ArIndex].Width;
    genScrpt->QVConfig.HDMIHeight = StillVoutMgt[1][ArIndex].Height;
    genScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (genScrpt->b2LVCfg)? 1: 0;
    genScrpt->ScrnEnable = 1;
    genScrpt->ThmEnable = 1;
    genScrpt->PostProc = &PostBurstCapCB;
    genScrpt->PreProc = &PreBurstCapCB;

    genScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    genScrpt->MainBuf.Width = genScrpt->MainBuf.Pitch = yuvWidth;
    genScrpt->MainBuf.Height = yuvHeight;
    genScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    genScrpt->MainBuf.ChromaAddr = genScrpt->MainBuf.LumaAddr + yuvSize/2;
    genScrpt->MainBuf.AOI.X = 0;
    genScrpt->MainBuf.AOI.Y = 0;
    genScrpt->MainBuf.AOI.Width = StillEncMgt[encID].StillMainWidth;
    genScrpt->MainBuf.AOI.Height = StillEncMgt[encID].StillMainHeight;

    genScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ScrnBuf.Width = genScrpt->ScrnBuf.Pitch = scrnW;
    genScrpt->ScrnBuf.Height = scrnH;
    genScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    genScrpt->ScrnBuf.ChromaAddr = genScrpt->ScrnBuf.LumaAddr + scrnSize/2;
    genScrpt->ScrnBuf.AOI.X = 0;
    genScrpt->ScrnBuf.AOI.Y = 0;
    genScrpt->ScrnBuf.AOI.Width = StillEncMgt[encID].ScrnWidthAct;
    genScrpt->ScrnBuf.AOI.Height = StillEncMgt[encID].ScrnHeightAct;
    genScrpt->ScrnWidth = StillEncMgt[encID].ScrnWidth;
    genScrpt->ScrnHeight = StillEncMgt[encID].ScrnHeight;

    genScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ThmBuf.Width = genScrpt->ThmBuf.Pitch = thmW;
    genScrpt->ThmBuf.Height = thmH;
    genScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    genScrpt->ThmBuf.ChromaAddr = genScrpt->ThmBuf.LumaAddr + thmSize/2;
    genScrpt->ThmBuf.AOI.X = 0;
    genScrpt->ThmBuf.AOI.Y = 0;
    genScrpt->ThmBuf.AOI.Width = StillEncMgt[encID].ThmWidthAct;
    genScrpt->ThmBuf.AOI.Height = StillEncMgt[encID].ThmHeightAct;
    genScrpt->ThmWidth = StillEncMgt[encID].ThmWidth;
    genScrpt->ThmHeight = StillEncMgt[encID].ThmHeight;
    AmpUT_StillEncSetJpegBrc(genScrpt, targetSize, encodeLoop);

    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCapture]Stage_0 0x%08X", stageAddr);

    //raw cap config
    stageAddr = ScriptAddr + totalScriptSize;
    rawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)stageAddr;
    memset(rawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    rawCapScrpt->Cmd = SENC_RAWCAP;
    rawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    rawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    SensorMode.Data = StillEncMgt[encID].InputStillMode;
    rawCapScrpt->SensorMode = SensorMode;
#ifdef CONFIG_SOC_A9
    rawCapScrpt->SensorMode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
#endif
    rawCapScrpt->FvRawCapArea.VcapWidth = StillEncMgt[encID].StillCaptureWidth;
    rawCapScrpt->FvRawCapArea.VcapHeight = StillEncMgt[encID].StillCaptureHeight;
    rawCapScrpt->FvRawCapArea.EffectArea.X = rawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    rawCapScrpt->FvRawCapArea.EffectArea.Width = rawCapScrpt->FvRawCapArea.VcapWidth;
    rawCapScrpt->FvRawCapArea.EffectArea.Height = rawCapScrpt->FvRawCapArea.VcapHeight;
    rawCapScrpt->FvRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    rawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    rawCapScrpt->FvRawBuf.Width = rawWidth;
    rawCapScrpt->FvRawBuf.Height = rawHeight;
    rawCapScrpt->FvRawBuf.Pitch = rawPitch;
    rawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_RING;
    rawCapScrpt->FvRingBufSize = rawSize*capcnt;
    rawCapScrpt->CapCB.RawCapCB = AmpUT_StillEncRawCapCB;
    rawCapScrpt->CapCB.RawCapMultiCB = NULL;
    rawCapScrpt->CapCB.RawCapMultiPrepCB = NULL;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCapture]Stage_1 0x%X", stageAddr);

    //raw2yuv config
    stageAddr = ScriptAddr + totalScriptSize;
    raw2YuvScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)stageAddr;
    memset(raw2YuvScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    raw2YuvScrpt->Cmd = SENC_RAW2YUV;
    raw2YuvScrpt->RawType = rawCapScrpt->FvRawType;
    raw2YuvScrpt->RawBuf.Buf = rawCapScrpt->FvRawBuf.Buf;
    raw2YuvScrpt->RawBuf.Width = rawCapScrpt->FvRawBuf.Width;
    raw2YuvScrpt->RawBuf.Height = rawCapScrpt->FvRawBuf.Height;
    raw2YuvScrpt->RawBuf.Pitch = rawCapScrpt->FvRawBuf.Pitch;
    raw2YuvScrpt->RawBufRule = rawCapScrpt->FvBufRule;
    raw2YuvScrpt->RingBufSize = 0;
    raw2YuvScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    raw2YuvScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCapture]Stage_2 0x%X", stageAddr);

    //yuv2jpg config
    stageAddr = ScriptAddr + totalScriptSize;
    yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)stageAddr;
    memset(yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    yuv2JpgScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCapture]Stage_3 0x%X", stageAddr);

    //script config
    scrpt.mode = AMP_SCRPT_MODE_STILL;
    scrpt.StepPreproc = NULL;
    scrpt.StepPostproc = NULL;
    scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    scrpt.ScriptTotalSize = totalScriptSize;
    scrpt.ScriptStageNum = totalStageNum;
    AmbaPrint("[UT_burstCapture]Scrpt addr 0x%X, Sz %uByte, stg %d", scrpt.ScriptStartAddr, scrpt.ScriptTotalSize, scrpt.ScriptStageNum);

    StillBGProcessing = 1;
    Status = STATUS_STILL_RAWCAPTURE;
    StillRawCaptureRunning = 1;
    StillPivProcess = 0;

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    /* Step4. execute script */
    scriptID = AmpEnc_RunScript(StillEncPipe, &scrpt, AMP_ENC_FUNC_FLAG_NONE);
    /* Step5. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");

    return scriptID;
_DONE:
    return 0;
}

/**
 * UnitTest: continuous burst capture(ie. rawcap + raw2yuv + yuv2jpeg until shutter key release)
 *
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_burstCaptureCont(UINT32 encID, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop)
{
    int er, scriptID;
    void *tempPtr;
    void *TempRawPtr;
    UINT16 rawPitch = 0, rawWidth = 0, rawHeight = 0;
    UINT16 yuvWidth = 0, yuvHeight = 0, scrnW = 0, scrnH = 0, thmW = 0, thmH = 0;
    UINT32 rawSize = 0, yuvSize = 0, scrnSize = 0, thmSize = 0;
    UINT16 qvLCDW = 0, qvLCDH = 0, qvHDMIW = 0, qvHDMIH = 0;
    UINT32 qvLCDSize = 0, qvHDMISize = 0;
    UINT8 *stageAddr = NULL;
    UINT32 totalScriptSize = 0, totalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *genScrpt;
    AMP_SENC_SCRPT_RAWCAP_s *rawCapScrpt;
    AMP_SENC_SCRPT_RAW2YUV_s *raw2YuvScrpt;
    AMP_SENC_SCRPT_YUV2JPG_s *yuv2JpgScrpt;
    AMP_SCRPT_CONFIG_s scrpt;
    UINT8 ArIndex = 0;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};
    const UINT32 capcnt = 6;    // raw buffer number should be allocated by app, 6 here is just for test

    if (StillRawCaptureRunning) {
        AmbaPrint("Error status %d", Status);
        goto _DONE;
    }

    G_raw_cmpr = cmpr;
    StillEncModeIdx = encID;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /* Step 1.1 calculate single raw buffer memory*/
    rawPitch = (cmpr)?AMP_COMPRESSED_RAW_WIDTH(StillEncMgt[encID].StillCaptureWidth): \
                     StillEncMgt[encID].StillCaptureWidth*2;
    rawPitch = ALIGN_32(rawPitch);
    rawWidth = StillEncMgt[encID].StillCaptureWidth;
    rawHeight = StillEncMgt[encID].StillCaptureHeight;
    rawSize = rawPitch*rawHeight;
    AmbaPrint("[UT_burstcaptureCont]raw(%u %u %u)(%u)", rawPitch, rawWidth, rawHeight, rawSize);

    /* Step 1.2 calculate single yuv buffer memory */
#ifdef CONFIG_SOC_A9
    /* DSP lib needs 32_Align for width and 16_Align for height in yuv buffer allocation */
    yuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
    yuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
    yuvSize = yuvWidth*yuvHeight*2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
#else
    yuvWidth = StillEncMgt[encID].StillMainWidth;
    yuvHeight = StillEncMgt[encID].StillMainHeight;
    AmpUT_GetYuvWorkingBuffer(yuvWidth, yuvHeight, rawWidth, rawHeight, &yuvWidth, &yuvHeight);
    yuvSize = yuvWidth * yuvHeight * 2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = StillEncMgt[encID].ScrnHeight;
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = StillEncMgt[encID].ThmHeight;
#endif
    AmbaPrint("[UT_burscaptureCont]yuv(%u %u)(%u)", yuvWidth, yuvHeight, yuvSize);

    /* Step 1.3 calculate single screennail, thumbnail memory */
    scrnSize = scrnW*scrnH*2;
    thmSize = thmW*thmH*2;

    if (iso != 2) {
    #ifdef CONFIG_SOC_A9
        yuvSize += (yuvSize*10)/100;
    #endif
        scrnSize += (scrnSize*10)/100;
        thmSize += (thmSize*10)/100;
    }

#ifdef CONFIG_SOC_A12
    qvLCDW = StillVoutMgt[0][ArIndex].Width;
    qvLCDH = StillVoutMgt[0][ArIndex].Height;
    qvHDMIW = StillVoutMgt[1][ArIndex].Width;
    qvHDMIH = StillVoutMgt[1][ArIndex].Height;
#else
    /* QV need Alignment */
    qvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
    qvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
    qvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
    qvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
#endif
    qvLCDSize = qvLCDW*qvLCDH*2;
    qvLCDSize += (qvLCDSize*15)/100;
    qvHDMISize = qvHDMIW*qvHDMIH*2;
    qvHDMISize += (qvHDMISize*15)/100;
    AmbaPrint("[UT_burstCaptureCont]qvLCD(%u) qvHDMI(%u)!", qvLCDSize, qvHDMISize);

    /* Step 2.1 allocate raw and yuv/scrn/thm buffer address*/
    /* allocate QV show buffer before working buffer */
    AmpUT_qvShowBufferAllocate(qvLCDSize, qvHDMISize);

#ifdef _STILL_BUFFER_FROM_DSP_
    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        UINT8 *bufAddr;
        int rt = 0;

       rt = AmpUT_StillEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        if (rt == -1) goto _DONE;
            bufAddr = dspWorkAddr + dspWorkSize;
        RawBuffAddr = bufAddr;
        bufAddr += rawSize*capcnt;
        AmbaPrint("[UT_burstCaptureCont]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, rawSize);

        YuvBuffAddr = bufAddr;
        bufAddr += yuvSize;
        AmbaPrint("[UT_burstCaptureCont]yuvBuffAddr (0x%08X)!", YuvBuffAddr);

        ScrnBuffAddr = bufAddr;
        bufAddr += scrnSize;
        AmbaPrint("[UT_burstCaptureCont]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);

        ThmBuffAddr = bufAddr;
        bufAddr += thmSize;
        AmbaPrint("[UT_burstCaptureCont]thmBuffAddr (0x%08X)!", ThmBuffAddr);

        QvLCDBuffAddr = bufAddr;
        bufAddr += qvLCDSize;
        AmbaPrint("[UT_burstCaptureCont]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);

        QvHDMIBuffAddr = bufAddr;
        bufAddr += qvHDMISize;
        AmbaPrint("[UT_burstCaptureCont]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#else
    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, &TempRawPtr, rawSize*capcnt, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureCont]Cache_DDR alloc raw buffer fail (%u*%d=%u)!",rawSize,capcnt,rawSize*capcnt);
    } else {
        RawBuffAddr = (UINT8*)tempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureCont]rawBuffAddr (0x%08X) (%u*%d=%u)!", RawBuffAddr,rawSize,capcnt,rawSize*capcnt);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, yuvSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureCont]Cache_DDR alloc yuv_main fail (%u)!", yuvSize);
    } else {
        YuvBuffAddr = (UINT8*)tempPtr;
        OriYuvBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureCont]yuvBuffAddr (0x%08X) (%u)!", YuvBuffAddr,yuvSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, scrnSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureCont]Cache_DDR alloc yuv_scrn fail (%u)!",scrnSize);
    } else {
        ScrnBuffAddr = (UINT8*)tempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureCont]scrnBuffAddr (0x%08X) (%u)!", ScrnBuffAddr,scrnSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, thmSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureCont]Cache_DDR alloc yuv_thm fail (%u)!",thmSize);
    } else {
        ThmBuffAddr = (UINT8*)tempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureCont]thmBuffAddr (0x%08X) (%u)!", ThmBuffAddr,thmSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvLCDSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureCont]Cache_DDR alloc yuv_lcd fail (%u)!", qvLCDSize);
    } else {
        QvLCDBuffAddr = (UINT8*)tempPtr;
        OriQvLCDBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureCont]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvHDMISize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureCont]Cache_DDR alloc yuv_hdmi fail (%u)!", qvHDMISize);
    } else {
        QvHDMIBuffAddr = (UINT8*)tempPtr;
        OriQvHDMIBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureCont]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#endif
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, ScriptSize, 32);
        if (er != OK) {
            AmbaPrint("[UT_burstCaptureCont]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)tempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_burstCaptureCont]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill in script content */
    //general config
    stageAddr = ScriptAddr;
    genScrpt = (AMP_SENC_SCRPT_GENCFG_s *)stageAddr;
    memset(genScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    genScrpt->Cmd = SENC_GENCFG;
    genScrpt->RawEncRepeat = 1;      // More than one raw, need to do rawenc repeatedly.
    genScrpt->RawEncRepeatStage = 2; // Repeat from R2Y

    genScrpt->RawToCap = 1;
    genScrpt->EndlessCapture = 1;   //TBD
    genScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    genScrpt->EncRotateFlip = StillEncRotate;
#endif
    genScrpt->QVConfig.DisableLCDQV = (QvDisplayCfg == 0)? 1: 0;
    genScrpt->QVConfig.DisableHDMIQV = (QvDisplayCfg == 0)? 1: ((TVLiveViewEnable)? 0: 1);
    QvLcdEnable = (QvDisplayCfg == 0)? 0: 1;
    QvTvEnable = (QvDisplayCfg == 0)? 0: ((TVLiveViewEnable)? 1: 0);
    genScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    genScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + qvLCDSize/2;
    genScrpt->QVConfig.LCDWidth = StillVoutMgt[0][ArIndex].Width;
    genScrpt->QVConfig.LCDHeight = StillVoutMgt[0][ArIndex].Height;
    genScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    genScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + qvHDMISize/2;
    genScrpt->QVConfig.HDMIWidth = StillVoutMgt[1][ArIndex].Width;
    genScrpt->QVConfig.HDMIHeight = StillVoutMgt[1][ArIndex].Height;
    genScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (genScrpt->b2LVCfg)? 1: 0;
    genScrpt->ScrnEnable = 1;
    genScrpt->ThmEnable = 1;
    genScrpt->PostProc = &PostBurstCapContCB;
    genScrpt->PreProc = &PreBurstCapContCB;

    genScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    genScrpt->MainBuf.Width = genScrpt->MainBuf.Pitch = yuvWidth;
    genScrpt->MainBuf.Height = yuvHeight;
    genScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    genScrpt->MainBuf.ChromaAddr = genScrpt->MainBuf.LumaAddr + yuvSize/2;
    genScrpt->MainBuf.AOI.X = 0;
    genScrpt->MainBuf.AOI.Y = 0;
    genScrpt->MainBuf.AOI.Width = StillEncMgt[encID].StillMainWidth;
    genScrpt->MainBuf.AOI.Height = StillEncMgt[encID].StillMainHeight;

    genScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ScrnBuf.Width = genScrpt->ScrnBuf.Pitch = scrnW;
    genScrpt->ScrnBuf.Height = scrnH;
    genScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    genScrpt->ScrnBuf.ChromaAddr = genScrpt->ScrnBuf.LumaAddr + scrnSize/2;
    genScrpt->ScrnBuf.AOI.X = 0;
    genScrpt->ScrnBuf.AOI.Y = 0;
    genScrpt->ScrnBuf.AOI.Width = StillEncMgt[encID].ScrnWidthAct;
    genScrpt->ScrnBuf.AOI.Height = StillEncMgt[encID].ScrnHeightAct;
    genScrpt->ScrnWidth = StillEncMgt[encID].ScrnWidth;
    genScrpt->ScrnHeight = StillEncMgt[encID].ScrnHeight;

    genScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ThmBuf.Width = genScrpt->ThmBuf.Pitch = thmW;
    genScrpt->ThmBuf.Height = thmH;
    genScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    genScrpt->ThmBuf.ChromaAddr = genScrpt->ThmBuf.LumaAddr + thmSize/2;
    genScrpt->ThmBuf.AOI.X = 0;
    genScrpt->ThmBuf.AOI.Y = 0;
    genScrpt->ThmBuf.AOI.Width = StillEncMgt[encID].ThmWidthAct;
    genScrpt->ThmBuf.AOI.Height = StillEncMgt[encID].ThmHeightAct;
    genScrpt->ThmWidth = StillEncMgt[encID].ThmWidth;
    genScrpt->ThmHeight = StillEncMgt[encID].ThmHeight;
    AmpUT_StillEncSetJpegBrc(genScrpt, targetSize, encodeLoop);

    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCaptureCont]Stage_0 0x%08X", stageAddr);

    //raw cap config
    stageAddr = ScriptAddr + totalScriptSize;
    rawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)stageAddr;
    memset(rawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    rawCapScrpt->Cmd = SENC_RAWCAP;
    rawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    rawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    SensorMode.Data = StillEncMgt[encID].InputStillMode;
    rawCapScrpt->SensorMode = SensorMode;
#ifdef CONFIG_SOC_A9
    rawCapScrpt->SensorMode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
#endif
    rawCapScrpt->FvRawCapArea.VcapWidth = StillEncMgt[encID].StillCaptureWidth;
    rawCapScrpt->FvRawCapArea.VcapHeight = StillEncMgt[encID].StillCaptureHeight;
    rawCapScrpt->FvRawCapArea.EffectArea.X = rawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    rawCapScrpt->FvRawCapArea.EffectArea.Width = rawCapScrpt->FvRawCapArea.VcapWidth;
    rawCapScrpt->FvRawCapArea.EffectArea.Height = rawCapScrpt->FvRawCapArea.VcapHeight;
    rawCapScrpt->FvRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    rawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    rawCapScrpt->FvRawBuf.Width = rawWidth;
    rawCapScrpt->FvRawBuf.Height = rawHeight;
    rawCapScrpt->FvRawBuf.Pitch = rawPitch;
    rawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_RING;
    rawCapScrpt->FvRingBufSize = rawSize*6;
    rawCapScrpt->CapCB.RawCapCB = AmpUT_StillEncRawCapCB;
    rawCapScrpt->CapCB.RawCapMultiCB = AmpUT_StillEncMultiRawCapCB;
    rawCapScrpt->CapCB.RawCapMultiPrepCB = AmpUT_SensorPrep;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCaptureCont]Stage_1 0x%X", stageAddr);

    //raw2yuv config
    stageAddr = ScriptAddr + totalScriptSize;
    raw2YuvScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)stageAddr;
    memset(raw2YuvScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    raw2YuvScrpt->Cmd = SENC_RAW2YUV;
    raw2YuvScrpt->RawType = rawCapScrpt->FvRawType;
    raw2YuvScrpt->RawBuf.Buf = rawCapScrpt->FvRawBuf.Buf;
    raw2YuvScrpt->RawBuf.Width = rawCapScrpt->FvRawBuf.Width;
    raw2YuvScrpt->RawBuf.Height = rawCapScrpt->FvRawBuf.Height;
    raw2YuvScrpt->RawBuf.Pitch = rawCapScrpt->FvRawBuf.Pitch;
    raw2YuvScrpt->RawBufRule = rawCapScrpt->FvBufRule;
    raw2YuvScrpt->RingBufSize = 0;
    raw2YuvScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    raw2YuvScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCaptureCont]Stage_2 0x%X", stageAddr);

    //yuv2jpg config
    stageAddr = ScriptAddr + totalScriptSize;
    yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)stageAddr;
    memset(yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    yuv2JpgScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCaptureCont]Stage_3 0x%X", stageAddr);

    //script config
    scrpt.mode = AMP_SCRPT_MODE_STILL;
    scrpt.StepPreproc = NULL;
    scrpt.StepPostproc = NULL;
    scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    scrpt.ScriptTotalSize = totalScriptSize;
    scrpt.ScriptStageNum = totalStageNum;
    AmbaPrint("[UT_burstCaptureCont]Scrpt addr 0x%X, Sz %uByte, stg %d", scrpt.ScriptStartAddr, scrpt.ScriptTotalSize, scrpt.ScriptStageNum);

    StillRawCaptureRunning = 1;
    Status = STATUS_STILL_RAWCAPTURE;
    StillBGProcessing = 1;
    StillPivProcess = 0;

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    /* Step4. execute script */
    scriptID = AmpEnc_RunScript(StillEncPipe, &scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step5. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");

    return scriptID;
_DONE:
    return 0;
}

/**
 * UnitTest: continuous burst capture WB(ie. rawcap + raw2raw + raw2yuv + yuv2jpeg until shutter key release)
 *
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_burstCaptureContWB(UINT32 encID, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop)
{
    int er, scriptID;
    void *tempPtr;
    void *TempRawPtr;
    UINT16 rawPitch = 0, rawWidth = 0, rawHeight = 0;
    UINT16 yuvWidth = 0, yuvHeight = 0, scrnW = 0, scrnH = 0, thmW = 0, thmH = 0;
    UINT32 rawSize = 0, yuvSize = 0, scrnSize = 0, thmSize = 0;
    UINT16 qvLCDW = 0, qvLCDH = 0, qvHDMIW = 0, qvHDMIH = 0;
    UINT32 qvLCDSize = 0, qvHDMISize = 0, roiSize = 0, raw3AStatSize = 0;
    UINT8 *stageAddr = NULL;
    UINT32 totalScriptSize = 0, totalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *genScrpt;
    AMP_SENC_SCRPT_RAWCAP_s *rawCapScrpt;
    AMP_SENC_SCRPT_RAW2RAW_s *raw2RawScrpt;
    AMP_SENC_SCRPT_RAW2YUV_s *raw2YuvScrpt;
    AMP_SENC_SCRPT_YUV2JPG_s *yuv2JpgScrpt;
    AMP_SCRPT_CONFIG_s scrpt;
    UINT8 ArIndex = 0;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};

    if (StillRawCaptureRunning) {
        AmbaPrint("Error status %d", Status);
        goto _DONE;
    }

    G_raw_cmpr = cmpr;
    StillEncModeIdx = encID;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /* Step 1.1 calculate single raw buffer memory*/
    rawPitch = (cmpr)?AMP_COMPRESSED_RAW_WIDTH(StillEncMgt[encID].StillCaptureWidth): \
                     StillEncMgt[encID].StillCaptureWidth*2;
    rawPitch = ALIGN_32(rawPitch);
    rawWidth = StillEncMgt[encID].StillCaptureWidth;
    rawHeight = StillEncMgt[encID].StillCaptureHeight;
    rawSize = rawPitch*rawHeight;
    AmbaPrint("[UT_burstcaptureContWB]raw(%u %u %u)(%u)", rawPitch, rawWidth, rawHeight, rawSize);

    /* Step 1.2 calculate single yuv buffer memory */
#ifdef CONFIG_SOC_A9
    /* DSP lib needs 32_Align for width and 16_Align for height in yuv buffer allocation */
    yuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
    yuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
    yuvSize = yuvWidth*yuvHeight*2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
#else
    yuvWidth = StillEncMgt[encID].StillMainWidth;
    yuvHeight = StillEncMgt[encID].StillMainHeight;
    AmpUT_GetYuvWorkingBuffer(yuvWidth, yuvHeight, rawWidth, rawHeight, &yuvWidth, &yuvHeight);
    yuvSize = yuvWidth * yuvHeight * 2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = StillEncMgt[encID].ScrnHeight;
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = StillEncMgt[encID].ThmHeight;
#endif
    AmbaPrint("[UT_burscaptureContWB]yuv(%u %u)(%u)", yuvWidth, yuvHeight, yuvSize);

    /* Step 1.3 calculate single screennail, thumbnail memory */
    scrnSize = scrnW*scrnH*2;
    thmSize = thmW*thmH*2;

    if (iso != 2) {
    #ifdef CONFIG_SOC_A9
        yuvSize += (yuvSize*10)/100;
    #endif
        scrnSize += (scrnSize*10)/100;
        thmSize += (thmSize)/100;
    }

#ifdef CONFIG_SOC_A12
    qvLCDW = StillVoutMgt[0][ArIndex].Width;
    qvLCDH = StillVoutMgt[0][ArIndex].Height;
    qvHDMIW = StillVoutMgt[1][ArIndex].Width;
    qvHDMIH = StillVoutMgt[1][ArIndex].Height;
#else
    /* QV need Alignment */
    qvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
    qvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
    qvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
    qvHDMIH = ALIGN_16(StillVoutMgt[1][ArIndex].Height);
#endif
    qvLCDSize = qvLCDW*qvLCDH*2;
    qvLCDSize += (qvLCDSize*15)/100;
    qvHDMISize = qvHDMIW*qvHDMIH*2;
    qvHDMISize += (qvHDMISize*15)/100;
    AmbaPrint("[UT_burstCaptureContWB]qvLCD(%u) qvHDMI(%u)!", qvLCDSize, qvHDMISize);

    /* Step 1.5 calculate 3a statistics size */
    roiSize = TileNumber*sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
    raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    /* allocate for QV show buffer before working buffer */
    AmpUT_qvShowBufferAllocate(qvLCDSize, qvHDMISize);

#ifdef _STILL_BUFFER_FROM_DSP_
    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        UINT8 *bufAddr;
        int rt = 0;

       rt = AmpUT_StillEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        if (rt == -1) goto _DONE;
            bufAddr = dspWorkAddr + dspWorkSize;
        RawBuffAddr = bufAddr;
        bufAddr += rawSize*6; // raw buffer number should be aloocate by app, 6 here is just for test
        AmbaPrint("[UT_burstCaptureContWB]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, rawSize);

        Raw3ARoiBuffAddr = bufAddr;
        bufAddr += roiSize;
        AmbaPrint("UT_burstCaptureContWB]raw3AStatBuffAddr (0x%08X) (%u)!", Raw3ARoiBuffAddr, roiSize);

        Raw3AStatBuffAddr = bufAddr;
        bufAddr += raw3AStatSize;
        AmbaPrint("UT_burstCaptureContWB]raw3AStatBuffAddr (0x%08X) (%u)!", Raw3AStatBuffAddr, raw3AStatSize);

        YuvBuffAddr = bufAddr;
        bufAddr += yuvSize;
        AmbaPrint("[UT_burstCaptureContWB]yuvBuffAddr (0x%08X)!", YuvBuffAddr);

        ScrnBuffAddr = bufAddr;
        bufAddr += scrnSize;
        AmbaPrint("[UT_burstCaptureContWB]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);

        ThmBuffAddr = bufAddr;
        bufAddr += thmSize;
        AmbaPrint("[UT_burstCaptureContWB]thmBuffAddr (0x%08X)!", ThmBuffAddr);

        QvLCDBuffAddr = bufAddr;
        bufAddr += qvLCDSize;
        AmbaPrint("[UT_burstCaptureContWB]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);

        QvHDMIBuffAddr = bufAddr;
        bufAddr += qvHDMISize;
        AmbaPrint("[UT_burstCaptureContWB]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#else
    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, &TempRawPtr, rawSize*6, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureContWB]Cache_DDR alloc raw buffer fail (%u*%d=%u)!",rawSize,1,rawSize*6);
    } else {
        RawBuffAddr = (UINT8*)tempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureContWB]rawBuffAddr (0x%08X) (%u*%d=%u)!", RawBuffAddr,rawSize,1,rawSize*6);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, yuvSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureContWB]Cache_DDR alloc yuv_main fail (%u*%d=%u)!", yuvSize,1,yuvSize*1);
    } else {
        YuvBuffAddr = (UINT8*)tempPtr;
        OriYuvBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureContWB]yuvBuffAddr (0x%08X) (%u*%d=%u)!", YuvBuffAddr,yuvSize,1,yuvSize*1);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, scrnSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureContWB]Cache_DDR alloc yuv_scrn fail (%u*%d=%u)!",scrnSize,1,scrnSize*1);
    } else {
        ScrnBuffAddr = (UINT8*)tempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureContWB]scrnBuffAddr (0x%08X) (%u*%d=%u)!", ScrnBuffAddr,scrnSize,1,scrnSize*1);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, thmSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureContWB]Cache_DDR alloc yuv_thm fail (%u*%d=%u)!",thmSize,1,thmSize*1);
    } else {
        ThmBuffAddr = (UINT8*)tempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureContWB]thmBuffAddr (0x%08X) (%u*%d=%u)!", ThmBuffAddr,thmSize,1,thmSize*1);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvLCDSize*1, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureContWB]Cache_DDR alloc yuv_lcd fail (%u)!", qvLCDSize*1);
    } else {
        QvLCDBuffAddr = (UINT8*)tempPtr;
        OriQvLCDBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureContWB]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvHDMISize*1, 32);
    if (er != OK) {
        AmbaPrint("[UT_burstCaptureContWB]Cache_DDR alloc yuv_hdmi fail (%u)!", qvHDMISize*1);
    } else {
        QvHDMIBuffAddr = (UINT8*)tempPtr;
        OriQvHDMIBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_burstCaptureContWB]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#endif
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, ScriptSize, 32);
        if (er != OK) {
            AmbaPrint("[UT_burstCaptureContWB]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)tempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_burstCaptureContWB]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill in script content */
    //general config
    stageAddr = ScriptAddr;
    genScrpt = (AMP_SENC_SCRPT_GENCFG_s *)stageAddr;
    memset(genScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    genScrpt->Cmd = SENC_GENCFG;
    genScrpt->RawEncRepeat = 1;      // More than one raw, need to do rawenc repeatedly.
    genScrpt->RawEncRepeatStage = 2; // Repeat from R2R
    genScrpt->RawToCap = 1;
    genScrpt->EndlessCapture = 1;
    genScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    genScrpt->EncRotateFlip = StillEncRotate;
#endif
    genScrpt->QVConfig.DisableLCDQV = (QvDisplayCfg == 0)? 1: 0;
    genScrpt->QVConfig.DisableHDMIQV = (QvDisplayCfg == 0)? 1: ((TVLiveViewEnable)? 0: 1);
    QvLcdEnable = (QvDisplayCfg == 0)? 0: 1;
    QvTvEnable = (QvDisplayCfg == 0)? 0: ((TVLiveViewEnable)? 1: 0);
    genScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    genScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + qvLCDSize/2;
    genScrpt->QVConfig.LCDWidth = StillVoutMgt[0][ArIndex].Width;
    genScrpt->QVConfig.LCDHeight = StillVoutMgt[0][ArIndex].Height;
    genScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    genScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + qvHDMISize/2;
    genScrpt->QVConfig.HDMIWidth = StillVoutMgt[1][ArIndex].Width;
    genScrpt->QVConfig.HDMIHeight = StillVoutMgt[1][ArIndex].Height;
    genScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (genScrpt->b2LVCfg)? 1: 0;
    genScrpt->ScrnEnable = 1;
    genScrpt->ThmEnable = 1;
    genScrpt->PostProc = &PostBurstCapContWBCB;
    genScrpt->PreProc = &PreBurstCapContWBCB;

    genScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    genScrpt->MainBuf.Width = genScrpt->MainBuf.Pitch = yuvWidth;
    genScrpt->MainBuf.Height = yuvHeight;
    genScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    genScrpt->MainBuf.ChromaAddr = genScrpt->MainBuf.LumaAddr + yuvSize/2;
    genScrpt->MainBuf.AOI.X = 0;
    genScrpt->MainBuf.AOI.Y = 0;
    genScrpt->MainBuf.AOI.Width = StillEncMgt[encID].StillMainWidth;
    genScrpt->MainBuf.AOI.Height = StillEncMgt[encID].StillMainHeight;

    genScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ScrnBuf.Width = genScrpt->ScrnBuf.Pitch = scrnW;
    genScrpt->ScrnBuf.Height = scrnH;
    genScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    genScrpt->ScrnBuf.ChromaAddr = genScrpt->ScrnBuf.LumaAddr + scrnSize/2;
    genScrpt->ScrnBuf.AOI.X = 0;
    genScrpt->ScrnBuf.AOI.Y = 0;
    genScrpt->ScrnBuf.AOI.Width = StillEncMgt[encID].ScrnWidthAct;
    genScrpt->ScrnBuf.AOI.Height = StillEncMgt[encID].ScrnHeightAct;
    genScrpt->ScrnWidth = StillEncMgt[encID].ScrnWidth;
    genScrpt->ScrnHeight = StillEncMgt[encID].ScrnHeight;

    genScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ThmBuf.Width = genScrpt->ThmBuf.Pitch = thmW;
    genScrpt->ThmBuf.Height = thmH;
    genScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    genScrpt->ThmBuf.ChromaAddr = genScrpt->ThmBuf.LumaAddr + thmSize/2;
    genScrpt->ThmBuf.AOI.X = 0;
    genScrpt->ThmBuf.AOI.Y = 0;
    genScrpt->ThmBuf.AOI.Width = StillEncMgt[encID].ThmWidthAct;
    genScrpt->ThmBuf.AOI.Height = StillEncMgt[encID].ThmHeightAct;
    genScrpt->ThmWidth = StillEncMgt[encID].ThmWidth;
    genScrpt->ThmHeight = StillEncMgt[encID].ThmHeight;
    AmpUT_StillEncSetJpegBrc(genScrpt, targetSize, encodeLoop);

    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCaptureContWB]Stage_0 0x%08X", stageAddr);

    //raw cap config
    stageAddr = ScriptAddr + totalScriptSize;
    rawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)stageAddr;
    memset(rawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    rawCapScrpt->Cmd = SENC_RAWCAP;
    rawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    rawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    SensorMode.Data = StillEncMgt[encID].InputStillMode;
    rawCapScrpt->SensorMode = SensorMode;
#ifdef CONFIG_SOC_A9
    rawCapScrpt->SensorMode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
#endif
    rawCapScrpt->FvRawCapArea.VcapWidth = StillEncMgt[encID].StillCaptureWidth;
    rawCapScrpt->FvRawCapArea.VcapHeight = StillEncMgt[encID].StillCaptureHeight;
    rawCapScrpt->FvRawCapArea.EffectArea.X = rawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    rawCapScrpt->FvRawCapArea.EffectArea.Width = rawCapScrpt->FvRawCapArea.VcapWidth;
    rawCapScrpt->FvRawCapArea.EffectArea.Height = rawCapScrpt->FvRawCapArea.VcapHeight;
    rawCapScrpt->FvRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    rawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    rawCapScrpt->FvRawBuf.Width = rawWidth;
    rawCapScrpt->FvRawBuf.Height = rawHeight;
    rawCapScrpt->FvRawBuf.Pitch = rawPitch;
    rawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_RING;
    rawCapScrpt->FvRingBufSize = rawSize*6;
    rawCapScrpt->CapCB.RawCapCB = AmpUT_StillEncRawCapCB;
    rawCapScrpt->CapCB.RawCapMultiCB = AmpUT_StillEncMultiRawCapCB;
    rawCapScrpt->CapCB.RawCapMultiPrepCB = AmpUT_SensorPrep;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCaptureContWB]Stage_1 0x%X", stageAddr);

    //raw2raw config
    stageAddr = ScriptAddr + totalScriptSize;
    raw2RawScrpt = (AMP_SENC_SCRPT_RAW2RAW_s *)stageAddr;
    memset(raw2RawScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2RAW_s));
    raw2RawScrpt->Cmd = SENC_RAW2RAW;
    raw2RawScrpt->SrcRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    raw2RawScrpt->SrcRawBuf.Buf = RawBuffAddr; // TBD
    raw2RawScrpt->SrcRawBuf.Width = rawWidth;
    raw2RawScrpt->SrcRawBuf.Height = rawHeight;
    raw2RawScrpt->SrcRawBuf.Pitch = rawPitch;
    raw2RawScrpt->RawBufRule = AMP_ENC_SCRPT_BUFF_RING;
    raw2RawScrpt->TileNumber = TileNumber;
    if (TileNumber == 1) { //full frame
        AMP_STILLENC_RAW2RAW_ROI_s *roi = (AMP_STILLENC_RAW2RAW_ROI_s *)Raw3ARoiBuffAddr;
        roi->RoiColStart = 0;
        roi->RoiRowStart = 0;
        roi->RoiWidth = rawWidth;
        roi->RoiHeight = rawHeight;
    } else if (TileNumber == 3) {
        AMP_STILLENC_RAW2RAW_ROI_s *roi = (AMP_STILLENC_RAW2RAW_ROI_s *)Raw3ARoiBuffAddr;
        roi[0].RoiColStart = 64;
        roi[0].RoiRowStart = 64;
        roi[0].RoiWidth = 128;
        roi[0].RoiHeight = 128;

        roi[1].RoiColStart = 2368;
        roi[1].RoiRowStart = 64;
        roi[1].RoiWidth = 512;
        roi[1].RoiHeight = 512;

        roi[2].RoiColStart = 128;
        roi[2].RoiRowStart = 128;
        roi[2].RoiWidth = 256;
        roi[2].RoiHeight = 256;
    }
    raw2RawScrpt->TileListAddr = (UINT32)Raw3ARoiBuffAddr;
    raw2RawScrpt->Raw3AStatAddr = (UINT32)Raw3AStatBuffAddr;
    raw2RawScrpt->Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCaptureContWB]Stage_2 0x%X", stageAddr);

    //raw2yuv config
    stageAddr = ScriptAddr + totalScriptSize;
    raw2YuvScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)stageAddr;
    memset(raw2YuvScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    raw2YuvScrpt->Cmd = SENC_RAW2YUV;
    raw2YuvScrpt->RawType = rawCapScrpt->FvRawType;
    raw2YuvScrpt->RawBuf.Buf = rawCapScrpt->FvRawBuf.Buf;
    raw2YuvScrpt->RawBuf.Width = rawCapScrpt->FvRawBuf.Width;
    raw2YuvScrpt->RawBuf.Height = rawCapScrpt->FvRawBuf.Height;
    raw2YuvScrpt->RawBuf.Pitch = rawCapScrpt->FvRawBuf.Pitch;
    raw2YuvScrpt->RawBufRule = rawCapScrpt->FvBufRule;
    raw2YuvScrpt->RingBufSize = 0;
    raw2YuvScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    raw2YuvScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCaptureContWB]Stage_3 0x%X", stageAddr);

    //yuv2jpg config
    stageAddr = ScriptAddr + totalScriptSize;
    yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)stageAddr;
    memset(yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    yuv2JpgScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    totalStageNum ++;
    AmbaPrint("[UT_burstCaptureContWB]Stage_4 0x%X", stageAddr);

    //script config
    scrpt.mode = AMP_SCRPT_MODE_STILL;
    scrpt.StepPreproc = NULL;
    scrpt.StepPostproc = NULL;
    scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    scrpt.ScriptTotalSize = totalScriptSize;
    scrpt.ScriptStageNum = totalStageNum;
    AmbaPrint("[UT_burstCaptureContWB]Scrpt addr 0x%X, Sz %uByte, stg %d", scrpt.ScriptStartAddr, scrpt.ScriptTotalSize, scrpt.ScriptStageNum);

    StillRawCaptureRunning = 1;
    Status = STATUS_STILL_RAWCAPTURE;
    StillBGProcessing = 1;
    StillPivProcess = 0;

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    /* Step4. execute script */
    scriptID = AmpEnc_RunScript(StillEncPipe, &scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step5. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");

    return scriptID;
_DONE:
    return 0;
}

/**
 * UnitTest: continuous burst capture(ie. rawcap + raw2yuv + yuv2jpeg until shutter key release)
 *
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_AutoExposureBracketing(UINT32 encID, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop)
{
    int er;
    void *tempPtr;
    void *TempRawPtr;
    UINT16 rawPitch = 0, rawWidth = 0, rawHeight = 0;
    UINT16 yuvWidth = 0, yuvHeight = 0, scrnW = 0, scrnH = 0, thmW = 0, thmH = 0;
    UINT32 rawSize = 0, yuvSize = 0, scrnSize = 0, thmSize = 0;
    UINT16 qvLCDW = 0, qvLCDH = 0, qvHDMIW = 0, qvHDMIH = 0;
    UINT32 qvLCDSize = 0, qvHDMISize = 0;
    UINT8 *stageAddr = NULL;
    UINT32 totalScriptSize = 0, totalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *genScrpt;
    AMP_SENC_SCRPT_RAWCAP_s *rawCapScrpt;
    AMP_SENC_SCRPT_RAW2YUV_s *raw2YuvScrpt;
    AMP_SENC_SCRPT_YUV2JPG_s *yuv2JpgScrpt;
    AMP_SCRPT_CONFIG_s scrpt;
    UINT8 ArIndex = 0;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};

    if (StillRawCaptureRunning) {
        AmbaPrint("Error status %d", Status);
        goto _DONE;
    }

    G_raw_cmpr = cmpr;
    StillEncModeIdx = encID;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /* Step1. calc single raw and yuv buffer memory */
    rawPitch = (cmpr)? \
        AMP_COMPRESSED_RAW_WIDTH(StillEncMgt[encID].StillCaptureWidth): \
        StillEncMgt[encID].StillCaptureWidth*2;
    rawPitch = ALIGN_32(rawPitch);
    rawWidth =  StillEncMgt[encID].StillCaptureWidth;
    rawHeight = StillEncMgt[encID].StillCaptureHeight;
    rawSize = rawPitch*rawHeight;
    AmbaPrint("[UT_autoExposureBracketing]raw(%u %u %u) ", rawPitch, rawWidth, rawHeight);

#ifdef CONFIG_SOC_A9
    //DSP lib need 32Align for Width and 16_Align for height in buffer allocation
    yuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
    yuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
    yuvSize = yuvWidth*yuvHeight*2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
#else
    yuvWidth = StillEncMgt[encID].StillMainWidth;
    yuvHeight = StillEncMgt[encID].StillMainHeight;
    AmpUT_GetYuvWorkingBuffer(yuvWidth, yuvHeight, rawWidth, rawHeight, &yuvWidth, &yuvHeight);
    yuvSize = yuvWidth * yuvHeight * 2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = StillEncMgt[encID].ScrnHeight;
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = StillEncMgt[encID].ThmHeight;
#endif
    AmbaPrint("[UT_autoExposureBracketing]yuv(%u %u %u)!", yuvWidth, yuvHeight, yuvSize);

    scrnSize = scrnW*scrnH*2;
    thmSize = thmW*thmH*2;
    if (iso!=2) {
#ifdef CONFIG_SOC_A9
        yuvSize += (yuvSize*10)/100;
#endif
        scrnSize += (scrnSize*10)/100;
        thmSize += (thmSize*10)/100;
    }

#ifdef CONFIG_SOC_A12
    qvLCDW = StillVoutMgt[0][ArIndex].Width;
    qvLCDH = StillVoutMgt[0][ArIndex].Height;
    qvHDMIW = StillVoutMgt[1][ArIndex].Width;
    qvHDMIH = StillVoutMgt[0][ArIndex].Height;
#else
    /* QV need Alignment */
    qvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
    qvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
    qvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
    qvHDMIH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
#endif
    qvLCDSize = qvLCDW*qvLCDH*2;
    qvLCDSize += (qvLCDSize*15)/100;
    qvHDMISize = qvHDMIW*qvHDMIH*2;
    qvHDMISize += (qvHDMISize*15)/100;
    AmbaPrint("[UT_autoExposureBracketing]qvLCD(%u) qvHDMI(%u)!", qvLCDSize, qvHDMISize);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    /* allocate for QV show buffer before working buffer */
    AmpUT_qvShowBufferAllocate(qvLCDSize, qvHDMISize);

#ifdef _STILL_BUFFER_FROM_DSP_
    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        UINT8 *bufAddr;
        int rt = 0;

       rt = AmpUT_StillEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        if (rt == -1) goto _DONE;
            bufAddr = dspWorkAddr + dspWorkSize;
        RawBuffAddr = bufAddr;
        bufAddr += rawSize*G_capcnt;
        AmbaPrint("[UT_autoExposureBracketing]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, rawSize);

        YuvBuffAddr = bufAddr;
        bufAddr += yuvSize;
        AmbaPrint("[UT_autoExposureBracketing]yuvBuffAddr (0x%08X)!", YuvBuffAddr);

        ScrnBuffAddr = bufAddr;
        bufAddr += scrnSize;
        AmbaPrint("[UT_autoExposureBracketing]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);

        ThmBuffAddr = bufAddr;
        bufAddr += thmSize;
        AmbaPrint("[UT_autoExposureBracketing]thmBuffAddr (0x%08X)!", ThmBuffAddr);

        QvLCDBuffAddr = bufAddr;
        bufAddr += qvLCDSize;
        AmbaPrint("[UT_autoExposureBracketing]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);

        QvHDMIBuffAddr = bufAddr;
        bufAddr += qvHDMISize;
        AmbaPrint("[UT_autoExposureBracketing]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#else
    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, &TempRawPtr, rawSize*G_capcnt, 32);
    if (er != OK) {
        AmbaPrint("[UT_autoExposureBracketing]Cache_DDR alloc raw buffer fail (%u*%d=%u)!",rawSize,G_capcnt,rawSize*G_capcnt);
    } else {
        RawBuffAddr = (UINT8*)tempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_autoExposureBracketing]rawBuffAddr (0x%08X) (%u*%d=%u)!", RawBuffAddr,rawSize,G_capcnt,rawSize*G_capcnt);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, yuvSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_autoExposureBracketing]Cache_DDR alloc yuv_main fail (%u)!", yuvSize);
    } else {
        YuvBuffAddr = (UINT8*)tempPtr;
        OriYuvBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_autoExposureBracketing]yuvBuffAddr (0x%08X) (%u)!", YuvBuffAddr,yuvSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, scrnSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_autoExposureBracketing]Cache_DDR alloc yuv_scrn fail (%u)!",scrnSize);
    } else {
        ScrnBuffAddr = (UINT8*)tempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_autoExposureBracketing]scrnBuffAddr (0x%08X) (%u)!", ScrnBuffAddr,scrnSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, thmSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_autoExposureBracketing]Cache_DDR alloc yuv_thm fail (%u)!",thmSize);
    } else {
        ThmBuffAddr = (UINT8*)tempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_autoExposureBracketing]thmBuffAddr (0x%08X) (%u)!", ThmBuffAddr,thmSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvLCDSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_autoExposureBracketing]Cache_DDR alloc yuv_lcd fail (%u)!", qvLCDSize);
    } else {
        QvLCDBuffAddr = (UINT8*)tempPtr;
        OriQvLCDBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_autoExposureBracketing]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvHDMISize, 32);
    if (er != OK) {
        AmbaPrint("[UT_autoExposureBracketing]Cache_DDR alloc yuv_hdmi fail (%u)!", qvHDMISize);
    } else {
        QvHDMIBuffAddr = (UINT8*)tempPtr;
        OriQvHDMIBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_autoExposureBracketing]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#endif
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, ScriptSize, 32);
        if (er != OK) {
            AmbaPrint("[UT_autoExposureBracketing]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)tempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_autoExposureBracketing]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    stageAddr = ScriptAddr;
    genScrpt = (AMP_SENC_SCRPT_GENCFG_s *)stageAddr;
    memset(genScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    genScrpt->Cmd = SENC_GENCFG;
    genScrpt->RawEncRepeat = 1;      // More than one raw, need to do rawenc repeatedly.
    genScrpt->RawEncRepeatStage = 2; // Repeat from R2Y
    genScrpt->RawToCap = G_capcnt;
    genScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    genScrpt->EncRotateFlip = StillEncRotate;
#endif
    genScrpt->QVConfig.DisableLCDQV = (QvDisplayCfg == 0)? 1: 0;
    genScrpt->QVConfig.DisableHDMIQV = (QvDisplayCfg == 0)? 1: ((TVLiveViewEnable)? 0: 1);
    QvLcdEnable = (QvDisplayCfg == 0)? 0: 1;
    QvTvEnable = (QvDisplayCfg == 0)? 0: ((TVLiveViewEnable)? 1: 0);
    genScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    genScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + qvLCDSize/2;
    genScrpt->QVConfig.LCDWidth = StillVoutMgt[0][ArIndex].Width;
    genScrpt->QVConfig.LCDHeight = StillVoutMgt[0][ArIndex].Height;
    genScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    genScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + qvHDMISize/2;
    genScrpt->QVConfig.HDMIWidth = StillVoutMgt[1][ArIndex].Width;
    genScrpt->QVConfig.HDMIHeight = StillVoutMgt[1][ArIndex].Height;
    genScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (genScrpt->b2LVCfg)? 1: 0;
    genScrpt->ScrnEnable = 1;
    genScrpt->ThmEnable = 1;
    genScrpt->PostProc = &PostAEBCB;
    genScrpt->PreProc = &PreAEBCB;

    genScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    genScrpt->MainBuf.Width = genScrpt->MainBuf.Pitch = yuvWidth;
    genScrpt->MainBuf.Height = yuvHeight;
    genScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    genScrpt->MainBuf.ChromaAddr = genScrpt->MainBuf.LumaAddr + yuvSize/2;
    genScrpt->MainBuf.AOI.X = 0;
    genScrpt->MainBuf.AOI.Y = 0;
    genScrpt->MainBuf.AOI.Width = StillEncMgt[encID].StillMainWidth;
    genScrpt->MainBuf.AOI.Height = StillEncMgt[encID].StillMainHeight;

    genScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ScrnBuf.Width = genScrpt->ScrnBuf.Pitch = scrnW;
    genScrpt->ScrnBuf.Height = scrnH;
    genScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    genScrpt->ScrnBuf.ChromaAddr = genScrpt->ScrnBuf.LumaAddr + scrnSize/2;
    genScrpt->ScrnBuf.AOI.X = 0;
    genScrpt->ScrnBuf.AOI.Y = 0;
    genScrpt->ScrnBuf.AOI.Width = StillEncMgt[encID].ScrnWidthAct;
    genScrpt->ScrnBuf.AOI.Height = StillEncMgt[encID].ScrnHeightAct;
    genScrpt->ScrnWidth = StillEncMgt[encID].ScrnWidth;
    genScrpt->ScrnHeight = StillEncMgt[encID].ScrnHeight;

    genScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ThmBuf.Width = genScrpt->ThmBuf.Pitch = thmW;
    genScrpt->ThmBuf.Height = thmH;
    genScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    genScrpt->ThmBuf.ChromaAddr = genScrpt->ThmBuf.LumaAddr + thmSize/2;
    genScrpt->ThmBuf.AOI.X = 0;
    genScrpt->ThmBuf.AOI.Y = 0;
    genScrpt->ThmBuf.AOI.Width = StillEncMgt[encID].ThmWidthAct;
    genScrpt->ThmBuf.AOI.Height = StillEncMgt[encID].ThmHeightAct;
    genScrpt->ThmWidth = StillEncMgt[encID].ThmWidth;
    genScrpt->ThmHeight = StillEncMgt[encID].ThmHeight;
    AmpUT_StillEncSetJpegBrc(genScrpt, targetSize, encodeLoop);

    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    totalStageNum ++;
    AmbaPrint("[UT_autoExposureBracketing]Stage_0 0x%08X", stageAddr);

    //raw cap config
    stageAddr = ScriptAddr + totalScriptSize;
    rawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)stageAddr;
    memset(rawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    rawCapScrpt->Cmd = SENC_RAWCAP;
    rawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    rawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    SensorMode.Data = StillEncMgt[encID].InputStillMode;
    rawCapScrpt->SensorMode = SensorMode;
#ifdef CONFIG_SOC_A9
    rawCapScrpt->SensorMode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
#endif
    rawCapScrpt->FvRawCapArea.VcapWidth = StillEncMgt[encID].StillCaptureWidth;
    rawCapScrpt->FvRawCapArea.VcapHeight = StillEncMgt[encID].StillCaptureHeight;
    rawCapScrpt->FvRawCapArea.EffectArea.X = rawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    rawCapScrpt->FvRawCapArea.EffectArea.Width = rawCapScrpt->FvRawCapArea.VcapWidth;
    rawCapScrpt->FvRawCapArea.EffectArea.Height = rawCapScrpt->FvRawCapArea.VcapHeight;
    rawCapScrpt->FvRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    rawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    rawCapScrpt->FvRawBuf.Width = rawWidth;
    rawCapScrpt->FvRawBuf.Height = rawHeight;
    rawCapScrpt->FvRawBuf.Pitch = rawPitch;
    rawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_RING;
    rawCapScrpt->FvRingBufSize = rawSize*3;
    rawCapScrpt->CapCB.RawCapCB = AmpUT_StillEncRawCapCB;
    rawCapScrpt->CapCB.RawCapMultiCB = AmpUT_StillEncMultiRawCapCB;
    rawCapScrpt->CapCB.RawCapMultiPrepCB = AmpUT_SensorPrep;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    totalStageNum ++;
    AmbaPrint("[UT_autoExposureBracketing]Stage_1 0x%X", stageAddr);

    //raw2yuv config
    stageAddr = ScriptAddr + totalScriptSize;
    raw2YuvScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)stageAddr;
    memset(raw2YuvScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    raw2YuvScrpt->Cmd = SENC_RAW2YUV;
    raw2YuvScrpt->RawType = rawCapScrpt->FvRawType;
    raw2YuvScrpt->RawBuf.Buf = rawCapScrpt->FvRawBuf.Buf;
    raw2YuvScrpt->RawBuf.Width = rawCapScrpt->FvRawBuf.Width;
    raw2YuvScrpt->RawBuf.Height = rawCapScrpt->FvRawBuf.Height;
    raw2YuvScrpt->RawBuf.Pitch = rawCapScrpt->FvRawBuf.Pitch;
    raw2YuvScrpt->RawBufRule = rawCapScrpt->FvBufRule;
    raw2YuvScrpt->RingBufSize = 0;
    raw2YuvScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    raw2YuvScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    totalStageNum ++;
    AmbaPrint("[UT_autoExposureBracketing]Stage_2 0x%X", stageAddr);

    //yuv2jpg config
    stageAddr = ScriptAddr + totalScriptSize;
    yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)stageAddr;
    memset(yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    yuv2JpgScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    totalStageNum ++;
    AmbaPrint("[UT_autoExposureBracketing]Stage_3 0x%X", stageAddr);

    //script config
    scrpt.mode = AMP_SCRPT_MODE_STILL;
    scrpt.StepPreproc = NULL;
    scrpt.StepPostproc = NULL;
    scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    scrpt.ScriptTotalSize = totalScriptSize;
    scrpt.ScriptStageNum = totalStageNum;
    AmbaPrint("[UT_autoExposureBracketing]Scrpt addr 0x%X, Sz %uByte, stg %d", scrpt.ScriptStartAddr, scrpt.ScriptTotalSize, scrpt.ScriptStageNum);

    StillBGProcessing = 1;
    Status = STATUS_STILL_RAWCAPTURE;
    StillRawCaptureRunning = 1;
    StillPivProcess = 0;

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    /* Step4. execute script */
    AmpEnc_RunScript(StillEncPipe, &scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step5. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");


_DONE:
    return 0;
}

/**
 * UnitTest: PreCapture(ie. rawcap until shutter key release then do raw2yuv + yuv2jpg)
 *
 * @param [in] encID encode specification ID
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_PreCapture(UINT32 encID, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop)
{
    int er, scriptID;
    void *tempPtr;
    void *TempRawPtr;
    UINT16 rawPitch = 0, rawWidth = 0, rawHeight = 0;
    UINT16 yuvWidth = 0, yuvHeight = 0, scrnW = 0, scrnH = 0, thmW = 0, thmH = 0;
    UINT32 rawSize = 0, yuvSize = 0, scrnSize = 0, thmSize = 0;
    UINT16 qvLCDW = 0, qvLCDH = 0, qvHDMIW = 0, qvHDMIH = 0;
    UINT32 qvLCDSize = 0, qvHDMISize = 0;
    UINT8 *stageAddr = NULL;
    UINT32 totalScriptSize = 0, totalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *genScrpt;
    AMP_SENC_SCRPT_RAWCAP_s *rawCapScrpt;
    AMP_SENC_SCRPT_RAW2YUV_s *raw2YuvScrpt;
    AMP_SENC_SCRPT_YUV2JPG_s *yuv2JpgScrpt;
    AMP_SCRPT_CONFIG_s scrpt;
    UINT8 ArIndex = 0;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};
    const UINT32 capcnt = 5;     // TBD: Pre-Capture owns 5 raw buffer

    if (StillRawCaptureRunning) {
        AmbaPrint("Error status %d", Status);
        goto _DONE;
    }

    G_raw_cmpr = cmpr;
    StillEncModeIdx = encID;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /* Step1. calc single raw and yuv buffer memory */
    rawPitch = (cmpr)? \
        AMP_COMPRESSED_RAW_WIDTH(StillEncMgt[encID].StillCaptureWidth): \
        StillEncMgt[encID].StillCaptureWidth*2;
    rawPitch = ALIGN_32(rawPitch);
    rawWidth =  StillEncMgt[encID].StillCaptureWidth;
    rawHeight = StillEncMgt[encID].StillCaptureHeight;
    rawSize = rawPitch*rawHeight;
    AmbaPrint("[UT_PreCapture]raw(%u %u %u) ", rawPitch, rawWidth, rawHeight);

#ifdef CONFIG_SOC_A9
    //DSP lib need 32Align for Width and 16_Align for height in buffer allocation
    yuvWidth = ALIGN_32(StillEncMgt[encID].StillMainWidth);
    yuvHeight = ALIGN_16(StillEncMgt[encID].StillMainHeight);
    yuvSize = yuvWidth*yuvHeight*2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = ALIGN_16(StillEncMgt[encID].ScrnHeight);
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = ALIGN_16(StillEncMgt[encID].ThmHeight);
#else
    yuvWidth = StillEncMgt[encID].StillMainWidth;
    yuvHeight = StillEncMgt[encID].StillMainHeight;
    AmpUT_GetYuvWorkingBuffer(yuvWidth, yuvHeight, rawWidth, rawHeight, &yuvWidth, &yuvHeight);
    yuvSize = yuvWidth * yuvHeight * 2;
    scrnW = ALIGN_32(StillEncMgt[encID].ScrnWidth);
    scrnH = StillEncMgt[encID].ScrnHeight;
    thmW = ALIGN_32(StillEncMgt[encID].ThmWidth);
    thmH = StillEncMgt[encID].ThmHeight;
#endif
    AmbaPrint("[UT_PreCapture]yuv(%u %u %u)!", yuvWidth, yuvHeight, yuvSize);
    scrnSize = scrnW*scrnH*2;
    thmSize = thmW*thmH*2;

    if (iso != 2) {
    #ifdef CONFIG_SOC_A9
        yuvSize += (yuvSize*10)/100;
    #endif
        scrnSize += (scrnSize*10)/100;
        thmSize += (thmSize*10)/100;
    }
#ifdef CONFIG_SOC_A12
    qvLCDW = StillVoutMgt[0][ArIndex].Width;
    qvLCDH = StillVoutMgt[0][ArIndex].Height;
    qvHDMIW = StillVoutMgt[1][ArIndex].Width;
    qvHDMIH = StillVoutMgt[0][ArIndex].Height;
#else
    /* QV need Alignment */
    qvLCDW = ALIGN_32(StillVoutMgt[0][ArIndex].Width);
    qvLCDH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
    qvHDMIW = ALIGN_32(StillVoutMgt[1][ArIndex].Width);
    qvHDMIH = ALIGN_16(StillVoutMgt[0][ArIndex].Height);
#endif
    qvLCDSize = qvLCDW*qvLCDH*2;
    qvLCDSize += (qvLCDSize*15)/100;
    qvHDMISize = qvHDMIW*qvHDMIH*2;
    qvHDMISize += (qvHDMISize*15)/100;
    AmbaPrint("[UT_PreCapture]qvLCD(%u) qvHDMI(%u)!", qvLCDSize, qvHDMISize);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    /* allocate for QV show buffer before working buffer */
    AmpUT_qvShowBufferAllocate(qvLCDSize, qvHDMISize);

#ifdef _STILL_BUFFER_FROM_DSP_
    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        UINT8 *bufAddr;
        int rt = 0;

       rt = AmpUT_StillEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        if (rt == -1) goto _DONE;
            bufAddr = dspWorkAddr + dspWorkSize;
        RawBuffAddr = bufAddr;
        bufAddr += rawSize*capcnt;
        AmbaPrint("[UT_PreCapture]rawBuffAddr (0x%08X) (%u*%d=%u)!", RawBuffAddr,rawSize,capcnt,rawSize*capcnt);

        YuvBuffAddr = bufAddr;
        bufAddr += yuvSize;
        AmbaPrint("[UT_PreCapture]yuvBuffAddr (0x%08X)!", YuvBuffAddr);

        ScrnBuffAddr = bufAddr;
        bufAddr += scrnSize;
        AmbaPrint("[UT_PreCapture]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);

        ThmBuffAddr = bufAddr;
        bufAddr += thmSize;
        AmbaPrint("[UT_PreCapture]thmBuffAddr (0x%08X)!", ThmBuffAddr);

        QvLCDBuffAddr = bufAddr;
        bufAddr += qvLCDSize;
        AmbaPrint("[UT_PreCapture]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);

        QvHDMIBuffAddr = bufAddr;
        bufAddr += qvHDMISize;
        AmbaPrint("[UT_PreCapture]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#else
    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, &TempRawPtr, rawSize*capcnt, 32);
    if (er != OK) {
        AmbaPrint("[UT_PreCapture]Cache_DDR alloc raw buffer fail (%u*%d=%u)!",rawSize,capcnt,rawSize*capcnt);
    } else {
        RawBuffAddr = (UINT8*)tempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_PreCapture]rawBuffAddr (0x%08X) (%u*%d=%u)!", RawBuffAddr,rawSize,capcnt,rawSize*capcnt);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, yuvSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_PreCapture]Cache_DDR alloc yuv_main fail (%u)!", yuvSize);
    } else {
        YuvBuffAddr = (UINT8*)tempPtr;
        OriYuvBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_PreCapture]yuvBuffAddr (0x%08X) (%u)!", YuvBuffAddr,yuvSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, scrnSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_PreCapture]Cache_DDR alloc yuv_scrn fail (%u)!",scrnSize);
    } else {
        ScrnBuffAddr = (UINT8*)tempPtr;
        OriScrnBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_PreCapture]scrnBuffAddr (0x%08X) (%u)!", ScrnBuffAddr,scrnSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, thmSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_PreCapture]Cache_DDR alloc yuv_thm fail (%u)!",thmSize);
    } else {
        ThmBuffAddr = (UINT8*)tempPtr;
        OriThmBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_PreCapture]thmBuffAddr (0x%08X) (%u)!", ThmBuffAddr,thmSize);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvLCDSize, 32);
    if (er != OK) {
        AmbaPrint("[UT_PreCapture]Cache_DDR alloc yuv_lcd fail (%u)!", qvLCDSize);
    } else {
        QvLCDBuffAddr = (UINT8*)tempPtr;
        OriQvLCDBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_PreCapture]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, qvHDMISize, 32);
    if (er != OK) {
        AmbaPrint("[UT_PreCapture]Cache_DDR alloc yuv_hdmi fail (%u)!", qvHDMISize);
    } else {
        QvHDMIBuffAddr = (UINT8*)tempPtr;
        OriQvHDMIBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_PreCapture]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
#endif
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, ScriptSize, 32);
        if (er != OK) {
            AmbaPrint("[UT_PreCapture]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)tempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_PreCapture]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    stageAddr = ScriptAddr;
    genScrpt = (AMP_SENC_SCRPT_GENCFG_s *)stageAddr;
    memset(genScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    genScrpt->Cmd = SENC_GENCFG;
    genScrpt->RawEncRepeat = 1;      // More than one raw, need to do rawenc repeatedly.
    genScrpt->RawEncRepeatStage = 2; // Repeat from R2Y
    genScrpt->RawToCap = 1;
    genScrpt->PreRawNumber = 2;
    genScrpt->PostRawNumber = 3;
    genScrpt->EndlessCapture = 1;
    genScrpt->StillProcMode = iso;
#ifdef CONFIG_SOC_A12
    genScrpt->EncRotateFlip = StillEncRotate;
#endif
    genScrpt->QVConfig.DisableLCDQV = (QvDisplayCfg == 0)? 1: 0;
    genScrpt->QVConfig.DisableHDMIQV = (QvDisplayCfg == 0)? 1: ((TVLiveViewEnable)? 0: 1);
    QvLcdEnable = (QvDisplayCfg == 0)? 0: 1;
    QvTvEnable = (QvDisplayCfg == 0)? 0: ((TVLiveViewEnable)? 1: 0);
    genScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    genScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + qvLCDSize/2;
    genScrpt->QVConfig.LCDWidth = StillVoutMgt[0][ArIndex].Width;
    genScrpt->QVConfig.LCDHeight = StillVoutMgt[0][ArIndex].Height;
    genScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    genScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    genScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + qvHDMISize/2;
    genScrpt->QVConfig.HDMIWidth = StillVoutMgt[1][ArIndex].Width;
    genScrpt->QVConfig.HDMIHeight = StillVoutMgt[1][ArIndex].Height;
    genScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (genScrpt->b2LVCfg)? 1: 0;
    genScrpt->ScrnEnable = 1;
    genScrpt->ThmEnable = 1;
    genScrpt->PostProc = &PostPreCaptureCB;
    genScrpt->PreProc = &PrePreCaptureCB;

    genScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    genScrpt->MainBuf.Width = genScrpt->MainBuf.Pitch = yuvWidth;
    genScrpt->MainBuf.Height = yuvHeight;
    genScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    genScrpt->MainBuf.ChromaAddr = genScrpt->MainBuf.LumaAddr + yuvSize/2;
    genScrpt->MainBuf.AOI.X = 0;
    genScrpt->MainBuf.AOI.Y = 0;
    genScrpt->MainBuf.AOI.Width = StillEncMgt[encID].StillMainWidth;
    genScrpt->MainBuf.AOI.Height = StillEncMgt[encID].StillMainHeight;

    genScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ScrnBuf.Width = genScrpt->ScrnBuf.Pitch = scrnW;
    genScrpt->ScrnBuf.Height = scrnH;
    genScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    genScrpt->ScrnBuf.ChromaAddr = genScrpt->ScrnBuf.LumaAddr + scrnSize/2;
    genScrpt->ScrnBuf.AOI.X = 0;
    genScrpt->ScrnBuf.AOI.Y = 0;
    genScrpt->ScrnBuf.AOI.Width = StillEncMgt[encID].ScrnWidthAct;
    genScrpt->ScrnBuf.AOI.Height = StillEncMgt[encID].ScrnHeightAct;
    genScrpt->ScrnWidth = StillEncMgt[encID].ScrnWidth;
    genScrpt->ScrnHeight = StillEncMgt[encID].ScrnHeight;

    genScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    genScrpt->ThmBuf.Width = genScrpt->ThmBuf.Pitch = thmW;
    genScrpt->ThmBuf.Height = thmH;
    genScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    genScrpt->ThmBuf.ChromaAddr = genScrpt->ThmBuf.LumaAddr + thmSize/2;
    genScrpt->ThmBuf.AOI.X = 0;
    genScrpt->ThmBuf.AOI.Y = 0;
    genScrpt->ThmBuf.AOI.Width = StillEncMgt[encID].ThmWidthAct;
    genScrpt->ThmBuf.AOI.Height = StillEncMgt[encID].ThmHeightAct;
    genScrpt->ThmWidth = StillEncMgt[encID].ThmWidth;
    genScrpt->ThmHeight = StillEncMgt[encID].ThmHeight;
    AmpUT_StillEncSetJpegBrc(genScrpt, targetSize, encodeLoop);

    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    totalStageNum ++;
    AmbaPrint("[UT_PreCapture]Stage_0 0x%08X", stageAddr);

    //raw cap config
    stageAddr = ScriptAddr + totalScriptSize;
    rawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)stageAddr;
    memset(rawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    rawCapScrpt->Cmd = SENC_RAWCAP;
    rawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    rawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    SensorMode.Data = StillEncMgt[encID].InputStillMode;
    rawCapScrpt->SensorMode = SensorMode;
#ifdef CONFIG_SOC_A9
    rawCapScrpt->SensorMode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
#endif
    rawCapScrpt->FvRawCapArea.VcapWidth = StillEncMgt[encID].StillCaptureWidth;
    rawCapScrpt->FvRawCapArea.VcapHeight = StillEncMgt[encID].StillCaptureHeight;
    rawCapScrpt->FvRawCapArea.EffectArea.X = rawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    rawCapScrpt->FvRawCapArea.EffectArea.Width = rawCapScrpt->FvRawCapArea.VcapWidth;
    rawCapScrpt->FvRawCapArea.EffectArea.Height = rawCapScrpt->FvRawCapArea.VcapHeight;
    rawCapScrpt->FvRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    rawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    rawCapScrpt->FvRawBuf.Width = rawWidth;
    rawCapScrpt->FvRawBuf.Height = rawHeight;
    rawCapScrpt->FvRawBuf.Pitch = rawPitch;
    rawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_RING;
    rawCapScrpt->FvRingBufSize = rawSize*5;
    rawCapScrpt->CapCB.RawCapCB = AmpUT_StillEncRawCapCB;
    rawCapScrpt->CapCB.RawCapMultiCB = AmpUT_StillEncMultiRawCapCB;
    rawCapScrpt->CapCB.RawCapMultiPrepCB = AmpUT_SensorPrep;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    totalStageNum ++;
    AmbaPrint("[UT_PreCapture]Stage_1 0x%X", stageAddr);

    //raw2yuv config
    stageAddr = ScriptAddr + totalScriptSize;
    raw2YuvScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)stageAddr;
    memset(raw2YuvScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    raw2YuvScrpt->Cmd = SENC_RAW2YUV;
    raw2YuvScrpt->RawType = rawCapScrpt->FvRawType;
    raw2YuvScrpt->RawBuf.Buf = rawCapScrpt->FvRawBuf.Buf;
    raw2YuvScrpt->RawBuf.Width = rawCapScrpt->FvRawBuf.Width;
    raw2YuvScrpt->RawBuf.Height = rawCapScrpt->FvRawBuf.Height;
    raw2YuvScrpt->RawBuf.Pitch = rawCapScrpt->FvRawBuf.Pitch;
    raw2YuvScrpt->RawBufRule = rawCapScrpt->FvBufRule;
    raw2YuvScrpt->RingBufSize = 0;
    raw2YuvScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    raw2YuvScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    totalStageNum ++;
    AmbaPrint("[UT_PreCapture]Stage_2 0x%X", stageAddr);

    //yuv2jpg config
    stageAddr = ScriptAddr + totalScriptSize;
    yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)stageAddr;
    memset(yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    yuv2JpgScrpt->YuvRingBufSize = 0;
    totalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    totalStageNum ++;
    AmbaPrint("[UT_PreCapture]Stage_3 0x%X", stageAddr);

    //script config
    scrpt.mode = AMP_SCRPT_MODE_STILL;
    scrpt.StepPreproc = NULL;
    scrpt.StepPostproc = NULL;
    scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    scrpt.ScriptTotalSize = totalScriptSize;
    scrpt.ScriptStageNum = totalStageNum;
    AmbaPrint("[UT_PreCapture]Scrpt addr 0x%X, Sz %uByte, stg %d", scrpt.ScriptStartAddr, scrpt.ScriptTotalSize, scrpt.ScriptStageNum);

    StillBGProcessing = 1;
    Status = STATUS_STILL_RAWCAPTURE;
    StillRawCaptureRunning = 1;
    StillPivProcess = 0;

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    /* Step4. execute script */
    scriptID = AmpEnc_RunScript(StillEncPipe, &scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step5. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");

    return scriptID;
_DONE:
    return 0;
}

/* CB for raw2raw idsp setting  */
UINT32 AmpUT_StillEncRaw2RawIdspCfgCB(UINT16 index)
{
    AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
    AMBA_DSP_IMG_BLACK_CORRECTION_s StaticBlackLevel = {0};
    AMBA_DSP_IMG_AAA_STAT_INFO_s AaaStatInfo = {0};
    AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s CfaLeakage = {0};
    AMBA_DSP_IMG_DGAIN_SATURATION_s DgainSaturation = {0};
    AMBA_DSP_IMG_DBP_CORRECTION_s DbpCorr = {0};
    AMBA_DSP_IMG_SBP_CORRECTION_s SbpCorr = {0};
    AMBA_DSP_IMG_ANTI_ALIASING_s AntiAliasing = {0};

    AmbaPrint("[Amp_UT] raw2raw idsp CB %d", index);

    /* Setup mode of ImageKernel */
    ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
    if (G_iso == 0) {
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
    } else if (G_iso == 0) {
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
    } else {
        ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
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

    if (TileNumber == 1) { //full frame
        UINT16 RawWidth = StillEncMgt[StillEncModeIdx].StillCaptureWidth;
        UINT16 RawHeight = StillEncMgt[StillEncModeIdx].StillCaptureHeight;
        if (index == 0) {
            if ((StillOBModeEnable == 1) && (CaptureMode == STILL_CAPTURE_SINGLE_SHOT)) {
                PIPELINE_STILL_CONTROL_s *StillPipeCtrl;
                Amba_Img_Get_Still_Pipe_Ctrl_Addr(0/*TBD*/, (UINT32 *)&StillPipeCtrl);
                AmbaDSP_ImgSetStaticBlackLevel(&ImgMode, &StillPipeCtrl->BlackCorr);
            } else {
                AmbaDSP_ImgGetStaticBlackLevel(&ImgMode, &StaticBlackLevel);
                AmbaDSP_ImgSetStaticBlackLevel(&ImgMode, &StaticBlackLevel);
            }

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
    } else if (TileNumber == 3) { //FIXME:  multi roi
        AMBA_DSP_IMG_AE_STAT_INFO_s AeStatInfo = {0};
        AMBA_DSP_IMG_AF_STAT_INFO_s AfStatInfo = {0};
        AMBA_DSP_IMG_AWB_STAT_INFO_s AwbStatInfo = {0};
        if (index == 0) {
            AeStatInfo.AeTileNumCol = 1;
            AeStatInfo.AeTileNumRow = 1;
            AeStatInfo.AeTileColStart = 0;
            AeStatInfo.AeTileRowStart = 0;
            AeStatInfo.AeTileWidth = 128 / AeStatInfo.AeTileNumCol;
            AeStatInfo.AeTileHeight = 128 / AeStatInfo.AeTileNumRow;
            AeStatInfo.AePixMinValue = 0;
            AeStatInfo.AePixMaxValue = 16100;

            AwbStatInfo.AwbTileNumCol = 1;
            AwbStatInfo.AwbTileNumRow = 1;
            AwbStatInfo.AwbTileColStart = 0;
            AwbStatInfo.AwbTileRowStart = 0;
            AwbStatInfo.AwbTileWidth = 128 / AwbStatInfo.AwbTileNumCol;
            AwbStatInfo.AwbTileHeight = 128 / AwbStatInfo.AwbTileNumRow;
            AwbStatInfo.AwbTileActiveWidth = AwbStatInfo.AwbTileWidth;
            AwbStatInfo.AwbTileActiveHeight = AwbStatInfo.AwbTileHeight;
            AwbStatInfo.AwbPixMinValue = 0;
            AwbStatInfo.AwbPixMaxValue = 16100;

            AfStatInfo.AfTileNumCol = 1;
            AfStatInfo.AfTileNumRow = 1;
            AfStatInfo.AfTileColStart = 0;
            AfStatInfo.AfTileRowStart = 0;
            AfStatInfo.AfTileWidth = 128 / AfStatInfo.AfTileNumCol;
            AfStatInfo.AfTileHeight = 128 / AfStatInfo.AfTileNumRow;
            AfStatInfo.AfTileActiveWidth = AfStatInfo.AfTileWidth;
            AfStatInfo.AfTileActiveHeight = AfStatInfo.AfTileHeight;
        } else if (index == 1) {
            AeStatInfo.AeTileNumCol = 4;
            AeStatInfo.AeTileNumRow = 4;
            AeStatInfo.AeTileColStart = 0;
            AeStatInfo.AeTileRowStart = 0;
            AeStatInfo.AeTileWidth = 512 / AeStatInfo.AeTileNumCol;
            AeStatInfo.AeTileHeight = 512 / AeStatInfo.AeTileNumRow;
            AeStatInfo.AePixMinValue = 0;
            AeStatInfo.AePixMaxValue = 16100;

            AwbStatInfo.AwbTileNumCol = 4;
            AwbStatInfo.AwbTileNumRow = 4;
            AwbStatInfo.AwbTileColStart = 0;
            AwbStatInfo.AwbTileRowStart = 0;
            AwbStatInfo.AwbTileWidth = 512 / AwbStatInfo.AwbTileNumCol;
            AwbStatInfo.AwbTileHeight = 512 / AwbStatInfo.AwbTileNumRow;
            AwbStatInfo.AwbTileActiveWidth = AwbStatInfo.AwbTileWidth;
            AwbStatInfo.AwbTileActiveHeight = AwbStatInfo.AwbTileHeight;
            AwbStatInfo.AwbPixMinValue = 0;
            AwbStatInfo.AwbPixMaxValue = 16100;

            AfStatInfo.AfTileNumCol = 4;
            AfStatInfo.AfTileNumRow = 4;
            AfStatInfo.AfTileColStart = 0;
            AfStatInfo.AfTileRowStart = 0;
            AfStatInfo.AfTileWidth = 512 / AfStatInfo.AfTileNumCol;
            AfStatInfo.AfTileHeight = 512 / AfStatInfo.AfTileNumRow;
            AfStatInfo.AfTileActiveWidth = AfStatInfo.AfTileWidth;
            AfStatInfo.AfTileActiveHeight = AfStatInfo.AfTileHeight;
        } else if (index == 2) {
            AeStatInfo.AeTileNumCol = 2;
            AeStatInfo.AeTileNumRow = 2;
            AeStatInfo.AeTileColStart = 0;
            AeStatInfo.AeTileRowStart = 0;
            AeStatInfo.AeTileWidth = 256 / AeStatInfo.AeTileNumCol;
            AeStatInfo.AeTileHeight = 256 / AeStatInfo.AeTileNumRow;
            AeStatInfo.AePixMinValue = 0;
            AeStatInfo.AePixMaxValue = 16100;

            AwbStatInfo.AwbTileNumCol = 2;
            AwbStatInfo.AwbTileNumRow = 2;
            AwbStatInfo.AwbTileColStart = 0;
            AwbStatInfo.AwbTileRowStart = 0;
            AwbStatInfo.AwbTileWidth = 256 / AwbStatInfo.AwbTileNumCol;
            AwbStatInfo.AwbTileHeight = 256 / AwbStatInfo.AwbTileNumRow;
            AwbStatInfo.AwbTileActiveWidth = AwbStatInfo.AwbTileWidth;
            AwbStatInfo.AwbTileActiveHeight = AwbStatInfo.AwbTileHeight;
            AwbStatInfo.AwbPixMinValue = 0;
            AwbStatInfo.AwbPixMaxValue = 16100;

            AfStatInfo.AfTileNumCol = 2;
            AfStatInfo.AfTileNumRow = 2;
            AfStatInfo.AfTileColStart = 0;
            AfStatInfo.AfTileRowStart = 0;
            AfStatInfo.AfTileWidth = 256 / AfStatInfo.AfTileNumCol;
            AfStatInfo.AfTileHeight = 256 / AfStatInfo.AfTileNumRow;
            AfStatInfo.AfTileActiveWidth = AfStatInfo.AfTileWidth;
            AfStatInfo.AfTileActiveHeight = AfStatInfo.AfTileHeight;
        }

        AmbaDSP_Img3aSetAeStatInfo(&ImgMode, &AeStatInfo);
        AmbaDSP_Img3aSetAfStatInfo(&ImgMode, &AfStatInfo);
        AmbaDSP_Img3aSetAwbStatInfo(&ImgMode, &AwbStatInfo);
    } else if (TileNumber == 0) {
        //TBD
    }
    return OK;
}

/**
 * UnitTest: Config sensor's exposure time and gain
 *
 * @param [in] vin channel
 * @param [in] ae index
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_SensorPrep(AMBA_DSP_CHANNEL_ID_u vinChan, UINT32 exposureIdx)
{
    UINT8 ExposureFrames = 0;
    UINT32 AeIdx = 0;
    UINT32 Rval = 0;
    UINT32 ImgIpChNo = 0;
    UINT32 GainFactor = 0, AGainCtrl = 0, DGainCtrl = 0, ShutterCtrl = 0;
    AMBA_AE_INFO_s StillAeInfo[MAX_AEB_NUM];
    AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
    AMBA_SENSOR_MODE_ID_u StillModeId = {0};

    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, ImgIpChNo, IP_MODE_STILL, (UINT32)StillAeInfo);
    if (CaptureMode == STILL_CAPTURE_AEB) {
        AeIdx = exposureIdx%G_capcnt;
    } else {
        AeIdx = 0;
    }

    AmbaPrintColor(GREEN, "AE info[%d]: EV:%d NF:%d ShIdx:%d AgcIdx:%d IrisIdx:%d Dgain:%d ISO:%d Flash:%d Mode:%d ShTime:%f AgcGain:%f", \
        AeIdx, \
        StillAeInfo[AeIdx].EvIndex, StillAeInfo[AeIdx].NfIndex, \
        StillAeInfo[AeIdx].ShutterIndex, StillAeInfo[AeIdx].AgcIndex, \
        StillAeInfo[AeIdx].IrisIndex, StillAeInfo[AeIdx].Dgain, \
        StillAeInfo[AeIdx].IsoValue, StillAeInfo[AeIdx].Flash, \
        StillAeInfo[AeIdx].Mode, \
        StillAeInfo[AeIdx].ShutterTime, StillAeInfo[AeIdx].AgcGain);

    StillModeId.Data = StillEncMgt[StillEncModeIdx].InputStillMode;
    AmbaSensor_GetModeInfo(vinChan, StillModeId, &SensorModeInfo);
    AmbaSensor_ConvertGainFactor(vinChan, StillAeInfo[AeIdx].AgcGain, &GainFactor, &AGainCtrl, &DGainCtrl);
    AmbaSensor_SetAnalogGainCtrl(vinChan, AGainCtrl);
    AmbaSensor_SetDigitalGainCtrl(vinChan, DGainCtrl);
    AmbaSensor_ConvertShutterSpeed(vinChan, StillAeInfo[AeIdx].ShutterTime, &ShutterCtrl);
    ExposureFrames = (ShutterCtrl/SensorModeInfo.FrameLengthLines);
    ExposureFrames = (ShutterCtrl%SensorModeInfo.FrameLengthLines)? ExposureFrames+1: ExposureFrames;
    AmbaSensor_SetSlowShutterCtrl(vinChan, ExposureFrames);
    AmbaSensor_SetShutterCtrl(vinChan, ShutterCtrl);
    return Rval;
}

/**
 * UnitTest: Raw2Raw PreProc Callback
 * Stage1: Raw2Raw  -> setup r2r idsp cfg
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_Raw2RawPreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 1) {
    #ifdef CONFIG_SOC_A12
        AmpUT_StillEncIdspParamSetup(0/*TBD*/, StillEncMgt[StillEncModeIdx].StillCaptureWidth, StillEncMgt[StillEncModeIdx].StillCaptureHeight);
    #endif
        if (info->IsRaw2Raw3A == 1) {
            AmpUT_StillEncRaw2RawIdspCfgCB(info->CfaIndex);
        }
    }
    return 0;
}

/**
 * UnitTest: raw2raw PostProc Callback
 * Stage1: Raw2Raw  -> Parse and dump CFA stats
 * @param [in] info postproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_Raw2RawPostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    if (info->StageCnt == 1) {
        if(info->IsRaw2Raw3A == 1){
            AmbaPrint("[Amp_UT] raw2RawPPCB: cfa index %d cfa addr 0x%X", info->CfaIndex, info->media.CfaStatInfo);

            if (!(StillDumpSkip & STILL_ENC_SKIP_CFA)) {
                TUNE_Initial_Config_s ItunerInitConfig = {0};
                ItunerInitConfig.Text.pBytePool = &G_MMPL;
                AmbaTUNE_Change_Parser_Mode(TEXT_TUNE);
                if (OK != AmbaTUNE_Init(&ItunerInitConfig)) {
                    AmbaPrintColor(RED, "AmbaTUNE_Init() failed");
                } else {
                    char CfaFileName[32];
                    Ituner_Ext_File_Param_s ExtFileParam = {0};
                    snprintf(CfaFileName, sizeof(CfaFileName), "%s:\\cfa_3a.bin", DefaultSlot);
                    ExtFileParam.Stat_Save_Param.Address = info->media.CfaStatInfo;
                    ExtFileParam.Stat_Save_Param.Size = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
                    ExtFileParam.Stat_Save_Param.Target_File_Path = CfaFileName;
                    AmbaTUNE_Save_Data(EXT_FILE_CFA_STAT, &ExtFileParam);
                    CfaFno++;
                }
            }

            if (info->CfaIndex == TileNumber-1) {
                // whole CFA done, release buffer
                if (RawBuffAddr) {
                    if (AmbaKAL_BytePoolFree((void *)OriRawBuffAddr) != OK)
                        AmbaPrint("[Amp_UT] memFree Fail raw!");
                    RawBuffAddr = NULL;
                }
                if (Raw3ARoiBuffAddr) {
                    if (AmbaKAL_BytePoolFree((void *)OriRaw3ARoiBuffAddr) != OK)
                        AmbaPrint("[Amp_UT] memFree Fail raw3AROI!");
                    Raw3ARoiBuffAddr = NULL;
                }
                if (Raw3AStatBuffAddr) {
                    if (AmbaKAL_BytePoolFree((void *)OriRaw3AStatBuffAddr) != OK)
                        AmbaPrint("[Amp_UT] memFree Fail raw3AStat!");
                    Raw3AStatBuffAddr = NULL;
                }
            }
        }
        if (info->IsRaw2RawResample == 1) {
            char Fn[MAX_FILENAME_LENGTH];
            AMP_CFS_FILE_s *Raw = NULL;
            char MdASCII[3] = {'w','+','\0'};

            //raw ready, dump it
            sprintf(Fn, "%s:\\%04d_resample.RAW", DefaultSlot, RawFno);

            if (!(StillDumpSkip & STILL_ENC_SKIP_RAW)) {
                Raw = UT_StillEncodefopen((const char *)Fn, (const char *)MdASCII);
                AmbaCache_Invalidate((void *)info->media.RawInfo.RawAddr, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight);
                UT_StillEncodefwrite((const void *)info->media.RawInfo.RawAddr, 1, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight, (void *)Raw);
                UT_StillEncodefsync((void *)Raw);
                UT_StillEncodefclose((void *)Raw);
            }
            AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d %d  to %s Start!", \
                info->media.RawInfo.RawAddr, \
                info->media.RawInfo.RawPitch, \
                info->media.RawInfo.RawWidth, \
                info->media.RawInfo.RawHeight, Fn);
        }
    }

    return 0;
}

/**
 * UnitTest: simple raw2raw flow
 *
 * @param [in] inputID ID of raw
 * @param [in] encID encode specification ID
 * @param [in] mode raw2raw execute mode
 * @param [in] cmpr compressed raw or not
 * @param [in] bits raw data bits
 * @param [in] bayer raw bayer pattern
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_raw2raw(UINT32 inputID, UINT8 encID, UINT32 mode, UINT8 cmpr, UINT8 bits, UINT8 bayer)
{
    int Er = OK;
    void *TempPtr = NULL, *TempRawPtr = NULL;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0, DstRawPitch = 0;
    UINT32 RawSize = 0, DstRawSize = 0;
    UINT8 NeedRaw3A = (mode&0x1);
    UINT8 NeedDstRaw = (mode&0x2);
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0;
    AMP_SCRPT_CONFIG_s Scrpt = {0};
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt = NULL;
    AMP_SENC_SCRPT_RAW2RAW_s *Raw2RawScrpt = NULL;

    if (StillRawCaptureRunning) {
        AmbaPrint("Error status %d", Status);
        goto _DONE;
    }

    if (mode == 0x0) {
        AmbaPrint("Error raw2raw execution mode %d", mode);
        goto _DONE;
    }

    StillEncModeIdx = encID;

    /* Step1. calc src_raw and dst_raw buffer memory */
    RawPitch = (cmpr) ? AMP_COMPRESSED_RAW_WIDTH(StillEncMgt[encID].StillCaptureWidth) : StillEncMgt[encID].StillCaptureWidth*2;
    RawWidth = StillEncMgt[encID].StillCaptureWidth;
    RawHeight = StillEncMgt[encID].StillCaptureHeight;
    RawSize = RawPitch*RawHeight;
    AmbaPrint("[UT_pivRaw2Raw]src raw(%u %u %u)", RawPitch, RawWidth, RawHeight);
    if (NeedDstRaw) {
        /* need destination raw , opposite to original source raw */
        DstRawPitch = (cmpr) ? StillEncMgt[encID].StillCaptureWidth*2 : AMP_COMPRESSED_RAW_WIDTH(StillEncMgt[encID].StillCaptureWidth);
        DstRawSize = DstRawPitch*RawHeight;
        AmbaPrint("[UT_pivRaw2Raw]dst raw(%u %u %u)", DstRawPitch, RawWidth, RawHeight);
    }

    /* Step2. allocate raw and dst_raw buffer address, script address */
    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, RawSize, 32);
    if (Er != OK) {
        AmbaPrint("[UT_pivRaw2Raw]DDR alloc raw fail (%u)!", RawSize);
    } else {
        RawBuffAddr = (UINT8*)TempPtr;
        OriRawBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_pivRaw2Raw]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize);
    }

    // Read raw file from SD card based on input ID
    {
        char Fn[32];
        AMP_CFS_FILE_s *Raw = NULL;

        char MdASCII[3] = {'r','+','\0'};

        sprintf(Fn, "%s:\\%04d.raw", DefaultSlot, (int)inputID);

        Raw = UT_StillEncodefopen((const char *)Fn, (const char *)MdASCII);
        AmbaPrint("[UT_pivRaw2Raw]Read raw 0x%X %d %d from %s Start!", RawBuffAddr, RawPitch, RawHeight, Fn);
        UT_StillEncodefread((void *)RawBuffAddr, 1, RawPitch*RawHeight, (void *)Raw);
        UT_StillEncodefclose((void *)Raw);
        AmbaCache_Clean((void *)RawBuffAddr, RawPitch*RawHeight);
        AmbaPrint("[UT_pivRaw2Raw]Read Raw Done!");
    }

    if (NeedDstRaw) {
        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, DstRawSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_pivRaw2Raw]DDR alloc dst_raw fail (%u)!", DstRawSize);
        } else {
            DstRawBuffAddr = (UINT8*)TempPtr;
            OriDstRawBuffAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_pivRaw2Raw]dstRawBuffAddr (0x%08X) (%u)!", DstRawBuffAddr, DstRawSize);
        }
    }

    if (NeedRaw3A) {
        UINT32 roiSize = TileNumber*sizeof(AMP_STILLENC_RAW2RAW_ROI_s);
        UINT32 raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, roiSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_pivRaw2Raw]Cache_DDR alloc roi buffer fail (%u)!", roiSize);
        } else {
            Raw3ARoiBuffAddr = (UINT8*)TempPtr;
            OriRaw3ARoiBuffAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_pivRaw2Raw]raw3ARoiBuffAddr (0x%08X) (%u)!", Raw3ARoiBuffAddr, roiSize);
        }

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, raw3AStatSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_pivRaw2Raw]Cache_DDR alloc 3A stat buffer fail (%u)!", raw3AStatSize);
        } else {
            Raw3AStatBuffAddr = (UINT8*)TempPtr;
            OriRaw3AStatBuffAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_pivRaw2Raw]raw3AStatBuffAddr (0x%08X) (%u)!", Raw3AStatBuffAddr, raw3AStatSize);
        }
    }
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s));

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ScriptSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_pivRaw2Raw]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)TempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_pivRaw2Raw]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    StageAddr = ScriptAddr;
    GenScrpt = (AMP_SENC_SCRPT_GENCFG_s *)StageAddr;
    memset(GenScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    GenScrpt->Cmd = SENC_GENCFG;
    GenScrpt->RawEncRepeat = 0;
    GenScrpt->RawToCap = 0;
#ifdef CONFIG_SOC_A12
    GenScrpt->StillProcMode = G_iso;
    GenScrpt->EncRotateFlip = StillEncRotate;
#endif
    GenScrpt->QVConfig.DisableLCDQV = GenScrpt->QVConfig.DisableHDMIQV = 1;
    QvLcdEnable = 0;
    QvTvEnable = 0;
    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    AutoBackToLiveview = (GenScrpt->b2LVCfg)? 1: 0;
    GenScrpt->ScrnEnable = 0;
    GenScrpt->ThmEnable = 0;
    GenScrpt->PostProc = &PostRaw2RawCB;
    GenScrpt->PreProc = &PreRaw2RawCB;

    GenScrpt->RawDataBits = bits;
    GenScrpt->RawBayerPattern = bayer;

    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_pivRaw2Raw]Stage_0 0x%08X", StageAddr);

    //raw2raw config
    StageAddr = ScriptAddr + TotalScriptSize;
    Raw2RawScrpt = (AMP_SENC_SCRPT_RAW2RAW_s *)StageAddr;
    memset(Raw2RawScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2RAW_s));
    Raw2RawScrpt->Cmd = SENC_RAW2RAW;
    Raw2RawScrpt->SrcRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    Raw2RawScrpt->SrcRawBuf.Buf = RawBuffAddr;
    Raw2RawScrpt->SrcRawBuf.Width = RawWidth;
    Raw2RawScrpt->SrcRawBuf.Height = RawHeight;
    Raw2RawScrpt->SrcRawBuf.Pitch = RawPitch;

    if (NeedDstRaw) {
        Raw2RawScrpt->DstRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
        Raw2RawScrpt->DstRawBuf.Buf = DstRawBuffAddr;
        Raw2RawScrpt->DstRawBuf.Width = RawWidth;
        Raw2RawScrpt->DstRawBuf.Height = RawHeight;
        Raw2RawScrpt->DstRawBuf.Pitch = DstRawPitch;
    }

    if (NeedRaw3A) {
        Raw2RawScrpt->TileNumber = TileNumber;
        if (TileNumber == 1) { //full frame
            AMP_STILLENC_RAW2RAW_ROI_s *roi = (AMP_STILLENC_RAW2RAW_ROI_s *)Raw3ARoiBuffAddr;
            roi->RoiColStart = 0;
            roi->RoiRowStart = 0;
            roi->RoiWidth = RawWidth;
            roi->RoiHeight = RawHeight;
        } else if (TileNumber == 3) {
            AMP_STILLENC_RAW2RAW_ROI_s *roi = (AMP_STILLENC_RAW2RAW_ROI_s *)Raw3ARoiBuffAddr;
            roi[0].RoiColStart = 64;
            roi[0].RoiRowStart = 64;
            roi[0].RoiWidth = 128;
            roi[0].RoiHeight = 128;

            roi[1].RoiColStart = 2368;
            roi[1].RoiRowStart = 64;
            roi[1].RoiWidth = 512;
            roi[1].RoiHeight = 512;

            roi[2].RoiColStart = 128;
            roi[2].RoiRowStart = 128;
            roi[2].RoiWidth = 256;
            roi[2].RoiHeight = 256;
        }
        Raw2RawScrpt->TileListAddr = (UINT32)Raw3ARoiBuffAddr;
        Raw2RawScrpt->Raw3AStatAddr = (UINT32)Raw3AStatBuffAddr;
        Raw2RawScrpt->Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);
    }

    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2RAW_s));
    TotalStageNum ++;
    AmbaPrint("[UT_pivRaw2Raw]Stage_1 0x%08X", StageAddr);

    //script config
    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[UT_pivRaw2Raw]Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
    AmpEnc_StopLiveview(StillEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
    Status = STATUS_STILL_RAWENCODE;
    StillBGProcessing = 1;
    StillPivProcess = 1;

    /* Step5. execute script */
    AmpEnc_RunScript(StillEncPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step6. release script */
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK)
        AmbaPrint("memFree Fail (scrpt)");
    ScriptAddr = NULL;

_DONE:
    return Er;
}


/* CB for raw capture */
UINT32 AmpUT_StillEncRawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl)
{
    UINT8 IsOBEnable = 0;
    AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
    AMBA_SENSOR_AREA_INFO_s *RecPixel = &SensorStatus.ModeInfo.OutputInfo.RecordingPixels;
    AMBA_SENSOR_OUTPUT_INFO_s *OutputInfo = &SensorStatus.ModeInfo.OutputInfo;
    AMBA_DSP_WINDOW_s VinCapture = {0};
    AMP_STILLENC_RAWCAP_DSP_CTRL_s DspCtrl = {0};

    /* Stop LiveView */
    if (AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_CAPTURE, 0) == NG) {
        AmbaPrintColor(RED,"Cannot Stop Liveview");
        return NG;
    } else {
        AmbaPrint("Liveview Stop");
    }

    /* Program Sensor */
    AmbaSensor_Config(ctrl->VinChan, ctrl->SensorMode, ctrl->ShType);
    AmbaSensor_GetStatus(ctrl->VinChan, &SensorStatus);
    AmpUT_SensorPrep(ctrl->VinChan, ctrl->AeIdx);
    AmbaPrint("RawCapCB: (sensor) %d %d", ctrl->SensorMode.Bits.Mode, ctrl->ShType);

    /* Program Vin */
    VinCapture.Width = ctrl->VcapWidth;
    VinCapture.Height = ctrl->VcapHeight;
    if ((VinCapture.Width > RecPixel->Width) || (VinCapture.Height > RecPixel->Height)) {
        VinCapture.OffsetX = ((OutputInfo->OutputWidth - VinCapture.Width)/2) & 0xFFFE;
        VinCapture.OffsetY = ((OutputInfo->OutputHeight - VinCapture.Height)/2) & 0xFFFE;
        IsOBEnable = 1; // Vin capture window size > sensor recording region means OB is enable
    } else {
        VinCapture.OffsetX = (RecPixel->StartX + ((RecPixel->Width - VinCapture.Width)/2)) & 0xFFFE;
        VinCapture.OffsetY = (RecPixel->StartY + ((RecPixel->Height - VinCapture.Height)/2)) & 0xFFFE;
    }
    AmbaPrint("RawCapCB: (vin) %d %d %d %d", VinCapture.Width, VinCapture.Height, VinCapture.OffsetX, VinCapture.OffsetY);
    AmbaDSP_VinCaptureConfig(0, &VinCapture);

    if (1 == IsOBEnable) {
        AMBA_CALIB_SENSOR_MODE_INFO_s CalibSensorInfo = {0};
        AmpCalib_SetOpticalBlackFlag(&IsOBEnable);
        CalibSensorInfo.Mode = ctrl->SensorMode.Data;
        CalibSensorInfo.VerticalFlip = ctrl->SensorMode.Bits.VerticalFlip;
        CalibSensorInfo.VinSensorGeo.StartX = VinCapture.OffsetX;
        CalibSensorInfo.VinSensorGeo.StartY = VinCapture.OffsetY;
        CalibSensorInfo.VinSensorGeo.Width = VinCapture.Width;
        CalibSensorInfo.VinSensorGeo.Height = VinCapture.Height;
        CalibSensorInfo.VinSensorGeo.HSubSample.FactorDen = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorDen;
        CalibSensorInfo.VinSensorGeo.HSubSample.FactorNum = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorNum;
        CalibSensorInfo.VinSensorGeo.VSubSample.FactorDen = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorDen;
        CalibSensorInfo.VinSensorGeo.VSubSample.FactorNum = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorNum;
        CalibSensorInfo.DSPImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
        CalibSensorInfo.DSPImgMode.AlgoMode = (G_iso == 0)? AMBA_DSP_IMG_ALGO_MODE_HISO:((G_iso == 1)? AMBA_DSP_IMG_ALGO_MODE_LISO: AMBA_DSP_IMG_ALGO_MODE_FAST);
        CalibSensorInfo.DSPImgMode.BatchId = (G_iso == 0)? AMBA_DSP_STILL_HISO_FILTER:((G_iso == 1)? AMBA_DSP_STILL_LISO_FILTER: AMBA_DSP_VIDEO_FILTER);
        CalibSensorInfo.DSPImgMode.ContextId = 0;
        CalibSensorInfo.DSPImgMode.ConfigId = 0;
        AmpCalib_SetSensorModeInfo(0/*TBD*/, &CalibSensorInfo);
    }

    /* Set still idsp param */
    AmpUT_StillEncIdspParamSetup(ctrl->AeIdx, ctrl->VcapWidth, ctrl->VcapHeight);

    DspCtrl.VinChan = ctrl->VinChan;
    DspCtrl.VidSkip = 0; // VidSkip of first shot(after mode switch) is control by SSP
    DspCtrl.RawCapNum = ctrl->TotalRawToCap;
    DspCtrl.StillProc = G_iso;
    AmpStillEnc_StartRawCapture(StillEncPri, &DspCtrl);

    return 0;
}

/* CB for multi raw capture */
UINT32 AmpUT_StillEncMultiRawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl)
{
    AMP_STILLENC_RAWCAP_DSP_CTRL_s DspCtrl = {0};

    DspCtrl.RawCapNum = ctrl->TotalRawToCap;
    DspCtrl.VinChan = ctrl->VinChan;
    DspCtrl.StillProc = G_iso;

    if (CaptureMode == STILL_CAPTURE_AEB) {
        UINT8 FrateDelay = 0, ShutterDelay = 0, AGainDelay = 0, DGainDelay = 0;
        AMBA_SENSOR_DEVICE_INFO_s DevInfo = {0};
        AmbaSensor_GetDeviceInfo(EncChannel, &DevInfo);
        FrateDelay = DevInfo.FrameRateCtrlInfo.FirstReflectedFrame -1;
        ShutterDelay = DevInfo.ShutterSpeedCtrlInfo.FirstReflectedFrame - 1;
        AGainDelay = DevInfo.AnalogGainCtrlInfo.FirstReflectedFrame -1;
        DGainDelay = DevInfo.DigitalGainCtrlInfo.FirstReflectedFrame -1;
        DspCtrl.VidSkip = MAX2(MAX2(FrateDelay, ShutterDelay),MAX2(AGainDelay, DGainDelay));
    } else {
        DspCtrl.VidSkip = 0;
    }

    AmpStillEnc_StartFollowingRawCapture(StillEncPri, &DspCtrl);

    return OK;
}

/* Inform 3A to lock AE/AWB before capture */
UINT32 AmpUT_StillEncAAALock(void)
{
    UINT8 CurrMode = IP_PREVIEW_MODE, NextMode = IP_CAPTURE_MODE;
    UINT32 ChNo = 0;
    AMBA_3A_OP_INFO_s AaaOpInfo = {0};
    AMBA_3A_STATUS_s VideoStatus = {0};
    AMBA_3A_STATUS_s StillStatus = {0};

    if (CaptureMode == STILL_CAPTURE_AEB) {
        AEB_INFO_s AebInfo = {0};
        AmbaImg_Proc_Cmd(MW_IP_GET_AEB_INFO, (UINT32)&AebInfo, 0, 0);
        AebInfo.Num = G_capcnt;
        AebInfo.EvBias[0] = 0;   // manual ev bias
        AebInfo.EvBias[1] = -64;
        AebInfo.EvBias[2] = 64;
        AmbaImg_Proc_Cmd(MW_IP_SET_AEB_INFO, (UINT32)&AebInfo, 0, 0);
    }

    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
    AmbaImg_Proc_Cmd(MW_IP_SET_MODE, (UINT32)&CurrMode, (UINT32)&NextMode, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, ChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);

    if (CaptureMode == STILL_CAPTURE_AEB) {
        AmbaImg_Proc_Cmd(MW_IP_SET_CAP_FORMAT, ChNo, IMG_CAP_AEB, 0);
    } else {
        AmbaImg_Proc_Cmd(MW_IP_SET_CAP_FORMAT, ChNo, IMG_CAP_NORMAL, 0);
    }

    // Wait AE lock
    if (AaaOpInfo.AeOp == ENABLE) {
        static UINT16 StillEncAeWaitCnt = 666;
        while (StillStatus.Ae != AMBA_LOCK) {
            AmbaKAL_TaskSleep(3);
            AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, ChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
            StillEncAeWaitCnt--;
            if (StillEncAeWaitCnt == 0) {
                AmbaPrintColor(GREEN, "[UT_Still] Ae lock?");
                StillEncAeWaitCnt = 666;
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
        static UINT16 StillEncAwbWaitCnt = 666;
        while (StillStatus.Awb != AMBA_LOCK) {
            AmbaKAL_TaskSleep(3);
            AmbaImg_Proc_Cmd(MW_IP_GET_3A_STATUS, ChNo, (UINT32)&VideoStatus, (UINT32)&StillStatus);
            StillEncAwbWaitCnt--;
            if (StillEncAwbWaitCnt == 0) {
                AmbaPrintColor(GREEN, "[UT_Still] Awb lock?");
                StillEncAwbWaitCnt = 666;
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

/* Deliver CFA stat to 3A and do still post WB calculation */
UINT32 AmpUT_StillEncPostWBCalculation(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s *cfaStat)
{
    UINT32 ImgChipNo = 0;
    int Speed = 0;
    AmbaImg_Proc_Cmd(MW_IP_SET_CFA_3A_STAT, ImgChipNo, (UINT32) cfaStat, 0);
    AmbaImg_Proc_Cmd(MW_IP_AMBA_POST_STILL, ImgChipNo, (UINT32) Speed, 0);
    return OK;
}

/* Inform 3A to unlock AE/AWB before b2lv */
UINT32 AmpUT_StillEncAAAUnlock(void)
{
    UINT8 CurrMode = IP_PREVIEW_MODE, NextMode = IP_PREVIEW_MODE;
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

UINT32 AmpUT_StillEncSetShotCount(UINT32 shotCount)
{
    UINT32 ChNo = 0;
    /* According to A7L, set cont shot count number from second capture */
    AmbaImg_Proc_Cmd(MW_IP_SET_CONTI_SHOTCOUNT, ChNo, shotCount, 0);
    return OK;
}

UINT32 AmpUT_StillEncAttachOBScript(AMP_SENC_SCRPT_RAW2RAW_s *raw2rawScript, UINT8 encID, UINT8 cmpr)
{
    UINT16 RawWidth = 0, RawHeight = 0, RawPitch = 0;
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};
    AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
    AMP_STILLENC_RAW2RAW_ROI_s *Roi = (AMP_STILLENC_RAW2RAW_ROI_s *)Raw3ARoiBuffAddr;

    SensorMode.Data = StillEncMgt[encID].InputStillMode;
    AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);

    if (StillOBModeEnable == 1) {
        RawWidth = SensorModeInfo.OutputInfo.OutputWidth;
        RawHeight = SensorModeInfo.OutputInfo.OutputHeight;
    } else {
        // Should never reach here
        RawWidth =  StillEncMgt[encID].StillCaptureWidth;
        RawHeight = StillEncMgt[encID].StillCaptureHeight;
    }
    RawPitch = (cmpr)?AMP_COMPRESSED_RAW_WIDTH(RawWidth):RawWidth*2;

    memset(raw2rawScript, 0x0, sizeof(AMP_SENC_SCRPT_RAW2RAW_s));
    raw2rawScript->Cmd = SENC_RAW2RAW;
    raw2rawScript->SrcRawType = (cmpr)?(AMP_STILLENC_RAW_COMPR):(AMP_STILLENC_RAW_UNCOMPR);
    raw2rawScript->SrcRawBuf.Buf = RawBuffAddr;
    raw2rawScript->SrcRawBuf.Pitch = RawPitch;
    raw2rawScript->SrcRawBuf.Width = RawWidth;
    raw2rawScript->SrcRawBuf.Height = RawHeight;
    raw2rawScript->TileNumber = TileNumber;
    if (raw2rawScript->TileNumber == 1) {
        Roi->RoiWidth = StillEncMgt[encID].StillCaptureWidth;
        Roi->RoiHeight = StillEncMgt[encID].StillCaptureHeight;
        if (StillOBModeEnable == 1) {
            Roi->RoiColStart = SensorModeInfo.OutputInfo.RecordingPixels.StartX + \
                ((SensorModeInfo.OutputInfo.RecordingPixels.Width - Roi->RoiWidth)/2&0xFFFE);
            Roi->RoiRowStart = SensorModeInfo.OutputInfo.RecordingPixels.StartY + \
                ((SensorModeInfo.OutputInfo.RecordingPixels.Height - Roi->RoiHeight)/2&0xFFFE);
        } else {
            Roi->RoiColStart = 0;
            Roi->RoiRowStart = 0;
        }
    } else if (raw2rawScript->TileNumber == 3) {
        Roi[0].RoiColStart = 64;
        Roi[0].RoiRowStart = 64;
        Roi[0].RoiWidth = 128;
        Roi[0].RoiHeight = 128;

        Roi[1].RoiColStart = 2368;
        Roi[1].RoiRowStart = 64;
        Roi[1].RoiWidth = 512;
        Roi[1].RoiHeight = 512;

        Roi[2].RoiColStart = 128;
        Roi[2].RoiRowStart = 128;
        Roi[2].RoiWidth = 256;
        Roi[2].RoiHeight = 256;
    }
    raw2rawScript->TileListAddr = (UINT32)Raw3ARoiBuffAddr;
    raw2rawScript->Raw3AStatAddr = (UINT32)Raw3AStatBuffAddr;
    raw2rawScript->Raw3AStatSize = sizeof(AMBA_DSP_EVENT_STILL_CFA_3A_DATA_s);

    return OK;
}

UINT32 AmpUT_StillEncSetJpegBrc(AMP_SENC_SCRPT_GENCFG_s *genScript, UINT32 targetSize, UINT8 encodeLp)
{
    if (targetSize) {
        AmpUT_initJpegDqt(AmpUTJpegQTable[0], -1);
        AmpUT_initJpegDqt(AmpUTJpegQTable[1], -1);
        AmpUT_initJpegDqt(AmpUTJpegQTable[2], -1);
        genScript->BrcCtrl.Tolerance = 10;
        genScript->BrcCtrl.MaxEncLoop = encodeLp;
        genScript->BrcCtrl.JpgBrcCB = AmpUT_JpegBRCPredictCB;
        genScript->BrcCtrl.TargetBitRate = \
           (((targetSize<<13)/((StillCustomMainWidth)? StillCustomMainWidth:StillEncMgt[StillEncModeIdx].StillMainWidth))<<12) \
           /((StillCustomMainHeight)? StillCustomMainHeight:StillEncMgt[StillEncModeIdx].StillMainHeight);
        AmbaPrint("[AmpUT] target size %u kByts loop %u", targetSize,encodeLp);
    } else {
        AmpUT_initJpegDqt(AmpUTJpegQTable[0], 90);
        AmpUT_initJpegDqt(AmpUTJpegQTable[1], 90);
        AmpUT_initJpegDqt(AmpUTJpegQTable[2], 90);
        genScript->BrcCtrl.Tolerance = 0;
        genScript->BrcCtrl.MaxEncLoop = 0;
        genScript->BrcCtrl.JpgBrcCB = NULL;
        genScript->BrcCtrl.TargetBitRate = 0;
    }
    genScript->BrcCtrl.MainQTAddr = AmpUTJpegQTable[0];
    genScript->BrcCtrl.ThmQTAddr = AmpUTJpegQTable[1];
    genScript->BrcCtrl.ScrnQTAddr = AmpUTJpegQTable[2];

    return OK;
}


void AmpUT_StillEnc_UtTask(UINT32 info)
{
    STILLENC_UT_MSG_S msg;

    AmbaPrint("AmpUT_StillEnc_UtTask Start");

    while (1) {
        AmbaKAL_MsgQueueReceive(&UtMsgQueue, (void *)&msg, AMBA_KAL_WAIT_FOREVER);

        //AmbaPrint("[UT_task] Rcv 0x%X", msg.Event);
        switch (msg.Event) {
            case MSG_STILLENC_UT_QV_TIMER_TIMEOUT:
            {
                AmbaPrint("[AMP_UT] QV time up!");

                /* reset Timer */
                AmbaKAL_TimerStop(&QvTimerHdlr);
                AmbaKAL_TimerDelete(&QvTimerHdlr);
                QvTimerHdlrID = -1;

                /* reset qvshow buffer */
                QvLcdShowBuffIndex = QvTvShowBuffIndex = 0;

                /* Switch back to VideoSource */
            #if 0
                if (EncLcdWinHdlr) VoutLcdSrc = AMP_DISP_ENC;
                if (EncTvWinHdlr) VoutTvSrc = AMP_DISP_ENC;
            #endif
                if ((1) && Status != STATUS_STILL_LIVEVIEW) {
            #ifdef CONFIG_SOC_A9
                    //Send LiveStartCmd instead of issue videoSourceSel, SSP forbid user to select voutSource to vin
                    AmpStillEnc_EnableLiveviewCapture(AMP_STILL_SWITCH_TO_STILL_LIVEVIEW, 1);
            #else
                    AmpUT_StillEncAAAUnlock();
                    AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_LIVEVIEW, 0);
                    AmpUT_StillEnc_ChangeResolution(StillEncModeIdx);
                    AmpEnc_StartLiveview(StillEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
                    Status = STATUS_STILL_LIVEVIEW;
                    AmbaPrintColor(BLUE, "[AMP_UT] Back2Liveview: Capture mode %d", CaptureMode);
            #endif
                }
            }
                break;
            case MSG_STILLENC_UT_QV_LCD_READY:
            {
                AMP_YUV_BUFFER_s defImage;
                AMP_DISP_WINDOW_CFG_s window;
                UINT8 *lumaAddr = msg.Data.yuvInfo.yAddr;
                UINT8 *chromaAddr = lumaAddr + msg.Data.yuvInfo.ySize;
                UINT8 ArIndex = 0;

                if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                if ((QvDisplayCfg != 0) && 1/*Last frame*/) {
                    //switch VOUT base on qvConfig
                    /* 1. Set VOUT soruce as Default image */
                    memset(&defImage, 0x0, sizeof(AMP_YUV_BUFFER_s));
                    defImage.ColorFmt = AMP_YUV_422;
                    defImage.Width = msg.Data.yuvInfo.width;
                    defImage.Height = msg.Data.yuvInfo.height;
                    defImage.Pitch = msg.Data.yuvInfo.pitch;
                    defImage.LumaAddr = lumaAddr;
                    defImage.ChromaAddr = chromaAddr;

                    window.Source = AMP_DISP_DEFIMG;
                    window.SourceDesc.DefImage.Image = &defImage;
                    window.SourceDesc.DefImage.FieldRepeat = 0;
                    window.TargetAreaOnPlane.Width = StillVoutMgt[0][ArIndex].Width;
                    window.TargetAreaOnPlane.Height = StillVoutMgt[0][ArIndex].Height;
                    window.TargetAreaOnPlane.X = 0;
                    window.TargetAreaOnPlane.Y = (480 - window.TargetAreaOnPlane.Height)/2;
                    AmpUT_Display_SetWindowCfg(0, &window);

                    /* 2. switch VOUT to default image */
                    AmpUT_Display_Act_Window(0);
    #if 0
                    VoutLcdSrc = AMP_DISP_DEFIMG;
    #endif
                    QvLcdShowBuffIndex++;
                    QvLcdShowBuffIndex %= 2;

                    /* 3. start Qv timer handler base on qvShowConfig */
                    if (QvTimerHdlrID == -1) {
                        QvTimerHdlrID = AmbaKAL_TimerCreate(&QvTimerHdlr, AMBA_KAL_AUTO_START, &AmpUT_qvTimer_Handler, \
                            QvTimerID, QvShowTimeTable[QvDisplayCfg-1], 0xFFFFFFFF);
                        if (QvTimerHdlrID < 0) {
                            AmbaPrintColor(RED,"[Amp_UT] create QV tmr handler failed !!");
                        } else {
                            AmbaPrint("[Amp_UT] QV tmr handler ID(%d)", QvTimerHdlrID);
                        }
                    } else {
                        if ((QvTvEnable && QvTvShowBuffIndex == QvLcdShowBuffIndex) || !QvTvEnable) {
                            /* Qv timer Handler already exists, reset it */
                            AmbaKAL_TimerStop(&QvTimerHdlr);
                            AmbaKAL_TimerChange(&QvTimerHdlr, QvShowTimeTable[QvDisplayCfg-1], 0xFFFFFFFF, AMBA_KAL_AUTO_START);
                        }
                    }
                }

            }
                break;
            case MSG_STILLENC_UT_QV_TV_READY:
            {
                AMP_YUV_BUFFER_s defImage;
                AMP_DISP_WINDOW_CFG_s window;
                UINT8 *lumaAddr = msg.Data.yuvInfo.yAddr;
                UINT8 *chromaAddr = lumaAddr + msg.Data.yuvInfo.ySize;
                UINT8 ArIndex = 0;

                if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                //switch VOUT base on qvConfig
                /* 1. Set VOUT soruce as Default image */
                memset(&defImage, 0x0, sizeof(AMP_YUV_BUFFER_s));
                defImage.ColorFmt = AMP_YUV_422;
                defImage.Width = msg.Data.yuvInfo.width;
                defImage.Height = msg.Data.yuvInfo.height;
                defImage.Pitch = msg.Data.yuvInfo.pitch;
                defImage.LumaAddr = lumaAddr;
                defImage.ChromaAddr = chromaAddr;

                window.Source = AMP_DISP_DEFIMG;
                window.SourceDesc.DefImage.Image = &defImage;
                window.SourceDesc.DefImage.FieldRepeat = 0;
                window.TargetAreaOnPlane.Width = StillVoutMgt[1][ArIndex].Width;
                window.TargetAreaOnPlane.Height = StillVoutMgt[1][ArIndex].Height;
                window.TargetAreaOnPlane.X = (1920 - window.TargetAreaOnPlane.Width)/2;
                window.TargetAreaOnPlane.Y = 0;
                AmpUT_Display_SetWindowCfg(1, &window);

                /* 2. switch VOUT to default image */
                AmpUT_Display_Act_Window(1);
#if 0
                VoutTvSrc = AMP_DISP_DEFIMG;
#endif
                QvTvShowBuffIndex++;
                QvTvShowBuffIndex %= 2;

                /* 3. Qv start timer handler base on qvShowConfig */
                if (QvTimerHdlrID == -1) {
                    QvTimerHdlrID = AmbaKAL_TimerCreate(&QvTimerHdlr, AMBA_KAL_AUTO_START, &AmpUT_qvTimer_Handler,
                            QvTimerID, QvShowTimeTable[QvDisplayCfg-1], QvShowTimeTable[QvDisplayCfg-1]);
                    if (QvTimerHdlrID < 0) {
                        AmbaPrintColor(RED,"[Amp_UT] create QV tmr handler failed !!");
                    } else AmbaPrint("[Amp_UT] QV tmr handler %d", QvTimerHdlrID);
                } else {
                    if ((QvLcdEnable && QvLcdShowBuffIndex == QvTvShowBuffIndex) || \
                        !QvLcdEnable) {
                        /* Qv timer Handler already exists, reset it */
                        AmbaKAL_TimerStop(&QvTimerHdlr);
                        AmbaKAL_TimerChange(&QvTimerHdlr, QvShowTimeTable[QvDisplayCfg-1], QvShowTimeTable[QvDisplayCfg-1], AMBA_KAL_AUTO_START);
                    }
                }
            }
                break;
            case MSG_STILLENC_UT_RAW_CAPTURE_START:
            {
                AMP_ENC_RAW_INFO_s *RawInfo = (AMP_ENC_RAW_INFO_s *) msg.Data.DataInfo[0];
                AmbaPrint("[AmpUT] Raw capture start: RawAddr 0x%X Cmpr %u (p,w,h) = (%u %u %u) ob (%u %u %u %u)",\
                    RawInfo->RawAddr, (RawInfo->compressed == AMP_STILLENC_RAW_COMPR), RawInfo->RawPitch, \
                    RawInfo->RawWidth, RawInfo->RawHeight, \
                    RawInfo->OBWidth, RawInfo->OBHeight, RawInfo->OBOffsetX, RawInfo->OBOffsetY);

            #ifdef CONFIG_SOC_A12
                if (StillOBModeEnable == 1) {
                    UINT32 RawAddr = 0, RawPitch = 0, CurrRow = 0, LastOBRow = 0, ChNo = 0;
                    LastOBRow = RawInfo->OBOffsetY + RawInfo->OBHeight;

                    do {
                        AmbaDSP_StillRawCaptureMonitorGetCurrentInfo(0/*TBD*/, &RawAddr, &RawPitch, &CurrRow);
                        AmbaKAL_TaskSleep(1);
                    } while (CurrRow <= LastOBRow);
                    AmbaPrint("[AmpUT] OB region %d(%d) capture done", CurrRow, LastOBRow);

                    if (CaptureMode == STILL_CAPTURE_SINGLE_SHOT) {
                        AMBA_IMG_RAW_INFO_s CaptureRawInfo = {0};
                        AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
                        AmbaSensor_GetStatus(EncChannel, &SensorStatus);
                        CaptureRawInfo.Compressed = (RawInfo->compressed == AMP_STILLENC_RAW_COMPR)? 1:0;
                        CaptureRawInfo.pBaseAddr = RawInfo->RawAddr;
                        CaptureRawInfo.Pitch = RawInfo->RawPitch;
                        CaptureRawInfo.BayerPattern = SensorStatus.ModeInfo.OutputInfo.CfaPattern;
                        CaptureRawInfo.NumDataBits = SensorStatus.ModeInfo.OutputInfo.NumDataBits;
                        CaptureRawInfo.Window.Width = RawInfo->RawWidth;
                        CaptureRawInfo.Window.Height = RawInfo->RawHeight;
                        CaptureRawInfo.Window.OffsetX = 0;
                        CaptureRawInfo.Window.OffsetY = 0;
                        CaptureRawInfo.OBWindow.Width = RawInfo->OBWidth;
                        CaptureRawInfo.OBWindow.Height = RawInfo->OBHeight;
                        CaptureRawInfo.OBWindow.OffsetX = RawInfo->OBOffsetX;
                        CaptureRawInfo.OBWindow.OffsetY = RawInfo->OBOffsetY;
                        AmbaImg_Proc_Cmd(MW_IP_COMPUTE_STILL_OB, ChNo, (UINT32)&CaptureRawInfo, 0);
                    }
                }
            #endif
            }
                break;
            case MSG_STILLENC_UT_RAW_CAPTURE_DONE:
            {
                //Exif
                {
                    COMPUTE_EXIF_PARAMS_s ExifParam = {0};
                    EXIF_INFO_s ExifInfo = {0};

                    ExifParam.AeIdx = 0; //TBD
                    ExifParam.Mode = IMG_EXIF_STILL;
                    Amba_Img_Exif_Compute_AAA_Exif(&ExifParam);

                    Amba_Img_Exif_Get_Exif_Info(ExifParam.AeIdx, &ExifInfo);
                    AmbaPrint("[AmpUT][Still Exif]");
                    AmbaPrint("======== AE ========");
                    AmbaPrint("ExpTime    : %u/%u sec", ExifInfo.ExposureTimeNum, ExifInfo.ExposureTimeDen);
                    AmbaPrint("ShtSpeed   : %u/%u", ExifInfo.ShutterSpeedNum, ExifInfo.ShutterSpeedDen);
                    AmbaPrint("ISO        : %d", ExifInfo.IsoSpeedRating);
                    AmbaPrint("Flash      : %d", ExifInfo.Flash);
                    AmbaPrint("MeterMode  : %d", ExifInfo.MeteringMode);
                    AmbaPrint("Sensing    : %d", ExifInfo.SensingMethod);
                    AmbaPrint("ExpMode    : %d", ExifInfo.ExposureMode);
                    AmbaPrint("LightSource: %d", ExifInfo.LightSource);
                    AmbaPrint("======== AWB =======");
                    AmbaPrint("WB         : %d", ExifInfo.WhiteBalance);
                    AmbaPrint("EVBias     : %u/%u", ExifInfo.ExposureBiasValueNum, ExifInfo.ExposureBiasValueDen);
                    AmbaPrint("ColorSpace : %d", ExifInfo.ColorSpace);
                    AmbaPrint("====================");
                }

                /* This part is for script stop verification, should be removed */
                if (ForceStopTimerHdlrID > -1) {
                    AmbaKAL_TimerStart(&ForceStopTimerHdlr);
                }
                if (AutoBackToLiveview == 0) {
                    /* DSP does not support bgproc due to HISO issue,
                     * if AutoBackToLiveview == 0, then app should go back to
                     * preview after rcv background process complete
                     */
                    if (CaptureMode == STILL_CAPTURE_SINGLE_SHOT_CONT) {
                        G_rawCapCnt +=1; // use for indicate how many raw capture done
                        if ((StillIsScapCTimeLapse == 1) && (G_rawCapCnt == G_capcnt)) {
                            /* desired capture is done. Reset continuous single shot timer hdlr. */
                            AmbaPrint("[AMP_UT] SingleCaptureCont is finished");
                            AmbaKAL_TimerStop(&ScapCTimerHdlr);
                            AmbaKAL_TimerDelete(&ScapCTimerHdlr);
                            ScapCTimerHdlrID = -1;
                        }
                    } else {
                        AmbaPrint("[AMP_UT] Capture is finished");
                    }
                }
            }
                break;
            case MSG_STILLENC_UT_BG_PROCESS_DONE:
            {
                UINT32 flag = 0;

                /* When MW inform APP BG_PROC is done,
                   that means next raw capture(script) is allow from now on */
                StillRawCaptureRunning = 0;
                AmbaPrintColor(BLUE, "[AMP_UT] STILL_SCRIPT_PROCESS_DONE");
                if (AutoBackToLiveview == 0 && Status != STATUS_STILL_LIVEVIEW) {
                    if (CaptureMode == STILL_CAPTURE_SINGLE_SHOT_CONT) {
                        if (StillIsScapCTimeLapse == 1) {
                            if (G_rawCapCnt == G_capcnt) {
                                G_rawCapCnt = 0;
                                StillContSCapShotCount = 0;
                                AmpUT_StillEncAAAUnlock();
                                AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_LIVEVIEW, 0);
                                AmpUT_StillEnc_ChangeResolution(StillEncModeIdx);
                                flag = AMP_ENC_FUNC_FLAG_WAIT;
                                if (QvDisplayCfg) flag |= AMP_ENC_FUNC_FLAG_NO_SHOW_VIDEO_PLANE;
                                AmpEnc_StartLiveview(StillEncPipe, flag);
                                Status = STATUS_STILL_LIVEVIEW;
                                AmbaPrintColor(BLUE, "[AMP_UT] Back2Liveview: Capture mode %d", CaptureMode);
                            } else {
                                StillContSCapShotCount++;
                                AmpUT_StillEncSetShotCount(StillContSCapShotCount);
                            }
                        } else if (StillIsScapCTimeLapse == 0) {
                            if (G_rawCapCnt == G_capcnt) {
                                G_rawCapCnt = 0;
                                StillContSCapShotCount = 0;
                                AmpUT_StillEncAAAUnlock();
                                AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_LIVEVIEW, 0);
                                AmpUT_StillEnc_ChangeResolution(StillEncModeIdx);
                                flag = AMP_ENC_FUNC_FLAG_WAIT;
                                if (QvDisplayCfg) flag |= AMP_ENC_FUNC_FLAG_NO_SHOW_VIDEO_PLANE;
                                AmpEnc_StartLiveview(StillEncPipe, flag);
                                Status = STATUS_STILL_LIVEVIEW;
                                AmbaPrintColor(BLUE, "[AMP_UT] Back2Liveview: Capture mode %d", CaptureMode);
                            } else {
                                // trigger next capture as soon as rcv last bg_process done
                                StillContSCapShotCount++;
                                AmpUT_StillEncSetShotCount(StillContSCapShotCount);
                                AmpUT_singleCaptureCont(ScapCont.EncID, ScapCont.Iso, ScapCont.Compressed, ScapCont.TargetSize, ScapCont.EncLoop);
                            }
                        }
                    } else if (CaptureMode == STILL_CAPTURE_BURST_SHOT_CONT ||\
                            CaptureMode == STILL_CAPTURE_BURST_SHOT_CONT_WB) {
                        if (CaptureButtonPressed == 0) {
                            AmbaPrint("[AMP_UT]  BurstCaptureCont is finished");
                            AmbaKAL_TimerStop(&CaptureButtonTimerHdlr);
                            AmbaKAL_TimerDelete(&CaptureButtonTimerHdlr);
                            CaptureButtonTimerHdlrID = -1;
                            AmpUT_StillEncAAAUnlock();
                            AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_LIVEVIEW, 0);
                            AmpUT_StillEnc_ChangeResolution(StillEncModeIdx);
                            flag = AMP_ENC_FUNC_FLAG_WAIT;
                            if (QvDisplayCfg) flag |= AMP_ENC_FUNC_FLAG_NO_SHOW_VIDEO_PLANE;
                            AmpEnc_StartLiveview(StillEncPipe, flag);
                            Status = STATUS_STILL_LIVEVIEW;
                            AmbaPrintColor(BLUE, "[AMP_UT] Back2Liveview: Capture mode %d", CaptureMode);
                        }
                    } else if (CaptureMode == STILL_CAPTURE_PRE_CAPTURE) {
                        if (CaptureButtonPressed == 0) {
                            AmbaPrint("[AMP_UT]  PreCapture is finished");
                            AmbaKAL_TimerStop(&CaptureButtonTimerHdlr);
                            AmbaKAL_TimerDelete(&CaptureButtonTimerHdlr);
                            CaptureButtonTimerHdlrID = -1;
                            AmpUT_StillEncAAAUnlock();
                            AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_LIVEVIEW, 0);
                            AmpUT_StillEnc_ChangeResolution(StillEncModeIdx);
                            flag = AMP_ENC_FUNC_FLAG_WAIT;
                            if (QvDisplayCfg) flag |= AMP_ENC_FUNC_FLAG_NO_SHOW_VIDEO_PLANE;
                            AmpEnc_StartLiveview(StillEncPipe, flag);
                            Status = STATUS_STILL_LIVEVIEW;
                            AmbaPrintColor(BLUE, "[AMP_UT] Back2Liveview: Capture mode %d", CaptureMode);
                        }
                    } else {
                    #ifdef CONFIG_SOC_A9
                        AmpUT_StillEncAAAUnlock();
                        AmpUT_StillEnc_ChangeResolution(StillEncModeIdx);
                        flag = AMP_ENC_FUNC_FLAG_WAIT;
                        if (QvDisplayCfg) flag |= AMP_ENC_FUNC_FLAG_NO_SHOW_VIDEO_PLANE;
                        AmpEnc_StartLiveview(StillEncPipe, flag);
                        Status = STATUS_STILL_LIVEVIEW;
                    #else
                        if ((QvDisplayCfg == 0) || \
                            (CaptureMode == STILL_CAPTURE_NONE) || \
                            (CaptureMode == STILL_CAPTURE_RAW2RAW) || \
                            (CaptureMode == STILL_CAPTURE_YUV2JPG) || \
                            (CaptureMode == STILL_CAPTURE_BURST_SHOT) || \
                            (CaptureMode == STILL_CAPTURE_BURST_SHOT_CONT) || \
                            (CaptureMode == STILL_CAPTURE_AEB)) {
                            AmpUT_StillEncAAAUnlock();
                            AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_STILL_LIVEVIEW, 0);
                            AmpUT_StillEnc_ChangeResolution(StillEncModeIdx);
                            AmpEnc_StartLiveview(StillEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
                            Status = STATUS_STILL_LIVEVIEW;
                            AmbaPrintColor(BLUE, "[AMP_UT] Back2Liveview: Capture mode %d", CaptureMode);
                        }
                    #endif
                    }
                #ifdef CONFIG_SOC_A12
                    IsTurnOnLcdQV = 0;
                    IsTurnOnHdmiQV = 0;
                #endif
                }

                if (StillPivProcess == 1) StillPivProcess = 0;

                /* This part is for script stop verification, should be removed */
                if (ForceStopTimerHdlrID > -1) {
                    AmbaKAL_TimerStop(&ForceStopTimerHdlr);
                    AmbaKAL_TimerDelete(&ForceStopTimerHdlr);
                    ForceStopTimerHdlrID = -1;
                }
            }
                break;
            case MSG_STILLENC_UT_SCAPC_TIMER_TRIGGER_START:
            {
                AMP_RAW_CAPTURE_PARAM_s *ScapC = (AMP_RAW_CAPTURE_PARAM_s *) msg.Data.DataInfo[0];
                AmpUT_singleCaptureCont(ScapC->EncID, ScapC->Iso, ScapC->Compressed, ScapC->TargetSize, ScapC->EncLoop);
                break;
            }
            default:
                break;
        }
    }
}

void AmpUT_StillEnc_MuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *desc;
    int er;
    UINT8 *bitsLimit = BitsBuf + STILL_BISFIFO_SIZE - 1;
    UINT8 dumpSkip = 0;

    AmbaPrint("AmpUT_StillEnc_MuxTask Start");

    while (1) {
        AmbaKAL_SemTake(&StillEncSem, AMBA_KAL_WAIT_FOREVER);

        er = AmpFifo_PeekEntry(StillEncVirtualFifoHdlr, &desc, 0);
        if (er == 0) {
            AmbaPrint("Muxer PeekEntry: size:%5d@0x%08X Ft(%d) Seg(%u)", desc->Size, desc->StartAddr, desc->Type, desc->SeqNum);
        } else {
            while (er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                er = AmpFifo_PeekEntry(StillEncVirtualFifoHdlr, &desc, 0);
            }
        }

        dumpSkip = 0;
        if (!(StillDumpSkip & STILL_ENC_SKIP_JPG)) {
            if (desc->Size != AMP_FIFO_MARK_EOS) {
                char fn[32];
                char mdASCII[3] = {'w','+','\0'};

                static UINT32 prevSegNum = 0;

                if (prevSegNum != desc->SeqNum) {
                    JpgFno++;
                    prevSegNum = desc->SeqNum;
                }
                if (desc->Type == AMP_FIFO_TYPE_JPEG_FRAME)
                    sprintf(fn,"%s:\\%04d_m.jpg", DefaultSlot, JpgFno);
                else if (desc->Type == AMP_FIFO_TYPE_THUMBNAIL_FRAME)
                    sprintf(fn,"%s:\\%04d_t.jpg", DefaultSlot, JpgFno);
                else if (desc->Type == AMP_FIFO_TYPE_SCREENNAIL_FRAME)
                    sprintf(fn,"%s:\\%04d_s.jpg", DefaultSlot, JpgFno);

                if (dumpSkip == 0) {
                    OutputFile = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
                    AmbaPrint("%s opened", fn);
                }

                bitsLimit = BitsBuf + STILL_BISFIFO_SIZE - 1;
            }
        }

        if (desc->Size == AMP_FIFO_MARK_EOS) {
            // EOS, stillenc do not produce EOS anymore...
            #ifdef AMPUT_FILE_DUMP
                if (dumpSkip == 0) {
                    //UT_StillEncodefsync((void *)OutputFile);
                    //UT_StillEncodefclose((void *)OutputFile);
                    OutputFile = NULL;
                }
            #endif
            AmbaPrint("Muxer met EOS");
        } else {
            if (!(StillDumpSkip & STILL_ENC_SKIP_JPG)) {
                static UINT8 i = 0;
                if (dumpSkip == 0) {
                    AmbaPrint("Write: 0x%x sz %d limit %X",  desc->StartAddr, desc->Size, bitsLimit);
                    if (desc->StartAddr + desc->Size <= bitsLimit) {
                        UT_StillEncodefwrite((const void *)desc->StartAddr, 1, desc->Size, (void *)OutputFile);
                    }else{
                        UT_StillEncodefwrite((const void *)desc->StartAddr, 1, bitsLimit - desc->StartAddr + 1, (void *)OutputFile);
                        UT_StillEncodefwrite((const void *)BitsBuf, 1, desc->Size - (bitsLimit - desc->StartAddr + 1), (void *)OutputFile);
                    }
                    UT_StillEncodefsync((void *)OutputFile);
                    UT_StillEncodefclose((void *)OutputFile);
                }
                i++;
                i %= 3;
                if (i == 0) {
                    if (ThmBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriThmBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail thm!");
                    #endif
                        ThmBuffAddr = NULL;
                    }

                    if (ScrnBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriScrnBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail scrn!");
                    #endif
                        ScrnBuffAddr = NULL;
                    }

                    if (YuvBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriYuvBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail yuv!");
                    #endif
                        YuvBuffAddr = NULL;
                    }

                    if (QvLCDBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriQvLCDBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail qvLCD!");
                    #endif
                        QvLCDBuffAddr = NULL;
                    }

                    if (QvHDMIBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriQvHDMIBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail qvHDMI!");
                    #endif
                        QvHDMIBuffAddr = NULL;
                    }
                }

            } else {
                static UINT8 i = 0;
                i++;
                i %= 3;
                if (i == 0) {
                    if (ThmBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriThmBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail thm!");
                    #endif
                        ThmBuffAddr = NULL;
                    }

                    if (ScrnBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriScrnBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail scrn!");
                    #endif
                        ScrnBuffAddr = NULL;
                    }

                    if (YuvBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriYuvBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail yuv!");
                    #endif
                        YuvBuffAddr = NULL;
                    }

                    if (QvLCDBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriQvLCDBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail qvLCD!");
                    #endif
                        QvLCDBuffAddr = NULL;
                    }

                    if (QvHDMIBuffAddr) {
                    #ifndef _STILL_BUFFER_FROM_DSP_
                        if (AmbaKAL_BytePoolFree((void *)OriQvHDMIBuffAddr) != OK)
                            AmbaPrint("[Amp_MUX] memFree Fail qvHDMI!");
                    #endif
                        QvHDMIBuffAddr = NULL;
                    }
                }
                AmbaKAL_TaskSleep(1);
            }
            AmpFifo_RemoveEntry(StillEncVirtualFifoHdlr, 1);
         }
    }
}

static int AmpUT_StillEnc_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *numFrames = info;

    //AmbaPrint("AmpUT_StillEnc_FifoCB on Event: 0x%x 0x%x", event, *numFrames);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for(i=0; i<*numFrames; i++) {
            AmbaKAL_SemGive(&StillEncSem);
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            AmbaKAL_SemGive(&StillEncSem);
    }

    return 0;
}


/*~muxer*/

/**
 * Generic StillEnc ImgSchdlr callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_StillEncImgSchdlrCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMBA_IMG_SCHDLR_EVENT_CFA_STAT_READY:
            if (StillEnc3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspCfa_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_RGB_STAT_READY:
            if (StillEnc3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_MAIN_CFA_HIST_READY:
            if (StillEnc3AEnable) {
                //AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                //Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_HDR_CFA_HIST_READY:
            if (StillEnc3AEnable) {
                //AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                //Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        default:
            AmbaPrint("[%s] Unknown %X info: %x", __func__, event, info);
            break;
    }
    return 0;
}

static int AmpUT_StillEncVinSwitchCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {

        case AMP_VIN_EVENT_INVALID:
            AmbaPrint("[Still]VinSWCB: AMP_VIN_EVENT_INVALID info: %X", info);

            if (SencImgSchdlr) AmbaImgSchdlr_Enable(SencImgSchdlr, 0);
            Amba_Img_VIn_Invalid(hdlr, (UINT32 *)NULL);
            break;
        case AMP_VIN_EVENT_VALID:
            AmbaPrint("[Still]VinSWCB: AMP_VIN_EVENT_VALID info: %X", info);
            if (StillEnc3AEnable) {
                Amba_Img_VIn_Valid(hdlr, (UINT32 *)NULL);
                if (SencImgSchdlr) {
                    AmbaImgSchdlr_Enable(SencImgSchdlr, 1);
                }
            }
            break;
        case AMP_VIN_EVENT_CHANGED_PRIOR:
            AmbaPrint("[Still]VinSWCB: AMP_VIN_EVENT_CHANGED_PRIOR info: %X", info);
            if (StillEnc3AEnable) {
                Amba_Img_VIn_Changed_Prior(hdlr, (UINT32 *)NULL);
            }
        #ifdef CONFIG_SOC_A12
            //LCD
            if (Status == STATUS_STILL_LIVEVIEW || StillBGProcessing) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s Window = {0}, DefWindow = {0};
                UINT8 ArIndex = 0;

                if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                Window.Source = AMP_DISP_ENC;
                Window.SourceDesc.Enc.VinCh = EncChannel;
                Window.CropArea.Width = 0;
                Window.CropArea.Height = 0;
                Window.CropArea.X = 0;
                Window.CropArea.Y = 0;
                Window.TargetAreaOnPlane.Width = StillVoutMgt[0][ArIndex].Width;
                Window.TargetAreaOnPlane.Height = StillVoutMgt[0][ArIndex].Height;
                Window.TargetAreaOnPlane.X = 0;
                Window.TargetAreaOnPlane.Y = (480 - Window.TargetAreaOnPlane.Height)/2;
                if(AmpUT_Display_GetWindowCfg(0, &DefWindow) != OK) {
                    AmpUT_Display_Window_Create(0, &Window);
                } else {
                    AmpUT_Display_SetWindowCfg(0, &Window);
                }
                AmpUT_Display_Act_Window(0);
            }

            //Tv
            if (Status == STATUS_STILL_LIVEVIEW || StillBGProcessing) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s Window = {0}, DefWindow = {0};
                UINT8 ArIndex = 0;

                if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                Window.Source = AMP_DISP_ENC;
                Window.SourceDesc.Enc.VinCh = EncChannel;
                Window.CropArea.Width = 0;
                Window.CropArea.Height = 0;
                Window.CropArea.X = 0;
                Window.CropArea.Y = 0;
                Window.TargetAreaOnPlane.Width = StillVoutMgt[1][ArIndex].Width;
                Window.TargetAreaOnPlane.Height = StillVoutMgt[1][ArIndex].Height;
                Window.TargetAreaOnPlane.X = (1920 - Window.TargetAreaOnPlane.Width)/2;
                Window.TargetAreaOnPlane.Y = 0;
                if(AmpUT_Display_GetWindowCfg(1, &DefWindow) != OK) {
                    AmpUT_Display_Window_Create(1, &Window);
                } else {
                    AmpUT_Display_SetWindowCfg(1, &Window);
                }
                AmpUT_Display_Act_Window(1);
            }

        #endif
            break;
        case AMP_VIN_EVENT_CHANGED_POST:
            AmbaPrint("[Still]VinSWCB: AMP_VIN_EVENT_CHANGED_POST info: %X", info);
            if (StillEnc3AEnable) {
                UINT8 IsPhotoLiveView = 1;
                UINT16 PipeMode = IP_EXPERSS_MODE;
                UINT16 AlgoMode = IP_MODE_LISO_VIDEO;
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo;
                AMBA_SENSOR_MODE_ID_u SensorMode = {0};

                SensorMode.Data = (StillEncodeSystem==0)? StillEncMgt[StillEncModeIdx].InputMode: StillEncMgt[StillEncModeIdx].InputPALMode;
                AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);

                if (SencImgSchdlr) {
                    AMBA_IMG_SCHDLR_UPDATE_CFG_s SchdlrCfg = {0};

                    AmbaImgSchdlr_GetConfig(SencImgSchdlr, &SchdlrCfg);
                    if (StillLiveViewProcMode && StillLiveViewAlgoMode) {
                        SchdlrCfg.VideoProcMode = 1;
                        if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                            SchdlrCfg.VideoProcMode |= 0x10;
                        }
                    }
                    AmbaImgSchdlr_SetConfig(SencImgSchdlr, &SchdlrCfg);  // One MainViewID (not vin) need one scheduler.
                }

            #ifdef CONFIG_SOC_A12
                { // Load IQ params
                    extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
                    UINT8 IsSensorHdrMode;
                    // Inform 3A LV sensor mode is Hdr or not
                    IsSensorHdrMode = (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE)? 1: 0;
                    AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_HDR_ENABLE, 0/*ChNo*/, (UINT32)IsSensorHdrMode, 0);

                    if (StillEncIsIqParamInit == 0 || IsSensorHdrMode != StillEncIsHdrIqParam) {
                        App_Image_Init_Iq_Params(0, StillSensorIdx);
                        StillEncIsIqParamInit = 1;
                        StillEncIsHdrIqParam = IsSensorHdrMode;
                    }
                }
            #endif

                //inform 3A LiveView pipeline
                if (StillLiveViewProcMode && StillLiveViewAlgoMode) {
                    PipeMode = IP_HYBRID_MODE;
                } else {
                    PipeMode = IP_EXPERSS_MODE;
                }
                AmbaImg_Proc_Cmd(MW_IP_SET_PIPE_MODE, 0/*ChNo*/, (UINT32)PipeMode, 0);

                //inform 3A LiveView Algo
                if (StillLiveViewAlgoMode == 0 || StillLiveViewAlgoMode == 1) {
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
                    LiveViewInfo.MainW = StillEncMgt[StillEncModeIdx].MainWidth;
                    LiveViewInfo.MainH = StillEncMgt[StillEncModeIdx].MainHeight;
                    if (StillEncodeSystem == 0) {
                        LiveViewInfo.FrameRateInt = StillEncMgt[StillEncModeIdx].TimeScale/StillEncMgt[StillEncModeIdx].TickPerPicture;
                    } else {
                        LiveViewInfo.FrameRateInt = StillEncMgt[StillEncModeIdx].TimeScalePAL/StillEncMgt[StillEncModeIdx].TickPerPicturePAL;
                    }
                    LiveViewInfo.FrameRateInt = UT_StillFrameRateIntConvert(LiveViewInfo.FrameRateInt);
                    AmbaSensor_GetStatus(EncChannel, &SsrStatus);
                    LiveViewInfo.BinningHNum = InputInfo->HSubsample.FactorNum;
                    LiveViewInfo.BinningHDen = InputInfo->HSubsample.FactorDen;
                    LiveViewInfo.BinningVNum = InputInfo->VSubsample.FactorNum;
                    LiveViewInfo.BinningVDen = InputInfo->VSubsample.FactorDen;
                    AmbaImg_Proc_Cmd(MW_IP_SET_LIVEVIEW_INFO, 0/*ChIndex*/, (UINT32)&LiveViewInfo, 0);
                }

                //inform 3A FrameRate info
                {
					UINT32 FrameRate = 0, FrameRatex1000 = 0;

                    if (StillEncodeSystem == 0) {
                        FrameRate = StillEncMgt[StillEncModeIdx].TimeScale/StillEncMgt[StillEncModeIdx].TickPerPicture;
                        FrameRatex1000 = StillEncMgt[StillEncModeIdx].TimeScale*1000/StillEncMgt[StillEncModeIdx].TickPerPicture;
                    } else {
                        FrameRate = StillEncMgt[StillEncModeIdx].TimeScalePAL/StillEncMgt[StillEncModeIdx].TickPerPicturePAL;
                        FrameRatex1000 = StillEncMgt[StillEncModeIdx].TimeScalePAL*1000/StillEncMgt[StillEncModeIdx].TickPerPicturePAL;
                    }
                    FrameRate = UT_StillFrameRateIntConvert(FrameRate);
                    AmbaImg_Proc_Cmd(MW_IP_SET_FRAME_RATE, 0/*ChIndex*/, FrameRate, FrameRatex1000);
                }

                Amba_Img_VIn_Changed_Post(hdlr, (UINT32 *)NULL);
            }
        #ifdef CONFIG_SOC_A9
            if ((Status == STATUS_STILL_INIT || \
                Status == STATUS_STILL_LIVEVIEW || \
                StillBGProcessing) && EncLcdWinHdlr) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window;
                UINT8 ArIndex = 0;

                if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
                window.Source = AMP_DISP_ENC;
                window.SourceDesc.Enc.VinCh = EncChannel;
                window.CropArea.Width = 0;
                window.CropArea.Height = 0;
                window.CropArea.X = 0;
                window.CropArea.Y = 0;
                window.TargetAreaOnPlane.Width = StillVoutMgt[0][ArIndex].Width;
                window.TargetAreaOnPlane.Height = StillVoutMgt[0][ArIndex].Height;
                window.TargetAreaOnPlane.X = 0;
                window.TargetAreaOnPlane.Y = (480 - window.TargetAreaOnPlane.Height)/2;
                AmpDisplay_SetWindowCfg(EncLcdWinHdlr, &window);
                AmpDisplay_SetWindowActivateFlag(EncLcdWinHdlr, 1);
                AmpDisplay_Update(LCDHdlr);
            #if 0
                VoutLcdSrc = AMP_DISP_ENC;
            #endif
            }

            if ((Status == STATUS_STILL_INIT || \
                Status == STATUS_STILL_LIVEVIEW || \
                StillBGProcessing) && EncTvWinHdlr) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window;
                UINT8 ArIndex = 0;

                if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
                window.Source = AMP_DISP_ENC;
                window.SourceDesc.Enc.VinCh = EncChannel;
                window.CropArea.Width = 0;
                window.CropArea.Height = 0;
                window.CropArea.X = 0;
                window.CropArea.Y = 0;
                window.TargetAreaOnPlane.Width = StillVoutMgt[1][ArIndex].Width;
                window.TargetAreaOnPlane.Height = StillVoutMgt[1][ArIndex].Height;
                window.TargetAreaOnPlane.X = (1920 - window.TargetAreaOnPlane.Width)/2;
                window.TargetAreaOnPlane.Y = 0;
                AmpDisplay_SetWindowCfg(EncTvWinHdlr, &window);
                AmpDisplay_SetWindowActivateFlag(EncTvWinHdlr, 1);
                AmpDisplay_Update(TVHdlr);
            #if 0
                VoutTvSrc = AMP_DISP_ENC;
            #endif
            }
        #endif
            break;
        default:
            AmbaPrint("[Still]VinSWCB: Unknown %X info: %x", event, info);
           break;
    }
    return 0;
}

//static int xcnt = 0, ycnt = 0, zcnt = 0; // Just to reduce console print
static int AmpUT_StillEncVinEventCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {
        case AMP_VIN_EVENT_FRAME_READY:
          /*
            if (xcnt % 30 == 0)
              AmbaPrint("[Still]VinEVCB: AMP_VIN_EVENT_FRAME_READY info: %X", info);
            xcnt++;
          */
            break;
        case AMP_VIN_EVENT_FRAME_DROPPED:
            AmbaPrint("[Still]VinEVCB: AMP_VIN_EVENT_FRAME_DROPPED info: %X", info);
            break;
        default:
            AmbaPrint("[Still]VinEVCB: Unknown %X info: %x", event, info);
           break;
    }
    return 0;
}

static int AmpUT_StillEncCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_START:
        {
            STILLENC_UT_MSG_S Msg = {0};
            Msg.Event = MSG_STILLENC_UT_RAW_CAPTURE_START;
            Msg.Data.DataInfo[0] = (UINT32)info;
            AmbaKAL_MsgQueueSend(&UtMsgQueue, &Msg, AMBA_KAL_NO_WAIT);
        }
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_DONE:
        {
            STILLENC_UT_MSG_S Msg = {0};
            Msg.Event = MSG_STILLENC_UT_RAW_CAPTURE_DONE;
            AmbaKAL_MsgQueueSend(&UtMsgQueue, &Msg, AMBA_KAL_NO_WAIT);
        }
            break;
        case AMP_ENC_EVENT_BACK_TO_LIVEVIEW:
        {
            // if script.b2lv > 0,
            // this event_CB is invoke when system already back to liveview
            if (AutoBackToLiveview > 0) {
                Status = STATUS_STILL_LIVEVIEW;
            }
        }
            break;
        case AMP_ENC_EVENT_BG_PROCESS_DONE:
        {   // BG_PROCESS_DONE means mw finish whole script, app can go b2lv from now on
            STILLENC_UT_MSG_S Msg = {0};
            Msg.Event = MSG_STILLENC_UT_BG_PROCESS_DONE;
            AmbaKAL_MsgQueueSend(&UtMsgQueue, &Msg, AMBA_KAL_NO_WAIT);

            //whole script process done.
            StillBGProcessing = 0;
        }
            break;
        case AMP_ENC_EVENT_LIVEVIEW_RAW_READY:
#ifdef CONFIG_SOC_A12
            if (StillEnc3AEnable &&
                ((StillLiveViewProcMode == 0 && StillLiveViewOSMode == 1) || \
                StillLiveViewProcMode == 1)) {
                extern int Amba_Img_VDspRaw_Handler(void *hdlr, UINT32 *pRgbData)  __attribute__((weak));
                AMP_ENC_RAW_INFO_s *RawInfo = info;
                AMBA_IMG_RAW_INFO_s RawBufInfo = {0};
                AMBA_SENSOR_MODE_ID_u Mode = {0};
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo;

                Mode.Data = (StillEncodeSystem == 0)? StillEncMgt[StillEncModeIdx].InputMode: StillEncMgt[StillEncModeIdx].InputPALMode;
                AmbaSensor_GetModeInfo(EncChannel, Mode, &SensorModeInfo);

                RawBufInfo.Compressed = RawInfo->compressed;
                RawBufInfo.pBaseAddr = RawInfo->RawAddr;
                RawBufInfo.Pitch = RawInfo->RawPitch;
                RawBufInfo.BayerPattern = SensorModeInfo.OutputInfo.CfaPattern;
                RawBufInfo.NumDataBits = SensorModeInfo.OutputInfo.NumDataBits;
                RawBufInfo.Window.Width = RawInfo->RawWidth;
                RawBufInfo.Window.Height = RawInfo->RawHeight;

                RawBufInfo.OBWindow.Width = RawInfo->OBWidth;
                RawBufInfo.OBWindow.Height = RawInfo->OBHeight;
                RawBufInfo.OBWindow.OffsetX = RawInfo->OBOffsetX;
                RawBufInfo.OBWindow.OffsetY = RawInfo->OBOffsetY;
                Amba_Img_VDspRaw_Handler(hdlr, (UINT32 *)&RawBufInfo);
            }
#endif
            break;
        case AMP_ENC_EVENT_LIVEVIEW_DCHAN_YUV_READY:
            {
              //AMP_ENC_YUV_INFO_s *ptr = info;
              //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
              //AmbaPrint("[AmpUT_StillEnc] AMP_STILLENC_MSG_LIVEVIEW_DCHAN_YUV_READY addr: %X p:%d %dx%d", inf.yAddr, inf.pitch, inf.width, inf.height);
            }
            break;
        case AMP_ENC_EVENT_LIVEVIEW_FCHAN_YUV_READY:
            {
              //AMP_ENC_YUV_INFO_s *ptr = info;
              //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
              //AmbaPrint("[AmpUT_StillEnc] AMP_STILLENC_MSG_LIVEVIEW_FCHAN_YUV_READY info: %X", info);
            }
            break;
        case AMP_ENC_EVENT_VCAP_YUV_READY:
            //AmbaPrint("[AmpUT_StillEnc]: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VCAP_2ND_YUV_READY:
            //AmbaPrint("[AmpUT_StillEnc]: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_2ND_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VCAP_ME1_Y_READY:
            //AmbaPrint("[AmpUT_StillEnc]: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_ME1_Y_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_HYBRID_YUV_READY:
            //AmbaPrint("[AmpUT_StillEnc]: !!!!!!!!!!! AMP_ENC_EVENT_HYBRID_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VDSP_ASSERT:
            //AmbaPrintColor(RED, "[AmpUT_StillEnc]: !!!!!!!!!!! AMP_ENC_EVENT_VDSP_ASSERT !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_QUICKVIEW_DCHAN_YUV_READY:
        {
            // In A9, app must copy LCD or HDMI qv to their local buffer and ask DISPLAY module to show defIMG.
            // But in A12, we need to switch display to video src as soon as qv_data_rdy to prevent vout buffer full.
            // preview_a buffer is control by uCode in a12, APP is able to access it.
            // preview_b buffer is control by uCode in a12, App is unable to access it.
#ifdef CONFIG_SOC_A12
            {
                if (IsTurnOnLcdQV == 0) {
                    AMP_DISP_WINDOW_CFG_s window;
                    UINT8 ArIndex = 0;
                    IsTurnOnLcdQV = 1;
                    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                    if (1) {
                        memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
                        window.Source = AMP_DISP_ENC;
                        window.SourceDesc.Enc.VinCh = EncChannel;
                        window.CropArea.Width = 0;
                        window.CropArea.Height = 0;
                        window.CropArea.X = 0;
                        window.CropArea.Y = 0;
                        window.TargetAreaOnPlane.Width = StillVoutMgt[0][ArIndex].Width;
                        window.TargetAreaOnPlane.Height = StillVoutMgt[0][ArIndex].Height;
                        window.TargetAreaOnPlane.X = 0;
                        window.TargetAreaOnPlane.Y = (480 - window.TargetAreaOnPlane.Height)/2;
                        AmpUT_Display_SetWindowCfg(0, &window);
                        AmpUT_Display_Act_Window(0);
                        AmbaPrint("[AmpUT_StillEnc] QV_DCHAN_YUV_READY, switch to VDSRC");
                    } else {
                        AmbaPrint("[AmpUT_StillEnc] ERROR, A12 MUST turn on VDSRC as soon as received QV_DCHAN_YUV_READY");
                    }
                } else {
                    // AmbaPrint("[AmpUT_StillEnc] QV_DCHAN_YUV_READY, Already switch to VDSRC");
                }
            }
#endif
            {
                AMP_ENC_YUV_INFO_s *YuvInfo = info;
                //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
                AmbaPrint("[AmpUT_StillEnc] AMP_ENC_EVENT_QUICKVIEW_DCHAN_YUV_READY info: 0x%X (%d %d %d) %u qv %d", YuvInfo->yAddr, YuvInfo->pitch, YuvInfo->width, YuvInfo->height, YuvInfo->ySize, QvDisplayCfg);
                //AmbaKAL_TaskSleep(3); //temp solution to wait memory sync

            #if 0 // dump DCHAN_QV
                {
                    char fn[32], fn1[32];
                    UINT8 *LumaAddr, *ChromaAddr;
                    UINT16 Pitch;
                    AMP_CFS_FILE_s *y = NULL;
                    AMP_CFS_FILE_s *uv = NULL;
                    char mdASCII[3] = {'w','+','\0'};
                    AMP_AREA_s Aoi;

                    sprintf(fn,"%s:\\%04d_D.y", DefaultSlot, YuvFno);
                    sprintf(fn1,"%s:\\%04d_D.uv", DefaultSlot, YuvFno);

                    memset(&Aoi, 0x0, sizeof(AMP_AREA_s));
                    LumaAddr = YuvInfo->yAddr;
                    ChromaAddr = YuvInfo->uvAddr;
                    Pitch = YuvInfo->pitch;
                    Aoi.Width = YuvInfo->width;
                    Aoi.Height = YuvInfo->height;
                    Aoi.X = Aoi.Y = 0;

                    AmbaPrint("[Amp_UT] Dump DCHAN YUV (%s %s) to (0x%X 0x%X) %d %d %d Start!", \
                        fn, fn1, LumaAddr, ChromaAddr, Pitch, Aoi.Width, Aoi.Height);
                    y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
                    AmpUT_DumpAoi(LumaAddr, Pitch, Aoi, (void *)y);
                    UT_StillEncodefsync((void *)y);
                    UT_StillEncodefclose((void *)y);

                    uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
                    AmpUT_DumpAoi(ChromaAddr, Pitch, Aoi, (void *)uv);
                    UT_StillEncodefsync((void *)uv);
                    UT_StillEncodefclose((void *)uv);
                    AmbaPrint("[Amp_UT] Dump DCHAN YUV Done!");
                }
            #endif

                if (QvDisplayCfg != 0 && (CaptureMode != STILL_CAPTURE_BURST_SHOT) &&\
                    (CaptureMode != STILL_CAPTURE_BURST_SHOT_CONT) && \
                    (CaptureMode != STILL_CAPTURE_BURST_SHOT_CONT_WB) && \
                    (CaptureMode != STILL_CAPTURE_AEB) && \
                    (CaptureMode != STILL_CAPTURE_PRE_CAPTURE) && \
                    (CaptureMode != STILL_CAPTURE_SINGLE_SHOT_CONT)) {
                    UINT8* BufferAddr;
                    STILLENC_UT_MSG_S msg = {0};

                    if (QvLcdShowBuffIndex == 0) {
                        //Even buffer is using
                        memcpy(QvLcdShowBuffAddrOdd, YuvInfo->yAddr, YuvInfo->ySize);
                        BufferAddr = QvLcdShowBuffAddrOdd + YuvInfo->ySize;
                        memcpy(BufferAddr, YuvInfo->uvAddr, YuvInfo->ySize);
                        AmbaCache_Clean((void*)QvLcdShowBuffAddrOdd, YuvInfo->ySize*2);
                    } else {
                        //Odd buffer is using
                        memcpy(QvLcdShowBuffAddrEven, YuvInfo->yAddr, YuvInfo->ySize*2);
                        BufferAddr = QvLcdShowBuffAddrEven + YuvInfo->ySize;
                        memcpy(BufferAddr, YuvInfo->uvAddr, YuvInfo->ySize);
                        AmbaCache_Clean((void*)QvLcdShowBuffAddrEven, YuvInfo->ySize*2);
                    }

                    msg.Event = MSG_STILLENC_UT_QV_LCD_READY;
                    msg.Data.yuvInfo.yAddr = (QvLcdShowBuffIndex == 0)? QvLcdShowBuffAddrOdd: QvLcdShowBuffAddrEven;
                    msg.Data.yuvInfo.ySize = YuvInfo->ySize;
                    msg.Data.yuvInfo.pitch = YuvInfo->pitch;
                    msg.Data.yuvInfo.width = YuvInfo->width;
                    msg.Data.yuvInfo.height = YuvInfo->height;
                    AmbaKAL_MsgQueueSend(&UtMsgQueue, &msg, AMBA_KAL_NO_WAIT);
                }
            }


        }
            break;
        case AMP_ENC_EVENT_QUICKVIEW_FCHAN_YUV_READY:
        {
            // In A9, app must copy LCD or HDMI qv to their local buffer and ask DISPLAY module to show defIMG.
            // But in A12, we need to switch display to video src as soon as qv_data_rdy to prevent vout buffer full.
            // preview_a buffer is control by uCode in a12, APP is able to access it.
            // preview_b buffer is control by uCode in a12, App is unable to access it.
#ifdef CONFIG_SOC_A12
            if (IsTurnOnHdmiQV == 0) {
                AMP_DISP_WINDOW_CFG_s window;
                UINT8 ArIndex = 0;
                IsTurnOnHdmiQV = 1;
                if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
                else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

                if (1) {
                    memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
                    window.Source = AMP_DISP_ENC;
                    window.SourceDesc.Enc.VinCh = EncChannel;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;
                    window.TargetAreaOnPlane.Width = StillVoutMgt[1][ArIndex].Width;
                    window.TargetAreaOnPlane.Height = StillVoutMgt[1][ArIndex].Height;
                    window.TargetAreaOnPlane.X = (1920 - window.TargetAreaOnPlane.Width)/2;
                    window.TargetAreaOnPlane.Y = 0;
                    AmpUT_Display_SetWindowCfg(1, &window);
                    AmpUT_Display_Act_Window(1);
                    AmbaPrint("[AmpUT_StillEnc] QV_FCHAN_YUV_READY, switch to VDSRC");

                } else {
                    AmbaPrint("[AmpUT_StillEnc] ERROR, A12 MUST turn on VDSRC as soon as received QV_FCHAN_YUV_READY");
                }
            } else {
                // AmbaPrint("[AmpUT_StillEnc] QV_FCHAN_YUV_READY, Already switch to VDSRC");
            }
#else
            AMP_ENC_YUV_INFO_s *ptr = info;
            //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
            AmbaPrint("[AmpUT_StillEnc] AMP_ENC_EVENT_QUICKVIEW_FCHAN_YUV_READY info: 0x%X (%d %d %d) %u", ptr->yAddr, ptr->pitch, ptr->width, ptr->height, ptr->ySize);
            AmbaKAL_TaskSleep(3); //temp solution to wait memory sync

        #if 0
            {
                char fn[32], fn1[32];
                UINT8 *LumaAddr, *ChromaAddr;
                UINT16 Pitch;
                AMP_CFS_FILE_s *y = NULL;
                AMP_CFS_FILE_s *uv = NULL;
                char mdASCII[3] = {'w','+','\0'};
                AMP_AREA_s Aoi;

                sprintf(fn,"%s:\\%04d_F.y", DefaultSlot, YuvFno);
                sprintf(fn1,"%s:\\%04d_F.uv", DefaultSlot, YuvFno);
                LumaAddr = ptr->yAddr;
                ChromaAddr = ptr->uvAddr;
                Pitch = ptr->pitch;
                Aoi.Width = ptr->width;
                Aoi.Height = ptr->height;
                Aoi.X = Aoi.Y = 0;

                AmbaPrint("[Amp_UT] Dump FCHAN YUV (%s %s) to (0x%X 0x%X) %d %d %d Start!", \
                    fn, fn1, LumaAddr, ChromaAddr, Pitch, Aoi.Width, Aoi.Height);
                y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
                AmpUT_DumpAoi(LumaAddr, Pitch, Aoi, (void *)y);
                UT_StillEncodefsync((void *)y);
                UT_StillEncodefclose((void *)y);

                uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
                AmpUT_DumpAoi(ChromaAddr, Pitch, Aoi, (void *)uv);
                UT_StillEncodefsync((void *)uv);
                UT_StillEncodefclose((void *)uv);
                AmbaPrint("[Amp_UT] Dump FCHAN YUV Done!");
            }
        #endif

            if (QvDisplayCfg != 0 && (CaptureMode != STILL_CAPTURE_BURST_SHOT) &&\
                (CaptureMode != STILL_CAPTURE_BURST_SHOT_CONT) && \
                (CaptureMode != STILL_CAPTURE_BURST_SHOT_CONT_WB) && \
                (CaptureMode != STILL_CAPTURE_AEB) && \
                (CaptureMode != STILL_CAPTURE_PRE_CAPTURE)) {
                //TBD
                #if 0
                    AmbaPrint("[Amp_UT] Copy FCHAN YUV Start!");
                    if (QvTvShowBuffIndex == 0) {
                        //Even buffer is using
                        memcpy(QvTvShowBuffAddrOdd, ptr->yAddr, ptr->ySize*2);
                        AmbaCache_Clean((void*)QvTvShowBuffAddrOdd, ptr->ySize*2);
                    } else {
                        //Odd buffer is using
                        memcpy(QvTvShowBuffAddrEven, ptr->yAddr, ptr->ySize*2);
                        AmbaCache_Clean((void*)QvTvShowBuffAddrEven, ptr->ySize*2);
                    }
                    AmbaPrint("[Amp_UT] Copy FCHAN YUV Done!");
                #endif
            }
#endif
        }
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("[AmpUT_StillEnc] ~~~ AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD ~~~");
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("[AmpUT_StillEnc] ~~~ AMP_ENC_EVENT_DATA_OVERRUN ~~~");
            break;
        case AMP_ENC_EVENT_DESC_OVERRUN:
            AmbaPrint("[AmpUT_StillEnc] ~~~ AMP_ENC_EVENT_DESC_OVERRUN ~~~");
            break;
        default:
            AmbaPrint("[AmpUT_StillEnc] Unknown %X info: %x", event, info);
            break;
    }
    return 0;
}

static int AmpUT_StillEncPipeCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
        {
            AMP_ENC_STATE_CHANGED_INFO_s *inf = info;
            AmbaPrint("[Still]PipeCB: AMP_ENC_EVENT_STATE_CHANGED newState %X", inf->newState);
        }
        break;
    }
    return 0;

}

static int AmpUT_StillEnc_DisplayStart(void)
{
    AMP_DISP_WINDOW_CFG_s window;
    UINT8 ArIndex = 0;

    if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_4x3) ArIndex = 0;
    else if (StillEncMgt[StillEncModeIdx].AspectRatio == VAR_16x9) ArIndex = 1;

    /** Step 1: Display config & window config */
//    if (AmpUT_Display_Init() == NG) {
//        return NG;
//    }

    // Creat LCD Window
    if (1) {
        memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
        window.Source = AMP_DISP_ENC;
        window.SourceDesc.Enc.VinCh = EncChannel;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;
        window.TargetAreaOnPlane.Width = StillVoutMgt[0][ArIndex].Width;
        window.TargetAreaOnPlane.Height = StillVoutMgt[0][ArIndex].Height;
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
        window.SourceDesc.Enc.VinCh = EncChannel;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;
        window.TargetAreaOnPlane.Width = StillVoutMgt[1][ArIndex].Width;
        window.TargetAreaOnPlane.Height = StillVoutMgt[1][ArIndex].Height;
        window.TargetAreaOnPlane.X = (1920-window.TargetAreaOnPlane.Width)/2;
        window.TargetAreaOnPlane.Y = 0;
        if (AmpUT_Display_Window_Create(1, &window) == NG) {
            return NG;
        }
    }

    /** Step 2: Setup device */
    // Setup LCD & TV
    if (1) {
        AmpUT_Display_Start(0);
        if (TVLiveViewEnable) {
            AmpUT_Display_Start(1);
        } else {
            AmpUT_Display_Stop(1);
        }
    }

    // Active Window 7002 cmd
    AmpUT_Display_Act_Window(0);
    if (TVLiveViewEnable) {
        AmpUT_Display_Act_Window(1);
    } else {
        AmpUT_Display_DeAct_Window(1);
    }

    return 0;

}

int AmpUT_StillEnc_Init(int sensorID, int LcdID)
{
    int er = OK;
    void *TmpbufRaw = NULL;
    if (StillCodecInit) {
        AmbaPrint("StillEnc already init");
        return er;
    }

    //imgproc initialization

    // Register LCD driver
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
        memset(&EncChannel, 0x0, sizeof(AMBA_DSP_CHANNEL_ID_u));
        EncChannel.Bits.VinID = 0;
        EncChannel.Bits.SensorID = 0x1;
        StillEncMgt = RegisterMWUT_Sensor_Driver(EncChannel, sensorID);
        {
    #ifdef CONFIG_SOC_A12 // Temporarily disable for A9
            extern int App_Image_Init(UINT32 ChCount, int sensorID);
            App_Image_Init(1, sensorID);
    #endif
        }
    }

    // Create simple muxer semophore
    if (AmbaKAL_SemCreate(&StillEncSem, 0) != OK) {
        AmbaPrint("StillEnc UnitTest: Semaphore creation failed");
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_StillEncMuxStack, (void **)&TmpbufRaw, STILL_ENC_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    // Create simple muxer task
    if (AmbaKAL_TaskCreate(&StillEncMuxTask, "Still Encoder UnitTest Muxing Task", 10, \
         AmpUT_StillEnc_MuxTask, 0x0, AmpUT_StillEncMuxStack, STILL_ENC_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("StillEnc UnitTest: Muxer task creation failed");
    }

    // Simple UnitTest Task
    {
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_StillEncUtStack, (void **)&TmpbufRaw, STILL_ENC_MUX_UT_STACK_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for UnitTest stack!!");
        }

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_StillEncUtMsgQPool, (void **)&TmpbufRaw, (sizeof(STILLENC_UT_MSG_S)*MAX_STILL_UT_MSG_NUM), 32);
        if (er != OK) {
            AmbaPrint("Out of memory for UnitTestMsgQPool!!");
        }

        // Create mQueue
        er = AmbaKAL_MsgQueueCreate(&UtMsgQueue, AmpUT_StillEncUtMsgQPool, sizeof(STILLENC_UT_MSG_S), MAX_STILL_UT_MSG_NUM);
        if (er != OK) {
            AmbaPrint("UnitTest MsgQPoll create fail");
        }

        // Create simple still_UT task
        if (AmbaKAL_TaskCreate(&StillEncUtTask, "Still Encoder UnitTest Task", 50, \
             AmpUT_StillEnc_UtTask, 0x0, AmpUT_StillEncUtStack, STILL_ENC_MUX_UT_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
             AmbaPrint("UnitTest task creation failed");
        }
    }

    // Vin module init
    {
        AMP_VIN_INIT_CFG_s VinInitCfg;

        // Init VIN module
        AmpVin_GetInitDefaultCfg(&VinInitCfg);
        if (StillVinWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillVinWorkBuf, (void **)&TmpbufRaw, VinInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for vin!!");
            }
        }
        VinInitCfg.MemoryPoolAddr = StillVinWorkBuf;
        AmpVin_Init(&VinInitCfg);
    }

    // Create Still encoder object
    {
        AMP_STILLENC_INIT_CFG_s EncInitCfg;

        // Init STILLENC module
        AmpStillEnc_GetInitDefaultCfg(&EncInitCfg);
        if (StillEncWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillEncWorkBuf, (void **)&TmpbufRaw, EncInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for stillmain!!");
            }
        }
        EncInitCfg.MemoryPoolAddr = StillEncWorkBuf;
        AmpStillEnc_Init(&EncInitCfg);
    }

    //ImgSchdlr module init
    {
        AMBA_IMG_SCHDLR_INIT_CFG_s ImgSschdlrInitCfg;
        AMBA_DSP_IMG_PIPE_e Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
        UINT32 MainViewNumber = 1;
        UINT32 ISPoolSize = 0;
        AmbaImgSchdlr_GetInitDefaultCfg(&ImgSschdlrInitCfg);
        ImgSschdlrInitCfg.MainViewNum = MainViewNumber;
        AmbaImgSchdlr_QueryMemsize(MainViewNumber, &ISPoolSize);
#ifdef CONFIG_SOC_A9
        ImgSschdlrInitCfg.MemoryPoolSize = ISPoolSize;
#else
        ImgSschdlrInitCfg.MemoryPoolSize = ISPoolSize + ImgSschdlrInitCfg.MsgTaskStackSize;
#endif

        if (StillImgSchdlrWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillImgSchdlrWorkBuf, (void **)&TmpbufRaw, ImgSschdlrInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for imgschdlr!!");
            }
        }
        ImgSschdlrInitCfg.IsoCfgNum = AmpResource_GetIKIsoConfigNumber(Pipe);
        ImgSschdlrInitCfg.MemoryPoolAddr = StillImgSchdlrWorkBuf;
        er = AmbaImgSchdlr_Init(&ImgSschdlrInitCfg);
        if (er != AMP_OK) {
            AmbaPrint("AmbaImgSchdlr_Init Fail!");
        }
    }

    // Initialize Dzoom module
    {
        AMP_IMG_DZOOM_INIT_CFG_s DzoomInitCfg;
        AmpImgDzoom_GetInitDefaultCfg(&DzoomInitCfg);
        AmpImgDzoom_Init(&DzoomInitCfg);
    }

    if (AmpUT_Display_Init() == NG) {
        return NG;
    }

    StillCodecInit = 1;

    return 0;
}

int AmpUT_StillEnc_LiveviewStart(UINT32 encID)
{
    int er = OK;
    void *TmpbufRaw = NULL;
    StillEncModeIdx = encID;

    if (StillEncVinA) {
        if (SencImgSchdlr == NULL) {
            AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg = {0};

            AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

            ImgSchdlrCfg.MainViewID = 0; //single channel have one MainView
            ImgSchdlrCfg.Channel = EncChannel;
            ImgSchdlrCfg.Vin = StillEncVinA;
            ImgSchdlrCfg.cbEvent = AmpUT_StillEncImgSchdlrCallback;
            if (StillLiveViewProcMode && StillLiveViewAlgoMode) {
                ImgSchdlrCfg.VideoProcMode = 1;
            }
            AmbaImgSchdlr_Create(&ImgSchdlrCfg, &SencImgSchdlr);  // One sensor (not vin) need one scheduler.
        }

        AmpUT_StillEnc_ChangeResolution(encID);
        AmpEnc_StartLiveview(StillEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
        return 0;
    }

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: LiveviewStart at %s", MWUT_GetInputStillModeName(encID, StillEncodeSystem));
    AmbaPrint(" =========================================================");

    // Create Vin instance
    if (StillEncVinA == NULL) {
        AMBA_SENSOR_MODE_INFO_s VinInfo = {0};
        AMP_VIN_HDLR_CFG_s VinCfg = {0};
        AMP_VIN_LAYOUT_CFG_s Layout = {0};
        AMBA_SENSOR_MODE_ID_u SensorMode = {0};
        SensorMode.Data = StillEncMgt[encID].InputMode;
        AmbaSensor_GetModeInfo(EncChannel, SensorMode, &VinInfo);
        // Create VIN instance
        AmpVin_GetDefaultCfg(&VinCfg);
        VinCfg.Channel.Bits.VinID = 0;
        VinCfg.Channel.Bits.SensorID = 0x1;
        VinCfg.Mode = SensorMode;
    #ifdef CONFIG_SOC_A9
        VinCfg.Mode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
    #endif
        VinCfg.LayoutNumber = 1;

        if (StillOBModeEnable && VinInfo.OutputInfo.OpticalBlackPixels.Width && VinInfo.OutputInfo.OpticalBlackPixels.Height) {
            INT16 ActStartX = MIN2(VinInfo.OutputInfo.OpticalBlackPixels.StartX, VinInfo.OutputInfo.RecordingPixels.StartX);
            INT16 ActStartY = MIN2(VinInfo.OutputInfo.OpticalBlackPixels.StartY, VinInfo.OutputInfo.RecordingPixels.StartY);

            VinCfg.HwCaptureWindow.Width = VinInfo.OutputInfo.OutputWidth - ActStartX;
            VinCfg.HwCaptureWindow.Height = VinInfo.OutputInfo.OutputHeight - ActStartY;
            VinCfg.HwCaptureWindow.X = ActStartX;
            VinCfg.HwCaptureWindow.Y = ActStartY;
            Layout.ActiveArea.Width = StillEncMgt[encID].CaptureWidth;
            Layout.ActiveArea.Height = StillEncMgt[encID].CaptureHeight;
            Layout.ActiveArea.X = (VinInfo.OutputInfo.RecordingPixels.StartX - ActStartX) + \
                (((VinInfo.OutputInfo.RecordingPixels.Width - Layout.ActiveArea.Width)/2)&0xFFF8);
            Layout.ActiveArea.Y = (VinInfo.OutputInfo.RecordingPixels.StartY - ActStartY) + \
                (((VinInfo.OutputInfo.RecordingPixels.Height - Layout.ActiveArea.Height)/2)&0xFFFE);
            Layout.OBArea.Width = VinInfo.OutputInfo.OpticalBlackPixels.Width;
            Layout.OBArea.Height = VinInfo.OutputInfo.OpticalBlackPixels.Height;
            Layout.OBArea.X = VinInfo.OutputInfo.OpticalBlackPixels.StartX - ActStartX;
            Layout.OBArea.Y = VinInfo.OutputInfo.OpticalBlackPixels.StartY - ActStartY;
            Layout.EnableOBArea = 1;
        } else {
            VinCfg.HwCaptureWindow.Width = StillEncMgt[encID].CaptureWidth;
            VinCfg.HwCaptureWindow.Height = StillEncMgt[encID].CaptureHeight;
            VinCfg.HwCaptureWindow.X = VinInfo.OutputInfo.RecordingPixels.StartX + (((VinInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
            VinCfg.HwCaptureWindow.Y = VinInfo.OutputInfo.RecordingPixels.StartY + (((VinInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)&0xFFFE);
            Layout.ActiveArea.Width = StillEncMgt[encID].CaptureWidth;
            Layout.ActiveArea.Height = StillEncMgt[encID].CaptureHeight;
            Layout.ActiveArea.X = VinCfg.HwCaptureWindow.X;
            Layout.ActiveArea.Y = VinCfg.HwCaptureWindow.Y;
            Layout.EnableOBArea = 0;
        }

        Layout.Width = StillEncMgt[encID].MainWidth;
        Layout.Height = StillEncMgt[encID].MainHeight;
        Layout.EnableSourceArea = 0; // Get all capture window to main
        Layout.DzoomFactorX = INIT_DZOOM_FACTOR;
        Layout.DzoomFactorY = INIT_DZOOM_FACTOR;
        Layout.DzoomOffsetX = 0;
        Layout.DzoomOffsetY = 0;
        VinCfg.Layout = &Layout;
        VinCfg.cbEvent = AmpUT_StillEncVinEventCallback;
        VinCfg.cbSwitch = AmpUT_StillEncVinSwitchCallback;
        AmbaPrint("Vin creat %d %d %d", VinCfg.Mode, Layout.Width, Layout.Height);
        AmpVin_Create(&VinCfg, &StillEncVinA);
    }

    // Create ImgSchdlr instance
    {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg = {0};
        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);
        ImgSchdlrCfg.MainViewID = 0; //single channel have one MainView
        ImgSchdlrCfg.Channel = EncChannel;
        ImgSchdlrCfg.Vin = StillEncVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_StillEncImgSchdlrCallback;
        if (StillLiveViewProcMode && StillLiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &SencImgSchdlr);  // One sensor (not vin) need one scheduler.
    }

    // Create Still encoder object
    if (StillEncPri == NULL) {
        AMP_STILLENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s EncLayer = {0};
    #if 0 //TBD, MultiChan
        {
            AMP_VIDEOENC_LAYER_DESC_s MultChanEncLayer[4];
            memset(multChanEncLayer, 0x0, sizeof(AMP_VIDEOENC_LAYER_DESC_s)*4);
            EncCfg.MainLayout.Layer = MultChanEncLayer;
        }
    #else
        EncCfg.MainLayout.Layer = &EncLayer;
    #endif
        AmpStillEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_StillEncCallback;

        // Assign main layout
        EncCfg.MainLayout.Width = StillEncMgt[encID].MainWidth;
        EncCfg.MainLayout.Height = StillEncMgt[encID].MainHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = StillEncMgt[encID].Interlace;
        EncCfg.MainTimeScale = StillEncMgt[encID].TimeScale;
        EncCfg.MainTickPerPicture = StillEncMgt[encID].TickPerPicture;
        {
            UINT8 *DspWorkAddr;
            UINT32 DspWorkSize;
            STILL_CAPTURE_MODE_e CurrCapMode = CaptureMode;
            CaptureMode = STILL_CAPTURE_NONE;
            AmpUT_StillEnc_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
            CaptureMode = CurrCapMode;
            EncCfg.DspWorkBufAddr = DspWorkAddr;
            EncCfg.DspWorkBufSize = DspWorkSize;
        }

        if (StillLiveViewProcMode == 0) {
            if (StillEncMgt[encID].MainWidth > 1920 || StillEncMgt[encID].CaptureWidth > 1920)
                StillLiveViewOSMode = 1;
        }

        EncCfg.LiveViewProcMode = StillLiveViewProcMode;
        EncCfg.LiveViewAlgoMode = StillLiveViewAlgoMode;
        EncCfg.LiveViewOSMode = StillLiveViewOSMode;

        // Assign bitstream/descriptor buffer
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&BitsBuf, (void **)&TmpbufRaw, STILL_BISFIFO_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }
        EncCfg.BitsBufCfg.BitsBufAddr = BitsBuf;
        EncCfg.BitsBufCfg.BitsBufSize = STILL_BISFIFO_SIZE;

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&DescBuf, (void **)&TmpbufRaw, STILL_DESC_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for DescFifo!!");
        }
        DescBuf = (UINT8 *)ALIGN_32((UINT32)DescBuf);
        EncCfg.BitsBufCfg.DescBufAddr = DescBuf;
        EncCfg.BitsBufCfg.DescBufSize = STILL_DESC_SIZE;
        EncCfg.BitsBufCfg.BitsRunoutThreshold = STILL_BISFIFO_SIZE - 4*1024*1024; // leave 4MB
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", BitsBuf, STILL_BISFIFO_SIZE, DescBuf, STILL_DESC_SIZE);

        EncLayer.SourceType = AMP_ENC_SOURCE_VIN;
        EncLayer.Source = StillEncVinA;
        EncLayer.SourceLayoutId = 0;
        EncLayer.EnableSourceArea = 0;  // No source cropping
        EncLayer.EnableTargetArea = 0;  // No target pip
    #ifdef CONFIG_SOC_A9
        EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    #else
        EncCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
        EncCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
        EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
        EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
        EncCfg.LiveviewOBModeEnable = StillOBModeEnable;
    #endif
        AmbaPrint("Enc creat %d %d %d %d", EncCfg.MainLayout.Width, EncCfg.MainLayout.Height, \
            EncCfg.MainTimeScale, EncCfg.MainTickPerPicture);
        AmpStillEnc_Create(&EncCfg, &StillEncPri);

        // create a virtual fifo
        if (StillEncVirtualFifoHdlr == NULL) {
            AMP_FIFO_CFG_s FifoDefCfg = {0};
            AmpFifo_GetDefaultCfg(&FifoDefCfg);
            FifoDefCfg.hCodec = StillEncPri;
            FifoDefCfg.IsVirtual = 1;
            FifoDefCfg.NumEntries = 1024;
            FifoDefCfg.cbEvent = AmpUT_StillEnc_FifoCB;
            AmpFifo_Create(&FifoDefCfg, &StillEncVirtualFifoHdlr);
        }
    } else {
        //May have to update VIN
        AMP_VIN_RUNTIME_CFG_s VinCfg = {0};
        AMP_STILLENC_MAIN_CFG_s MainCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s Newlayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
        AMBA_SENSOR_MODE_INFO_s VinInfo = {0};
        AMP_VIN_LAYOUT_CFG_s Layout = {0};
        AMBA_SENSOR_MODE_ID_u SensorMode = {0};

        SensorMode.Data = StillEncMgt[encID].InputMode;
        AmbaSensor_GetModeInfo(EncChannel, SensorMode, &VinInfo);

        VinCfg.Hdlr = StillEncVinA;
        VinCfg.Mode = SensorMode;
    #ifdef CONFIG_SOC_A9
        VinCfg.Mode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
    #endif
        VinCfg.LayoutNumber = 1;
        VinCfg.HwCaptureWindow.Width = StillEncMgt[encID].CaptureWidth;
        VinCfg.HwCaptureWindow.Height = StillEncMgt[encID].CaptureHeight;
        VinCfg.HwCaptureWindow.X = VinInfo.OutputInfo.RecordingPixels.StartX + (((VinInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        VinCfg.HwCaptureWindow.Y = VinInfo.OutputInfo.RecordingPixels.StartY + (((VinInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)&0xFFFE);
        Layout.Width = StillEncMgt[encID].MainWidth;
        Layout.Height = StillEncMgt[encID].MainHeight;
        Layout.EnableSourceArea = 0; // Get all capture window to main
        Layout.DzoomFactorX = INIT_DZOOM_FACTOR;
        Layout.DzoomFactorY = INIT_DZOOM_FACTOR;
        Layout.DzoomOffsetX = 0;
        Layout.DzoomOffsetY = 0;
        VinCfg.Layout = &Layout;

        MainCfg.Hdlr = StillEncPri;
        MainCfg.MainLayout.LayerNumber = 1;
        MainCfg.MainLayout.Layer = &Newlayer;
        MainCfg.MainLayout.Width = StillEncMgt[encID].MainWidth;
        MainCfg.MainLayout.Height = StillEncMgt[encID].MainHeight;
        MainCfg.Interlace = StillEncMgt[encID].Interlace;
        MainCfg.MainTickPerPicture = StillEncMgt[encID].TickPerPicture;
        MainCfg.MainTimeScale = StillEncMgt[encID].TimeScale;
    #ifdef CONFIG_SOC_A9
        MainCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        MainCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        MainCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    #else
        MainCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
        MainCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
        MainCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
        MainCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
        MainCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
        MainCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
    #endif
        {
            UINT8 *DspWorkAddr;
            UINT32 DspWorkSize;
            STILL_CAPTURE_MODE_e currCapMode = CaptureMode;
            CaptureMode = STILL_CAPTURE_NONE;
            AmpUT_StillEnc_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
            CaptureMode = currCapMode;
            MainCfg.DspWorkBufAddr = DspWorkAddr;
            MainCfg.DspWorkBufSize = DspWorkSize;
        }
        // Config LiveView Ctrl Flag
        if (StillLiveViewProcMode == 0) {
            if (StillEncMgt[encID].MainWidth > 1920 || \
                StillEncMgt[encID].CaptureWidth > 1920)
                StillLiveViewOSMode = 1;
        }

        MainCfg.LiveViewProcMode = StillLiveViewProcMode;
        MainCfg.LiveViewAlgoMode = StillLiveViewAlgoMode;
        MainCfg.LiveViewOSMode = StillLiveViewOSMode;

        Newlayer.EnableSourceArea = 0;
        Newlayer.EnableTargetArea = 0;
        Newlayer.LayerId = 0;
        Newlayer.SourceType = AMP_ENC_SOURCE_VIN;
        Newlayer.Source = StillEncVinA;
        Newlayer.SourceLayoutId = 0;
        AmpStillEnc_UpdateVinMain(1, &VinCfg, &MainCfg);
    }

    // Init Display
    AmpUT_StillEnc_DisplayStart();

    // Register pipeline
    if (StillEncPipe == NULL) {
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};
        // Register pipeline
        AmpEnc_GetDefaultCfg(&PipeCfg);
        PipeCfg.encoder[0] = StillEncPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_StillEncPipeCallback;
        PipeCfg.type = AMP_ENC_STILL_PIPE;
        AmpEnc_Create(&PipeCfg, &StillEncPipe);
        AmpEnc_Add(StillEncPipe);
    }

    // Init Dzoom
    {
        extern int Dzoom_Step_Factor_Table(int Step, UINT32 *Factor);
        AMP_IMG_DZOOM_CFG_s DzoomCfg = {0};
        AMP_IMG_DZOOM_VIN_INVALID_INFO_s DzoomVinInvalidInfo = {0};
        AmpImgDzoom_GetDefaultCfg(&DzoomCfg);
        DzoomCfg.ImgModeBatchId = 0;
        DzoomCfg.ImgModeContextId = 0;
        AmpImgDzoom_Create(&DzoomCfg, &StillDzoomHdlr);
        StillDzoomTable.TotalStepNumber = STILL_DZOOM_10X_TOTAL_STEP;
        StillDzoomTable.MaxDzoomFactor = STILL_DZOOM_10X_MAX_RATIO;
        StillDzoomTable.GetDzoomFactor = Dzoom_Step_Factor_Table;
        AmpImgDzoom_RegDzoomTable(StillDzoomHdlr, &StillDzoomTable);
        DzoomVinInvalidInfo.CapW = StillEncMgt[StillEncModeIdx].CaptureWidth;
        DzoomVinInvalidInfo.CapH = StillEncMgt[StillEncModeIdx].CaptureHeight;
        AmpImgDzoom_ChangeResolutionHandler(StillDzoomHdlr, &DzoomVinInvalidInfo);
        AmpImgDzoom_ResetStatus(StillDzoomHdlr);
    }
    AmpEnc_StartLiveview(StillEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
    Status = STATUS_STILL_LIVEVIEW;
    return 0;
}

int AmpUT_StillEnc_ChangeResolution(UINT32 encID)
{
    AMP_VIN_RUNTIME_CFG_s VinCfg = {0};
    AMP_STILLENC_MAIN_CFG_s MainCfg = {0};
    AMP_VIDEOENC_LAYER_DESC_s Newlayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
    AMBA_SENSOR_MODE_INFO_s VinInfo = {0};
    AMP_VIN_LAYOUT_CFG_s Layout = {0};
    AMBA_SENSOR_MODE_ID_u SensorMode = {0};

    StillEncModeIdx = encID;

    AmbaPrint(" =========================================================");
    AmbaPrint(" AmbaUnitTest: Change Resolution to %s", MWUT_GetInputStillModeName(encID, StillEncodeSystem));
    AmbaPrint(" =========================================================");

    SensorMode.Data = StillEncMgt[encID].InputMode;
    AmbaSensor_GetModeInfo(EncChannel, SensorMode, &VinInfo);

    VinCfg.Hdlr = StillEncVinA;
    VinCfg.Mode = SensorMode;
#ifdef CONFIG_SOC_A9
    VinCfg.Mode.Bits.VerticalFlip = (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP) ? 1 : 0;
#endif
    VinCfg.LayoutNumber = 1;
    if (StillOBModeEnable && VinInfo.OutputInfo.OpticalBlackPixels.Width && VinInfo.OutputInfo.OpticalBlackPixels.Height) {
        INT16 ActStartX = MIN2(VinInfo.OutputInfo.OpticalBlackPixels.StartX, VinInfo.OutputInfo.RecordingPixels.StartX);
        INT16 ActStartY = MIN2(VinInfo.OutputInfo.OpticalBlackPixels.StartY, VinInfo.OutputInfo.RecordingPixels.StartY);

        VinCfg.HwCaptureWindow.Width = VinInfo.OutputInfo.OutputWidth - ActStartX;
        VinCfg.HwCaptureWindow.Height = VinInfo.OutputInfo.OutputHeight - ActStartY;
        VinCfg.HwCaptureWindow.X = ActStartX;
        VinCfg.HwCaptureWindow.Y = ActStartY;
        Layout.ActiveArea.Width = StillEncMgt[encID].CaptureWidth;
        Layout.ActiveArea.Height = StillEncMgt[encID].CaptureHeight;
        Layout.ActiveArea.X = (VinInfo.OutputInfo.RecordingPixels.StartX - ActStartX) + \
            (((VinInfo.OutputInfo.RecordingPixels.Width - Layout.ActiveArea.Width)/2)&0xFFF8);
        Layout.ActiveArea.Y = (VinInfo.OutputInfo.RecordingPixels.StartY - ActStartY) + \
            (((VinInfo.OutputInfo.RecordingPixels.Height - Layout.ActiveArea.Height)/2)&0xFFFE);
        Layout.OBArea.Width = VinInfo.OutputInfo.OpticalBlackPixels.Width;
        Layout.OBArea.Height = VinInfo.OutputInfo.OpticalBlackPixels.Height;
        Layout.OBArea.X = VinInfo.OutputInfo.OpticalBlackPixels.StartX - ActStartX;
        Layout.OBArea.Y = VinInfo.OutputInfo.OpticalBlackPixels.StartY - ActStartY;
        Layout.EnableOBArea = 1;
    } else {
        VinCfg.HwCaptureWindow.Width = StillEncMgt[encID].CaptureWidth;
        VinCfg.HwCaptureWindow.Height = StillEncMgt[encID].CaptureHeight;
        VinCfg.HwCaptureWindow.X = VinInfo.OutputInfo.RecordingPixels.StartX + (((VinInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        VinCfg.HwCaptureWindow.Y = VinInfo.OutputInfo.RecordingPixels.StartY + (((VinInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)&0xFFFE);
        Layout.ActiveArea.Width = StillEncMgt[encID].CaptureWidth;
        Layout.ActiveArea.Height = StillEncMgt[encID].CaptureHeight;
        Layout.ActiveArea.X = VinCfg.HwCaptureWindow.X;
        Layout.ActiveArea.Y = VinCfg.HwCaptureWindow.Y;
        Layout.EnableOBArea = 0;
    }
    Layout.Width = StillEncMgt[encID].MainWidth;
    Layout.Height = StillEncMgt[encID].MainHeight;
    Layout.EnableSourceArea = 0; // Get all capture window to main
    Layout.DzoomFactorX = INIT_DZOOM_FACTOR;
    Layout.DzoomFactorY = INIT_DZOOM_FACTOR;
    Layout.DzoomOffsetX = 0;
    Layout.DzoomOffsetY = 0;
    VinCfg.Layout = &Layout;

    MainCfg.Hdlr = StillEncPri;
    MainCfg.MainLayout.LayerNumber = 1;
    MainCfg.MainLayout.Layer = &Newlayer;
    MainCfg.MainLayout.Width = StillEncMgt[encID].MainWidth;
    MainCfg.MainLayout.Height = StillEncMgt[encID].MainHeight;
    MainCfg.Interlace = StillEncMgt[encID].Interlace;
    MainCfg.MainTickPerPicture = StillEncMgt[encID].TickPerPicture;
    MainCfg.MainTimeScale = StillEncMgt[encID].TimeScale;
    {
        UINT8 *DspWorkAddr;
        UINT32 DspWorkSize;
        STILL_CAPTURE_MODE_e CurrCapMode = CaptureMode;
        CaptureMode = STILL_CAPTURE_NONE;
        AmpUT_StillEnc_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
        CaptureMode = CurrCapMode;
        MainCfg.DspWorkBufAddr = DspWorkAddr;
        MainCfg.DspWorkBufSize = DspWorkSize;
    }

    if (StillLiveViewProcMode == 0) {
        if (StillEncMgt[encID].MainWidth > 1920 || \
            StillEncMgt[encID].CaptureWidth > 1920)
            StillLiveViewOSMode = 1;
    }

    MainCfg.LiveViewProcMode = StillLiveViewProcMode;
    MainCfg.LiveViewAlgoMode = StillLiveViewAlgoMode;
    MainCfg.LiveViewOSMode = StillLiveViewOSMode;

#ifdef CONFIG_SOC_A9
    MainCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
#else
    MainCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
    MainCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
    MainCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
    MainCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
    MainCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
#endif
    Newlayer.EnableSourceArea = 0;
    Newlayer.EnableTargetArea = 0;
    Newlayer.LayerId = 0;
    Newlayer.SourceType = AMP_ENC_SOURCE_VIN;
    Newlayer.Source = StillEncVinA;
    Newlayer.SourceLayoutId = 0;

    // Update capture window for Dzoom
    {
        AMP_IMG_DZOOM_VIN_INVALID_INFO_s DzoomVinInvalidInfo = {0};
        DzoomVinInvalidInfo.CapW = StillEncMgt[StillEncModeIdx].CaptureWidth;
        DzoomVinInvalidInfo.CapH = StillEncMgt[StillEncModeIdx].CaptureHeight;

        AmpImgDzoom_StopDzoom(StillDzoomHdlr);
        AmpImgDzoom_ResetStatus(StillDzoomHdlr);
        AmpImgDzoom_ChangeResolutionHandler(StillDzoomHdlr, &DzoomVinInvalidInfo);
    }

    AmpStillEnc_ConfigVinMain(1, &VinCfg, &MainCfg);
    return 0;
}

int AmpUT_StillEncTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_StillEncTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        int LcdId;
        if (argc < 3) {
            UINT8 i = 0;
            AmbaPrint("Usage: t stillenc init [sensorID][LcdID]");
            AmbaPrint("               sensorID:");
            for (i = 0; i<INPUT_DEVICE_NUM; i++) {
                AmbaPrint("                        %2d -- %s", i, MWUT_GetInputDeviceName(i));
            }
            AmbaPrint("               LcdID: 0 -- WDF9648W");
            return -1;
        } else if (argc == 3){
            StillSensorIdx = atoi(argv[2]);
            LcdId = 0;
        } else {
            StillSensorIdx = atoi(argv[2]);
            LcdId = atoi(argv[3]);
        }
        AmpUT_StillEnc_Init(StillSensorIdx, LcdId);
        Status = STATUS_STILL_INIT;
        AmbaPrintColor(BLUE,"[AMP_UT] Init done %d", StillSensorIdx);
    } else if (strcmp(argv[1],"encID") == 0) {
        if (NULL == StillEncMgt) {
            AmbaPrint("Please do init first");
        } else {
            AmbaPrint("[%s] mode_id: ", MWUT_GetInputDeviceName(StillSensorIdx));
            MWUT_InputStillModePrintOutAll(StillEncMgt);
        }
    } else if (strcmp(argv[1],"capMode") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc capmode [mode]");
            AmbaPrint("             mode: capture mode");
            AmbaPrint("              0x0: LiveView only/R2R/rawcap/r2y/y2j/");
            AmbaPrint("              0x1: single raw capture");
            return -1;
        } else {
            CaptureMode = (STILL_CAPTURE_MODE_e)atoi(argv[2]);
        }
    } else if (strcmp(argv[1],"capprof") == 0 || strcmp(argv[1],"captureprofiling") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc captureprofiling [en]");
            AmbaPrint("                0: disable");
            AmbaPrint("                1: enable");
            return -1;
        } else {
            StillIsCapProfEnable = atoi(argv[2]);
            AmbaPrint(" Capture profile is %s", (StillIsCapProfEnable)?"enable":"disble");
        }
    } else if (strcmp(argv[1],"stamp") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc stamp [en]");
            AmbaPrint("                   en: enable or disable stamp for single capture");
            AmbaPrint("                       0 : disable");
            AmbaPrint("                       1 : enable");
        } else {
            IsStillStampEnable = atoi(argv[2]);
            if (IsStillStampEnable == 1) {
                UINT16 i, j;
                memset(StillStampMainY, 0xFF, sizeof(StillStampMainY));
                memset(StillStampMainUV, 0xFF, sizeof(StillStampMainUV));
                memset(StillStampScrnY, 0xFF, sizeof(StillStampScrnY));
                memset(StillStampScrnUV, 0xFF, sizeof(StillStampScrnUV));
                memset(StillStampThmY, 0xFF, sizeof(StillStampThmY));
                memset(StillStampThmUV, 0xFF, sizeof(StillStampThmUV));
                // Draw some line for testing
                for (i=0; i<20; i++) {
                    for (j=0; j<DEFAULT_STAMP_MAIN_WIDTH; j++) {
                        StillStampMainY[DEFAULT_STAMP_MAIN_WIDTH*(i+(DEFAULT_STAMP_MAIN_HEIGHT>>1)) + j] = 0x0;
                        StillStampMainUV[DEFAULT_STAMP_MAIN_WIDTH*(i+(DEFAULT_STAMP_MAIN_HEIGHT>>1)) + j] = 0x0;
                    }
                    for (j=0; j<DEFAULT_STAMP_SCRN_WIDTH; j++) {
                        StillStampScrnY[DEFAULT_STAMP_SCRN_WIDTH*(i+(DEFAULT_STAMP_SCRN_HEIGHT>>1)) + j] = 0x0;
                        StillStampScrnUV[DEFAULT_STAMP_SCRN_WIDTH*(i+(DEFAULT_STAMP_SCRN_HEIGHT>>1)) + j] = 0x0;
                    }
                    for (j=0; j<DEFAULT_STAMP_THM_WIDTH; j++) {
                        StillStampThmY[DEFAULT_STAMP_THM_WIDTH*(i+(DEFAULT_STAMP_THM_HEIGHT>>1)) + j] = 0x0;
                        StillStampThmUV[DEFAULT_STAMP_THM_WIDTH*(i+(DEFAULT_STAMP_THM_HEIGHT>>1)) + j] = 0x0;
                    }
                }

            }
            AmbaPrint("Still stamp %s", (IsStillStampEnable)?"Enable":"Disable");
        }
    } else if (strcmp(argv[1],"customyuv") == 0) {
        if (argc < 4) {
            AmbaPrint("Usage: t stillenc customyuv [width][height]");
            AmbaPrint("              width: width for yuv");
            AmbaPrint("             heigth: height for yuv");
        } else {
            StillCustomMainWidth = atoi(argv[2]);
            StillCustomMainHeight = atoi(argv[3]);
            AmbaPrint("[AMP_UT] Custom yuv width %u height %u", StillCustomMainWidth, StillCustomMainHeight);
        }
    } else if (strcmp(argv[1],"dzoomjump") == 0 || strcmp(argv[1],"dzjump") == 0) {
        if (argc >= 5) {
            AMP_IMG_DZOOM_JUMP_s DzoomJump;

            UINT32 DzoomJumpStep = atoi(argv[2]);
            double DzoomJumpShiftX = atof(argv[3]);
            double DzoomJumpShiftY = atof(argv[4]);

            DzoomJump.Step = DzoomJumpStep;
            DzoomJump.ShiftX = (int) (DzoomJumpShiftX * 65536);
            DzoomJump.ShiftY = (int) (DzoomJumpShiftY * 65536);

            if (0 != AmpImgDzoom_PresetDzoomJump(StillDzoomHdlr, &DzoomJump)) {
                //FIXME: %f issue
                //AmbaPrint("Preset Dzoom jump error, step %u shift x %f shift y %f", DzoomJumpStep, DzoomJumpShiftX, DzoomJumpShiftY);
            } else {
                if (0 != AmpImgDzoom_SetDzoomJump(StillDzoomHdlr, &DzoomJump)) {
                    AmbaPrint("Dzoom Jump error");
                }
            }
        } else {
            AmbaPrint("Usage: t stillenc dzoomjump [Step][ShiftX][ShiftY]");
            AmbaPrint("               Step: dzoom step");
            AmbaPrint("             ShiftX: x-axis shift");
            AmbaPrint("             ShiftY: Y-axis shift");
        }
    } else if (strcmp(argv[1],"dzoommove") == 0 || strcmp(argv[1],"dzmove") == 0) {
        if (argc >= 7) {
            AMP_IMG_DZOOM_POSITION_s DzoomPosition;
            UINT32 DzoomDirection = atoi(argv[2]);
            UINT32 DzoomSpeed = atoi(argv[3]);
            UINT32 DzoomDistance = atoi(argv[4]);
            double DzoomShiftX = atof(argv[5]);
            double DzoomShiftY = atof(argv[6]);

            DzoomPosition.Direction = DzoomDirection;
            DzoomPosition.Speed = DzoomSpeed;
            DzoomPosition.Distance = DzoomDistance;
            DzoomPosition.ShiftX = (int)(DzoomShiftX*65536);
            DzoomPosition.ShiftY = (int)(DzoomShiftY*65536);
            if (0 != AmpImgDzoom_SetDzoomPosition(StillDzoomHdlr, &DzoomPosition)) {
                /*FIXME: %f issue
                AmbaPrint("Dzoom move error, direction %u speed %u distance %u shiftX %f shiftY", DzoomDirection, \
                    DzoomSpeed, DzoomDistance, DzoomShiftX, DzoomShiftY);*/
            }
        } else {
            AmbaPrint("Usage: t stillenc dzoommove [Direction][Speed][Distance][ShiftX][ShiftY]");
            AmbaPrint("              Direction: dzoom direction");
            AmbaPrint("                  Speed: dzoom speed");
            AmbaPrint("               Distance: dzoom distance");
            AmbaPrint("                 ShiftX: x-axis shift");
            AmbaPrint("                 ShiftY: Y-axis shift");
        }
    } else if (strcmp(argv[1],"dzoomsatus") == 0 || strcmp(argv[1],"dzstatus") == 0) {
        AMP_IMG_DZOOM_STATUS_s DzoomStatus;
        AmpImgDzoom_GetDzoomStatus(StillDzoomHdlr, &DzoomStatus);
        AmbaPrint("Dzoom status: Region %u InProc %u Step %u Factor 0x%X ShiftX %d ShiftY %d",  DzoomStatus.Region, \
            DzoomStatus.IsProcessing, DzoomStatus.Step, DzoomStatus.Factor, DzoomStatus.ShiftX, DzoomStatus.ShiftY);
    } else if (strcmp(argv[1],"dzoomstop") == 0 || strcmp(argv[1],"dzstop") == 0) {
        if (0 != AmpImgDzoom_StopDzoom(StillDzoomHdlr)) {
            AmbaPrint("Dzoom stop error");
        }
    } else if (strcmp(argv[1],"lvproc") == 0 || strcmp(argv[1],"liveviewproc") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc liveviewproc [proc][Algo/OS]");
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
            StillLiveViewProcMode = Proc;
            if (Proc == 0) {
                StillLiveViewOSMode = Aux;
                StillLiveViewAlgoMode = 0;
            } else {
                StillLiveViewAlgoMode = Aux;
                StillLiveViewOSMode = 0;
            }
            if (StillLiveViewProcMode == 0) {
                AmbaPrint("Set LvProc = %s_%s", "Express", Aux?"OS":"Basic");
            } else {
                AmbaPrint("Set LvProc = %s_%s", "Hybrid",
                    (Aux==0)? "Fast": ((Aux==1)? "LISO": "HISO"));
            }
        }
    } else if (strcmp(argv[1],"lvst") == 0 || strcmp(argv[1],"liveviewstart") == 0) {
        if (Status != STATUS_STILL_INIT)
            return -1;

        if (argc < 3) {
            AmbaPrint("Usage: t stillenc liveviewstart [encID]");
            if (StillEncMgt == NULL) {
                AmbaPrintColor(RED, "Please hook sensor first!!!");
            } else {
                AmbaPrint("[%s] mode_id: ", MWUT_GetInputDeviceName(StillSensorIdx));
                MWUT_InputStillModePrintOutAll(StillEncMgt);
            }
            return -1;
        } else {
            UINT32 EncID = atoi(argv[2]);
            AmpUT_StillEnc_LiveviewStart(EncID);

            if (StillEncMgt[EncID].ForbidMode) {
                AmbaPrintColor(RED, "Not Support this Mode (%u)", EncID);
                return -1;
            }
            Status = STATUS_STILL_LIVEVIEW;
            AmbaPrintColor(BLUE,"[AMP_UT] LVST %d", EncID);
        }
    } else if (strcmp(argv[1],"lvsp") == 0 || strcmp(argv[1],"liveviewstop") == 0) {
        if (Status != STATUS_STILL_LIVEVIEW)
            return -1;
        AmpUT_StillEnc_LiveviewStop();
        Status = STATUS_STILL_INIT;
        AmbaPrintColor(BLUE,"[AMP_UT] LVSP done");
    } else if (strcmp(argv[1],"chg") == 0 || strcmp(argv[1],"modechange") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc modechange [encID]");
            AmbaPrint("             encID: encode ID");
            return -1;
        } else {
            UINT32 NewEncID = atoi(argv[2]);
            AmpUT_StillEnc_ChangeResolution(NewEncID);
            AmbaPrintColor(BLUE,"[AMP_UT] Mode change done %d", NewEncID);
        }
    } else if (strcmp(argv[1],"rawcap") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc rawcap [encID][iso][cmpr]");
            AmbaPrint("             encID: encode ID");
        #ifdef CONFIG_SOC_A9
            AmbaPrint("             iso: algo mode, 0:HISO 1:LISO 2:MFHISO(hack as FastMode)");
        #else
            AmbaPrint("             iso: algo mode, 0:HISO 1:LISO 2:MFHISO(hack as FastMode) 3:MISO");
        #endif
            AmbaPrint("             cmpr: compressed raw");
            return -1;
        } else {
            UINT8 EncID = atoi(argv[2]);
            UINT8 Cmpr = atoi(argv[4]);
            G_iso = atoi(argv[3]);
        #ifdef CONFIG_SOC_A12
            if (G_iso == 3) {
                G_iso = 1;
                Force3PassLiso = 1;
            } else if (G_iso == 1) {
                Force3PassLiso = 0;
            }
        #endif
            CaptureMode = STILL_CAPTURE_NONE;
            AmpUT_StillEncAAALock();
            AmpUT_rawcap(EncID, G_iso, Cmpr);
        }
    } else if (strcmp(argv[1],"yuvconv") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc yuvconv [inputID][encID][iso][cmpr][bits][bayer]");
            AmbaPrint("             inputID: input raw ID");
            AmbaPrint("             encID: encode ID");
#ifdef CONFIG_SOC_A9
            AmbaPrint("             iso: algo mode, 0:HISO 1:LISO 2:MFHISO(hack as FastMode)");
#else
            AmbaPrint("             iso: algo mode, 0:HISO 1:LISO 2:MFHISO(hack as FastMode) 3:MISO");
#endif
            AmbaPrint("             cmpr: compressed raw");
            AmbaPrint("             bits: raw data bits");
            AmbaPrint("             bayer: raw bayer pattern, RG(0) BG(1) GR(2) GB(3)");
            return -1;
        } else {
            UINT32 InputID = atoi(argv[2]);
            UINT8 EncID = atoi(argv[3]);
            UINT8 Cmpr = atoi(argv[5]);
            UINT8 Bits = atoi(argv[6]);
            UINT8 Bayer = atoi(argv[7]);

            G_iso = atoi(argv[4]);
        #ifdef CONFIG_SOC_A12
            if (G_iso == 3) {
                G_iso = 1;
                Force3PassLiso = 1;
            } else if (G_iso == 1) {
                Force3PassLiso = 0;
            }
        #endif
        #ifdef CONFIG_SOC_A9
            if (G_iso == 2) {
                AmbaPrint("Not support FastMode PIV Raw2yuv !!!");
                return -1;
            }
        #endif
            CaptureMode = STILL_CAPTURE_RAW2YUV;
            AmpUT_pivRaw2Yuv(InputID, EncID, G_iso, Cmpr, Bits, Bayer);
        }
    } else if (strcmp(argv[1],"jpgenc") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc jpgenc [inputID][encID][iso][tsize][loop]");
            AmbaPrint("             inputID: input yuv ID");
            AmbaPrint("             encID: encode ID");
#ifdef CONFIG_SOC_A9
            AmbaPrint("             iso: algo mode, 0:HISO 1:LISO 2:MFHISO(hack as FastMode)");
#else
            AmbaPrint("             iso: algo mode, 0:HISO 1:LISO 2:MFHISO(hack as FastMode) 3:MISO");
#endif
            AmbaPrint("             tsize: target Size in Kbyte unit");
            AmbaPrint("                   0 : default w*h/2000 without BRC control");
            AmbaPrint("             loop: Jpeg BRC re-encode loop");
            AmbaPrint("                   0~0xFE : re-encode number + 1");
            AmbaPrint("                   0xFF : for burst capture");
            return -1;
        } else {
            UINT32 InputID = atoi(argv[2]);
            UINT8 EncID = atoi(argv[3]);
            UINT32 TargetSize = atoi(argv[5]);
            UINT8 EncodeLoop = atoi(argv[6]);
            G_iso = atoi(argv[4]);
        #ifdef CONFIG_SOC_A12
            if (G_iso == 3) {
                G_iso = 1;
                Force3PassLiso = 1;
            } else if (G_iso == 1) {
                Force3PassLiso = 0;
            }
        #endif
            CaptureMode = STILL_CAPTURE_YUV2JPG;
            AmpUT_pivYuv2Jpeg(InputID, EncID, G_iso, TargetSize, EncodeLoop);
        }
    } else if (strcmp(argv[1],"singlecapture") == 0 || strcmp(argv[1],"scap") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc singlecapture [encID][iso][cmpr][tsize][loop]");
            AmbaPrint("             encID: encode ID");
#ifdef CONFIG_SOC_A9
            AmbaPrint("             iso: algo mode, 0:HISO 1:LISO 2:MFHISO(hack as FastMode)");
#else
            AmbaPrint("             iso: algo mode, 0:HISO 1:LISO 2:MFHISO(hack as FastMode) 3:MISO");
#endif
            AmbaPrint("             cmpr: compressed raw");
            AmbaPrint("             tsize: target Size in Kbyte unit");
            AmbaPrint("                   0 : default w*h/2000 without BRC control");
            AmbaPrint("             loop: Jpeg BRC re-encode loop");
            AmbaPrint("                   0~0xFE : re-encode number + 1");
            AmbaPrint("                   0xFF : for burst capture");
            return -1;
        } else {
            UINT8 EncID = atoi(argv[2]);
            UINT8 Cmpr = atoi(argv[4]);
            UINT32 TargetSize = atoi(argv[5]);
            UINT8 EncodeLoop = atoi(argv[6]);
            G_iso = atoi(argv[3]);
        #ifdef CONFIG_SOC_A12
            if (G_iso == 3) {
                G_iso = 1;
                Force3PassLiso = 1;
            } else if (G_iso == 1) {
                Force3PassLiso = 0;
            }
        #endif
            G_capcnt = 1;
            CaptureMode = STILL_CAPTURE_SINGLE_SHOT;
            AmpUT_StillEncAAALock();
            AmpUT_SingleCapture(EncID, G_iso, Cmpr, TargetSize, EncodeLoop);
        }
    } else if (strcmp(argv[1],"burstcapture") == 0 || strcmp(argv[1],"bcap") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc burstcapture [encID][capcnt]");
            AmbaPrint("             encID: encode ID");
            AmbaPrint("             capcnt: target capture number");
            return -1;
       } else {
            UINT8 EncID = atoi(argv[2]);
            UINT8 Cmpr = DEFAULT_STILL_RAW_COMPRESSED_MODE;
            UINT32 TargetSize = DEFAULT_STILL_JPEG_TARGET_SIZE;
            UINT8 EncodeLoop = 0xFF;
            G_iso = DEFAULT_STILL_ISO_MODE;
            G_capcnt = atoi(argv[3]);
            CaptureMode = STILL_CAPTURE_BURST_SHOT;
            AmpUT_StillEncAAALock();
            AmpUT_burstCapture(EncID, G_iso, Cmpr, TargetSize, EncodeLoop, G_capcnt);
        }
    } else if (strcmp(argv[1],"burstcapturecont") == 0 ||strcmp(argv[1],"bcapc") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc burstcapturecont [encID][duration][wb]");
            AmbaPrint("             encID: encode ID");
            AmbaPrint("             duration: simulate how long shutter key is pressed(unit in ms)");
            AmbaPrint("             wb: enbale postWB calculation or not");
        } else {
            UINT8 EncID = atoi(argv[2]);
            UINT32 Duration = atoi(argv[3]);
            UINT8 EnableWB = atoi(argv[4]);
            UINT8 Cmpr = DEFAULT_STILL_RAW_COMPRESSED_MODE;
            UINT8 EncodeLoop = 0xFF;
            UINT32 TargetSize = DEFAULT_STILL_JPEG_TARGET_SIZE;
            UINT32 ScriptID = 0;
            G_iso = DEFAULT_STILL_ISO_MODE;
            CaptureButtonPressed = 1;
            AmpUT_StillEncAAALock();
            if (EnableWB == 0) {
                CaptureMode = STILL_CAPTURE_BURST_SHOT_CONT;
                ScriptID = AmpUT_burstCaptureCont(EncID, G_iso, Cmpr, TargetSize, EncodeLoop);
            } else if (EnableWB == 1) {
                CaptureMode = STILL_CAPTURE_BURST_SHOT_CONT_WB;
                ScriptID = AmpUT_burstCaptureContWB(EncID, G_iso, Cmpr, TargetSize, EncodeLoop);
            }

            /* Timer is use for simulate button pressed time */
            if (CaptureButtonTimerHdlrID < 0) {
                CaptureButtonTimerHdlrID = AmbaKAL_TimerCreate(&CaptureButtonTimerHdlr, AMBA_KAL_AUTO_START, \
                    &AmpUT_CaptureButtonTimer_Handler, ScriptID, Duration, 0xFFFFFFFF); // only invoke once
            } else {
                AmbaPrint(" Button Timer does not reset !!!!!");
            }
        }
    } else if (strcmp(argv[1],"singlecapturecont") == 0 || strcmp(argv[1],"scapc") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc singlecapturecont [encID][capcnt][timeLapse]");
            AmbaPrint("             encID: encode ID");
            AmbaPrint("             capcnt: target capture number");
            AmbaPrint("             timeLapse: time interval(unit in ms) to trigger next shot");
            AmbaPrint("                          0: trigger capture as soon as possible");
            AmbaPrint("                         >0: suggest this value larger than 500");
            return -1;
       } else {
            UINT16 TimeLapse = 0;
            memset(&ScapCont, 0x0, sizeof(AMP_RAW_CAPTURE_PARAM_s));
            ScapCont.EncID = atoi(argv[2]);
            ScapCont.CaptureCount = G_capcnt = atoi(argv[3]);
            TimeLapse = atoi(argv[4]);
            ScapCont.Iso = G_iso = DEFAULT_STILL_ISO_MODE;
            ScapCont.Compressed = DEFAULT_STILL_RAW_COMPRESSED_MODE;
            ScapCont.TargetSize = DEFAULT_STILL_JPEG_TARGET_SIZE;
            ScapCont.EncLoop = DEFAULT_STILL_JPEG_ENCODE_LOOP;
            CaptureMode = STILL_CAPTURE_SINGLE_SHOT_CONT;
            AmpUT_StillEncAAALock();
            if (TimeLapse == 0) {
                StillIsScapCTimeLapse = 0;
                AmpUT_singleCaptureCont(ScapCont.EncID, ScapCont.Iso, ScapCont.Compressed, ScapCont.TargetSize, ScapCont.EncLoop);
            } else {
                StillIsScapCTimeLapse = 1;
                if (ScapCTimerHdlrID < 0) {
                    ScapCTimerHdlrID = AmbaKAL_TimerCreate(&ScapCTimerHdlr, AMBA_KAL_AUTO_START,\
                        &AmpUT_ScapCTimer_Handler, (UINT32) &ScapCont, 1, TimeLapse); // Set InitTick as 1ms to trigger capture ASAP.
                }
            }
        }
    } else if (strcmp(argv[1],"autoExposureBracketing") == 0 || strcmp(argv[1],"aeb") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc autoExposureBracketing [encID]");
            AmbaPrint("             encID: encode ID");
            return -1;
        } else {
            UINT8 EncID = atoi(argv[2]);
            UINT8 Cmpr = DEFAULT_STILL_RAW_COMPRESSED_MODE;
            UINT32 TargetSize = DEFAULT_STILL_JPEG_TARGET_SIZE;
            UINT8 EncodeLoop = DEFAULT_STILL_JPEG_ENCODE_LOOP;
            G_iso = DEFAULT_STILL_ISO_MODE;
            G_capcnt = 3;
            CaptureMode = STILL_CAPTURE_AEB;
            AmpUT_StillEncAAALock();
            AmpUT_AutoExposureBracketing(EncID, G_iso, Cmpr, TargetSize, EncodeLoop);
        }
    } else if (strcmp(argv[1],"precapture") == 0 || strcmp(argv[1],"precap") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc precap [encID][duration]");
            AmbaPrint("             encID: encode ID");
            AmbaPrint("             duration: simulate how long shutter key is pressed(unit in ms)");
            return -1;
        } else {
            UINT8 EncID = atoi(argv[2]);
            UINT32 Duration = atoi(argv[3]);
            UINT8 Cmpr = DEFAULT_STILL_RAW_COMPRESSED_MODE;
            UINT32 TargetSize = DEFAULT_STILL_JPEG_TARGET_SIZE;
            UINT8 EncodeLoop = DEFAULT_STILL_JPEG_ENCODE_LOOP;
            UINT32 ScriptId = 0;
            G_iso = DEFAULT_STILL_ISO_MODE;
            CaptureMode = STILL_CAPTURE_PRE_CAPTURE;
            AmpUT_StillEncAAALock();
            CaptureButtonPressed = 1;

            ScriptId = AmpUT_PreCapture(EncID, G_iso, Cmpr, TargetSize, EncodeLoop);
            if (CaptureButtonTimerHdlrID < 0) {
                CaptureButtonTimerHdlrID = AmbaKAL_TimerCreate(&CaptureButtonTimerHdlr, AMBA_KAL_AUTO_START, \
                    &AmpUT_CaptureButtonTimer_Handler, ScriptId, Duration, 0xFFFFFFFF); // only invoke once
            } else {
                AmbaPrint(" Button Timer does not reset !!!!!");
            }
        }
    } else if (strcmp(argv[1],"scriptstop") == 0 || strcmp(argv[1],"scrpsp") == 0) {
        UINT8 EncID = 0;
        UINT8 Cmpr = DEFAULT_STILL_RAW_COMPRESSED_MODE;
        UINT32 TargetSize = DEFAULT_STILL_JPEG_TARGET_SIZE;
        UINT8 EncodeLoop = DEFAULT_STILL_JPEG_ENCODE_LOOP;
        UINT32 TriggerTime = atoi(argv[2]);
        UINT32 ScriptId = 0;

        if (argc < 3) {
            AmbaPrint("Usage: t stillenc scriptstop [TriggerTime]");
            AmbaPrint("            TriggerTime: time to trigger stop command after received raw capture done");
            AmbaPrint("            Burst mode for six photos is used for simulation");
            return -1;
        }

        G_iso = 1;
        G_capcnt = 6;
        CaptureMode = STILL_CAPTURE_BURST_SHOT;
        AmpUT_StillEncAAALock();
        ScriptId = AmpUT_burstCapture(EncID, G_iso, Cmpr, TargetSize, EncodeLoop, G_capcnt);
        if (ForceStopTimerHdlrID < 0) {
            ForceStopTimerHdlrID = AmbaKAL_TimerCreate(&ForceStopTimerHdlr, AMBA_KAL_DO_NOT_START, \
                &AmpUT_ForceStopTimer_Handler, ScriptId, TriggerTime, 0xFFFFFFFF);
        }
    } else if (strcmp(argv[1],"raw2raw") == 0 || strcmp(argv[1],"r2r") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc raw2raw [inputID][encID][mode][cmpr][bits][bayer]");
            AmbaPrint("             inputID: input raw ID");
            AmbaPrint("             encID: encode ID");
            AmbaPrint("             mode: raw2raw execute mode");
            AmbaPrint("                 0x1: raw2raw 3A");
        #ifdef CONFIG_SOC_A12
            AmbaPrint("                 0x2: raw2raw compress/umcompress");
        #else
            AmbaPrint("                 0x2: raw2raw compress/umcompress (Not Support Yet!!)");
        #endif
            AmbaPrint("             cmpr: compressed raw");
            AmbaPrint("             bits: raw data bits");
            AmbaPrint("             bayer: raw bayer pattern, RG(0) BG(1) GR(2) GB(3)");
            return -1;
        } else {
            UINT32 InputID = atoi(argv[2]);
            UINT8 EncID = atoi(argv[3]);
            UINT32 Mode = atoi(argv[4]);
            UINT8 Cmpr = atoi(argv[5]);
            UINT8 Bits = atoi(argv[6]);
            UINT8 Bayer = atoi(argv[7]);

            CaptureMode = STILL_CAPTURE_RAW2RAW;
            G_iso = DEFAULT_STILL_ISO_MODE; // IK may need iso config for debug purpose. (just use LISO)
            if (Mode == 1) {
                AmpUT_raw2raw(InputID, EncID, Mode, Cmpr, Bits, Bayer);
            } else if (Mode == 2) {
            #ifdef CONFIG_SOC_A12
                AmpUT_raw2raw(InputID, EncID, Mode, Cmpr, Bits, Bayer);
            #else
                AmbaPrint("currently not support raw compress/decompress");
            #endif
            }
        }
    } else if (strcmp(argv[1],"quickview") == 0 || strcmp(argv[1],"qv") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc quickview [qvMode]");
            AmbaPrint("             qvmode: quickview display mode");
            AmbaPrint("                 0x0: disable quickview");
            AmbaPrint("                 0x1: 1 sec");
            AmbaPrint("                 0x2: 3 sec");
            AmbaPrint("                 0x3: 5 sec");
            AmbaPrint("                 0x4: infinite until app press cancel key");
            return -1;
        } else {
            QvDisplayCfg = atoi(argv[2]);
            AmbaPrint("[AmpUT] Quickview delay time %dms", (QvDisplayCfg == 0)?0:QvShowTimeTable[QvDisplayCfg-1]);
        }
    } else if (strcmp(argv[1],"quickviewstop") == 0 || strcmp(argv[1],"qvsp") == 0) {
        STILLENC_UT_MSG_S Msg = {0};
        Msg.Event = MSG_STILLENC_UT_QV_TIMER_TIMEOUT;
        Msg.Data.DataInfo[0] = QvTimerID;
        AmbaPrint("Snd 0x%X (%d)!", Msg.Event, Msg.Data.DataInfo[0]);
        AmbaKAL_MsgQueueSend(&UtMsgQueue, &Msg, AMBA_KAL_WAIT_FOREVER);
    } else if ((strcmp(argv[1],"3a") == 0)) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc 3a [enable]");
            AmbaPrint("             0: disable");
            AmbaPrint("             1: enable");
        } else {
            UINT8 Is3aEnable = atoi(argv[2]);
            UINT8 i;
            UINT32 ChannelCount = 0;
            AMBA_3A_OP_INFO_s AaaOpInfo = {0};

            AmbaImg_Proc_Cmd(MW_IP_GET_TOTAL_CH_COUNT, (UINT32)&ChannelCount,0, 0);

            if (Is3aEnable == 0) {
                StillEnc3AEnable = 0;
                AaaOpInfo.AeOp = DISABLE;
                AaaOpInfo.AfOp = DISABLE;
                AaaOpInfo.AwbOp = DISABLE;
                AaaOpInfo.AdjOp = DISABLE;
            } else if (Is3aEnable == 1) {
                StillEnc3AEnable = 1;
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
                AmbaPrint("ChNo[%u] 3A %s: ae:%u af:%u awb:%u adj:%u", i, StillEnc3AEnable? "Enable": "Disable", \
                    AaaOpInfo.AeOp, AaaOpInfo.AfOp, AaaOpInfo.AwbOp, AaaOpInfo.AdjOp);
            }
        }
    } else if (strcmp(argv[1],"ob")==0) {
        if (argc < 3) {
            AmbaPrint("Usage: t stillenc ob [enable]");
            AmbaPrint("             0: disable");
            AmbaPrint("             1: enable");
        } else {
            UINT8 IsOBEnable = atoi(argv[2]);
            if (IsOBEnable == 0) {
                StillOBModeEnable = 0;
                AmbaPrint("Disable ob for sensor mode %u", StillEncModeIdx);
            } else if (IsOBEnable == 1) {
                UINT8 IsSensorSupportOB = 0;
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
                AMBA_SENSOR_MODE_ID_u SensorMode = {0};

                if (Status == STATUS_STILL_LIVEVIEW) {
                    SensorMode.Data = StillEncMgt[StillEncModeIdx].InputStillMode;
                } else {
                    SensorMode.Data = (StillEncodeSystem)? StillEncMgt[StillEncModeIdx].InputMode: StillEncMgt[StillEncModeIdx].InputPALMode;
                }

                SensorMode.Data = StillEncMgt[StillEncModeIdx].InputStillMode;
                AmbaSensor_GetModeInfo(EncChannel, SensorMode, &SensorModeInfo);
                if (SensorModeInfo.OutputInfo.OpticalBlackPixels.Width || SensorModeInfo.OutputInfo.OpticalBlackPixels.Height) {
                    IsSensorSupportOB = 1;
                }
                if (IsSensorSupportOB == 1) {
                    StillOBModeEnable = 1;
                    AmbaPrint("Enable ob for sensor mode %u", StillEncModeIdx);
                } else {
                    AmbaPrint("sensor mode %u does not support ob", StillEncModeIdx);
                }
            }
        }
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
    } else if (strcmp(argv[1],"pbcap") == 0) {
        int er;
        void *tempPtr;
        void *TempRawPtr;
        UINT32 yuvSize = 0;
        char fn[32];
        char fn1[32];
        AMP_CFS_FILE_s *y = NULL;
        AMP_CFS_FILE_s *uv = NULL;
        char mdASCII[3] = {'r','+','\0'};

        AMP_STILLENC_PB_CAPTURE_CTRL_s pBCapCtrl;

        pBCapCtrl.MainBuf.ColorFmt = AMP_YUV_422;
        pBCapCtrl.MainBuf.Width = 4096;//4000;
        pBCapCtrl.MainBuf.Height = 2160;//3000;
        pBCapCtrl.MainBuf.Pitch = 4096;//4000;
        pBCapCtrl.MainBuf.AOI.X = pBCapCtrl.MainBuf.AOI.Y= 0;
        pBCapCtrl.MainBuf.AOI.Width = 4096;//4000;
        pBCapCtrl.MainBuf.AOI.Height = 2160;//3000;
        pBCapCtrl.MainWidth = pBCapCtrl.MainHeight = 0;
        pBCapCtrl.ScrnWidth = 960;
        pBCapCtrl.ScrnHeight = ALIGN_16(540);
        pBCapCtrl.ScrnActiveWidth = 960;
        pBCapCtrl.ScrnActiveHeight = ALIGN_16(540);//ALIGN_16(720);
        pBCapCtrl.ThmWidth = 160;
        pBCapCtrl.ThmHeight = ALIGN_16(120);
        pBCapCtrl.ThmActiveWidth = 160;
        pBCapCtrl.ThmActiveHeight = ALIGN_16(90);//ALIGN_16(120);
        AmpUT_initJpegDqt(AmpUTJpegQTable[0], 95);
        pBCapCtrl.MainQTAddr = AmpUTJpegQTable[0];
        AmpUT_initJpegDqt(AmpUTJpegQTable[1], 95);
        pBCapCtrl.ThmQTAddr = AmpUTJpegQTable[1];
        AmpUT_initJpegDqt(AmpUTJpegQTable[2], 95);
        pBCapCtrl.ScrnQTAddr = AmpUTJpegQTable[2];

        yuvSize = pBCapCtrl.MainBuf.Pitch*pBCapCtrl.MainBuf.Height*2;
        er = AmpUtil_GetAlignedPool(&G_MMPL, &tempPtr, &TempRawPtr, yuvSize, 32);
        if (er != OK) {
            AmbaPrint("[UT_PBCap]Cache_DDR alloc yuv fail (%u)!", yuvSize);
        } else {
            YuvBuffAddr = (UINT8*)tempPtr;
            AmbaPrint("[UT_PBCap]yuvBuffAddr (0x%08X)!", YuvBuffAddr);
        }

        sprintf(fn, "%s:\\pbcap.y", DefaultSlot);
        sprintf(fn1, "%s:\\pbcap.uv", DefaultSlot);

        y = UT_StillEncodefopen((const char *)fn, (const char *)mdASCII);
        AmbaPrint("[UT_PBCap]Read y 0x%X %d %d from %s Start!", \
                YuvBuffAddr, pBCapCtrl.MainBuf.Width, pBCapCtrl.MainBuf.Height, fn);
        UT_StillEncodefread((void *)YuvBuffAddr, 1, (yuvSize>>1), (void *)y);
        UT_StillEncodefclose((void *)y);
        AmbaCache_Clean((void *)YuvBuffAddr, (yuvSize>>1));

        uv = UT_StillEncodefopen((const char *)fn1, (const char *)mdASCII);
        AmbaPrint("[UT_PBCap]Read uv 0x%X %d %d from %s Start!", \
                YuvBuffAddr+(yuvSize>>1), pBCapCtrl.MainBuf.Width, pBCapCtrl.MainBuf.Height, fn1);
        UT_StillEncodefread((void *)YuvBuffAddr+(yuvSize>>1), 1, (yuvSize>>1), (void *)uv);
        UT_StillEncodefclose((void *)uv);
        AmbaCache_Clean((void *)YuvBuffAddr+(yuvSize>>1), (yuvSize>>1));
        AmbaPrint("[UT_PBCap]Read Yuv Done!");

        pBCapCtrl.MainBuf.LumaAddr = YuvBuffAddr;
        pBCapCtrl.MainBuf.ChromaAddr = YuvBuffAddr + (yuvSize>>1);
        AmpUT_StillEnc_PBCapture(pBCapCtrl);
    } else if (strcmp(argv[1],"del") == 0) {
        AmpStillEnc_Delete(StillEncPri);
        StillEncPri = NULL;
    } else if (strcmp(argv[1],"rotate") == 0 || strcmp(argv[1],"rot") == 0) {
        if (argc != 3) {
            AmbaPrint("Usage: t stillenc rotation [type] ");
            AmbaPrint("              type: Predefined rotation value");
            AmbaPrint("                    0 - No rotation ");
            AmbaPrint("                    1 - Horizontal flip");
            AmbaPrint("                    2 - 90 clockwise");
            AmbaPrint("                    3 - 90 clockwise then verical flip");
            AmbaPrint("                    4 - 180 clockwise");
            AmbaPrint("                    5 - 180 clockwise then horizontal flip");
            AmbaPrint("                    6 - 270 clockwise");
            AmbaPrint("                    7 - 270 clockwise then vertical flip");
        #ifdef CONFIG_SOC_A9
            AmbaPrint(" Temp only support Type_5 via sensor VFlip reg");
        #else
            AmbaPrint(" Shall only Jpeg has effect");
        #endif
            return -1;
        }
        StillEncRotate = (AMP_ROTATION_e)atoi(argv[2]);
    #ifdef CONFIG_SOC_A9
        if (StillEncRotate == AMP_ROTATE_180_HORZ_FLIP || StillEncRotate == AMP_ROTATE_0) {
            AmbaPrint("Rotation %d ", StillEncRotate);
        } else {
            AmbaPrint("Rotation type Not Support");
            StillEncRotate = AMP_ROTATE_0;
        }
    #else
        AmbaPrint("Rotation %d ", StillEncRotate);
    #endif
    } else if (strcmp(argv[1],"tv") == 0) {
        if (Status != STATUS_STILL_INIT) {
            AmbaPrint("Please set it before liveviewstart");
        }

        TVLiveViewEnable = atoi(argv[2]);

        AmbaPrint("TV: %s", TVLiveViewEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"manualexposure") == 0 || strcmp(argv[1],"me") == 0) {
        if (argc != 4) {
            AmbaPrint("Usage: t stillenc manualexposure [shutter][gain] ");
            AmbaPrint("              shutter: shutter time, 0 = default");
            AmbaPrint("                 gain: gain control, 0 = default");
        } else {
            float ShutterTime = atof(argv[2]);
            float GainControl = atof(argv[3]);
            UINT32 GainFactor, AGainCtrl, DGainCtrl;
            UINT32 ShutterCtrl;
            UINT8 ExposureFrames = 0;
            AMBA_SENSOR_STATUS_INFO_s SensorStatus;
            AMBA_DSP_CHANNEL_ID_u Chan;
            Chan.Bits.VinID = 0;
            Chan.Bits.SensorID = 0;

            if (ShutterTime > 0.0) {
                AmbaSensor_GetStatus(Chan, &SensorStatus);
                AmbaSensor_ConvertShutterSpeed(Chan, ShutterTime, &ShutterCtrl);
                ExposureFrames = (ShutterCtrl/SensorStatus.ModeInfo.NumExposureStepPerFrame);
                ExposureFrames = (ShutterCtrl%SensorStatus.ModeInfo.NumExposureStepPerFrame)? ExposureFrames+1: ExposureFrames;
                AmbaSensor_SetSlowShutterCtrl(Chan, ExposureFrames);
                AmbaSensor_SetShutterCtrl(Chan, ShutterCtrl);
                AmbaPrint("Manual Shutter time: %fms", ShutterTime);
            } else {
                AmbaPrint("Shutter time: %fms should larger than 0", ShutterTime);
            }
            if (GainControl >= 0.0) {
                AmbaSensor_ConvertGainFactor(Chan, GainControl, &GainFactor, &AGainCtrl, &DGainCtrl);
                AmbaSensor_SetAnalogGainCtrl(Chan, AGainCtrl);
                AmbaSensor_SetDigitalGainCtrl(Chan, DGainCtrl);
                AmbaPrint("Manual Gain control: %f", GainControl);
            } else {
                AmbaPrint("Gain control: %f should larger than 1.0(0dB)", GainControl);
            }
        }
    } else if (strcmp(argv[1],"dumpskip") == 0) {
        UINT8 Skip = atoi(argv[2]);

        AmbaPrint("DumpSkipFlag is 0x%X", Skip);
        StillDumpSkip = Skip;
    } else if (strcmp(argv[1],"dumpituner") == 0) {
        UINT8 Enable = atoi(argv[2]);
        if (Enable == 1) {
            TUNE_Initial_Config_s ItunerInitConfig = {0};
            ItunerInitConfig.Text.pBytePool = &G_MMPL;
            StillEncDumpItuner = 1;
            AmbaTUNE_Change_Parser_Mode(TEXT_TUNE);
            if (OK != AmbaTUNE_Init(&ItunerInitConfig)) {
                AmbaPrintColor(RED, "AmbaTUNE_Init() failed");
            } else {
                AmbaPrint("Enable dump ituner file (%u)", StillEncDumpItuner);
            }
        } else if (Enable == 0) {
            AmbaTUNE_UnInit();
            StillEncDumpItuner = 0;
            AmbaPrint("Disable dump ituner file (%u)", StillEncDumpItuner);
        }
    } else if (strcmp(argv[1],"system") == 0) {
        UINT8 System = atoi(argv[2]);
        AmbaPrint("Encode System is %s", System? "PAL": "NTSC");
        StillEncodeSystem = System;
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
    } else if (strcmp(argv[1],"suspend") == 0) {
        DspSuspendEnable = atoi(argv[2]);
        AmbaPrint("%s Dsp suspend in LiveveiwStop !", DspSuspendEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"wirelessmode") == 0) {
        WirelessModeEnable = atoi(argv[2]);
        AmbaPrint("%s Enter Wireless mode in LiveveiwStop !", WirelessModeEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"dspwork") == 0) {
        UINT32 WorkSize = atoi(argv[2]);

        if (argc < 3) {
            AmbaPrint("Usage: t stillenc dspwork [size]");
            AmbaPrint("                    size : In MByte unit");
            return 0;
        }
        StillEncCusomtDspWorkSize = WorkSize;
        AmbaPrint("Set Dsp work size : %d MByte", WorkSize);
    } else {
        AmbaPrint("Usage: t stillenc [cmd]");
        AmbaPrint("       init: init codec");
        AmbaPrint("       encID: show encode ID and Spec.");
        AmbaPrint("       liveviewproc: Liveview Proc Config");
        AmbaPrint("       liveviewstart: start Liveview");
        AmbaPrint("       liveviewstop: stop Liveview");
        AmbaPrint("       modechange: change LiveView mode");
        AmbaPrint("       rawcap : Execute single raw capture");
        AmbaPrint("       yuvconv : Execute single yuv conversion");
        AmbaPrint("       jpgenc : Execute single Jpeg encode");
        AmbaPrint("       singleCapture : Execute single capture");
        AmbaPrint("       burstCapture : Execute burst capture");
        AmbaPrint("       burstCaptureCont: Execute continuous burst capture");
        AmbaPrint("       singleCaptureCont : Execute continuous single capture");
        AmbaPrint("       autoExposureBracketing: Execute auto exposure bracketing(AEB) capture");
        AmbaPrint("       preCapture: Execute pre-capture");
        AmbaPrint("       raw2raw : Execute raw2raw");
        AmbaPrint("       quickview : quickview config");
        AmbaPrint("       quickviewstop : stop quickview and back to LiveView");
    }
    return 0;
}

int AmpUT_StillEncTestAdd(void)
{
    AmbaPrint("Adding AmpUT_StillEnc");
    // hook command
    AmbaTest_RegisterCommand("se", AmpUT_StillEncTest);
    AmbaTest_RegisterCommand("stillenc", AmpUT_StillEncTest);

    return AMP_OK;
}
