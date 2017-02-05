 /**
  * @file src/app/sample/unittest/AmpUT_VideoTuning.c
  *
  * Video Tuning unit test
  *
  * History:
  *    2013/09/17 - [Edgar Lai] created file
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
#include <display/Osd.h>
#include <display/Display.h>
#include <util.h>
#include "AmbaAudio.h"
#include <AmbaDSP_WarpCore.h>
#include <recorder/StillEnc.h>
#include <scheduler.h>
#include <img/ImgDzoom.h>
#include "AmbaUtility.h"
#include "AmbaCache.h"
#include "AmbaTUNE_HdlrManager.h"

#include <cfs/AmpCfs.h>
#include <AmbaCalibInfo.h>
#include "AmbaLCD.h"
#ifdef CONFIG_SOC_A12
#include "AmbaTUNE_HdlrManager.h"
#include <AmbaImg_Adjustment_Def.h>
#include <AmbaImg_AeAwb.h>
#include <math.h>
#endif
#include <AmbaImg_AaaDef.h>

#include <AmbaImg_Proc.h>
#include <AmbaImg_Impl_Cmd.h>
#include <AmbaImg_VIn_Handler.h>
#include <AmbaImg_VDsp_Handler.h>
#include "AmpUT_Display.h"

static char DefaultSlot[] = "C";


void *UT_VideoTuningfopen(const char *pFileName, const char *pMode)
{
    AMP_CFS_FILE_PARAM_s cfsParam;
    AmpCFS_GetFileParam(&cfsParam);
    cfsParam.AsyncMode = 1;
    cfsParam.AsyncData.MaxNumBank = 2;
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
        AmbaPrint("[%s]: Unicode should be abandoned", __func__);
        return NULL;
    } else {
        strcpy((char *)cfsParam.Filename, pFileName);
        return (void *)AmpCFS_fopen(&cfsParam);
    }
}

int UT_VideoTuningfclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_VideoTuningfread(void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fread(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_VideoTuningfwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_VideoTuningfsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

int AmpUT_VideoTuningDumpAoi(UINT8 *Addr, UINT16 Pitch, AMP_AREA_s Aoi, void *Fp)
{
    UINT8 *StartAddress = Addr;
    UINT16 i;
    StartAddress += (Aoi.X + Aoi.Y*Pitch);
    for (i=0; i<Aoi.Height; i++) {
        UT_VideoTuningfwrite((const void *)StartAddress, 1, Aoi.Width, Fp);
        StartAddress += Pitch;
    }

    return OK;
}

UINT32 UT_VideoTuningFrameRateIntConvert(UINT32 OriFrameRate)
{
    UINT32 FrameRateInt = OriFrameRate;

    if (OriFrameRate==29 || OriFrameRate==59 || OriFrameRate==119 || OriFrameRate==239) {
        FrameRateInt++;
    }

    return FrameRateInt;
}

static UINT8 VideoTuningDumpRawSkip = 0;
static UINT8 DumpYuvEnable = 0;
static UINT8 DumpMeYEnable = 0;
static UINT16 YuvFno = 1;
static UINT16 MeYFno = 1;
static UINT8 VideoTuningRawIndex = 0;
UINT8* VideoTuningRawAddr[128];

#define _STILL_BUFFER_FROM_DSP_

static UINT8 VideoRawEncIsoCfgIndex = 0;
#define VIDEORAWENC_MAX_FN_SIZE (64)

// global var for VideoTuning
static AMP_VIN_HDLR_s *VideoTuningVinA = NULL;      // Vin instance
static AMP_AVENC_HDLR_s *VideoTuningPri = NULL;     // Primary VideoTuning codec instance
static AMP_ENC_PIPE_HDLR_s *VideoTuningPipe = NULL; // VideoTuning pipeline instance
static UINT8 *VideoTuningVinWorkBuf = NULL;         // Vin module working buffer
static UINT8 *VideoTuningVencWorkBuf = NULL;        // VideoTuning working buffer
static UINT8 *MainVideoTuningImgSchdlrWorkBuf = NULL;   // Img scheduler working buffer
static AMBA_DSP_CHANNEL_ID_u TuningEncChannel;      // Vin channel
#define TUNING_STATUS_NO_INIT     1
#define TUNING_STATUS_IDLE     2
#define TUNING_STATUS_LIVEVIEW 3
#define TUNING_STATUS_ENCODE   4
static UINT8 TuningStatus = TUNING_STATUS_NO_INIT;     // UT_Tuning status

#ifdef CONFIG_LCD_WDF9648W
extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;
#endif

static UINT8 TuningLCDLiveview = 1;                 // Enable LCD liveview or not
static UINT8 TuningTvLiveview = 1;                  // Enable TV liveview or not
static UINT8 VideoTuning3AEnable = 1;               // Enable AE/AWB/ADJ algo or not
static UINT8 VideoTuning3AEnableDefault = 1;        // Default Enable AE/AWB/ADJ algo or not
static UINT8 TuningLiveViewProcMode = 0;            // LiveView Process Mode, 0: Express 1:Hybrid
static UINT8 TuningLiveViewAlgoMode = 0;            // LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
static UINT8 TuningLiveViewOSMode = 0;              // LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 TuningLiveViewHdrMode = 0;             // LiveView HDR Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 TuningEncodeSystem = 0;                // Encode System, 0:NTSC, 1: PAL
#ifdef CONFIG_SOC_A12
static UINT8 TuningStillIso = 0;
static UINT8 TuningEncThumbnail = 0;                // Enable video thumbnail or not
static UINT8 IsTurnOnLcdQV = 0;                     // A12 always ask for LCD qv
#endif
static ITUNER_INFO_s ItunerInfo;
static AMBA_DSP_IMG_MODE_CFG_s ImgMode;

typedef enum _TUNNING_CAPTURE_MODE_e_ {
    TUNNING_MODE_CAPTURE_NONE,
    TUNNING_MODE_LIVEVIEW_CAPTUE,
    TUNNING_MODE_VIDEORAWENCODE,
    TUNNING_CAPTURE_UNKNOWN = 0xFF
} TUNNING_CAPTURE_MODE_e;
static TUNNING_CAPTURE_MODE_e VideoTuningCaptureMode = TUNNING_MODE_CAPTURE_NONE;
static UINT8 IsFastCapture = 0;
static UINT8 IsRawCompr = 0;

// global var for StillEnc codec
static UINT8 *VideoTuningSencWorkBuf = NULL;
static AMP_STLENC_HDLR_s *VideoTuningSencPri = NULL;
static AMP_ENC_PIPE_HDLR_s *VideoTuningSencPipe = NULL;
static UINT8 VideoTuningSencInit = 0; // VideoRawEncode StillCodec init flag
static UINT8 VideoTuningRawCapRunning = 0; // VideoRawEncode Raw Capture Running flag
static UINT8 VideoTuningEnableFastCapture = 0;
static UINT8  LvCapture = 0; // Under LiveView raw capture or not
static UINT32 LvCaptureNum = 0; // LiveView raw capture raw index
static AMP_STILLENC_RAW_TYPE_e LvCaptureRawType = (AMP_STILLENC_RAW_TYPE_e)1; // CFA raw format

#define VIN_MAIN_ONLY (0)
#define VIN_PIP_ONLY (1)
#define VIN_MAIN_PIP (2) // VT forbid dual vin case
static UINT8 VtLiveviewVinSelect = VIN_MAIN_ONLY;
static UINT8 VtStillEncVinSelect = VIN_MAIN_ONLY;

typedef struct _AMPUT_VIDEOENC_LVCAPTURE_BIN_s_ {
    UINT16                  CaptureNum;     /**< CFA raw number be captured */
    UINT16                  RawPitch;       /**< CFA raw buffer pitch */
    UINT16                  RawWidth;       /**< CFA raw width */
    UINT16                  RawHeight;      /**< CFA raw height */
    UINT8                   IsYuv;          /**< Yuv raw data */
    AMP_STILLENC_RAW_TYPE_e RawType;        /**< CFA raw format */
    AMP_COLOR_FORMAT_e      YuvFmt;         /**< Yuv format */
    UINT32                  TimeScale;      /**< Time scale */
    UINT32                  TickPerPicture; /**< tick per picture */
    UINT8                   Interlace;      /**< pregressive mode or not */
    UINT16                  AspectRatio;    /**< CFA raw aspect ratio */
} AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s;
typedef struct _AMPUT_VIDEOENC_RAWENCODE_s_ {
    AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s rawBin;            /**< liveview raw capture informaion */
    AMP_CFS_FILE_s                     *currRawFP;        /**< current raw file pointer */
    UINT32                             currBinaryIndex;   /**< current processing binary index */
    UINT16                             currRawIndex;      /**< current processing raw index */
    char                               currRawPath[VIDEORAWENC_MAX_FN_SIZE];   /**< current raw file path */
    char                               currTunerPath[VIDEORAWENC_MAX_FN_SIZE]; /**< current iTuner file path */
} AMPUT_VIDEOTUNING_LVCAPTURE_BIN_s;

static AMPUT_VIDEOTUNING_LVCAPTURE_BIN_s VideoRawEncVar; // VideoRawEncode information

static UINT8 *VideoRawBuffAddr = NULL;
static UINT8 *ScriptAddr = NULL;
static UINT8 *RawBuffAddr = NULL;
static UINT8 *YuvBuffAddr = NULL;
static UINT8 *ScrnBuffAddr = NULL;
static UINT8 *ThmBuffAddr = NULL;
static UINT8 *QvLCDBuffAddr = NULL;
static UINT8 *QvHDMIBuffAddr = NULL;
static UINT8 *OriScriptAddr = NULL;
static UINT8 *OriRawBuffAddr = NULL;
static UINT8 *OriQvLCDBuffAddr = NULL;
static UINT8 *OriQvHDMIBuffAddr = NULL;

static UINT16 QvLCDW = 0;
static UINT16 QvLCDH = 0;
static UINT16 QvHDMIW = 0;
static UINT16 QvHDMIH = 0;

/* Still Codec Function prototype*/
UINT32 AmpUT_VideoTuning_LvCaptureRawPostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_VideoTuning_LvCaptureRawPreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_VideoTuning_LvCaptureRawAndYuvPostCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_VideoTuning_LvCaptureRawAndYuvPreCB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_VideoTuning_DspWork_Calculate(UINT8 **addr, UINT32 *size);

static AMP_STILLENC_POSTP_s PostVideoTuningLVCaptureRawCB = {.Process = AmpUT_VideoTuning_LvCaptureRawPostCB};
static AMP_STILLENC_PREP_s PreVideoTuningLVCaptureRawCB = {.Process = AmpUT_VideoTuning_LvCaptureRawPreCB};
static AMP_STILLENC_POSTP_s PostVideoTuningLVCaptureRawAndYuvCB = {.Process = AmpUT_VideoTuning_LvCaptureRawAndYuvPostCB};
static AMP_STILLENC_PREP_s PreVideoTuningLVCaptureRawAndYuvCB = {.Process = AmpUT_VideoTuning_LvCaptureRawAndYuvPreCB};

static INPUT_ENC_MGT_s *VideoTuningMgt;
static UINT8 InputDeviceId = 0;
static UINT8 TuningModeIdx = 1;
static UINT32 InitZoomX = 1<<16;
static UINT32 InitZoomY = 1<<16;

#ifdef CONFIG_SOC_A9
// Window handler
static AMP_DISP_WINDOW_HDLR_s *EncLcdWinHdlr = NULL;
static AMP_DISP_WINDOW_HDLR_s *EncTvWinHdlr = NULL;
#endif

// Image scheduler handler
AMBA_IMG_SCHDLR_HDLR_s *MainVideoTuningImgSchdlr = NULL;
AMBA_IMG_SCHDLR_HDLR_s *PipVideoTuningImgSchdlr = NULL;

/* muxer */
#define BITSFIFO_SIZE (14*1024*1024)
static UINT8 *H264BitsBuf;
#define BISFIFO_STILL_SIZE (16*1024*1024)
static UINT8* JpegBitsBuf;
#define DESC_SIZE (40*1680)
static UINT8 *H264DescBuf;
static UINT8 *JpegDescBuf;
static AMP_FIFO_HDLR_s *VideoTuningVirtualPriFifoHdlr = NULL;
static AMP_FIFO_HDLR_s *VideoTuningVirtualJpegFifoHdlr = NULL;
static AMBA_KAL_TASK_t VideoTuningPriMuxTask = {0};
static AMBA_KAL_TASK_t VideoTuningJpegMuxTask = {0};
static AMBA_KAL_SEM_t VideoTuningPriSem = {0};
static AMBA_KAL_SEM_t VideoTuningJpegSem = {0};
static AMP_CFS_FILE_s *OutputPriFile = NULL;
static AMP_CFS_FILE_s *OutputPriIdxFile = NULL;
static UINT8 *AmpUT_VideoTuningPriStack;
#define VIDEO_TUNING_PRI_MUX_TASK_STACK_SIZE (8192)
static UINT8 *AmpUT_VideoTuningJpegStack;
#define VIDEO_TUNING_JPG_MUX_TASK_STACK_SIZE (8192)

static int FnoPri = 0;
static UINT16 FnoLvCap = 1;
static UINT16 SubRawFnoLvCap = 1;
static UINT16 SubYuvFnoLvCap = 1;
static UINT8 VideoTuningDumpItuner = 0;   // Dump ituner text file or not
#ifdef CONFIG_SOC_A12
UINT32 AmpUT_VideoTuning_PivPOSTCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_VideoTuning_PivPRECB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_VideoTuning_DspWork_Calculate(UINT8 **addr, UINT32 *size);

static AMP_STILLENC_POSTP_s post_videoTuningPiv_cb = {.Process = AmpUT_VideoTuning_PivPOSTCB};
static AMP_STILLENC_PREP_s pre_videoTuningPiv_cb = {.Process = AmpUT_VideoTuning_PivPRECB};
int AmpUT_VideoTuning_PIV(AMP_VIDEOENC_PIV_CTRL_s pivCtrl, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop);
static UINT32 TuningPivProcess = 0;
static UINT16 TuningPivCaptureWidth = 0;
static UINT16 TuningPivCaptureHeight = 0;
static UINT16 TuningPivMainWidth = 0;
static UINT16 TuningPivMainHeight = 0;
static UINT16 TuningScrnWidthAct = 0;
static UINT16 TuningScrnHeightAct = 0;
static UINT16 TuningThmWidthAct = 0;
static UINT16 TuningThmHeightAct = 0;
static UINT8  TuningPivVideoThm = 0;
static UINT8  TuningPivVideoThmJpgCount = 0;
static UINT16 TuningfnoRaw = 1;               // Frame number counter for piv/thumbnail raw picture
static UINT16 TuningfnoYuv = 1;               // Frame number counter for piv/thumbnail yuv picture
static UINT8 VideoTuningIsIqParamInit = 0;
static UINT8 VideoTuningIsHdrIqParam = 0;
#endif
//#define AMPUT_JPEG_DUMP
#ifdef AMPUT_JPEG_DUMP
static UINT16 SubJpgFnoLvCap = 1;
static UINT16 TuningfnoPiv = 1;               // Frame number counter for piv/thumbnail jpeg picture

static AMBA_FS_FILE *OutputFile = NULL;
#endif

#define GOP_N   8                   // I-frame distance
#define GOP_IDR 8                   // IDR-frame distance

/* Bitrate calculator */
static UINT32 EncFrameRate = 0;
static UINT64 EncPriBytesTotal = 0;
static UINT32 EncPriTotalFrames = 0;

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

static AMP_IMG_DZOOM_TABLE_s DzoomTable;
static AMP_IMG_DZOOM_HDLR_s *DzoomHdlr;

/* UT function prototype */
int AmpUT_VideoTuning_EncStop(void);
int AmpUT_VideoTuning_ChangeResolution(UINT32 modeIdx);
int AmpUT_VideoTuning_LiveviewStart(UINT32 modeIdx);
int AmpUT_VideoTuning_RawEncodeLiveviewStart(void);

/*
 * JPEG QTable
 */
static UINT8 VideoTuningStdJpegQTable[128] = {
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
UINT8 VideoTuningJpegQTable[3][128] = {
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


/* Initial JPEG DQT */
void AmpUT_VidoeTuning_InitJpegDqt(UINT8 *qTable, int quality)
{
    int i, scale, temp;

    /** for jpeg brc; return the quantization table*/
    if (quality == -1) {
        memcpy(qTable, VideoTuningStdJpegQTable, 128);
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
        temp = ((long) VideoTuningStdJpegQTable[i] * scale + 50L) / 100L;
        /* limit the values to the valid range */
        if (temp <= 0L) temp = 1L;
        if (temp > 255L) temp = 255L; /* max quantizer needed for baseline */
        qTable[i] = temp;
    }
}

int AmpUT_VideoTuningDumpItunerFile(char *pItunerFileName, char *pTuningModeExt, char *pRawPath)
{
    UINT32 ImgIpChNo = 0;
    ITUNER_SYSTEM_s ItunerSysInfo = {0};
    AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
    AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
    AMBA_AE_INFO_s VideoAeInfo = {0};
    ITUNER_AE_INFO_s ItunerAeInfo = {0};

    snprintf(ItunerSysInfo.ChipRev, sizeof(ItunerSysInfo.ChipRev), "A12");
    snprintf(ItunerSysInfo.TuningModeExt, sizeof(ItunerSysInfo.TuningModeExt), pTuningModeExt);
    memcpy(ItunerSysInfo.RawPath, pRawPath, sizeof(ItunerSysInfo.RawPath));

    AmbaSensor_GetStatus(TuningEncChannel, &SensorStatus);
    ItunerSysInfo.RawBayer = SensorStatus.ModeInfo.OutputInfo.CfaPattern;
    ItunerSysInfo.RawResolution = SensorStatus.ModeInfo.OutputInfo.NumDataBits;
    ItunerSysInfo.HSubSampleFactorDen = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorDen;
    ItunerSysInfo.HSubSampleFactorNum = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorNum;
    ItunerSysInfo.VSubSampleFactorDen = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorDen;
    ItunerSysInfo.VSubSampleFactorNum = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorNum;
    ItunerSysInfo.RawStartX = SensorStatus.ModeInfo.InputInfo.PhotodiodeArray.StartX; // Unit in pixel. Before down-sample.
    ItunerSysInfo.RawStartY = SensorStatus.ModeInfo.InputInfo.PhotodiodeArray.StartY; // Unit in pixel. Before down-sample.
    ItunerSysInfo.CompressedRaw = IsRawCompr;
    ItunerSysInfo.RawWidth = VideoTuningMgt[TuningModeIdx].CaptureWidth;
    ItunerSysInfo.RawHeight = VideoTuningMgt[TuningModeIdx].CaptureHeight;
    ItunerSysInfo.RawPitch = ALIGN_32((IsRawCompr)? \
        (AMP_COMPRESSED_RAW_WIDTH(VideoTuningMgt[TuningModeIdx].CaptureWidth)): \
        (VideoTuningMgt[TuningModeIdx].CaptureWidth*2));
    ItunerSysInfo.MainWidth = VideoTuningMgt[TuningModeIdx].MainWidth;
    ItunerSysInfo.MainHeight = VideoTuningMgt[TuningModeIdx].MainHeight;
    AmbaTUNE_Set_SystemInfo(&ItunerSysInfo);

    AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, ImgIpChNo, IP_MODE_VIDEO, (UINT32) &VideoAeInfo);
    ItunerAeInfo.EvIndex = VideoAeInfo.EvIndex;
    ItunerAeInfo.NfIndex = VideoAeInfo.NfIndex;
    ItunerAeInfo.ShutterIndex = VideoAeInfo.ShutterIndex;
    ItunerAeInfo.AgcIndex = VideoAeInfo.AgcIndex;
    ItunerAeInfo.IrisIndex = VideoAeInfo.IrisIndex;
    ItunerAeInfo.Dgain = VideoAeInfo.Dgain;
    ItunerAeInfo.IsoValue = VideoAeInfo.IsoValue;
    ItunerAeInfo.Flash = VideoAeInfo.Flash;
    ItunerAeInfo.Mode = VideoAeInfo.Mode;
    ItunerAeInfo.ShutterTime = (INT32) VideoAeInfo.ShutterTime;
    ItunerAeInfo.AgcGain = (INT32) VideoAeInfo.AgcGain;
    ItunerAeInfo.Target = VideoAeInfo.Target;
    ItunerAeInfo.LumaStat = VideoAeInfo.LumaStat;
    ItunerAeInfo.LimitStatus = VideoAeInfo.LimitStatus;
    ItunerAeInfo.Multiplier = 0; // FIXME:
    AmbaTUNE_Set_AeInfo(&ItunerAeInfo);

    ImgMode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO; //TBD
    ImgMode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST; //TBD
    ImgMode.ConfigId = 0;
    ImgMode.ContextId = 0;

    if (NULL != pItunerFileName) {
        TUNE_Save_Param_s Save_Param = {0};
        Save_Param.Text.Filepath = pItunerFileName;
        if (0 != AmbaTUNE_Save_IDSP(&ImgMode, &Save_Param)) {
            AmbaPrintColor(RED,"TextHdlr_Save_IDSP() failed");
        } else {
            AmbaPrint("[Amp_UT] Dump ituner text to %s done", pItunerFileName);
        }
    }

    return 0;
}

void AmpUT_VideoTuning_PriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *desc = NULL;
    int er = NG;
    UINT8 *bitsLimit = NULL;

    NHNT_HEADER_s nhntHeader = {0};
    NHNT_SAMPLE_HEADER_s nhntSample = {0};
    UINT64 fileOffset = 0;

    AmbaPrint("AmpUT_VideoTuning_MuxTask Start");

    while (1) {
        er = AmbaKAL_SemTake(&VideoTuningPriSem, 10000);
        if (er!= OK) {
            //AmbaPrint(" no sem fff");
            continue;
        }

#define AMPUT_FILE_DUMP
        #ifdef AMPUT_FILE_DUMP
        if (OutputPriFile == NULL) {
            char fn[80];
            char mdASCII[3] = {'w','+','\0'};
            AMP_CFS_FILE_s *udtaFile;
            FORMAT_USER_DATA_s udta;

            sprintf(fn,"%s:\\OUT_%04d.h264", DefaultSlot, FnoPri);
            OutputPriFile = UT_VideoTuningfopen((const char *)fn, (const char *)mdASCII);
            AmbaPrint("%s opened", fn);

            sprintf(fn,"%s:\\OUT_%04d.nhnt", DefaultSlot, FnoPri);
            OutputPriIdxFile = UT_VideoTuningfopen((const char *)fn, (const char *)mdASCII);
            AmbaPrint("%s opened", fn);

            sprintf(fn,"%s:\\OUT_%04d.udta", DefaultSlot, FnoPri);
            udtaFile = UT_VideoTuningfopen((const char *)fn, (const char *)mdASCII);
            AmbaPrint("%s opened", fn);

            udta.nIdrInterval = GOP_IDR/GOP_N;
            udta.nTimeScale = VideoTuningMgt[TuningModeIdx].TimeScale;
            udta.nTickPerPicture = VideoTuningMgt[TuningModeIdx].TickPerPicture;
            udta.nN = 15;
            udta.nM = VideoTuningMgt[TuningModeIdx].GopM;
            udta.nVideoWidth = VideoTuningMgt[TuningModeIdx].MainWidth;
            udta.nVideoHeight = VideoTuningMgt[TuningModeIdx].MainHeight;

            UT_VideoTuningfwrite((const void *)&udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)udtaFile);
            UT_VideoTuningfclose((void *)udtaFile);

            nhntHeader.Signature[0]='n';
            nhntHeader.Signature[1]='h';
            nhntHeader.Signature[2]='n';
            nhntHeader.Signature[3]='t';
            nhntHeader.TimeStampResolution = 90000;
            UT_VideoTuningfwrite((const void *)&nhntHeader, 1, sizeof(nhntHeader), (void *)OutputPriIdxFile);

            bitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
        }
        #endif

        er = AmpFifo_PeekEntry(VideoTuningVirtualPriFifoHdlr, &desc, 0);
        if (er == 0) {
            AmbaPrint("Pri[%d] pts:%8lld size:%5d@0x%08x", EncPriTotalFrames, desc->Pts, desc->Size, desc->StartAddr);
        } else {
            while (er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                er = AmpFifo_PeekEntry(VideoTuningVirtualPriFifoHdlr, &desc, 0);
            }
        }
        if (desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 avg;
            // EOS
            #ifdef AMPUT_FILE_DUMP
                if (OutputPriFile) {
                    UT_VideoTuningfsync((void *)OutputPriFile);
                    UT_VideoTuningfclose((void *)OutputPriFile);
                    FnoPri++;
                    UT_VideoTuningfsync((void *)OutputPriIdxFile);
                    UT_VideoTuningfclose((void *)OutputPriIdxFile);
                    OutputPriFile = NULL;
                    OutputPriIdxFile = NULL;
                    fileOffset = 0;
                }
            #endif

            avg = (UINT32)(EncPriBytesTotal*8/(EncPriTotalFrames/EncFrameRate)/1E6);


            AmbaPrint("Primary Muxer met EOS, total %d frames/fields", EncPriTotalFrames);
            AmbaPrint("Primary Bitrate Average: %d Mbps\n", avg);

            EncPriBytesTotal = 0;
            EncPriTotalFrames = 0;

            if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE) {
                //restore original 3A enable flag
                VideoTuning3AEnable = VideoTuning3AEnableDefault;
            }
        } else {
            #ifdef AMPUT_FILE_DUMP
                if (OutputPriFile) {
                    nhntSample.CompositionTimeStamp = desc->Pts;
                    nhntSample.DecodingTimeStamp = desc->Pts;
                    nhntSample.DataSize = desc->Size;
                    nhntSample.FileOffset = fileOffset;
                    fileOffset += desc->Size;
                    nhntSample.FrameType = desc->Type;

                    UT_VideoTuningfwrite((const void *)&nhntSample, 1, sizeof(nhntSample), (void *)OutputPriIdxFile);
             //       AmbaPrint("Write: 0x%x sz %d limit %X",desc->StartAddr,desc->size, bitsLimit);
                    if (desc->StartAddr + desc->Size <= bitsLimit) {
                       // AmbaCache_Flush(desc->StartAddr, desc->size); // Need to flush when using cacheable memory
                       UT_VideoTuningfwrite((const void *)desc->StartAddr, 1, desc->Size, (void *)OutputPriFile);
                    } else {
                       // AmbaCache_Flush(desc->StartAddr, bitsLimit - desc->StartAddr + 1);
                       UT_VideoTuningfwrite((const void *)desc->StartAddr, 1, bitsLimit - desc->StartAddr + 1, (void *)OutputPriFile);
                       // AmbaCache_Flush(H264BitsBuf, desc->size - (bitsLimit - desc->StartAddr + 1));
                       UT_VideoTuningfwrite((const void *)H264BitsBuf, 1, desc->Size - (bitsLimit - desc->StartAddr + 1), (void *)OutputPriFile);
                    }
                }
            #else
                AmbaKAL_TaskSleep(1);
            #endif
            EncPriBytesTotal += desc->Size;
            EncPriTotalFrames ++;
        }
        AmpFifo_RemoveEntry(VideoTuningVirtualPriFifoHdlr, 1);
    }
}

void AmpUT_VideoTuning_JpegMuxTask(UINT32 info)
{
    int Er;
    AMP_BITS_DESC_s *Desc;
    UINT8 dumpSkip = 0;

    AmbaPrint("AmpUT_VideoTuning_JpegMuxTask Start");

    while (1) {
        AmbaKAL_SemTake(&VideoTuningJpegSem, AMBA_KAL_WAIT_FOREVER);
        Er = AmpFifo_PeekEntry(VideoTuningVirtualJpegFifoHdlr, &Desc, 0);
        if (Er == 0) {
            AmbaPrint("Muxer PeekEntry: size:%5d@0x%08X Ft(%d) Seg(%u)", Desc->Size, Desc->StartAddr, Desc->Type, Desc->SeqNum);
        } else {
            while (Er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(VideoTuningVirtualJpegFifoHdlr, &Desc, 0);
            }
        }

        dumpSkip = 0;
    #ifdef AMPUT_JPEG_DUMP
        if (Desc->Size != AMP_FIFO_MARK_EOS) {
            char Fn[32];
            char mdASCII[3] = {'w','+','\0'};
            static UINT32 PrevSegNum = 0;
            if (PrevSegNum != Desc->SeqNum) {
                SubJpgFnoLvCap++;
                PrevSegNum = Desc->SeqNum;
                if (TuningPivProcess) {
                    TuningfnoPiv++;
                }
            }

            if (TuningPivProcess) {
                if (Desc->Type == AMP_FIFO_TYPE_JPEG_FRAME) {
                    sprintf(Fn,"%s:\\%04d_m_%02d.jpg", DefaultSlot, FnoPri, TuningfnoPiv);
                } else if (Desc->Type == AMP_FIFO_TYPE_THUMBNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d_t_%02d.jpg", DefaultSlot, FnoPri, TuningfnoPiv);
                } else if (Desc->Type == AMP_FIFO_TYPE_SCREENNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d_s_%02d.jpg", DefaultSlot, FnoPri, TuningfnoPiv);
                }
            } else if (TuningPivVideoThm) {
                if (Desc->Type == AMP_FIFO_TYPE_JPEG_FRAME || \
                    Desc->Type == AMP_FIFO_TYPE_SCREENNAIL_FRAME) {
                    // do not dump
                    dumpSkip = 1;
                    TuningPivVideoThmJpgCount--;
                } else if (Desc->Type == AMP_FIFO_TYPE_THUMBNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d.thm", DefaultSlot, FnoPri);
                    TuningPivVideoThmJpgCount--;
                }
                if (TuningPivVideoThmJpgCount == 0) {
                    TuningPivVideoThm = 0;
                }
            } else {
                if (Desc->Type == AMP_FIFO_TYPE_JPEG_FRAME) {
                    sprintf(Fn,"%s:\\LvCap_%04d_%04d_m.jpg", DefaultSlot, FnoLvCap, SubJpgFnoLvCap);
                } else if (Desc->Type == AMP_FIFO_TYPE_THUMBNAIL_FRAME) {
                    sprintf(Fn,"%s:\\LvCap_%04d_%04d_t.jpg", DefaultSlot, FnoLvCap, SubJpgFnoLvCap);
                } else if (Desc->Type == AMP_FIFO_TYPE_SCREENNAIL_FRAME) {
                    sprintf(Fn,"%s:\\LvCap_%04d_%04d_s.jpg", DefaultSlot, FnoLvCap, SubJpgFnoLvCap);
                }
            }

            if (dumpSkip == 0) {
                OutputFile = UT_VideoTuningfopen((const char *)Fn, (const char *)mdASCII);
                AmbaPrint("%s opened", Fn);
            }
            BitsLimit = JpegBitsBuf + BISFIFO_STILL_SIZE - 1;
        }
    #endif
        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            // EOS, stillenc do not produce EOS anymore...
        #ifdef AMPUT_JPEG_DUMP
            //UT_VideoTuningfsync((void *)OutputFile);
            //UT_VideoTuningfclose((void *)OutputFile);
            OutputFile = NULL;
        #endif
            AmbaPrint("Muxer met EOS");
        } else {
            static int i = 0;
#ifdef AMPUT_JPEG_DUMP
            if (dumpSkip == 0) {
                UINT8 *BitsLimit = JpegBitsBuf + BISFIFO_STILL_SIZE - 1;
                AmbaPrint("Write: 0x%x sz %d limit %X",  Desc->StartAddr, Desc->Size, BitsLimit);
                if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                    UT_VideoTuningfwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)OutputFile);
                }else{
                    UT_VideoTuningfwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)OutputFile);
                    UT_VideoTuningfwrite((const void *)JpegBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)OutputFile);
                }
                UT_VideoTuningfsync((void *)OutputFile);
                UT_VideoTuningfclose((void *)OutputFile);
            }
#endif
            i++;
            i%=3;
            if (i == 0) {
                if (TuningPivProcess) {
                    TuningPivProcess--;
                }

                if (ThmBuffAddr) {
                #ifndef _STILL_BUFFER_FROM_DSP_
                    if (AmbaKAL_BytePoolFree((void *)OriThmBuffAddr) != OK) {
                        AmbaPrint("[Amp_MUX] memFree Fail thm!");
                    }
                #endif
                    ThmBuffAddr = NULL;
                }
                if (ScrnBuffAddr) {
                #ifndef _STILL_BUFFER_FROM_DSP_
                    if (AmbaKAL_BytePoolFree((void *)OriScrnBuffAddr) != OK) {
                        AmbaPrint("[Amp_MUX] memFree Fail scrn!");
                    }
                #endif
                    ScrnBuffAddr = NULL;
                }
                if (YuvBuffAddr) {
                #ifndef _STILL_BUFFER_FROM_DSP_
                    if (AmbaKAL_BytePoolFree((void *)OriYuvBuffAddr) != OK) {
                        AmbaPrint("[Amp_MUX] memFree Fail yuv!");
                    }
                #endif
                    YuvBuffAddr = NULL;
                }
                if (QvLCDBuffAddr) {
                #ifndef _STILL_BUFFER_FROM_DSP_
                    if (AmbaKAL_BytePoolFree((void *)OriQvLCDBuffAddr) != OK) {
                        AmbaPrint("[Amp_MUX] memFree Fail qvLCD!");
                    }
                #endif
                    QvLCDBuffAddr = NULL;
                }

                if (QvHDMIBuffAddr) {
                #ifndef _STILL_BUFFER_FROM_DSP_
                    if (AmbaKAL_BytePoolFree((void *)OriQvHDMIBuffAddr) != OK) {
                        AmbaPrint("[Amp_MUX] memFree Fail qvHDMI!");
                    }
                #endif
                    QvHDMIBuffAddr = NULL;
                }
            }

        }
        AmpFifo_RemoveEntry(VideoTuningVirtualJpegFifoHdlr, 1);
    }
}


/**
 * Fifo callback when new entries are produced
 *
 * @param [in] hdlr  The event belongs to which virtual fifo
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_VideoTuning_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *numFrames = info;
    AMBA_KAL_SEM_t *pSem = NULL;

    if (hdlr == VideoTuningVirtualPriFifoHdlr) {
        pSem = &VideoTuningPriSem;
    } else if (hdlr == VideoTuningVirtualJpegFifoHdlr) {
        pSem = &VideoTuningJpegSem;
    }

    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for (i=0; i< *numFrames; i++) {
            AmbaKAL_SemGive(pSem);
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            AmbaKAL_SemGive(pSem);
    }

    return 0;
}

/*~muxer*/

/**
 * Generic VideoTuning ImgSchdlr callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_VideoTuningImgSchdlrCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMBA_IMG_SCHDLR_EVENT_CFA_STAT_READY:
            if (VideoTuning3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspCfa_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_RGB_STAT_READY:
            if (VideoTuning3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_MAIN_CFA_HIST_READY:
            if (VideoTuning3AEnable) {
//                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                //Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_HDR_CFA_HIST_READY:
            if (VideoTuning3AEnable) {
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

/**
 * VIN sensor mode switch callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_VideoTuningVinSwitchCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {

        case AMP_VIN_EVENT_INVALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_INVALID info: %X", info);
            if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE && \
                TuningLiveViewProcMode == 1) {
                //nothing to do
            } else {
                if (MainVideoTuningImgSchdlr) AmbaImgSchdlr_Enable(MainVideoTuningImgSchdlr, 0);
                if (PipVideoTuningImgSchdlr) AmbaImgSchdlr_Enable(PipVideoTuningImgSchdlr, 0);
                Amba_Img_VIn_Invalid(hdlr, (UINT32 *)NULL);
            }
            break;
        case AMP_VIN_EVENT_VALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_VALID info: %X", info);
            if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE && \
                TuningLiveViewProcMode == 1) {
                //nothing to do
            } else {
                if (VideoTuning3AEnable) {
                    Amba_Img_VIn_Valid(hdlr, (UINT32 *)NULL);
                    if (MainVideoTuningImgSchdlr) AmbaImgSchdlr_Enable(MainVideoTuningImgSchdlr, 1);
                    if (PipVideoTuningImgSchdlr) AmbaImgSchdlr_Enable(PipVideoTuningImgSchdlr, 1);
                }
            }
            break;
        case AMP_VIN_EVENT_CHANGED_PRIOR:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_PRIOR info: %X", info);
            if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE && \
                TuningLiveViewProcMode == 1) {
                //nothing to do
            } else {
                if (VideoTuning3AEnable) {
                    Amba_Img_VIn_Changed_Prior(hdlr, (UINT32 *)NULL);
                }
            }
        #ifdef CONFIG_SOC_A12
            if (TuningStatus == TUNING_STATUS_LIVEVIEW) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window = {0}, DefWindow = {0};

                window.Source = AMP_DISP_ENC;
                window.CropArea.Width = 0;
                window.CropArea.Height = 0;
                window.CropArea.X = 0;
                window.CropArea.Y = 0;
                window.TargetAreaOnPlane.Width = 960;
                window.TargetAreaOnPlane.Height = (VideoTuningMgt[TuningModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                window.TargetAreaOnPlane.X = 00;
                window.TargetAreaOnPlane.Y = (480 - window.TargetAreaOnPlane.Height)/2;
                if(AmpUT_Display_GetWindowCfg(0, &DefWindow) != OK) {
                    AmpUT_Display_Window_Create(0, &window);
                } else {
                    AmpUT_Display_SetWindowCfg(0, &window);
                }
                if (TuningLCDLiveview) {
                    AmpUT_Display_Act_Window(0);
                } else {
                    AmpUT_Display_DeAct_Window(0);
                }
            }
            if (TuningStatus == TUNING_STATUS_LIVEVIEW && \
                TuningTvLiveview) {
                AMP_DISP_WINDOW_CFG_s window = {0}, DefWindow = {0};

                window.Source = AMP_DISP_ENC;
                window.CropArea.Width = 0;
                window.CropArea.Height = 0;
                window.CropArea.X = 0;
                window.CropArea.Y = 0;
                window.TargetAreaOnPlane.Width = (VideoTuningMgt[TuningModeIdx].AspectRatio == VAR_16x9)? 1920: 1440;
                window.TargetAreaOnPlane.Height = 1080;
                window.TargetAreaOnPlane.X = (1920 - window.TargetAreaOnPlane.Width)/2;
                window.TargetAreaOnPlane.Y = 0;
                AmpUT_Display_SetWindowCfg(1, &window);
                if(AmpUT_Display_GetWindowCfg(1, &DefWindow) != OK) {
                    AmpUT_Display_Window_Create(1, &window);
                } else {
                    AmpUT_Display_SetWindowCfg(1, &window);
                }
                if (TuningTvLiveview) {
                    AmpUT_Display_Act_Window(1);
                } else {
                    AmpUT_Display_DeAct_Window(1);
                }
            }
        #endif
            break;
        case AMP_VIN_EVENT_CHANGED_POST:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_POST info: %X", info);
            if (VideoTuning3AEnable) {
                extern void Amba_AeAwbAdj_Init(UINT32 chNo,UINT8 initFlg, AMBA_KAL_BYTE_POOL_t *pMMPL);
                UINT8 IsPhotoLiveView = 0;
                UINT16 PipeMode = IP_EXPERSS_MODE;
                UINT16 AlgoMode = IP_MODE_LISO_VIDEO;
                UINT32 ChNo = (UINT32)TuningEncChannel.Bits.VinID;
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo;
                AMBA_SENSOR_MODE_ID_u SensorMode = {0};


                SensorMode.Data = (TuningEncodeSystem==0)? VideoTuningMgt[TuningModeIdx].InputMode: VideoTuningMgt[TuningModeIdx].InputPALMode;
                AmbaSensor_GetModeInfo(TuningEncChannel, SensorMode, &SensorModeInfo);

                if (MainVideoTuningImgSchdlr) {
                    AMBA_IMG_SCHDLR_UPDATE_CFG_s SchdlrCfg = {0};

                    AmbaImgSchdlr_GetConfig(MainVideoTuningImgSchdlr, &SchdlrCfg);
                    if (TuningLiveViewProcMode && TuningLiveViewAlgoMode) {
                        SchdlrCfg.VideoProcMode = 1;
                        if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                            SchdlrCfg.VideoProcMode |= 0x10;
                        }
                    }
                    AmbaImgSchdlr_SetConfig(MainVideoTuningImgSchdlr, &SchdlrCfg);  // One MainViewID (not vin) need one scheduler.
                }

        #ifdef CONFIG_SOC_A12
            { // Load IQ params
                extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
                UINT8 IsSensorHdrMode;

                // Inform 3A LV sensor mode is Hdr or not
                IsSensorHdrMode = (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE)? 1: 0;
                AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_HDR_ENABLE, ChNo, (UINT32)IsSensorHdrMode, 0);

                if (VideoTuningIsIqParamInit == 0 || IsSensorHdrMode != VideoTuningIsHdrIqParam) {
                    if (IsSensorHdrMode == 0) {
                        App_Image_Init_Iq_Params(ChNo, InputDeviceId);
                    } else if (IsSensorHdrMode == 1) {
                        App_Image_Init_Iq_Params(0, InputDeviceId);
                        App_Image_Init_Iq_Params(1, InputDeviceId);
                    }
                    VideoTuningIsIqParamInit = 1;
                    VideoTuningIsHdrIqParam = IsSensorHdrMode;
                }
            }
        #endif

                //inform 3A LiveView pipeline
                if (TuningLiveViewProcMode && TuningLiveViewAlgoMode) {
                    PipeMode = IP_HYBRID_MODE;
                } else {
                    PipeMode = IP_EXPERSS_MODE;
                }
                AmbaImg_Proc_Cmd(MW_IP_SET_PIPE_MODE, ChNo, (UINT32)PipeMode, 0);

                //inform 3A LiveView Algo
                if (TuningLiveViewAlgoMode == 0 || TuningLiveViewAlgoMode == 1) {
                    AlgoMode = IP_MODE_LISO_VIDEO;
                } else {
                    AlgoMode = IP_MODE_HISO_VIDEO;
                }
                AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_ALGO_MODE, ChNo, (UINT32)AlgoMode, 0);

                //inform 3A LiveView type
                AmbaImg_Proc_Cmd(MW_IP_SET_PHOTO_PREVIEW, (UINT32)&IsPhotoLiveView, 0, 0);

                //inform 3A LiveView info
                {
                    LIVEVIEW_INFO_s LiveViewInfo = {0};
                    AMBA_SENSOR_STATUS_INFO_s SsrStatus = {0};
                    AMBA_SENSOR_INPUT_INFO_s *InputInfo = &SsrStatus.ModeInfo.InputInfo;

                    LiveViewInfo.OverSamplingEnable = 1; //HybridMode use OverSampling
                    LiveViewInfo.MainW = VideoTuningMgt[TuningModeIdx].MainWidth;
                    LiveViewInfo.MainH = VideoTuningMgt[TuningModeIdx].MainHeight;
                    if (TuningEncodeSystem == 0) {
                        LiveViewInfo.FrameRateInt = VideoTuningMgt[TuningModeIdx].TimeScale/VideoTuningMgt[TuningModeIdx].TickPerPicture;
                    } else {
                        LiveViewInfo.FrameRateInt = VideoTuningMgt[TuningModeIdx].TimeScalePAL/VideoTuningMgt[TuningModeIdx].TickPerPicturePAL;
                    }
                    LiveViewInfo.FrameRateInt = UT_VideoTuningFrameRateIntConvert(LiveViewInfo.FrameRateInt);
                    AmbaSensor_GetStatus(TuningEncChannel, &SsrStatus);
                    LiveViewInfo.BinningHNum = InputInfo->HSubsample.FactorNum;
                    LiveViewInfo.BinningHDen = InputInfo->HSubsample.FactorDen;
                    LiveViewInfo.BinningVNum = InputInfo->VSubsample.FactorNum;
                    LiveViewInfo.BinningVDen = InputInfo->VSubsample.FactorDen;
                    AmbaImg_Proc_Cmd(MW_IP_SET_LIVEVIEW_INFO, ChNo, (UINT32)&LiveViewInfo, 0);
                }

                //inform 3A FrameRate info
                {
                    UINT32 FrameRate = 0, FrameRatex1000 = 0;

                    if (TuningEncodeSystem == 0) {
                        FrameRate = VideoTuningMgt[TuningModeIdx].TimeScale/VideoTuningMgt[TuningModeIdx].TickPerPicture;
                        FrameRatex1000 = VideoTuningMgt[TuningModeIdx].TimeScale*1000/VideoTuningMgt[TuningModeIdx].TickPerPicture;
                    } else {
                        FrameRate = VideoTuningMgt[TuningModeIdx].TimeScalePAL/VideoTuningMgt[TuningModeIdx].TickPerPicturePAL;
                        FrameRatex1000 = VideoTuningMgt[TuningModeIdx].TimeScalePAL*1000/VideoTuningMgt[TuningModeIdx].TickPerPicturePAL;
                    }
                    FrameRate = UT_VideoTuningFrameRateIntConvert(FrameRate);
                    AmbaImg_Proc_Cmd(MW_IP_SET_FRAME_RATE, ChNo, FrameRate, FrameRatex1000);
                }

            #ifdef CONFIG_SOC_A12 // Temporary disable for A9
                Amba_AeAwbAdj_Init(ChNo, 0, &G_MMPL);
            #endif
                Amba_Img_VIn_Changed_Post(hdlr, &ChNo);
            }

            if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE && \
                TuningLiveViewProcMode == 1) {
                /* Do what VIN_CHANGE_POST do */
                UINT32 MainViewID = TuningEncChannel.Bits.VinID;
                AMBA_ITUNER_PROC_INFO_s ProcInfo = {0};
                AMBA_IMG_SCHDLR_EXP_s ExpInfo = {0};
                AMBA_IMG_SCHDLR_EXP_INFO_s CurExpInfo = {0};
            #ifdef CONFIG_SOC_A12
                ITUNER_VIDEO_HDR_RAW_INFO_s HdrRawInfo = {0};
            #endif

                AmbaPrint("Ituner Execute");
                if (0 != AmbaTUNE_Execute_IDSP(&ImgMode, &ProcInfo)) {
                    AmbaPrint("Call AmbaTUNE_Execute_IDSP() Fail");
                }

                AmbaImgSchdlr_GetExposureInfo(MainViewID, &CurExpInfo);
                memcpy(&ExpInfo.Info, &CurExpInfo, sizeof(AMBA_IMG_SCHDLR_EXP_INFO_s));

                ExpInfo.Type = AMBA_IMG_SCHDLR_SET_TYPE_DIRECT;
                ExpInfo.Info.AdjUpdated = 1;
                if (ItunerInfo.TuningModeExt == SINGLE_SHOT_MULTI_EXPOSURE_HDR) {
                #ifdef CONFIG_SOC_A12
                    AmbaItuner_Get_VideoHdrRawInfo(&HdrRawInfo);
                    ExpInfo.Info.SubChannelNum = HdrRawInfo.NumberOfExposures - 1;
                #endif
                    ExpInfo.Info.HdrUpdated[0].Data = 0xFFFF;
                }
                memcpy(&ExpInfo.Info.Mode, &ImgMode, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
                AmbaImgSchdlr_SetExposure(MainViewID, &ExpInfo);
            }
        #ifdef CONFIG_SOC_A9
            if (TuningStatus == TUNING_STATUS_LIVEVIEW && \
                EncLcdWinHdlr) {
                // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window;

                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
                window.Source = AMP_DISP_ENC;
                window.CropArea.Width = 0;
                window.CropArea.Height = 0;
                window.CropArea.X = 0;
                window.CropArea.Y = 0;
                window.TargetAreaOnPlane.Width = 960;
                window.TargetAreaOnPlane.Height = (VideoTuningMgt[TuningModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                window.TargetAreaOnPlane.X = 00;
                window.TargetAreaOnPlane.Y = (480 - window.TargetAreaOnPlane.Height)/2;
                AmpDisplay_SetWindowCfg(EncLcdWinHdlr, &window);
                if (TuningLCDLiveview) {
                    AmpDisplay_SetWindowActivateFlag(EncLcdWinHdlr, 1);
                } else {
                    AmpDisplay_SetWindowActivateFlag(EncLcdWinHdlr, 0);
                }
                AmpDisplay_Update(LCDHdlr);
            }
            if (TuningStatus == TUNING_STATUS_LIVEVIEW && \
                EncTvWinHdlr && \
                TuningTvLiveview) {
                AMP_DISP_WINDOW_CFG_s window;

                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
                window.Source = AMP_DISP_ENC;
                window.CropArea.Width = 0;
                window.CropArea.Height = 0;
                window.CropArea.X = 0;
                window.CropArea.Y = 0;
                window.TargetAreaOnPlane.Width = (VideoTuningMgt[TuningModeIdx].AspectRatio == VAR_16x9)? 1920: 1440;
                window.TargetAreaOnPlane.Height = 1080;
                window.TargetAreaOnPlane.X = (1920 - window.TargetAreaOnPlane.Width)/2;
                window.TargetAreaOnPlane.Y = 0;
                AmpDisplay_SetWindowCfg(EncTvWinHdlr, &window);
                if (TuningTvLiveview) {
                    AmpDisplay_SetWindowActivateFlag(EncTvWinHdlr, 1);
                } else {
                    AmpDisplay_SetWindowActivateFlag(EncTvWinHdlr, 0);
                }
                AmpDisplay_Update(TVHdlr);
            }
        #endif
            break;
        default:
            AmbaPrint("VinSWCB: Unknown %X info: %x", event, info);
           break;
    }
    return 0;
}

/**
 * Generic VIN event callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
//static int xcnt = 0, ycnt=0; // Just to reduce console print
static int AmpUT_VideoTuningVinEventCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_VIN_EVENT_FRAME_READY:
            break;
        case AMP_VIN_EVENT_FRAME_DROPPED:
            break;
        default:
            AmbaPrint("VinEVCB: Unknown %X info: %x", event, info);
           break;
    }
    return 0;
}

/**
 * Generic VideoTuning codec callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_VideoTuningCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_START:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_DONE:
        {
            AmbaPrint("AMP_ENC_EVENT_RAW_CAPTURE_DONE (%d)", VideoTuningRawCapRunning);
        }
            break;
        case AMP_ENC_EVENT_BACK_TO_LIVEVIEW:
            break;
        case AMP_ENC_EVENT_BG_PROCESS_DONE:
        {
            AmbaPrintColor(BLUE, "[AMP_UT] STILL_SCRIPT_PROCESS_DONE");
            if (VideoTuningRawCapRunning == 1) {
                VideoTuningRawCapRunning = 0;
                if (LvCapture) {
                    FnoLvCap++;
                    LvCapture = 0;
                    VideoTuningCaptureMode = TUNNING_MODE_CAPTURE_NONE;

                    if (VideoTuningDumpRawSkip == 0 && IsFastCapture == 1) {
                        UINT8 i;
                        AMP_CFS_FILE_s *Raw = NULL;
                        char Fn[64];
                        char MdASCII[3] = {'w','+','\0'};
                        UINT16 RawHeight = VideoTuningMgt[TuningModeIdx].CaptureHeight;
                        UINT16 RawWidth = VideoTuningMgt[TuningModeIdx].CaptureWidth;
                        UINT16 RawPitch = (IsRawCompr)? AMP_COMPRESSED_RAW_WIDTH(RawWidth): RawWidth*2;
                        for (i=0; i<VideoTuningRawIndex; i++) {
                            sprintf(Fn, "%s:\\LvCap_%04d_%04d.RAW", DefaultSlot, FnoLvCap, SubRawFnoLvCap);
                            Raw = UT_VideoTuningfopen((const char *)Fn, (const char *)MdASCII);
                            AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d  to %s", \
                                    VideoTuningRawAddr[i], RawPitch, RawHeight, Fn);
                            UT_VideoTuningfwrite((const void *)VideoTuningRawAddr[i], 1, RawPitch*RawHeight, (void *)Raw);
                            UT_VideoTuningfsync((void *)Raw);
                            UT_VideoTuningfclose((void *)Raw);
                            SubRawFnoLvCap++;
                        }
                        VideoTuningRawIndex = 0;
                    }

                    AmpStillEnc_EnableLiveviewCapture(AMP_STILL_PREPARE_TO_VIDEO_LIVEVIEW, 0);
                    AmpUT_VideoTuning_LiveviewStart(TuningModeIdx);
                #ifdef CONFIG_SOC_A12
                    IsTurnOnLcdQV = 0;
                #endif
                    AmbaPrintColor(BLUE, "[AMP_UT] Back2Liveview: Capture mode %d", VideoTuningCaptureMode);
                }
            }
        }
            break;
        case AMP_ENC_EVENT_LIVEVIEW_RAW_READY:
            break;
        case AMP_ENC_EVENT_LIVEVIEW_DCHAN_YUV_READY:
            break;
        case AMP_ENC_EVENT_LIVEVIEW_FCHAN_YUV_READY:
            break;
        case AMP_ENC_EVENT_QUICKVIEW_DCHAN_YUV_READY:
        {
#ifdef CONFIG_SOC_A12
            if (IsTurnOnLcdQV == 0) {
                AMP_DISP_WINDOW_CFG_s Window;
                IsTurnOnLcdQV = 1;

                if (1) {
                    memset(&Window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
                    Window.Source = AMP_DISP_ENC;
                    Window.SourceDesc.Enc.VinCh = TuningEncChannel;
                    Window.CropArea.Width = 0;
                    Window.CropArea.Height = 0;
                    Window.CropArea.X = 0;
                    Window.CropArea.Y = 0;
                    Window.TargetAreaOnPlane.Width = 960;
                    Window.TargetAreaOnPlane.Height = (VideoTuningMgt[TuningModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                    Window.TargetAreaOnPlane.X = 0;
                    Window.TargetAreaOnPlane.Y = (480 - Window.TargetAreaOnPlane.Height)/2;
                    AmpUT_Display_SetWindowCfg(0, &Window);
                    AmpUT_Display_Act_Window(0);
                    AmbaPrint("[AmpUT_VideoTuning] QV_DCHAN_YUV_READY, switch to VDSRC");
                } else {
                    AmbaPrint("[AmpUT_VideoTuning] ERROR, A12 MUST turn on VDSRC as soon as received QV_FCHAN_YUV_READY");
                }
            } else {
                // AmbaPrint("[AmpUT_VideoTuning] QV_DCHAN_YUV_READY, Already switch to VDSRC");
            }
#endif
        }
            break;
        case AMP_ENC_EVENT_QUICKVIEW_FCHAN_YUV_READY:
        {
#ifdef CONFIG_SOC_A12
            AMP_DISP_WINDOW_CFG_s Window;
            if (1) {
                memset(&Window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
                Window.Source = AMP_DISP_ENC;
                Window.SourceDesc.Enc.VinCh = TuningEncChannel;
                Window.CropArea.Width = 0;
                Window.CropArea.Height = 0;
                Window.CropArea.X = 0;
                Window.CropArea.Y = 0;
                Window.TargetAreaOnPlane.Width = (VideoTuningMgt[TuningModeIdx].AspectRatio == VAR_16x9)?1920:1440;
                Window.TargetAreaOnPlane.Height = 1080;
                Window.TargetAreaOnPlane.X = (1920 - Window.TargetAreaOnPlane.Width)/2;
                Window.TargetAreaOnPlane.Y = 0;
                AmpUT_Display_SetWindowCfg(1, &Window);
                AmpUT_Display_Act_Window(1);
                AmbaPrint("[AmpUT_VideoTuning] QV_FCHAN_YUV_READY, switch to VDSRC");
            } else {
                AmbaPrint("[AmpUT_VideoTuning] ERROR, A12 MUST turn on VDSRC as soon as received QV_FCHAN_YUV_READY");
            }
#endif
        }
            break;
        case AMP_ENC_EVENT_VCAP_YUV_READY:
            {
                if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE) {

                    if (DumpYuvEnable) {
                        UINT32 Size = 0;
                        AMP_ENC_YUV_INFO_s *YuvInfo = info;
                        char Fn[64];
                        char MdASCII[3] = {'w','b','\0'};
                        AMP_CFS_FILE_s *y = NULL;
                        AMP_CFS_FILE_s *uv = NULL;

                        /* Dump yuv */
                        sprintf(Fn,"%s:\\LvCap_%04d_%04d.y", DefaultSlot, (int)VideoRawEncVar.currBinaryIndex, YuvFno);
                        y = UT_VideoTuningfopen((const char *)Fn, (const char *)MdASCII);

                        Size = YuvInfo->pitch*YuvInfo->height;
                        AmbaPrint("[Amp_UT]Dump Y(0x%X)(%d) to %s", YuvInfo->yAddr, Size, Fn);
                        AmbaCache_Invalidate((void *)YuvInfo->yAddr, Size);
                        UT_VideoTuningfwrite((const void *)YuvInfo->yAddr, 1, Size, (void *)y);
                        UT_VideoTuningfsync((void *)y);
                        UT_VideoTuningfclose((void *)y);

                        sprintf(Fn,"%s:\\LvCap_%04d_%04d.uv", DefaultSlot, (int)VideoRawEncVar.currBinaryIndex, YuvFno);
                        uv = UT_VideoTuningfopen((const char *)Fn, (const char *)MdASCII);
                        Size = (YuvInfo->colorFmt == AMP_YUV_420)? (YuvInfo->pitch*YuvInfo->height)>>1: YuvInfo->pitch*YuvInfo->height;
                        AmbaPrint("[Amp_UT]Dump UV(0x%X)(%d) to %s", YuvInfo->uvAddr, Size, Fn);
                        AmbaCache_Invalidate((void *)YuvInfo->uvAddr, Size);
                        UT_VideoTuningfwrite((const void *)YuvInfo->uvAddr, 1, Size, (void *)uv);
                        UT_VideoTuningfsync((void *)uv);
                        UT_VideoTuningfclose((void *)uv);
                    }
                    YuvFno++;
                }
            }
            break;
        case AMP_ENC_EVENT_VCAP_2ND_YUV_READY:
            //AmbaPrint("AmpUT_VideoTuning: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_2ND_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VCAP_ME1_Y_READY:
            if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE) {
                if (DumpMeYEnable) {
                    UINT32 Size = 0;
                    AMP_ENC_YUV_INFO_s *YuvInfo = info;
                    char Fn[MAX_FILENAME_LENGTH];
                    char MdASCII[3] = {'w','b','\0'};
                    AMP_CFS_FILE_s *y = NULL;

                    /* Dump yuv */
                    sprintf(Fn,"%s:\\LvCap_%04d_%04d_me.y", DefaultSlot, (int)VideoRawEncVar.currBinaryIndex, MeYFno);
                    y = UT_VideoTuningfopen((const char *)Fn, (const char *)MdASCII);

                    Size = YuvInfo->pitch*YuvInfo->height;
                    AmbaPrint("[Amp_UT]Dump MeY(0x%X)(%d) to %s", YuvInfo->yAddr, Size, Fn);
                    AmbaCache_Invalidate((void *)YuvInfo->yAddr, Size);
                    UT_VideoTuningfwrite((const void *)YuvInfo->yAddr, 1, Size, (void *)y);
                    UT_VideoTuningfsync((void *)y);
                    UT_VideoTuningfclose((void *)y);
                }
                MeYFno++;
            }
            break;
        case AMP_ENC_EVENT_HYBRID_YUV_READY:
            //AmbaPrint("AmpUT_VideoTuning: !!!!!!!!!!! AMP_ENC_EVENT_HYBRID_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VDSP_ASSERT:
            //AmbaPrintColor(RED, "AmpUT_VideoTuning: !!!!!!!!!!! AMP_ENC_EVENT_VDSP_ASSERT !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_VideoTuning: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            AmpUT_VideoTuning_EncStop();
            TuningStatus = TUNING_STATUS_LIVEVIEW;
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_VideoTuning: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        default:
            AmbaPrint("AmpUT_VideoTuning: Unknown %X info: %x", event, info);
            break;
    }
    return 0;
}

/**
 * Pipeline callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_VideoTuningPipeCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            {
                AMP_ENC_STATE_CHANGED_INFO_s *inf = info;

                AmbaPrint("PipeCB: Pipe[%X] AMP_ENC_EVENT_STATE_CHANGED newState %X", hdlr, inf->newState);
            }
            break;
    }
    return 0;

}

/**
 * Display window initialization
 */
static int AmpUT_VideoTuning_DisplayStart(void)
{
    AMP_DISP_WINDOW_CFG_s window;
    /** Step 1: Display config & window config */
//    if (AmpUT_Display_Init() == NG) {
//        return NG;
//    }

    // Creat LCD Window
    if (1) {
        memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
        window.Source = AMP_DISP_ENC;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;
        window.TargetAreaOnPlane.Width = 960;
        if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE)
            window.TargetAreaOnPlane.Height = (VideoRawEncVar.rawBin.AspectRatio == VAR_16x9)? 360: 480;
        else
            window.TargetAreaOnPlane.Height = (VideoTuningMgt[TuningModeIdx].AspectRatio == VAR_16x9)? 360: 480;
        window.TargetAreaOnPlane.X = 0;
        window.TargetAreaOnPlane.Y = (480-window.TargetAreaOnPlane.Height)/2;
        window.SourceDesc.Enc.VinCh = TuningEncChannel;
        if (AmpUT_Display_Window_Create(0, &window) == NG) {
            return NG;
        }
    }

    // Creat TV Window
    if (1) {
        memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));
        window.Source = AMP_DISP_ENC;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;
        window.TargetAreaOnPlane.Width = 1920;
        window.TargetAreaOnPlane.Height = 1080;//  interlance should be consider in MW
        window.TargetAreaOnPlane.X = 0;
        window.TargetAreaOnPlane.Y = 0;
        window.SourceDesc.Enc.VinCh = TuningEncChannel;
        if (AmpUT_Display_Window_Create(1, &window) == NG) {
            return NG;
        }
    }

    if (1) {
        /** Step 2: Setup device */
        // Setup LCD & TV
        if (TuningLCDLiveview) {
            AmpUT_Display_Start(0);
        } else {
            AmpUT_Display_Stop(0);
        }
        if (TuningTvLiveview) {
            AmpUT_Display_Start(1);
        } else {
            AmpUT_Display_Stop(1);
        }
    }

    // Active Window 7002 cmd
    if (TuningLCDLiveview) {
        AmpUT_Display_Act_Window(0);
    } else {
        AmpUT_Display_DeAct_Window(0);
    }

    if (TuningTvLiveview) {
        AmpUT_Display_Act_Window(1);
    } else {
        AmpUT_Display_DeAct_Window(1);
    }

    return 0;
}

#define DZOOM_10X_TOTAL_STEP    (487)       /** 414+73 */
#define DZOOM_10X_MAX_RATIO     (654542)    /** 9.987518<<16 */
#define ZF_1X    (0x10000)

//Dzoom Step Factor Table
int VideoTuning_Dzoom_Step_Factor_Table(int Step, UINT32 *Factor)
{
    int i, type1CrossLimit;
    UINT32 width, tmpZF;
    UINT64 u64TmpZF;
    if (Step < 413) {
        /** Use type 2, detail in low X */
        width = ZF_1X;
        width -= (127)*Step;   /** 127 = (65536/512) - 1 */

        u64TmpZF = 0x100000000 / width; /** ZF_1X << 16 */
        tmpZF = u64TmpZF;
    } else {
        /** USe type 1, detail in high X */
        u64TmpZF = ZF_1X;
        type1CrossLimit = Step - 244;
        for (i=1; i<type1CrossLimit; i++) {
            u64TmpZF *= 66165;  /** 66165 = (120 ^ (1/512)) << 16 */
            u64TmpZF = u64TmpZF >> 16;
        }
        tmpZF = u64TmpZF;
    }
    // Get the dzoom factor under corresponding status
    *Factor = tmpZF;
    return 0;
}

void VideoTuning_AspectRatioGet(UINT16 Width, UINT16 Height, UINT16 *AspectRatio)
{
    UINT16 Ratio = 0;

    Ratio = (UINT16)(((UINT32)Height*1000)/Width);

    if (Ratio > 950) {
        *AspectRatio = VAR_1x1;
    } else if (Ratio > 700) {
        *AspectRatio = VAR_4x3;
    } else if (Ratio > 620) {
        *AspectRatio = VAR_3x2;
    } else if (Ratio > 580) {
        *AspectRatio = VAR_15x9;
    } else if (Ratio > 500) {
        *AspectRatio = VAR_16x9;
    }
}

/**
 *  Still Codec Initialization
 */
int AmpUT_VideoTuning_StillCodecInit(void)
{
    int Er=OK;
    void *TmpbufRaw = NULL;
    AMP_STILLENC_INIT_CFG_s EncInitCfg = {0};

    // Init STILLENC module
    AmpStillEnc_GetInitDefaultCfg(&EncInitCfg);
    if (VideoTuningSencWorkBuf == NULL) {
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoTuningSencWorkBuf, &TmpbufRaw, (UINT32)EncInitCfg.MemoryPoolSize, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for stillmain!!");
        }
    }
    EncInitCfg.MemoryPoolAddr = VideoTuningSencWorkBuf;
    AmpStillEnc_Init(&EncInitCfg);

    return Er;
}

#ifdef CONFIG_SOC_A12
static UINT8 TuningMjpegStandardQuantMatrix[128] = {      // Standard JPEG qualty 50 table.
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

/* Initial JPEG DQT */
void AmpUT_VidoeTuning_InitMJpegDqt(UINT8 *qTable, int quality)
{
    int i, scale, temp;

    /** for jpeg brc; return the quantization table*/
    if (quality == -1) {
        memcpy(qTable, TuningMjpegStandardQuantMatrix, 128);
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
        temp = ((long) TuningMjpegStandardQuantMatrix[i] * scale + 50L) / 100L;
        /* limit the values to the valid range */
        if (temp <= 0L) temp = 1L;
        if (temp > 255L) temp = 255L; /* max quantizer needed for baseline */
        qTable[i] = temp;
    }
}

static UINT8 TuningVideoPIVMainQuality = 90;              // PIV main Quality 0~100
static UINT8 TuningVideoPIVThmbQuality = 90;              // PIV thumbnail Quality 0~100
static UINT8 TuningVideoPIVScrnQuality = 90;              // PIV screennail Quality 0~100
UINT8 TuningVideoPIVQTable[3][128] = {
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

/**
 * UnitTest: Video Encode PIV PreProc Callback
 *
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoTuning_PivPRECB(AMP_STILLENC_PREP_INFO_s *info)
{
    extern void AmpUT_TuneGetItunerMode(UINT8 *ItunerMode);

    if (info->StageCnt == 3) {
        UINT8 ItunerMode = 0;

        AmpUT_TuneGetItunerMode(&ItunerMode);

        if (ItunerMode == 0/*ITUNER_BASIC_TUNING_MODE*/) {
            /* set still idsp param */
            if (TuningStillIso != 2 && VideoTuning3AEnable == 1){ // comes from AmbaSample_AdjPivControl()
                AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
                ADJ_STILL_CONTROL_s AdjPivCtrl = {0};
                float BaseStillBlcShtTime = 60.0;
                UINT16 ShutterIndex = 0;
                AMBA_AE_INFO_s VideoAeInfo;
                AMBA_DSP_IMG_WB_GAIN_s VideoWbGain = {WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN};

                ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
                ImgMode.AlgoMode = (TuningStillIso==1)? AMBA_DSP_IMG_ALGO_MODE_LISO: AMBA_DSP_IMG_ALGO_MODE_HISO;
                ImgMode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_PIV;
                ImgMode.ContextId = 0; //TBD
                if (TuningLiveViewProcMode == 0 || (TuningLiveViewProcMode == 1 && TuningLiveViewAlgoMode == 0)) {
                    ImgMode.BatchId = AMBA_DSP_STILL_LISO_FILTER;
                }

                /* Run Adj compute */
                AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, 0/*TBD*/, IP_MODE_VIDEO, (UINT32)&VideoAeInfo);
                AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, 0/*TBD*/, IP_MODE_VIDEO, (UINT32)&VideoWbGain);

                ShutterIndex = (UINT16)(log2(BaseStillBlcShtTime/VideoAeInfo.ShutterTime) * 128);
                AdjPivCtrl.StillMode = (TuningStillIso==1)? IP_MODE_LISO_STILL: IP_MODE_HISO_STILL;
                AdjPivCtrl.ShIndex = ShutterIndex;//stillAeInfo[aeIndx].ShutterIndex;
                AdjPivCtrl.EvIndex = VideoAeInfo.EvIndex;
                AdjPivCtrl.NfIndex = VideoAeInfo.NfIndex;
                AdjPivCtrl.WbGain = VideoWbGain;
                AdjPivCtrl.DZoomStep = 0;
                AdjPivCtrl.FlashMode = 0;
                AdjPivCtrl.LutNo = 0; //TBD

                AmbaPrintColor(GREEN, "[%s], WbGain, GainR : %5d, GainG : %5d,  GainB : %5d",
                    __FUNCTION__, VideoWbGain.GainR, VideoWbGain.GainG, VideoWbGain.GainB);
                AmbaPrintColor(GREEN, "PIV, StillMode : %d, ShIndex : %5d, EvIndex : %5d, NfIndex : %5d",
                        AdjPivCtrl.StillMode, AdjPivCtrl.ShIndex, AdjPivCtrl.EvIndex, AdjPivCtrl.NfIndex);

                AmbaImg_Proc_Cmd(MW_IP_ADJ_STILL_CONTROL, 0/*TBD*/ , (UINT32)&AdjPivCtrl , 0);
                AmbaImg_Proc_Cmd(MW_IP_SET_STILL_PIPE_CTRL_PARAMS, 0/*TBD*/ , (UINT32)&ImgMode , 0);
            }
        } else if (ItunerMode == 1/*ITUENR_PIV_VERIFIED_MODE*/) {
            AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
            AMBA_ITUNER_PROC_INFO_s ProcInfo = {0};
            AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
            static ITUNER_SYSTEM_s System; // prevent stack overflow
            memset(&System, 0x0, sizeof(ITUNER_SYSTEM_s));
            memset(&ImgMode, 0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
            memset(&SensorStatus, 0, sizeof(AMBA_SENSOR_STATUS_INFO_s));

            AmbaSensor_GetStatus(TuningEncChannel, &SensorStatus);
            AmbaItuner_Get_SystemInfo(&System);
            System.RawStartX = 0;
            System.RawStartY = 0;
            System.RawWidth = TuningPivCaptureWidth;
            System.RawHeight = TuningPivCaptureHeight;
            System.MainWidth = TuningPivMainWidth;
            System.MainHeight = TuningPivMainHeight;
            System.HSubSampleFactorNum = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorNum;
            System.HSubSampleFactorDen = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorDen;
            System.VSubSampleFactorNum = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorNum;
            System.VSubSampleFactorDen = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorDen;
            AmbaItuner_Set_SystemInfo(&System);

            ImgMode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_PIV;
            ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
            ImgMode.AlgoMode  = AMBA_DSP_IMG_ALGO_MODE_LISO;
            ImgMode.BatchId   = AMBA_DSP_STILL_LISO_FILTER;
            ImgMode.ContextId = 0;
            ImgMode.ConfigId  = 0;
            AmbaItuner_Execute(&ImgMode,&ProcInfo);
        } else {
            AmbaPrint("[AmpUT] Incorrect ituner mode %d", ItunerMode);
        }

    }

    return 0;
}

/**
 * UnitTest: PIV PostProc Callback
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoTuning_PivPOSTCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    static UINT8 yuvFlag = 0;

    if (info->StageCnt == 1) {
        yuvFlag = 0;
        TuningfnoRaw++;
    } else if (info->StageCnt == 2) {
        //release raw buffers
        if (RawBuffAddr) {
            if (AmbaKAL_BytePoolFree((void *)OriRawBuffAddr) != OK) {
                AmbaPrint("[Amp_UT] memFree Fail raw!");
            }
            RawBuffAddr = NULL;
        }

        TuningfnoYuv++;
    } else {
        //nothing to do
    }

    return 0;
}

/* CB for PIV raw capture */
UINT32 AmpUT_VideoTuning_PIV_RawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl)
{
    AMP_STILLENC_RAWCAP_DSP_CTRL_s DspCtrl = {0};
    DspCtrl.VidSkip = 0; //TBD
    DspCtrl.RawCapNum = ctrl->TotalRawToCap;
    DspCtrl.StillProc = TuningStillIso;
    AmpStillEnc_StartRawCapture(VideoTuningSencPri, &DspCtrl);

    return 0;
}

/**
 * UnitTest: simple PIV(ie rawcap from mem + raw2yuv + yuv2jpeg)
 *
 * @param [in] pivCtrl PIV control information
 * @param [in] iso iso mode
 * @param [in] cmpr compressed raw or not
 * @param [in] targetSize jpeg target Size in Kbyte unit
 * @param [in] encodeloop re-encode number
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_VideoTuning_PIV(AMP_VIDEOENC_PIV_CTRL_s pivCtrl, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop)
{
    int Er;
    void *TempPtr;
    void *TempRawPtr;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
    UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
    UINT32 QvLCDSize = 0, QvHDMISize = 0;
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt;
    AMP_SENC_SCRPT_RAWCAP_s *RawCapScrpt;
    AMP_SENC_SCRPT_RAW2YUV_s *Raw2YvuScrpt;
    AMP_SENC_SCRPT_YUV2JPG_s *Yuv2JpgScrpt;
    AMP_SCRPT_CONFIG_s Scrpt;
    AMBA_SENSOR_MODE_INFO_s SensorInfo;
    UINT16 ScrnWidth, ScrnHeight, ThmWidth, ThmHeight;

    TuningStillIso = iso;

    /* Pre-Phase */
    /* Error handle */
    if (TuningLiveViewProcMode == 0 && iso == 0) {
        AmbaPrint("Can not Support HISO-PIV @ ExpressMode!!");
        return NG;
    }

    /* Phase I */
    /* check still codec Status */
    if (VideoTuningSencInit == 0) { //not init yet
        if (AmpUT_VideoTuning_StillCodecInit() == OK) {
            VideoTuningSencInit = 1;
        }
    }

    if (VideoTuningSencPri == NULL) { //no codec be create
        AMP_STILLENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s Elayer = {0};

        EncCfg.MainLayout.Layer = &Elayer;
        AmpStillEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_VideoTuningCallback;

        // Assign bitstream/descriptor buffer
        // A12 express PIV will use H264 BS
        EncCfg.BitsBufCfg.BitsBufAddr = H264BitsBuf;
        EncCfg.BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
        EncCfg.BitsBufCfg.DescBufAddr = H264DescBuf;
        EncCfg.BitsBufCfg.DescBufSize = DESC_SIZE;
        EncCfg.BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
        AmpStillEnc_Create(&EncCfg, &VideoTuningSencPri);

        // create a virtual fifo
        if (VideoTuningVirtualJpegFifoHdlr == NULL) {
            AMP_FIFO_CFG_s FifoDefCfg = {0};

            AmpFifo_GetDefaultCfg(&FifoDefCfg);
            FifoDefCfg.hCodec = VideoTuningSencPri;
            FifoDefCfg.IsVirtual = 1;
            FifoDefCfg.NumEntries = 1024;
            FifoDefCfg.cbEvent = AmpUT_VideoTuning_FifoCB;
            AmpFifo_Create(&FifoDefCfg, &VideoTuningVirtualJpegFifoHdlr);
        }
    }

    //create pipeline
    if (VideoTuningSencPipe == NULL) {
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};

        AmpEnc_GetDefaultCfg(&PipeCfg);
        PipeCfg.encoder[0] = VideoTuningSencPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_VideoTuningPipeCallback;
        PipeCfg.type = AMP_ENC_STILL_PIPE;
        AmpEnc_Create(&PipeCfg, &VideoTuningSencPipe);

        AmpEnc_Add(VideoTuningSencPipe);
    }

    /* Phase II */
    /* fill script and run */
    if (VideoTuningRawCapRunning) {
        AmbaPrint("Error Status");
        goto _DONE;
    }

    if (TuningPivVideoThm == 0) {
        TuningPivProcess++;
    }

    AmbaSensor_GetModeInfo(TuningEncChannel, pivCtrl.SensorMode, &SensorInfo);
    TuningPivCaptureWidth = pivCtrl.CaptureWidth;
    TuningPivCaptureHeight = pivCtrl.CaptureHeight;
    TuningPivMainWidth = pivCtrl.MainWidth;
    TuningPivMainHeight = pivCtrl.MainHeight;
    ScrnWidth = 960;
    ThmWidth = 160;
    ThmHeight = 120;
    if (pivCtrl.AspectRatio == VAR_16x9) {
        ScrnHeight = 540;
        TuningScrnWidthAct = 960;
        TuningScrnHeightAct = 540;
        TuningThmWidthAct = 160;
        TuningThmHeightAct = 90;
        QvLCDW = 960;
        QvLCDH = 360;
        QvHDMIW = 1920;
        QvHDMIH = 1080;
    } else if (pivCtrl.AspectRatio == VAR_3x2) {
        ScrnHeight = 640;
        TuningScrnWidthAct = 960;
        TuningScrnHeightAct = 640;
        TuningThmWidthAct = 160;
        TuningThmHeightAct = 106;
        QvLCDW = 960;
        QvLCDH = 426;
        QvHDMIW = 1920;
        QvHDMIH = 1280;
    } else if (pivCtrl.AspectRatio == VAR_1x1) {
        ScrnHeight = 720;
        TuningScrnWidthAct = 720;
        TuningScrnHeightAct = 720;
        TuningThmWidthAct = 120;
        TuningThmHeightAct = 120;
        QvLCDW = 720;
        QvLCDH = 480;
        QvHDMIW = 1080;
        QvHDMIH = 1080;
    } else { //4:3
        ScrnHeight = 720;
        TuningScrnWidthAct = 960;
        TuningScrnHeightAct = 720;
        TuningThmWidthAct = 160;
        TuningThmHeightAct = 120;
        QvLCDW = 960;
        QvLCDH = 480;
        QvHDMIW = 1440;
        QvHDMIH = 1080;
    }

    /* Step1. calc raw and yuv buffer memory */
    RawPitch = (cmpr)? \
        AMP_COMPRESSED_RAW_WIDTH(TuningPivCaptureWidth): \
        TuningPivCaptureWidth*2;
    RawPitch = ALIGN_32(RawPitch);
    RawWidth =  TuningPivCaptureWidth;
    RawHeight = TuningPivCaptureHeight;
    RawSize = RawPitch*RawHeight;
    AmbaPrint("[UT_videoEncPIV]raw(%u %u %u)", RawPitch, RawWidth, RawHeight);

    //FastMode need 16_align enc_height
    if (iso == 2) {
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32(TuningPivMainWidth);
        YuvHeight = ALIGN_16(TuningPivMainHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        AmbaPrint("[UT_videoEncPIV]yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);
        ScrnW = ALIGN_32(ScrnWidth);
        ScrnH = ALIGN_16(ScrnHeight);
        ScrnSize = ScrnW*ScrnH*2;
        ThmW = ALIGN_32(ThmWidth);
        ThmH = ALIGN_16(ThmHeight);
        ThmSize = ThmW*ThmH*2;
        AmbaPrint("[UT_videoEncPIV]scrn(%d %d %u) thm(%d %d %u)!", \
            ScrnW, ScrnH, ScrnSize, \
            ThmW, ThmH, ThmSize);
    } else {
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32(TuningPivMainWidth);
        YuvHeight = ALIGN_16(TuningPivMainHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        YuvSize += (YuvSize*10)/100;
        AmbaPrint("[UT_videoEncPIV]yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);
        ScrnW = ALIGN_32(ScrnWidth);
        ScrnH = ALIGN_16(ScrnHeight);
        ScrnSize = ScrnW*ScrnH*2;
        ScrnSize += (ScrnSize*10)/100;
        ThmW = ALIGN_32(ThmWidth);
        ThmH = ALIGN_16(ThmHeight);
        ThmSize = ThmW*ThmH*2;
        ThmSize += (ThmSize*10)/100;
        AmbaPrint("[UT_videoEncPIV]scrn(%d %d %u) thm(%d %d %u)!", \
            ScrnW, ScrnH, ScrnSize, \
            ThmW, ThmH, ThmSize);
    }
    /* QV need 16_Align */
    QvLCDSize = QvLCDW*QvLCDH*2;
    QvLCDSize += (QvLCDSize*15)/100;
    QvHDMISize = QvHDMIW*QvHDMIH*2;
    QvHDMISize += (QvHDMISize*15)/100;

    AmbaPrint("[UT_videoEncPIV]qvLCD(%u) qvHDMI(%u)!", QvLCDSize, QvHDMISize);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    //In Mode_0 PIV, APP do not need to provide buffer memory
    RawBuffAddr = 0;
    YuvBuffAddr = ScrnBuffAddr = ThmBuffAddr = 0;

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, QvLCDSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_videoEncPIV]DDR alloc yuv_lcd fail (%u)!", QvLCDSize*1);
    } else {
        QvLCDBuffAddr = (UINT8*)TempPtr;
        OriQvLCDBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_videoEncPIV]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, QvHDMISize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_videoEncPIV]DDR alloc yuv_hdmi fail (%u)!", QvHDMISize*1);
    } else {
        QvHDMIBuffAddr = (UINT8*)TempPtr;
        OriQvHDMIBuffAddr = (UINT8*)TempRawPtr;
        AmbaPrint("[UT_videoEncPIV]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);
    }
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempRawPtr, ScriptSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_videoEncPIV]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)TempPtr;
            OriScriptAddr = (UINT8*)TempRawPtr;
            AmbaPrint("[UT_videoEncPIV]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
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

    GenScrpt->QVConfig.DisableLCDQV = 1;
    GenScrpt->QVConfig.DisableHDMIQV = 1;
    GenScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    GenScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + QvLCDSize/2;
    GenScrpt->QVConfig.LCDWidth = QvLCDW;
    GenScrpt->QVConfig.LCDHeight = QvLCDH;
    GenScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    GenScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + QvHDMISize/2;
    GenScrpt->QVConfig.HDMIWidth = QvHDMIW;
    GenScrpt->QVConfig.HDMIHeight = QvHDMIH;

    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    GenScrpt->ScrnEnable = 1;
    GenScrpt->ThmEnable = 1;

    GenScrpt->MainBuf.ColorFmt = AMP_YUV_420;
    GenScrpt->MainBuf.Width = GenScrpt->MainBuf.Pitch = YuvWidth;
    GenScrpt->MainBuf.Height = YuvHeight;
    GenScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    GenScrpt->MainBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->MainBuf.AOI.X = 0;
    GenScrpt->MainBuf.AOI.Y = 0;
    GenScrpt->MainBuf.AOI.Width = TuningPivMainWidth;
    GenScrpt->MainBuf.AOI.Height = TuningPivMainHeight;
    GenScrpt->ScrnBuf.ColorFmt = AMP_YUV_420;
    GenScrpt->ScrnBuf.Width = GenScrpt->ScrnBuf.Pitch = ScrnW;
    GenScrpt->ScrnBuf.Height = ScrnH;
    GenScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    GenScrpt->ScrnBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->ScrnBuf.AOI.X = 0;
    GenScrpt->ScrnBuf.AOI.Y = 0;
    GenScrpt->ScrnBuf.AOI.Width = TuningScrnWidthAct;
    GenScrpt->ScrnBuf.AOI.Height = TuningScrnHeightAct;
    GenScrpt->ScrnWidth = ScrnWidth;
    GenScrpt->ScrnHeight = ScrnHeight;
    GenScrpt->ThmBuf.ColorFmt = AMP_YUV_420;
    GenScrpt->ThmBuf.Width = GenScrpt->ThmBuf.Pitch = ThmW;
    GenScrpt->ThmBuf.Height = ThmH;
    GenScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    GenScrpt->ThmBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->ThmBuf.AOI.X = 0;
    GenScrpt->ThmBuf.AOI.Y = 0;
    GenScrpt->ThmBuf.AOI.Width = TuningThmWidthAct;
    GenScrpt->ThmBuf.AOI.Height = TuningThmHeightAct;
    GenScrpt->ThmWidth = ThmWidth;
    GenScrpt->ThmHeight = ThmHeight;

    if (targetSize) {
        extern UINT32 AmpUT_JpegBRCPredictCB(UINT16 targetBpp, UINT32 stillProc, UINT8* predictQ, UINT8 *curveNum, UINT32 *curveAddr);
        AmbaPrint("[UT_videoTuningPIV]Target Size %u Kbyte", targetSize);
        AmpUT_VidoeTuning_InitMJpegDqt(TuningVideoPIVQTable[0], -1);
        AmpUT_VidoeTuning_InitMJpegDqt(TuningVideoPIVQTable[1], -1);
        AmpUT_VidoeTuning_InitMJpegDqt(TuningVideoPIVQTable[2], -1);
        GenScrpt->BrcCtrl.Tolerance = 10;
        GenScrpt->BrcCtrl.MaxEncLoop = encodeLoop;
        GenScrpt->BrcCtrl.JpgBrcCB = AmpUT_JpegBRCPredictCB;
        GenScrpt->BrcCtrl.TargetBitRate = \
           (((targetSize<<13)/TuningPivMainWidth)<<12)/TuningPivMainHeight;
        GenScrpt->BrcCtrl.MainQTAddr = TuningVideoPIVQTable[0];
        GenScrpt->BrcCtrl.ThmQTAddr = TuningVideoPIVQTable[1];
        GenScrpt->BrcCtrl.ScrnQTAddr = TuningVideoPIVQTable[2];
    } else {
        AmpUT_VidoeTuning_InitMJpegDqt(TuningVideoPIVQTable[0], TuningVideoPIVMainQuality);
        AmpUT_VidoeTuning_InitMJpegDqt(TuningVideoPIVQTable[1], TuningVideoPIVThmbQuality);
        AmpUT_VidoeTuning_InitMJpegDqt(TuningVideoPIVQTable[2], TuningVideoPIVScrnQuality);
        GenScrpt->BrcCtrl.Tolerance = 0;
        GenScrpt->BrcCtrl.MaxEncLoop = 0;
        GenScrpt->BrcCtrl.JpgBrcCB = NULL;
        GenScrpt->BrcCtrl.TargetBitRate = 0;
        GenScrpt->BrcCtrl.MainQTAddr = TuningVideoPIVQTable[0];
        GenScrpt->BrcCtrl.ThmQTAddr = TuningVideoPIVQTable[1];
        GenScrpt->BrcCtrl.ScrnQTAddr = TuningVideoPIVQTable[2];
    }

    GenScrpt->PostProc = &post_videoTuningPiv_cb;
    GenScrpt->PreProc = &pre_videoTuningPiv_cb;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoEncPIV]Stage_0 0x%X", StageAddr);

    //raw cap config
    StageAddr = ScriptAddr + TotalScriptSize;
    RawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)StageAddr;
    memset(RawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    RawCapScrpt->Cmd = SENC_RAWCAP;
    RawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    RawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    RawCapScrpt->SensorMode = pivCtrl.SensorMode;
    RawCapScrpt->FvRawCapArea.VcapWidth = TuningPivCaptureWidth;
    RawCapScrpt->FvRawCapArea.VcapHeight = TuningPivCaptureHeight;
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
    RawCapScrpt->CapCB.RawCapCB = AmpUT_VideoTuning_PIV_RawCapCB;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoEncPIV]Stage_1 0x%X", StageAddr);

    //raw2yuv config
    StageAddr = ScriptAddr + TotalScriptSize;
    Raw2YvuScrpt = (AMP_SENC_SCRPT_RAW2YUV_s *)StageAddr;
    memset(Raw2YvuScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    Raw2YvuScrpt->Cmd = SENC_RAW2YUV;
    Raw2YvuScrpt->RawType = RawCapScrpt->FvRawType;
    Raw2YvuScrpt->RawBuf.Buf = RawCapScrpt->FvRawBuf.Buf;
    Raw2YvuScrpt->RawBuf.Width = RawCapScrpt->FvRawBuf.Width;
    Raw2YvuScrpt->RawBuf.Height = RawCapScrpt->FvRawBuf.Height;
    Raw2YvuScrpt->RawBuf.Pitch = RawCapScrpt->FvRawBuf.Pitch;
    Raw2YvuScrpt->RawBufRule = RawCapScrpt->FvBufRule;
    Raw2YvuScrpt->RingBufSize = 0;
    Raw2YvuScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    Raw2YvuScrpt->YuvRingBufSize = 0;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoEncPIV]Stage_2 0x%X", StageAddr);

    //yuv2jpg config
    StageAddr = ScriptAddr + TotalScriptSize;
    Yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)StageAddr;
    memset(Yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    Yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    Yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    Yuv2JpgScrpt->YuvRingBufSize = 0;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoEncPIV]Stage_3 0x%X", StageAddr);

    //script config
    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[UT_videoEncPIV]Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    VideoTuningRawCapRunning = 1;

    /* Step4. execute script */
    AmpEnc_RunScript(VideoTuningSencPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step4. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)OriScriptAddr) != OK) {
        AmbaPrint("memFree Fail (scrpt)");
    }
    ScriptAddr = NULL;

    AmbaPrint("memFree Done");

_DONE:

    return 0;
}
#endif

/**
 *  Unit Test Initialization
 *
 * @param [in] sensorID sensor need to be hooked
 *
 */
int AmpUT_VideoTuning_Init(int sensorID, int lcdID)
{
    int er;
    void *TmpbufRaw = NULL;

    // Register LCD driver
//    if (lcdID >= 0) {
//        if (lcdID == 0) {
//        #ifdef CONFIG_LCD_WDF9648W
//            AmbaPrint("Hook Wdf9648w LCD");
//            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_WdF9648wObj);
//        #else
//            AmbaPrint("Please enable Wdf9648w LCD");
//        #endif
//        } else {
//            AmbaPrint("Unsupport lcd id!");
//        }
//    } else {
//        AmbaPrint("Wrong lcd id!");
//    }

    // Register sensor driver
    if (sensorID >= 0) {
        if (VtLiveviewVinSelect == VIN_MAIN_ONLY) {
            memset(&TuningEncChannel, 0x0, sizeof(AMBA_DSP_CHANNEL_ID_u));
            TuningEncChannel.Bits.VinID = 0;
            TuningEncChannel.Bits.SensorID = 0x1;
            VideoTuningMgt = RegisterMWUT_Sensor_Driver(TuningEncChannel,sensorID);
            {
            #ifdef CONFIG_SOC_A12 // Temporary disable for A9
                extern int App_Image_Init(UINT32 ChCount, int sensorID);
                AMBA_SENSOR_DEVICE_INFO_s SensorDeviceInfo = {0};
                AmbaSensor_GetDeviceInfo(TuningEncChannel, &SensorDeviceInfo);
                if (SensorDeviceInfo.HdrIsSupport == 1) {
                    App_Image_Init(2, sensorID);
                } else {
                    App_Image_Init(1, sensorID);
                }
            #endif
            }
        } else if (VtLiveviewVinSelect == VIN_PIP_ONLY) {
            memset(&TuningEncChannel, 0x0, sizeof(AMBA_DSP_CHANNEL_ID_u));
            /* FIXME, because 3A lib AmbaImg_CtrlFunc_Get_Sensor_Status by scan all channel*/
            TuningEncChannel.Bits.VinID = 0;
            TuningEncChannel.Bits.SensorID = 0x1;
            VideoTuningMgt = RegisterMWUT_Sensor_Driver(TuningEncChannel,sensorID);

            TuningEncChannel.Bits.VinID = 1;
            TuningEncChannel.Bits.SensorID = 0x4;
            VideoTuningMgt = RegisterMWUT_Sensor_Driver(TuningEncChannel,sensorID);
            {
            #ifdef CONFIG_SOC_A12 // Temporary disable for A9
                extern int App_Image_Init(UINT32 ChCount, int sensorID);
                extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
                AMBA_SENSOR_DEVICE_INFO_s SensorDeviceInfo = {0};
                AmbaSensor_GetDeviceInfo(TuningEncChannel, &SensorDeviceInfo);
                App_Image_Init(2, sensorID);
                App_Image_Init_Iq_Params(0, sensorID);
                App_Image_Init_Iq_Params(1, sensorID);
            #endif
            }
        }
    } else {
        AmbaPrint("Wrong sensor id!");
    }

    // Create simple muxer semophore
    if (AmbaKAL_SemCreate(&VideoTuningPriSem, 0) != OK) {
        AmbaPrint("VideoTunning UnitTest: Semaphore creation failed");
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_VideoTuningPriStack, &TmpbufRaw, VIDEO_TUNING_PRI_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }

    // Create simple muxer task
    if (AmbaKAL_TaskCreate(&VideoTuningPriMuxTask, "Video Tuning UnitTest Primary Muxing Task", 11, \
         AmpUT_VideoTuning_PriMuxTask, 0x0, AmpUT_VideoTuningPriStack, VIDEO_TUNING_PRI_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoTuning UnitTest: Muxer task creation failed");
    }

    // Create simple jpeg muxer semophore
    if (AmbaKAL_SemCreate(&VideoTuningJpegSem, 0) != OK) {
        AmbaPrint("VideoTunning UnitTest: Semaphore creation failed");
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_VideoTuningJpegStack, &TmpbufRaw, VIDEO_TUNING_JPG_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    // Create simple muxer task
    if (AmbaKAL_TaskCreate(&VideoTuningJpegMuxTask, "Video Tuning UnitTest Jpeg Muxing Task", 11, \
         AmpUT_VideoTuning_JpegMuxTask, 0x0, AmpUT_VideoTuningJpegStack, VIDEO_TUNING_JPG_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoTuning UnitTest: Muxer task creation failed");
    }

    {   // Initialize VIN module
        AMP_VIN_INIT_CFG_s VinInitCfg = {0};
        AmpVin_GetInitDefaultCfg(&VinInitCfg);
        if (VideoTuningVinWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoTuningVinWorkBuf, &TmpbufRaw, VinInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for vin!!");
            }

        }
        VinInitCfg.MemoryPoolAddr = VideoTuningVinWorkBuf;
        AmpVin_Init(&VinInitCfg);
    }

    {   // Initialize VIDEOENC module
        AMP_VIDEOENC_INIT_CFG_s EncInitCfg = {0};
        AmpVideoEnc_GetInitDefaultCfg(&EncInitCfg);
        if (VideoTuningVencWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoTuningVencWorkBuf, &TmpbufRaw, (UINT32)EncInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for enc!!");
            }

        }
        EncInitCfg.MemoryPoolAddr = VideoTuningVencWorkBuf;
        AmpVideoEnc_Init(&EncInitCfg);
    }

    {
        AMBA_IMG_SCHDLR_INIT_CFG_s ImgSchdlrInitCfg;
        AMBA_DSP_IMG_PIPE_e Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
        UINT32 MainViewNumber = 2;
        UINT32 MemSize = 0;

        AmbaImgSchdlr_GetInitDefaultCfg(&ImgSchdlrInitCfg);

        AmbaImgSchdlr_QueryMemsize(MainViewNumber, &MemSize);
        ImgSchdlrInitCfg.MainViewNum = MainViewNumber;
    #ifdef CONFIG_SOC_A9
        ImgSchdlrInitCfg.MemoryPoolSize = MemSize;
    #else
        ImgSchdlrInitCfg.MemoryPoolSize = MemSize + ImgSchdlrInitCfg.MsgTaskStackSize;
    #endif
        if (MainVideoTuningImgSchdlrWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&MainVideoTuningImgSchdlrWorkBuf, &TmpbufRaw, ImgSchdlrInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for imgschdlr!!");
            }
        }
        ImgSchdlrInitCfg.IsoCfgNum = AmpResource_GetIKIsoConfigNumber(Pipe);
        ImgSchdlrInitCfg.MemoryPoolAddr = MainVideoTuningImgSchdlrWorkBuf;
        er = AmbaImgSchdlr_Init(&ImgSchdlrInitCfg);
        if (er != AMP_OK) {
            AmbaPrint("AmbaImgSchdlr_Init Fail!");
        }
    }

    // Allocate Pri bitstream buffer
    {
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&H264BitsBuf, &TmpbufRaw, BITSFIFO_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for pri bitsFifo!!");
        }
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&H264DescBuf, &TmpbufRaw, DESC_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for pri bitsFifo!!");
        }
    }

    // Allocate jpeg bitstream buffer
    {
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&JpegBitsBuf, &TmpbufRaw, BISFIFO_STILL_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for jpeg bitsFifo!!");
        }
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&JpegDescBuf, &TmpbufRaw, DESC_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for jpeg bitsFifo!!");
        }
    }

    // Initialize Dzoom
    {
        AMP_IMG_DZOOM_INIT_CFG_s dzoomInitCfg;
        AmpImgDzoom_GetInitDefaultCfg(&dzoomInitCfg);
        AmpImgDzoom_Init(&dzoomInitCfg);
    }

    if (AmpUT_Display_Init() == NG) {
        return NG;
    }
    return 0;
}

/**
 * Liveview start
 *
 * @param [in] modeIdx encode mode index
 *
 */
int AmpUT_VideoTuning_LiveviewStart(UINT32 modeIdx)
{
    AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
    TuningModeIdx = modeIdx;

    if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE) {
	#ifdef CONFIG_SOC_A9
        AmbaPrintColor(RED,"AmbaUnitTest: video rawencode liveview should boot up after normal liveview");
	#else
		AmpUT_VideoTuning_RawEncodeLiveviewStart();
	#endif
        return 0;
    }

    if (VideoTuningVinA) {
        AmpUT_VideoTuning_ChangeResolution(modeIdx);
        AmpEnc_StartLiveview(VideoTuningPipe, 0);
        return 0;
    }

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Liveview at %s", MWUT_GetInputVideoModeName(TuningModeIdx, TuningEncodeSystem));
    AmbaPrint(" =========================================================");

    // Create Vin instance
    if (VideoTuningVinA == 0) {
        AMP_VIN_HDLR_CFG_s vinCfg;
        AMP_VIN_LAYOUT_CFG_s layout[1];
        AMBA_SENSOR_MODE_ID_u mode = {0};
        memset(&vinCfg, 0x0, sizeof(AMP_VIN_HDLR_CFG_s));
        memset(&layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s));
        mode.Data = VideoTuningMgt[TuningModeIdx].InputMode;
        AmbaSensor_GetModeInfo(TuningEncChannel, mode, &SensorModeInfo);
        // Create VIN instance
        AmpVin_GetDefaultCfg(&vinCfg);
        vinCfg.Channel = TuningEncChannel;
        vinCfg.Mode = mode;
        vinCfg.LayoutNumber = 1;
        vinCfg.HwCaptureWindow.Width = VideoTuningMgt[TuningModeIdx].CaptureWidth;
        vinCfg.HwCaptureWindow.Height = VideoTuningMgt[TuningModeIdx].CaptureHeight;
        vinCfg.HwCaptureWindow.X = SensorModeInfo.OutputInfo.RecordingPixels.StartX +
            (((SensorModeInfo.OutputInfo.RecordingPixels.Width - vinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        vinCfg.HwCaptureWindow.Y = SensorModeInfo.OutputInfo.RecordingPixels.StartY +
            (((SensorModeInfo.OutputInfo.RecordingPixels.Height - vinCfg.HwCaptureWindow.Height)/2)&0xFFFE);
        layout[0].Width = VideoTuningMgt[TuningModeIdx].MainWidth;
        layout[0].Height = VideoTuningMgt[TuningModeIdx].MainHeight;
        layout[0].EnableSourceArea = 0; // Get all capture window to main
        layout[0].DzoomFactorX = InitZoomX; // 16.16 format
        layout[0].DzoomFactorY = InitZoomY;
        layout[0].DzoomOffsetX = 0;
        layout[0].DzoomOffsetY = 0;
        layout[0].MainviewReportRate = 1;
        vinCfg.Layout = layout;
        vinCfg.cbEvent = AmpUT_VideoTuningVinEventCallback;
        vinCfg.cbSwitch= AmpUT_VideoTuningVinSwitchCallback;
        AmpVin_Create(&vinCfg, &VideoTuningVinA);
    }

    // Remember frame/field rate
    EncFrameRate = VideoTuningMgt[TuningModeIdx].TimeScale/VideoTuningMgt[TuningModeIdx].TickPerPicture;

    // Create ImgSchdlr instance
    if (MainVideoTuningImgSchdlr == NULL) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg = {0};
        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);
        ImgSchdlrCfg.MainViewID = 0;
        ImgSchdlrCfg.Channel = TuningEncChannel;
        ImgSchdlrCfg.Vin = VideoTuningVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_VideoTuningImgSchdlrCallback;
        if (TuningLiveViewProcMode && TuningLiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
            if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                ImgSchdlrCfg.VideoProcMode |= 0x10;
            }
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &MainVideoTuningImgSchdlr);
    }

    if (PipVideoTuningImgSchdlr == NULL && VtLiveviewVinSelect >= VIN_PIP_ONLY) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg = {0};
        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);
        ImgSchdlrCfg.MainViewID = 1;
        ImgSchdlrCfg.Channel = TuningEncChannel;
        ImgSchdlrCfg.Vin = VideoTuningVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_VideoTuningImgSchdlrCallback;
        if (TuningLiveViewProcMode && TuningLiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
            if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                ImgSchdlrCfg.VideoProcMode |= 0x10;
            }
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &PipVideoTuningImgSchdlr);
    }

    // Create video encoder objects
    if (VideoTuningPri == 0) {
        AMP_VIDEOENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s EncLayer = {0};

        EncCfg.MainLayout.Layer = &EncLayer;
        AmpVideoEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_VideoTuningCallback;

        // Assign main layout
        EncCfg.MainLayout.Width = VideoTuningMgt[TuningModeIdx].MainWidth;
        EncCfg.MainLayout.Height = VideoTuningMgt[TuningModeIdx].MainHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = VideoTuningMgt[TuningModeIdx].Interlace;
        EncCfg.MainTimeScale = VideoTuningMgt[TuningModeIdx].TimeScale;
        EncCfg.MainTickPerPicture = VideoTuningMgt[TuningModeIdx].TickPerPicture;
        EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
    #ifdef CONFIG_SOC_A12
        EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
        EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
        EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
    #endif
        EncLayer.SourceType = AMP_ENC_SOURCE_VIN;
        EncLayer.Source = VideoTuningVinA;
        EncLayer.SourceLayoutId = 0;
        EncLayer.EnableSourceArea = 0;  // No source cropping
        EncLayer.EnableTargetArea = 0;  // No target pip
        EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
        EncCfg.StreamId = AMP_VIDEOENC_STREAM_PRIMARY;
        {
            UINT8 *DspWorkAddr;
            UINT32 DspWorkSize;
            AmpUT_VideoTuning_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
            EncCfg.DspWorkBufAddr = DspWorkAddr;
            EncCfg.DspWorkBufSize = DspWorkSize;
        }
        if (TuningLiveViewProcMode == 0) {
            if (VideoTuningMgt[TuningModeIdx].MainWidth > 1920 || \
                VideoTuningMgt[TuningModeIdx].CaptureWidth > 1920)
                TuningLiveViewOSMode = 1;
        }
        EncCfg.LiveViewProcMode = TuningLiveViewProcMode;
        EncCfg.LiveViewAlgoMode = TuningLiveViewAlgoMode;
        EncCfg.LiveViewOSMode = TuningLiveViewOSMode;
        if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
            EncCfg.LiveViewHdrMode = TuningLiveViewHdrMode = 1;
        } else {
            EncCfg.LiveViewHdrMode = TuningLiveViewHdrMode = 0;
        }
        EncCfg.VinSelect = VtLiveviewVinSelect;
        // Create primary stream handler
        AmpVideoEnc_Create(&EncCfg, &VideoTuningPri); // Don't have to worry about h.264 spec settings when liveview
    }

    // Register pipeline
    {
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};
        // Register pipeline
        AmpEnc_GetDefaultCfg(&PipeCfg);
        //PipeCfg.cbEvent
        PipeCfg.encoder[0] = VideoTuningPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_VideoTuningPipeCallback;
        PipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&PipeCfg, &VideoTuningPipe);
        AmpEnc_Add(VideoTuningPipe);
    }

    AmpUT_VideoTuning_DisplayStart();

    // Set capture in Dzoom
    {
        AMP_IMG_DZOOM_CFG_s DzoomCfg = {0};
        AMP_IMG_DZOOM_VIN_INVALID_INFO_s DzoomVinInvalidInfo = {0};

        AmpImgDzoom_GetDefaultCfg(&DzoomCfg);
        DzoomCfg.ImgModeContextId = 0;
        DzoomCfg.ImgModeBatchId = 0;
        AmpImgDzoom_Create(&DzoomCfg, &DzoomHdlr);
        DzoomTable.TotalStepNumber = DZOOM_10X_TOTAL_STEP;
        DzoomTable.MaxDzoomFactor = DZOOM_10X_MAX_RATIO;
        DzoomTable.GetDzoomFactor = VideoTuning_Dzoom_Step_Factor_Table;
        AmpImgDzoom_RegDzoomTable(DzoomHdlr, &DzoomTable);
        DzoomVinInvalidInfo.CapW = VideoTuningMgt[TuningModeIdx].CaptureWidth;
        DzoomVinInvalidInfo.CapH = VideoTuningMgt[TuningModeIdx].CaptureHeight;
        AmpImgDzoom_ChangeResolutionHandler(DzoomHdlr, &DzoomVinInvalidInfo);
        AmpImgDzoom_ResetStatus(DzoomHdlr);
    }
    AmpEnc_StartLiveview(VideoTuningPipe, 0);
    return 0;
}

int AmpUT_VideoTuning_EncStop(void)
{
    AmpEnc_StopRecord(VideoTuningPipe, 0);
    VideoRawEncIsoCfgIndex = 0;
    return 0;
}

int AmpUT_VideoTuning_LiveviewStop(void)
{
    UINT32 Flag = 0;

    // Set Dzoom
    AmpImgDzoom_StopDzoom(DzoomHdlr);
    if (MainVideoTuningImgSchdlr) {
        AmbaImgSchdlr_Enable(MainVideoTuningImgSchdlr, 0);
    }
    if (PipVideoTuningImgSchdlr) {
        AmbaImgSchdlr_Enable(PipVideoTuningImgSchdlr, 0);
    }
    Amba_Img_VIn_Invalid(VideoTuningPri, (UINT32 *)NULL);
    if (WirelessModeEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        DspSuspendEnable = 0;
    } else if (DspSuspendEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        WirelessModeEnable = 0;
    }
    AmpEnc_StopLiveview(VideoTuningPipe, Flag);

#if 1
    AmbaKAL_TaskSleep(500); // wait dsp really stopped

    if (VideoTuningPipe) {
        AmpEnc_Delete(VideoTuningPipe); VideoTuningPipe = NULL;
    }
    if (VideoTuningVinA) {
        AmpVin_Delete(VideoTuningVinA); VideoTuningVinA = NULL;
    }
    // FIXME: VideoEnc_Delete will reset bits manager, this will leads still codec error when capture raw+yuv.
    //if (VideoTuningPri) {
    //    AmpVideoEnc_Delete(VideoTuningPri); VideoTuningPri = NULL;
    //}

    if (AmpResource_GetVoutSeamlessEnable() == 0) {
        if (TuningLCDLiveview) {
            AmpUT_Display_Stop(0);
        }
    }

    if (TuningTvLiveview) {
        AmpUT_Display_Stop(1);
    }

    // Delete Dzoom
    {
        AmpImgDzoom_Delete(DzoomHdlr);
    }
#endif

    return 0;
}

int AmpUT_VideoTuning_ChangeResolution(UINT32 modeIdx)
{
    AMP_VIN_RUNTIME_CFG_s vinCfg = {0};
    AMP_VIDEOENC_MAIN_CFG_s mainCfg[1] = {0};
    AMP_VIDEOENC_LAYER_DESC_s newPriLayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
    AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
    AMP_VIN_LAYOUT_CFG_s layout[1];
    AMBA_SENSOR_MODE_ID_u mode = {0};

    TuningModeIdx = modeIdx;

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Change Resolution to %s", MWUT_GetInputVideoModeName(TuningModeIdx, TuningEncodeSystem));
    AmbaPrint(" =========================================================");

    memset(&layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s));

    mode.Data = VideoTuningMgt[TuningModeIdx].InputMode;
    AmbaSensor_GetModeInfo(TuningEncChannel, mode, &SensorModeInfo);

    vinCfg.Hdlr = VideoTuningVinA;
    vinCfg.Mode = mode;
    vinCfg.LayoutNumber = 1;
    vinCfg.HwCaptureWindow.Width = VideoTuningMgt[TuningModeIdx].CaptureWidth;
    vinCfg.HwCaptureWindow.Height = VideoTuningMgt[TuningModeIdx].CaptureHeight;
    vinCfg.HwCaptureWindow.X = SensorModeInfo.OutputInfo.RecordingPixels.StartX +
        (((SensorModeInfo.OutputInfo.RecordingPixels.Width - vinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
    vinCfg.HwCaptureWindow.Y = SensorModeInfo.OutputInfo.RecordingPixels.StartY +
        (((SensorModeInfo.OutputInfo.RecordingPixels.Height - vinCfg.HwCaptureWindow.Height)/2)&0xFFFE);
    layout[0].Width = VideoTuningMgt[TuningModeIdx].MainWidth;
    layout[0].Height = VideoTuningMgt[TuningModeIdx].MainHeight;
    layout[0].EnableSourceArea = 0; // Get all capture window to main
    layout[0].DzoomFactorX = InitZoomX;
    layout[0].DzoomFactorY = InitZoomY;
    layout[0].DzoomOffsetX = 0;
    layout[0].DzoomOffsetY = 0;
    layout[0].MainviewReportRate = 1;
    vinCfg.Layout = &layout[0];

    if (MainVideoTuningImgSchdlr == NULL) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg = {0};
        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);
        ImgSchdlrCfg.MainViewID = 0;
        ImgSchdlrCfg.Channel = TuningEncChannel;
        ImgSchdlrCfg.Vin = VideoTuningVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_VideoTuningImgSchdlrCallback;
        if (TuningLiveViewProcMode && TuningLiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
            if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                ImgSchdlrCfg.VideoProcMode |= 0x10;
            }
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &MainVideoTuningImgSchdlr);
    }

    if (PipVideoTuningImgSchdlr == NULL && VtLiveviewVinSelect >= VIN_PIP_ONLY) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg = {0};
        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);
        ImgSchdlrCfg.MainViewID = 1;
        ImgSchdlrCfg.Channel = TuningEncChannel;
        ImgSchdlrCfg.Vin = VideoTuningVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_VideoTuningImgSchdlrCallback;
        if (TuningLiveViewProcMode && TuningLiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
            if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                ImgSchdlrCfg.VideoProcMode |= 0x10;
            }
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &PipVideoTuningImgSchdlr);
    }

    mainCfg[0].Hdlr = VideoTuningPri;
    mainCfg[0].MainLayout.LayerNumber = 1;
    mainCfg[0].MainLayout.Layer = &newPriLayer;
    mainCfg[0].MainLayout.Width = VideoTuningMgt[TuningModeIdx].MainWidth;
    mainCfg[0].MainLayout.Height = VideoTuningMgt[TuningModeIdx].MainHeight;
    mainCfg[0].Interlace = VideoTuningMgt[TuningModeIdx].Interlace;
    mainCfg[0].MainTickPerPicture = VideoTuningMgt[TuningModeIdx].TickPerPicture;
    mainCfg[0].MainTimeScale = VideoTuningMgt[TuningModeIdx].TimeScale;
    mainCfg[0].SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
    mainCfg[0].SysFreq.IdspFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
    mainCfg[0].SysFreq.CoreFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
#ifdef CONFIG_SOC_A12
    mainCfg[0].SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
    mainCfg[0].SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
    mainCfg[0].SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
#endif

    {
        UINT8 *DspWorkAddr;
        UINT32 DspWorkSize;
        AmpUT_VideoTuning_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
        mainCfg[0].DspWorkBufAddr = DspWorkAddr;
        mainCfg[0].DspWorkBufSize = DspWorkSize;
    }
    newPriLayer.EnableSourceArea = 0;
    newPriLayer.EnableTargetArea = 0;
    newPriLayer.LayerId = 0;
    newPriLayer.SourceType = AMP_ENC_SOURCE_VIN;
    newPriLayer.Source = VideoTuningVinA;
    newPriLayer.SourceLayoutId = 0;

    // Set capture in Dzoom
    {
        AMP_IMG_DZOOM_VIN_INVALID_INFO_s dzoomVinInvalidInfo;
        dzoomVinInvalidInfo.CapW = VideoTuningMgt[TuningModeIdx].CaptureWidth;
        dzoomVinInvalidInfo.CapH = VideoTuningMgt[TuningModeIdx].CaptureHeight;

        AmpImgDzoom_StopDzoom(DzoomHdlr);
        AmpImgDzoom_ResetStatus(DzoomHdlr);
        AmpImgDzoom_ChangeResolutionHandler(DzoomHdlr, &dzoomVinInvalidInfo);
    }

    if (TuningLiveViewProcMode == 0) {
        if (VideoTuningMgt[TuningModeIdx].MainWidth > 1920 || \
            VideoTuningMgt[TuningModeIdx].CaptureWidth > 1920) {
            TuningLiveViewOSMode = 1;
        }
    }

    mainCfg[0].LiveViewProcMode = TuningLiveViewProcMode;
    mainCfg[0].LiveViewAlgoMode = TuningLiveViewAlgoMode;
    mainCfg[0].LiveViewOSMode = TuningLiveViewOSMode;
    if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
        mainCfg[0].LiveViewHdrMode = TuningLiveViewHdrMode = 1;
    } else {
        mainCfg[0].LiveViewHdrMode = TuningLiveViewHdrMode = 0;
    }

    AmpVideoEnc_ConfigVinMain(1, &vinCfg, 1, &mainCfg[0]);

    // Remember frame/field rate
    EncFrameRate = VideoTuningMgt[TuningModeIdx].TimeScale/VideoTuningMgt[TuningModeIdx].TickPerPicture;

    return 0;
}

/**
 * UnitTest: Calculate dsp working address and size base on current capture mode
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoTuning_DspWork_Calculate(UINT8 **addr, UINT32 *size)
{
    extern UINT8 *DspWorkAreaResvStart;
    extern UINT32 DspWorkAreaResvSize;

    if (VideoTuningCaptureMode == TUNNING_MODE_CAPTURE_NONE) {
        //only allocate MJPEG Bits
        (*addr) = DspWorkAreaResvStart;
        (*size) = DspWorkAreaResvSize;
    } else if (VideoTuningCaptureMode == TUNNING_MODE_LIVEVIEW_CAPTUE) {
        //allocate Raw buffer + MJPEG Bits
        (*addr) = DspWorkAreaResvStart;
        (*size) = (IsFastCapture)? (DspWorkAreaResvSize): (200<<20);
    } else if (VideoTuningCaptureMode == TUNNING_MODE_VIDEORAWENCODE) {
        //allocate MJPEG Bits
        (*addr) = DspWorkAreaResvStart;
        (*size) = DspWorkAreaResvSize - (20<<20); //temp resv 20MB for MJPEG Bits
    }
    AmbaPrint("[DspWork_Calculate] addr 0x%X, Sz %u", *addr, *size);
    return 0;
}

/**
 * UnitTest: Video RawEncode LiveView Start
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_VideoTuning_RawEncodeLiveviewStart(void)
{
    ITUNER_SYSTEM_s SystemInfo = {0};
    ITUNER_INFO_s ItunerInfo = {0};
#ifdef CONFIG_SOC_A12
    ITUNER_VIDEO_HDR_RAW_INFO_s HdrRawInfo = {0};
#endif

    VideoTuning3AEnableDefault = VideoTuning3AEnable;
    VideoTuning3AEnable = 0;

#ifdef CONFIG_SOC_A9
    // 20140709 Chester: video raw encode should have following flow:
    // boot up -> normal lv -> switch to video raw encode lv -> encode start -> feed raw -> end
    // since SSP will invoke one 0x96005 in 1st boot up (for def. cmd) and every time when feed raw,
    // if we boot to video raw encode lv, dsp will complain rcving two 0x96005 but no r2y done when 1st feed raw.

    if (VideoTuningVinA) {
        AMP_VIN_RUNTIME_CFG_s vinCfg;
        AMP_VIDEOENC_MAIN_CFG_s mainCfg[1] = {0};
        AMP_VIDEOENC_LAYER_DESC_s newPriLayer = {0, 0, 0, AMP_ENC_SOURCE_MEMORY, 0, 0, {0,0,0,0},{0,0,0,0}};
        UINT8 *DspWorkAddr;
        UINT32 DspWorkSize;

        AmbaPrint(" ========================================================= ");
        AmbaPrint(" AmbaUnitTest: Change Resolution to Video RawEncode");
        AmbaPrint(" =========================================================");

        memset(&vinCfg, 0x0, sizeof(AMP_VIN_RUNTIME_CFG_s));
        AmpUT_VideoTuning_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);

        newPriLayer.EnableSourceArea = 0;
        newPriLayer.EnableTargetArea = 0;
        newPriLayer.LayerId          = 0;
        newPriLayer.SourceType       = AMP_ENC_SOURCE_MEMORY;
        newPriLayer.Source           = 0; // No need vin src
        newPriLayer.SourceLayoutId   = 0;

        mainCfg[0].Hdlr                   = VideoTuningPri;
        mainCfg[0].MainLayout.LayerNumber = 1;
        mainCfg[0].MainLayout.Layer       = &newPriLayer;
        mainCfg[0].MainLayout.Width       = VideoRawEncVar.rawBin.RawWidth;
        mainCfg[0].MainLayout.Height      = VideoRawEncVar.rawBin.RawHeight;
        mainCfg[0].MainTickPerPicture     = VideoRawEncVar.rawBin.TickPerPicture;
        mainCfg[0].MainTimeScale          = VideoRawEncVar.rawBin.TimeScale;
        mainCfg[0].Interlace              = VideoRawEncVar.rawBin.Interlace;
        mainCfg[0].DspWorkBufAddr         = DspWorkAddr;
        mainCfg[0].DspWorkBufSize         = DspWorkSize;
        mainCfg[0].SysFreq.ArmCortexFreq  = AMP_SYSTEM_FREQ_KEEPCURRENT;
        mainCfg[0].SysFreq.IdspFreq       = AMP_SYSTEM_FREQ_KEEPCURRENT;
        mainCfg[0].SysFreq.CoreFreq       = AMP_SYSTEM_FREQ_KEEPCURRENT;

        // Set capture in Dzoom
        if (DzoomHdlr) {
            AMP_IMG_DZOOM_VIN_INVALID_INFO_s dzoomVinInvalidInfo;
            dzoomVinInvalidInfo.CapW = VideoRawEncVar.rawBin.RawWidth;
            dzoomVinInvalidInfo.CapH = VideoRawEncVar.rawBin.RawHeight;
            AmpImgDzoom_StopDzoom(DzoomHdlr);
            AmpImgDzoom_ResetStatus(DzoomHdlr);
            AmpImgDzoom_ChangeResolutionHandler(DzoomHdlr, &dzoomVinInvalidInfo);
        }

        AmpVideoEnc_ConfigVinMain(0, &vinCfg, 1, &mainCfg[0]);
        AmpEnc_StartLiveview(VideoTuningPipe, 0);
    } else {
        AmbaPrintColor(RED,"AmbaUnitTest: video raw encode liveview should change from normal liveview");
    }
#else
    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Liveview at Video RawEncode");
    AmbaPrint(" =========================================================");

    AmbaTUNE_Get_SystemInfo(&SystemInfo);
    AmbaTUNE_Get_ItunerInfo(&ItunerInfo);
    AmbaItuner_Get_VideoHdrRawInfo(&HdrRawInfo);

    // fill RawEncVar
    {
        UINT16 AspectRatio = 0;

        VideoRawEncVar.rawBin.CaptureNum = 10; //FIXME
        VideoRawEncVar.rawBin.RawPitch = SystemInfo.RawPitch;
        VideoRawEncVar.rawBin.RawWidth = SystemInfo.RawWidth;
        VideoRawEncVar.rawBin.RawHeight = SystemInfo.RawHeight;
        VideoRawEncVar.rawBin.RawType = (SystemInfo.CompressedRaw == 1)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
        VideoTuning_AspectRatioGet(SystemInfo.RawWidth, SystemInfo.RawWidth, &AspectRatio);
        VideoRawEncVar.rawBin.AspectRatio = AspectRatio;

        AmbaPrint("[UT_VdoRawEnc]====== Video Encode Info ======");
        //AmbaPrint("[UT_VdoRawEnc] Raw Number : %04d", VideoRawEncVar.rawBin.CaptureNum);
        AmbaPrint("[UT_VdoRawEnc]      Pitch : %04d", VideoRawEncVar.rawBin.RawPitch);
        AmbaPrint("[UT_VdoRawEnc]      Width : %04d", VideoRawEncVar.rawBin.RawWidth);
        AmbaPrint("[UT_VdoRawEnc]     Height : %04d", VideoRawEncVar.rawBin.RawHeight);
        if (VideoRawEncVar.rawBin.IsYuv) {
            AmbaPrint("[UT_VdoRawEnc]       Type : %s", (VideoRawEncVar.rawBin.RawType == AMP_STILLENC_RAW_COMPR)? "compr": "uncompr");
        } else {
            AmbaPrint("[UT_VdoRawEnc]       Type : %s", (VideoRawEncVar.rawBin.YuvFmt == AMP_YUV_420)? "420": "422");
        }
        AmbaPrint("[UT_VdoRawEnc]  TimeScale : %d", VideoRawEncVar.rawBin.TimeScale);
        AmbaPrint("[UT_VdoRawEnc]       Tick : %d", VideoRawEncVar.rawBin.TickPerPicture);
        AmbaPrint("[UT_VdoRawEnc]  Interlace : %d", VideoRawEncVar.rawBin.Interlace);
        AmbaPrint("[UT_VdoRawEnc]         AR : 0x%X", VideoRawEncVar.rawBin.AspectRatio);
        AmbaPrint("[UT_VdoRawEnc]=========================");
    }

    // Create Vin instance
    if (VideoTuningVinA == 0) {
        AMP_VIN_HDLR_CFG_s vinCfg;
        AMBA_SENSOR_MODE_INFO_s SensorModeInfo = {0};
        AMP_VIN_LAYOUT_CFG_s layout[1];
        AMBA_SENSOR_MODE_ID_u mode = {0};

        memset(&vinCfg, 0x0, sizeof(AMP_VIN_HDLR_CFG_s));
        memset(&layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s));

        mode.Data = VideoTuningMgt[TuningModeIdx].InputMode;
        AmbaSensor_GetModeInfo(TuningEncChannel, mode, &SensorModeInfo);

        // Create VIN instance
        AmpVin_GetDefaultCfg(&vinCfg);

        vinCfg.Channel = TuningEncChannel;
        vinCfg.Mode = mode;
        vinCfg.LayoutNumber = 1;
        vinCfg.HwCaptureWindow.Width = VideoRawEncVar.rawBin.RawWidth;
        vinCfg.HwCaptureWindow.Height = VideoRawEncVar.rawBin.RawHeight;
        vinCfg.HwCaptureWindow.X = vinCfg.HwCaptureWindow.Y = 0;
        layout[0].Width = SystemInfo.MainWidth;
        layout[0].Height = SystemInfo.MainHeight;
        layout[0].EnableSourceArea = 0; // Get all capture window to main
        layout[0].DzoomFactorX = InitZoomX; // 16.16 format
        layout[0].DzoomFactorY = InitZoomY;
        layout[0].DzoomOffsetX = 0;
        layout[0].DzoomOffsetY = 0;
        layout[0].MainviewReportRate = 1;
        vinCfg.Layout = layout;
        vinCfg.cbEvent = AmpUT_VideoTuningVinEventCallback;
        vinCfg.cbSwitch= AmpUT_VideoTuningVinSwitchCallback;
        vinCfg.VinSource = AMP_VIN_MEMORY_CFA;
        AmpVin_Create(&vinCfg, &VideoTuningVinA);
    }

    // Remember frame/field rate
    EncFrameRate = VideoRawEncVar.rawBin.TimeScale/VideoRawEncVar.rawBin.TickPerPicture;

    // Create ImgSchdlr instance
    if (MainVideoTuningImgSchdlr == NULL) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg;

        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

        ImgSchdlrCfg.MainViewID = TuningEncChannel.Bits.VinID;
        ImgSchdlrCfg.Channel = TuningEncChannel;
        ImgSchdlrCfg.Vin = 0; //RawEncode does not have real Vin
        ImgSchdlrCfg.cbEvent = AmpUT_VideoTuningImgSchdlrCallback;
        if (TuningLiveViewProcMode && TuningLiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
            if (ItunerInfo.TuningModeExt == SINGLE_SHOT_MULTI_EXPOSURE_HDR) {
                ImgSchdlrCfg.VideoProcMode |= 0x10;
            }
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &MainVideoTuningImgSchdlr);  // One sensor (not vin) need one scheduler.
    }
    if (PipVideoTuningImgSchdlr == NULL && VtLiveviewVinSelect >= VIN_PIP_ONLY) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg;

        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

        ImgSchdlrCfg.MainViewID = TuningEncChannel.Bits.VinID; //single channle have one MainView
        ImgSchdlrCfg.Channel = TuningEncChannel;
        ImgSchdlrCfg.Vin = 0; //RawEncode does not have real Vin
        ImgSchdlrCfg.cbEvent = AmpUT_VideoTuningImgSchdlrCallback;
        if (TuningLiveViewProcMode && TuningLiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
            if (ItunerInfo.TuningModeExt == SINGLE_SHOT_MULTI_EXPOSURE_HDR) {
                ImgSchdlrCfg.VideoProcMode |= 0x10;
            }
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &PipVideoTuningImgSchdlr);  // One sensor (not vin) need one scheduler.
    }

    // Create video encoder objects
    if (VideoTuningPri == 0) {
        AMP_VIDEOENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s EncLayer = {0};

        EncCfg.MainLayout.Layer = &EncLayer;
        AmpVideoEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_VideoTuningCallback;

        // Assign main layout
        EncCfg.MainLayout.Width = SystemInfo.MainWidth;
        EncCfg.MainLayout.Height = SystemInfo.MainHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = VideoRawEncVar.rawBin.Interlace;
        EncCfg.MainTimeScale = VideoRawEncVar.rawBin.TimeScale;
        EncCfg.MainTickPerPicture = VideoRawEncVar.rawBin.TickPerPicture;
        EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
    #ifdef CONFIG_SOC_A12
        EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
        EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
        EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
    #endif
        { // fill memory buffer
            UINT8 *bufaddr;
            UINT32 bufsize = 0, avalRawNum = 0;
            UINT32 rawSize = 0, resvSize = 0;

            AmpUT_VideoTuning_DspWork_Calculate(&bufaddr, &bufsize);
            resvSize = bufsize - 150*1024*1024;
            rawSize = VideoRawEncVar.rawBin.RawPitch*VideoRawEncVar.rawBin.RawHeight*VideoRawEncVar.rawBin.CaptureNum;
            rawSize = ALIGN_32(rawSize+31);
            if (resvSize >= rawSize) {
                VideoRawBuffAddr = bufaddr + bufsize - rawSize;
                VideoRawBuffAddr = (UINT8 *)ALIGN_32((UINT32)VideoRawBuffAddr);
                avalRawNum = VideoRawEncVar.rawBin.CaptureNum;
            } else {
                VideoRawBuffAddr = bufaddr + bufsize - (resvSize - 32);
                VideoRawBuffAddr = (UINT8 *)ALIGN_32((UINT32)VideoRawBuffAddr);
                avalRawNum = (resvSize - 32)/(VideoRawEncVar.rawBin.RawPitch*VideoRawEncVar.rawBin.RawHeight);
            }
            AmbaPrint("[%s]RawStartAddr 0x%X, BufNum %d", __FUNCTION__, VideoRawBuffAddr, avalRawNum);

            EncLayer.MemorySource.pBaseAddr = VideoRawBuffAddr;
            EncLayer.MemorySource.Pitch = VideoRawEncVar.rawBin.RawPitch;
            EncLayer.MemorySource.Window.Width = VideoRawEncVar.rawBin.RawWidth;
            EncLayer.MemorySource.Window.Height = VideoRawEncVar.rawBin.RawHeight;
            EncLayer.MemorySource.Window.OffsetX = 0;
            EncLayer.MemorySource.Window.OffsetY = 0;
            EncLayer.MemoryBufferNumber = avalRawNum;

            EncLayer.MemoryRawBayerPattern = SystemInfo.RawBayer;
            EncLayer.MemoryRawDataBits = SystemInfo.RawResolution;
        }
        EncLayer.SourceType = AMP_ENC_SOURCE_MEMORY;
        EncLayer.Source = VideoTuningVinA;
        EncLayer.SourceLayoutId = 0;
        EncLayer.EnableSourceArea = 0;  // No source cropping
        EncLayer.EnableTargetArea = 0;  // No target pip
        EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
        EncCfg.StreamId = AMP_VIDEOENC_STREAM_PRIMARY;

        {
            UINT8 *DspWorkAddr;
            UINT32 DspWorkSize;
            AmpUT_VideoTuning_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
            EncCfg.DspWorkBufAddr = DspWorkAddr;
            EncCfg.DspWorkBufSize = DspWorkSize;
        }

        //A12 VideoTuning always run at Hybrid Mode
        EncCfg.LiveViewProcMode = TuningLiveViewProcMode = 1;
        EncCfg.LiveViewOSMode = TuningLiveViewOSMode = 0;
        EncCfg.LiveViewAlgoMode = TuningLiveViewAlgoMode = ItunerInfo.TuningAlgoMode.AlgoMode;
        EncCfg.LiveViewHdrMode = TuningLiveViewHdrMode = (ItunerInfo.TuningModeExt == SINGLE_SHOT_MULTI_EXPOSURE_HDR)? 1: 0;

        EncCfg.VinSelect = VtLiveviewVinSelect;
        // Create primary stream handler
        AmpVideoEnc_Create(&EncCfg, &VideoTuningPri); // Don't have to worry about h.264 spec settings when liveview
    }

    AmpUT_VideoTuning_DisplayStart();

    // Register pipeline
    {
        AMP_ENC_PIPE_CFG_s PipeCfg;
        // Register pipeline
        AmpEnc_GetDefaultCfg(&PipeCfg);
        //PipeCfg.cbEvent
        PipeCfg.encoder[0] = VideoTuningPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_VideoTuningPipeCallback;
        PipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&PipeCfg, &VideoTuningPipe);

        AmpEnc_Add(VideoTuningPipe);
    }

    // Set capture in Dzoom
    if (0) {
        AMP_IMG_DZOOM_CFG_s dzoomCfg;
        AMP_IMG_DZOOM_VIN_INVALID_INFO_s dzoomVinInvalidInfo;

        AmpImgDzoom_GetDefaultCfg(&dzoomCfg);
        dzoomCfg.ImgModeContextId = 0;
        dzoomCfg.ImgModeBatchId = 0;
        AmpImgDzoom_Create(&dzoomCfg, &DzoomHdlr);

        DzoomTable.TotalStepNumber = DZOOM_10X_TOTAL_STEP;
        DzoomTable.MaxDzoomFactor = DZOOM_10X_MAX_RATIO;
        DzoomTable.GetDzoomFactor = VideoTuning_Dzoom_Step_Factor_Table;
        AmpImgDzoom_RegDzoomTable(DzoomHdlr, &DzoomTable);

        dzoomVinInvalidInfo.CapW = VideoRawEncVar.rawBin.RawWidth;
        dzoomVinInvalidInfo.CapH = VideoRawEncVar.rawBin.RawHeight;

        AmpImgDzoom_ChangeResolutionHandler(DzoomHdlr, &dzoomVinInvalidInfo);

        AmpImgDzoom_ResetStatus(DzoomHdlr);
    }

    //
    // Note: For A9, Bitstream-specific configs can be assigned here (before encode start), or when codec instance creation.
    //       But for A7L/A12 family, it is better to assign them when codec instance creation.
    //
    {
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
        AMP_VIDEOENC_BITSTREAM_CFG_s BitsCfg = {0};

    #ifdef CONFIG_SOC_A12
        BitsCfg.VideoThumbnail = TuningEncThumbnail;
    #endif

        // Assign bitstream-specific configs
        BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
        H264Cfg = &BitsCfg.Spec.H264Cfg;
        H264Cfg->GopM = 1;
        H264Cfg->GopN = GOP_N;
        H264Cfg->GopIDR = GOP_IDR;
        H264Cfg->GopHierarchical = 0;
        H264Cfg->QPControl.QpMinI = 14;
        H264Cfg->QPControl.QpMaxI = 51;
        H264Cfg->QPControl.QpMinP = 17;
        H264Cfg->QPControl.QpMaxP = 51;
        H264Cfg->QPControl.QpMinB = 21;
        H264Cfg->QPControl.QpMaxB = 51;
        H264Cfg->Cabac = 1;
        H264Cfg->QualityControl.LoopFilterEnable = 1;
        H264Cfg->QualityControl.LoopFilterAlpha = 0;
        H264Cfg->QualityControl.LoopFilterBeta = 0;
        H264Cfg->StartFromBFrame = 1;
        H264Cfg->Interlace = VideoRawEncVar.rawBin.Interlace;
        H264Cfg->TimeScale = VideoRawEncVar.rawBin.TimeScale;
        H264Cfg->TickPerPicture = VideoRawEncVar.rawBin.TickPerPicture;
        H264Cfg->AuDelimiterType = 1;
        H264Cfg->QualityLevel = 0;
        H264Cfg->StopMethod = AMP_VIDEOENC_STOP_NEXT_IP;
        HeaderInfo.GopM = H264Cfg->GopM;
        {
            ITUNER_SYSTEM_s SystemInfo = {0};
            AmbaTUNE_Get_SystemInfo(&SystemInfo);
            HeaderInfo.Width = SystemInfo.MainWidth;
            HeaderInfo.Height = SystemInfo.MainHeight;
            HeaderInfo.Interlace = VideoRawEncVar.rawBin.Interlace;
        }
        HeaderInfo.Rotation = (AMP_ROTATION_e)0;

        // Use default SPS/VUI
        AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
        H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

        // Bitrate control
        H264Cfg->BitRateControl.BrcMode = VIDEOENC_CBR;
        H264Cfg->BitRateControl.AverageBitrate = 12 * 1E6;
        if (H264Cfg->BitRateControl.BrcMode == VIDEOENC_SMART_VBR) {
            H264Cfg->BitRateControl.MaxBitrate = 18 * 1E6;
            H264Cfg->BitRateControl.MinBitrate = 6 * 1E6;
        }
        H264Cfg->QualityControl.IBeatMode = 0;
        AmpVideoEnc_SetBitstreamConfig(VideoTuningPri, &BitsCfg);

        //
        // Setup bitstream buffer.
        //
        // Rule: H.264 and MJPEG can't use the same bitstream/descriptor buffer. Same Spec uses the same buffer. No matter it is primary or secondary
        // Note: Since buffer allocation depends on the above rule, it is better to assign bitstream buffer before encode start.
        //       Otherwise you have to know what you are going to encode when codec instance creation
        {
            AMP_ENC_BITSBUFFER_CFG_s  bitsBufCfg = {0};

            bitsBufCfg.BitsBufAddr = H264BitsBuf;
            bitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
            bitsBufCfg.DescBufAddr = H264DescBuf;
            bitsBufCfg.DescBufSize = DESC_SIZE;
            bitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            AmpVideoEnc_SetBitstreamBuffer(VideoTuningPri, &bitsBufCfg);

            AmbaPrint("H.264 Bits 0x%x size %d Desc 0x%x size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
        }
    }

    AmpEnc_StartLiveview(VideoTuningPipe, AMP_ENC_FUNC_FLAG_NONE);
#endif
    return 0;
}

/**
 * UnitTest: simple Video RawEncode enable(ie manual videoEncode with external raw input)
 *
 * @param [in] inputID input ID of raw
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_VideoTuning_VideoRawEncEnable(UINT32 inputID)
{
    char fn[32];
    char mdASCII[3] = {'r','b','\0'};
    UINT64 retRd = 0;

    /* Error handle */

    /* may need ituner to get header file */
    memset(&VideoRawEncVar, 0x0, sizeof(AMPUT_VIDEOTUNING_LVCAPTURE_BIN_s));
    sprintf(fn,"%s:\\LvCap_%04d.bin", DefaultSlot, (int)inputID);

    VideoRawEncVar.currRawFP = UT_VideoTuningfopen((const char *)fn, (const char *)mdASCII);
    AmbaPrint("[UT_VdoRawEncEnable]Read Binary(%d) from %s Start!", \
            sizeof(AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s), fn);

    retRd = UT_VideoTuningfread((void *)&(VideoRawEncVar.rawBin), 1, sizeof(AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s), (void *)VideoRawEncVar.currRawFP);
    if (retRd != sizeof(AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s)) {
        AmbaPrintColor(RED,"[UT_VdoRawEncEnable]Read RawBinary fail (%llu)", retRd);
    }
    UT_VideoTuningfclose((void *)VideoRawEncVar.currRawFP);

    VideoRawEncVar.currBinaryIndex = inputID;

    AmbaPrint("[UT_VdoRawEnc]====== Binary Read ======");
    AmbaPrint("[UT_VdoRawEnc] Raw Number : %04d", VideoRawEncVar.rawBin.CaptureNum);
    AmbaPrint("[UT_VdoRawEnc]      Pitch : %04d", VideoRawEncVar.rawBin.RawPitch);
    AmbaPrint("[UT_VdoRawEnc]      Width : %04d", VideoRawEncVar.rawBin.RawWidth);
    AmbaPrint("[UT_VdoRawEnc]     Height : %04d", VideoRawEncVar.rawBin.RawHeight);
    if (VideoRawEncVar.rawBin.IsYuv) {
        AmbaPrint("[UT_VdoRawEnc]       Type : %s", (VideoRawEncVar.rawBin.RawType == AMP_STILLENC_RAW_COMPR)? "compr": "uncompr");
    } else {
        AmbaPrint("[UT_VdoRawEnc]       Type : %s", (VideoRawEncVar.rawBin.YuvFmt == AMP_YUV_420)? "420": "422");
    }
    AmbaPrint("[UT_VdoRawEnc]  TimeScale : %d", VideoRawEncVar.rawBin.TimeScale);
    AmbaPrint("[UT_VdoRawEnc]       Tick : %d", VideoRawEncVar.rawBin.TickPerPicture);
    AmbaPrint("[UT_VdoRawEnc]         AR : 0x%X", VideoRawEncVar.rawBin.AspectRatio);
    AmbaPrint("[UT_VdoRawEnc]=========================");
    return 0;
}

/**
 * UnitTest: simple Video RawEncode start(ie manual videoEncode with external raw input)
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_VideoTuning_VideoRawEncStart(void)
{
    /* Error handle */
    if (VideoRawEncVar.rawBin.CaptureNum == 0) {
        AmbaPrint("[UT_VdoRawEncEnable] Raw Number Error!!");
    }

    VideoRawEncVar.currRawIndex = 0;

#ifdef CONFIG_SOC_A9
    //
    // Note: For A9, Bitstream-specific configs can be assigned here (before encode start), or when codec instance creation.
    //       But for A7L/A12 family, it is better to assign them when codec instance creation.
    //
    {
        AMP_VIDEOENC_H264_CFG_s *h264Cfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s headerInfo;
        AMP_VIDEOENC_BITSTREAM_CFG_s bitsCfg = {0};

        bitsCfg.VideoThumbnail = EncThumbnail;

        // Assign bitstream-specific configs
        bitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
        h264Cfg = &bitsCfg.Spec.H264Cfg;
        h264Cfg->GopM = 1;
        h264Cfg->GopN = GOP_N;
        h264Cfg->GopIDR = GOP_IDR;
        h264Cfg->GopHierarchical = 0;
        h264Cfg->Cabac = 1;
        h264Cfg->QualityControl.LoopFilterEnable = 1;
        h264Cfg->QualityControl.LoopFilterAlpha = 0;
        h264Cfg->QualityControl.LoopFilterBeta = 0;
        h264Cfg->StartFromBFrame = 1;
        h264Cfg->Interlace = VideoRawEncVar.rawBin.Interlace;
        h264Cfg->TimeScale = VideoRawEncVar.rawBin.TimeScale;
        h264Cfg->TickPerPicture = VideoRawEncVar.rawBin.TickPerPicture;
        h264Cfg->AuDelimiterType = 1;
        h264Cfg->QualityLevel = 0x94; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
        h264Cfg->StopMethod = AMP_VIDEOENC_STOP_NEXT_IP;
        headerInfo.GopM = h264Cfg->GopM;
        headerInfo.Width = VideoRawEncVar.rawBin.RawWidth;
        headerInfo.Height = VideoRawEncVar.rawBin.RawHeight;
        headerInfo.Interlace = 0;
        headerInfo.Rotation = (AMP_ROTATION_e)0;

        // Use default SPS/VUI
        AmpVideoEnc_GetDefaultH264Header(&headerInfo, &h264Cfg->SPS, &h264Cfg->VUI);
        h264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

        // Bitrate control
        h264Cfg->BitRateControl.BrcMode = VIDEOENC_CBR;
        h264Cfg->BitRateControl.AverageBitrate = 12 * 1E6;
        if (h264Cfg->BitRateControl.BrcMode == VIDEOENC_SMART_VBR) {
            h264Cfg->BitRateControl.MaxBitrate = 18 * 1E6;
            h264Cfg->BitRateControl.MinBitrate = 6 * 1E6;
        }
        h264Cfg->QualityControl.IBeatMode = 0;
        AmpVideoEnc_SetBitstreamConfig(VideoTuningPri, &bitsCfg);

        //
        // Setup bitstream buffer.
        //
        // Rule: H.264 and MJPEG can't use the same bitstream/descriptor buffer. Same Spec uses the same buffer. No matter it is primary or secondary
        // Note: Since buffer allocation depends on the above rule, it is better to assign bitstream buffer before encode start.
        //       Otherwise you have to know what you are going to encode when codec instance creation
        {
            AMP_ENC_BITSBUFFER_CFG_s  bitsBufCfg = {0};

            bitsBufCfg.BitsBufAddr = H264BitsBuf;
            bitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
            bitsBufCfg.DescBufAddr = H264DescBuf;
            bitsBufCfg.DescBufSize = DESC_SIZE;
            bitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            AmpVideoEnc_SetBitstreamBuffer(VideoTuningPri, &bitsBufCfg);

            AmbaPrint("H.264 Bits 0x%x size %d Desc 0x%x size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
        }
    }
#endif

    if (VideoTuningVirtualPriFifoHdlr == NULL) {
        AMP_FIFO_CFG_s FifoDefCfg = {0};

        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&FifoDefCfg);
        FifoDefCfg.hCodec = VideoTuningPri;
        FifoDefCfg.IsVirtual = 1;
        FifoDefCfg.NumEntries = 1024;
        FifoDefCfg.cbEvent = AmpUT_VideoTuning_FifoCB;
        AmpFifo_Create(&FifoDefCfg, &VideoTuningVirtualPriFifoHdlr);
    }

    AmpEnc_StartRecord(VideoTuningPipe, 0);

#ifdef CONFIG_SOC_A12
    if (TuningEncThumbnail) {
        //issue PIV cmd to let SSP start turely video-encode
        AMP_VIDEOENC_PIV_CTRL_s PivCtrl = {0};
        UINT32 iso = 2, TargetSize = 0;
        UINT8 Compression = 1, EncodeLoop = 0;
        ITUNER_SYSTEM_s SystemInfo = {0};
        UINT16 AspectRatio = 0;

        AmbaTUNE_Get_SystemInfo(&SystemInfo);

        TuningPivVideoThm = 1;
        TuningPivVideoThmJpgCount = 3; //TBD
        PivCtrl.SensorMode.Data = SystemInfo.SensorReadoutMode;
        PivCtrl.CaptureWidth = SystemInfo.RawWidth;
        PivCtrl.CaptureHeight = SystemInfo.RawHeight;
        PivCtrl.MainWidth = SystemInfo.MainWidth;
        PivCtrl.MainHeight = SystemInfo.MainHeight;
        VideoTuning_AspectRatioGet(SystemInfo.RawWidth, SystemInfo.RawHeight, &AspectRatio);
        PivCtrl.AspectRatio = AspectRatio;
        if (TuningLiveViewAlgoMode == 0) {
            iso = 2;
        } else if (TuningLiveViewAlgoMode == 1) {
            iso = 1;
        } else if (TuningLiveViewAlgoMode == 2) {
            iso = 0;
        }
        AmpUT_VideoTuning_PIV(PivCtrl, iso, Compression, TargetSize, EncodeLoop);
    }
#endif

    return 0;
}

/**
 * UnitTest: simple Video RawEncode next step(ie manual videoEncode with external raw/yuv input)
 *
 * @param [in] isLast lastFrame or not
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_VideoTuning_VideoRawEncNextStep(UINT8 isLast)
{
#ifdef CONFIG_SOC_A9
    char fn[VIDEORAWENC_MAX_FN_SIZE];
#endif
    char MdASCII[3] = {'r','b','\0'};
    UINT8 *BufAddr = NULL, *BufLimit = NULL, *RawAddr = NULL;
    UINT8 LastFrame = 0;
    UINT32 BufSize = 0, RawNum = 0, i;
    UINT64 RetRd = 0, SizeRd = 0;
    AMP_ENC_RAW_INFO_s RawBuf = {0};

    VideoRawEncVar.currRawIndex++;
    if (VideoRawEncVar.rawBin.CaptureNum < VideoRawEncVar.currRawIndex) {
        AmbaPrint("[UT_VdoRawEncNextStep] Wrong RawIndex(%d) Total(%d)", \
            VideoRawEncVar.currRawIndex, VideoRawEncVar.rawBin.CaptureNum);
    } else if (VideoRawEncVar.rawBin.CaptureNum == VideoRawEncVar.currRawIndex || isLast) {
        LastFrame = 1;
    }

    /* may need ituner to get rawIndex */
#ifdef CONFIG_SOC_A9
    sprintf(fn,"%s:\\LvCap_%04d_%04d.RAW", DefaultSlot, (int)VideoRawEncVar.currBinaryIndex, VideoRawEncVar.currRawIndex);
#endif

#ifdef CONFIG_SOC_A9
    VideoRawEncVar.currRawFP = UT_VideoTuningfopen((const char *)fn, (const char *)mdASCII);
#else
    VideoRawEncVar.currRawFP = UT_VideoTuningfopen((const char *)VideoRawEncVar.currRawPath, (const char *)MdASCII);
#endif
    AmpUT_VideoTuning_DspWork_Calculate(&BufAddr, &BufSize);

    BufLimit = BufAddr + BufSize;
    SizeRd = (UINT64)(VideoRawEncVar.rawBin.RawPitch*VideoRawEncVar.rawBin.RawHeight);
    RawNum = (BufLimit - VideoRawBuffAddr)/SizeRd;
    RawAddr = VideoRawBuffAddr + ((VideoRawEncVar.currRawIndex-1)%RawNum)*SizeRd;

#ifdef CONFIG_SOC_A9
    AmbaPrint("[%s]Read Raw(%llu) to 0x%X from %s Start!", __FUNCTION__, SizeRd, RawAddr, fn);
#else
    AmbaPrint("[%s]Read Data(%llu) to 0x%X from %s Start!", __FUNCTION__, SizeRd, RawAddr, VideoRawEncVar.currRawPath);
#endif
    if (VideoRawEncVar.rawBin.RawType == AMP_STILLENC_RAW_UNCOMPR && \
        VideoRawEncVar.rawBin.RawPitch != VideoRawEncVar.rawBin.RawWidth*2) {

        for (i=0; i<VideoRawEncVar.rawBin.RawHeight; i++) {
            RetRd = UT_VideoTuningfread((void *)RawAddr, 1, VideoRawEncVar.rawBin.RawWidth*2, (void *)VideoRawEncVar.currRawFP);
            RawAddr += VideoRawEncVar.rawBin.RawPitch;
        }
        if (RetRd != VideoRawEncVar.rawBin.RawWidth*2*VideoRawEncVar.rawBin.RawHeight) {
            AmbaPrintColor(RED, "Read Raw fail (%llu)", RetRd);
        } else {
            UT_VideoTuningfsync((void *)VideoRawEncVar.currRawFP);
            UT_VideoTuningfclose((void *)VideoRawEncVar.currRawFP);
            AmbaPrint("Read Raw Done!");
        }
    } else {
        RetRd = UT_VideoTuningfread((void *)RawAddr, 1, SizeRd, (void *)VideoRawEncVar.currRawFP);
        if (RetRd != SizeRd) {
            AmbaPrintColor(RED, "Read Raw fail (%llu)", RetRd);
        } else {
            UT_VideoTuningfsync((void *)VideoRawEncVar.currRawFP);
            UT_VideoTuningfclose((void *)VideoRawEncVar.currRawFP);
            AmbaPrint("Read Raw Done!");
        }
    }
    AmbaCache_Clean((void *)RawAddr, SizeRd);

    if (VideoRawEncVar.rawBin.IsYuv == 0) {
        RawBuf.compressed = (VideoRawEncVar.rawBin.RawType == AMP_STILLENC_RAW_COMPR)? 1: 0;
    }
    RawBuf.RawAddr = RawAddr;
    RawBuf.RawWidth = VideoRawEncVar.rawBin.RawWidth;
    RawBuf.RawHeight = VideoRawEncVar.rawBin.RawHeight;
    RawBuf.RawPitch = VideoRawEncVar.rawBin.RawPitch;
    AmpVideoEnc_FeedEncodeRaw(&RawBuf, LastFrame);

    return 0;
}

/**
 * UnitTest: simple Video RawEncode next IK step(ie manual videoEncode with external Ituner input)
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_VideoTuning_VideoRawEncIKNextStep(void)
{
#ifdef CONFIG_SOC_A9
        return 0;
#else
    int CtxId = 0;
    UINT32 MainViewID = 0; //TBD
    AMBA_IMG_SCHDLR_EXP_s ExpInfo;
    AMBA_IMG_SCHDLR_EXP_INFO_s CurExpInfo;

    do {
        AMBA_ITUNER_PROC_INFO_s ProcInfo = {0};
        TUNE_Initial_Config_s InitialConfig = {0};
        TUNE_Load_Param_s Load_Param = {0};
        AmbaTUNE_Change_Parser_Mode(TEXT_TUNE);
        InitialConfig.Text.pBytePool = &G_MMPL;
        if (0 != AmbaTUNE_Init(&InitialConfig)) {
            AmbaPrint("Call AmbaTUNE_Init() Fail");
            break;
        }
        Load_Param.Text.FilePath = VideoRawEncVar.currTunerPath;
        if (0 != AmbaTUNE_Load_IDSP(&Load_Param)) {
            AmbaPrint("[%s], call AmbaTUNE_Load_IDSP(FilePath:%s) Fail", __func__, VideoRawEncVar.currTunerPath);
            break;
        }
        if (0 != AmbaTUNE_Get_ItunerInfo(&ItunerInfo)) {
            AmbaPrint("[%s], call AmbaTUNE_Get_ItunerInfo() Fail", __func__);
            break;
        }
        AmbaPrint("[%s], ItunerInfo AlgoMode %d, Pipe %d, FuncMode %d", __func__, \
                  ItunerInfo.TuningAlgoMode.AlgoMode,
                  ItunerInfo.TuningAlgoMode.Pipe,
                  ItunerInfo.TuningAlgoMode.FuncMode);
        memcpy(&ImgMode, &ItunerInfo.TuningAlgoMode, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
        CtxId = AmbaImgSchdlr_GetIsoCtxIndex(MainViewID, ImgMode.AlgoMode);
        if (CtxId >= 0) {
            ImgMode.ContextId = CtxId;
        }
        AmbaPrint("CtxId %d", ImgMode.ContextId);

        if (VideoRawEncIsoCfgIndex) {
            AmbaPrint("[%s] Execute", __func__);
            if (0 != AmbaTUNE_Execute_IDSP(&ImgMode, &ProcInfo)) {
                AmbaPrint("[%s], call AmbaTUNE_Execute_IDSP() Fail", __func__);
            }
        }
    } while (0);

    if (VideoRawEncIsoCfgIndex) {
        // following IsoCfg
        memset(&CurExpInfo, 0x0, sizeof(AMBA_IMG_SCHDLR_EXP_INFO_s));
        AmbaImgSchdlr_GetExposureInfo(MainViewID, &CurExpInfo);
        memcpy(&ExpInfo.Info, &CurExpInfo, sizeof(AMBA_IMG_SCHDLR_EXP_INFO_s));

        ExpInfo.Type = AMBA_IMG_SCHDLR_SET_TYPE_DIRECT_EXE; //No_Vin_Int
        ExpInfo.Info.AdjUpdated = 1;
        if (ItunerInfo.TuningModeExt == SINGLE_SHOT_MULTI_EXPOSURE_HDR) {
            ExpInfo.Info.HdrUpdated[0].Data = 0xFFFF;
        }
        memcpy(&ExpInfo.Info.Mode, &ImgMode, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
        AmbaImgSchdlr_SetExposure(MainViewID, &ExpInfo);
    }
    VideoRawEncIsoCfgIndex += 1;

    return 0;
#endif
}

/**
 * UnitTest: Video Encode LiveView Capture raw PostProc Callback
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoTuning_LvCaptureRawPostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    if (info->StageCnt == 1) {
        char Fn[64];
        char MdASCII[3] = {'w','+','\0'};
        AMP_CFS_FILE_s *Raw = NULL;

        if (SubRawFnoLvCap == 1 && IsFastCapture == 0) {
            UINT64 FileSize = 0;
            AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s Bin = {0};

            Bin.CaptureNum = LvCaptureNum;
            Bin.RawPitch = info->media.RawInfo.RawPitch;
            Bin.RawWidth = info->media.RawInfo.RawWidth;
            Bin.RawHeight = info->media.RawInfo.RawHeight;
            Bin.RawType = LvCaptureRawType;
            Bin.Interlace = VideoTuningMgt[TuningModeIdx].Interlace;
            Bin.TimeScale = VideoTuningMgt[TuningModeIdx].TimeScale;
            Bin.TickPerPicture = VideoTuningMgt[TuningModeIdx].TickPerPicture;
            Bin.AspectRatio = VideoTuningMgt[TuningModeIdx].AspectRatio;

            sprintf(Fn,"%s:\\LvCap_%04d.bin", DefaultSlot, FnoLvCap);

            Raw = UT_VideoTuningfopen((const char *)Fn, (const char *)MdASCII);
            FileSize = UT_VideoTuningfwrite((const void *)&Bin, 1, sizeof(AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s), (void *)Raw);
            if (FileSize != (UINT64)sizeof(AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s)) {
                AmbaPrint("[Amp_UT]Dump bin Error(%llu)", FileSize);
            } else {
                AmbaPrint("[Amp_UT]Dump bin(%d) to %s", sizeof(AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s), Fn);
            }
            UT_VideoTuningfsync((void *)Raw);
            UT_VideoTuningfclose((void *)Raw);
        }

        if (VideoTuningDumpRawSkip == 0 && IsFastCapture == 0) {
            //raw ready, dump it
            sprintf(Fn,"%s:\\LvCap_%04d_%04d.RAW", DefaultSlot, FnoLvCap, SubRawFnoLvCap);

            Raw = UT_VideoTuningfopen((const char *)Fn, (const char *)MdASCII);
            AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d  to %s", \
                info->media.RawInfo.RawAddr, \
                info->media.RawInfo.RawPitch, \
                info->media.RawInfo.RawHeight, Fn);
            UT_VideoTuningfwrite((const void *)info->media.RawInfo.RawAddr, 1, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight, (void *)Raw);
            UT_VideoTuningfsync((void *)Raw);
            UT_VideoTuningfclose((void *)Raw);
            SubRawFnoLvCap++;
        } else if (VideoTuningDumpRawSkip == 0 && IsFastCapture == 1) {
            VideoTuningRawAddr[VideoTuningRawIndex] = info->media.RawInfo.RawAddr;
            AmbaPrint("[Amp_UT]Raw Addr 0x%X", VideoTuningRawAddr[VideoTuningRawIndex]);
            VideoTuningRawIndex++;
        }

        if (VideoTuningDumpItuner == 1) {
            char TuningModeExt[32] = "SINGLE_SHOT";
            char RawPath[64];
            char ItunerFileName[64];

            snprintf(RawPath, sizeof(RawPath), "%s:\\%04d.RAW", DefaultSlot, SubRawFnoLvCap);
            snprintf(ItunerFileName, sizeof(ItunerFileName), "%s:\\A12_%04d.txt", DefaultSlot, SubRawFnoLvCap);
            AmpUT_VideoTuningDumpItunerFile(ItunerFileName, TuningModeExt, RawPath);
        }
    } else {
        //nothing to do
    }

    return 0;
}

/**
 * UnitTest: Video Encode LiveView Capture raw PreProc Callback
 *
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoTuning_LvCaptureRawPreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    //nothing to do
    return 0;
}

/**
 * UnitTest: Video Encode LiveView Capture raw and yuv PostProc Callback
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoTuning_LvCaptureRawAndYuvPostCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    if (info->StageCnt == 1) {
        char fn[64];
        char mdASCII[3] = {'w','+','\0'};
        AMP_CFS_FILE_s *raw = NULL;

        UINT8 dumpRawSkip = 0;

        if (SubRawFnoLvCap == 1) { // 1st raw done, dump bin
            UINT64 FileSize;
            AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s Bin;
            Bin.CaptureNum = LvCaptureNum;
            Bin.RawPitch = info->media.RawInfo.RawPitch;
            Bin.RawWidth = info->media.RawInfo.RawWidth;
            Bin.RawHeight = info->media.RawInfo.RawHeight;
            Bin.RawType = LvCaptureRawType;
            Bin.Interlace = VideoTuningMgt[TuningModeIdx].Interlace;
            Bin.TimeScale = VideoTuningMgt[TuningModeIdx].TimeScale;
            Bin.TickPerPicture = VideoTuningMgt[TuningModeIdx].TickPerPicture;
            Bin.AspectRatio = VideoTuningMgt[TuningModeIdx].AspectRatio;
            sprintf(fn,"%s:\\LvCap_%04d.bin", DefaultSlot, FnoLvCap);

            raw = UT_VideoTuningfopen((const char *)fn, (const char *)mdASCII);
            FileSize = UT_VideoTuningfwrite((const void *)&Bin, 1, sizeof(AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s), (void *)raw);
            if (FileSize != (UINT64)sizeof(AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s)) {
                AmbaPrint("[Amp_UT]Dump bin Error(%llu)", FileSize);
            } else {
                AmbaPrint("[Amp_UT]Dump bin(%d) to %s", sizeof(AMPUT_VIDEOETUNING_LVCAPTURE_BIN_s), fn);
            }
            UT_VideoTuningfsync((void *)raw);
            UT_VideoTuningfclose((void *)raw);
        }
        if (dumpRawSkip == 0) {
            sprintf(fn,"%s:\\LvCap_%04d_%04d.RAW", DefaultSlot, FnoLvCap, SubRawFnoLvCap);

            raw = UT_VideoTuningfopen((const char *)fn, (const char *)mdASCII);
            AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d  to %s", \
                info->media.RawInfo.RawAddr, \
                info->media.RawInfo.RawPitch, \
                info->media.RawInfo.RawHeight, fn);
            UT_VideoTuningfwrite((const void *)info->media.RawInfo.RawAddr, 1, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight, (void *)raw);
            UT_VideoTuningfsync((void *)raw);
            UT_VideoTuningfclose((void *)raw);
        }
        SubRawFnoLvCap++;
    } else if (info->StageCnt == 2) {
        static UINT8 YuvFlag = 0;
        char fn[MAX_FILENAME_LENGTH];
        char fn1[MAX_FILENAME_LENGTH];
        UINT8 *lumaAddr = NULL, *chromaAddr = NULL;
        UINT16 pitch = 0, width = 0, height = 0;
        char mdASCII[3] = {'w','+','\0'};
        AMP_CFS_FILE_s *y = NULL;
        AMP_CFS_FILE_s *uv = NULL;
        UINT8 dumpYuvSkip = 0;

        if (info->media.YuvInfo.ThmLumaAddr) {
            sprintf(fn,"%s:\\LvCap_%04d_%04d_t.y", DefaultSlot, FnoLvCap, SubYuvFnoLvCap);
            sprintf(fn1,"%s:\\LvCap_%04d_%04d_t.uv", DefaultSlot, FnoLvCap, SubYuvFnoLvCap);
            lumaAddr = info->media.YuvInfo.ThmLumaAddr;
            chromaAddr = info->media.YuvInfo.ThmChromaAddr;
            pitch = info->media.YuvInfo.ThmPitch;
            width = info->media.YuvInfo.ThmWidth;
            height = 120;
        } else if (info->media.YuvInfo.ScrnLumaAddr) {
            sprintf(fn,"%s:\\LvCap_%04d_%04d_s.y", DefaultSlot, FnoLvCap, SubYuvFnoLvCap);
            sprintf(fn1,"%s:\\LvCap_%04d_%04d_s.uv", DefaultSlot, FnoLvCap, SubYuvFnoLvCap);
            lumaAddr = info->media.YuvInfo.ScrnLumaAddr;
            chromaAddr = info->media.YuvInfo.ScrnChromaAddr;
            pitch = info->media.YuvInfo.ScrnPitch;
            width = info->media.YuvInfo.ScrnWidth;
            height = info->media.YuvInfo.ScrnHeight;
        } else if (info->media.YuvInfo.LumaAddr) {
            sprintf(fn,"%s:\\LvCap_%04d_%04d_m.y", DefaultSlot, FnoLvCap, SubYuvFnoLvCap);
            sprintf(fn1,"%s:\\LvCap_%04d_%04d_m.uv", DefaultSlot, FnoLvCap, SubYuvFnoLvCap);
            lumaAddr = info->media.YuvInfo.LumaAddr;
            chromaAddr = info->media.YuvInfo.ChromaAddr;
            pitch = info->media.YuvInfo.Pitch;
            width = info->media.YuvInfo.Width;
            height = info->media.YuvInfo.Height;
        }

        if (dumpYuvSkip == 0) {
            AMP_AREA_s Aoi;

            Aoi.X = Aoi.Y = 0;
            Aoi.Width = width;
            Aoi.Height = height;
            AmbaPrint("[Amp_UT] Dump YUV(%s) (0x%X 0x%X) %d %d %d Start!", \
                (info->media.YuvInfo.DataFormat)?"422":"420", \
                lumaAddr, chromaAddr, pitch, width, height);
            y = UT_VideoTuningfopen((const char *)fn, (const char *)mdASCII);
            AmpUT_VideoTuningDumpAoi(lumaAddr, pitch, Aoi, (void *)y);
            UT_VideoTuningfsync((void *)y);
            UT_VideoTuningfclose((void *)y);

            uv = UT_VideoTuningfopen((const char *)fn1, (const char *)mdASCII);
            AmpUT_VideoTuningDumpAoi(chromaAddr, pitch, Aoi, (void *)uv);
            UT_VideoTuningfsync((void *)uv);
            UT_VideoTuningfclose((void *)uv);
        }

        if (info->media.YuvInfo.ThmLumaAddr) {
            YuvFlag |= 0x4;
        } else if (info->media.YuvInfo.ScrnLumaAddr) {
            YuvFlag |= 0x2;
        } else if (info->media.YuvInfo.LumaAddr) {
            YuvFlag |= 0x1;
        }

        if (YuvFlag == 0x7) {
            YuvFlag = 0;
            SubYuvFnoLvCap++;
            if (VideoTuningDumpItuner == 1) {
                char TuningModeExt[32] = "SINGLE_SHOT";
                char RawPath[64];
                char ItunerFileName[64];

                snprintf(RawPath, sizeof(RawPath), "%s:\\%04d.RAW", DefaultSlot, YuvFno);
                snprintf(ItunerFileName, sizeof(ItunerFileName), "%s:\\A12_%04d.txt", DefaultSlot, YuvFno);
                AmpUT_VideoTuningDumpItunerFile(ItunerFileName, TuningModeExt, RawPath);
            }
        }
    }
    return 0;
}

/**
 * UnitTest: Video Encode LiveView Capture raw and yuv PreProc Callback
 *
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoTuning_LvCaptureRawAndYuvPreCB(AMP_STILLENC_PREP_INFO_s *info)
{
    //nothing to do
    return 0;
}

/* CB for LiveView raw capture */
UINT32 AmpUT_VideoTuning_LvCapture_RawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl)
{
    AMP_STILLENC_RAWCAP_DSP_CTRL_s DspCtrl = {0};

    /* Stop LiveView */
    AmpStillEnc_EnableLiveviewCapture(AMP_STILL_STOP_LIVEVIEW, 0);

    DspCtrl.VinChan = ctrl->VinChan;
    DspCtrl.VidSkip = 0; //TBD
    DspCtrl.RawCapNum = ctrl->TotalRawToCap;
    DspCtrl.StillProc = 2; //hack as FastMode
    AmpStillEnc_StartRawCapture(VideoTuningSencPri, &DspCtrl);

    return 0;
}

UINT32 AmpUT_VideoTuning_LvCapture_MultiRawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl)
{
    AMP_STILLENC_RAWCAP_DSP_CTRL_s DspCtrl = {0};
    DspCtrl.RawCapNum = ctrl->TotalRawToCap;
    DspCtrl.VinChan = ctrl->VinChan;
    DspCtrl.StillProc = 2; //hack as FastMode
    DspCtrl.VidSkip = 0;
    AmpStillEnc_StartFollowingRawCapture(VideoTuningSencPri, &DspCtrl);
    return 0;
}

UINT32 AmpUT_VideoTuning_LvCapture_SensorPrep(AMBA_DSP_CHANNEL_ID_u vinChan,UINT32 aeIdx)
{
    // we don't need to config sensor under video raw capture
    return 0;
}


/**
 * UnitTest: simple LiveView Capture(ie rawcap in sequence)
 *
 * @param [in] capNum capture number
 * @param [in] cmpr compressed raw or not
 * @param [in] AMBA_SENSOR_MODE_ID_u sensor mode
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_VideoTuning_LiveviewCaptureRaw(UINT32 capNum, UINT8 cmpr, AMBA_SENSOR_MODE_ID_u sensorMode)
{
    int Er = 0;
    void *TempPtr = NULL, *ScriptAddrRaw = NULL;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT32 RawSize = 0;
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt = NULL;
    AMP_SENC_SCRPT_RAWCAP_s *RawCapScrpt = NULL;
    AMP_SCRPT_CONFIG_s Scrpt = {0};
    AMBA_SENSOR_MODE_INFO_s SensorInfo = {0};
#ifdef CONFIG_SOC_A9
    UINT8 *dspWorkingLimit = NULL;
    UINT8 dspBackgroundProcMode = AmpResource_GetDspBackgroundProcMode();
#endif

    LvCapture = 1;
    SubRawFnoLvCap = 1;
    IsRawCompr = cmpr;

    /* Stop Video LiveView first */
    AmpEnc_StopLiveview(VideoTuningPipe, AMP_ENC_FUNC_FLAG_WAIT);
    TuningStatus = TUNING_STATUS_IDLE;

    /* Phase I */
    /* check still codec status */
    if (VideoTuningSencInit == 0) { //not init yet
        if (AmpUT_VideoTuning_StillCodecInit() == OK) {
            VideoTuningSencInit = 1;
        }
    }

    if (VideoTuningSencPri == NULL) { //no codec be create
        AMP_STILLENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s Elayer = {0};

        EncCfg.MainLayout.Layer = &Elayer;
        AmpStillEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_VideoTuningCallback;

        // Assign main layout
        EncCfg.MainLayout.Width = VideoTuningMgt[TuningModeIdx].CaptureWidth;
        EncCfg.MainLayout.Height = VideoTuningMgt[TuningModeIdx].CaptureHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = VideoTuningMgt[TuningModeIdx].Interlace;
        EncCfg.MainTimeScale = VideoTuningMgt[TuningModeIdx].TimeScale;
        EncCfg.MainTickPerPicture = VideoTuningMgt[TuningModeIdx].TickPerPicture;
        EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_PERFORMANCE;
    #ifdef CONFIG_SOC_A12
        EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
        EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
        EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
    #endif
        {
            UINT8 *DspWorkAddr = NULL;
            UINT32 DspWorkSize = 0;
            AmpUT_VideoTuning_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
            EncCfg.DspWorkBufAddr = DspWorkAddr;
            EncCfg.DspWorkBufSize = DspWorkSize;
        }

        EncCfg.BitsBufCfg.BitsBufAddr = JpegBitsBuf;
        EncCfg.BitsBufCfg.BitsBufSize = BISFIFO_STILL_SIZE;
        EncCfg.BitsBufCfg.BitsRunoutThreshold = BISFIFO_STILL_SIZE - 4*1024*1024; // leave 4MB
        EncCfg.BitsBufCfg.DescBufAddr = JpegDescBuf;
        EncCfg.BitsBufCfg.DescBufSize = DESC_SIZE;
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", JpegBitsBuf, BISFIFO_STILL_SIZE, JpegDescBuf, DESC_SIZE);

        Elayer.SourceType = AMP_ENC_SOURCE_VIN;
        Elayer.Source = VideoTuningVinA;
        Elayer.SourceLayoutId = 0;
        Elayer.EnableSourceArea = 0; // No source cropping
        Elayer.EnableTargetArea = 0; // No target pip
        AmpStillEnc_Create(&EncCfg, &VideoTuningSencPri);
    }
    if (VideoTuningSencPipe == NULL) { //no pipeline be create
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};
        AmpEnc_GetDefaultCfg(&PipeCfg);
        PipeCfg.encoder[0] = VideoTuningSencPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_VideoTuningPipeCallback;
        PipeCfg.type = AMP_ENC_STILL_PIPE;
        AmpEnc_Create(&PipeCfg, &VideoTuningSencPipe);
        AmpEnc_Add(VideoTuningSencPipe);
    }

    /* Phase II */
    /* fill script and run */
    if (VideoTuningRawCapRunning) {
        AmbaPrint("Error status");
        goto _DONE;
    }

    /* Step1. calc raw and yuv buffer memory */
    AmbaSensor_GetModeInfo(TuningEncChannel, sensorMode, &SensorInfo);
    RawWidth = VideoTuningMgt[TuningModeIdx].CaptureWidth;
    RawHeight = VideoTuningMgt[TuningModeIdx].CaptureHeight;
    RawPitch = ALIGN_32((cmpr)? AMP_COMPRESSED_RAW_WIDTH(RawWidth): RawWidth*2);
    RawSize = RawPitch*RawHeight;
    AmbaPrint("[UT_videoTuning_LvCapture]raw(%u %u %u)", RawPitch, RawWidth, RawHeight);

    {
        extern UINT8 *DspWorkAreaResvStart;
        extern UINT32 DspWorkAreaResvSize;
        UINT8* DspWorkResvLimit = DspWorkAreaResvStart + DspWorkAreaResvSize;
        UINT8 *Addr, *BufAddr = 0;
        UINT32 Size = 0;
        UINT32 MaxRawNum = 0;
        AmpUT_VideoTuning_DspWork_Calculate(&Addr, &Size);
    #ifdef CONFIG_SOC_A9
        dspWorkingLimit = addr + size ;
    #endif
        if (IsFastCapture == 1) {
            BufAddr = Addr + (160<<20); // Assume DSP needs 160MB,,,
            MaxRawNum = (DspWorkResvLimit - BufAddr)/RawSize;
        } else if (IsFastCapture == 0) {
            BufAddr = Addr + Size;
            MaxRawNum = (DspWorkResvLimit - (BufAddr))/RawSize;
        }
        if (capNum == 0) {
            capNum = MaxRawNum;
        } else if (capNum > MaxRawNum) {
            AmbaPrintColor(RED,"[UT_videoTuning_LvCapture]raw buffer shortage, need %u, curr %u", \
                RawSize*capNum, DspWorkResvLimit - (Addr + Size));
            capNum = MaxRawNum;
        }
        AmbaPrint("[UT_videoTuning_LvCapture]RawBuffNum %d", capNum);

#ifdef CONFIG_SOC_A9
    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    if (dspBackgroundProcMode) {
        //Only borrow memory from DSP
        RawBuffAddr = dspWorkingLimit;
    } else {
        //In Mode_0 PIV, APP do not need to provide buffer memory
        RawBuffAddr = 0;
    }
#else
    #if 0
    er = AmbaKAL_BytePoolAllocate(&G_MMPL, &tempPtr, rawSize, 100);
    if (er != OK) {
        AmbaPrint("[UT_videoTuning_LvCapture]C_DDR alloc raw fail (%u)!", rawSize);
    } else {
        RawBuffAddr = (UINT8*)tempPtr;
        AmbaPrint("[UT_videoTuning_LvCapture]rawBuffAddr (0x%08X) (%u)!", RawBuffAddr, rawSize);
    }
    #endif
    RawBuffAddr = BufAddr;
#endif
    }
    AmbaPrint("[UT_videoTuning_LvCapture]raw_addr 0x%X", RawBuffAddr);
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &ScriptAddrRaw, ScriptSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_videoTuning_LvCapture]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            ScriptAddr = (UINT8*)TempPtr;
            AmbaPrint("[UT_videoTuning_LvCapture]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    StageAddr = ScriptAddr;
    GenScrpt = (AMP_SENC_SCRPT_GENCFG_s *)StageAddr;
    memset(GenScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    GenScrpt->Cmd = SENC_GENCFG;
    GenScrpt->RawEncRepeat = 0;
    GenScrpt->RawToCap = LvCaptureNum = capNum;
    GenScrpt->StillProcMode = 2; //hack as FastMode
    GenScrpt->QVConfig.DisableLCDQV = 1;
    GenScrpt->QVConfig.DisableHDMIQV = 1;
    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    GenScrpt->ScrnEnable = 0;
    GenScrpt->ThmEnable = 0;
    GenScrpt->PostProc = &PostVideoTuningLVCaptureRawCB;
    GenScrpt->PreProc = &PreVideoTuningLVCaptureRawCB;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoTuning_LvCapture]Stage_0 0x%08X", StageAddr);

    //raw cap config
    StageAddr = ScriptAddr + TotalScriptSize;
    RawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)StageAddr;
    memset(RawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    RawCapScrpt->Cmd = SENC_RAWCAP;
    RawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    RawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    RawCapScrpt->SensorMode = sensorMode;
    RawCapScrpt->FvRawCapArea.VcapWidth = RawWidth;
    RawCapScrpt->FvRawCapArea.VcapHeight = RawHeight;
    RawCapScrpt->FvRawCapArea.EffectArea.X = RawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    RawCapScrpt->FvRawCapArea.EffectArea.Width = RawCapScrpt->FvRawCapArea.VcapWidth;
    RawCapScrpt->FvRawCapArea.EffectArea.Height = RawCapScrpt->FvRawCapArea.VcapHeight;
    RawCapScrpt->FvRawType = LvCaptureRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    RawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    RawCapScrpt->FvRawBuf.Width = RawWidth;
    RawCapScrpt->FvRawBuf.Height = RawHeight;
    RawCapScrpt->FvRawBuf.Pitch = RawPitch;
    RawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_RING;
    RawCapScrpt->FvRingBufSize = RawSize*capNum;
    RawCapScrpt->CapCB.RawCapCB = AmpUT_VideoTuning_LvCapture_RawCapCB;
    RawCapScrpt->CapCB.RawCapMultiPrepCB = (VideoTuningEnableFastCapture == 0)? AmpUT_VideoTuning_LvCapture_SensorPrep: NULL;
    RawCapScrpt->CapCB.RawCapMultiCB = (VideoTuningEnableFastCapture == 0)? AmpUT_VideoTuning_LvCapture_MultiRawCapCB: NULL;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    TotalStageNum++;
    AmbaPrint("[UT_videoTuning_LvCapture]Stage_1 0x%X", StageAddr);

    //script config
    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[UT_videoTuning_LvCapture]scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    VideoTuningRawCapRunning = 1;

    /* Step4. execute script */
    AmpEnc_RunScript(VideoTuningSencPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step4. release script */
    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)ScriptAddrRaw) != OK) {
        AmbaPrint("memFree Fail (scrpt)");
    }
    ScriptAddr = NULL;
    AmbaPrint("memFree Done");

_DONE:
    return 0;
}

int AmpUT_VideoTuning_LiveviewCaptureRawAndYuv(UINT32 capNum, UINT8 cmpr, AMBA_SENSOR_MODE_ID_u sensorMode)
{
    int Er = 0;
    void *TempPtr = NULL;
    UINT8 Ar = 0;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
    UINT16 QvLCDW = 0, QvLCDH = 0, QvHDMIW = 0, QvHDMIH = 0;
    UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0, QvLCDSize = 0, QvHDMISize = 0;
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt = NULL;
    AMP_SENC_SCRPT_RAWCAP_s *RawCapScrpt = NULL;
    AMP_SENC_SCRPT_RAW2YUV_s *Raw2YuvScrpt = NULL;
    AMP_SENC_SCRPT_YUV2JPG_s *Yuv2JpgScrpt = NULL;
    AMP_SCRPT_CONFIG_s Scrpt = {0};
    AMBA_SENSOR_MODE_INFO_s SensorInfo = {0};
    void *ScriptAddrRaw = NULL;
#ifdef CONFIG_SOC_A9
    UINT8 *dspWorkingLimit = NULL;
#endif

    LvCapture = 1;
    SubRawFnoLvCap = 1;
    SubYuvFnoLvCap = 1;
    IsRawCompr = cmpr;

    /* Phase 0: sanity check */
    if (VideoTuningRawCapRunning) {
        AmbaPrint("Error status");
        goto _DONE;
    }

    /* Stop Video LiveView first */
    AmpEnc_StopLiveview(VideoTuningPipe, AMP_ENC_FUNC_FLAG_WAIT);
    TuningStatus = TUNING_STATUS_IDLE;

    /* Phase I: check StillCodec status */
    if (VideoTuningSencInit == 0) { // StillCodec NOT init yet
        if (AmpUT_VideoTuning_StillCodecInit() == OK) {
            VideoTuningSencInit = 1;
        }
    }

    if (VideoTuningSencPri == NULL) { // StillCodec NOT created
        AMP_STILLENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s Elayer = {0};

        EncCfg.MainLayout.Layer = &Elayer;
        AmpStillEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_VideoTuningCallback;
        // Assign main layout
        EncCfg.MainLayout.Width = VideoTuningMgt[TuningModeIdx].CaptureWidth;
        EncCfg.MainLayout.Height = VideoTuningMgt[TuningModeIdx].CaptureHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = VideoTuningMgt[TuningModeIdx].Interlace;
        EncCfg.MainTimeScale = VideoTuningMgt[TuningModeIdx].TimeScale;
        EncCfg.MainTickPerPicture = VideoTuningMgt[TuningModeIdx].TickPerPicture;

        {
            UINT8 *DspWorkAddr = NULL;
            UINT32 DspWorkSize = 0;
            AmpUT_VideoTuning_DspWork_Calculate(&DspWorkAddr, &DspWorkSize);
            EncCfg.DspWorkBufAddr = DspWorkAddr;
            EncCfg.DspWorkBufSize = DspWorkSize;
        }

        EncCfg.BitsBufCfg.BitsBufAddr = JpegBitsBuf;
        EncCfg.BitsBufCfg.BitsBufSize = BISFIFO_STILL_SIZE;
        EncCfg.BitsBufCfg.BitsRunoutThreshold = BISFIFO_STILL_SIZE - 4*1024*1024; // leave 4MB
        EncCfg.BitsBufCfg.DescBufAddr = JpegDescBuf;
        EncCfg.BitsBufCfg.DescBufSize = DESC_SIZE;
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", H264BitsBuf, BISFIFO_STILL_SIZE, JpegDescBuf, DESC_SIZE);

        Elayer.SourceType = AMP_ENC_SOURCE_VIN;
        Elayer.Source = VideoTuningVinA;
        Elayer.SourceLayoutId = 0;
        Elayer.EnableSourceArea = 0;  // No source cropping
        Elayer.EnableTargetArea = 0;  // No target pip
        AmpStillEnc_Create(&EncCfg, &VideoTuningSencPri);

        if (VideoTuningVirtualJpegFifoHdlr == NULL) {
            AMP_FIFO_CFG_s FifoDefCfg = {0};
            // create a virtual fifo
            AmpFifo_GetDefaultCfg(&FifoDefCfg);
            FifoDefCfg.hCodec = VideoTuningSencPri;
            FifoDefCfg.IsVirtual = 1;
            FifoDefCfg.NumEntries = 1024;
            FifoDefCfg.cbEvent = AmpUT_VideoTuning_FifoCB;
            AmpFifo_Create(&FifoDefCfg, &VideoTuningVirtualJpegFifoHdlr);
        }
    }

    if (VideoTuningSencPipe == NULL) { // VideoTuning pipeline NOT init
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};
        AmpEnc_GetDefaultCfg(&PipeCfg);
        PipeCfg.encoder[0] = VideoTuningSencPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_VideoTuningPipeCallback;
        PipeCfg.type = AMP_ENC_STILL_PIPE;
        AmpEnc_Create(&PipeCfg, &VideoTuningSencPipe);
        AmpEnc_Add(VideoTuningSencPipe);
    }

    /* Phase II: Memory allocation */
    {
    #ifdef CONFIG_SOC_A12
        extern int AmpUT_GetYuvWorkingBuffer(UINT16 MainWidth, UINT16 MainHeight, UINT16 RawWidth, UINT16 RawHeight, UINT16 *BufWidth, UINT16 *BufHeight);
        Ar = (VideoTuningMgt[TuningModeIdx].AspectRatio == VAR_4x3)?1:0;
        AmbaSensor_GetModeInfo(TuningEncChannel, sensorMode, &SensorInfo);
        RawWidth = VideoTuningMgt[TuningModeIdx].CaptureWidth;
        RawHeight = VideoTuningMgt[TuningModeIdx].CaptureHeight;
        RawPitch = ALIGN_32((cmpr)? AMP_COMPRESSED_RAW_WIDTH(RawWidth): RawWidth*2);
        RawSize = (UINT32) RawPitch*RawHeight;

        YuvWidth = VideoTuningMgt[TuningModeIdx].MainWidth;
        YuvHeight = VideoTuningMgt[TuningModeIdx].MainHeight;
        AmpUT_GetYuvWorkingBuffer(YuvWidth, YuvHeight, RawWidth, RawHeight, &YuvWidth, &YuvHeight);
        YuvSize = (UINT32)(YuvWidth*YuvHeight*2);
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv] yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);
        ScrnW = ALIGN_32(VideoTuningMgt[TuningModeIdx].ScrnWidth);
        ScrnH = VideoTuningMgt[TuningModeIdx].ScrnHeight;
        ScrnSize = (UINT32)(ScrnW*ScrnH*2);
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv] scrn(%u %u %u)!", ScrnW, ScrnH, ScrnSize);
        ThmW = ALIGN_32(VideoTuningMgt[TuningModeIdx].ThmWidth);
        ThmH = VideoTuningMgt[TuningModeIdx].ThmHeight;
        ThmSize = (UINT32)(ThmW*ThmH*2);
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv] thm(%u %u %u)!", ThmW, ThmH, ThmSize);
        QvLCDW = 960;
        QvLCDH = Ar? 480: 360;
        QvHDMIW = Ar? 1440: 1920;
        QvHDMIH = 1080;
        QvLCDSize = QvLCDW*QvLCDH*2;
        QvLCDSize += (QvLCDSize*10)/100;
        QvHDMISize = QvHDMIW*QvHDMIH*2;
        QvHDMISize += (QvHDMISize*10)/100;
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv] qvLCD(%u) qvHDMI(%u)!", QvLCDSize, QvHDMISize);
#else
        RawWidth = VideoTuningMgt[TuningModeIdx].CaptureWidth;
        RawHeight = VideoTuningMgt[TuningModeIdx].CaptureHeight;
        RawPitch = ALIGN_32((cmpr)? AMP_COMPRESSED_RAW_WIDTH(RawWidth): RawWidth*2);
        RawSize = (UINT32) RawPitch*RawHeight;
#endif
    }

#ifdef _STILL_BUFFER_FROM_DSP_
    {
        extern UINT8 *DspWorkAreaResvStart;
        extern UINT32 DspWorkAreaResvSize;
        UINT8 *Addr = NULL, *BufAddr = NULL, *DspWorkResvLimit = DspWorkAreaResvStart + DspWorkAreaResvSize;
        UINT32 Size = 0, MaxRawNum = 0;
        AmpUT_VideoTuning_DspWork_Calculate(&Addr, &Size);
    #ifdef CONFIG_SOC_A9
        dspWorkingLimit = Addr + Size ;
    #endif
        /*
         *    |------------------------------------------------------------------------------|
         *    | YuvBuf | ScrnBuf | ThmBuf | QvLcd | QvHDMI |          RawBuf                 |
         *    |------------------------------------------------------------------------------|
         *  bufAddr
         */
        BufAddr = Addr + Size;
        YuvBuffAddr = BufAddr;
        BufAddr += YuvSize;
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]yuvBuffAddr (0x%08X)!", YuvBuffAddr);
        ScrnBuffAddr = BufAddr;
        BufAddr += ScrnSize;
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]scrnBuffAddr (0x%08X)!", ScrnBuffAddr);
        ThmBuffAddr = BufAddr;
        BufAddr += ThmSize;
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]thmBuffAddr (0x%08X)!", ThmBuffAddr);
        QvLCDBuffAddr = BufAddr;
        BufAddr += QvLCDSize;
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]qvLCDBuffaddr (0x%08X)!", QvLCDBuffAddr);
        QvHDMIBuffAddr = BufAddr;
        BufAddr += QvHDMISize;
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]qvHDMIBuffaddr (0x%08X)!", QvHDMIBuffAddr);

        MaxRawNum = (DspWorkResvLimit - BufAddr)/RawSize;
        RawBuffAddr = BufAddr;
        if (capNum == 0) {
            capNum = MaxRawNum;
        } else if (capNum > MaxRawNum) {
            AmbaPrintColor(RED, "[UT_videoTuning_LvCaptureRawAndYuv]raw buffer shortage, need %u, curr %u", \
                RawSize*capNum, DspWorkResvLimit - (Addr + Size));
            capNum = MaxRawNum;
        }
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv] RawBufAddr 0x%08X RawBufNum %d ", RawBuffAddr, capNum);
    }
#else
        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, &TempPtr, YuvSize, 100);
        if (Er != OK) {
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]C_DDR alloc yuv fail (%u)!", YuvSize);
        } else {
            YuvBuffAddr = (UINT8*)TempPtr;
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]YuvBuffAddr (0x%08X) (%u)!", YuvBuffAddr, YuvSize);
        }

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, &TempPtr, ScrnSize, 100);
        if (Er != OK) {
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]C_DDR alloc scrn fail (%u)!", ScrnSize);
        } else {
            ScrnBuffAddr = (UINT8*)TempPtr;
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]ScrnBuffAddr (0x%08X) (%u)!", ScrnBuffAddr, ScrnSize);
        }

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, &TempPtr, ThmSize, 100);
        if (Er != OK) {
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]C_DDR alloc thm fail (%u)!", ThmSize);
        } else {
            ThmBuffAddr = (UINT8*)TempPtr;
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]ThmBuffAddr (0x%08X) (%u)!", ThmBuffAddr, ThmSize);
        }

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, &TempPtr, QvLCDSize, 100);
        if (Er != OK) {
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]C_DDR alloc yuv fail (%u)!", QvLCDSize);
        } else {
            QvLCDBuffAddr = (UINT8*)TempPtr;
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]QvLCDBuffAddr (0x%08X) (%u)!", QvLCDBuffAddr, QvLCDSize);
        }

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, &TempPtr, QvHDMISize, 100);
        if (Er != OK) {
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]C_DDR alloc yuv fail (%u)!", QvHDMISize);
        } else {
            QvHDMIBuffAddr = (UINT8*)TempPtr;
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]QvHDMIBuffAddr (0x%08X) (%u)!", QvHDMIBuffAddr, QvHDMISize);
        }

        Er = AmbaKAL_BytePoolAllocate(&G_MMPL, &TempPtr, RawSize*capNum, 100);
        if (Er != OK) {
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]C_DDR alloc raw fail (%u)!", RawSize*capNum);
        } else {
            RawBuffAddr = (UINT8*)TempPtr;
            AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]RawBuffAddr (0x%08X) (%u)!", RawBuffAddr, RawSize*capNum);
        }
#endif
        {
            UINT32 ScriptSize = 0;
            ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                         ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                         ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                         ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

            Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &ScriptAddrRaw, ScriptSize, 32);
            if (Er != OK) {
                AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
            } else {
                ScriptAddr = (UINT8*)TempPtr;
                AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]scriptAddr (0x%08X) (%d)!", ScriptAddr, ScriptSize);
            }
        }

    /* Phase III: Fill-in script parameters */
        /*
         *            |------------|
         *    stage 0:|   GenCfg   |
         *            |------------|
         *    stage 1:|   RawCap   |
         *            |------------|
         *    stage 2:|  Raw2Yuv   |
         *            |------------|
         *    stage 3:|  Yuv2Jpg   |
         *            |------------|
         */

    /* Stage 0: General config */
    StageAddr = ScriptAddr;
    GenScrpt = (AMP_SENC_SCRPT_GENCFG_s *)StageAddr;
    memset(GenScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    GenScrpt->Cmd = SENC_GENCFG;
    GenScrpt->RawEncRepeat = 1; // More than one raw, need to do rawenc repeatedly.
    GenScrpt->RawEncRepeatStage = 2; // Repeat from R2Y
    GenScrpt->RawToCap = LvCaptureNum = capNum;
    GenScrpt->StillProcMode = 2; // FIXME: hack as FastMode
    GenScrpt->QVConfig.DisableLCDQV = 0;
    GenScrpt->QVConfig.DisableHDMIQV = 0;
    GenScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.LCDLumaAddr = QvLCDBuffAddr;
    GenScrpt->QVConfig.LCDChromaAddr = QvLCDBuffAddr + QvLCDSize/2;
    GenScrpt->QVConfig.LCDWidth = QvLCDW;
    GenScrpt->QVConfig.LCDHeight = QvLCDH;
    GenScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.HDMILumaAddr = QvHDMIBuffAddr;
    GenScrpt->QVConfig.HDMIChromaAddr = QvHDMIBuffAddr + QvHDMISize/2;
    GenScrpt->QVConfig.HDMIWidth = QvHDMIW;
    GenScrpt->QVConfig.HDMIHeight = QvHDMIH;
    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    GenScrpt->ScrnEnable = 1;
    GenScrpt->ThmEnable = 1;
    GenScrpt->MainBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->MainBuf.Width = GenScrpt->MainBuf.Pitch = YuvWidth;
    GenScrpt->MainBuf.Height = YuvHeight;
    GenScrpt->MainBuf.LumaAddr = YuvBuffAddr;
    GenScrpt->MainBuf.ChromaAddr = GenScrpt->MainBuf.LumaAddr + YuvSize/2;
    GenScrpt->MainBuf.AOI.X = 0;
    GenScrpt->MainBuf.AOI.Y = 0;
    GenScrpt->MainBuf.AOI.Width = VideoTuningMgt[TuningModeIdx].MainWidth;
    GenScrpt->MainBuf.AOI.Height = VideoTuningMgt[TuningModeIdx].MainHeight;

    GenScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ScrnBuf.Width = GenScrpt->ScrnBuf.Pitch = ScrnW;
    GenScrpt->ScrnBuf.Height = ScrnH;
    GenScrpt->ScrnBuf.LumaAddr = ScrnBuffAddr;
    GenScrpt->ScrnBuf.ChromaAddr = GenScrpt->ScrnBuf.LumaAddr + ScrnSize/2;
    GenScrpt->ScrnBuf.AOI.X = 0;
    GenScrpt->ScrnBuf.AOI.Y = 0;
    GenScrpt->ScrnBuf.AOI.Width = VideoTuningMgt[TuningModeIdx].ScrnWidthAct;
    GenScrpt->ScrnBuf.AOI.Height = VideoTuningMgt[TuningModeIdx].ScrnHeightAct;
    GenScrpt->ScrnWidth = VideoTuningMgt[TuningModeIdx].ScrnWidth;
    GenScrpt->ScrnHeight = VideoTuningMgt[TuningModeIdx].ScrnHeight;

    GenScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
    GenScrpt->ThmBuf.Width = GenScrpt->ThmBuf.Pitch = ThmW;
    GenScrpt->ThmBuf.Height = ThmH;
    GenScrpt->ThmBuf.LumaAddr = ThmBuffAddr;
    GenScrpt->ThmBuf.ChromaAddr = GenScrpt->ThmBuf.LumaAddr + ThmSize/2;
    GenScrpt->ThmBuf.AOI.X = 0;
    GenScrpt->ThmBuf.AOI.Y = 0;
    GenScrpt->ThmBuf.AOI.Width = VideoTuningMgt[TuningModeIdx].ThmWidthAct;
    GenScrpt->ThmBuf.AOI.Height = VideoTuningMgt[TuningModeIdx].ThmHeightAct;
    GenScrpt->ThmWidth = VideoTuningMgt[TuningModeIdx].ThmWidth;
    GenScrpt->ThmHeight = VideoTuningMgt[TuningModeIdx].ThmHeight;

    if (0/*targetSize*/) {
        extern UINT32 AmpUT_JpegBRCPredictCB(UINT16 targetBpp, UINT32 stillProc, UINT8* predictQ, UINT8 *curveNum, UINT32 *curveAddr);
        UINT32 TargetSize = 0, EncodeLoop = 0;
        AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]Target Size %u Kbyte", TargetSize);
        AmpUT_VidoeTuning_InitJpegDqt(VideoTuningJpegQTable[0], -1);
        AmpUT_VidoeTuning_InitJpegDqt(VideoTuningJpegQTable[1], -1);
        AmpUT_VidoeTuning_InitJpegDqt(VideoTuningJpegQTable[2], -1);
        GenScrpt->BrcCtrl.Tolerance = 10;
        GenScrpt->BrcCtrl.MaxEncLoop = EncodeLoop;
        GenScrpt->BrcCtrl.JpgBrcCB = AmpUT_JpegBRCPredictCB;
        GenScrpt->BrcCtrl.TargetBitRate = (((TargetSize<<13)/VideoTuningMgt[TuningModeIdx].MainWidth)<<12)/VideoTuningMgt[TuningModeIdx].MainHeight;
        GenScrpt->BrcCtrl.MainQTAddr = VideoTuningJpegQTable[0];
        GenScrpt->BrcCtrl.ThmQTAddr = VideoTuningJpegQTable[1];
        GenScrpt->BrcCtrl.ScrnQTAddr = VideoTuningJpegQTable[2];
    } else {
        AmpUT_VidoeTuning_InitJpegDqt(VideoTuningJpegQTable[0], 95);
        AmpUT_VidoeTuning_InitJpegDqt(VideoTuningJpegQTable[1], 95);
        AmpUT_VidoeTuning_InitJpegDqt(VideoTuningJpegQTable[2], 95);
        GenScrpt->BrcCtrl.Tolerance = 0;
        GenScrpt->BrcCtrl.MaxEncLoop = 0;
        GenScrpt->BrcCtrl.JpgBrcCB = NULL;
        GenScrpt->BrcCtrl.TargetBitRate = 0;
        GenScrpt->BrcCtrl.MainQTAddr = VideoTuningJpegQTable[0];
        GenScrpt->BrcCtrl.ThmQTAddr = VideoTuningJpegQTable[1];
        GenScrpt->BrcCtrl.ScrnQTAddr = VideoTuningJpegQTable[2];
    }

    GenScrpt->PostProc = &PostVideoTuningLVCaptureRawAndYuvCB;
    GenScrpt->PreProc = &PreVideoTuningLVCaptureRawAndYuvCB;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]Stage_0 0x%08X", StageAddr);

    /* Stage 1: Raw capture */
    StageAddr = ScriptAddr + TotalScriptSize;
    RawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)StageAddr;
    memset(RawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    RawCapScrpt->Cmd = SENC_RAWCAP;
    RawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    RawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    RawCapScrpt->SensorMode = sensorMode;
    RawCapScrpt->FvRawCapArea.VcapWidth = RawWidth;
    RawCapScrpt->FvRawCapArea.VcapHeight = RawHeight;
    RawCapScrpt->FvRawCapArea.EffectArea.X = RawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    RawCapScrpt->FvRawCapArea.EffectArea.Width = RawCapScrpt->FvRawCapArea.VcapWidth;
    RawCapScrpt->FvRawCapArea.EffectArea.Height = RawCapScrpt->FvRawCapArea.VcapHeight;
    RawCapScrpt->FvRawType = LvCaptureRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    RawCapScrpt->FvRawBuf.Buf = RawBuffAddr;
    RawCapScrpt->FvRawBuf.Width = RawWidth;
    RawCapScrpt->FvRawBuf.Height = RawHeight;
    RawCapScrpt->FvRawBuf.Pitch = RawPitch;
    RawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_RING;
    RawCapScrpt->FvRingBufSize = RawSize*capNum;
    RawCapScrpt->CapCB.RawCapCB = AmpUT_VideoTuning_LvCapture_RawCapCB;
    RawCapScrpt->CapCB.RawCapMultiPrepCB = (VideoTuningEnableFastCapture == 0)? AmpUT_VideoTuning_LvCapture_SensorPrep: NULL;
    RawCapScrpt->CapCB.RawCapMultiCB = (VideoTuningEnableFastCapture == 0)? AmpUT_VideoTuning_LvCapture_MultiRawCapCB: NULL;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]Stage_1 0x%X", StageAddr);

    /* Stage 2: Raw to yuv conversion */
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
    AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]Stage_2 0x%X", StageAddr);

    /* Stage 3: Yuv to Jpeg encode */
    StageAddr = ScriptAddr + TotalScriptSize;
    Yuv2JpgScrpt = (AMP_SENC_SCRPT_YUV2JPG_s *)StageAddr;
    memset(Yuv2JpgScrpt, 0x0, sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    Yuv2JpgScrpt->Cmd = SENC_YUV2JPG;
    Yuv2JpgScrpt->YuvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    Yuv2JpgScrpt->YuvRingBufSize = 0;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]Stage_3 0x%X", StageAddr);

    //script config
    Scrpt.mode = AMP_SCRPT_MODE_STILL;
    Scrpt.StepPreproc = NULL;
    Scrpt.StepPostproc = NULL;
    Scrpt.ScriptStartAddr = (UINT32)ScriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[UT_videoTuning_LvCaptureRawAndYuv]Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    VideoTuningRawCapRunning = 1;

    /* Phase IV: Start to run script */
    AmpEnc_RunScript(VideoTuningSencPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    AmbaPrint("[0x%08X] memFree", ScriptAddr);
    if (AmbaKAL_BytePoolFree((void *)ScriptAddrRaw) != OK) {
        AmbaPrint("memFree Fail (Scrpt)");
    }
    ScriptAddr = NULL;
    AmbaPrint("memFree Done");

_DONE:
    return 0;
}

int AmpUT_VideoTuningTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_VideoTuningTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        int LcdId = 0;
        if (TuningStatus != TUNING_STATUS_NO_INIT) {
            return -1;
        }
        if (argc < 3) {
            UINT8 i = 0;
            AmbaPrint("Usage: t videotuning init [SensorID][LcdID]");
            AmbaPrint("               SensorID:");
            for (i = 0; i < INPUT_DEVICE_NUM; i++) {
                AmbaPrint("                        %2d -- %s", i, MWUT_GetInputDeviceName(i));
            }
            AmbaPrint("               LcdID: 0 -- WDF9648W");
            return -1;
        } else if (argc == 3) {
            InputDeviceId = atoi(argv[2]);
        } else {
            InputDeviceId = atoi(argv[2]);
            LcdId = atoi(argv[3]);
        }
        AmpUT_VideoTuning_Init(InputDeviceId, LcdId);
        TuningStatus = TUNING_STATUS_IDLE;
        AmbaPrintColor(BLUE, "[AMP_UT] Init done %d", InputDeviceId);
    } else if (strcmp(argv[1],"lvproc") == 0 || strcmp(argv[1],"liveviewproc") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t videotuning liveviewproc [proc][Algo/OS]");
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

            TuningLiveViewProcMode = Proc;
            if (Proc == 0) {
                TuningLiveViewOSMode = Aux;
                TuningLiveViewAlgoMode = 0;
            } else {
                TuningLiveViewAlgoMode = Aux;
                TuningLiveViewOSMode = 0;
            }
            if (TuningLiveViewProcMode == 0) {
                AmbaPrint("Set LvProc = %s_%s", "Express", Aux?"OS":"Basic");
            } else {
                AmbaPrint("Set LvProc = %s_%s", "Hybrid",
                    (Aux==0)? "Fast": ((Aux==1)? "LISO": "HISO"));
            }
        }
    } else if ((strcmp(argv[1],"liveviewstart") == 0) || (strcmp(argv[1],"lvst") == 0)) {
        UINT32 SensorModeId;

        if (TuningStatus != TUNING_STATUS_IDLE) {
            return -1;
        }
        if (argc < 3) {
            AmbaPrint("Usage: t videotuning liveviewstart [modeId]");
            if (VideoTuningMgt == NULL) {
                AmbaPrintColor(RED, "Please hook sensor first!!!");
                return -1;
            }
            AmbaPrint("[%s] mode_id: ", MWUT_GetInputDeviceName(InputDeviceId));
            MWUT_InputVideoModePrintOutAll(VideoTuningMgt);
            return -1;
        }
        SensorModeId = atoi(argv[2]);
        if (VideoTuningMgt[SensorModeId].ForbidMode) {
            AmbaPrint("Not Suppot this Mode (%d)", SensorModeId);
            return -1;
        }
        if (VideoTuningMgt[SensorModeId].PALModeOnly && TuningEncodeSystem == 0)  {
            AmbaPrintColor(RED, "Only Support PAL mode !!!!!");
            return -1;
        }
        AmpUT_VideoTuning_LiveviewStart(SensorModeId);
        TuningStatus = TUNING_STATUS_LIVEVIEW;
        AmbaPrintColor(BLUE, "[AMP_UT] LVST %d", SensorModeId);
    } else if ((strcmp(argv[1],"encstop") == 0) || (strcmp(argv[1],"ensp") == 0)) {
        if (TuningStatus != TUNING_STATUS_ENCODE) {
            return -1;
        }
        AmpUT_VideoTuning_EncStop();
        TuningStatus = TUNING_STATUS_LIVEVIEW;
    } else if ((strcmp(argv[1],"liveviewstop") == 0) || (strcmp(argv[1],"lvsp") == 0)) {
        if (TuningStatus != TUNING_STATUS_LIVEVIEW) {
            return -1;
        }
        AmpUT_VideoTuning_LiveviewStop();
        TuningStatus = TUNING_STATUS_IDLE;
        AmbaPrintColor(BLUE, "[AMP_UT] LVSP");
    } else if ((strcmp(argv[1],"3a") == 0)) {
        UINT8 Is3aEnable = atoi(argv[2]);
        UINT32 ChNo = TuningEncChannel.Bits.VinID;
        AMBA_3A_OP_INFO_s AaaOpInfo = {0};
        if (Is3aEnable == 0) {
            VideoTuning3AEnable = 0;
            AaaOpInfo.AeOp = DISABLE;
            AaaOpInfo.AfOp = DISABLE;
            AaaOpInfo.AwbOp = DISABLE;
            AaaOpInfo.AdjOp = DISABLE;
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        } else if (Is3aEnable == 1) {
            VideoTuning3AEnable = 1;
            AaaOpInfo.AeOp = ENABLE;
            AaaOpInfo.AfOp = ENABLE;
            AaaOpInfo.AwbOp = ENABLE;
            AaaOpInfo.AdjOp = ENABLE;
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        }
        AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        AmbaPrint("3A %s: ae:%u af:%u awb:%u adj:%u", VideoTuning3AEnable? "Enable": "Disable", AaaOpInfo.AeOp, \
            AaaOpInfo.AfOp, AaaOpInfo.AwbOp, AaaOpInfo.AdjOp);
    }  else if ((strcmp(argv[1],"vinencchg") == 0) || (strcmp(argv[1],"chg") == 0)) {
        UINT32 NewSensorModeID;

        if (TuningStatus != TUNING_STATUS_LIVEVIEW) {
            return 0;
        }

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc vinencchg [modeId]");
            if (VideoTuningMgt == NULL) {
                AmbaPrintColor(RED, "Please hook sensor first!!!");
                return -1;
            }
            AmbaPrint("[%s] mode_id: ", MWUT_GetInputDeviceName(InputDeviceId));
            MWUT_InputVideoModePrintOutAll(VideoTuningMgt);
            return -1;
        }
        NewSensorModeID = atoi(argv[2]);
        if (VideoTuningMgt[NewSensorModeID].ForbidMode) {
            AmbaPrint("Not Suppot this Mode (%d)", NewSensorModeID);
            return -1;
        }
        if (VideoTuningMgt[NewSensorModeID].PALModeOnly && TuningEncodeSystem == 0)  {
            AmbaPrintColor(RED, "Only Support PAL mode !!!!!");
            return -1;
        }
        AmpUT_VideoTuning_ChangeResolution(NewSensorModeID);
        AmbaPrintColor(BLUE,"[AMP_UT] Mode change done %d", NewSensorModeID);
    } else if (strcmp(argv[1],"liveviewcapture") == 0 || strcmp(argv[1],"lvcap") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t videotuning liveviewcapture [config|start]");
            AmbaPrint("              config [en]: config liveview capture");
            //AmbaPrint("             query [cmpr]: query Max raw number");
            //AmbaPrint("                     cmpr: compressed raw or not");
            AmbaPrint("        start [num] [cmpr]: start liveview raw capture");
            AmbaPrint("                      num: number of raw to capture");
            AmbaPrint("                     cmpr: compressed raw or not");
            return 0;
        } else {
            if (strcmp(argv[2],"config") == 0 || strcmp(argv[2],"cfg") == 0) {
                if (argc < 4) {
                    AmbaPrint("Usage: t videotuning liveviewcapture config [en]");
                    AmbaPrint("                     en: enable liveview Capture or not");
                } else {
                    UINT8 enable = atoi(argv[3]);
                    if (enable) VideoTuningCaptureMode = TUNNING_MODE_LIVEVIEW_CAPTUE;
                    else VideoTuningCaptureMode = TUNNING_MODE_CAPTURE_NONE;
                    AmbaPrint("[VideoTuning] LiveViewCapture : %s", enable?"Capture":"None");
                }
            } else if (strcmp(argv[2],"fastcapture") == 0 || strcmp(argv[2],"fcap") == 0) {
                    if (argc < 4) {
                        AmbaPrint("Usage: t videotuning liveviewcapture fastcapture [en]");
                        AmbaPrint("                     en: enable fast capture or not");
                    } else {
                    #ifdef CONFIG_SOC_A12
                        extern UINT8 ForceSlowCapture;
                        VideoTuningEnableFastCapture = atoi(argv[3]);
                        ForceSlowCapture = (VideoTuningEnableFastCapture == 0)? 1: 0;
                        AmbaPrint("[VideoTuning] Fast capture : %d", VideoTuningEnableFastCapture);
                    #endif
                    }
            } else if (strcmp(argv[2],"query") == 0) {
                if (argc < 4) {
                    AmbaPrint("Usage: t videotuning liveviewcapture query [cmpr]");
                    AmbaPrint("                     cmpr: compressed raw or not");
                } else {
                    //TBD
                }
            } else if (strcmp(argv[2],"start") == 0 || strcmp(argv[2],"st") == 0) {
                if (argc < 4) {
                    AmbaPrint("Usage: t videotuning liveviewcapture start [num] [cmpr] [yuv]");
                    AmbaPrint("                      num: number of raw to capture, 0 means let system to decide maxRawNum");
                    AmbaPrint("                     cmpr: compressed raw or not");
                    AmbaPrint("                      yuv: dump yuv or not");
                } else {
                    if (VideoTuningCaptureMode == TUNNING_MODE_LIVEVIEW_CAPTUE) {
                        UINT32 CapNum = atoi(argv[3]);
                        UINT8 Cmpr = atoi(argv[4]);
                        UINT8 DumpYuv = atoi(argv[5]);
                        AMBA_SENSOR_MODE_ID_u Mode = {0};
                        if (DumpYuv == 0) {
                            AmbaPrint("[VideoTuning] LiveViewCapture for %s raw: Start", (Cmpr)? "compressed": "uncompressed");
                            Mode.Data = (UINT16)VideoTuningMgt[TuningModeIdx].InputMode;
                            AmpUT_VideoTuning_LiveviewCaptureRaw(CapNum, Cmpr, Mode);
                        } else {
                            AmbaPrint("[VideoTuning] LiveViewCapture for %s raw + yuv: Start", (Cmpr)? "compressed": "uncompressed");
                            Mode.Data = (UINT16)VideoTuningMgt[TuningModeIdx].InputMode;
                            AmpUT_VideoTuning_LiveviewCaptureRawAndYuv(CapNum, Cmpr, Mode);
                        }
                    } else {
                        AmbaPrintColor(RED, "Incorrect capture mode %u", VideoTuningCaptureMode);
                    }
                }
            }
        }
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
                //FIXME: %f issue
                //AmbaPrint("Manual Shutter time: %fms", ShutterTime);
            } else {
                //FIXME: %f issue
                //AmbaPrint("Shutter time: %fms should larger than 0", ShutterTime);
            }
            if (GainControl >= 1.0) {
                AmbaSensor_ConvertGainFactor(Chan, GainControl, &GainFactor, &AGainCtrl, &DGainCtrl);
                AmbaSensor_SetAnalogGainCtrl(Chan, AGainCtrl);
                AmbaSensor_SetDigitalGainCtrl(Chan, DGainCtrl);
                //FIXME: %f issue
                //AmbaPrint("Manual Gain control: %f", GainControl);
            } else {
                //FIXME: %f issue
                //AmbaPrint("Gain control: %f should larger than 1.0(0dB)", GainControl);
            }
        }
    } else if (strcmp(argv[1],"videorawenc") == 0 || strcmp(argv[1],"re") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t videotuning videorawenc [config|start|next|end]");
            AmbaPrint("                              config [InputID]: InputID, 0 means disable");
            AmbaPrint("                              start: start VideoRawEncode");
            AmbaPrint("                              frate: config encode frame rate");
            AmbaPrint("                              next [path]: execute next raw input with raw path");
        #ifdef CONFIG_SOC_A9
            AmbaPrint("                              last [path]: execute last raw input with raw path");
        #endif
            AmbaPrint("                              iknext [path]: execute next IK with ituner path");
            AmbaPrint("                              end: stop VideoRawEncode");
            return 0;
        } else {
            if (strcmp(argv[2],"config") == 0 || strcmp(argv[2],"cfg") == 0) {
                if (argc < 4) {
                    AmbaPrint("Usage: t videotuning videorawenc config [InputID]");
                    AmbaPrint("                     InputID: InputID, 0 means disable");
                } else {
                    UINT32 inputID = atoi(argv[3]);
                    if (inputID) {
                        VideoTuningCaptureMode = TUNNING_MODE_VIDEORAWENCODE;
                        AmpUT_VideoTuning_VideoRawEncEnable(inputID);
                    } else {
                        VideoTuningCaptureMode = TUNNING_MODE_CAPTURE_NONE;
                    }
                    AmbaPrint("[VideoTuning] VideoRawEncode : %s", inputID?"Enable":"Disable");
                }
            } else if (strcmp(argv[2],"start") == 0 || strcmp(argv[2],"st") == 0) {
                if (TuningStatus != TUNING_STATUS_LIVEVIEW) {
                    AmbaPrint("VideoRawEncodeStart at wrong state(%d)", TuningStatus);
                    return 0;
                }
                AmbaPrint("[VideoTuning] VideoRawEncode : Start");
                AmpUT_VideoTuning_VideoRawEncStart();
                TuningStatus = TUNING_STATUS_ENCODE;
                YuvFno = 0;
            } else if (strcmp(argv[2],"frate") == 0 || strcmp(argv[2],"fr") == 0) {
                if (TuningStatus != TUNING_STATUS_IDLE) {
                    AmbaPrint("Wrong state(%d)", TuningStatus);
                    return 0;
                } else {
                    UINT32 TimeScale = atoi(argv[3]);
                    UINT32 TickPerPicture = atoi(argv[4]);
                    UINT8  Interlace = atoi(argv[5]);

                    /* Reset RawEncVar for this reound */
                    VideoTuningCaptureMode = TUNNING_MODE_VIDEORAWENCODE;
                    memset(&VideoRawEncVar, 0x0, sizeof(AMPUT_VIDEOTUNING_LVCAPTURE_BIN_s));

                    if (argc == 3) {
                        // use default value
                        TimeScale = 30000;
                        TickPerPicture = 1001;
                        Interlace = 0;
                    }
                    VideoRawEncVar.rawBin.TimeScale = TimeScale;
                    VideoRawEncVar.rawBin.TickPerPicture = TickPerPicture;
                    VideoRawEncVar.rawBin.Interlace = Interlace;
                    AmbaPrint("[VideoTuning] VideoRawEncode : Enc FrameRate Config (%d %d %d)", \
                        TimeScale, TickPerPicture, Interlace);
                }
            } else if (strcmp(argv[2],"next") == 0 || strcmp(argv[2],"ne") == 0) {
                if (TuningStatus != TUNING_STATUS_ENCODE) {
                    AmbaPrint("VideoRawEncodeNextStep at wrong state(%d)", TuningStatus);
                    return 0;
                }
                if (argc == 3) {
                    if (VideoRawEncVar.currRawPath[0] == '\0') {
                        AmbaPrintColor(RED,"[VideoTuning] VideoRawEncode : should feed raw");
                        return -1;
                    } else {
                        AmbaPrint("[VideoTuning] VideoRawEncode : Next raw(%s)", VideoRawEncVar.currRawPath);
                    }
                } else if (argc == 4) {
                    strncpy(VideoRawEncVar.currRawPath, argv[3], VIDEORAWENC_MAX_FN_SIZE);
                    AmbaPrint("[VideoTuning] VideoRawEncode : Next raw(%s)", VideoRawEncVar.currRawPath);
                }
                AmpUT_VideoTuning_VideoRawEncNextStep(0);
        #ifdef CONFIG_SOC_A9
            } else if (strcmp(argv[2],"last") == 0) {
                if (TuningStatus != TUNING_STATUS_ENCODE) {
                    AmbaPrint("VideoRawEncodeNextStep at wrong state(%d)", TuningStatus);
                    return 0;
                }
                if (argc == 3) {
                    if (VideoRawEncVar.currRawPath[0] == '\0') {
                        AmbaPrintColor(RED,"[VideoTuning] VideoRawEncode : should feed raw");
                        return -1;
                    } else {
                        AmbaPrint("[VideoTuning] VideoRawEncode : Last raw(%s)", VideoRawEncVar.currRawPath);
                    }
                } else if (argc == 4) {
                    strncpy(VideoRawEncVar.currRawPath, argv[3], VIDEORAWENC_MAX_FN_SIZE);
                    AmbaPrint("[VideoTuning] VideoRawEncode : Last raw(%s)", VideoRawEncVar.currRawPath);
                }
                AmpUT_VideoTuning_VideoRawEncNextStep(1);
        #endif
            } else if (strcmp(argv[2],"iknext") == 0 || strcmp(argv[2],"ikne") == 0) {
                if (argc == 3) {
                    if (VideoRawEncVar.currTunerPath[0] == '\0') {
                        AmbaPrintColor(RED,"[VideoTuning] VideoRawEncode : should feed iTuner.txt");
                        return -1;
                    } else {
                        AmbaPrint("[VideoTuning] VideoRawEncode : IKNext(%s)", VideoRawEncVar.currTunerPath);
                    }
                } else if (argc == 4) {
                    strncpy(VideoRawEncVar.currTunerPath, argv[3], VIDEORAWENC_MAX_FN_SIZE);
                    AmbaPrint("[VideoTuning] VideoRawEncode : IKNext(%s)", VideoRawEncVar.currTunerPath);
                }
                AmpUT_VideoTuning_VideoRawEncIKNextStep();
            } else if (strcmp(argv[2],"end") == 0) {
                if (TuningStatus != TUNING_STATUS_ENCODE) {
                    AmbaPrint("AmpUT_VideoTuning_VideoRawEncStop at wrong state(%d)", TuningStatus);
                    return 0;
                }
                AmbaPrint("[VideoTuning] VideoRawEncode : End");
                AmpUT_VideoTuning_EncStop();
                TuningStatus = TUNING_STATUS_LIVEVIEW;
            } else if (strcmp(argv[2],"liveview") == 0 || strcmp(argv[2],"lv") == 0) {
                if (VideoTuningCaptureMode != TUNNING_MODE_VIDEORAWENCODE) {
                    AmbaPrint("[VideoTuning] VideoRawEncode : CaptureMode should be %d (%d)", \
                        TUNNING_MODE_VIDEORAWENCODE, VideoTuningCaptureMode);
                    return 0;
                } else {
                    AmpUT_VideoTuning_RawEncodeLiveviewStart();
                }
            } else if (strcmp(argv[2],"dumpyuv") == 0) {
                if (argc < 3) {
                    AmbaPrint("Usage: t videotuning dumpyuv [enable]");
                }
                DumpYuvEnable = atoi(argv[3]);
                AmbaPrint("DumpYuv: %s", DumpYuvEnable?"Enable":"Disable");
            } else if (strcmp(argv[2],"dumpmey") == 0) {
                if (argc < 3) {
                    AmbaPrint("Usage: t videotuning dumpmey [enable]");
                }
                DumpMeYEnable = atoi(argv[3]);
                AmbaPrint("DumpMeY: %s", DumpMeYEnable?"Enable":"Disable");
            }
        }
    } else if ((strcmp(argv[1],"brc") == 0)) {
        if (argc < 3) {
            AmbaPrint("Usage: t videotuning brc [option]");
            AmbaPrint("         option: config quality control or invoke");
            AmbaPrint("               config - config quality control parameters");
            AmbaPrint("               invoke - invoke command");
            return -1;
        }

        if ((strcmp(argv[2],"config") == 0)) {
            UINT8 Mode = 0, Channel = 0;
            AMP_AVENC_HDLR_s *EncHdlr = NULL;
            Mode = atoi(argv[3]);
            Channel = atoi(argv[4]);

            if (Channel == 0) {
                EncHdlr = VideoTuningPri;
            } else if (Channel == 1) {
               AmbaPrintColor(RED,"Secondary channel not support yet");
            } else {
                AmbaPrintColor(RED,"Incorrect channel %u", Channel);
            }

            switch (Mode) {
            case 0:
            {
                if (TuningStatus != TUNING_STATUS_LIVEVIEW) {
                    AmbaPrint("Please set bitrate after liveview start %d", TuningStatus);
                    return -1;
                }
                {
                    UINT8 BrcMode = atoi(argv[5]);
                    AMP_VIDEOENC_BITSTREAM_CFG_s CurrentCfg = {0};
                    VideoTuningMgt[TuningModeIdx].BrcMode = BrcMode;
                    VideoTuningMgt[TuningModeIdx].AverageBitRate = atoi(argv[6]);
                    VideoTuningMgt[TuningModeIdx].MaxBitRate = atoi(argv[7]);
                    VideoTuningMgt[TuningModeIdx].MinBitRate = atoi(argv[8]);
                    AmpVideoEnc_GetBitstreamConfig(EncHdlr, &CurrentCfg);
                    CurrentCfg.Spec.H264Cfg.BitRateControl.BrcMode = VideoTuningMgt[TuningModeIdx].BrcMode;
                    CurrentCfg.Spec.H264Cfg.BitRateControl.AverageBitrate = (UINT32)(VideoTuningMgt[TuningModeIdx].AverageBitRate * 1E6);
                    CurrentCfg.Spec.H264Cfg.QualityControl.IBeatMode = atoi(argv[9]);
                    if (CurrentCfg.Spec.H264Cfg.BitRateControl.BrcMode == VIDEOENC_SMART_VBR) {
                        CurrentCfg.Spec.H264Cfg.BitRateControl.MaxBitrate = (UINT32)(VideoTuningMgt[TuningModeIdx].MaxBitRate * 1E6);
                        CurrentCfg.Spec.H264Cfg.BitRateControl.MinBitrate = (UINT32)(VideoTuningMgt[TuningModeIdx].MinBitRate * 1E6);
                    }
                    AmpVideoEnc_SetBitstreamConfig(EncHdlr, &CurrentCfg);
                    AmbaPrint("BRC mode %s Avg %dMbps Max %dMbps Min %dMbps iBeat %d",VideoTuningMgt[TuningModeIdx].BrcMode==1?"CBR":"SmartVBR", \
                        VideoTuningMgt[TuningModeIdx].AverageBitRate,VideoTuningMgt[TuningModeIdx].MaxBitRate,VideoTuningMgt[TuningModeIdx].MinBitRate, \
                        CurrentCfg.Spec.H264Cfg.QualityControl.IBeatMode);
                }
            }
                break;
            case 2:
                if (argc == 8) {
                    if (TuningStatus == TUNING_STATUS_ENCODE) {
                        //TBD
                    } else if (TuningStatus == TUNING_STATUS_LIVEVIEW) {
                        AMP_VIDEOENC_BITSTREAM_CFG_s CurrentCfg;
                        AmpVideoEnc_GetBitstreamConfig(EncHdlr, &CurrentCfg);
                        CurrentCfg.Spec.H264Cfg.GopM = atoi(argv[5]);
                        CurrentCfg.Spec.H264Cfg.GopN = atoi(argv[6]);
                        CurrentCfg.Spec.H264Cfg.GopIDR = atoi(argv[7]);
                        AmpVideoEnc_SetBitstreamConfig(EncHdlr, &CurrentCfg);
                    }
                } else {
                    return -1;
                }
                break;
            case 3:
                if (argc == 11) {
                    if (TuningStatus == TUNING_STATUS_ENCODE) {
                        //TBD
                    } else if (TuningStatus == TUNING_STATUS_LIVEVIEW) {
                        AMP_VIDEOENC_BITSTREAM_CFG_s CurrentCfg;
                        AmpVideoEnc_GetBitstreamConfig(EncHdlr, &CurrentCfg);
                        CurrentCfg.Spec.H264Cfg.QPControl.QpMinI = atoi(argv[5]);
                        CurrentCfg.Spec.H264Cfg.QPControl.QpMaxI = atoi(argv[6]);
                        CurrentCfg.Spec.H264Cfg.QPControl.QpMinP = atoi(argv[7]);
                        CurrentCfg.Spec.H264Cfg.QPControl.QpMaxP = atoi(argv[8]);
                        CurrentCfg.Spec.H264Cfg.QPControl.QpMinB = atoi(argv[9]);
                        CurrentCfg.Spec.H264Cfg.QPControl.QpMaxB = atoi(argv[10]);
                        AmpVideoEnc_SetBitstreamConfig(EncHdlr, &CurrentCfg);
                    }
                } else {
                    return -1;
                }
                break;
            case 4:
                if (argc == 15){
                    if (TuningStatus == TUNING_STATUS_ENCODE) {
                        //TBD
                    } else {
                        AMP_VIDEOENC_BITSTREAM_CFG_s CurrentCfg = {0};
                        AmpVideoEnc_GetBitstreamConfig(EncHdlr, &CurrentCfg);
                        CurrentCfg.Spec.H264Cfg.QualityControl.AutoQpStrength = atoi(argv[5]);
                        CurrentCfg.Spec.H264Cfg.QualityControl.Intra16x16Bias = atoi(argv[6]);
                        CurrentCfg.Spec.H264Cfg.QualityControl.Intra4x4Bias = atoi(argv[7]);
                        CurrentCfg.Spec.H264Cfg.QualityControl.Inter16x16Bias = atoi(argv[8]);
                        CurrentCfg.Spec.H264Cfg.QualityControl.Inter8x8Bias = atoi(argv[9]);
                        CurrentCfg.Spec.H264Cfg.QualityControl.Direct16x16Bias = atoi(argv[10]);
                        CurrentCfg.Spec.H264Cfg.QualityControl.Direct8x8Bias = atoi(argv[11]);
                        CurrentCfg.Spec.H264Cfg.QualityControl.MELambdaQpOffset = atoi(argv[12]);
                        CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterAlpha = atoi(argv[13]);
                        CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterBeta = atoi(argv[14]);
                        AmpVideoEnc_SetBitstreamConfig(EncHdlr, &CurrentCfg);
                    }
                } else {
                    return -1;
                }
                break;
            default:
                break;
            }

        } else if ((strcmp(argv[2],"invoke") == 0)) {
            //TBD
        }
    } else if ((strcmp(argv[1],"stq") == 0)) {
        AmbaPrint("[VideoTuningState] (%d)", TuningStatus);
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
    } else if (strcmp(argv[1],"isdump") == 0) {
        UINT32 v1 = atoi(argv[2]); //MainView
        extern void ImgSchdlr_IsoCfgBuffDump(UINT32 *MainViewID) __attribute__((weak));

        ImgSchdlr_IsoCfgBuffDump(&v1);
    } else if (strcmp(argv[1],"dumpituner") == 0) {
        UINT8 Enable = atoi(argv[2]);
        if (Enable == 1) {
            TUNE_Initial_Config_s ItunerInitConfig;
            memset(&ItunerInitConfig, 0x0, sizeof(TUNE_Initial_Config_s));
            ItunerInitConfig.Text.pBytePool = &G_MMPL;
            VideoTuningDumpItuner = 1;
            AmbaTUNE_Change_Parser_Mode(TEXT_TUNE);
            if (OK != AmbaTUNE_Init(&ItunerInitConfig)) {
                AmbaPrintColor(RED, "TexHdlr_Init() failed");
            } else {
                AmbaPrint("Enable dump ituner file (%u)", VideoTuningDumpItuner);
            }
        } else if (Enable == 0) {
            AmbaTUNE_UnInit();
            VideoTuningDumpItuner = 0;
            AmbaPrint("Disable dump ituner file (%u)", VideoTuningDumpItuner);
        }
    } else if (strcmp(argv[1],"system") == 0) {
        UINT8 v1 = atoi(argv[2]);

        AmbaPrint("Encode System is %s", v1?"PAL": "NTSC");
        TuningEncodeSystem = v1;
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
#ifdef CONFIG_SOC_A12
    } else if (strcmp(argv[1],"thm") == 0) {
        if (TuningStatus == TUNING_STATUS_ENCODE) {
            return 0;
        }

        if (argc < 3) {
            AmbaPrint("Usage: t videotuning thm [enable]");
            AmbaPrint("             enable: enable video thumbnail");
            return 0;
        } else {
            UINT8 tt = atoi(argv[2]);
            TuningEncThumbnail = tt;
            AmbaPrint("VideoThm %s", TuningEncThumbnail?"Enable":"Disable");
        }
    } else if (strcmp(argv[1],"vinsel") == 0) {
        UINT8 VinSelect = atoi(argv[2]);
        if (VinSelect == VIN_MAIN_ONLY || VinSelect == VIN_PIP_ONLY) {
            VtLiveviewVinSelect = VtStillEncVinSelect = VinSelect;
            AmbaPrint("vin select %s", (VinSelect == VIN_MAIN_ONLY)? "Main": "Pip");
        } else {
            AmbaPrint("Incorrect vin select %u", VinSelect);
        }
#endif
    } else {
        AmbaPrint("Usage: t videotuning init|liveviewstart|chg");
        AmbaPrint("       init: init all");
        AmbaPrint("       liveviewstart: start liveview");
        AmbaPrint("       chg: change sensor resolution (when liveview)");
        AmbaPrint("       liveviewcapture: do LiveView Capture");
        AmbaPrint("       videorawenc: do Video raw encode");
        //AmbaPrint("       rotate: Set encode rotation");
    }
    return 0;
}

int AmpUT_VideoTuningTestAdd(void)
{
    AmbaPrint("Adding AmpUT_VideoTuning");
    // hook command
    AmbaTest_RegisterCommand("vt", AmpUT_VideoTuningTest);
    AmbaTest_RegisterCommand("videotuning", AmpUT_VideoTuningTest);

    //AmpUT_VideoTuing_Init(0);

    return AMP_OK;
}
