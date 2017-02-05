 /**
  * @file src/app/sample/unittest/AmpUT_VideoEnc.c
  *
  * Video Encode/Liveview unit test
  *
  * History:
  *    2013/05/02 - [Jenghung Luo] created file
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
#include "AmbaPLL.h"
#include <AmbaDSP_WarpCore.h>
#include <recorder/StillEnc.h>
#include <scheduler.h>
#include <img/ImgDzoom.h>
#include "AmbaUtility.h"
#include <cfs/AmpCfs.h>
#include <AmbaLCD.h>
#include <AmbaHDMI.h>
#include "AmpUT_Display.h"

/* For Standby mode test */
#include "AmbaINT.h"
#include "AmbaINT_Def.h"
#include "AmbaLink.h"

#include <AmbaCalibInfo.h>
#ifdef CONFIG_SOC_A12
#include "AmbaUTCalibMgr.h"
#endif

#include <AmbaImg_Proc.h>
#include <AmbaImg_Impl_Cmd.h>
#include <AmbaImg_VIn_Handler.h>
#include <AmbaImg_VDsp_Handler.h>
#include <math.h>
#include <recorder/AudioEnc.h>
#ifdef CONFIG_SOC_A12
#include "AmpUT_VideoIsoConfig.h"
#include "AmbaDSP_ImgHighIsoFilter.h"
#endif
#include <AmbaImg_AaaDef.h>
#include <AmbaImg_Exif.h>
#include "AmbaImgCalibItuner.h"
#include "AmbaTUNE_HdlrManager.h"

#ifdef CONFIG_SOC_A12
#include "encMonitor.h"
#include "encMonitorService.h"
#include "encMonitorStream.h"
#endif

/* EIS test */
#ifdef CONFIG_SOC_A12
#include "AmbaIMU.h"
extern AMBA_IMU_OBJ_s AmbaIMU_DMY0000Obj;
extern AMBA_IMU_OBJ_s AmbaIMU_MPU6500Obj;
extern AMBA_IMU_OBJ_s AmbaIMU_ICM20608Obj;
extern int AmbaEis_Inactive(void);
extern void AmbaEisTask_Init(UINT32, UINT8);
static UINT8 VideoEnc_EisAlgoEnable = 0;
#endif

static char DefaultSlot[] = "C";

void *UT_VideoEncodefopen(const char *pFileName, const char *pMode, BOOL8 AsyncMode)
{
    AMP_CFS_FILE_PARAM_s cfsParam;
    AmpCFS_GetFileParam(&cfsParam);
    cfsParam.AsyncMode = AsyncMode;
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
        //strcpy(cfsParam.Filename, pFileName);
        //return (void *)AmpCFS_fopen(&cfsParam);
        AmbaPrint("[%s]: Unicode should be abandoned", __func__);
        return NULL;
    } else {
        strcpy((char *)cfsParam.Filename, pFileName);
        return (void *)AmpCFS_fopen(&cfsParam);
    }
}

int UT_VideoEncodefclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_VideoEncodefwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_VideoEncodefsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

UINT32 UT_VideoFrameRateIntConvert(UINT32 OriFrameRate)
{
    UINT32 FrameRateInt = OriFrameRate;

    if (OriFrameRate==29 || OriFrameRate==59 || OriFrameRate==119 || OriFrameRate==239) {
        FrameRateInt++;
    }

    return FrameRateInt;
}

static UINT8 VideoEncDspAssert = 0;
static char DspMsgBuf[1024] __attribute__((section("no_init")));

// Global var for VideoEnc codec
static AMP_VIN_HDLR_s *VideoEncVinA = NULL;       // Vin instance
static AMP_AVENC_HDLR_s *VideoEncPri = NULL;      // Primary VideoEnc codec instance
static AMP_AVENC_HDLR_s *VideoEncSec = NULL;      // Secondary VideoEnc codec instance
static AMP_ENC_PIPE_HDLR_s *VideoEncPipe = NULL;  // Encode pipeline  instance
static AMBA_IMG_SCHDLR_HDLR_s *ImgSchdlr = NULL;    // Image scheduler instance
#ifdef CONFIG_SOC_A12
static AMBA_IMG_ENC_MONITOR_BITRATE_HDLR_s *BrcHdlrPri = NULL;  // Pri Stream BitRateMonitorControl instance
static AMBA_IMG_ENC_MONITOR_BITRATE_HDLR_s *BrcHdlrSec = NULL;  // Sec Stream BitRateMonitorControl instance
static AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *EncMonitorStrmHdlrPri = NULL;  // Pri Stream in encode monitor instance
static AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *EncMonitorStrmHdlrSec = NULL;  // Sec Stream in encode monitor instance
static AMBA_IMG_ENC_MONITOR_AQP_HDLR_s *AqpHdlrPri = NULL;  // Pri Stream AqpMonitorControl instance
static AMBA_IMG_ENC_MONITOR_AQP_HDLR_s *AqpHdlrSec = NULL;  // Sec Stream AqpMonitorControl instance
#endif
static UINT8 *VinWorkBuffer = NULL;               // Vin module working buffer
static UINT8 *VEncWorkBuffer = NULL;              // VideoEnc working buffer
static UINT8 *ImgSchdlrWorkBuffer = NULL;         // Img scheduler working buffer
#ifdef CONFIG_SOC_A12
static UINT8 *EncMonitorCyclicWorkBuffer = NULL;  // Encode monitor Cyclic working buffer
static UINT8 *EncMonitorServiceWorkBuffer = NULL; // BitRateMonitorControl working buffer
static UINT8 *EncMonitorServiceAqpWorkBuffer = NULL; // AQpMonitorControl working buffer
static UINT8 *EncMonitorStrmWorkBuffer = NULL;    // Encode monitor stream working buffer
#endif
static AMBA_DSP_CHANNEL_ID_u VinChannel;          // Vin channel

#define STATUS_IDLE     1
#define STATUS_INIT     2
#define STATUS_LIVEVIEW 3
#define STATUS_ENCODE   4
#define STATUS_PAUSE    5
static UINT8 Status = STATUS_IDLE;                  // Simple state machine to avoid wrong command issuing

#ifdef CONFIG_LCD_WDF9648W
extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;
#endif

static UINT8 TvLiveview = 1;                        // Enable TV liveview or not
static UINT8 LCDLiveview = 1;                       // Enable LCD liveview or not
static UINT8 VideoEnc3AEnable = 1;                  // Enable AE/AWB/ADJ algo or not
static UINT8 VideoEncTAEnable = 0;                  // Enable TA in HybridAlgoMode
static UINT8 EncPriSpecH264 = 1;                    // Primary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 EncSecSpecH264 = 1;                    // Secondary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 EncPriInteralce = 0;                   // Primary codec instance output type. 0 = Progressive, 1 = Interlave
static UINT8 EncSecInteralce = 0;                   // Secondary codec instance output type. 0 = Progressive, 1 = Interlave
static UINT8 EncTimeLapse = 0;                      // Enable time lapsed encoding or not
static UINT8 EncThumbnail = 0;                      // Enable video thumbnail or not
static UINT8 EncDualStream = 0;                     // Enable dual stream or not
static UINT8 EncDualHDStream = 0;                   // Enable Dual HD stream or not
static UINT8 EncDateTimeStampPri = 0;               // Enable Primary stream Date time stamp or not
static UINT8 EncDateTimeStampSec = 0;               // Enable Secondary stream Date time stamp or not
static UINT8 EncIBeat = 0;                          // Enable I-beating RC
#define INIT_DZOOM_FACTOR   (1<<16)
static UINT32 InitZoomX = INIT_DZOOM_FACTOR;        // Initial Horizontal dzoom factor
static UINT32 InitZoomY = INIT_DZOOM_FACTOR;        // Initial Vertical dzoom factor
static AMP_ROTATION_e EncRotation = AMP_ROTATE_0;   // Video encode rotation
static AMP_VIDEOENC_H264_STOP_METHOD_e EncStopMethod = AMP_VIDEOENC_STOP_NEXT_IP; // Video encode stop method
static UINT8 LiveViewProcMode = 0;                  // LiveView Process Mode, 0: Express 1:Hybrid
static UINT8 LiveViewAlgoMode = 0;                  // LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
static UINT8 LiveViewOSMode = 1;                    // LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 LiveViewHdrMode = 0;                   // LiveView HDR Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 ExtLiveViewProcMode = 0xFF;            // External LiveView Process Mode, 0: Express 1:Hybrid
static UINT8 ExtLiveViewAlgoMode = 0xFF;            // External LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
static UINT8 ExtLiveViewOSMode = 0xFF;              // External LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
static UINT16 CalibEnable = 0;                      // Calibration data enable or not
#define CALIB_SBP   0x1
#define CALIB_WARP  (0x1<<1)
#define CALIB_CA    (0x1<<2)
#define CALIB_VIG   (0x1<<3)

static UINT8 VideoEncIsIqParamInit = 0;
static UINT8 VideoEncIsHdrIqParam = 0;

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
static UINT8 EncodeSystem = 0;                      // Encode System, 0:NTSC, 1: PAL
static UINT8 EncodeDumpSkip = 0;                    // Encode Skip dump file, write data to SD card or just lies to fifo that it has muxed pending data
#define VIDEO_ENC_SKIP_PRI   0x1
#define VIDEO_ENC_SKIP_SEC   (0x1<<1)
#define VIDEO_ENC_SKIP_JPG   (0x1<<7)
static UINT8 MainviewReportRate = 0;                // vcap report interval, 1: every vcap; 2:every two vcap....
#ifdef CONFIG_SOC_A12
static UINT8 EncMonitorEnable = 1;                  // Enable encode monitor or not
static UINT8 EncMonitorAQPEnable = 1;               // Enable encode monitor AQP or not
#endif
static UINT8 VideoOBModeEnable = 0;
static UINT8 DspSuspendEnable = 0;
static UINT8 WirelessModeEnable = 0;
static UINT8 SlowShutterEnable = 0;

static UINT8 MjpegStandardQuantMatrix[128] = {      // Standard JPEG qualty 50 table.
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

static UINT8 MjpegQuality = 90;                     //Mjpeg Quality 0~100
static UINT8 MjpegQuantMatrix[128] = {              // base on Standard JPEG qualty 50 table.
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
static UINT8 VideoPIVMainQuality = 90;              // PIV main Quality 0~100
static UINT8 VideoPIVThmbQuality = 90;              // PIV thumbnail Quality 0~100
static UINT8 VideoPIVScrnQuality = 90;              // PIV screennail Quality 0~100
UINT8 VideoPIVQTable[3][128] = {
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

AMP_PIVENC_BLEND_INFO_s PIVInfo;

static UINT8 EncBlendY[256*100];                    // Y buffer for blending test
static UINT8 EncBlendAlphaY[256*100];               // Alpha Y buffer for blending test
#ifdef CONFIG_SOC_A9
static UINT8 EncBlendUV[256*100/2];                 // UV buffer for blending test
static UINT8 EncBlendAlphaUV[256*100/2];            // Alpha UV buffer for blending test
#else
static UINT8 EncBlendUV[256*100];                 // UV buffer for blending test
static UINT8 EncBlendAlphaUV[256*100];            // Alpha UV buffer for blending test
#endif

#define CAPTURE_MODE_NONE               0
static UINT8 CaptureMode = CAPTURE_MODE_NONE;

// Global var for StillEnc codec
static UINT8 *StillEncWorkBuf = NULL;
static AMP_STLENC_HDLR_s *StillEncPri = NULL;
static AMP_ENC_PIPE_HDLR_s *StillEncPipe = NULL;
static UINT8 StillIso = 0;
static UINT8 StillCodecInit = 0;
static UINT8 StillRawCaptureRunning = 0;
static UINT32 PivProcess = 0;
static UINT16 PivCaptureWidth = 0;
static UINT16 PivCaptureHeight = 0;
static UINT16 PivMainWidth = 0;
static UINT16 PivMainHeight = 0;
static UINT8  PivVideoThm = 0;
static UINT8  PivVideoThmJpgCount = 0;
static UINT16 ScrnWidthAct = 0;
static UINT16 ScrnHeightAct = 0;
static UINT16 ThmWidthAct = 0;
static UINT16 ThmHeightAct = 0;

static UINT8 *s_scriptAddr = NULL;
static UINT8 *Ori_s_scriptAddr = NULL;
static UINT8 *s_rawBuffAddr = NULL;
static UINT8 *Ori_s_rawBuffAddr = NULL;
static UINT8 *s_yuvBuffAddr = NULL;
static UINT8 *Ori_s_yuvBuffAddr = NULL;
static UINT8 *s_scrnBuffAddr = NULL;
static UINT8 *Ori_s_scrnBuffAddr = NULL;
static UINT8 *s_thmBuffAddr = NULL;
static UINT8 *Ori_s_thmBuffAddr = NULL;
static UINT16 QvLCDW = 0;
static UINT16 QvLCDH = 0;
static UINT16 QvHDMIW = 0;
static UINT16 QvHDMIH = 0;
//static UINT8 qvDisplayCfg = 0;
static UINT8 *s_qvLCDBuffAddr = NULL;
static UINT8 *Ori_s_qvLCDBuffAddr = NULL;
static UINT8 *s_qvHDMIBuffAddr = NULL;
static UINT8 *Ori_s_qvHDMIBuffAddr = NULL;
static UINT32 VideoEncCusomtDspWorkSize = 0;


/* Still Codec Function prototype*/
UINT32 AmpUT_VideoEnc_PivPOSTCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_VideoEnc_PivPRECB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_VideoEnc_DspWork_Calculate(UINT8 **addr, UINT32 *size);

static AMP_STILLENC_POSTP_s post_videoEncPiv_cb = {.Process = AmpUT_VideoEnc_PivPOSTCB};
static AMP_STILLENC_PREP_s pre_videoEncPiv_cb = {.Process = AmpUT_VideoEnc_PivPRECB};
int AmpUT_VideoEnc_PIV(AMP_VIDEOENC_PIV_CTRL_s pivCtrl, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop);

static INPUT_ENC_MGT_s *VideoEncMgt = NULL;// Pointer to above tables
static UINT8 InputDeviceId = 0;
static UINT8 EncModeIdx = 1;        // Current mode index

#define GOP_N   8                   // I-frame distance
#define GOP_IDR 8                   // IDR-frame distance

static UINT16 SecStreamCustomWidth = 0;
static UINT16 SecStreamCustomHeight = 0;

#ifdef CONFIG_SOC_A9
static AMP_DISP_WINDOW_HDLR_s *encLcdWinHdlr = NULL; // LCD video plane window handler
static AMP_DISP_WINDOW_HDLR_s *encTvWinHdlr = NULL;  // TV video plane window handler
#endif

/* Simple muxer to communicate with FIFO */
#define Max_PIV_Width       4096
#define Max_PIV_Height      2304

#define BITSFIFO_SIZE 32*1024*1024+(Max_PIV_Width*Max_PIV_Height)
static UINT8 *H264BitsBuf;          // H.264 bitstream buffer
static UINT8 *MjpgBitsBuf;          // MJPEG bitstream buffer
#ifdef CONFIG_SOC_A9
#define BISFIFO_STILL_SIZE 10*1024*1024
static UINT8 *JpegBitsBuf;          // JPEG bitstream buffer
static UINT8 *JpegDescBuf;          // JPEG descriptor buffer (physical)
#endif
#define DESC_SIZE 40*3000
static UINT8 *H264DescBuf;          // H.264 descriptor buffer (physical)
static UINT8 *MjpgDescBuf;          // MJPEG descriptor buffer (physical)
static AMBA_KAL_TASK_t VideoEncPriMuxTask = {0};    // Primary stream muxer task
static AMBA_KAL_TASK_t VideoEncSecMuxTask = {0};    // Secondary stream muxer task
static AMBA_KAL_TASK_t VideoEncJpegMuxTask = {0};   // JPEG stream muxer task
static UINT8 *VideoEncPriMuxStack = NULL;           // Stack for primary stream muxer task
#define VIDEO_ENC_PRI_MUX_TASK_STACK_SIZE   (8192)
static UINT8 *VideoEncSecMuxStack = NULL;           // Stack for secondary stream muxer task
#define VIDEO_ENC_SEC_MUX_TASK_STACK_SIZE   (8192)
static UINT8 *VideoEncJpegMuxStack = NULL;          // Stack for JPEG(piv/thumbnail) stream muxer task
#define VIDEO_ENC_JPG_MUX_TASK_STACK_SIZE   (8192)
static AMBA_KAL_SEM_t VideoEncPriSem = {0};         // Counting semaphore for primary stream muxer task and fifo callback
static AMBA_KAL_SEM_t VideoEncSecSem = {0};         // Counting semaphore for secondary stream muxer task and fifo callback
static AMBA_KAL_SEM_t VideoEncJpegSem = {0};        // Counting semaphore for JPEG(piv/thumbnail) stream muxer task and fifo callback

static AMP_CFS_FILE_s *outputPriFile = NULL;        // Output file pointer for primary stream
static AMP_CFS_FILE_s *outputSecFile = NULL;        // Output file pointer for secondary stream
static AMP_CFS_FILE_s *outputJpegFile = NULL;       // Output file pointer for JPEG(piv/thumbnail) stream
static AMP_CFS_FILE_s *outputPriIdxFile = NULL;     // Output file pointer for primary stream index (frame offset/size...etc)
static AMP_CFS_FILE_s *outputSecIdxFile = NULL;     // Output file pointer for secondary stream index (frame offset/size...etc)
static AMP_CFS_FILE_s *UdtaPriFile = NULL;          // Output file pointer for primary stream UserData
static AMP_CFS_FILE_s *UdtaSecFile = NULL;          // Output file pointer for secondary stream UserData
static AMP_CFS_FILE_s *stillRawFile = NULL;         // raw Output file pointer
static AMP_CFS_FILE_s *stillYFile = NULL;           // y Output file pointer
static AMP_CFS_FILE_s *stillUvFile = NULL;          // uv Output file pointer
static AMP_CFS_FILE_s *DspMsgFile = NULL;           // dsp debug message

#define ClipMaxStorageSize  3E9
static int fnoPri = 0;                  // Frame number counter for primary stream muxer
static int FnoPriSplit = 0;                  // Frame number counter for primary stream muxer
static int fnoSec = 0;                  // Frame number counter for secondary stream muxer
static int FnoSecSplit = 0;             // Frame number counter for secondary stream muxer
static int MjpegFmt = 1;                // MJPEG file fmt, 0 : storage frame by frame, 1: storage 1 file
static UINT16 fnoRaw = 1;               // Frame number counter for piv/thumbnail raw picture
static UINT16 fnoYuv = 1;               // Frame number counter for piv/thumbnail yuv picture
static UINT16 fnoPiv = 0xFFFF;          // Frame number counter for piv/thumbnail jpeg picture
static UINT16 fnoScrnPiv = 0xFFFF;          // Frame number counter for piv/screennail jpeg picture
static UINT16 fnoThmPiv = 0xFFFF;          // Frame number counter for piv/thumbnail jpeg picture
static UINT32 EncFrameRate = 0;         // Remember current framerate for primary muxer to calculate actual bitrate
static UINT64 encPriBytesTotal = 0;     // total bytes primary stream muxer received
static UINT32 encPriTotalFrames = 0;    // total frames primary stream muxer received
static UINT64 encPriBytesStorageTotal = 0;     // total bytes primary stream storage received
static UINT64 encSecBytesTotal = 0;     // total bytes secondary stream muxer received
static UINT32 encSecTotalFrames = 0;    // total frames secondary stream muxer received
static UINT64 encSecBytesStorageTotal = 0;     // total bytes primary stream storage received
static AMP_FIFO_HDLR_s *VirtualPriFifoHdlr = NULL;  // Primary stream vitrual fifo
static AMP_FIFO_HDLR_s *VirtualSecFifoHdlr = NULL;  // Secondary stream vitrual fifo
static AMP_FIFO_HDLR_s *VirtualJpegFifoHdlr = NULL; // JPEG (piv/thumbnail) stream vitrual fifo
static UINT8 LogMuxer = 0;

static AMP_IMG_DZOOM_TABLE_s dzoomTable;    // Digital zoom table
static AMP_IMG_DZOOM_HDLR_s *dzoomHdlr;     // Digital zoom instance

/** UT function prototype */
int AmpUT_VideoEnc_EncStop(void);
int AmpUT_VideoEnc_ChangeResolution(UINT32 modeIdx);
int AmpUT_VideoEnc_LiveviewStart(UINT32 modeIdx);
int AmpUT_VideoEnc_ChangeMainView(UINT32 MainViewID);
extern UINT32 _find_jpeg_segment(UINT8* img, UINT32 size, UINT16 marker);

static int DoDumpDspMsg(void *pFile)
{
    AMBA_PRINTK_DSP_LAST_MSG_PARAM_s DspMsgParam = {0};
    AMBA_DSP_PRINTF_s *DspPf = NULL;
    int l;

    AmbaPrintk_GetDspLastMsg(4096, &DspMsgParam);
    if (DspMsgParam.Head > DspMsgParam.Tail) {
        for (DspPf = DspMsgParam.Head; DspPf <= DspMsgParam.AbsEnd;
             DspPf = (AMBA_DSP_PRINTF_s *)
                     ((UINT32) DspPf + sizeof(AMBA_DSP_PRINTF_s))) {
            l = AmbaPrintk_PrintfDspMsgToBuf(DspMsgBuf, DspPf, DspMsgParam.CodeAddr, DspMsgParam.MemdAddr, DspMsgParam.MdxfAddr);

            /* Process the EOL characters */
            if (DspMsgBuf[l - 1] != '\n')
                DspMsgBuf[l++] = '\n';
            DspMsgBuf[l] = '\0';

            UT_VideoEncodefwrite((const void *)&DspMsgBuf, 1, l, pFile);
        }
        for (DspPf = DspMsgParam.AbsStart; DspPf <= DspMsgParam.Tail;
             DspPf = (AMBA_DSP_PRINTF_s *)
                     ((UINT32) DspPf + sizeof(AMBA_DSP_PRINTF_s))) {
             l = AmbaPrintk_PrintfDspMsgToBuf(DspMsgBuf, DspPf, DspMsgParam.CodeAddr, DspMsgParam.MemdAddr, DspMsgParam.MdxfAddr);

             /* Process the EOL characters */
             if (DspMsgBuf[l - 1] != '\n')
                 DspMsgBuf[l++] = '\n';
             DspMsgBuf[l] = '\0';

             UT_VideoEncodefwrite((const void *)&DspMsgBuf, 1, l, pFile);
        }
    } else {
        for (DspPf = DspMsgParam.Head; DspPf <= DspMsgParam.Tail; ) {
            l = AmbaPrintk_PrintfDspMsgToBuf(DspMsgBuf, DspPf, DspMsgParam.CodeAddr, DspMsgParam.MemdAddr, DspMsgParam.MdxfAddr);

            /* Process the EOL characters */
            if (DspMsgBuf[l - 1] != '\n')
                DspMsgBuf[l++] = '\n';
            DspMsgBuf[l] = '\0';
            UT_VideoEncodefwrite((const void *)&DspMsgBuf, 1, l, pFile);

            DspPf = (AMBA_DSP_PRINTF_s *)
                    ((UINT32) DspPf + sizeof(AMBA_DSP_PRINTF_s));
        }
    }

    return 0;
}

/*************************************** Muxer (Data flow) implementation start ********************************************************/

/**
 * Primary muxer task
 *
 * @param [in] info initial value
 *
 */

static UINT32 BrcFrameCount = 0;
static UINT32 BrcByteCount = 0;

void AmpUT_VideoEnc_PriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc = NULL;
    int Er = 0;
    UINT8 *BitsLimit = NULL;
    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_VideoEnc_PriMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&VideoEncPriSem, 10000);  // Check if there is any pending frame to be muxed
        if (Er != OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }
        Er = AmpFifo_PeekEntry(VirtualPriFifoHdlr, &Desc, 0);  // Get a pending entry
        if (Er == 0) {
            char Ty[4];

            if (Desc->Type == AMP_FIFO_TYPE_IDR_FRAME) {
                Ty[0] = 'I'; Ty[1] = 'D'; Ty[2] = 'R'; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_I_FRAME) {
                Ty[0] = 'I'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_P_FRAME) {
                Ty[0] = 'P'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_B_FRAME) {
                Ty[0] = 'B'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
                Ty[0] = 'P'; Ty[1] = 'U'; Ty[2] = 'S'; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_EOS) {
                Ty[0] = 'E'; Ty[1] = 'O'; Ty[2] = 'S'; Ty[3] = '\0';
            } else {
                Ty[0] = 'J'; Ty[1] = 'P'; Ty[2] = 'G'; Ty[3] = '\0';
            }
            if (LogMuxer)
                AmbaPrint("Pri[%d] %s pts:%8lld 0x%08x 0x%X", encPriTotalFrames, Ty, Desc->Pts, Desc->StartAddr, Desc->Size);
        } else {
            while (Er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(VirtualPriFifoHdlr, &Desc, 0);
            }
        }
        if (encPriBytesStorageTotal > ClipMaxStorageSize) {
            if (outputPriFile && (Desc->Type<=AMP_FIFO_TYPE_I_FRAME)) {
                UT_VideoEncodefsync((void *)outputPriFile);
                UT_VideoEncodefclose((void *)outputPriFile);
                FnoPriSplit++;
                UT_VideoEncodefsync((void *)outputPriIdxFile);
                UT_VideoEncodefclose((void *)outputPriIdxFile);
                outputPriFile = NULL;
                outputPriIdxFile = NULL;
                FileOffset = 0;
            }
        }

        if (((!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) || (encPriBytesStorageTotal > ClipMaxStorageSize) || ((EncPriSpecH264==0)&& (MjpegFmt==0 || (encPriTotalFrames==0))))&&(Desc->Size != AMP_FIFO_MARK_EOS)) {
        //if ((!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI))) {
            if (outputPriFile == NULL) { // Open files when receiving the 1st frame
                char Fn[80];
                char mdASCII[3] = {'w','+','\0'};
                FORMAT_USER_DATA_s Udta;

                encPriBytesStorageTotal = 0;
                if (EncPriSpecH264 || MjpegFmt) {
                    if (FnoPriSplit) {
                        sprintf(Fn,"%s:\\OU%02d%04d.%s", DefaultSlot, FnoPriSplit,fnoPri,EncPriSpecH264?"h264":"mjpg");
                    } else {
                        sprintf(Fn,"%s:\\OUT_%04d.%s", DefaultSlot, fnoPri,EncPriSpecH264?"h264":"mjpg");
                    }
                    outputPriFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                } else {
                    sprintf(Fn,"%s:\\OU%02d%04d.%s", DefaultSlot, fnoPri, (int)encPriTotalFrames, "jpg");
                    outputPriFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                }

                while (outputPriFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    outputPriFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);
                if (EncPriSpecH264) {
                    if (FnoPriSplit) {
                        sprintf(Fn,"%s:\\OU%02d%04d.nhnt", DefaultSlot, FnoPriSplit,fnoPri);
                    } else {
                        sprintf(Fn,"%s:\\OUT_%04d.nhnt", DefaultSlot, fnoPri);
                    }
                    outputPriIdxFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                    while (outputPriIdxFile==0) {
                        AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                        AmbaKAL_TaskSleep(10);
                        outputPriIdxFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                    }
                    AmbaPrint("%s opened", Fn);

                    if (FnoPriSplit==0) {
                        sprintf(Fn,"%s:\\OUT_%04d.udta", DefaultSlot, fnoPri);
                        UdtaPriFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                        while (UdtaPriFile==0) {
                            AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                            AmbaKAL_TaskSleep(10);
                            UdtaPriFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                        }

                        AmbaPrint("%s opened", Fn);

                        Udta.nIdrInterval = GOP_IDR/GOP_N;
                        Udta.nTimeScale = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;
                        Udta.nTickPerPicture = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TickPerPicture: VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                        Udta.nN = GOP_N;
                        Udta.nM = VideoEncMgt[EncModeIdx].GopM;
                        if (EncRotation == AMP_ROTATE_90 || EncRotation == AMP_ROTATE_90_VERT_FLIP || \
                            EncRotation == AMP_ROTATE_270 || EncRotation == AMP_ROTATE_270_VERT_FLIP) {
                            Udta.nVideoWidth = VideoEncMgt[EncModeIdx].MainHeight;
                            Udta.nVideoHeight = VideoEncMgt[EncModeIdx].MainWidth;
                        } else {
                            Udta.nVideoWidth = VideoEncMgt[EncModeIdx].MainWidth;
                            Udta.nVideoHeight = VideoEncMgt[EncModeIdx].MainHeight;
                        }
                        Udta.nInterlaced = VideoEncMgt[EncModeIdx].Interlace;
                        UT_VideoEncodefwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)UdtaPriFile);
                        UT_VideoEncodefclose((void *)UdtaPriFile);
                    }
                    NhntHeader.Signature[0]='n';
                    NhntHeader.Signature[1]='h';
                    NhntHeader.Signature[2]='n';
                    NhntHeader.Signature[3]='t';
                    NhntHeader.TimeStampResolution = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;
                    UT_VideoEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputPriIdxFile);
                }
                if (EncPriSpecH264) {
                    BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
                } else {
                    BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
                }
            }
        }

        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) {
                if (outputPriFile) {
                    UT_VideoEncodefsync((void *)outputPriFile);
                    UT_VideoEncodefclose((void *)outputPriFile);
                    fnoPri++;
                    if (outputPriIdxFile) {
                        UT_VideoEncodefsync((void *)outputPriIdxFile);
                        UT_VideoEncodefclose((void *)outputPriIdxFile);
                    }
                    outputPriFile = NULL;
                    outputPriIdxFile = NULL;
                    FileOffset = 0;
                    FnoPriSplit = 0;
                }
            }

            Avg = (UINT32)(encPriBytesTotal*8.0*EncFrameRate/encPriTotalFrames/1E3);

            AmbaPrint("Primary Muxer met EOS, total %d frames/fields", encPriTotalFrames);
            AmbaPrint("Primary Bitrate Average: %d kbps\n", Avg);

            encPriBytesTotal = 0;
            encPriTotalFrames = 0;
        } else if (Desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
            //Do Nothing
        } else {
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) {
                if (outputPriFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;
                    if (outputPriIdxFile) {
                        UT_VideoEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)outputPriIdxFile);
                    }

                    if (EncPriSpecH264==0 && VideoEncMgt[EncModeIdx].MainHeight==1080) {
                        UINT8 *Mjpeg_bs=0,*OriMjpeg_bs=0;
                        UINT32 sof_addr=0;
                        UINT8 *sof_ptr=0;

                        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&Mjpeg_bs, (void **)&OriMjpeg_bs, ALIGN_32(Desc->Size), 32);
                        if (Er != OK) {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                               // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                               UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputPriFile);
                            } else {
                                // AmbaCache_Invalidate(Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputPriFile);
                                // AmbaCache_Invalidate(H264BitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                                UT_VideoEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
                            }
                            //AmbaPrint("Out of cached memory for MJPEG header change flow!!");
                        } else {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                                memcpy(Mjpeg_bs, Desc->StartAddr, Desc->Size);
                            } else {
                                UINT8 *Bs = Mjpeg_bs;
                                memcpy(Mjpeg_bs, Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                Bs += BitsLimit - Desc->StartAddr + 1;
                                memcpy(Bs, MjpgBitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                            }
                            sof_addr = _find_jpeg_segment(Mjpeg_bs, Desc->Size, 0xFFC0);
                            if (sof_addr == 0xFFFFFFFF) {
                                if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                                   // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                                   UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputPriFile);
                                } else {
                                    // AmbaCache_Invalidate(Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                    UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputPriFile);
                                    // AmbaCache_Invalidate(H264BitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                                    UT_VideoEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
                                }
                            } else {
                                sof_ptr = (UINT8*)(sof_addr + 5);
                                (*sof_ptr)    =  (UINT8)(VideoEncMgt[EncModeIdx].MainHeight >> 8);
                                *(sof_ptr+1)  = (UINT8)(VideoEncMgt[EncModeIdx].MainHeight &  0xFF);

                                UT_VideoEncodefwrite((const void *)Mjpeg_bs, 1, Desc->Size, (void *)outputPriFile);
                            }

                            if (AmbaKAL_BytePoolFree((void *)OriMjpeg_bs) != OK) {
                                AmbaPrint("cached memory release fail for MJPEG header change flow");
                            }
                        }

                    } else {
                 //       AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                        if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                           // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                           UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputPriFile);
                        } else {
                            UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputPriFile);
                            if (EncPriSpecH264) {
                                UT_VideoEncodefwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
                            } else {
                                UT_VideoEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
                            }
                        }
                    }
                }
            } else {
                AmbaKAL_TaskSleep(1);    // Do nothing
            }

// Bitrate statistics
//            if (((BrcFrameCount%EncFrameRate) == 0)&& ((BrcFrameCount/EncFrameRate)%3 == 0) && BrcFrameCount) {
//                AmbaPrint("Pri AvgBitRate = %fMbps, Target %dMbps",(BrcByteCount*8.0*EncFrameRate/BrcFrameCount/1E6),VideoEncMgt[EncModeIdx].AverageBitRate);
//            }

            if (EncPriSpecH264==0 && MjpegFmt==0) {
                UT_VideoEncodefsync((void *)outputPriFile);
                UT_VideoEncodefclose((void *)outputPriFile);
                outputPriFile = NULL;
                outputPriIdxFile = NULL;
            }
            encPriBytesTotal += Desc->Size;
            encPriBytesStorageTotal += Desc->Size;
            encPriTotalFrames++;
            BrcFrameCount++;
            BrcByteCount+=Desc->Size;
        }
        AmpFifo_RemoveEntry(VirtualPriFifoHdlr, 1);
    }
}


/**
 * Secondary muxer task
 *
 * @param [in] info initial value
 *
 */
void AmpUT_VideoEnc_SecMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc = NULL;
    int Er = 0;
    UINT8 *BitsLimit = NULL;
    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_VideoEnc_SecMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&VideoEncSecSem, 10000);
        if (Er!= OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }
        Er = AmpFifo_PeekEntry(VirtualSecFifoHdlr, &Desc, 0);
        if (Er == 0) {
            char Ty[4];

            if (Desc->Type == AMP_FIFO_TYPE_IDR_FRAME) {
                Ty[0] = 'I'; Ty[1] = 'D'; Ty[2] = 'R'; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_I_FRAME) {
                Ty[0] = 'I'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_P_FRAME) {
                Ty[0] = 'P'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_B_FRAME) {
                Ty[0] = 'B'; Ty[1] = ' '; Ty[2] = ' '; Ty[3] = '\0';
            } else if (Desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
                Ty[0] = 'P'; Ty[1] = 'U'; Ty[2] = 'S'; Ty[3] = '\0';
            } else if (Desc->Type == AMP_FIFO_TYPE_EOS) {
                Ty[0] = 'E'; Ty[1] = 'O'; Ty[2] = 'S'; Ty[3] = '\0';
            } else {
                Ty[0] = 'J'; Ty[1] = 'P'; Ty[2] = 'G'; Ty[3] = '\0';
            }

            if (LogMuxer) {
                AmbaPrint("Sec[%d] %s pts:%8lld 0x%08x 0x%X", encSecTotalFrames, Ty, Desc->Pts, Desc->StartAddr, Desc->Size);
            }
        } else {
            while (Er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(VirtualSecFifoHdlr, &Desc, 0);
            }
        }

        if (encSecBytesStorageTotal > ClipMaxStorageSize) {
            if (outputSecFile && (Desc->Type<=AMP_FIFO_TYPE_I_FRAME)) {
                UT_VideoEncodefsync((void *)outputSecFile);
                UT_VideoEncodefclose((void *)outputSecFile);
                FnoSecSplit++;
                UT_VideoEncodefsync((void *)outputSecIdxFile);
                UT_VideoEncodefclose((void *)outputSecIdxFile);
                outputSecFile = NULL;
                outputSecIdxFile = NULL;
                FileOffset = 0;
            }
        }
        if (((!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) || (encSecBytesStorageTotal > ClipMaxStorageSize) || ((EncPriSpecH264==0)&& (MjpegFmt==0 || (encSecTotalFrames==0))))&&(Desc->Size != AMP_FIFO_MARK_EOS)) {
            if (outputSecFile == NULL) {
                char Fn[80];
                char mdASCII[3] = {'w','+','\0'};
                FORMAT_USER_DATA_s Udta;

                encSecBytesStorageTotal = 0;
                if (EncSecSpecH264 || MjpegFmt) {
                    if (FnoSecSplit) {
                        sprintf(Fn,"%s:\\OU%02d%04d_s.%s", DefaultSlot, FnoSecSplit, fnoSec,EncSecSpecH264?"h264":"mjpg");
                    } else {
                        sprintf(Fn,"%s:\\OUT_%04d_s.%s", DefaultSlot, fnoSec,EncSecSpecH264?"h264":"mjpg");
                    }
                    outputSecFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                } else {
                    sprintf(Fn,"%s:\\OU%02d%04d_s.%s", DefaultSlot, fnoSec, (int)encSecTotalFrames, "jpg");
                    outputSecFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                }
                while (outputSecFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    outputSecFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);
                if (EncSecSpecH264) {
                    if (FnoSecSplit) {
                        sprintf(Fn,"%s:\\OU%02d%04d_s.nhnt", DefaultSlot, FnoSecSplit, fnoSec);
                    } else {
                        sprintf(Fn,"%s:\\OUT_%04d_s.nhnt", DefaultSlot, fnoSec);
                    }
                    outputSecIdxFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                    while (outputSecIdxFile==0) {
                        AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                        AmbaKAL_TaskSleep(10);
                        outputSecIdxFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                    }
                    AmbaPrint("%s opened", Fn);

                    if (FnoSecSplit==0) {
                        sprintf(Fn,"%s:\\OUT_%04d_s.udta", DefaultSlot, fnoSec);

                        UdtaSecFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                        while (UdtaSecFile==0) {
                            AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                            AmbaKAL_TaskSleep(10);
                            UdtaSecFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                        }
                        AmbaPrint("%s opened", Fn);
                        Udta.nIdrInterval = GOP_IDR/GOP_N;
                        if (EncodeSystem == 0) {
                            Udta.nTimeScale =  SEC_STREAM_TIMESCALE;
                            Udta.nTickPerPicture = SEC_STREAM_TICK;
                        } else {
                            Udta.nTimeScale = SEC_STREAM_TIMESCALE_PAL;
                            Udta.nTickPerPicture = SEC_STREAM_TICK_PAL;
                        }

                        Udta.nN = 8;
                        Udta.nM = 1;
                        if (EncDualHDStream) {
                            if (SecStreamCustomWidth) {
                                Udta.nVideoWidth = SecStreamCustomWidth;
                                Udta.nVideoHeight = SecStreamCustomHeight;
                            } else {
                                UINT16 OutWidth = 0, OutHeight = 0;
                                MWUT_InputSetDualHDWindow(VideoEncMgt, EncModeIdx, &OutWidth, &OutHeight);
                                Udta.nVideoWidth = OutWidth;
                                Udta.nVideoHeight = OutHeight;
                            }
                        } else {
                            if (SecStreamCustomWidth) {
                                Udta.nVideoWidth = SecStreamCustomWidth;
                                Udta.nVideoHeight = SecStreamCustomHeight;
                            } else {
                                Udta.nVideoWidth = SEC_STREAM_WIDTH;
                                Udta.nVideoHeight = SEC_STREAM_HEIGHT;
                            }
                        }

                        if (EncRotation == AMP_ROTATE_90 || EncRotation == AMP_ROTATE_90_VERT_FLIP || \
                            EncRotation == AMP_ROTATE_270 || EncRotation == AMP_ROTATE_270_VERT_FLIP) {
                            UINT16 Tmp = Udta.nVideoWidth;
                            Udta.nVideoWidth = Udta.nVideoHeight;
                            Udta.nVideoHeight = Tmp;
                        }
                        Udta.nInterlaced = VideoEncMgt[EncModeIdx].Interlace;
                        UT_VideoEncodefwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)UdtaSecFile);
                        UT_VideoEncodefclose((void *)UdtaSecFile);
                    }

                    NhntHeader.Signature[0]='n';
                    NhntHeader.Signature[1]='h';
                    NhntHeader.Signature[2]='n';
                    NhntHeader.Signature[3]='t';
                    if (EncodeSystem == 0) {
                        NhntHeader.TimeStampResolution = SEC_STREAM_TIMESCALE;
                    } else {
                        NhntHeader.TimeStampResolution = SEC_STREAM_TIMESCALE_PAL;
                    }

                    UT_VideoEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputSecIdxFile);
                }
                if (EncSecSpecH264) {
                    BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
                } else {
                    BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
                }
            }

        }
        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_SEC)) {
                if (outputSecFile) {
                    UT_VideoEncodefsync((void *)outputSecFile);
                    UT_VideoEncodefclose((void *)outputSecFile);
                    if (outputSecIdxFile) {
                        UT_VideoEncodefsync((void *)outputSecIdxFile);
                        UT_VideoEncodefclose((void *)outputSecIdxFile);
                    }
                    outputSecFile = NULL;
                    outputSecIdxFile = NULL;
                    FileOffset = 0;
                }
            }

            if (EncodeSystem) {
                Avg = (UINT32)(encSecBytesTotal*8.0*(SEC_STREAM_TIMESCALE_PAL/SEC_STREAM_TICK_PAL)/encSecTotalFrames/1E3);
            } else {
                Avg = (UINT32)(encSecBytesTotal*8.0*(SEC_STREAM_TIMESCALE/SEC_STREAM_TICK)/encSecTotalFrames/1E3);
            }

            AmbaPrint("Secondary Muxer met EOS, total %d frames/fields", encSecTotalFrames);
            AmbaPrint("Secondary Bitrate Average: %d kbps\n", Avg);

            encSecBytesTotal = 0;
            encSecTotalFrames = 0;
            FnoSecSplit = 0;
            fnoSec++;
        } else if (Desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
            //Do Nothing
        } else {
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_SEC)) {
                if (outputSecFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;
                    if (outputSecIdxFile) {
                        UT_VideoEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)outputSecIdxFile);
                    }
                //      AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                    if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                        UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputSecFile);
                    } else {
                        UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputSecFile);
                        if (EncSecSpecH264) {
                            UT_VideoEncodefwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputSecFile);
                        } else {
                            UT_VideoEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputSecFile);
                        }
                    }
                }
            } else {
                AmbaKAL_TaskSleep(1);
            }
            if (EncSecSpecH264==0 && MjpegFmt==0) {
                UT_VideoEncodefsync((void *)outputSecFile);
                UT_VideoEncodefclose((void *)outputSecFile);
                outputSecFile = NULL;
                outputSecIdxFile = NULL;
            }
            encSecBytesTotal += Desc->Size;
            encSecBytesStorageTotal += Desc->Size;
            encSecTotalFrames ++;
        }
        AmpFifo_RemoveEntry(VirtualSecFifoHdlr, 1);
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
static int AmpUT_VideoEnc_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *NumFrames = info;
//    UINT32 cnt;
    AMBA_KAL_SEM_t *pSem = NULL;;

    if (hdlr == VirtualPriFifoHdlr) {
        pSem = &VideoEncPriSem;
    } else if (hdlr == VirtualSecFifoHdlr) {
        pSem = &VideoEncSecSem;
    } else if (hdlr == VirtualJpegFifoHdlr) {
        pSem = &VideoEncJpegSem;
    }

//    AmbaKAL_SemQuery(&VideoEncSem, &cnt);
//    AmbaPrint("AmpUT_FifoCB: %d", cnt);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for(i = 0; i < *NumFrames; i++) {
            AmbaKAL_SemGive(pSem);      // Give semaphore for muxer tasks
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            AmbaKAL_SemGive(pSem);
    }

    return 0;
}

/*************************************** Muxer (Data flow) implementation end ********************************************************/

#ifdef CONFIG_SOC_A12
#define VENC_IS_ROTATE(x)  (x == AMP_ROTATE_90 || x == AMP_ROTATE_90_VERT_FLIP || \
                          x == AMP_ROTATE_270 || x == AMP_ROTATE_270_VERT_FLIP)

/**
 * Vout calculate
 *
 *
 * @return 0
 */
static void AmpUT_VideoEncVoutCalculate(AMP_AREA_s *area, UINT16 areaAspectRatio, AMP_DISP_CHANNEL_IDX_e Channel)
{
    int AspectRatio;
    UINT32 AspectRatioX, AspectRatioY;
    UINT32 DeviceAspectRatioX, DeviceAspectRatioY;
    UINT16 DeviceAspect;
    float DevicePixelAspectRatio;
    UINT16 DeviceWidth, DeviceHeight;

    if (Channel == AMP_DISP_CHANNEL_DCHAN) {
        AMBA_LCD_INFO_s LcdInfo;

        AmbaLCD_GetInfo(Channel, &LcdInfo);
        DeviceWidth = 960;//LcdInfo.Width;
        DeviceHeight = 480;//LcdInfo.Height;
        DeviceAspect = VAR_4x3;//(LcdInfo.AspectRatio.X << 8) | (LcdInfo.AspectRatio.Y);
    } else if (Channel == AMP_DISP_CHANNEL_FCHAN) {
        AMBA_HDMI_MODE_INFO_s HdmiInfo;

        AmbaHDMI_GetModeInfo(&HdmiInfo);
        DeviceWidth = 1920;//HdmiInfo.pDisplayTiming->FrameActiveColWidth;
        DeviceHeight = 1080;//HdmiInfo.pDisplayTiming->FrameActiveRowHeight;
        DeviceAspect = VAR_16x9;
    } else {
        return;
    }

    DeviceAspectRatioX = GET_VAR_X(DeviceAspect);
    DeviceAspectRatioY = GET_VAR_Y(DeviceAspect);
    DevicePixelAspectRatio = (1.0*DeviceAspectRatioX*DeviceHeight) /
                                (1.0*DeviceAspectRatioY*DeviceWidth);

    if (DeviceWidth == 0) {
        area->Width = 720;
        area->Height = 480;
        area->X = 0;
        area->Y = 0;
        return;
    }
    AspectRatio = (areaAspectRatio == VAR_ANY)? DeviceAspect: areaAspectRatio;
    AspectRatioX = GET_VAR_X(AspectRatio);
    AspectRatioY = GET_VAR_Y(AspectRatio);

    if(AspectRatioY == 0) {
        AmbaPrint("Unknown device AR!! No Vout AR adjusting!");
        area->X = 0;
        area->Y = 0;
        area->Width = DeviceWidth;
        area->Height = DeviceHeight;
    } else {
        UINT16 KeepWidth = 1;
        UINT32 DeviceWidhtWide = (DeviceAspectRatioX << 16) / AspectRatioY;
        UINT32 VoutWidhtWide = (AspectRatioX << 16) / AspectRatioY;

        if(DeviceWidhtWide > VoutWidhtWide) {
            KeepWidth = 0;
        }
_CALC:
        if(KeepWidth) {
            UINT16 RealWidth = (UINT16)((float)DeviceWidth*DevicePixelAspectRatio);

            area->Width = DeviceWidth;
            area->X = 0;
            area->Height = RealWidth*AspectRatioY/AspectRatioX;
            area->Height = (area->Height+3) & 0xFFFC;
            area->Y = (DeviceHeight - area->Height) / 2;

            // Result height is too big, use height to recalc constraint by height again.
            if(area->Height > DeviceHeight) {
                KeepWidth = 1 - KeepWidth;
                goto _CALC;
            }
        } else {
            UINT16 RealHeight = (UINT16)((float)DeviceHeight/DevicePixelAspectRatio);

            area->Height = DeviceHeight;
            area->Y = 0;
            area->Width = RealHeight*AspectRatioX/AspectRatioY;
            area->Width = (area->Width+3) & 0xFFFC;
            area->X = (DeviceWidth - area->Width) / 2;

            // Result width is too big, use height to recalc constraint by width again.
            if(area->Width > DeviceWidth) {
                KeepWidth = 1 - KeepWidth;
                goto _CALC;
            }
        }
    }
}

static UINT8 VideoEncCalibSBPInit = 0;
static UINT8 VideoEncCalibWarpInit = 0;
static UINT8 VideoEncCalibCAInit = 0;
static UINT8 VideoEncCalibVigInit = 0;

void AmpUT_VideoEncCalibrationHook(AMBA_DSP_IMG_MODE_CFG_s ImgMode)
{
    extern Cal_Obj_s* AmpUT_CalibGetObj(UINT32 CalId);
    extern int AmbaUT_CalibSetDspMode(AMBA_DSP_IMG_MODE_CFG_s *pMode);
    Cal_Obj_s *CalObj = 0;

    AmbaUT_CalibSetDspMode(&ImgMode);

    //SBP
    if (CalibEnable & CALIB_SBP) {
        if (VideoEncCalibSBPInit == 0) {
            extern int AmpUTCalibBPC_Init(Cal_Obj_s *CalObj);
            /* Init will do following action :
             * 1) Init SBPCalibModule
             * 2) Decompressed all Maps
             * 3) Set Map information
             */
            CalObj = AmpUT_CalibGetObj(CAL_BPC_ID);
            AmpUTCalibBPC_Init(CalObj);
            VideoEncCalibSBPInit = 1;
        } else {
            extern int AmpUTCalibBPC_MapUpdate(UINT8 JobID);
            /* Only need to Update Map */
            AmpUTCalibBPC_MapUpdate(0);
        }
    }

    //Warp
    if (CalibEnable & CALIB_WARP) {
        if (VideoEncCalibWarpInit == 0) {
            extern int AmpUTCalibWarp_Init(Cal_Obj_s *CalObj);

            CalObj = AmpUT_CalibGetObj(CAL_WARP_ID);
            AmpUTCalibWarp_Init(CalObj);
            VideoEncCalibWarpInit = 1;
        }
    }

    //CA
    if (CalibEnable & CALIB_CA) {
        if (VideoEncCalibCAInit == 0) {
            extern int AmpUTCalibCA_Init(Cal_Obj_s *CalObj);

            CalObj = AmpUT_CalibGetObj(CAL_CA_ID);
            AmpUTCalibCA_Init(CalObj);
            VideoEncCalibCAInit = 1;
        }
    }

    //Vig
    if (CalibEnable & CALIB_VIG) {
        if (VideoEncCalibVigInit == 0) {
            extern int AmpUTCalibVignette_Init(Cal_Obj_s *CalObj);

            CalObj = AmpUT_CalibGetObj(CAL_VIGNETTE_ID);
            AmpUTCalibVignette_Init(CalObj);
            VideoEncCalibVigInit = 1;
        } else {
            extern void AmpUTCalibVignette_MapUpdate(void);
            AmpUTCalibVignette_MapUpdate();
        }
    }
}

/**
 * UnitTest: Fake and default iso config
 *
 * @return 0 - success, -1 - fail
 */
int AmpUT_VideoIsoConfigSet(AMBA_DSP_IMG_ALGO_MODE_e isoMode, AMBA_DSP_IMG_MODE_CFG_s *imgMode)
{
    extern VIDEO_ISO_CONFIG_s AmpUT_VideoIsoConfig;
    extern const UINT8 VideoCc3DConfig[];
    extern const UINT8 VideoCcRegConfig[];

    /* Prepare filters, same as Amba_Img_Set_Video_Pipe_Ctrl_Params() */
    if (AmpUT_VideoIsoConfig.Rgb2YuvInfo.Enable) {
        AmbaDSP_ImgSetRgbToYuvMatrix(imgMode, &(AmpUT_VideoIsoConfig.Rgb2YuvInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.StaticBlackLevelInfo.Enable) {
        AmbaDSP_ImgSetStaticBlackLevel(imgMode, &AmpUT_VideoIsoConfig.StaticBlackLevelInfo.Data);
    }

    if (AmpUT_VideoIsoConfig.AntiAliasingInfo.Enable) {
        AmbaDSP_ImgSetAntiAliasing(imgMode, &AmpUT_VideoIsoConfig.AntiAliasingInfo.Data);
    }

    if (AmpUT_VideoIsoConfig.BadPixelCorrectionInfo.Enable) {
        AmbaDSP_ImgSetDynamicBadPixelCorrection(imgMode, &(AmpUT_VideoIsoConfig.BadPixelCorrectionInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.CfaNoiseFilterInfo.Enable) {
        AmbaDSP_ImgSetCfaNoiseFilter(imgMode, &(AmpUT_VideoIsoConfig.CfaNoiseFilterInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.LocalExposureInfo.Enable) {
        AmbaDSP_ImgSetLocalExposure(imgMode, &(AmpUT_VideoIsoConfig.LocalExposureInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.DemosaicInfo.Enable) {
        AmbaDSP_ImgSetDemosaic(imgMode, &(AmpUT_VideoIsoConfig.DemosaicInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.ColorCorrectionInfo.Enable) {
        AmpUT_VideoIsoConfig.ColorCorrectionRegInfo.Data.RegSettingAddr = (UINT32)VideoCcRegConfig;
        AmbaDSP_ImgSetColorCorrectionReg(imgMode, &(AmpUT_VideoIsoConfig.ColorCorrectionRegInfo.Data));

        AmpUT_VideoIsoConfig.ColorCorrectionInfo.Data.MatrixThreeDTableAddr = (UINT32)VideoCc3DConfig;
        AmbaDSP_ImgSetColorCorrection(imgMode, &(AmpUT_VideoIsoConfig.ColorCorrectionInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.ToneCurveInfo.Enable) {
        AmbaDSP_ImgSetToneCurve(imgMode, &(AmpUT_VideoIsoConfig.ToneCurveInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.ChromaScaleInfo.Enable) {
        AmbaDSP_ImgSetChromaScale(imgMode, &(AmpUT_VideoIsoConfig.ChromaScaleInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.ChromaMedianFilterInfo.Enable) {
        AmbaDSP_ImgSetChromaMedianFilter(imgMode, &(AmpUT_VideoIsoConfig.ChromaMedianFilterInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.CDNR.Enable) {
        AmbaDSP_ImgSetColorDependentNoiseReduction(imgMode, &(AmpUT_VideoIsoConfig.CDNR.Data));
    }

    if (AmpUT_VideoIsoConfig.LumaProcess.Enable) {
        AmbaDSP_ImgSetLumaProcessingMode(imgMode, &(AmpUT_VideoIsoConfig.LumaProcess.Data));
    }

    if (AmpUT_VideoIsoConfig.Asf.Enable) {
        AmbaDSP_ImgSetAdvanceSpatialFilter(imgMode, &(AmpUT_VideoIsoConfig.Asf.Data));
    }

    if (AmpUT_VideoIsoConfig.SharpenABoth.Enable) {
        AmbaDSP_ImgSet1stSharpenNoiseBoth(imgMode, &(AmpUT_VideoIsoConfig.SharpenABoth.Data));
    }

    if (AmpUT_VideoIsoConfig.SharpenNoise.Enable) {
        AmbaDSP_ImgSet1stSharpenNoiseNoise(imgMode, &(AmpUT_VideoIsoConfig.SharpenNoise.Data));
    }

    if (AmpUT_VideoIsoConfig.SharpenAFir.Enable) {
        AmbaDSP_ImgSet1stSharpenNoiseSharpenFir(imgMode, &(AmpUT_VideoIsoConfig.SharpenAFir.Data));
    }

    if (AmpUT_VideoIsoConfig.SharpenCoring.Enable) {
        AmbaDSP_ImgSet1stSharpenNoiseSharpenCoring(imgMode, &(AmpUT_VideoIsoConfig.SharpenCoring.Data));
    }

    if (AmpUT_VideoIsoConfig.SharpenCoringIdxScale.Enable) {
        AmbaDSP_ImgSet1stSharpenNoiseSharpenCoringIndexScale(imgMode, &(AmpUT_VideoIsoConfig.SharpenCoringIdxScale.Data));
    }

    if (AmpUT_VideoIsoConfig.SharpenMinCoring.Enable) {
        AmbaDSP_ImgSet1stSharpenNoiseSharpenMinCoringResult(imgMode, &(AmpUT_VideoIsoConfig.SharpenMinCoring.Data));
    }

    if (AmpUT_VideoIsoConfig.SharpenScaleCoring.Enable) {
        AmbaDSP_ImgSet1stSharpenNoiseSharpenScaleCoring(imgMode, &(AmpUT_VideoIsoConfig.SharpenScaleCoring.Data));
    }

    if (AmpUT_VideoIsoConfig.LiCDNR.Enable) {
        AmbaDSP_ImgSetColorDependentNoiseReduction(imgMode, &(AmpUT_VideoIsoConfig.LiCDNR.Data));
    }

    if (AmpUT_VideoIsoConfig.ChromaFilterInfo.Enable) {
        AmbaDSP_ImgSetChromaFilter(imgMode, &(AmpUT_VideoIsoConfig.ChromaFilterInfo.Data));
    }

    if (AmpUT_VideoIsoConfig.LiWideChromaFilterComb.Enable) {
        AmbaDSP_ImgSetWideChromaFilterCombine(imgMode, &(AmpUT_VideoIsoConfig.LiWideChromaFilterComb.Data));
    }

    if (AmpUT_VideoIsoConfig.GbGrMismatch.Enable) {
        AmbaDSP_ImgSetGbGrMismatch(imgMode, &(AmpUT_VideoIsoConfig.GbGrMismatch.Data));
    }

    if (isoMode == AMBA_DSP_IMG_ALGO_MODE_HISO) {
        if (AmpUT_VideoIsoConfig.CfaLeakageFilter.Enable) {
            AmbaDSP_ImgSetHighIsoCfaLeakageFilter(imgMode, &(AmpUT_VideoIsoConfig.CfaLeakageFilter.Data));
        }

        if (AmpUT_VideoIsoConfig.HiAntiAliasing.Enable) {
            AmbaDSP_ImgSetHighIsoAntiAliasing(imgMode, &AmpUT_VideoIsoConfig.HiAntiAliasing.Data);
        }

        if (AmpUT_VideoIsoConfig.HiDynamicBadPixelCorrection.Enable) {
            AmbaDSP_ImgSetHighIsoDynamicBadPixelCorrection(imgMode, &(AmpUT_VideoIsoConfig.HiDynamicBadPixelCorrection.Data));
        }

        if (AmpUT_VideoIsoConfig.HiCfaNoiseFilter.Enable) {
            AmbaDSP_ImgSetHighIsoCfaNoiseFilter(imgMode, &(AmpUT_VideoIsoConfig.HiCfaNoiseFilter.Data));
        }

        if (AmpUT_VideoIsoConfig.HiGbGrMismatch.Enable) {
            AmbaDSP_ImgSetHighIsoGbGrMismatch(imgMode, &(AmpUT_VideoIsoConfig.HiGbGrMismatch.Data));
        }

        if (AmpUT_VideoIsoConfig.HiDemosaic.Enable) {
            AmbaDSP_ImgSetHighIsoDemosaic(imgMode, &(AmpUT_VideoIsoConfig.HiDemosaic.Data));
        }

        if (AmpUT_VideoIsoConfig.HiChromaMedianFilter.Enable) {
            AmbaDSP_ImgSetHighIsoChromaMedianFilter(imgMode, &(AmpUT_VideoIsoConfig.HiChromaMedianFilter.Data));
        }

        if (AmpUT_VideoIsoConfig.HiDeferColorCorrection.Enable) {
            AmbaDSP_ImgSetHighIsoDeferColorCorrection(imgMode, &(AmpUT_VideoIsoConfig.HiDeferColorCorrection.Data));
        }

    //HISO Luma ASF
#if 0
    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoAsfUpdate == 1){
        AmbaDSP_ImgSetHighIsoAdvanceSpatialFilter(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoAsf);
    }

    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoHighAsfUpdate == 1){
        AmbaDSP_ImgSetHighIsoHighAdvanceSpatialFilter(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoHighAsf);
    }

    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoMedAsfUpdate == 1){
        AmbaDSP_ImgSetHighIsoMedAdvanceSpatialFilter(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoMedAsf);
        pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoMedAsfUpdate = 0;
    }

    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoLowAsfUpdate == 1){
        AmbaDSP_ImgSetHighIsoLowAdvanceSpatialFilter(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoLowAsf);
        pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoLowAsfUpdate = 0;
    }

    //High Sharpen
    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoHighSharpenBothUpdate == 1){
        AmbaDSP_ImgSetHighIsoHighSharpenNoiseBoth(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoHighSharpenBoth);
        pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoHighSharpenBothUpdate = 0;
    }
#endif

        if (AmpUT_VideoIsoConfig.HighSharpenNoiseNoise.Enable) {
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseNoise(imgMode, &(AmpUT_VideoIsoConfig.HighSharpenNoiseNoise.Data));
        }

        if (AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenFir.Enable) {
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenFir(imgMode, &(AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenFir.Data));
        }

        if (AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenCoring.Enable) {
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenCoring(imgMode, &(AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenCoring.Data));
        }

        if (AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenCoringIndexScale.Enable) {
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenCoringIndexScale(imgMode, &(AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenCoringIndexScale.Data));
        }

        if (AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenMinCoringResult.Enable) {
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenMinCoringResult(imgMode, &(AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenMinCoringResult.Data));
        }

        if (AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenScaleCoring.Enable) {
            AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenScaleCoring(imgMode, &(AmpUT_VideoIsoConfig.HighSharpenNoiseSharpenScaleCoring.Data));
        }

    //Med Sharpen
#if 0
    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoMedSharpenBothUpdate == 1){
        AmbaDSP_ImgSetHighIsoMedSharpenNoiseBoth(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoMedSharpenBoth);
        pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoMedSharpenBothUpdate = 0;
    }
#endif
        if (AmpUT_VideoIsoConfig.MedSharpenNoiseNoise.Enable) {
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseNoise(imgMode, &(AmpUT_VideoIsoConfig.MedSharpenNoiseNoise.Data));
        }

        if (AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenFir.Enable) {
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenFir(imgMode, &(AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenFir.Data));
        }

        if (AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenCoring.Enable) {
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenCoring(imgMode, &(AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenCoring.Data));
        }

        if (AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenCoringIndexScale.Enable) {
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenCoringIndexScale(imgMode, &(AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenCoringIndexScale.Data));
        }

        if (AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenMinCoringResult.Enable) {
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenMinCoringResult(imgMode, &(AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenMinCoringResult.Data));
        }

        if (AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenScaleCoring.Enable) {
            AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenScaleCoring(imgMode, &(AmpUT_VideoIsoConfig.MedSharpenNoiseSharpenScaleCoring.Data));
        }


    //HISO Chroma ASF
#if 0
    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoChromaAsfUpdate == 1){
        AmbaDSP_ImgSetHighIsoChromaAdvanceSpatialFilter(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoChromaAsf);
        pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoChromaAsfUpdate = 0;
    }
#endif

        //HISO Chroma filter
        if (AmpUT_VideoIsoConfig.ChromaFilterPre.Enable) {
            AmbaDSP_ImgHighIsoSetChromaFilterPre(imgMode, &(AmpUT_VideoIsoConfig.ChromaFilterPre.Data));
        }

        if (AmpUT_VideoIsoConfig.ChromaFilterHigh.Enable) {
            AmbaDSP_ImgHighIsoSetChromaFilterHigh(imgMode, &(AmpUT_VideoIsoConfig.ChromaFilterHigh.Data));
        }

        if (AmpUT_VideoIsoConfig.ChromaFilterMed.Enable) {
            AmbaDSP_ImgHighIsoSetChromaFilterMed(imgMode, &(AmpUT_VideoIsoConfig.ChromaFilterMed.Data));
        }

        if (AmpUT_VideoIsoConfig.ChromaFilterLow.Enable) {
            AmbaDSP_ImgHighIsoSetChromaFilterLow(imgMode, &(AmpUT_VideoIsoConfig.ChromaFilterLow.Data));
        }

        if (AmpUT_VideoIsoConfig.ChromaFilterVeryLow.Enable) {
            AmbaDSP_ImgHighIsoSetChromaFilterVeryLow(imgMode, &(AmpUT_VideoIsoConfig.ChromaFilterVeryLow.Data));
        }

        if (AmpUT_VideoIsoConfig.ChromaFilterLowVeryLow.Enable) {
            AmbaDSP_ImgHighIsoSetChromaFilterLowVeryLow(imgMode, &(AmpUT_VideoIsoConfig.ChromaFilterLowVeryLow.Data));
        }

    //HISO Chroma Combine
#if 0
    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoChromaFilterMedCombineUpdate == 1){
        AmbaDSP_ImgHighIsoSetChromaFilterMedCombine(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoChromaFilterMedCombine);
    }
    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoChromaFilterLowCombineUpdate == 1){
        AmbaDSP_ImgHighIsoSetChromaFilterLowCombine(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoChromaFilterLowCombine);
    }
    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoChromaFilterVLowCombineUpdate == 1){
        AmbaDSP_ImgHighIsoSetChromaFilterVeryLowCombine(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoChromaFilterVLowCombine);
    }


    //HISO Luma Combine
    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoLumaFilterCombineUpdate == 1){
        AmbaDSP_ImgHighIsoSetLumaNoiseCombine(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoLumaFilterCombine);
    }
    if (pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoLowAsfCombineUpdate == 1){
        AmbaDSP_ImgHighIsoSetLowASFCombine(&Mode, &pVDspHdlrParam[chNo]->VideoPipeCtrl.HIsoLowAsfCombine);
    }
#endif

    }

    return OK;
}

int AmpUT_VideoEnc_AqpPriStream(AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *StrmHdlr)
{
    ADJ_AQP_INFO_s AqpInfo;

    memset(&AqpInfo, 0x0, sizeof(ADJ_AQP_INFO_s));
    AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_AQP_INFO, 0/*chNo*/, (UINT32)&AqpInfo, 0);
    if (AqpInfo.UpdateFlg) {
        AMBA_ENCMONITOR_RUNTIME_QUALITY_CFG_s Cfg;

        memset(&Cfg, 0, sizeof(AMBA_ENCMONITOR_RUNTIME_QUALITY_CFG_s));
        Cfg.Cmd |= QC_QMODEL;
        Cfg.AQPStrength = AqpInfo.AQPParams.Value[0];
        Cfg.Intra16x16Bias = AqpInfo.AQPParams.Value[1];
        Cfg.Intra4x4Bias = AqpInfo.AQPParams.Value[2];
        Cfg.Inter16x16Bias = AqpInfo.AQPParams.Value[3];
        Cfg.Inter8x8Bias = AqpInfo.AQPParams.Value[4];
        Cfg.Direct16x16Bias = AqpInfo.AQPParams.Value[5];
        Cfg.Direct8x8Bias = AqpInfo.AQPParams.Value[6];
        Cfg.MELambdaQpOffset = AqpInfo.AQPParams.Value[7];
        Cfg.Alpha = AqpInfo.AQPParams.Value[8];
        Cfg.Beta = AqpInfo.AQPParams.Value[9];
        AmbaEncMonitor_SetRuntimeQuality(StrmHdlr, &Cfg);

        AqpInfo.UpdateFlg = 0;
        AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_AQP_INFO, 0/*chNo*/, (UINT32)&AqpInfo, 0);
    }

    return 0;
}

int AmpUT_VideoEnc_AqpSecStream(AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *StrmHdlr)
{
    ADJ_AQP_INFO_s AqpInfo;

    memset(&AqpInfo, 0x0, sizeof(ADJ_AQP_INFO_s));
    AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_AQP_INFO, 0/*chNo*/, (UINT32)&AqpInfo, 0);
    if (AqpInfo.UpdateFlg) {
        AMBA_ENCMONITOR_RUNTIME_QUALITY_CFG_s Cfg;

        memset(&Cfg, 0, sizeof(AMBA_ENCMONITOR_RUNTIME_QUALITY_CFG_s));
        Cfg.Cmd |= QC_QMODEL;
        Cfg.AQPStrength = AqpInfo.AQPParams.Value[0];
        Cfg.Intra16x16Bias = AqpInfo.AQPParams.Value[1];
        Cfg.Intra4x4Bias = AqpInfo.AQPParams.Value[2];
        Cfg.Inter16x16Bias = AqpInfo.AQPParams.Value[3];
        Cfg.Inter8x8Bias = AqpInfo.AQPParams.Value[4];
        Cfg.Direct16x16Bias = AqpInfo.AQPParams.Value[5];
        Cfg.Direct8x8Bias = AqpInfo.AQPParams.Value[6];
        Cfg.MELambdaQpOffset = AqpInfo.AQPParams.Value[7];
        Cfg.Alpha = AqpInfo.AQPParams.Value[8];
        Cfg.Beta = AqpInfo.AQPParams.Value[9];
        AmbaEncMonitor_SetRuntimeQuality(StrmHdlr, &Cfg);

        AqpInfo.UpdateFlg = 0;
        AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_AQP_INFO, 0/*chNo*/, (UINT32)&AqpInfo, 0);
    }

    return 0;
}

#define DAY_LUMA_NORMAL      (0)
#define DAY_LUMA_HDR         (1)
#define DAY_LUMA_OVERSAMPLE  (2)

static int AmpUT_VideoEnc_GetDayLumaThresholdCB(int mode, UINT32 *threshold)
{
    //AmbaPrintColor(BLUE, "Day Luma Mode %d", mode);
    switch (mode) {
    case DAY_LUMA_NORMAL:
        *threshold = 124900;
        break;
    case DAY_LUMA_HDR:
        *threshold = 181550;
        break;
    case DAY_LUMA_OVERSAMPLE:
        *threshold = 259400;
        break;
    default:
        *threshold = 124900;
        break;
    }

    return 0;
}

#define COMPLEX_DAY_NORMAL       (0)
#define COMPLEX_NIGHT_NORMAL     (1)
#define COMPLEX_DAY_HDR          (2)
#define COMPLEX_NIGHT_HDR        (3)
#define COMPLEX_DAY_OVERSAMPLE   (4)
#define COMPLEX_NIGHT_OVERSAMPLE (5)
static int AmpUT_VideoEnc_GetSceneComplexityRangeCB(int mode, UINT32 *complexMin, UINT32 *complexMid, UINT32 *complexMax)
{
    //AmbaPrintColor(BLUE, "Complex Mode: %d", mode);
    switch (mode) {
    case COMPLEX_DAY_NORMAL:
        *complexMin = 31000; *complexMid = 52900; *complexMax = 66200;
        break;
    case COMPLEX_NIGHT_NORMAL:
        *complexMin = 18700; *complexMid = 32600; *complexMax = 40800;
        break;
    case COMPLEX_DAY_HDR:
        *complexMin = 24800; *complexMid = 40000; *complexMax = 50000;
        break;
    case COMPLEX_NIGHT_HDR:
        *complexMin = 13600; *complexMid = 26000; *complexMax = 32500;
        break;
    case COMPLEX_DAY_OVERSAMPLE:
        *complexMin = 29100; *complexMid = 46900; *complexMax = 58700;
        break;
    case COMPLEX_NIGHT_OVERSAMPLE:
        *complexMin = 16900; *complexMid = 30400; *complexMax = 38000;
        break;
    default:
        *complexMin = 31000; *complexMid = 52900; *complexMax = 66200;
        break;
    }
    return 0;
}

static UINT8 VideoEncPriQpIsZero = 0;
static int VideoEncPriCurrQpMinI = 0;
static int VideoEncPriCurrQpMaxI = 0;
static int VideoEncPriCurrQpMinP = 0;
static int VideoEncPriCurrQpMaxP = 0;
static int VideoEncPriCurrQpMinB = 0;
static int VideoEncPriCurrQpMaxB = 0;

static UINT8 VideoEncSecQpIsZero = 0;
static int VideoEncSecCurrQpMinI = 0;
static int VideoEncSecCurrQpMaxI = 0;
static int VideoEncSecCurrQpMinP = 0;
static int VideoEncSecCurrQpMaxP = 0;
static int VideoEncSecCurrQpMinB = 0;
static int VideoEncSecCurrQpMaxB = 0;
#define BITRATE_UNDER_SPEC (1)
#define BITRATE_FIT_SPEC   (2)
#define BITRATE_OVER_SPEC  (3)
static int AmpUT_VideoEnc_QpAdjustmentCB(int mode, UINT8 *isQpModify, AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *strmHdlr, AMBA_IMG_ENC_MONITOR_BITRATE_HDLR_s *hdlr)
{
    AMBA_ENCMONITOR_RUNTIME_QUALITY_CFG_s Cfg = {0};
    int *QpMinI = NULL, *QpMaxI = NULL;
    int *QpMinP = NULL, *QpMaxP = NULL;
    int *QpMinB = NULL, *QpMaxB = NULL;
    UINT8 IsFound = 1, *QpIsZero = NULL;

    *isQpModify = 0;
    if ((hdlr) == BrcHdlrPri) {
        QpMinI = &VideoEncPriCurrQpMinI;
        QpMaxI = &VideoEncPriCurrQpMaxI;
        QpMinP = &VideoEncPriCurrQpMinP;
        QpMaxP = &VideoEncPriCurrQpMaxP;
        QpMinB = &VideoEncPriCurrQpMinB;
        QpMaxB = &VideoEncPriCurrQpMaxB;
        QpIsZero = &VideoEncPriQpIsZero;
    } else if ((hdlr) == BrcHdlrSec) {
        QpMinI = &VideoEncSecCurrQpMinI;
        QpMaxI = &VideoEncSecCurrQpMaxI;
        QpMinP = &VideoEncSecCurrQpMinP;
        QpMaxP = &VideoEncSecCurrQpMaxP;
        QpMinB = &VideoEncSecCurrQpMinB;
        QpMaxB = &VideoEncSecCurrQpMaxB;
        QpIsZero = &VideoEncSecQpIsZero;
    } else {
        IsFound = 0;
    }

    /*
     * If bit rate is under SPEC, that means scene may be too simple that needs to change QP.
     * Below is SAMPLE code to show how to change QP settings to increase bit rate.
     *
     */
    if (IsFound == 1) {
        switch (mode) {
        case BITRATE_UNDER_SPEC:
            if ((*QpMinI == 0) && (*QpMinP == 0)) {
                if (*QpIsZero == 0) {
                    *QpIsZero = 1;
                    AmbaPrintColor(RED, "[AmpUT] <QP_adjust> MinQP of hdlr(0x%X) already set as 0", hdlr);
                }
            } else {
                *QpMinI = ((*QpMinI - 1) >= 0)? (*QpMinI - 1): 0;
                *QpMinP = ((*QpMinP - 1) >= 0)? (*QpMinP - 1): 0;
                *QpMinB = ((*QpMinB - 1) >= 0)? (*QpMinB - 1): 0;
                Cfg.Cmd |= QC_QP;
                Cfg.QpMinI = (UINT8) *QpMinI;
                Cfg.QpMaxI = (UINT8) *QpMaxI;
                Cfg.QpMinP = (UINT8) *QpMinP;
                Cfg.QpMaxP = (UINT8) *QpMaxP;
                Cfg.QpMinB = (UINT8) *QpMinB;
                Cfg.QpMaxB = (UINT8) *QpMaxB;
                AmbaEncMonitor_SetRuntimeQuality(strmHdlr, &Cfg);
                *isQpModify = 1;
            }
            break;
        case BITRATE_FIT_SPEC:
            break;
        case BITRATE_OVER_SPEC:
            break;
        default:
            break;
        }
    } else {
        AmbaPrint("Unknown brc hdlr 0x%X", hdlr);
    }

    return 0;
}
#endif

/* Initial JPEG DQT */
void AmpUT_VidoeEnc_InitMJpegDqt(UINT8 *qTable, int quality)
{
    int i, scale, temp;

    /** for jpeg brc; return the quantization table*/
    if (quality == -1) {
        memcpy(qTable, MjpegStandardQuantMatrix, 128);
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
        temp = ((long) MjpegStandardQuantMatrix[i] * scale + 50L) / 100L;
        /* limit the values to the valid range */
        if (temp <= 0L) temp = 1L;
        if (temp > 255L) temp = 255L; /* max quantizer needed for baseline */
        qTable[i] = temp;
    }
}


/**
 * Generic VideoEnc ImgSchdlr callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_VideoEncImgSchdlrCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMBA_IMG_SCHDLR_EVENT_CFA_STAT_READY:
            if (VideoEnc3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspCfa_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_RGB_STAT_READY:
            if (VideoEnc3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_MAIN_CFA_HIST_READY:
            if (VideoEnc3AEnable) {
//                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                //Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_HDR_CFA_HIST_READY:
            if (VideoEnc3AEnable) {
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
static int AmpUT_VideoEncVinSwitchCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {
        case AMP_VIN_EVENT_INVALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_INVALID info: %X %X", info, ImgSchdlr);

            if (VideoEnc_EisAlgoEnable) {
                AmbaEis_Inactive();
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_DISABLE, 0, 0, 0);
            }

            if (ImgSchdlr) AmbaImgSchdlr_Enable(ImgSchdlr, 0);
            Amba_Img_VIn_Invalid(hdlr, (UINT32 *)NULL);
            break;
        case AMP_VIN_EVENT_VALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_VALID info: %X %X", info, ImgSchdlr);
            if (VideoEnc3AEnable) {
                Amba_Img_VIn_Valid(hdlr, (UINT32 *)NULL);
            }

            if (ImgSchdlr) AmbaImgSchdlr_Enable(ImgSchdlr, 1);
            break;
        case AMP_VIN_EVENT_CHANGED_PRIOR:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_PRIOR info: %X", info);
            if (VideoEnc3AEnable) {
                Amba_Img_VIn_Changed_Prior(hdlr, (UINT32 *)NULL);
            }

        #ifdef CONFIG_SOC_A12
            if (Status == STATUS_LIVEVIEW) { // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window = {0}, DefWindow = {0};
                AMP_AREA_s WindowArea;
                UINT16 FinalAspectRatio;

                if (1) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;

                    WindowArea.Width = 960;
                    WindowArea.Height = (VideoEncMgt[EncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                    WindowArea.X = 0;
                    WindowArea.Y = (480-WindowArea.Height)/2;
                    if (VENC_IS_ROTATE(EncRotation) && LiveViewProcMode == 0) {
                        FinalAspectRatio = GET_VAR_X(VideoEncMgt[EncModeIdx].AspectRatio) |
                            (GET_VAR_Y(VideoEncMgt[EncModeIdx].AspectRatio)<<8);
                    } else {
                        FinalAspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
                    }
                    AmpUT_VideoEncVoutCalculate(&WindowArea, FinalAspectRatio, AMP_DISP_CHANNEL_DCHAN);

                    window.TargetAreaOnPlane.Width = WindowArea.Width;
                    window.TargetAreaOnPlane.Height = WindowArea.Height;
                    window.TargetAreaOnPlane.X = WindowArea.X;
                    window.TargetAreaOnPlane.Y = WindowArea.Y;
                    if(AmpUT_Display_GetWindowCfg(0, &DefWindow) != OK) {
                        AmpUT_Display_Window_Create(0, &window);
                    } else {
                        AmpUT_Display_SetWindowCfg(0, &window);
                    }

                    if (LCDLiveview) {
                        AmpUT_Display_Act_Window(0);
                    } else {
                        AmpUT_Display_DeAct_Window(0);
                    }
                }
            }
            if (Status == STATUS_LIVEVIEW) {
                AMP_DISP_WINDOW_CFG_s window = {0}, DefWindow = {0};
                AMP_AREA_s WindowArea;
                UINT16 FinalAspectRatio;

                if (1) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;

                    WindowArea.Width = (VideoEncMgt[EncModeIdx].AspectRatio == VAR_16x9)? 1920: 1440;
                    WindowArea.Height = 1080;
                    WindowArea.X = (1920 - WindowArea.Width)/2;
                    WindowArea.Y = 0;
                    if (VENC_IS_ROTATE(EncRotation) && LiveViewProcMode == 0) {
                        FinalAspectRatio = GET_VAR_X(VideoEncMgt[EncModeIdx].AspectRatio) |
                            (GET_VAR_Y(VideoEncMgt[EncModeIdx].AspectRatio)<<8);
                    } else {
                        FinalAspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
                    }
                    AmpUT_VideoEncVoutCalculate(&WindowArea, FinalAspectRatio, AMP_DISP_CHANNEL_FCHAN);

                    window.TargetAreaOnPlane.Width = WindowArea.Width;
                    window.TargetAreaOnPlane.Height = WindowArea.Height;
                    window.TargetAreaOnPlane.X = WindowArea.X;
                    window.TargetAreaOnPlane.Y = WindowArea.Y;
                    window.TargetAreaOnPlane.Width = WindowArea.Width;
                    window.TargetAreaOnPlane.Height = WindowArea.Height;
                    window.TargetAreaOnPlane.X = WindowArea.X;
                    window.TargetAreaOnPlane.Y = WindowArea.Y;

                    if(AmpUT_Display_GetWindowCfg(1, &DefWindow) != OK) {
                        AmpUT_Display_Window_Create(1, &window);
                    } else {
                        AmpUT_Display_SetWindowCfg(1, &window);
                    }
                    if (TvLiveview) {
                        AmpUT_Display_Act_Window(1);
                    } else {
                        AmpUT_Display_DeAct_Window(1);
                    }
                }
            }
        #endif
            break;
        case AMP_VIN_EVENT_CHANGED_POST:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_POST info: %X", info);

            if (VideoEnc3AEnable) {
                extern void _Set_AdjTableNo(UINT32 chNo, int AdjTableNo);
                UINT8 IsPhotoLiveView = 0;
                UINT16 PipeMode = IP_EXPERSS_MODE;
                UINT16 AlgoMode = IP_MODE_LISO_VIDEO;
                UINT32 FrameRate = 0;
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo;
                AMBA_SENSOR_MODE_ID_u SensorMode = {0};

                SensorMode.Data = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;
                AmbaSensor_GetModeInfo(VinChannel, SensorMode, &SensorModeInfo);

                if (ImgSchdlr) {
                    AMBA_IMG_SCHDLR_UPDATE_CFG_s SchdlrCfg = {0};

                    AmbaImgSchdlr_GetConfig(ImgSchdlr, &SchdlrCfg);
                    if (LiveViewProcMode && LiveViewAlgoMode) {
                        SchdlrCfg.VideoProcMode = 1;
                        if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                            SchdlrCfg.VideoProcMode |= 0x10;
                        }
                    }
                    SchdlrCfg.AAAStatSampleRate = (MainviewReportRate)? MainviewReportRate: VideoEncMgt[EncModeIdx].ReportRate;
                    AmbaImgSchdlr_SetConfig(ImgSchdlr, &SchdlrCfg);  // One MainViewID (not vin) need one scheduler.
                }

            #ifdef CONFIG_SOC_A12
                { // Load IQ params
                    extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
                    UINT8 IsSensorHdrMode;
                    // Inform 3A LV sensor mode is Hdr or not
                    IsSensorHdrMode = (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE)? 1: 0;
                    AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_HDR_ENABLE, 0/*ChNo*/, (UINT32)IsSensorHdrMode, 0);

                    if (VideoEncIsIqParamInit == 0 || IsSensorHdrMode != VideoEncIsHdrIqParam) {
                        if (IsSensorHdrMode == 0) {
                            App_Image_Init_Iq_Params(0, InputDeviceId);
                        } else if (IsSensorHdrMode == 1) {
                            App_Image_Init_Iq_Params(0, InputDeviceId);
                            App_Image_Init_Iq_Params(1, InputDeviceId);
                        }
                        VideoEncIsIqParamInit = 1;
                        VideoEncIsHdrIqParam = IsSensorHdrMode;
                    }
                }
            #endif

                //inform 3A LiveView pipeline
                if (LiveViewProcMode && LiveViewAlgoMode) {
                    PipeMode = IP_HYBRID_MODE;

                    //Change Adj Table to TA one if necessary
                    if (VideoEncTAEnable) {
                        _Set_AdjTableNo(0, 1);
                    } else {
                        _Set_AdjTableNo(0, -1);
                    }
                } else {
                    PipeMode = IP_EXPERSS_MODE;
                    _Set_AdjTableNo(0, -1);
                }

                AmbaImg_Proc_Cmd(MW_IP_SET_PIPE_MODE, 0/*ChNo*/, (UINT32)PipeMode, 0);

                //inform 3A LiveView Algo
                if (LiveViewAlgoMode == 0 || LiveViewAlgoMode == 1) {
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
                    LiveViewInfo.MainW = VideoEncMgt[EncModeIdx].MainWidth;
                    LiveViewInfo.MainH = VideoEncMgt[EncModeIdx].MainHeight;
                    if (EncodeSystem == 0) {
                        LiveViewInfo.FrameRateInt = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;
                    } else {
                        LiveViewInfo.FrameRateInt = VideoEncMgt[EncModeIdx].TimeScalePAL/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                    }
                    LiveViewInfo.FrameRateInt = UT_VideoFrameRateIntConvert(LiveViewInfo.FrameRateInt);
                    AmbaSensor_GetStatus(VinChannel, &SsrStatus);
                    LiveViewInfo.BinningHNum = InputInfo->HSubsample.FactorNum;
                    LiveViewInfo.BinningHDen = InputInfo->HSubsample.FactorDen;
                    LiveViewInfo.BinningVNum = InputInfo->VSubsample.FactorNum;
                    LiveViewInfo.BinningVDen = InputInfo->VSubsample.FactorDen;
                    AmbaImg_Proc_Cmd(MW_IP_SET_LIVEVIEW_INFO, 0/*ChIndex*/, (UINT32)&LiveViewInfo, 0);
                }

                //inform 3A FrameRate info
                {
                    UINT32 FrameRatex1000 = 0;

                    if (EncodeSystem == 0) {
                        FrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;
                        FrameRatex1000 = VideoEncMgt[EncModeIdx].TimeScale*1000/VideoEncMgt[EncModeIdx].TickPerPicture;
                    } else {
                        FrameRate = VideoEncMgt[EncModeIdx].TimeScalePAL/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                        FrameRatex1000 = VideoEncMgt[EncModeIdx].TimeScalePAL*1000/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                    }
                    FrameRate = UT_VideoFrameRateIntConvert(FrameRate);
                    AmbaImg_Proc_Cmd(MW_IP_SET_FRAME_RATE, 0/*ChIndex*/, FrameRate, FrameRatex1000);
                }

                Amba_Img_VIn_Changed_Post(hdlr, (UINT32 *)NULL);

                // Slow Shutter
                {
                    AE_CONTROL_s AeCtrlMode;
                    AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
                    UINT32 MinFrameTime;
                    AmbaSensor_GetStatus(VinChannel, &SensorStatus);
                    MinFrameTime = SensorStatus.ModeInfo.MinFrameRate.TimeScale/SensorStatus.ModeInfo.MinFrameRate.NumUnitsInTick;

                    AmbaImg_Proc_Cmd(MW_IP_GET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);
                    if (SlowShutterEnable && (FrameRate >= 50)) {
                        UINT8 MaxSSRatio;
                        MaxSSRatio = FrameRate/MinFrameTime;
                        if (MaxSSRatio>=4) {
                            AeCtrlMode.SlowShutter = 2;
                        } else if (MaxSSRatio>=2){
                            AeCtrlMode.SlowShutter = 1;
                        } else {
                            AeCtrlMode.SlowShutter = 0;
                        }
                    } else {
                        AeCtrlMode.SlowShutter = 0;
                    }
                    AmbaImg_Proc_Cmd(MW_IP_SET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);
                }
            } else {
                if (LiveViewProcMode && LiveViewAlgoMode) {
                    //check ituner existed or not
                    ITUNER_INFO_s ItunerInfo;
                    AmbaTUNE_Get_ItunerInfo(&ItunerInfo);

                    if (ItunerInfo.TuningAlgoMode.AlgoMode == (AMBA_DSP_IMG_ALGO_MODE_e)LiveViewAlgoMode) {
                        AMBA_IMG_SCHDLR_EXP_INFO_s CurExpInfo;
                        AMBA_IMG_SCHDLR_EXP_s ExpInfo;
                        int CtxId;

                        memset(&CurExpInfo, 0x0, sizeof(AMBA_IMG_SCHDLR_EXP_INFO_s));
                        AmbaImgSchdlr_GetExposureInfo(0/*MainViewID*/, &CurExpInfo);
                        memcpy(&ExpInfo.Info, &CurExpInfo, sizeof(AMBA_IMG_SCHDLR_EXP_INFO_s));

                        ExpInfo.Type = AMBA_IMG_SCHDLR_SET_TYPE_DIRECT;
                        ExpInfo.Info.AdjUpdated = 1;
                        memcpy(&ExpInfo.Info.Mode, &ItunerInfo.TuningAlgoMode, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
                        CtxId = AmbaImgSchdlr_GetIsoCtxIndex(0/*MainViewID*/, ExpInfo.Info.Mode.AlgoMode);
                        if (CtxId >= 0) {
                            ExpInfo.Info.Mode.ContextId = CtxId;
                        }

                        AmbaImgSchdlr_SetExposure(0/*MainViewID*/, &ExpInfo);
                    } else {
                        AMBA_DSP_IMG_MODE_CFG_s ImgMode;

                        ImgMode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
                        ImgMode.AlgoMode = (LiveViewAlgoMode == 1)? AMBA_DSP_IMG_ALGO_MODE_LISO: AMBA_DSP_IMG_ALGO_MODE_HISO;
                        ImgMode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_FV;
                        ImgMode.ContextId = AmbaImgSchdlr_GetIsoCtxIndex(0/*TBD*/, ImgMode.AlgoMode);
#ifdef CONFIG_SOC_A12
                        //Set Adj
                        AmpUT_VideoIsoConfigSet(ImgMode.AlgoMode, &ImgMode);
#endif
                    }
                }
            }

        #ifdef CONFIG_SOC_A12
            if (CalibEnable) {
                AMBA_DSP_IMG_MODE_CFG_s ImgMode;

                memset(&ImgMode, 0x0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
                ImgMode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
                ImgMode.AlgoMode = (LiveViewAlgoMode == 2)? AMBA_DSP_IMG_ALGO_MODE_HISO: AMBA_DSP_IMG_ALGO_MODE_LISO;
                ImgMode.ContextId = AmbaImgSchdlr_GetIsoCtxIndex(0/*TBD*/, ImgMode.AlgoMode);
                AmpUT_VideoEncCalibrationHook(ImgMode);
            }

            if (VideoEnc_EisAlgoEnable) {
//                AmbaEis_Inactive();
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_SET_FOCAL_LENGTH, 360, 0, 0);
//                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_DISABLE, 0, 0, 0);
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_INIT, 0, 0, 0);
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_MODE_INIT, 0, 0, 0);
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_ENABLE, 0, 0, 0);
            }

        #endif

        #ifdef CONFIG_SOC_A9
            if (Status == STATUS_LIVEVIEW) { // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window;
                AMP_AREA_s WindowArea;

                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

                if (encLcdWinHdlr) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;

                    WindowArea.Width = 960;
                    WindowArea.Height = (VideoEncMgt[EncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                    WindowArea.X = 0;
                    WindowArea.Y = (480-WindowArea.Height)/2;
                    window.TargetAreaOnPlane.Width = WindowArea.Width;
                    window.TargetAreaOnPlane.Height = WindowArea.Height;
                    window.TargetAreaOnPlane.X = WindowArea.X;
                    window.TargetAreaOnPlane.Y = WindowArea.Y;
                    AmpDisplay_SetWindowCfg(encLcdWinHdlr, &window);
                    if (LCDLiveview) {
                        AmpDisplay_SetWindowActivateFlag(encLcdWinHdlr, 1);
                    } else {
                        AmpDisplay_SetWindowActivateFlag(encLcdWinHdlr, 0);
                    }
                }
                AmpDisplay_Update(LCDHdlr);
            }
            if (Status == STATUS_LIVEVIEW) {
                AMP_DISP_WINDOW_CFG_s window;
                AMP_AREA_s WindowArea;

                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

                if (encTvWinHdlr) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;

                    WindowArea.Width = (VideoEncMgt[EncModeIdx].AspectRatio == VAR_16x9)? 1920: 1440;
                    WindowArea.Height = 1080;
                    WindowArea.X = (1920 - WindowArea.Width)/2;
                    WindowArea.Y = 0;
                    window.TargetAreaOnPlane.Width = WindowArea.Width;
                    window.TargetAreaOnPlane.Height = WindowArea.Height;
                    window.TargetAreaOnPlane.X = WindowArea.X;
                    window.TargetAreaOnPlane.Y = WindowArea.Y;
                    window.TargetAreaOnPlane.Width = WindowArea.Width;
                    window.TargetAreaOnPlane.Height = WindowArea.Height;
                    window.TargetAreaOnPlane.X = WindowArea.X;
                    window.TargetAreaOnPlane.Y = WindowArea.Y;

                    AmpDisplay_SetWindowCfg(encTvWinHdlr, &window);
                    if (TvLiveview) {
                        AmpDisplay_SetWindowActivateFlag(encTvWinHdlr, 1);
                    } else {
                        AmpDisplay_SetWindowActivateFlag(encTvWinHdlr, 0);
                    }
                }
                AmpDisplay_Update(TVHdlr);
            }
        #endif
            break;
        case AMP_VIN_EVENT_MAINVIEW_CHANGED_PRIOR:
            {
                //disable ImgSchdlr
            }
            break;
        case AMP_VIN_EVENT_MAINVIEW_CHANGED_POST:
            {
                //enable ImgSchdlr
            }
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
static int AmpUT_VideoEncVinEventCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_VIN_EVENT_FRAME_READY:
            //if (xcnt % 30 == 0)
           //  AmbaPrint("VinEVCB: AMP_VIN_EVENT_FRAME_READY info: %X", info);
           // xcnt++;
            break;
        case AMP_VIN_EVENT_FRAME_DROPPED:
         //   AmbaPrint("VinEVCB: AMP_VIN_EVENT_FRAME_DROPPED info: %X", info);
            break;
        default:
            AmbaPrint("VinEVCB: Unknown %X info: %x", event, info);
           break;
    }
    return 0;
}

/**
 * Generic VideoEnc codec callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_VideoEncCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_START:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_DONE:
            //Exif
            {
                COMPUTE_EXIF_PARAMS_s ExifParam = {0};
                EXIF_INFO_s ExifInfo = {0};

                ExifParam.AeIdx = 0; //TBD
                ExifParam.Mode = IMG_EXIF_PIV;
                Amba_Img_Exif_Compute_AAA_Exif(&ExifParam);

                Amba_Img_Exif_Get_Exif_Info(ExifParam.AeIdx, &ExifInfo);
                AmbaPrint("[AmpUT][PIV Exif]");
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

            // Next raw capture is allowed from now on
            StillRawCaptureRunning = 0;
            break;
        case AMP_ENC_EVENT_BACK_TO_LIVEVIEW:
            break;
        case AMP_ENC_EVENT_BG_PROCESS_DONE:
            AmbaPrintColor(BLUE, "STILL_SCRIPT_PROCESS_DONE");
            break;
        case AMP_ENC_EVENT_LIVEVIEW_RAW_READY:
#ifdef CONFIG_SOC_A12
             if (VideoEnc3AEnable) {
                extern int Amba_Img_VDspRaw_Handler(void *hdlr, UINT32 *pRgbData)  __attribute__((weak));
                AMP_ENC_RAW_INFO_s *ptr = info;
                AMBA_IMG_RAW_INFO_s RawBufInfo = {0};
                AMBA_SENSOR_MODE_ID_u Mode = {0};
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo;

                Mode.Data = (EncodeSystem == 0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;
                AmbaSensor_GetModeInfo(VinChannel, Mode, &SensorModeInfo);

                RawBufInfo.Compressed = ptr->compressed;
                RawBufInfo.pBaseAddr = ptr->RawAddr;
                RawBufInfo.Pitch = ptr->RawPitch;
                RawBufInfo.BayerPattern = SensorModeInfo.OutputInfo.CfaPattern;
                RawBufInfo.NumDataBits = SensorModeInfo.OutputInfo.NumDataBits;
                RawBufInfo.Window.Width = ptr->RawWidth;
                RawBufInfo.Window.Height = ptr->RawHeight;

                RawBufInfo.OBWindow.Width = ptr->OBWidth;
                RawBufInfo.OBWindow.Height = ptr->OBHeight;
                RawBufInfo.OBWindow.OffsetX = ptr->OBOffsetX;
                RawBufInfo.OBWindow.OffsetY = ptr->OBOffsetY;
                Amba_Img_VDspRaw_Handler(hdlr, (UINT32 *)&RawBufInfo);
            }
#endif
            break;
        case AMP_ENC_EVENT_LIVEVIEW_DCHAN_YUV_READY:
            {
              //AMP_ENC_YUV_INFO_s *ptr = info;
              //AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
              //AmbaPrint("EncCB: AMP_VIDEOENC_MSG_LIVEVIEW_DCHAN_YUV_READY addr: %X p:%d %dx%d", inf.yAddr, inf.pitch, inf.Width, inf.Height);
            }
            break;
        case AMP_ENC_EVENT_LIVEVIEW_FCHAN_YUV_READY:
            {
          //   AMP_ENC_YUV_INFO_s *ptr = info;
          //   AMP_ENC_YUV_INFO_s inf = *ptr;   // must copy to local. caller won't keep it after function exit
          //   AmbaPrint("EncCB: AMP_VIDEOENC_MSG_LIVEVIEW_FCHAN_YUV_READY info: %X", info);
            }
            break;
        case AMP_ENC_EVENT_VCAP_YUV_READY:
            //AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VCAP_2ND_YUV_READY:
#ifdef CONFIG_SOC_A12
            //AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_2ND_YUV_READY !!!!!!!!!!");
            {
                extern int AmbaVA_FrmHdlr_NewFrame(UINT32 event, AMP_ENC_YUV_INFO_s* YUVInfo) __attribute__((weak));
                if (AmbaVA_FrmHdlr_NewFrame) {
                    AMP_ENC_YUV_INFO_s* pinfo = info;
                    AMP_ENC_YUV_INFO_s tmpinfo[1];
                    tmpinfo[0].colorFmt= pinfo->colorFmt;
                    tmpinfo[0].height  = pinfo->height;
                    tmpinfo[0].pitch   = pinfo->pitch;
                    tmpinfo[0].width   = pinfo->width;
                    tmpinfo[0].yAddr   = pinfo->yAddr;
                    tmpinfo[0].uvAddr  = pinfo->uvAddr;
                    tmpinfo[0].ySize   = pinfo->ySize;
                    //AmbaPrintColor(RED, "...!!!!!!!!!! tmpinfo->height = %d ;  tmpinfo->width = %d !!!!!!!!!!", tmpinfo[0].height, tmpinfo[0].width);
                    AmbaVA_FrmHdlr_NewFrame(AMP_ENC_EVENT_VCAP_2ND_YUV_READY, tmpinfo);
                }
            }
#endif
            break;
        case AMP_ENC_EVENT_VCAP_ME1_Y_READY:
            //AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_ME1_Y_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_HYBRID_YUV_READY:
            //AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_HYBRID_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VDSP_ASSERT:
            if (VideoEncDspAssert == 0) {
                extern void AmbaVinInfoParse(AMBA_VIN_CHANNEL_e VinChanNo);

                VideoEncDspAssert = 1;
                AmbaPrintColor(RED, "AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_VDSP_ASSERT !!!!!!!!!!");

                //dump dsp message
                if (DspMsgFile == NULL) {
                    char Fn[80];
                    char mdASCII[3] = {'w','+','\0'};

                    sprintf(Fn, "%s:\\dsp.log", DefaultSlot);

                    DspMsgFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII, 0);
                    while (DspMsgFile == 0) {
                        AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                        AmbaKAL_TaskSleep(10);
                        DspMsgFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII, 0);
                    }
                    AmbaPrint("%s opened", Fn);
                    DoDumpDspMsg((void *)DspMsgFile);
                    UT_VideoEncodefclose((void *)DspMsgFile);
                    AmbaPrint("%s closed", Fn);
                    DspMsgFile = NULL;
                }
                AmbaKAL_TaskSleep(2000);

                //dump vin information, this UT only has MainVin
                AmbaVinInfoParse(AMBA_VIN_CHANNEL0);
            }
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            AmpUT_VideoEnc_EncStop();
            EncDateTimeStampPri = EncDateTimeStampSec = 0;
            Status = STATUS_LIVEVIEW;
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DESC_OVERRUN:
            AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_DESC_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY:
            if (0) {
                UINT32 *Percetage = (UINT32 *) info;
                AmbaPrint("AmpUT_VideoEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY, Percetage=%d!!!!!!!!!!",Percetage);
            }
            break;
        default:
            AmbaPrint("AmpUT_VideoEnc: Unknown %X info: %x", event, info);
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
static int AmpUT_VideoEncPipeCallback(void *hdlr,UINT32 event, void *info)
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
static int AmpUT_VideoEnc_DisplayStart(void)
{
    AMP_DISP_WINDOW_CFG_s window;
    AMP_AREA_s WindowArea;
#ifdef CONFIG_SOC_A12
    UINT16 FinalAspectRatio;
#endif

    /** Step 1: Display config & window config */
//moves to AmpUT_VideoEncInit()
//    if (AmpUT_Display_Init() == NG) {
//        return NG;
//    }

    // Creat LCD Window
    if (1) {
        AmpDisplay_GetDefaultWindowCfg(&window);
        window.Source = AMP_DISP_ENC;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;

        memset(&WindowArea, 0x0, sizeof(AMP_AREA_s));
        WindowArea.Width = 960;
        WindowArea.Height = (VideoEncMgt[EncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
        WindowArea.X = 0;
        WindowArea.Y = (480-WindowArea.Height)/2;
    #ifdef CONFIG_SOC_A12
        if (VENC_IS_ROTATE(EncRotation) && LiveViewProcMode == 0) {
            FinalAspectRatio = GET_VAR_X(VideoEncMgt[EncModeIdx].AspectRatio) |
                (GET_VAR_Y(VideoEncMgt[EncModeIdx].AspectRatio)<<8);
        } else {
            FinalAspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
        }
        AmpUT_VideoEncVoutCalculate(&WindowArea, FinalAspectRatio, AMP_DISP_CHANNEL_DCHAN);
    #endif

        window.TargetAreaOnPlane.Width = WindowArea.Width;
        window.TargetAreaOnPlane.Height = WindowArea.Height;
        window.TargetAreaOnPlane.X = WindowArea.X;
        window.TargetAreaOnPlane.Y = WindowArea.Y;
        window.SourceDesc.Enc.VinCh = VinChannel;
        if (AmpUT_Display_Window_Create(0, &window) == NG) {
            return NG;
        }
    }

    // Creat TV Window
    if (1) {
        AmpDisplay_GetDefaultWindowCfg(&window);
        window.Source = AMP_DISP_ENC;
        window.CropArea.Width = 0;
        window.CropArea.Height = 0;
        window.CropArea.X = 0;
        window.CropArea.Y = 0;

        memset(&WindowArea, 0x0, sizeof(AMP_AREA_s));
        WindowArea.Width = 1920;
        WindowArea.Height = 1080;
        WindowArea.X = 0;
        WindowArea.Y = 0;
    #ifdef CONFIG_SOC_A12
        if (VENC_IS_ROTATE(EncRotation) && LiveViewProcMode == 0) {
            FinalAspectRatio = GET_VAR_X(VideoEncMgt[EncModeIdx].AspectRatio) |
                (GET_VAR_Y(VideoEncMgt[EncModeIdx].AspectRatio)<<8);
        } else {
            FinalAspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
        }
        AmpUT_VideoEncVoutCalculate(&WindowArea, FinalAspectRatio, AMP_DISP_CHANNEL_FCHAN);
    #endif

        window.TargetAreaOnPlane.Width = WindowArea.Width;
        window.TargetAreaOnPlane.Height = WindowArea.Height;
        window.TargetAreaOnPlane.X = WindowArea.X;
        window.TargetAreaOnPlane.Y = WindowArea.Y;
        window.SourceDesc.Enc.VinCh = VinChannel;
        if (AmpUT_Display_Window_Create(1, &window) == NG) {
            return NG;
        }
    }

    if (1) {
        /** Step 2: Setup device */
        // Setup LCD & TV
        if (LCDLiveview) {
            AmpUT_Display_Start(0);
        } else {
            AmpUT_Display_Stop(0);
        }
        if (TvLiveview) {
            AmpUT_Display_Start(1);
        } else {
            AmpUT_Display_Stop(1);
        }
    }

    // Active Window 7002 cmd
    if (LCDLiveview) {
        AmpUT_Display_Act_Window(0);
    } else {
        AmpUT_Display_DeAct_Window(0);
    }
    if (TvLiveview) {
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
int Dzoom_Step_Factor_Table(int Step, UINT32 *Factor)
{
    int i, Type1CrossLimit;
    UINT32 Width, TmpZF;
    UINT64 U64TmpZF;
    if (Step < 413) {
        /** Use type 2, detail in low X */
        Width = ZF_1X;
        Width -= (127)*Step;   /** 127 = (65536/512) - 1 */

        U64TmpZF = 0x100000000 / Width; /** ZF_1X << 16 */
        TmpZF = U64TmpZF;
    } else {
        /** USe type 1, detail in high X */
        U64TmpZF = ZF_1X;
        Type1CrossLimit = Step - 244;
        for (i = 1; i < Type1CrossLimit; i++) {
            U64TmpZF *= 66165; /** 66165 = (120 ^ (1/512)) << 16 */
            U64TmpZF = U64TmpZF >> 16;
        }
        TmpZF = U64TmpZF;
    }
    // Get the dzoom factor under corresponding Status
    *Factor = TmpZF;
    return 0;
}

int AmpUT_VideoEnc_CfaOut_Callback(AMBA_DSP_IMG_WARP_CALC_CFA_INFO_s *pCfaInfo)
{
    /* Read Only */
    AmbaPrint("Input window width       : %d", pCfaInfo->pInputWinDim->Width);
    AmbaPrint("Input window height      : %d", pCfaInfo->pInputWinDim->Height);
    AmbaPrint("Main window height       : %d", pCfaInfo->pMainWinDim->Width);
    AmbaPrint("Main window height       : %d", pCfaInfo->pMainWinDim->Height);

    pCfaInfo->pOutCfaWinDim->Width  = 1920; //TBD
    pCfaInfo->pOutCfaWinDim->Height = 1080; //TBD

    AmbaPrint("Cfa output window width  : %d", pCfaInfo->pOutCfaWinDim->Width);
    AmbaPrint("Cfa output window height : %d", pCfaInfo->pOutCfaWinDim->Height);

    return OK;
}

/**
 *  Unit Test Initialization
 */
int AmpUT_VideoEnc_Init(int sensorID, int LcdID)
{
    int Er;
    void *TmpbufRaw = NULL;
    {
        extern int AmbaLoadDSPuCode(void);
        AmbaLoadDSPuCode(); /* Load uCodes from NAND */
    }

    //imgproc initialization

    // Register LCD driver
//    if (LcdID >= 0) {
//        if (LcdID == 0) {
//    #ifdef CONFIG_LCD_WDF9648W
//        AmbaPrint("Hook Wdf9648w LCD");
//        AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_WdF9648wObj);
//    #else
//        AmbaPrint("Please enable Wdf9648w LCD");
//    #endif
//        }
//    }

    // Register sensor driver
    InputDeviceId = sensorID;
    if (sensorID >= 0) {
        memset(&VinChannel, 0x0, sizeof(AMBA_DSP_CHANNEL_ID_u));
        VinChannel.Bits.VinID = 0;
        VinChannel.Bits.SensorID = 0x1;
        VideoEncMgt = RegisterMWUT_Sensor_Driver(VinChannel, sensorID);
        {
        #ifdef CONFIG_SOC_A12 // Temporary disable for A9
            extern int App_Image_Init(UINT32 ChCount, int sensorID);
            AMBA_SENSOR_DEVICE_INFO_s SensorDeviceInfo = {0};
            AmbaSensor_GetDeviceInfo(VinChannel, &SensorDeviceInfo);
            if (SensorDeviceInfo.HdrIsSupport == 1) {
                App_Image_Init(2, sensorID);
            } else {
                App_Image_Init(1, sensorID);
            }
        #endif
        }
    }

    // Create semaphores for muxers
    if (AmbaKAL_SemCreate(&VideoEncPriSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Semaphore creation failed");
    }
    if (AmbaKAL_SemCreate(&VideoEncSecSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Semaphore creation failed");
    }

    // Prepare stacks for muxer tasks
    Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoEncPriMuxStack, (void **)&TmpbufRaw, VIDEO_ENC_PRI_MUX_TASK_STACK_SIZE, 32);
    if (Er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoEncSecMuxStack, (void **)&TmpbufRaw, VIDEO_ENC_SEC_MUX_TASK_STACK_SIZE, 32);
    if (Er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }

    // Create muxer tasks
    if (AmbaKAL_TaskCreate(&VideoEncPriMuxTask, "Video Encoder UnitTest Primary Muxing Task", 50, \
         AmpUT_VideoEnc_PriMuxTask, 0x0, VideoEncPriMuxStack, VIDEO_ENC_PRI_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoEnc UnitTest: Muxer task creation failed");
    }
    if (AmbaKAL_TaskCreate(&VideoEncSecMuxTask, "Video Encoder UnitTest Secondary stream Muxing Task", 50, \
         AmpUT_VideoEnc_SecMuxTask, 0x0, VideoEncSecMuxStack, VIDEO_ENC_SEC_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoEnc UnitTest: Muxer task creation failed");
    }

    // Initialize VIN module
    {
        AMP_VIN_INIT_CFG_s vinInitCfg;

        AmpVin_GetInitDefaultCfg(&vinInitCfg);

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VinWorkBuffer, (void **)&TmpbufRaw, vinInitCfg.MemoryPoolSize, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for vin!!");
        }
        vinInitCfg.MemoryPoolAddr = VinWorkBuffer;
        AmpVin_Init(&vinInitCfg);
    }

    // Initialize VIDEOENC module
    {
        AMP_VIDEOENC_INIT_CFG_s encInitCfg;

        AmpVideoEnc_GetInitDefaultCfg(&encInitCfg);

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VEncWorkBuffer, (void **)&TmpbufRaw, encInitCfg.MemoryPoolSize, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for enc!!");
        }
        encInitCfg.MemoryPoolAddr = VEncWorkBuffer;
        AmpVideoEnc_Init(&encInitCfg);
    }

    // Initialize Image scheduler module
    {
        AMBA_IMG_SCHDLR_INIT_CFG_s ISInitCfg;
        UINT32 ISPoolSize = 0;
        UINT32 mainViewNum = 1;  // One MainViewID
        AMBA_DSP_IMG_PIPE_e Pipe = AMBA_DSP_IMG_PIPE_VIDEO;

        AmbaImgSchdlr_GetInitDefaultCfg(&ISInitCfg);
        ISInitCfg.MainViewNum = mainViewNum;
        AmbaImgSchdlr_QueryMemsize(mainViewNum, &ISPoolSize);
#ifdef CONFIG_SOC_A9
        ISInitCfg.MemoryPoolSize = ISPoolSize;
#else
        ISInitCfg.MemoryPoolSize = ISPoolSize + ISInitCfg.MsgTaskStackSize;
#endif
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&ImgSchdlrWorkBuffer, (void **)&TmpbufRaw, ISInitCfg.MemoryPoolSize, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for imgschdlr!!");
        }
        ISInitCfg.IsoCfgNum = AmpResource_GetIKIsoConfigNumber(Pipe);
        ISInitCfg.MemoryPoolAddr = ImgSchdlrWorkBuffer;
        Er = AmbaImgSchdlr_Init(&ISInitCfg);
        if (Er != AMP_OK) {
            AmbaPrint("AmbaImgSchdlr_Init Fail!");
        }
    }

#ifdef CONFIG_SOC_A12
    // Initialize EncMonitor Cyclic module
    {
        UINT32 MemSize = 0;
        AMBA_IMG_ENC_MONITOR_MEMORY_s MonitorCyclicQueryCfg;
        AMBA_IMG_ENC_MONITOR_INIT_CFG_s MonitorCyclicCfg;
        memset(&MonitorCyclicQueryCfg, 0x0, sizeof(AMBA_IMG_ENC_MONITOR_MEMORY_s));
        memset(&MonitorCyclicCfg, 0x0, sizeof(AMBA_IMG_ENC_MONITOR_INIT_CFG_s));

        AmbaEncMonitor_GetInitDefaultCfg(&MonitorCyclicCfg);
        MonitorCyclicQueryCfg.MaxTimerMonitorNumber = MonitorCyclicCfg.MaxTimerMonitorNumber;
        MonitorCyclicQueryCfg.MaxVdspMonitorNumber = MonitorCyclicCfg.MaxVdspMonitorNumber;
        MonitorCyclicQueryCfg.TimerMonitorTaskStackSize = MonitorCyclicCfg.TimerMonitorTaskStackSize;
        MonitorCyclicQueryCfg.VdspMonitorTaskStackSize = MonitorCyclicCfg.VdspMonitorTaskStackSize;
        AmbaEncMonitor_QueryMemsize(&MonitorCyclicQueryCfg, &MemSize);
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorCyclicWorkBuffer, (void **)&TmpbufRaw, MemSize, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for encMonitorCyclic!!");
        }
        MonitorCyclicCfg.MemoryPoolSize = MemSize;
        MonitorCyclicCfg.MemoryPoolAddr = EncMonitorCyclicWorkBuffer;
        Er = AmbaEncMonitor_Init(&MonitorCyclicCfg);
        if (Er != AMP_OK) {
            AmbaPrint("AmbaEncMonitor_Init Fail!");
        }
    }

    // Initialize EncMonitor BitRate Service module
    {
        UINT32 MemSize = 0;
        AMBA_IMG_ENC_MONITOR_BRC_MEMORY_s MonitorBRateQueryCfg;
        AMBA_IMG_ENC_MONITOR_BITRATE_INIT_CFG_s MonitorBRateCfg;
        memset(&MonitorBRateQueryCfg, 0x0, sizeof(AMBA_IMG_ENC_MONITOR_BRC_MEMORY_s));
        memset(&MonitorBRateCfg, 0x0, sizeof(AMBA_IMG_ENC_MONITOR_BITRATE_INIT_CFG_s));

        MonitorBRateCfg.MaxStreamNumber = MonitorBRateQueryCfg.MaxStreamNumber = 2; // dual stream
        AmbaEncMonitorBRC_QueryMemsize(&MonitorBRateQueryCfg, &MemSize);

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorServiceWorkBuffer, (void **)&TmpbufRaw, MemSize, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for encMonitorService!!");
        }
        MonitorBRateCfg.MemoryPoolSize = MemSize;
        MonitorBRateCfg.MemoryPoolAddr = EncMonitorServiceWorkBuffer;
        Er = AmbaEncMonitorBRC_init(&MonitorBRateCfg);
        if (Er != AMP_OK) {
            AmbaPrint("AmbaEncMonitorBRC_init Fail!");
        }
    }

    // Initialize EncMonitor AQP Service module
    {
        UINT32 MemSize = 0;
        AMBA_IMG_ENC_MONITOR_AQP_MEMORY_s MonitorAqpQueryCfg;
        AMBA_IMG_ENC_MONITOR_AQP_INIT_CFG_s MonitorAqpCfg;
        memset(&MonitorAqpQueryCfg, 0x0, sizeof(AMBA_IMG_ENC_MONITOR_AQP_MEMORY_s));
        memset(&MonitorAqpCfg, 0x0, sizeof(AMBA_IMG_ENC_MONITOR_AQP_INIT_CFG_s));

        MonitorAqpCfg.MaxStreamNumber = MonitorAqpQueryCfg.MaxStreamNumber = 2; // dual stream
        AmbaEncMonitorAQP_QueryMemsize(&MonitorAqpQueryCfg, &MemSize);

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorServiceAqpWorkBuffer, (void **)&TmpbufRaw, MemSize + 32, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for encMonitorAqpService!!");
        }
        EncMonitorServiceAqpWorkBuffer = (UINT8 *)ALIGN_32((UINT32)EncMonitorServiceAqpWorkBuffer);
        MonitorAqpCfg.MemoryPoolSize = MemSize;
        MonitorAqpCfg.MemoryPoolAddr = EncMonitorServiceAqpWorkBuffer;
        Er = AmbaEncMonitorAQP_init(&MonitorAqpCfg);
        if (Er != AMP_OK) {
            AmbaPrint("AmbaEncMonitorAQP_init Fail!");
        }
    }

    // Initialize EncMonitor Stream module
    {
        UINT32 MemSize = 0;
        AMBA_IMG_ENC_MONITOR_STRM_MEMORY_s MonitorStrmQueryCfg;
        AMBA_IMG_ENC_MONITOR_STRM_INIT_CFG_s MonitorStrmCfg;
        memset(&MonitorStrmQueryCfg, 0x0, sizeof(AMBA_IMG_ENC_MONITOR_STRM_MEMORY_s));
        memset(&MonitorStrmCfg, 0x0, sizeof(AMBA_IMG_ENC_MONITOR_STRM_INIT_CFG_s));

        MonitorStrmCfg.MaxStreamNumber = MonitorStrmQueryCfg.MaxStreamNumber = 2; // dual stream
        AmbaEncMonitorStream_QueryMemsize(&MonitorStrmQueryCfg, &MemSize);

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorStrmWorkBuffer, (void **)&TmpbufRaw, MemSize, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for encMonitorStream!!");
        }
        MonitorStrmCfg.MemoryPoolSize = MemSize;
        MonitorStrmCfg.MemoryPoolAddr = EncMonitorStrmWorkBuffer;
        Er = AmbaEncMonitorStream_Init(&MonitorStrmCfg);
        if (Er != AMP_OK) {
            AmbaPrint("AmbaEncMonitorStream_Init Fail!");
        }
    }
#endif

    // Allocate bitstream buffers
    {
#ifdef CONFIG_SOC_A9
        extern UINT8 *DspWorkAreaResvLimit;
#endif

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&H264BitsBuf, (void **)&TmpbufRaw, BITSFIFO_SIZE, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&H264DescBuf, (void **)&TmpbufRaw, DESC_SIZE, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }

#ifdef CONFIG_SOC_A12
        MjpgDescBuf=H264DescBuf;
        MjpgBitsBuf=H264BitsBuf;
#else
        // This is an example how to use DSP working memory when APP knows these memory area is not used.
        // We steal 15MB here
        MjpgDescBuf = DspWorkAreaResvLimit + 1 - 1*1024*1024;
        MjpgBitsBuf = MjpgDescBuf - BITSFIFO_SIZE;
#endif

    }

    // Initialize Dzoom module
    {
        AMP_IMG_DZOOM_INIT_CFG_s dzoomInitCfg;
        AmpImgDzoom_GetInitDefaultCfg(&dzoomInitCfg);
        AmpImgDzoom_Init(&dzoomInitCfg);
    }

    // Initialize blending buffers
    {
        int j, k;

        memset(EncBlendY, 255, 256*100);
        memset(EncBlendAlphaY, 0, 256*100); // 0 means 100% use OSD layer
    #ifdef CONFIG_SOC_A9
        memset(EncBlendUV, 128, 256*100/2);
        memset(EncBlendAlphaUV, 0, 256*100/2);
    #else
        memset(EncBlendUV, 128, 256*100);
        memset(EncBlendAlphaUV, 0, 256*100); // 0 means 100% use OSD layer
    #endif

        for(j = 20; j < 80; j++) {
            for(k = 0; k < 256; k++) {
                if (k < 20)
                    EncBlendAlphaY[k+j*256] = 0;
                else if (k < 240)
                    EncBlendAlphaY[k+j*256] = 0xFF;
                else
                    EncBlendAlphaY[k+j*256] = 0;
            }
        }

    #ifdef CONFIG_SOC_A9
        for(j = 10; j < 40; j++) {
            for(k = 0; k < 256; k++) {
                if (k < 20)
                    EncBlendAlphaUV[k+j*256] = 0;
                else if (k < 240)
                    EncBlendAlphaUV[k+j*256] = 0xFF;
                else
                    EncBlendAlphaUV[k+j*256] = 0;
            }
        }
    #else
        for(j = 20; j < 80; j++) {
            for(k = 0; k < 256; k++) {
                if (k < 20)
                    EncBlendAlphaUV[k+j*256] = 0;
                else if (k < 240)
                    EncBlendAlphaUV[k+j*256] = 0xFF;
                else
                    EncBlendAlphaUV[k+j*256] = 0;
            }
        }
    #endif

    }

    {   //init display
        if (AmpUT_Display_Init() == NG) {
            return NG;
        }
    }

#ifdef CONFIG_SOC_A12
    if (VideoEnc_EisAlgoEnable){
        AmbaEisTask_Init(12, 0);
    }
#endif
    return 0;
}

/**
 * Liveview start
 *
 * @param [in]
 *
 */
int AmpUT_VideoEnc_LiveviewStart(UINT32 modeIdx)
{
    AMBA_SENSOR_MODE_INFO_s SensorModeInfo;

    EncModeIdx = modeIdx;

//#define KEEP_ALL_INSTANCES    // When defined, LiveviewStop won't delete all instances, so the next LiveviewStart is equal to change resolution.
#ifdef KEEP_ALL_INSTANCES
    if (VideoEncVinA) {
        AmpUT_VideoEnc_ChangeResolution(modeIdx);
        AmpEnc_StartLiveview(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
        return 0;
    }
#endif

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Liveview at %s", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
    AmbaPrint(" =========================================================");

    // Force Liveview Process Mode
    {
        if (ExtLiveViewProcMode != 0xFF) {
            LiveViewProcMode = ExtLiveViewProcMode;
        #ifdef CONFIG_SOC_A12
            if (VideoEnc_EisAlgoEnable == 0) ExtLiveViewProcMode = 0xFF;
        #else
            ExtLiveViewProcMode = 0xFF;
        #endif
        }

        if (ExtLiveViewAlgoMode != 0xFF) {
            LiveViewAlgoMode = ExtLiveViewAlgoMode;
        #ifdef CONFIG_SOC_A12
            if (VideoEnc_EisAlgoEnable == 0) ExtLiveViewAlgoMode = 0xFF;
        #else
            ExtLiveViewAlgoMode = 0xFF;
        #endif
        }

        if (ExtLiveViewOSMode != 0xFF) {
            LiveViewOSMode = ExtLiveViewOSMode;
        #ifdef CONFIG_SOC_A12
            if (VideoEnc_EisAlgoEnable == 0) ExtLiveViewOSMode = 0xFF;
        #else
            ExtLiveViewOSMode = 0xFF;
        #endif
        } else {
            LiveViewOSMode = VideoEncMgt[EncModeIdx].OSMode;
        }
    }

    // Create Vin instance
    if (VideoEncVinA == 0) {
        AMP_VIN_HDLR_CFG_s VinCfg;
        AMP_VIN_LAYOUT_CFG_s Layout[2]; // Dualstream from same vin/vcapwindow
        AMBA_SENSOR_MODE_ID_u Mode = {0};

        memset(&VinCfg, 0x0, sizeof(AMP_VIN_HDLR_CFG_s));
        memset(&Layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s)*2);

        Mode.Data = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;
        memset(&SensorModeInfo, 0x0, sizeof(AMBA_SENSOR_MODE_INFO_s));
        AmbaSensor_GetModeInfo(VinChannel, Mode, &SensorModeInfo);

        AmpVin_GetDefaultCfg(&VinCfg);

#define CONFIG_WHEN_CREATE  // When defined, setup everything when creation. If not defined, configurations can be set after creation and before liveviewStart
#ifdef CONFIG_WHEN_CREATE
        VinCfg.Channel = VinChannel;
        VinCfg.Mode = Mode;
        VinCfg.LayoutNumber = 2;
#ifdef CONFIG_SOC_A9
        VinCfg.HwCaptureWindow.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
        VinCfg.HwCaptureWindow.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
        VinCfg.HwCaptureWindow.X = SensorModeInfo.OutputInfo.RecordingPixels.StartX +
            (((SensorModeInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        VinCfg.HwCaptureWindow.Y = (SensorModeInfo.OutputInfo.RecordingPixels.StartY +
            ((SensorModeInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;
#else
        if (VideoOBModeEnable && SensorModeInfo.OutputInfo.OpticalBlackPixels.Width && SensorModeInfo.OutputInfo.OpticalBlackPixels.Height) {
            INT16 ActStartX = MIN2(SensorModeInfo.OutputInfo.OpticalBlackPixels.StartX,SensorModeInfo.OutputInfo.RecordingPixels.StartX);
            INT16 ActStartY = MIN2(SensorModeInfo.OutputInfo.OpticalBlackPixels.StartY,SensorModeInfo.OutputInfo.RecordingPixels.StartY);

            VinCfg.HwCaptureWindow.Width = SensorModeInfo.OutputInfo.OutputWidth - ActStartX;
            VinCfg.HwCaptureWindow.Height = SensorModeInfo.OutputInfo.OutputHeight - ActStartY;
            VinCfg.HwCaptureWindow.X = ActStartX;
            VinCfg.HwCaptureWindow.Y = ActStartY;
            Layout[0].ActiveArea.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
            Layout[0].ActiveArea.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
            Layout[0].ActiveArea.X = (SensorModeInfo.OutputInfo.RecordingPixels.StartX - ActStartX) + \
                (((SensorModeInfo.OutputInfo.RecordingPixels.Width - Layout[0].ActiveArea.Width)/2)&0xFFF8);
            Layout[0].ActiveArea.Y = (SensorModeInfo.OutputInfo.RecordingPixels.StartY - ActStartY) + \
                (((SensorModeInfo.OutputInfo.RecordingPixels.Height - Layout[0].ActiveArea.Height)/2)&0xFFFE);
            Layout[0].OBArea.Width = SensorModeInfo.OutputInfo.OpticalBlackPixels.Width;
            Layout[0].OBArea.Height = SensorModeInfo.OutputInfo.OpticalBlackPixels.Height;
            Layout[0].OBArea.X = SensorModeInfo.OutputInfo.OpticalBlackPixels.StartX - ActStartX;
            Layout[0].OBArea.Y = SensorModeInfo.OutputInfo.OpticalBlackPixels.StartY - ActStartY;
            Layout[0].EnableOBArea = 1;
        } else {
            VinCfg.HwCaptureWindow.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
            VinCfg.HwCaptureWindow.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
            VinCfg.HwCaptureWindow.X = SensorModeInfo.OutputInfo.RecordingPixels.StartX + \
                (((SensorModeInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
            VinCfg.HwCaptureWindow.Y = SensorModeInfo.OutputInfo.RecordingPixels.StartY + \
                (((SensorModeInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)&0xFFFE);
            Layout[0].ActiveArea.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
            Layout[0].ActiveArea.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
            Layout[0].ActiveArea.X = VinCfg.HwCaptureWindow.X;
            Layout[0].ActiveArea.Y = VinCfg.HwCaptureWindow.Y;
            Layout[0].EnableOBArea = 0;
        }
#endif
        Layout[0].Width = VideoEncMgt[EncModeIdx].MainWidth;
        Layout[0].Height = VideoEncMgt[EncModeIdx].MainHeight;
        Layout[0].EnableSourceArea = 0; // Get all capture window to main
        Layout[0].DzoomFactorX = InitZoomX; // 16.16 format
        Layout[0].DzoomFactorY = InitZoomY;
        Layout[0].DzoomOffsetX = 0;
        Layout[0].DzoomOffsetY = 0;
        Layout[0].MainviewReportRate = (MainviewReportRate)? MainviewReportRate: VideoEncMgt[EncModeIdx].ReportRate;
        if (EncDualHDStream) {
            if (SecStreamCustomWidth) {
                Layout[1].Width = SecStreamCustomWidth;
                Layout[1].Height = SecStreamCustomHeight;
            } else {
                MWUT_InputSetDualHDWindow(VideoEncMgt, EncModeIdx, \
                    &Layout[1].Width, &Layout[1].Height);
            }
        } else {
            if (SecStreamCustomWidth) {
                Layout[1].Width = SecStreamCustomWidth;
                Layout[1].Height = SecStreamCustomHeight;
            } else {
                Layout[1].Width = SEC_STREAM_WIDTH;
                Layout[1].Height = SEC_STREAM_HEIGHT;
            }
        }
        Layout[1].EnableSourceArea = 0; // Get all capture window to main
        Layout[1].DzoomFactorX = InitZoomX;
        Layout[1].DzoomFactorY = InitZoomY;
        Layout[1].DzoomOffsetX = 0;
        Layout[1].DzoomOffsetY = 0;
        Layout[1].MainviewReportRate = 1;
        VinCfg.Layout = Layout;
#endif
        VinCfg.cbEvent = AmpUT_VideoEncVinEventCallback;
        VinCfg.cbSwitch= AmpUT_VideoEncVinSwitchCallback;

        AmpVin_Create(&VinCfg, &VideoEncVinA);
    }

    // Remember frame/field rate for muxers storing frame rate info
    if (EncodeSystem == 0) {
        EncFrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;
    } else {
        EncFrameRate = VideoEncMgt[EncModeIdx].TimeScalePAL/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
    }

    // Create ImgSchdlr instance
    {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg;

        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

        ImgSchdlrCfg.MainViewID = 0; //single channle have one MainView
        ImgSchdlrCfg.Channel = VinChannel;
        ImgSchdlrCfg.Vin = VideoEncVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_VideoEncImgSchdlrCallback;
        if (LiveViewProcMode && LiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
            if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                ImgSchdlrCfg.VideoProcMode |= 0x10;
            }
        }
        ImgSchdlrCfg.AAAStatSampleRate = (MainviewReportRate)? MainviewReportRate: VideoEncMgt[EncModeIdx].ReportRate;
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &ImgSchdlr);  // One MainViewID (not vin) need one scheduler.
    }

#ifdef CONFIG_SOC_A12
    // Create encMonitor Stream instance
    {
        AMBA_DSP_EVENT_VIDEO_ENC_STATUS_s Stream = {0};

        Stream.StreamId = 0; //Pri
        Stream.ChannelId = 0; //TBD
        AmbaEncMonitor_StreamRegister(Stream, &EncMonitorStrmHdlrPri);

        if (EncDualStream || EncDualHDStream) {
            Stream.StreamId = 1; //Sec
            Stream.ChannelId = 0; //TBD
            AmbaEncMonitor_StreamRegister(Stream, &EncMonitorStrmHdlrSec);
        }
    }

    // Create Brc instance
    if (EncMonitorEnable) {
        AMBA_IMG_ENC_MONITOR_BRC_HDLR_CFG_s BrcCfg = {0};
        AmbaEncMonitorBRC_GetDefaultCfg(&BrcCfg);
        if (BrcHdlrPri == NULL) {
            BrcCfg.Period = 3000; // 3sec
            BrcCfg.CmplxHdlr.GetDayLumaThresCB = AmpUT_VideoEnc_GetDayLumaThresholdCB;
            BrcCfg.CmplxHdlr.GetComplexityRangeCB = AmpUT_VideoEnc_GetSceneComplexityRangeCB;
            BrcCfg.CmplxHdlr.AdjustQpCB = AmpUT_VideoEnc_QpAdjustmentCB;
            BrcCfg.AverageBitrate = (UINT32)VideoEncMgt[EncModeIdx].AverageBitRate*1000*1000;
            BrcCfg.MaxBitrate = (UINT32)VideoEncMgt[EncModeIdx].MaxBitRate*1000*1000;
            BrcCfg.MinBitrate = (UINT32)VideoEncMgt[EncModeIdx].MinBitRate*1000*1000;
            BrcCfg.emonStrmHdlr = EncMonitorStrmHdlrPri;
            BrcCfg.VideoOSMode = LiveViewOSMode;
            BrcCfg.VideoProcMode = LiveViewProcMode;
            BrcCfg.VideoHdrMode = (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE)? 1: 0;
            AmbaEncMonitorBRC_RegisterService(&BrcCfg, &BrcHdlrPri);
        }

        if (EncSecSpecH264 && (EncDualStream || EncDualHDStream) && BrcHdlrSec == NULL) {
            if (EncDualHDStream) {
                if (SecStreamCustomWidth) {
                    //TBD, temp follow MainStrm
                    BrcCfg.AverageBitrate = (UINT32)VideoEncMgt[EncModeIdx].AverageBitRate*1000*1000;
                    BrcCfg.MaxBitrate = (UINT32)VideoEncMgt[EncModeIdx].MaxBitRate*1000*1000;
                    BrcCfg.MinBitrate = (UINT32)VideoEncMgt[EncModeIdx].MinBitRate*1000*1000;
                } else if (VideoEncMgt[EncModeIdx].MainWidth < SEC_STREAM_HD_WIDTH) {
                    BrcCfg.AverageBitrate = (UINT32)VideoEncMgt[EncModeIdx].AverageBitRate*1000*1000;
                    BrcCfg.MaxBitrate = (UINT32)VideoEncMgt[EncModeIdx].MaxBitRate*1000*1000;
                    BrcCfg.MinBitrate = (UINT32)VideoEncMgt[EncModeIdx].MinBitRate*1000*1000;
                } else {
                    BrcCfg.AverageBitrate = SEC_STREAM_HD_BRATE_AVG*1000*1000;
                    BrcCfg.MaxBitrate = SEC_STREAM_HD_BRATE_MAX*1000*1000;
                    BrcCfg.MinBitrate = SEC_STREAM_HD_BRATE_MIN*1000*1000;
                }
            } else {
                //Small SecStrm
                BrcCfg.AverageBitrate = (UINT32)SEC_STREAM_BRATE_AVG*1000*1000;
                BrcCfg.MaxBitrate = (UINT32)SEC_STREAM_BRATE_MAX*1000*1000;
                BrcCfg.MinBitrate = (UINT32)SEC_STREAM_BRATE_MIN*1000*1000;
            }
            BrcCfg.emonStrmHdlr = EncMonitorStrmHdlrSec;
            AmbaEncMonitorBRC_RegisterService(&BrcCfg, &BrcHdlrSec);
        }
    }
    if (EncMonitorAQPEnable) {
        AMBA_IMG_ENC_MONITOR_AQP_HDLR_CFG_s AqpCfg;

        memset(&AqpCfg, 0x0, sizeof(AMBA_IMG_ENC_MONITOR_AQP_HDLR_CFG_s));
        AmbaEncMonitorAQP_GetDefaultCfg(&AqpCfg);

        if (AqpHdlrPri == NULL) {
            AqpCfg.Period = 1000; // 1sec
            AqpCfg.emonStrmHdlr = EncMonitorStrmHdlrPri;
            AqpCfg.AqpCB = AmpUT_VideoEnc_AqpPriStream;
            AmbaEncMonitorAQP_RegisterService(&AqpCfg, &AqpHdlrPri);
        }

        if ((EncDualStream || EncDualHDStream) && AqpHdlrSec == NULL) {
            AqpCfg.AqpCB = AmpUT_VideoEnc_AqpSecStream;
            AqpCfg.emonStrmHdlr = EncMonitorStrmHdlrSec;
            AmbaEncMonitorAQP_RegisterService(&AqpCfg, &AqpHdlrSec);
        }
    }
#endif

    // Create video encoder instances
    if (VideoEncPri == 0 && VideoEncSec == 0) {
        AMP_VIDEOENC_HDLR_CFG_s EncCfg;
        AMP_VIDEOENC_LAYER_DESC_s EncLayer;
#if 0
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
#endif
        memset(&EncCfg, 0x0, sizeof(AMP_VIDEOENC_HDLR_CFG_s));
        memset(&EncLayer, 0x0, sizeof(AMP_VIDEOENC_LAYER_DESC_s));
        EncCfg.MainLayout.Layer = &EncLayer;
        AmpVideoEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_VideoEncCallback;

#ifdef CONFIG_WHEN_CREATE
        // Assign main layout in single chan
        EncCfg.MainLayout.Width = VideoEncMgt[EncModeIdx].MainWidth;
        EncCfg.MainLayout.Height = VideoEncMgt[EncModeIdx].MainHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = EncPriInteralce; //TBD
        EncCfg.MainTimeScale = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;
        EncCfg.MainTickPerPicture = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TickPerPicture: VideoEncMgt[EncModeIdx].TickPerPicturePAL;
    #ifdef CONFIG_SOC_A9
        EncCfg.SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    #else
        EncCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
        EncCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
        EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
        EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
    #endif
        EncLayer.SourceType = AMP_ENC_SOURCE_VIN;
        EncLayer.Source = VideoEncVinA;
        EncLayer.SourceLayoutId = 0;
        EncLayer.EnableSourceArea = 0;  // No source cropping
        EncLayer.EnableTargetArea = 0;  // No target pip
        EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
        EncCfg.StreamId = AMP_VIDEOENC_STREAM_PRIMARY;

        {
            UINT8 *dspWorkAddr;
            UINT32 dspWorkSize;
            AmpUT_VideoEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
            EncCfg.DspWorkBufAddr = dspWorkAddr;
            EncCfg.DspWorkBufSize = dspWorkSize;
        }
 #endif

        if (LiveViewProcMode == 0) {
            if (VideoEncMgt[EncModeIdx].MainWidth > 1920 || \
                VideoEncMgt[EncModeIdx].CaptureWidth > 1920) {
                if (LiveViewOSMode == 0) {
                    AmbaPrint("[UT_Video]Force to use NonOS Mode?? (VcapW %d,MainW %d)", VideoEncMgt[EncModeIdx].MainWidth, VideoEncMgt[EncModeIdx].CaptureWidth);
                }
            }
            if ((VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture)>60 && \
                LiveViewOSMode == 1) {
                LiveViewOSMode = 0;
                AmbaPrint("High Fr (>60fps) Force to use Basic in the Express mode");
            }
        }

        EncCfg.LiveViewProcMode = LiveViewProcMode;
        EncCfg.LiveViewAlgoMode = LiveViewAlgoMode;
        EncCfg.LiveViewOSMode = LiveViewOSMode;
        if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
            EncCfg.LiveViewHdrMode = LiveViewHdrMode = 1;
        } else {
            EncCfg.LiveViewHdrMode = LiveViewHdrMode = 0;
        }
#ifdef CONFIG_SOC_A12
        EncCfg.LiveviewOBModeEnable = VideoOBModeEnable;
#endif
       // Create primary stream handler
        AmpVideoEnc_Create(&EncCfg, &VideoEncPri); // Don't have to worry about h.264 spec settings when liveview

#ifndef CONFIG_WHEN_CREATE // An example if a user doesn't want to configure in detail when handler creation, but configure only before liveview start
{
        AMP_VIDEOENC_MAIN_CFG_s MainCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s NewPriLayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
        AMP_VIN_RUNTIME_CFG_s VinCfg = {0};
        AMP_VIN_LAYOUT_CFG_s Layout; // Dualstream from same vin/vcapwindow
        AMBA_SENSOR_MODE_ID_u Mode = {0};

        Mode.Data = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;
        memset(&SensorModeInfo, 0x0, sizeof(AMBA_SENSOR_MODE_INFO_s));
        AmbaSensor_GetModeInfo(VinChannel, Mode, &SensorModeInfo);

        VinCfg.Hdlr = VideoEncVinA;
        VinCfg.Mode = Mode;
        VinCfg.LayoutNumber = 1;
        if ( VideoOBModeEnable && SensorModeInfo.OutputInfo.OpticalBlackPixels.Width && SensorModeInfo.OutputInfo.OpticalBlackPixels.Height) {
            INT16 ActStartX = MIN2(SensorModeInfo.OutputInfo.OpticalBlackPixels.StartX,SensorModeInfo.OutputInfo.RecordingPixels.StartX);
            INT16 ActStartY = MIN2(SensorModeInfo.OutputInfo.OpticalBlackPixels.StartY,SensorModeInfo.OutputInfo.RecordingPixels.StartY);

            VinCfg.HwCaptureWindow.Width = SensorModeInfo.OutputInfo.OutputWidth - ActStartX;
            VinCfg.HwCaptureWindow.Height = SensorModeInfo.OutputInfo.OutputHeight - ActStartY;
            VinCfg.HwCaptureWindow.X = ActStartX;
            VinCfg.HwCaptureWindow.Y = ActStartY;
            Layout[0].ActiveArea.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
            Layout[0].ActiveArea.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
            Layout[0].ActiveArea.X = SensorModeInfo.OutputInfo.RecordingPixels.StartX - ActStartX;
            Layout[0].ActiveArea.Y = SensorModeInfo.OutputInfo.RecordingPixels.StartY - ActStartY;
            Layout[0].OBArea.Width = SensorModeInfo.OutputInfo.OpticalBlackPixels.Width;
            Layout[0].OBArea.Height = SensorModeInfo.OutputInfo.OpticalBlackPixels.Height;
            Layout[0].OBArea.X = SensorModeInfo.OutputInfo.OpticalBlackPixels.StartX - ActStartX;
            Layout[0].OBArea.Y = SensorModeInfo.OutputInfo.OpticalBlackPixels.StartY - ActStartY;
            Layout[0].EnableOBArea = 1;
        } else {
            Layout[0].ActiveArea.Width = VinCfg.HwCaptureWindow.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
            Layout[0].ActiveArea.Height = VinCfg.HwCaptureWindow.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
            Layout[0].ActiveArea.X = VinCfg.HwCaptureWindow.X = SensorModeInfo.OutputInfo.RecordingPixels.StartX +
                (((SensorModeInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
            Layout[0].ActiveArea.Y = VinCfg.HwCaptureWindow.Y = (SensorModeInfo.OutputInfo.RecordingPixels.StartY +
                ((SensorModeInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;
            Layout[0].EnableOBArea = 0;
        }

        Layout.Width = VideoEncMgt[EncModeIdx].MainWidth;
        Layout.Height = VideoEncMgt[EncModeIdx].MainHeight;
        Layout.EnableSourceArea = 0; // Get all capture window to main
        Layout.DzoomFactorX = InitZoomX;
        Layout.DzoomFactorY = InitZoomY;
        Layout.DzoomOffsetX = 0;
        Layout.DzoomOffsetY = 0;
        Layout.MainviewReportRate = (MainviewReportRate)? MainviewReportRate: VideoEncMgt[EncModeIdx].ReportRate;
        VinCfg.Layout = &Layout;

        MainCfg.Hdlr = VideoEncPri;
        MainCfg.MainLayout.LayerNumber = 1;
        MainCfg.MainLayout.Layer = &NewPriLayer;
        MainCfg.MainLayout.Width = VideoEncMgt[EncModeIdx].MainWidth;
        MainCfg.MainLayout.Height = VideoEncMgt[EncModeIdx].MainHeight;
        MainCfg.Interlace = EncPriInteralce; //TBD
        MainCfg.MainTickPerPicture = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TickPerPicture: VideoEncMgt[EncModeIdx].TickPerPicturePAL;
        MainCfg.MainTimeScale = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;

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
            UINT8 *dspWorkAddr;
            UINT32 dspWorkSize;
            AmpUT_VideoEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
            MainCfg.DspWorkBufAddr = dspWorkAddr;
            MainCfg.DspWorkBufSize = dspWorkSize;
        }

        if (LiveViewProcMode == 0) {
            if (VideoEncMgt[EncModeIdx].MainWidth > 1920 || \
                VideoEncMgt[EncModeIdx].CaptureWidth > 1920)
                if (LiveViewOSMode == 0) {
                    AmbaPrint("[UT_Video]Force to use NonOS Mode?? (VcapW %d,MainW %d)", VideoEncMgt[EncModeIdx].MainWidth, VideoEncMgt[EncModeIdx].CaptureWidth);
                }
        }

        MainCfg.LiveViewProcMode = LiveViewProcMode;
        MainCfg.LiveViewAlgoMode = LiveViewAlgoMode;
        MainCfg.LiveViewOSMode = LiveViewOSMode;
        if (SensorModeInfo.HdrIsSupport/* && HdrType==MULTI_SLICE */) {
            MainCfg.LiveViewHdrMode = LiveViewHdrMode = 1;
        } else {
            MainCfg.LiveViewHdrMode = LiveViewHdrMode = 0;
        }

        NewPriLayer.EnableSourceArea = 0;
        NewPriLayer.EnableTargetArea = 0;
        NewPriLayer.LayerId = 0;
        NewPriLayer.SourceType = AMP_ENC_SOURCE_VIN;
        NewPriLayer.Source = VideoEncVinA;
        NewPriLayer.SourceLayoutId = 0;

        AmpVideoEnc_ConfigVinMain(1, &VinCfg, 1, &MainCfg);
}
#endif
        // Assign Secondary main layout
        if (EncDualHDStream) {
            if (SecStreamCustomWidth) {
                EncCfg.MainLayout.Width = SecStreamCustomWidth;
                EncCfg.MainLayout.Height = SecStreamCustomHeight;
            } else {
                MWUT_InputSetDualHDWindow(VideoEncMgt, EncModeIdx, \
                    &EncCfg.MainLayout.Width, &EncCfg.MainLayout.Height);
            }
        } else {
            if (SecStreamCustomWidth) {
                EncCfg.MainLayout.Width = SecStreamCustomWidth;
                EncCfg.MainLayout.Height = SecStreamCustomHeight;
            } else {
                EncCfg.MainLayout.Width = SEC_STREAM_WIDTH;
                EncCfg.MainLayout.Height = SEC_STREAM_HEIGHT;
            }
        }
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = EncPriInteralce; //TBD
        if (EncodeSystem == 0) {
            EncCfg.MainTickPerPicture = SEC_STREAM_TICK;
            EncCfg.MainTimeScale = SEC_STREAM_TIMESCALE;
        } else {
            EncCfg.MainTickPerPicture = SEC_STREAM_TICK_PAL;
            EncCfg.MainTimeScale = SEC_STREAM_TIMESCALE_PAL;
        }
        EncLayer.SourceType = AMP_ENC_SOURCE_VIN;
        EncLayer.Source = VideoEncVinA;
        EncLayer.EnableSourceArea = 0;  // No source cropping
        EncLayer.EnableTargetArea = 0;  // No target pip
        EncLayer.SourceLayoutId = 1; // from 2nd layout of VideoEncVinA
        EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
        EncCfg.StreamId = AMP_VIDEOENC_STREAM_SECONDARY;
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
        EncCfg.LiveviewOBModeEnable = VideoOBModeEnable;
#endif
        // Create secondary stream handler
        AmpVideoEnc_Create(&EncCfg, &VideoEncSec); // Don't have to worry about h.264 spec settings when liveview
    }

#ifdef CONFIG_SOC_A9
    if (strcmp(MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem), "2560X1440  60P\0") == 0) {
        AmbaPrint("Turn OFF Tv");
        TvLiveview = 0;
    } else
#endif
    {
        AmbaPrint("Turn %s Tv",TvLiveview?"ON":"OFF");
    }

    {
        AmbaPrint("Turn ON LCD");
        LCDLiveview = 1;
    }

    // config wondow and start display
    AmpUT_VideoEnc_DisplayStart();

    // Register pipeline
    {
        AMP_ENC_PIPE_CFG_s pipeCfg;
        // Register pipeline
        AmpEnc_GetDefaultCfg(&pipeCfg);
        //pipeCfg.cbEvent
        pipeCfg.encoder[0] = VideoEncPri;
        pipeCfg.encoder[1] = VideoEncSec;
        if (EncDualStream || EncDualHDStream ) {
            pipeCfg.numEncoder = 2;
        } else {
            pipeCfg.numEncoder = 1;
        }
        pipeCfg.cbEvent = AmpUT_VideoEncPipeCallback;
        pipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&pipeCfg, &VideoEncPipe);

        AmpEnc_Add(VideoEncPipe);
    }

    // Set capture window to Dzoom
    {
        AMP_IMG_DZOOM_CFG_s dzoomCfg;
        AMP_IMG_DZOOM_VIN_INVALID_INFO_s dzoomVinInvalidInfo;

        AmpImgDzoom_GetDefaultCfg(&dzoomCfg);
        dzoomCfg.ImgModeContextId = 0;
        dzoomCfg.ImgModeBatchId = 0;
        AmpImgDzoom_Create(&dzoomCfg, &dzoomHdlr);
        dzoomTable.TotalStepNumber = DZOOM_10X_TOTAL_STEP;
        dzoomTable.MaxDzoomFactor = DZOOM_10X_MAX_RATIO;
        dzoomTable.GetDzoomFactor = Dzoom_Step_Factor_Table;
        AmpImgDzoom_RegDzoomTable(dzoomHdlr, &dzoomTable);

        dzoomVinInvalidInfo.CapW = VideoEncMgt[EncModeIdx].CaptureWidth;
        dzoomVinInvalidInfo.CapH = VideoEncMgt[EncModeIdx].CaptureHeight;

        AmpImgDzoom_ChangeResolutionHandler(dzoomHdlr, &dzoomVinInvalidInfo);

        AmpImgDzoom_ResetStatus(dzoomHdlr);
    }

    //
    // Note: For A9, Bitstream-specific configs can be assigned here (before encode start), or when codec instance creation.
    //       But for A7L/A12 family, it shall be assigned when codec instance creation and before liveview start.
    //
#ifdef CONFIG_SOC_A12
    {
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
        AMP_VIDEOENC_BITSTREAM_CFG_s BitsCfg = {0};

        BitsCfg.Rotation = EncRotation;
        BitsCfg.TimeLapse = EncTimeLapse;
        if (EncTimeLapse==0) {
            BitsCfg.VideoThumbnail = EncThumbnail;
        }
        BitsCfg.PIVMaxWidth = VideoEncMgt[EncModeIdx].MaxPivWidth;

        // Assign bitstream-specific configs
        if (VideoEncPri) {
            if (EncPriSpecH264) {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
                H264Cfg = &BitsCfg.Spec.H264Cfg;
                H264Cfg->GopM = VideoEncMgt[EncModeIdx].GopM;
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
                H264Cfg->StartFromBFrame = (H264Cfg->GopM > 1);
                H264Cfg->Interlace = EncPriInteralce;
                H264Cfg->TimeScale = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;
                H264Cfg->TickPerPicture = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TickPerPicture: VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
                H264Cfg->StopMethod = EncStopMethod;
                HeaderInfo.GopM = H264Cfg->GopM;
                HeaderInfo.Width = VideoEncMgt[EncModeIdx].MainWidth;
                HeaderInfo.Height = VideoEncMgt[EncModeIdx].MainHeight;
                HeaderInfo.Interlace = EncPriInteralce;
                HeaderInfo.Rotation = EncRotation;

                // Use default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = VideoEncMgt[EncModeIdx].BrcMode;
                H264Cfg->BitRateControl.AverageBitrate = (UINT32)(VideoEncMgt[EncModeIdx].AverageBitRate * 1E6);
                if (VideoEncMgt[EncModeIdx].BrcMode == VIDEOENC_SMART_VBR) {
                    H264Cfg->BitRateControl.MaxBitrate = (UINT32)(VideoEncMgt[EncModeIdx].MaxBitRate * 1E6);
                    H264Cfg->BitRateControl.MinBitrate = (UINT32)(VideoEncMgt[EncModeIdx].MinBitRate * 1E6);
                }
                H264Cfg->QualityControl.IBeatMode = EncIBeat;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                if ((HeaderInfo.Width>=1280) && (MjpegQuality>60)) {
                    MjpegQuality = 60;
                }
                AmpUT_VidoeEnc_InitMJpegDqt(MjpegQuantMatrix, MjpegQuality);
                MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(VideoEncPri, &BitsCfg);
        }

        if (VideoEncSec) {
            if (EncSecSpecH264) {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
                H264Cfg = &BitsCfg.Spec.H264Cfg;
                H264Cfg->GopM = SEC_STREAM_GOP_M;
                H264Cfg->GopN = SEC_STREAM_GOP_N;
                H264Cfg->GopIDR = SEC_STREAM_GOP_IDR;
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
                H264Cfg->StartFromBFrame = 0;
                H264Cfg->Interlace = EncSecInteralce;
                if (EncodeSystem == 0) {
                    H264Cfg->TimeScale = SEC_STREAM_TIMESCALE;
                    H264Cfg->TickPerPicture = SEC_STREAM_TICK;
                } else {
                    H264Cfg->TimeScale = SEC_STREAM_TIMESCALE_PAL;
                    H264Cfg->TickPerPicture = SEC_STREAM_TICK_PAL;
                }
                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
                H264Cfg->StopMethod = EncStopMethod;

                HeaderInfo.GopM = H264Cfg->GopM;
                if (EncDualHDStream) {
                    if (SecStreamCustomWidth) {
                        HeaderInfo.Width = SecStreamCustomWidth;
                        HeaderInfo.Height = SecStreamCustomHeight;
                    } else {
                        MWUT_InputSetDualHDWindow(VideoEncMgt, EncModeIdx, \
                            &HeaderInfo.Width, &HeaderInfo.Height);
                    }
                } else {
                    if (SecStreamCustomWidth) {
                        HeaderInfo.Width = SecStreamCustomWidth;
                        HeaderInfo.Height = SecStreamCustomHeight;
                    } else {
                        HeaderInfo.Width = SEC_STREAM_WIDTH;
                        HeaderInfo.Height = SEC_STREAM_HEIGHT;
                    }
                }
                HeaderInfo.Interlace = EncSecInteralce;
                HeaderInfo.Rotation = EncRotation;

                // Get default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->SPS.level_idc = 42;
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = VIDEOENC_SMART_VBR;
                if (EncDualHDStream) {
                    if (SecStreamCustomWidth) {
                        H264Cfg->BitRateControl.AverageBitrate = (UINT32)VideoEncMgt[EncModeIdx].AverageBitRate*1000*1000;
                        H264Cfg->BitRateControl.MaxBitrate = (UINT32)VideoEncMgt[EncModeIdx].MaxBitRate*1000*1000;
                        H264Cfg->BitRateControl.MinBitrate = (UINT32)VideoEncMgt[EncModeIdx].MinBitRate*1000*1000;
                    } else if (VideoEncMgt[EncModeIdx].MainWidth < SEC_STREAM_HD_WIDTH) {
                        H264Cfg->BitRateControl.AverageBitrate = (UINT32)VideoEncMgt[EncModeIdx].AverageBitRate*1000*1000;
                        H264Cfg->BitRateControl.MaxBitrate = (UINT32)VideoEncMgt[EncModeIdx].MaxBitRate*1000*1000;
                        H264Cfg->BitRateControl.MinBitrate = (UINT32)VideoEncMgt[EncModeIdx].MinBitRate*1000*1000;
                    } else {
                        H264Cfg->BitRateControl.AverageBitrate = SEC_STREAM_HD_BRATE_AVG*1000*1000;
                        H264Cfg->BitRateControl.MaxBitrate = SEC_STREAM_HD_BRATE_MAX*1000*1000;
                        H264Cfg->BitRateControl.MinBitrate = SEC_STREAM_HD_BRATE_MIN*1000*1000;
                    }
                } else {
                    //Small SecStrm
                    H264Cfg->BitRateControl.AverageBitrate = (UINT32)SEC_STREAM_BRATE_AVG*1000*1000;
                    H264Cfg->BitRateControl.MaxBitrate = (UINT32)SEC_STREAM_BRATE_MAX*1000*1000;
                    H264Cfg->BitRateControl.MinBitrate = (UINT32)SEC_STREAM_BRATE_MIN*1000*1000;
                }
                H264Cfg->QualityControl.IBeatMode = EncIBeat;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                if ((HeaderInfo.Width>=1280) && (MjpegQuality>60)) {
                    MjpegQuality = 60;
                }
                AmpUT_VidoeEnc_InitMJpegDqt(MjpegQuantMatrix, MjpegQuality);
                MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(VideoEncSec, &BitsCfg);
        }
    }
#endif

    AmpEnc_StartLiveview(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);

    return 0;
}

int AmpUT_VideoEnc_EncodeStart(void)
{
    AMP_FIFO_CFG_s FifoDefCfg;
    AMP_ENC_BITSBUFFER_CFG_s  BitsBufCfg;
    AMP_VIDEOENC_BITSTREAM_CFG_s CurrentCfg;

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" VideoEncode : %s", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
    AmbaPrint(" VideoEncode : CaptureWin[%dx%d]", VideoEncMgt[EncModeIdx].CaptureWidth, VideoEncMgt[EncModeIdx].CaptureHeight);
    AmbaPrint(" VideoEncode : PriStream EncWin [%dx%d]", VideoEncMgt[EncModeIdx].MainWidth, VideoEncMgt[EncModeIdx].MainHeight);
    if (EncPriSpecH264) {
        AmpVideoEnc_GetBitstreamConfig(VideoEncPri, &CurrentCfg);
        AmbaPrint(" VideoEncode : AvgBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.AverageBitrate/1000000);
        AmbaPrint(" VideoEncode : MaxBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.MaxBitrate/1000000);
        AmbaPrint(" VideoEncode : MinBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.MinBitrate/1000000);
        AmbaPrint(" VideoEncode : PriStream H264 GopM   %u", CurrentCfg.Spec.H264Cfg.GopM);
        AmbaPrint(" VideoEncode : PriStream H264 GopN   %u", CurrentCfg.Spec.H264Cfg.GopN);
        AmbaPrint(" VideoEncode : PriStream H264 GopIDR %u", CurrentCfg.Spec.H264Cfg.GopIDR);
    } else {
        AmbaPrint(" VideoEncode : PriStream MJpeg");
    }

    if (EncDualStream || EncDualHDStream) {
        UINT16 SecEncWidth, SecEncHeight;
        if (EncDualHDStream) {
            if (SecStreamCustomWidth) {
                SecEncWidth = SecStreamCustomWidth;
                SecEncHeight = SecStreamCustomHeight;
            } else {
                MWUT_InputSetDualHDWindow(VideoEncMgt, EncModeIdx, &SecEncWidth, &SecEncHeight);
            }
            AmbaPrint(" VideoEncode : SecStream EncWin [%dx%d]", SecEncWidth, SecEncHeight);
        } else {
            if (SecStreamCustomWidth) {
                SecEncWidth = SecStreamCustomWidth;
                SecEncHeight = SecStreamCustomHeight;
            } else {
                SecEncWidth = SEC_STREAM_WIDTH;
                SecEncHeight = SEC_STREAM_HEIGHT;
            }
            AmbaPrint(" VideoEncode : SecStream EncWin [%dx%d]", SecEncWidth, SecEncHeight);
        }
    }

    if (EncSecSpecH264 && (EncDualStream || EncDualHDStream)) {
        AmpVideoEnc_GetBitstreamConfig(VideoEncSec, &CurrentCfg);
        AmbaPrint(" VideoEncode : AvgBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.AverageBitrate/1000000);
        AmbaPrint(" VideoEncode : MaxBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.MaxBitrate/1000000);
        AmbaPrint(" VideoEncode : MinBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.MinBitrate/1000000);
        AmbaPrint(" VideoEncode : SecStream H264 GopM   %u", CurrentCfg.Spec.H264Cfg.GopM);
        AmbaPrint(" VideoEncode : SecStream H264 GopN   %u", CurrentCfg.Spec.H264Cfg.GopN);
        AmbaPrint(" VideoEncode : SecStream H264 GopIDR %u", CurrentCfg.Spec.H264Cfg.GopIDR);
    } else {
        if (EncDualStream || EncDualHDStream) {
            AmbaPrint(" VideoEncode : SecStream MJpeg");
        }
    }
    AmbaPrint(" =========================================================");

#ifdef CONFIG_SOC_A12
    { // retrieve current QP settings for QP_adjustment
        if (EncPriSpecH264 && VideoEncPri) {
            VideoEncPriQpIsZero = 0;
            AmpVideoEnc_GetBitstreamConfig(VideoEncPri, &CurrentCfg);
            VideoEncPriCurrQpMinI = CurrentCfg.Spec.H264Cfg.QPControl.QpMinI;
            VideoEncPriCurrQpMaxI = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxI;
            VideoEncPriCurrQpMinP = CurrentCfg.Spec.H264Cfg.QPControl.QpMinP;
            VideoEncPriCurrQpMaxP = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxP;
            VideoEncPriCurrQpMinB = CurrentCfg.Spec.H264Cfg.QPControl.QpMinB;
            VideoEncPriCurrQpMaxB = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxB;
        }

        if (EncSecSpecH264 && VideoEncSec && (EncDualStream || EncDualHDStream)) {
            VideoEncSecQpIsZero = 0;
            AmpVideoEnc_GetBitstreamConfig(VideoEncSec, &CurrentCfg);
            VideoEncSecCurrQpMinI = CurrentCfg.Spec.H264Cfg.QPControl.QpMinI;
            VideoEncSecCurrQpMaxI = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxI;
            VideoEncSecCurrQpMinP = CurrentCfg.Spec.H264Cfg.QPControl.QpMinP;
            VideoEncSecCurrQpMaxP = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxP;
            VideoEncSecCurrQpMinB = CurrentCfg.Spec.H264Cfg.QPControl.QpMinB;
            VideoEncSecCurrQpMaxB = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxB;
        }
    }

    {  // renew AQP table
        if (EncPriSpecH264 && VideoEncPri && AqpHdlrPri) {
            ADJ_AQP_INFO_s AqpInfo = {0};
            AMBA_IMG_ENC_MONITOR_ENCODING_INFO_s EncodeInfo = {0};
            AmpVideoEnc_GetBitstreamConfig(VideoEncPri, &CurrentCfg);

            /* Check ADJ data base first */
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_AQP_INFO, 0/*chNo*/, (UINT32)&AqpInfo, 0);
            if (AqpInfo.UpdateFlg) {
                AMBA_ENCMONITOR_RUNTIME_QUALITY_CFG_s Cfg = {0};

                Cfg.Cmd |= QC_QMODEL;
                Cfg.AQPStrength = AqpInfo.AQPParams.Value[0];
                Cfg.Intra16x16Bias = AqpInfo.AQPParams.Value[1];
                Cfg.Intra4x4Bias = AqpInfo.AQPParams.Value[2];
                Cfg.Inter16x16Bias = AqpInfo.AQPParams.Value[3];
                Cfg.Inter8x8Bias = AqpInfo.AQPParams.Value[4];
                Cfg.Direct16x16Bias = AqpInfo.AQPParams.Value[5];
                Cfg.Direct8x8Bias = AqpInfo.AQPParams.Value[6];
                Cfg.MELambdaQpOffset = AqpInfo.AQPParams.Value[7];
                Cfg.Alpha = AqpInfo.AQPParams.Value[8];
                Cfg.Beta = AqpInfo.AQPParams.Value[9];
                AmbaEncMonitor_SetRuntimeQuality(EncMonitorStrmHdlrPri, &Cfg);

                AqpInfo.UpdateFlg = 0;
                AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_AQP_INFO, 0/*chNo*/, (UINT32)&AqpInfo, 0);
                AmbaEncMonitor_GetCurrentEncodingInfo(EncMonitorStrmHdlrPri, &EncodeInfo);
                CurrentCfg.Spec.H264Cfg.QualityControl.AutoQpStrength = EncodeInfo.QCfg.QualityModelConfig.AQPStrength;
                CurrentCfg.Spec.H264Cfg.QualityControl.Intra16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Intra16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Intra4x4Bias = EncodeInfo.QCfg.QualityModelConfig.Intra4by4Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Inter16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Inter16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Inter8x8Bias = EncodeInfo.QCfg.QualityModelConfig.Inter8by8Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Direct16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Direct16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Direct8x8Bias = EncodeInfo.QCfg.QualityModelConfig.Direct8by8Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.MELambdaQpOffset = EncodeInfo.QCfg.QualityModelConfig.MeLambdaQPOffset;
                CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterAlpha = EncodeInfo.QCfg.QualityModelConfig.Alpha;
                CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterBeta = EncodeInfo.QCfg.QualityModelConfig.Beta;
                AmpVideoEnc_SetBitstreamConfig(VideoEncPri, &CurrentCfg);
            } else {
                //Using resident data in StrmHdlr
                AmbaEncMonitor_GetCurrentEncodingInfo(EncMonitorStrmHdlrPri, &EncodeInfo);
                CurrentCfg.Spec.H264Cfg.QualityControl.AutoQpStrength = EncodeInfo.QCfg.QualityModelConfig.AQPStrength;
                CurrentCfg.Spec.H264Cfg.QualityControl.Intra16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Intra16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Intra4x4Bias = EncodeInfo.QCfg.QualityModelConfig.Intra4by4Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Inter16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Inter16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Inter8x8Bias = EncodeInfo.QCfg.QualityModelConfig.Inter8by8Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Direct16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Direct16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Direct8x8Bias = EncodeInfo.QCfg.QualityModelConfig.Direct8by8Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.MELambdaQpOffset = EncodeInfo.QCfg.QualityModelConfig.MeLambdaQPOffset;
                CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterAlpha = EncodeInfo.QCfg.QualityModelConfig.Alpha;
                CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterBeta = EncodeInfo.QCfg.QualityModelConfig.Beta;
                AmpVideoEnc_SetBitstreamConfig(VideoEncPri, &CurrentCfg);
            }
        }

        if (EncSecSpecH264 && VideoEncSec && AqpHdlrSec) {
            ADJ_AQP_INFO_s AqpInfo = {0};
            AMBA_IMG_ENC_MONITOR_ENCODING_INFO_s EncodeInfo = {0};
            AmpVideoEnc_GetBitstreamConfig(VideoEncSec, &CurrentCfg);

            /* Check ADJ data base first */
            AmbaImg_Proc_Cmd(MW_IP_GET_ADJ_AQP_INFO, 0/*chNo*/, (UINT32)&AqpInfo, 0);
            if (AqpInfo.UpdateFlg) {
                AMBA_ENCMONITOR_RUNTIME_QUALITY_CFG_s Cfg = {0};

                Cfg.Cmd |= QC_QMODEL;
                Cfg.AQPStrength = AqpInfo.AQPParams.Value[0];
                Cfg.Intra16x16Bias = AqpInfo.AQPParams.Value[1];
                Cfg.Intra4x4Bias = AqpInfo.AQPParams.Value[2];
                Cfg.Inter16x16Bias = AqpInfo.AQPParams.Value[3];
                Cfg.Inter8x8Bias = AqpInfo.AQPParams.Value[4];
                Cfg.Direct16x16Bias = AqpInfo.AQPParams.Value[5];
                Cfg.Direct8x8Bias = AqpInfo.AQPParams.Value[6];
                Cfg.MELambdaQpOffset = AqpInfo.AQPParams.Value[7];
                Cfg.Alpha = AqpInfo.AQPParams.Value[8];
                Cfg.Beta = AqpInfo.AQPParams.Value[9];
                AmbaEncMonitor_SetRuntimeQuality(EncMonitorStrmHdlrSec, &Cfg);

                AqpInfo.UpdateFlg = 0;
                AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_AQP_INFO, 0/*chNo*/, (UINT32)&AqpInfo, 0);
                AmbaEncMonitor_GetCurrentEncodingInfo(EncMonitorStrmHdlrSec, &EncodeInfo);
                CurrentCfg.Spec.H264Cfg.QualityControl.AutoQpStrength = EncodeInfo.QCfg.QualityModelConfig.AQPStrength;
                CurrentCfg.Spec.H264Cfg.QualityControl.Intra16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Intra16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Intra4x4Bias = EncodeInfo.QCfg.QualityModelConfig.Intra4by4Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Inter16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Inter16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Inter8x8Bias = EncodeInfo.QCfg.QualityModelConfig.Inter8by8Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Direct16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Direct16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Direct8x8Bias = EncodeInfo.QCfg.QualityModelConfig.Direct8by8Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.MELambdaQpOffset = EncodeInfo.QCfg.QualityModelConfig.MeLambdaQPOffset;
                CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterAlpha = EncodeInfo.QCfg.QualityModelConfig.Alpha;
                CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterBeta = EncodeInfo.QCfg.QualityModelConfig.Beta;
                AmpVideoEnc_SetBitstreamConfig(VideoEncSec, &CurrentCfg);
            } else {
                //Using resident data in StrmHdlr
                AmbaEncMonitor_GetCurrentEncodingInfo(EncMonitorStrmHdlrSec, &EncodeInfo);
                CurrentCfg.Spec.H264Cfg.QualityControl.AutoQpStrength = EncodeInfo.QCfg.QualityModelConfig.AQPStrength;
                CurrentCfg.Spec.H264Cfg.QualityControl.Intra16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Intra16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Intra4x4Bias = EncodeInfo.QCfg.QualityModelConfig.Intra4by4Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Inter16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Inter16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Inter8x8Bias = EncodeInfo.QCfg.QualityModelConfig.Inter8by8Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Direct16x16Bias = EncodeInfo.QCfg.QualityModelConfig.Direct16by16Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.Direct8x8Bias = EncodeInfo.QCfg.QualityModelConfig.Direct8by8Bias;
                CurrentCfg.Spec.H264Cfg.QualityControl.MELambdaQpOffset = EncodeInfo.QCfg.QualityModelConfig.MeLambdaQPOffset;
                CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterAlpha = EncodeInfo.QCfg.QualityModelConfig.Alpha;
                CurrentCfg.Spec.H264Cfg.QualityControl.LoopFilterBeta = EncodeInfo.QCfg.QualityModelConfig.Beta;
                AmpVideoEnc_SetBitstreamConfig(VideoEncSec, &CurrentCfg);
            }
        }
    }

    {  // renew mjpeg Qtable
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_BITSTREAM_CFG_s BitsCfg = {0};

        if (VideoEncPri) {
            if (EncPriSpecH264 == 0) {
                AmpVideoEnc_GetBitstreamConfig(VideoEncPri, &BitsCfg);
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;
                AmpUT_VidoeEnc_InitMJpegDqt(MjpegQuantMatrix, MjpegQuality);
                MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;
                AmpVideoEnc_SetBitstreamConfig(VideoEncPri, &BitsCfg);
            }
        }

        if (VideoEncSec) {
            if (EncSecSpecH264 == 0) {
                AmpVideoEnc_GetBitstreamConfig(VideoEncSec, &BitsCfg);
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;
                AmpUT_VidoeEnc_InitMJpegDqt(MjpegQuantMatrix, MjpegQuality);
                MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;
                AmpVideoEnc_SetBitstreamConfig(VideoEncSec, &BitsCfg);
            }
        }
    }
#endif

    //
    // Note: For A9, Bitstream-specific configs can be assigned here (before encode start), or when codec instance creation.
    //       But for A7L/A12 family, it shall be assigned when codec instance creation and before liveview start.
    //
#ifdef CONFIG_SOC_A9
    {
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
        AMP_VIDEOENC_BITSTREAM_CFG_s BitsCfg;

        memset(&BitsCfg, 0x0, sizeof(AMP_VIDEOENC_BITSTREAM_CFG_s));
        BitsCfg.Rotation = EncRotation;
        BitsCfg.TimeLapse = EncTimeLapse;
        BitsCfg.VideoThumbnail = EncThumbnail;

        // Assign bitstream-specific configs
        if (VideoEncPri) {
            if (EncPriSpecH264) {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
                H264Cfg = &BitsCfg.Spec.H264Cfg;
                H264Cfg->GopM = VideoEncMgt[EncModeIdx].GopM;
                H264Cfg->GopN = GOP_N;
                H264Cfg->GopIDR = GOP_IDR;
                H264Cfg->GopHierarchical = 0;
                H264Cfg->Cabac = 1;
                H264Cfg->QualityControl.LoopFilterEnable = 1;
                H264Cfg->QualityControl.LoopFilterAlpha = 0;
                H264Cfg->QualityControl.LoopFilterBeta = 0;
                H264Cfg->StartFromBFrame = (H264Cfg->GopM > 1);
                H264Cfg->Interlace = EncPriInteralce;
                H264Cfg->TimeScale = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;
                H264Cfg->TickPerPicture = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TickPerPicture: VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0x94; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
                H264Cfg->StopMethod = EncStopMethod;
                HeaderInfo.GopM = H264Cfg->GopM;
                HeaderInfo.Width = VideoEncMgt[EncModeIdx].MainWidth;
                HeaderInfo.Height = VideoEncMgt[EncModeIdx].MainHeight;
                HeaderInfo.Interlace = EncPriInteralce;
                HeaderInfo.Rotation = EncRotation;

                // Use default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = VideoEncMgt[EncModeIdx].BrcMode;
                H264Cfg->BitRateControl.AverageBitrate = (UINT32)(VideoEncMgt[EncModeIdx].AverageBitRate * 1E6);
                if (VideoEncMgt[EncModeIdx].BrcMode == VIDEOENC_SMART_VBR) {
                    H264Cfg->BitRateControl.MaxBitrate = (UINT32)(VideoEncMgt[EncModeIdx].MaxBitRate * 1E6);
                    H264Cfg->BitRateControl.MinBitrate = (UINT32)(VideoEncMgt[EncModeIdx].MinBitRate * 1E6);
                }
                H264Cfg->QualityControl.IBeatMode = EncIBeat;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                AmpUT_VidoeEnc_InitMJpegDqt(MjpegQuantMatrix, MjpegQuality);
                MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(VideoEncPri, &BitsCfg);
        }

        if (VideoEncSec) {
            if (EncSecSpecH264) {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
                H264Cfg = &BitsCfg.Spec.H264Cfg;
                H264Cfg->GopM = SEC_STREAM_GOP_M;
                H264Cfg->GopN = SEC_STREAM_GOP_N;
                H264Cfg->GopIDR = SEC_STREAM_GOP_IDR;
                H264Cfg->GopHierarchical = 0;
                H264Cfg->Cabac = 1;
                H264Cfg->QualityControl.LoopFilterEnable = 1;
                H264Cfg->QualityControl.LoopFilterAlpha = 0;
                H264Cfg->QualityControl.LoopFilterBeta = 0;
                H264Cfg->StartFromBFrame = 0;
                H264Cfg->Interlace = EncSecInteralce;
                if (EncodeSystem == 0) {
                    H264Cfg->TimeScale = SEC_STREAM_TIMESCALE;
                    H264Cfg->TickPerPicture = SEC_STREAM_TICK;
                } else {
                    H264Cfg->TimeScale = SEC_STREAM_TIMESCALE_PAL;
                    H264Cfg->TickPerPicture = SEC_STREAM_TICK_PAL;
                }
                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0x94; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A

                H264Cfg->StopMethod = EncStopMethod;

                HeaderInfo.GopM = H264Cfg->GopM;
                HeaderInfo.Width = SecEncWidth;
                HeaderInfo.Height = SecEncHeight;

                HeaderInfo.Interlace = EncSecInteralce;
                HeaderInfo.Rotation = EncRotation;

                // Get default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->SPS.level_idc = 42;
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = VIDEOENC_SMART_VBR;
                if (EncDualHDStream) {
                    if (SecStreamCustomWidth) {
                        H264Cfg->BitRateControl.AverageBitrate = (UINT32)VideoEncMgt[EncModeIdx].AverageBitRate*1000*1000;
                    } else if (VideoEncMgt[EncModeIdx].MainWidth < SEC_STREAM_HD_WIDTH) {
                        H264Cfg->BitRateControl.AverageBitrate = (UINT32)VideoEncMgt[EncModeIdx].AverageBitRate*1000*1000;
                    } else {
                        H264Cfg->BitRateControl.AverageBitrate = (UINT32)SEC_STREAM_HD_BRATE_AVG*1000*1000;
                    }
                } else {
                    H264Cfg->BitRateControl.AverageBitrate = (UINT32)2E6;
                }
                H264Cfg->QualityControl.IBeatMode = EncIBeat;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                AmpUT_VidoeEnc_InitMJpegDqt(MjpegQuantMatrix, MjpegQuality);
                MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(VideoEncSec, &BitsCfg);
        }
    }
#endif

    //
    // Setup bitstream buffer.
    //
    // Rule: H.264 and MJPEG can't use the same bitstream/descriptor buffer. Same Spec uses the same buffer. No matter it is primary or secondary
    // Note: Since buffer allocation depends on the above rule, it is better to assign bitstream buffer before encode start.
    //       Otherwise you have to know what you are going to encode when codec instance creation
    {
        AmbaPrint("H.264 Bits 0x%x size %d Desc 0x%x size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
        AmbaPrint("MJPEG Bits 0x%x size %d Desc 0x%x size %d", MjpgBitsBuf, BITSFIFO_SIZE, MjpgDescBuf, DESC_SIZE);
    }

    memset(&BitsBufCfg, 0x0, sizeof(AMP_ENC_BITSBUFFER_CFG_s));
    if (VirtualPriFifoHdlr == NULL) {
        //Set BaseInformation only once
        if (EncPriSpecH264) {
            BitsBufCfg.BitsBufAddr = H264BitsBuf;
            BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
            BitsBufCfg.DescBufAddr = H264DescBuf;
            BitsBufCfg.DescBufSize = DESC_SIZE;
            BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        } else {
            BitsBufCfg.BitsBufAddr = MjpgBitsBuf;
            BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
            BitsBufCfg.DescBufAddr = MjpgDescBuf;
            BitsBufCfg.DescBufSize = DESC_SIZE;
            BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        }
        AmpVideoEnc_SetBitstreamBuffer(VideoEncPri, &BitsBufCfg);

        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&FifoDefCfg);
        FifoDefCfg.hCodec = VideoEncPri;
        FifoDefCfg.IsVirtual = 1;
        FifoDefCfg.NumEntries = DESC_SIZE/sizeof(AMBA_DSP_EVENT_ENC_PIC_READY_s);
        FifoDefCfg.cbEvent = AmpUT_VideoEnc_FifoCB;
        AmpFifo_Create(&FifoDefCfg, &VirtualPriFifoHdlr);
    } else {
        //Following are follow previous
        if (EncPriSpecH264) {
            BitsBufCfg.BitsBufAddr = 0;
            BitsBufCfg.BitsBufSize = 0;
            BitsBufCfg.DescBufAddr = 0;
            BitsBufCfg.DescBufSize = 0;
            BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        } else {
            BitsBufCfg.BitsBufAddr = 0;
            BitsBufCfg.BitsBufSize = 0;
            BitsBufCfg.DescBufAddr = 0;
            BitsBufCfg.DescBufSize = 0;
            BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        }
        AmpVideoEnc_SetBitstreamBuffer(VideoEncPri, &BitsBufCfg);
        AmpFifo_EraseAll(VirtualPriFifoHdlr);
    }
    if (VirtualSecFifoHdlr == NULL) {
        AMP_ENC_BITSBUFFER_CFG_s  BitsBufCfg = {0};

        if (VideoEncSec) {
            if (EncSecSpecH264) {
                BitsBufCfg.BitsBufAddr = H264BitsBuf;
                BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
                BitsBufCfg.DescBufAddr = H264DescBuf;
                BitsBufCfg.DescBufSize = DESC_SIZE;
                BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            } else {
                BitsBufCfg.BitsBufAddr = MjpgBitsBuf;
                BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
                BitsBufCfg.DescBufAddr = MjpgDescBuf;
                BitsBufCfg.DescBufSize = DESC_SIZE;
                BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            }
            AmpVideoEnc_SetBitstreamBuffer(VideoEncSec, &BitsBufCfg);
        }

        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&FifoDefCfg);
        FifoDefCfg.hCodec = VideoEncSec;
        FifoDefCfg.IsVirtual = 1;
        FifoDefCfg.NumEntries = DESC_SIZE/sizeof(AMBA_DSP_EVENT_ENC_PIC_READY_s);
        FifoDefCfg.cbEvent = AmpUT_VideoEnc_FifoCB;
        AmpFifo_Create(&FifoDefCfg, &VirtualSecFifoHdlr);
    } else {
        //Following are follow previous
        if (VideoEncSec) {
            if (EncSecSpecH264) {
                BitsBufCfg.BitsBufAddr = 0;
                BitsBufCfg.BitsBufSize = 0;
                BitsBufCfg.DescBufAddr = 0;
                BitsBufCfg.DescBufSize = 0;
                BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            } else {
                BitsBufCfg.BitsBufAddr = 0;
                BitsBufCfg.BitsBufSize = 0;
                BitsBufCfg.DescBufAddr = 0;
                BitsBufCfg.DescBufSize = 0;
                BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            }
            AmpVideoEnc_SetBitstreamBuffer(VideoEncSec, &BitsBufCfg);
            AmpFifo_EraseAll(VirtualSecFifoHdlr);
        }
    }

#ifdef CONFIG_SOC_A12
    if (EncMonitorEnable || EncMonitorAQPEnable) {
        AmbaEncMonitor_EnableStreamHandler(EncMonitorStrmHdlrPri, 1);
        if (EncMonitorEnable) {
            AmbaEncMonitorBRC_EnableService(BrcHdlrPri, 1);
        }
        if (EncMonitorAQPEnable) {
            AmbaEncMonitorAQP_EnableService(AqpHdlrPri, 1);
        }

        if (EncSecSpecH264 && (EncDualStream || EncDualHDStream)) {
            AmbaEncMonitor_EnableStreamHandler(EncMonitorStrmHdlrSec, 1);
            if (EncMonitorEnable) {
                AmbaEncMonitorBRC_EnableService(BrcHdlrSec, 1);
            }
            if (EncMonitorAQPEnable) {
                AmbaEncMonitorAQP_EnableService(AqpHdlrSec, 1);
            }
        }
    }
#endif

    if (EncThumbnail && EncTimeLapse == 0) {
        //issue PIV cmd to let SSP start turely video-encode
        AMP_VIDEOENC_PIV_CTRL_s PivCtrl;
        /* Shall use LISO directly to prevent HybridMode fail */
        UINT32 iso = 1; // 0:HISO 1: LISO 2:Fast
        UINT8 cmpr = 1;
        UINT32 tsize = 0;
        UINT8 loop = 0;

        PivVideoThm = 1;
        if (LiveViewProcMode) {
            PivVideoThmJpgCount = 2;
        } else {
            PivVideoThmJpgCount = 3;
        }
        PivCtrl.SensorMode.Data = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;;
        PivCtrl.CaptureWidth = VideoEncMgt[EncModeIdx].CaptureWidth;
        PivCtrl.CaptureHeight = VideoEncMgt[EncModeIdx].CaptureHeight;
        PivCtrl.MainWidth = VideoEncMgt[EncModeIdx].MainWidth;
        PivCtrl.MainHeight = VideoEncMgt[EncModeIdx].MainHeight;
        PivCtrl.AspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
        AmpEnc_StartRecord(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
        AmpUT_VideoEnc_PIV(PivCtrl, iso, cmpr, tsize, loop);
    } else {
        AmpEnc_StartRecord(VideoEncPipe, 0);
    }

    return 0;
}
int AmpUT_VideoEnc_EncStop()
{
    AmpEnc_StopRecord(VideoEncPipe, 0);
#ifdef CONFIG_SOC_A12
    if (EncMonitorEnable || EncMonitorAQPEnable) {
        AmbaEncMonitor_EnableStreamHandler(EncMonitorStrmHdlrPri, 0);
        if (EncMonitorEnable) {
            AmbaEncMonitorBRC_EnableService(BrcHdlrPri, 0);
        }
        if (EncMonitorAQPEnable) {
            AmbaEncMonitorAQP_EnableService(AqpHdlrPri, 0);
        }

        if (EncSecSpecH264 && (EncDualStream || EncDualHDStream)) {
            AmbaEncMonitor_EnableStreamHandler(EncMonitorStrmHdlrSec, 0);
            if (EncMonitorEnable) {
                AmbaEncMonitorBRC_EnableService(BrcHdlrSec, 0);
            }
            if (EncMonitorAQPEnable) {
                AmbaEncMonitorAQP_EnableService(AqpHdlrSec, 0);
            }
        }
    }
#endif
    fnoPiv = 0xFFFF; //reset Piv file numver
    fnoScrnPiv = 0xFFFF;
    fnoThmPiv = 0xFFFF;
    return 0;
}

int AmpUT_VideoEnc_LiveviewStop(void)
{
    UINT32 Flag = AMP_ENC_FUNC_FLAG_WAIT;

#ifdef CONFIG_SOC_A12
    if (EncMonitorEnable || EncMonitorAQPEnable) {
        AmbaEncMonitor_StreamUnregister(EncMonitorStrmHdlrPri);
        EncMonitorStrmHdlrPri = NULL;
        if (EncMonitorEnable) {
            AmbaEncMonitorBRC_UnRegisterService(BrcHdlrPri);
            BrcHdlrPri = NULL;
        }
        if (EncMonitorAQPEnable) {
            AmbaEncMonitorAQP_UnRegisterService(AqpHdlrPri);
            AqpHdlrPri = NULL;
        }

        if (EncSecSpecH264 && (EncDualStream || EncDualHDStream)) {
            AmbaEncMonitor_StreamUnregister(EncMonitorStrmHdlrSec);
            EncMonitorStrmHdlrSec = NULL;
            if (EncMonitorEnable) {
                AmbaEncMonitorBRC_UnRegisterService(BrcHdlrSec);
                BrcHdlrSec = NULL;
            }
            if (EncMonitorAQPEnable) {
                AmbaEncMonitorAQP_UnRegisterService(AqpHdlrSec);
                AqpHdlrSec = NULL;
            }
        }
    }
#endif

    // Set Dzoom
    {
        AmpImgDzoom_StopDzoom(dzoomHdlr);
    }

    if (ImgSchdlr) {
        AmbaImgSchdlr_Enable(ImgSchdlr, 0);
    }
    Amba_Img_VIn_Invalid(VideoEncPri, (UINT32 *)NULL);


#ifdef CONFIG_SOC_A12
    if (VideoEnc_EisAlgoEnable) {
        AmbaEis_Inactive();
        AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_DISABLE, 0, 0, 0);
        AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_SET_FOCAL_LENGTH, 360, 0, 0);
    }
#endif

    if (WirelessModeEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        DspSuspendEnable = 0;
    } else if (DspSuspendEnable) {
        Flag |= AMP_ENC_FUNC_FLAG_SUS_DSP_ON_STOP;
        Flag &= ~AMP_ENC_FUNC_FLAG_IDLE_DSP_ON_STOP;
        WirelessModeEnable = 0;
    }
    AmpEnc_StopLiveview(VideoEncPipe, Flag);

#ifndef KEEP_ALL_INSTANCES
    AmpEnc_Delete(VideoEncPipe); VideoEncPipe = NULL;
    AmpVin_Delete(VideoEncVinA); VideoEncVinA = NULL;
    AmbaImgSchdlr_Delete(ImgSchdlr);
    AmpVideoEnc_Delete(VideoEncPri); VideoEncPri = NULL;
    AmpVideoEnc_Delete(VideoEncSec); VideoEncSec = NULL;

    if (AmpResource_GetVoutSeamlessEnable() == 0) {
        if (LCDLiveview) {
            AmpUT_Display_Stop(0);
            AmpUT_Display_Window_Delete(0);
        }
    }

    if (TvLiveview) {
        AmpUT_Display_Stop(1);
        AmpUT_Display_Window_Delete(1);
    }

    if (VirtualPriFifoHdlr) {
        AmpFifo_Delete(VirtualPriFifoHdlr);
        VirtualPriFifoHdlr = NULL;
    }
    if (VirtualSecFifoHdlr) {
        AmpFifo_Delete(VirtualSecFifoHdlr);
        VirtualSecFifoHdlr = NULL;
    }

    // Delete Dzoom
    {
        AmpImgDzoom_Delete(dzoomHdlr);
    }

#endif

    return 0;
}


int AmpUT_VideoEnc_EncPause(void)
{
    if (Status == STATUS_ENCODE) {
        AmpEnc_PauseRecord(VideoEncPipe, 0);
    #ifdef CONFIG_SOC_A12
        if (EncMonitorEnable || EncMonitorAQPEnable) {
            if (EncMonitorEnable) {
                AmbaEncMonitorBRC_EnableService(BrcHdlrPri, 0);
            }
            if (EncMonitorAQPEnable) {
                AmbaEncMonitorAQP_EnableService(AqpHdlrPri, 0);
            }

            if (EncSecSpecH264 && (EncDualStream || EncDualHDStream)) {
                if (EncMonitorEnable) {
                    AmbaEncMonitorBRC_EnableService(BrcHdlrSec, 0);
                }
                if (EncMonitorAQPEnable) {
                    AmbaEncMonitorAQP_EnableService(AqpHdlrSec, 0);
                }
            }
        }
    #endif
        Status = STATUS_PAUSE;
    }
    return 0;
}

int AmpUT_VideoEnc_EncResume(void)
{
    AMP_ENC_BITSBUFFER_CFG_s  BitsBufCfg = {0};

    if (EncPriSpecH264) {
        BitsBufCfg.BitsBufAddr = H264BitsBuf;
        BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
        BitsBufCfg.DescBufAddr = H264DescBuf;
        BitsBufCfg.DescBufSize = DESC_SIZE;
        BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
    } else {
        BitsBufCfg.BitsBufAddr = MjpgBitsBuf;
        BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
        BitsBufCfg.DescBufAddr = MjpgDescBuf;
        BitsBufCfg.DescBufSize = DESC_SIZE;
        BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
    }
    AmpVideoEnc_SetBitstreamBuffer(VideoEncPri, &BitsBufCfg);

    if (VideoEncSec) {
        if (EncSecSpecH264) {
            BitsBufCfg.BitsBufAddr = H264BitsBuf;
            BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
            BitsBufCfg.DescBufAddr = H264DescBuf;
            BitsBufCfg.DescBufSize = DESC_SIZE;
            BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        } else {
            BitsBufCfg.BitsBufAddr = MjpgBitsBuf;
            BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
            BitsBufCfg.DescBufAddr = MjpgDescBuf;
            BitsBufCfg.DescBufSize = DESC_SIZE;
            BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        }
        AmpVideoEnc_SetBitstreamBuffer(VideoEncSec, &BitsBufCfg);
    }
    AmbaPrint("H.264 Bits 0x%x size %d Desc 0x%x size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
    AmbaPrint("MJPEG Bits 0x%x size %d Desc 0x%x size %d", MjpgBitsBuf, BITSFIFO_SIZE, MjpgDescBuf, DESC_SIZE);

    if (Status == STATUS_PAUSE) {
        AmpEnc_ResumeRecord(VideoEncPipe, 0);
    #ifdef CONFIG_SOC_A12
        if (EncMonitorEnable || EncMonitorAQPEnable) {
            if (EncMonitorEnable) {
                AmbaEncMonitorBRC_EnableService(BrcHdlrPri, 1);
            }
            if (EncMonitorAQPEnable) {
                AmbaEncMonitorAQP_EnableService(AqpHdlrPri, 1);
            }

            if (EncSecSpecH264 && (EncDualStream || EncDualHDStream)) {
                if (EncMonitorEnable) {
                    AmbaEncMonitorBRC_EnableService(BrcHdlrSec, 1);
                }
                if (EncMonitorAQPEnable) {
                    AmbaEncMonitorAQP_EnableService(AqpHdlrSec, 1);
                }
            }
        }
    #endif
        Status = STATUS_ENCODE;
    }

    return 0;
}

int AmpUT_VideoEnc_ChangeResolution(UINT32 modeIdx)
{
    AMP_VIN_RUNTIME_CFG_s VinCfg = {0};
    AMP_VIDEOENC_MAIN_CFG_s MainCfg[2] = {0};
    AMP_VIDEOENC_LAYER_DESC_s NewPriLayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
    AMP_VIDEOENC_LAYER_DESC_s NewSecLayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
    AMBA_SENSOR_MODE_INFO_s SensorModeInfo;
    AMP_VIN_LAYOUT_CFG_s Layout[2]; // Dualstream from same vin/vcapwindow
    AMBA_SENSOR_MODE_ID_u Mode = {0};

    EncModeIdx = modeIdx;

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Change Resolution to %s", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
    AmbaPrint(" =========================================================");

    // Force Liveview Process Mode
    {
        if (ExtLiveViewProcMode != 0xFF) {
            LiveViewProcMode = ExtLiveViewProcMode;
        #ifdef CONFIG_SOC_A12
            if (VideoEnc_EisAlgoEnable == 0) ExtLiveViewProcMode = 0xFF;
        #else
            ExtLiveViewProcMode = 0xFF;
        #endif
        }

        if (ExtLiveViewAlgoMode != 0xFF) {
            LiveViewAlgoMode = ExtLiveViewAlgoMode;
        #ifdef CONFIG_SOC_A12
            if (VideoEnc_EisAlgoEnable == 0) ExtLiveViewAlgoMode = 0xFF;
        #else
            ExtLiveViewAlgoMode = 0xFF;
        #endif

        }

        if (ExtLiveViewOSMode != 0xFF) {
            LiveViewOSMode = ExtLiveViewOSMode;
        #ifdef CONFIG_SOC_A12
            if (VideoEnc_EisAlgoEnable == 0) ExtLiveViewOSMode = 0xFF;
        #else
            ExtLiveViewOSMode = 0xFF;
        #endif
        } else {
            LiveViewOSMode = VideoEncMgt[EncModeIdx].OSMode;
        }
    }

    memset(&Layout, 0x0, 2*sizeof(AMP_VIN_LAYOUT_CFG_s));

    Mode.Data = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;;
    memset(&SensorModeInfo, 0x0, sizeof(AMBA_SENSOR_MODE_INFO_s));
    AmbaSensor_GetModeInfo(VinChannel, Mode, &SensorModeInfo);

#ifdef CONFIG_SOC_A9
    if (strcmp(MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem), "2560X1440  60P\0") == 0) {
        AmbaPrint("Turn OFF TV");
        TvLiveview = 0;
    } else
#endif
    {
        AmbaPrint("Turn %s Tv", TvLiveview? "ON": "OFF");
    }

    // Update VIN information
    VinCfg.Hdlr = VideoEncVinA;
    VinCfg.Mode = Mode;
    VinCfg.LayoutNumber = 2;
#ifdef CONFIG_SOC_A9
    VinCfg.HwCaptureWindow.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
    VinCfg.HwCaptureWindow.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
    VinCfg.HwCaptureWindow.X = SensorModeInfo.OutputInfo.RecordingPixels.StartX +
        (((SensorModeInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
    VinCfg.HwCaptureWindow.Y = (SensorModeInfo.OutputInfo.RecordingPixels.StartY +
        ((SensorModeInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;
#else
    if (VideoOBModeEnable && SensorModeInfo.OutputInfo.OpticalBlackPixels.Width && SensorModeInfo.OutputInfo.OpticalBlackPixels.Height) {
        INT16 ActStartX = MIN2(SensorModeInfo.OutputInfo.OpticalBlackPixels.StartX,SensorModeInfo.OutputInfo.RecordingPixels.StartX);
        INT16 ActStartY = MIN2(SensorModeInfo.OutputInfo.OpticalBlackPixels.StartY,SensorModeInfo.OutputInfo.RecordingPixels.StartY);

        VinCfg.HwCaptureWindow.Width = SensorModeInfo.OutputInfo.OutputWidth - ActStartX;
        VinCfg.HwCaptureWindow.Height = SensorModeInfo.OutputInfo.OutputHeight - ActStartY;
        VinCfg.HwCaptureWindow.X = ActStartX;
        VinCfg.HwCaptureWindow.Y = ActStartY;
        Layout[0].ActiveArea.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
        Layout[0].ActiveArea.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
        Layout[0].ActiveArea.X = SensorModeInfo.OutputInfo.RecordingPixels.StartX - ActStartX;
        Layout[0].ActiveArea.Y = SensorModeInfo.OutputInfo.RecordingPixels.StartY - ActStartY;
        Layout[0].OBArea.Width = SensorModeInfo.OutputInfo.OpticalBlackPixels.Width;
        Layout[0].OBArea.Height = SensorModeInfo.OutputInfo.OpticalBlackPixels.Height;
        Layout[0].OBArea.X = SensorModeInfo.OutputInfo.OpticalBlackPixels.StartX - ActStartX;
        Layout[0].OBArea.Y = SensorModeInfo.OutputInfo.OpticalBlackPixels.StartY - ActStartY;
        Layout[0].EnableOBArea = 1;
    } else {
        Layout[0].ActiveArea.Width = VinCfg.HwCaptureWindow.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
        Layout[0].ActiveArea.Height = VinCfg.HwCaptureWindow.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
        Layout[0].ActiveArea.X = VinCfg.HwCaptureWindow.X = SensorModeInfo.OutputInfo.RecordingPixels.StartX +
            (((SensorModeInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        Layout[0].ActiveArea.Y = VinCfg.HwCaptureWindow.Y = SensorModeInfo.OutputInfo.RecordingPixels.StartY + \
                (((SensorModeInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)&0xFFFE);
        Layout[0].EnableOBArea = 0;
    }
#endif
    Layout[0].Width = VideoEncMgt[EncModeIdx].MainWidth;
    Layout[0].Height = VideoEncMgt[EncModeIdx].MainHeight;

    Layout[0].EnableSourceArea = 0; // Get all capture window to main
    Layout[0].DzoomFactorX = InitZoomX;
    Layout[0].DzoomFactorY = InitZoomY;
    Layout[0].DzoomOffsetX = 0;
    Layout[0].DzoomOffsetY = 0;
    Layout[0].MainviewReportRate = (MainviewReportRate)? MainviewReportRate: VideoEncMgt[EncModeIdx].ReportRate;
    if (EncDualHDStream) {
        if (SecStreamCustomWidth) {
            Layout[1].Width = SecStreamCustomWidth;
            Layout[1].Height = SecStreamCustomHeight;
        } else {
            MWUT_InputSetDualHDWindow(VideoEncMgt, EncModeIdx, \
                &Layout[1].Width, &Layout[1].Height);
        }
    } else {
        if (SecStreamCustomWidth) {
            Layout[1].Width = SecStreamCustomWidth;
            Layout[1].Height = SecStreamCustomHeight;
        } else {
            Layout[1].Width = SEC_STREAM_WIDTH;
            Layout[1].Height = SEC_STREAM_HEIGHT;
        }
    }
    Layout[1].EnableSourceArea = 0; // Get all capture window to main
    Layout[1].DzoomFactorX = INIT_DZOOM_FACTOR;
    Layout[1].DzoomFactorY = INIT_DZOOM_FACTOR;
    Layout[1].DzoomOffsetX = 0;
    Layout[1].DzoomOffsetY = 0;
    Layout[1].MainviewReportRate = 1;
    VinCfg.Layout = &Layout[0];

    // Make sure Img scheduler has been created
    if (ImgSchdlr == NULL) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg;

        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

        ImgSchdlrCfg.MainViewID = 0; //single channle have one MainView
        ImgSchdlrCfg.Channel = VinChannel;
        ImgSchdlrCfg.Vin = VideoEncVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_VideoEncImgSchdlrCallback;
        if (LiveViewProcMode && LiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &ImgSchdlr);  // One sensor (not vin) need one scheduler.
    }

    // Update VideoEnc information
    {
        MainCfg[0].Hdlr = VideoEncPri;
        MainCfg[0].MainLayout.LayerNumber = 1;
        MainCfg[0].MainLayout.Layer = &NewPriLayer;
        MainCfg[0].MainLayout.Width = VideoEncMgt[EncModeIdx].MainWidth;
        MainCfg[0].MainLayout.Height = VideoEncMgt[EncModeIdx].MainHeight;
        MainCfg[0].Interlace = EncPriInteralce;
        MainCfg[0].MainTickPerPicture = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TickPerPicture: VideoEncMgt[EncModeIdx].TickPerPicturePAL;
        MainCfg[0].MainTimeScale = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;
        NewPriLayer.EnableSourceArea = 0;
        NewPriLayer.EnableTargetArea = 0;
        NewPriLayer.LayerId = 0;
        NewPriLayer.SourceType = AMP_ENC_SOURCE_VIN;
        NewPriLayer.Source = VideoEncVinA;
        NewPriLayer.SourceLayoutId = 0;
    }

    if (LiveViewProcMode == 0) {
        if (VideoEncMgt[EncModeIdx].MainWidth > 1920 || \
            VideoEncMgt[EncModeIdx].CaptureWidth > 1920) {
            if (LiveViewOSMode == 0) {
                AmbaPrint("[UT_Video]Force to use NonOS Mode?? (VcapW %d,MainW %d)", VideoEncMgt[EncModeIdx].MainWidth, VideoEncMgt[EncModeIdx].CaptureWidth);
            }
        }
    }

    MainCfg[0].LiveViewProcMode = LiveViewProcMode;
    MainCfg[0].LiveViewAlgoMode = LiveViewAlgoMode;
    MainCfg[0].LiveViewOSMode = LiveViewOSMode;
    if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
        MainCfg[0].LiveViewHdrMode = LiveViewHdrMode = 1;
    } else {
        MainCfg[0].LiveViewHdrMode = LiveViewHdrMode = 0;
    }

#ifdef CONFIG_SOC_A9
    MainCfg[0].SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[0].SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[0].SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
#else
    MainCfg[0].SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[0].SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
    MainCfg[0].SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
    MainCfg[0].SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
    MainCfg[0].SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
    MainCfg[0].SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
#endif

    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        AmpUT_VideoEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
        MainCfg[0].DspWorkBufAddr = dspWorkAddr;
        MainCfg[0].DspWorkBufSize = dspWorkSize;
    }

    MainCfg[1].Hdlr = VideoEncSec;
    MainCfg[1].MainLayout.LayerNumber = 1;
    MainCfg[1].MainLayout.Layer = &NewSecLayer;
    if (EncDualHDStream) {
        if (SecStreamCustomWidth) {
            MainCfg[1].MainLayout.Width = SecStreamCustomWidth;
            MainCfg[1].MainLayout.Height = SecStreamCustomHeight;
        } else {
            MWUT_InputSetDualHDWindow(VideoEncMgt, EncModeIdx, \
                &MainCfg[1].MainLayout.Width, &MainCfg[1].MainLayout.Height);
        }
    } else {
        if (SecStreamCustomWidth) {
            MainCfg[1].MainLayout.Width = SecStreamCustomWidth;
            MainCfg[1].MainLayout.Height = SecStreamCustomHeight;
        } else {
            MainCfg[1].MainLayout.Width = SEC_STREAM_WIDTH;
            MainCfg[1].MainLayout.Height = SEC_STREAM_HEIGHT;
        }
    }
    MainCfg[1].Interlace = EncSecInteralce;
    if (EncodeSystem == 0) {
        MainCfg[1].MainTickPerPicture = SEC_STREAM_TICK;
        MainCfg[1].MainTimeScale = SEC_STREAM_TIMESCALE;
    } else {
        MainCfg[1].MainTickPerPicture = SEC_STREAM_TICK;
        MainCfg[1].MainTimeScale = SEC_STREAM_TIMESCALE;
    }
#ifdef CONFIG_SOC_A9
    MainCfg[1].SysFreq.ArmCortexFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[1].SysFreq.IdspFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[1].SysFreq.CoreFreq = AMP_SYSTEM_FREQ_KEEPCURRENT;
#else
    MainCfg[1].SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[1].SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
    MainCfg[1].SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
    MainCfg[1].SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
    MainCfg[1].SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
    MainCfg[1].SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
#endif

    if (VideoEncMgt[EncModeIdx].DualValid == 0) {
        MainCfg[1].MainLayout.Width = 0;
        MainCfg[1].MainLayout.Height = 0;
        EncDualStream = 0;
        EncDualHDStream = 0;
        AmbaPrint(" ==> Disable dual stream");
    }

    if (LiveViewProcMode == 0) {
        if (VideoEncMgt[EncModeIdx].MainWidth > 1920 || \
            VideoEncMgt[EncModeIdx].CaptureWidth > 1920) {
            if (LiveViewOSMode == 0) {
                AmbaPrint("[UT_Video]Force to use NonOS Mode?? (VcapW %d,MainW %d)", VideoEncMgt[EncModeIdx].MainWidth, VideoEncMgt[EncModeIdx].CaptureWidth);
            }
        }
    }

    MainCfg[1].LiveViewProcMode = LiveViewProcMode;
    MainCfg[1].LiveViewAlgoMode = LiveViewAlgoMode;
    MainCfg[1].LiveViewOSMode = LiveViewOSMode;
    if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
        MainCfg[1].LiveViewHdrMode = LiveViewHdrMode = 1;
    } else {
        MainCfg[1].LiveViewHdrMode = LiveViewHdrMode = 0;
    }

    MainCfg[1].DspWorkBufAddr = 0;  // Don't want to change dsp buffer
    MainCfg[1].DspWorkBufSize = 0;  // Don't want to change dsp buffer
    NewSecLayer.EnableSourceArea = 0;
    NewSecLayer.EnableTargetArea = 0;
    NewSecLayer.LayerId = 0;
    NewSecLayer.SourceType = AMP_ENC_SOURCE_VIN;
    NewSecLayer.Source = VideoEncVinA;
    NewSecLayer.SourceLayoutId = 1;

    // Update capture window to Dzoom
    {
        AMP_IMG_DZOOM_VIN_INVALID_INFO_s dzoomVinInvalidInfo;
        dzoomVinInvalidInfo.CapW = VideoEncMgt[EncModeIdx].CaptureWidth;
        dzoomVinInvalidInfo.CapH = VideoEncMgt[EncModeIdx].CaptureHeight;

        AmpImgDzoom_StopDzoom(dzoomHdlr);
        AmpImgDzoom_ResetStatus(dzoomHdlr);
        AmpImgDzoom_ChangeResolutionHandler(dzoomHdlr, &dzoomVinInvalidInfo);
    }

    // Remember new frame/field rate for muxer
    if (EncodeSystem == 0) {
        EncFrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;
    } else {
        EncFrameRate = VideoEncMgt[EncModeIdx].TimeScalePAL/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
    }

    // Slow Shutter
    {
        AE_CONTROL_s AeCtrlMode;
        AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
        UINT32 MinFrameTime;

        AmbaSensor_GetStatus(VinChannel, &SensorStatus);
        MinFrameTime = SensorStatus.ModeInfo.MinFrameRate.TimeScale/SensorStatus.ModeInfo.MinFrameRate.NumUnitsInTick;

        AmbaImg_Proc_Cmd(MW_IP_GET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);
        if (SlowShutterEnable && (EncFrameRate >= 50)) {
            UINT8 MaxSSRatio;
            MaxSSRatio = EncFrameRate/MinFrameTime;
            if (MaxSSRatio>=4) {
                AeCtrlMode.SlowShutter = 2;
            } else if (MaxSSRatio>=2){
                AeCtrlMode.SlowShutter = 1;
            } else {
                AeCtrlMode.SlowShutter = 0;
            }
        } else {
            AeCtrlMode.SlowShutter = 0;
        }
        AmbaImg_Proc_Cmd(MW_IP_SET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);
    }
    AmpVideoEnc_ConfigVinMain(1, &VinCfg, 2, &MainCfg[0]);
    if (EncDateTimeStampPri) {
        AMP_VIDEOENC_BLEND_INFO_s BlendInfo = {0};
        BlendInfo.Enable = 1;
        BlendInfo.BufferID = 0;  // Unit Test only use one blend area, we choose ID = 0
        BlendInfo.OffsetX = 200;
        BlendInfo.OffsetY = 200;
        BlendInfo.Pitch = 256;
        BlendInfo.Width = 256;
        BlendInfo.Height = 100;
        BlendInfo.YAddr = EncBlendY;
        BlendInfo.UVAddr = EncBlendUV;
        BlendInfo.AlphaYAddr = EncBlendAlphaY;
        BlendInfo.AlphaUVAddr = EncBlendAlphaUV;

        AmpVideoEnc_SetEncodeBlend(VideoEncPri, &BlendInfo);
    }
    if (EncDateTimeStampSec) {
        AMP_VIDEOENC_BLEND_INFO_s BlendInfo = {0};
        BlendInfo.Enable = 1;
        BlendInfo.BufferID = 0;  // Unit Test only use one blend area, we choose ID = 0
        BlendInfo.OffsetX = 200;
        BlendInfo.OffsetY = 200;
        BlendInfo.Pitch = 256;
        BlendInfo.Width = 256;
        BlendInfo.Height = 100;
        BlendInfo.YAddr = EncBlendY;
        BlendInfo.UVAddr = EncBlendUV;
        BlendInfo.AlphaYAddr = EncBlendAlphaY;
        BlendInfo.AlphaUVAddr = EncBlendAlphaUV;

        AmpVideoEnc_SetEncodeBlend(VideoEncSec, &BlendInfo);
    }
    return 0;
}

/**
 * UnitTest: Calculate dsp working address and size base on current capture mode
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoEnc_DspWork_Calculate(UINT8 **addr, UINT32 *size)
{
    extern UINT8 *DspWorkAreaResvStart;
    extern UINT32 DspWorkAreaResvSize;

    if (CaptureMode == CAPTURE_MODE_NONE) {
        //only allocate MJPEG Bits
        (*addr) = DspWorkAreaResvStart;
        (*size) = DspWorkAreaResvSize - 15*1024*1024;
    }

    if (VideoEncCusomtDspWorkSize) {
        *size = VideoEncCusomtDspWorkSize<<20;
    }
    AmbaPrint("[DspWork_Calculate] Addr 0x%X, Sz %u", *addr, *size);
    return 0;
}

void AmpUT_VideoEnc_JpegMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc;
    int Er;
#ifdef CONFIG_SOC_A12
    // A12 express PIV will use H264 BS
    UINT8 *BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
#else
    UINT8 *BitsLimit = JpegBitsBuf + BISFIFO_STILL_SIZE - 1;
#endif

    UINT8 dumpSkip = 0;

    AmbaPrint("AmpUT_VideoEnc_JpegMuxTask Start");

    while (1) {
        AmbaKAL_SemTake(&VideoEncJpegSem, AMBA_KAL_WAIT_FOREVER);

        Er = AmpFifo_PeekEntry(VirtualJpegFifoHdlr, &Desc, 0);
        if (Er == 0) {
            AmbaPrint("Muxer PeekEntry: size:%5d@0x%08X Ft(%d) Seg(%u)", Desc->Size, Desc->StartAddr, Desc->Type, Desc->SeqNum);
        } else {
            while (Er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(VirtualJpegFifoHdlr, &Desc, 0);
            }
        }

        dumpSkip = 0;
#define AMPUT_FILE_DUMP
        #ifdef AMPUT_FILE_DUMP
        {
            char Fn[32];
            char mdASCII[3] = {'w','+','\0'};
            if (PivProcess) {
                if (Desc->Type == AMP_FIFO_TYPE_JPEG_FRAME) {
                    fnoPiv++;
                    sprintf(Fn,"%s:\\%04d_m_%02d.jpg", DefaultSlot, fnoPri, fnoPiv);
                } else if (Desc->Type == AMP_FIFO_TYPE_THUMBNAIL_FRAME) {
                    fnoThmPiv++;
                    sprintf(Fn,"%s:\\%04d_t_%02d.jpg", DefaultSlot, fnoPri, fnoThmPiv);
                } else if (Desc->Type == AMP_FIFO_TYPE_SCREENNAIL_FRAME) {
                    fnoScrnPiv++;
                    sprintf(Fn,"%s:\\%04d_s_%02d.jpg", DefaultSlot, fnoPri, fnoScrnPiv);
                }
            } else if (PivVideoThm) {
                if (Desc->Type == AMP_FIFO_TYPE_JPEG_FRAME || \
                    Desc->Type == AMP_FIFO_TYPE_SCREENNAIL_FRAME) {
                    // do not dump
                    dumpSkip = 1;
                    PivVideoThmJpgCount--;
                } else if (Desc->Type == AMP_FIFO_TYPE_THUMBNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d.thm", DefaultSlot, fnoPri);
                    PivVideoThmJpgCount--;
                }
                if (PivVideoThmJpgCount == 0) {
                    PivVideoThm = 0;
                }
            } else {
                if (Desc->Type == AMP_FIFO_TYPE_JPEG_FRAME) {
                    sprintf(Fn,"%s:\\%04d_m.jpg", DefaultSlot, fnoPri);
                } else if (Desc->Type == AMP_FIFO_TYPE_THUMBNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d_t.jpg", DefaultSlot, fnoPri);
                } else if (Desc->Type == AMP_FIFO_TYPE_SCREENNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d_s.jpg", DefaultSlot, fnoPri);
                }
            }

            if (dumpSkip == 0) {
                outputJpegFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                AmbaPrint("%s opened", Fn);
            }

        #ifdef CONFIG_SOC_A12
            // A12 express PIV will use H264 BS
            BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
        #else
            BitsLimit = JpegBitsBuf + BISFIFO_STILL_SIZE - 1;
        #endif
        }
        #endif

        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            // EOS, stillenc do not prodice EOS anymore...
            #ifdef AMPUT_FILE_DUMP
                if (dumpSkip == 0) {
                    UT_VideoEncodefsync((void *)outputJpegFile);
                    UT_VideoEncodefclose((void *)outputJpegFile);
                    outputJpegFile = NULL;
                }
            #endif
            AmbaPrint("Muxer met EOS");
        } else {
            #ifdef AMPUT_FILE_DUMP
                {
                    if (dumpSkip == 0) {
                        AmbaPrint("Write: 0x%x sz %d limit %X",  Desc->StartAddr, Desc->Size, BitsLimit);
                        if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                            UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputJpegFile);
                        }else{
                            UT_VideoEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputJpegFile);
                        #ifdef CONFIG_SOC_A12
                            // A12 express PIV will use H264 BS
                            UT_VideoEncodefwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputJpegFile);
                        #else
                            UT_VideoEncodefwrite((const void *)JpegBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputJpegFile);
                        #endif

                        }
                        UT_VideoEncodefsync((void *)outputJpegFile);
                        UT_VideoEncodefclose((void *)outputJpegFile);
                    }
                    if ((PivVideoThm==0)&&(fnoPiv==fnoThmPiv)&&(fnoPiv==fnoScrnPiv)&&(fnoScrnPiv==fnoThmPiv)) {
                        if (PivProcess) {
                            PivProcess--;
                        }

                        if (s_thmBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_thmBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail thm!");
                            }
                            s_thmBuffAddr = NULL;
                        }

                        if (s_scrnBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_scrnBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail scrn!");
                            }
                            s_scrnBuffAddr = NULL;
                        }

                        if (s_yuvBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_yuvBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail yuv!");
                            }
                            s_yuvBuffAddr = NULL;
                        }

                        if (s_qvLCDBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_qvLCDBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail qvLCD!");
                            }
                            s_qvLCDBuffAddr = NULL;
                        }

                        if (s_qvHDMIBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_qvHDMIBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail qvHDMI!");
                            }
                            s_qvHDMIBuffAddr = NULL;
                        }
                    }
                }
            #else
                {
                    static UINT8 i = 0;
                    i++;
                    i %= 3;
                    if (i == 0) {
                        if (PivProcess) {
                            PivProcess--;
                        }

                        if (s_thmBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_thmBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail thm!");
                            }
                            s_thmBuffAddr = NULL;
                        }

                        if (s_scrnBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_scrnBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail scrn!");
                            }
                            s_scrnBuffAddr = NULL;
                        }

                        if (s_yuvBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_yuvBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail yuv!");
                            }
                            s_yuvBuffAddr = NULL;
                        }

                        if (s_qvLCDBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_qvLCDBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail qvLCD!");
                            }
                            s_qvLCDBuffAddr = NULL;
                        }

                        if (s_qvHDMIBuffAddr) {
                            if (AmbaKAL_BytePoolFree((void *)Ori_s_qvHDMIBuffAddr) != OK) {
                                AmbaPrint("[Amp_MUX] memFree Fail qvHDMI!");
                            }
                            s_qvHDMIBuffAddr = NULL;
                        }
                    }
                }
                AmbaKAL_TaskSleep(1);
            #endif
        }
        AmpFifo_RemoveEntry(VirtualJpegFifoHdlr, 1);
    }
}

/**
 * UnitTest: Video Encode PIV PostProc Callback
 *
 * @param [in] info postproce information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoEnc_PivPOSTCB(AMP_STILLENC_POSTP_INFO_s *info)
{
    static UINT8 yuvFlag = 0;

    if (info->StageCnt == 1) {
        char Fn[64];
        char mdASCII[3] = {'w','+','\0'};
        UINT8 dumpSkip = 0;

        yuvFlag = 0;

        if (1/* mode_0 PIV */) {
            //mode_0 PIV can not access raw
            dumpSkip = 1;
        }

        if (dumpSkip == 0) {
            //raw ready, dump it
            sprintf(Fn,"%s:\\%04d.RAW", DefaultSlot, fnoRaw);
            stillRawFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
            AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d %d  to %s Start!", \
                info->media.RawInfo.RawAddr, \
                info->media.RawInfo.RawPitch, \
                info->media.RawInfo.RawWidth, \
                info->media.RawInfo.RawHeight, Fn);
            UT_VideoEncodefwrite((const void *)info->media.RawInfo.RawAddr, 1, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight, (void *)stillRawFile);
            UT_VideoEncodefsync((void *)stillRawFile);
            UT_VideoEncodefclose((void *)stillRawFile);
        }
        fnoRaw++;
    } else if (info->StageCnt == 2) {
        char Fn[32];
        char fn1[32];
        UINT8 *LumaAddr, *ChromaAddr;
        UINT16 Pitch, Width, Height;
        char mdASCII[3] = {'w','+','\0'};

        UINT8 dumpSkip = 0;

        if (1/* mode_0 PIV */) {
            //mode_0 PIV can not access yuv
            dumpSkip = 1;
        }

        if (dumpSkip == 0) {
            if (info->media.YuvInfo.ThmLumaAddr) {
                sprintf(Fn,"%s:\\%04d_t.y", DefaultSlot, fnoYuv);
                sprintf(fn1,"%s:\\%04d_t.uv", DefaultSlot, fnoYuv);
                LumaAddr = info->media.YuvInfo.ThmLumaAddr;
                ChromaAddr = info->media.YuvInfo.ThmChromaAddr;
                Pitch = info->media.YuvInfo.ThmPitch;
                Width = info->media.YuvInfo.ThmWidth;
                Height = info->media.YuvInfo.ThmHeight;
            } else if (info->media.YuvInfo.ScrnLumaAddr) {
                sprintf(Fn,"%s:\\%04d_s.y", DefaultSlot, fnoYuv);
                sprintf(fn1,"%s:\\%04d_s.uv", DefaultSlot, fnoYuv);
                LumaAddr = info->media.YuvInfo.ScrnLumaAddr;
                ChromaAddr = info->media.YuvInfo.ScrnChromaAddr;
                Pitch = info->media.YuvInfo.ScrnPitch;
                Width = info->media.YuvInfo.ScrnWidth;
                Height = info->media.YuvInfo.ScrnHeight;
            } else if (info->media.YuvInfo.LumaAddr) {
                sprintf(Fn,"%s:\\%04d_m.y", DefaultSlot, fnoYuv);
                sprintf(fn1,"%s:\\%04d_m.uv", DefaultSlot, fnoYuv);
                LumaAddr = info->media.YuvInfo.LumaAddr;
                ChromaAddr = info->media.YuvInfo.ChromaAddr;
                Pitch = info->media.YuvInfo.Pitch;
                Width = info->media.YuvInfo.Width;
                Height = info->media.YuvInfo.Height;
            }

            AmbaPrint("[Amp_UT] Dump YUV(%d) (0x%X 0x%X) %d %d %d Start!", \
                info->media.YuvInfo.DataFormat, \
                LumaAddr, ChromaAddr, Pitch, Width, Height);

            stillYFile = UT_VideoEncodefopen((const char *)Fn, (const char *)mdASCII,1);
            UT_VideoEncodefwrite((const void *)LumaAddr, 1, Pitch*Height, (void *)stillYFile);
            UT_VideoEncodefsync((void *)stillYFile);
            UT_VideoEncodefclose((void *)stillYFile);

            stillUvFile = UT_VideoEncodefopen((const char *)fn1, (const char *)mdASCII,1);
            UT_VideoEncodefwrite((const void *)ChromaAddr, 1, Pitch*Height, (void *)stillUvFile);
            UT_VideoEncodefsync((void *)stillUvFile);
            UT_VideoEncodefclose((void *)stillUvFile);
            //AmbaPrint("[Amp_UT] Dump YUV Done!");

            //release yuv buffers
            if (info->media.YuvInfo.ThmLumaAddr) {
                yuvFlag |= 0x4;
            } else if (info->media.YuvInfo.ScrnLumaAddr) {
                yuvFlag |= 0x2;
            } else if (info->media.YuvInfo.LumaAddr) {
                yuvFlag |= 0x1;
            }
        } else {
            yuvFlag |= 0x7;
        }
        if (yuvFlag == 0x7) {
            //release raw buffers
            if (s_rawBuffAddr) {
                if (AmbaKAL_BytePoolFree((void *)Ori_s_rawBuffAddr) != OK) {
                    AmbaPrint("[Amp_UT] memFree Fail raw!");
                }
                s_rawBuffAddr = NULL;
            }

            fnoYuv++;
        }
    } else {
        //nothing to do
    }

    return 0;
}

extern void AmpUT_TuneGetItunerMode(UINT8 *ItunerMode);

/**
 * UnitTest: Video Encode PIV PreProc Callback
 *
 * @param [in] info preproc information
 *
 * @return 0 - success, -1 - fail
 */
UINT32 AmpUT_VideoEnc_PivPRECB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 3) {
        UINT8 ItunerMode = 0;
#ifdef CONFIG_SOC_A12 // Temporary disable for A9
        AmpUT_TuneGetItunerMode(&ItunerMode);
#endif
        if (ItunerMode == 0/*ITUNER_BASIC_TUNING_MODE*/) {
#ifdef CONFIG_SOC_A12
            /* set still idsp param */
            if (StillIso != 2 && VideoEnc3AEnable == 1) { // comes from AmbaSample_AdjPivControl()
                AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
                ADJ_STILL_CONTROL_s AdjPivCtrl = {0};
                float BaseStillBlcShtTime = 60.0;
                UINT16 ShutterIndex = 0;
                AMBA_AE_INFO_s VideoAeInfo = {0};
                AMBA_DSP_IMG_WB_GAIN_s VideoWbGain = {WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN,WB_UNIT_GAIN};
                AMBA_DSP_IMG_SENSOR_INFO_s SensorInfo = {0};
                AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};

                ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
                ImgMode.AlgoMode = (StillIso==1)? AMBA_DSP_IMG_ALGO_MODE_LISO: AMBA_DSP_IMG_ALGO_MODE_HISO;
                ImgMode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_PIV;
                ImgMode.ContextId = 0; //TBD
                if (LiveViewProcMode == 0 || (LiveViewProcMode == 1 && LiveViewAlgoMode == 0)) {
                    ImgMode.BatchId = AMBA_DSP_STILL_LISO_FILTER;
                }

                /* Run Adj compute */
                AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, 0/*TBD*/, IP_MODE_VIDEO, (UINT32)&VideoAeInfo);
                if (LiveViewHdrMode) {
                    // HDR mode use UnitGain
                } else {
                    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, 0/*TBD*/, IP_MODE_VIDEO, (UINT32)&VideoWbGain);
                }
                ShutterIndex = (UINT16)(log2(BaseStillBlcShtTime/VideoAeInfo.ShutterTime) * 128);
                AdjPivCtrl.StillMode = (StillIso==1)? IP_MODE_LISO_STILL: IP_MODE_HISO_STILL;
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

                if (LiveViewProcMode == 1 && LiveViewAlgoMode != 0) {
                    AmbaDSP_ImgSetWbGain(&ImgMode, &VideoWbGain);
                    AmbaSensor_GetStatus(VinChannel, &SensorStatus);
                    SensorInfo.SensorPattern = (UINT8)SensorStatus.ModeInfo.OutputInfo.CfaPattern;
                    SensorInfo.SensorResolution = (UINT8)SensorStatus.ModeInfo.OutputInfo.NumDataBits;
                    AmbaDSP_ImgSetVinSensorInfo(&ImgMode, &SensorInfo);
                } else if (LiveViewProcMode == 0) {
                    //set WB gain since STILL_PIPE_CTRL do not issue it into batch
                    AmbaDSP_ImgSetWbGain(&ImgMode, &VideoWbGain);
                }
                if (VideoOBModeEnable) {
                    AMBA_SENSOR_MODE_INFO_s SensorModeInfo;
                    AMBA_SENSOR_MODE_ID_u Mode = {0};
                    AMBA_DSP_IMG_WIN_GEOMETRY_s VinActiveWindow = {0};

                    Mode.Data = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;
                    memset(&SensorModeInfo, 0x0, sizeof(AMBA_SENSOR_MODE_INFO_s));
                    AmbaSensor_GetModeInfo(VinChannel, Mode, &SensorModeInfo);

                    if(SensorModeInfo.OutputInfo.OpticalBlackPixels.Width && SensorModeInfo.OutputInfo.OpticalBlackPixels.Height) {
                        INT16 ActStartX = MIN2(SensorModeInfo.OutputInfo.OpticalBlackPixels.StartX,SensorModeInfo.OutputInfo.RecordingPixels.StartX);
                        INT16 ActStartY = MIN2(SensorModeInfo.OutputInfo.OpticalBlackPixels.StartY,SensorModeInfo.OutputInfo.RecordingPixels.StartY);

                        VinActiveWindow.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
                        VinActiveWindow.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
                        VinActiveWindow.StartX = (SensorModeInfo.OutputInfo.RecordingPixels.StartX - ActStartX) + \
                            (((SensorModeInfo.OutputInfo.RecordingPixels.Width - VinActiveWindow.Width)/2)&0xFFF8);
                        VinActiveWindow.StartY = (SensorModeInfo.OutputInfo.RecordingPixels.StartY - ActStartY) + \
                            (((SensorModeInfo.OutputInfo.RecordingPixels.Height - VinActiveWindow.Height)/2)&0xFFFE);

                        AmbaDSP_WarpCore_SetVinActiveWin(&ImgMode, &VinActiveWindow);
                    }
                }
           }
#else
#if 0
            /* set still idsp param */
            {
#if 0 //after still ADJ enable
                extern void Amba_Img_Set_Still_Pipe_Ctrl_Params(UINT32 chNo, AMBA_DSP_IMG_MODE_CFG_s mode);
                Amba_Img_set_still_pipe_ctrl_params();
#else
                extern int AmpUT_IsoConfigSet(AMBA_DSP_IMG_ALGO_MODE_e isoMode);
                if (StillIso == 1) {
                    AmpUT_IsoConfigSet(AMBA_DSP_IMG_ALGO_MODE_LISO);
                } else if (StillIso == 0) {
                    AmpUT_IsoConfigSet(AMBA_DSP_IMG_ALGO_MODE_HISO);
                }
#endif
            }

            // Only Mode5 need to take care warp/dzoom
            if (AmpResource_GetDspBackgroundProcMode() && StillIso != 2) {
                /* calc warp and execute ISO config */
                AMBA_SENSOR_STATUS_INFO_s SensorStatus;
                AMBA_SENSOR_AREA_INFO_s *recPixel = &SensorStatus.ModeInfo.OutputInfo.RecordingPixels;
                AMBA_SENSOR_INPUT_INFO_s *inputInfo = &SensorStatus.ModeInfo.InputInfo;
                AMBA_DSP_IMG_MODE_CFG_s ImgMode;
                AMBA_DSP_IMG_SIZE_INFO_s SizeInfo;
                AMBA_DSP_IMG_WARP_CALC_INFO_s CalcWarp = {0};
                double ZoomRatio;
                UINT16 capW, capH, encW, encH;
                AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s vinGeo;
                AMBA_DSP_IMG_OUT_WIN_INFO_s        ImgOutputWin;
                AMBA_DSP_IMG_SENSOR_FRAME_TIMING_s ImgSensorFrameTiming;
                AMBA_DSP_IMG_DZOOM_INFO_s          ImgDzoom;
                UINT16 offsetX = 0;

                memset(&ImgMode, 0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
                ImgMode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
                if (StillIso == 0) {
                    ImgMode.AlgoMode  = AMBA_DSP_IMG_ALGO_MODE_HISO;
                    ImgMode.BatchId   = AMBA_DSP_STILL_HISO_FILTER;
                } else if (StillIso == 1) {
                    ImgMode.AlgoMode  = AMBA_DSP_IMG_ALGO_MODE_LISO;
                    ImgMode.BatchId   = AMBA_DSP_STILL_LISO_FILTER;
                } else {
                    ImgMode.AlgoMode  = AMBA_DSP_IMG_ALGO_MODE_FAST;
                    ImgMode.BatchId   = AMBA_DSP_VIDEO_FILTER;
                }
                ImgMode.ContextId = 0;
                ImgMode.ConfigId  = 0;

                /* Dzoom */
                capW = ctrl->VcapWidth;
                capH = ctrl->VcapHeight;
                encW = PivMainWidth;
                encH = PivMainHeight;

                AmbaSensor_GetStatus(ctrl->VinChan, &SensorStatus);

                memset(&vinGeo, 0, sizeof(vinGeo));
                offsetX = ((recPixel->StartX + (((recPixel->Width - capW)/2)&0xFFF8) & 0xFFFE) - recPixel->StartX)* \
                            inputInfo->HSubsample.FactorDen/inputInfo->HSubsample.FactorNum;
                vinGeo.StartX = inputInfo->PhotodiodeArray.StartX + offsetX;
                vinGeo.StartY = inputInfo->PhotodiodeArray.StartX + \
                    (((recPixel->StartY + ((recPixel->Height - capH)/2)) & 0xFFFE) - recPixel->StartY)* \
                            inputInfo->VSubsample.FactorDen/inputInfo->VSubsample.FactorNum;
                vinGeo.Width  = capW;
                vinGeo.Height = capH;
                vinGeo.HSubSample.FactorNum = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorNum;
                vinGeo.HSubSample.FactorDen = SensorStatus.ModeInfo.InputInfo.HSubsample.FactorDen;
                vinGeo.VSubSample.FactorNum = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorNum;
                vinGeo.VSubSample.FactorDen = SensorStatus.ModeInfo.InputInfo.VSubsample.FactorDen;
                AmbaDSP_WarpCore_SetVinSensorGeo(&ImgMode, &vinGeo);
                AmbaPrint("[PIV rawcap] Geo(%d %d)(%d %d)", vinGeo.StartX, vinGeo.StartY, vinGeo.Width, vinGeo.Height);

                memset(&ImgSensorFrameTiming, 0, sizeof(ImgSensorFrameTiming));
                ImgSensorFrameTiming.TimeScale        = SensorStatus.ModeInfo.FrameTime.FrameRate.TimeScale;
                ImgSensorFrameTiming.NumUnitsInTick   = SensorStatus.ModeInfo.FrameTime.FrameRate.NumUnitsInTick;
                ImgSensorFrameTiming.FrameLengthLines = SensorStatus.ModeInfo.FrameLengthLines;
                AmbaDSP_WarpCore_SetSensorFrameTiming(&ImgMode, &ImgSensorFrameTiming);

                memset(&ImgOutputWin, 0, sizeof(ImgOutputWin));
                ImgOutputWin.MainWinDim.Width  = encW;
                ImgOutputWin.MainWinDim.Height = encH;
#ifdef CONFIG_SOC_A12
                ImgOutputWin.ScreennailDim.Width  = ScrnWidthAct;
                ImgOutputWin.ScreennailDim.Height = ScrnHeightAct;
                ImgOutputWin.ThumbnailDim.Width  = ThmWidthAct;
                ImgOutputWin.ThumbnailDim.Height = ThmHeightAct;
                ImgOutputWin.PrevWinDim[0].Width  = QvLCDW;
                ImgOutputWin.PrevWinDim[0].Height = QvLCDH;
                ImgOutputWin.PrevWinDim[1].Width  = QvHDMIW;
                ImgOutputWin.PrevWinDim[1].Height = QvHDMIH;
#endif
                AmbaDSP_WarpCore_SetOutputWin(&ImgMode, &ImgOutputWin);
                AmbaPrint("[PIV rawcap] OutputWin(%d %d)", encW, encH);

                /* Dzoom don't care, TBD */
                ZoomRatio = (double) 100 / 100;

                memset(&ImgDzoom, 0, sizeof(ImgDzoom));
                ImgDzoom.ZoomX = (UINT32)(ZoomRatio * 65536);
                ImgDzoom.ZoomY = (UINT32)(ZoomRatio * 65536);
                AmbaDSP_WarpCore_SetDzoomFactor(&ImgMode, &ImgDzoom);

                AmbaDSP_WarpCore_CalcDspWarp(&ImgMode, 0);
                AmbaDSP_WarpCore_CalcDspCawarp(&ImgMode, 0);

                AmbaDSP_WarpCore_SetDspWarp(&ImgMode);
                AmbaDSP_WarpCore_SetDspCawarp(&ImgMode);

                memset(&CalcWarp, 0, sizeof(CalcWarp));
                if (AmbaDSP_ImgGetWarpCompensation(&ImgMode, &CalcWarp) != OK) {
                    AmbaPrintColor(RED, "[PIV_RawCapCB] Get Warp Compensation fail!!");
                }

                memset(&SizeInfo, 0, sizeof(SizeInfo));
                SizeInfo.WidthIn     = ((CalcWarp.ActWinCrop.RightBotX - CalcWarp.ActWinCrop.LeftTopX + 0xFFFF)>>16);
                SizeInfo.HeightIn    = ((CalcWarp.ActWinCrop.RightBotY - CalcWarp.ActWinCrop.LeftTopY + 0xFFFF)>>16);
                SizeInfo.WidthMain   = PivMainWidth;
                SizeInfo.HeightMain  = PivMainHeight;
                SizeInfo.WidthPrevA = QvLCDW;
                SizeInfo.HeightPrevA = QvLCDH;
                SizeInfo.WidthPrevB = QvHDMIW;
                SizeInfo.HeightPrevB = QvHDMIH;

                AmbaDSP_ImgSetSizeInfo(&ImgMode, &SizeInfo);
                AmbaDSP_ImgPostExeCfg(&ImgMode, (AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e)0);
                }
#endif
#endif
        } else if (ItunerMode == 1/*ITUENR_PIV_VERIFIED_MODE*/) {
            AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
            AMBA_ITUNER_PROC_INFO_s ProcInfo = {0};
            AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
            static ITUNER_SYSTEM_s System; // prevent stack overflow
            memset(&System, 0x0, sizeof(ITUNER_SYSTEM_s));
            memset(&ImgMode, 0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
            memset(&SensorStatus, 0, sizeof(AMBA_SENSOR_STATUS_INFO_s));

            AmbaSensor_GetStatus(VinChannel, &SensorStatus);
            AmbaItuner_Get_SystemInfo(&System);
            System.RawStartX = 0;
            System.RawStartY = 0;
            System.RawWidth = PivCaptureWidth;
            System.RawHeight = PivCaptureHeight;
            System.MainWidth = PivMainWidth;
            System.MainHeight = PivMainHeight;
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

/* CB for PIV raw capture */
UINT32 AmpUT_VideoEnc_PIV_RawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl)
{
    AMP_STILLENC_RAWCAP_DSP_CTRL_s DspCtrl = {0};
    DspCtrl.VidSkip = 0;
    DspCtrl.RawCapNum = ctrl->TotalRawToCap;
    DspCtrl.StillProc = StillIso;
    AmpStillEnc_StartRawCapture(StillEncPri, &DspCtrl);

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
int AmpUT_VideoEnc_PIV(AMP_VIDEOENC_PIV_CTRL_s pivCtrl, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop)
{
    int Er = 0;
    void *TempPtr = NULL;
    void *TmpbufRaw = NULL;
    UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
    UINT16 YuvWidth = 0, YuvHeight = 0, ScrnW = 0, ScrnH = 0, ThmW = 0, ThmH = 0;
    UINT32 RawSize = 0, YuvSize = 0, ScrnSize = 0, ThmSize = 0;
    UINT32 QvLCDSize = 0, QvHDMISize = 0;
    UINT8 *StageAddr = NULL;
    UINT32 TotalScriptSize = 0, TotalStageNum = 0;
    AMP_SENC_SCRPT_GENCFG_s *GenScrpt = NULL;
    AMP_SENC_SCRPT_RAWCAP_s *RawCapScrpt = NULL;
    AMP_SENC_SCRPT_RAW2YUV_s *Raw2YvuScrpt = NULL;
    AMP_SENC_SCRPT_YUV2JPG_s *Yuv2JpgScrpt = NULL;
    AMP_SCRPT_CONFIG_s Scrpt = {0};
    AMBA_SENSOR_MODE_INFO_s SensorInfo = {0};
    UINT16 ScrnWidth = 0, ScrnHeight = 0, ThmWidth = 0, ThmHeight = 0;
    UINT8 DspBackgroundProcMode = AmpResource_GetDspBackgroundProcMode();

    StillIso = iso;

    /* Pre-Phase */
    /* Error handle */
#ifdef CONFIG_SOC_A9
    if (DspBackgroundProcMode == 0 && iso != 2) {
        AmbaPrint("Can not Support ISO-PIV @ Mode0!!");
        return NG;
    }
#else
    if (LiveViewProcMode == 0 && iso == 0) {
        AmbaPrint("Can not Support HISO-PIV @ ExpressMode!!");
        return NG;
    }
#endif

    /* Phase I */
    /* check still codec Status */
    if (StillCodecInit == 0) {
        // Create simple muxer semophore
        if (AmbaKAL_SemCreate(&VideoEncJpegSem, 0) != OK) {
            AmbaPrint("StillEnc UnitTest: Semaphore creation failed");
        }

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoEncJpegMuxStack, (void **)&TempPtr, VIDEO_ENC_JPG_MUX_TASK_STACK_SIZE, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for Jpeg muxer stack!!");
        }
        // Create simple muxer task
        if (AmbaKAL_TaskCreate(&VideoEncJpegMuxTask, "Video Encoder UnitTest Jpeg Muxing Task", 10, \
             AmpUT_VideoEnc_JpegMuxTask, 0x0, VideoEncJpegMuxStack, VIDEO_ENC_JPG_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
             AmbaPrint("jpeg Muxer task creation failed");
        }

        // Create Still encoder object
        {
            AMP_STILLENC_INIT_CFG_s EncInitCfg = {0};

            // Init STILLENC module
            AmpStillEnc_GetInitDefaultCfg(&EncInitCfg);
            if (StillEncWorkBuf == NULL) {
                Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillEncWorkBuf, (void **)&TempPtr, EncInitCfg.MemoryPoolSize, 32);
                if (Er != OK) {
                    AmbaPrint("Out of memory for stillmain!!");
                }
            }
            EncInitCfg.MemoryPoolAddr = StillEncWorkBuf;

            AmpStillEnc_Init(&EncInitCfg);
        }
        StillCodecInit = 1;
    }

    if (StillEncPri == NULL) { //no codec be create
        AMP_STILLENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s Elayer = {0};

        EncCfg.MainLayout.Layer = &Elayer;
        AmpStillEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_VideoEncCallback;

        // Assign bitstream/descriptor buffer
    #ifdef CONFIG_SOC_A12
        // A12 express PIV will use H264 BS
        EncCfg.BitsBufCfg.BitsBufAddr = H264BitsBuf;
        EncCfg.BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
        EncCfg.BitsBufCfg.DescBufAddr = H264DescBuf;
        EncCfg.BitsBufCfg.DescBufSize = DESC_SIZE;
        EncCfg.BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
        AmpStillEnc_Create(&EncCfg, &StillEncPri);
    #else
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&JpegBitsBuf, (void **)&TempPtr, BISFIFO_STILL_SIZE, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for Jpeg BitsFifo!!");
        }
        EncCfg.BitsBufCfg.BitsBufAddr = JpegBitsBuf;
        EncCfg.BitsBufCfg.BitsBufSize = BISFIFO_STILL_SIZE;

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&JpegDescBuf, (void **)&TempPtr, DESC_SIZE, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for Jpeg DescFifo!!");
        }
        EncCfg.BitsBufCfg.DescBufAddr = JpegDescBuf;
        EncCfg.BitsBufCfg.DescBufSize = DESC_SIZE;
        EncCfg.BitsBufCfg.BitsRunoutThreshold = BISFIFO_STILL_SIZE - 4*1024*1024; // leave 4MB
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", JpegBitsBuf, BISFIFO_STILL_SIZE, JpegDescBuf, DESC_SIZE);
        AmpStillEnc_Create(&EncCfg, &StillEncPri);
    #endif

        // create a virtual fifo
        if (VirtualJpegFifoHdlr == NULL) {
            AMP_FIFO_CFG_s FifoDefCfg = {0};

            AmpFifo_GetDefaultCfg(&FifoDefCfg);
            FifoDefCfg.hCodec = StillEncPri;
            FifoDefCfg.IsVirtual = 1;
            FifoDefCfg.NumEntries = 1024;
            FifoDefCfg.cbEvent = AmpUT_VideoEnc_FifoCB;
            AmpFifo_Create(&FifoDefCfg, &VirtualJpegFifoHdlr);
        }
    }

    //create pipeline
    if (StillEncPipe == NULL) {
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};

        AmpEnc_GetDefaultCfg(&PipeCfg);
        PipeCfg.encoder[0] = StillEncPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_VideoEncPipeCallback;
        PipeCfg.type = AMP_ENC_STILL_PIPE;
        AmpEnc_Create(&PipeCfg, &StillEncPipe);

        AmpEnc_Add(StillEncPipe);
    }

    /* Phase II */
    /* fill script and run */
    if (StillRawCaptureRunning) {
        AmbaPrint("Error Status");
        goto _DONE;
    }

    if (PivVideoThm == 0) {
        PivProcess++;
    }

    AmbaSensor_GetModeInfo(VinChannel, pivCtrl.SensorMode, &SensorInfo);
    PivCaptureWidth = pivCtrl.CaptureWidth;
    PivCaptureHeight = pivCtrl.CaptureHeight;
    PivMainWidth = pivCtrl.MainWidth;
    PivMainHeight = pivCtrl.MainHeight;
    ScrnWidth = 960;
    ThmWidth = 160;
    ThmHeight = 120;
    if (pivCtrl.AspectRatio == VAR_16x9) {
        ScrnHeight = 540;
        ScrnWidthAct = 960;
        ScrnHeightAct = 540;
        ThmWidthAct = 160;
        ThmHeightAct = 90;
        QvLCDW = 960;
        QvLCDH = 360;
        QvHDMIW = 1920;
        QvHDMIH = 1080;
    } else if (pivCtrl.AspectRatio == VAR_3x2) {
        ScrnHeight = 640;
        ScrnWidthAct = 960;
        ScrnHeightAct = 640;
        ThmWidthAct = 160;
        ThmHeightAct = 106;
        QvLCDW = 960;
        QvLCDH = 426;
        QvHDMIW = 1920;
        QvHDMIH = 1280;
    } else if (pivCtrl.AspectRatio == VAR_1x1) {
        ScrnHeight = 720;
        ScrnWidthAct = 720;
        ScrnHeightAct = 720;
        ThmWidthAct = 120;
        ThmHeightAct = 120;
        QvLCDW = 720;
        QvLCDH = 480;
        QvHDMIW = 1080;
        QvHDMIH = 1080;
    } else { //4:3
        ScrnHeight = 720;
        ScrnWidthAct = 960;
        ScrnHeightAct = 720;
        ThmWidthAct = 160;
        ThmHeightAct = 120;
        QvLCDW = 960;
        QvLCDH = 480;
        QvHDMIW = 1440;
        QvHDMIH = 1080;
    }

    /* Step1. calc raw and yuv buffer memory */
    RawPitch = (cmpr)? AMP_COMPRESSED_RAW_WIDTH(PivCaptureWidth): PivCaptureWidth*2;
    RawPitch = ALIGN_32(RawPitch);
    RawWidth =  PivCaptureWidth;
    RawHeight = PivCaptureHeight;
    RawSize = RawPitch*RawHeight;
    AmbaPrint("[UT_videoEncPIV]raw(%u %u %u)", RawPitch, RawWidth, RawHeight);

    //FastMode need 16_align enc_height
    if (iso == 2) {
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32(PivMainWidth);
        YuvHeight = ALIGN_16(PivMainHeight);
        YuvSize = YuvWidth*YuvHeight*2;
        AmbaPrint("[UT_videoEncPIV]yuv(%u %u %u)!", YuvWidth, YuvHeight, YuvSize);
        ScrnW = ALIGN_32(ScrnWidth);
        ScrnH = ALIGN_16(ScrnHeight);
        ScrnSize = ScrnW*ScrnH*2;
        ThmW = ALIGN_32(ThmWidth);
        ThmH = ALIGN_16(ThmHeight);
        ThmSize = ThmW*ThmH*2;
        AmbaPrint("[UT_videoEncPIV]scrn(%d %d %u) thm(%d %d %u)!", ScrnW, ScrnH, ScrnSize, ThmW, ThmH, ThmSize);
    } else {
        //DSP lib need 32ALign for Width and 16_Align for height in buffer allocation
        YuvWidth = ALIGN_32(PivMainWidth);
        YuvHeight = ALIGN_16(PivMainHeight);
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
        AmbaPrint("[UT_videoEncPIV]scrn(%d %d %u) thm(%d %d %u)!", ScrnW, ScrnH, ScrnSize, ThmW, ThmH, ThmSize);
    }
    /* QV need 16_Align */
    QvLCDSize = QvLCDW*QvLCDH*2;
    QvLCDSize += (QvLCDSize*15)/100;
    QvHDMISize = QvHDMIW*QvHDMIH*2;
    QvHDMISize += (QvHDMISize*15)/100;

    AmbaPrint("[UT_videoEncPIV]qvLCD(%u) qvHDMI(%u)!", QvLCDSize, QvHDMISize);

    /* Step2. allocate raw and yuv/scrn/thm buffer address, script address */
    if (DspBackgroundProcMode) {
        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, (void **)&TmpbufRaw, RawSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_videoEncPIV]DDR alloc raw fail (%u)!", RawSize);
        } else {
            s_rawBuffAddr = (UINT8*)TempPtr;
            Ori_s_rawBuffAddr = (UINT8*)TmpbufRaw;
            AmbaPrint("[UT_videoEncPIV]rawBuffAddr (0x%08X) (%u)!", s_rawBuffAddr, RawSize);
        }

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, (void **)&TmpbufRaw, YuvSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_videoEncPIV]DDR alloc yuv_main fail (%u)!", YuvSize);
        } else {
            s_yuvBuffAddr = (UINT8*)TempPtr;
            Ori_s_yuvBuffAddr = (UINT8*)TmpbufRaw;
            AmbaPrint("[UT_videoEncPIV]yuvBuffAddr (0x%08X)!", s_yuvBuffAddr);
        }

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, (void **)&TmpbufRaw, ScrnSize*1, 32);
        if (Er != OK) {
            AmbaPrint("[UT_videoEncPIV]DDR alloc yuv_scrn fail (%u)!", ScrnSize*1);
        } else {
            s_scrnBuffAddr = (UINT8*)TempPtr;
            Ori_s_scrnBuffAddr = (UINT8*)TmpbufRaw;
            AmbaPrint("[UT_videoEncPIV]scrnBuffAddr (0x%08X)!", s_scrnBuffAddr);
        }

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, (void **)&TmpbufRaw, ThmSize*1, 32);
        if (Er != OK) {
            AmbaPrint("[UT_videoEncPIV]DDR alloc yuv_thm fail (%u)!", ThmSize*1);
        } else {
            s_thmBuffAddr = (UINT8*)TempPtr;
            Ori_s_thmBuffAddr = (UINT8*)TmpbufRaw;
            AmbaPrint("[UT_videoEncPIV]thmBuffAddr (0x%08X)!", s_thmBuffAddr);
        }
    } else {
        //In Mode_0 PIV, APP do not need to provide buffer memory
        s_rawBuffAddr = 0;
        s_yuvBuffAddr = s_scrnBuffAddr = s_thmBuffAddr = 0;
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, (void **)&TmpbufRaw, QvLCDSize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_videoEncPIV]DDR alloc yuv_lcd fail (%u)!", QvLCDSize*1);
    } else {
        s_qvLCDBuffAddr = (UINT8*)TempPtr;
        Ori_s_qvLCDBuffAddr = (UINT8*)TmpbufRaw;
        AmbaPrint("[UT_videoEncPIV]qvLCDBuffaddr (0x%08X)!", s_qvLCDBuffAddr);
    }

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, (void **)&TmpbufRaw, QvHDMISize*1, 32);
    if (Er != OK) {
        AmbaPrint("[UT_videoEncPIV]DDR alloc yuv_hdmi fail (%u)!", QvHDMISize*1);
    } else {
        s_qvHDMIBuffAddr = (UINT8*)TempPtr;
        Ori_s_qvHDMIBuffAddr = (UINT8*)TmpbufRaw;
        AmbaPrint("[UT_videoEncPIV]qvHDMIBuffaddr (0x%08X)!", s_qvHDMIBuffAddr);
    }
    {
        UINT32 ScriptSize = 0;
        ScriptSize = ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s)) + \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_RAW2YUV_s))+ \
                     ALIGN_128(sizeof(AMP_SENC_SCRPT_YUV2JPG_s));

        Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, (void **)&TmpbufRaw, ScriptSize, 32);
        if (Er != OK) {
            AmbaPrint("[UT_videoEncPIV]Cache_DDR alloc scriptAddr fail (%u)!", ScriptSize);
        } else {
            s_scriptAddr = (UINT8*)TempPtr;
            Ori_s_scriptAddr = (UINT8*)TmpbufRaw;
            AmbaPrint("[UT_videoEncPIV]scriptAddr (0x%08X) (%d)!", s_scriptAddr, ScriptSize);
        }
    }

    /* Step3. fill script */
    //general config
    StageAddr = s_scriptAddr;
    GenScrpt = (AMP_SENC_SCRPT_GENCFG_s *)StageAddr;
    memset(GenScrpt, 0x0, sizeof(AMP_SENC_SCRPT_GENCFG_s));
    GenScrpt->Cmd = SENC_GENCFG;
    GenScrpt->RawEncRepeat = 0;
    GenScrpt->RawToCap = 1;
    GenScrpt->StillProcMode = iso;

#if 0 //Temp not support QV in PIV
    if (DspBackgroundProcMode) {
        GenScrpt->QVConfig.DisableLCDQV = (qvDisplayCfg == 0)? 1: 0;
        GenScrpt->QVConfig.DisableHDMIQV = (qvDisplayCfg == 0)? 1: 0;
    } else {
        //Mode_0 need QV working buffer
        GenScrpt->QVConfig.DisableLCDQV = 0;
        GenScrpt->QVConfig.DisableHDMIQV = 0;
    }
#else
    if (DspBackgroundProcMode) {
        GenScrpt->QVConfig.DisableLCDQV = 1;
        GenScrpt->QVConfig.DisableHDMIQV = 1;
    } else {
#ifdef CONFIG_SOC_A9
        //Mode_0 need QV working buffer
        GenScrpt->QVConfig.DisableLCDQV = 0;
        GenScrpt->QVConfig.DisableHDMIQV = 0;
#else
        GenScrpt->QVConfig.DisableLCDQV = 1;
        GenScrpt->QVConfig.DisableHDMIQV = 1;
#endif
    }
#endif

    GenScrpt->QVConfig.LCDDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.LCDLumaAddr = s_qvLCDBuffAddr;
    GenScrpt->QVConfig.LCDChromaAddr = s_qvLCDBuffAddr + QvLCDSize/2;
    GenScrpt->QVConfig.LCDWidth = QvLCDW;
    GenScrpt->QVConfig.LCDHeight = QvLCDH;
    GenScrpt->QVConfig.HDMIDataFormat = AMP_YUV_422;
    GenScrpt->QVConfig.HDMILumaAddr = s_qvHDMIBuffAddr;
    GenScrpt->QVConfig.HDMIChromaAddr = s_qvHDMIBuffAddr + QvHDMISize/2;
    GenScrpt->QVConfig.HDMIWidth = QvHDMIW;
    GenScrpt->QVConfig.HDMIHeight = QvHDMIH;

    GenScrpt->b2LVCfg = AMP_ENC_SCRPT_B2LV_NONE;
    GenScrpt->ScrnEnable = 1;
    GenScrpt->ThmEnable = 1;

#ifdef CONFIG_SOC_A9
    GenScrpt->MainBuf.ColorFmt = AMP_YUV_422;
#else
    GenScrpt->MainBuf.ColorFmt = AMP_YUV_420;
#endif
    GenScrpt->MainBuf.Width = GenScrpt->MainBuf.Pitch = YuvWidth;
    GenScrpt->MainBuf.Height = YuvHeight;
    GenScrpt->MainBuf.LumaAddr = s_yuvBuffAddr;
    GenScrpt->MainBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->MainBuf.AOI.X = 0;
    GenScrpt->MainBuf.AOI.Y = 0;
    GenScrpt->MainBuf.AOI.Width = PivMainWidth;
    GenScrpt->MainBuf.AOI.Height = PivMainHeight;

#ifdef CONFIG_SOC_A9
    GenScrpt->ScrnBuf.ColorFmt = AMP_YUV_422;
#else
    GenScrpt->ScrnBuf.ColorFmt = AMP_YUV_420;
#endif
    GenScrpt->ScrnBuf.Width = GenScrpt->ScrnBuf.Pitch = ScrnW;
    GenScrpt->ScrnBuf.Height = ScrnH;
    GenScrpt->ScrnBuf.LumaAddr = s_scrnBuffAddr;
    GenScrpt->ScrnBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->ScrnBuf.AOI.X = 0;
    GenScrpt->ScrnBuf.AOI.Y = 0;
    GenScrpt->ScrnBuf.AOI.Width = ScrnWidthAct;
    GenScrpt->ScrnBuf.AOI.Height = ScrnHeightAct;
    GenScrpt->ScrnWidth = ScrnWidth;
    GenScrpt->ScrnHeight = ScrnHeight;

#ifdef CONFIG_SOC_A9
    GenScrpt->ThmBuf.ColorFmt = AMP_YUV_422;
#else
    GenScrpt->ThmBuf.ColorFmt = AMP_YUV_420;
#endif
    GenScrpt->ThmBuf.Width = GenScrpt->ThmBuf.Pitch = ThmW;
    GenScrpt->ThmBuf.Height = ThmH;
    GenScrpt->ThmBuf.LumaAddr = s_thmBuffAddr;
    GenScrpt->ThmBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->ThmBuf.AOI.X = 0;
    GenScrpt->ThmBuf.AOI.Y = 0;
    GenScrpt->ThmBuf.AOI.Width = ThmWidthAct;
    GenScrpt->ThmBuf.AOI.Height = ThmHeightAct;
    GenScrpt->ThmWidth = ThmWidth;
    GenScrpt->ThmHeight = ThmHeight;
    memcpy(&GenScrpt->PIVMainBlendInfo[0],&PIVInfo,sizeof(AMP_VIDEOENC_BLEND_INFO_s));

    if (targetSize) {
        extern UINT32 AmpUT_JpegBRCPredictCB(UINT16 targetBpp, UINT32 stillProc, UINT8* predictQ, UINT8 *curveNum, UINT32 *curveAddr);
        AmbaPrint("[UT_videoEncPIV]Target Size %u Kbyte", targetSize);
        AmpUT_VidoeEnc_InitMJpegDqt(VideoPIVQTable[0], -1);
        AmpUT_VidoeEnc_InitMJpegDqt(VideoPIVQTable[1], -1);
        AmpUT_VidoeEnc_InitMJpegDqt(VideoPIVQTable[2], -1);
        GenScrpt->BrcCtrl.Tolerance = 10;
        GenScrpt->BrcCtrl.MaxEncLoop = encodeLoop;
        GenScrpt->BrcCtrl.JpgBrcCB = AmpUT_JpegBRCPredictCB;
        GenScrpt->BrcCtrl.TargetBitRate = \
           (((targetSize<<13)/PivMainWidth)<<12)/PivMainHeight;
        GenScrpt->BrcCtrl.MainQTAddr = VideoPIVQTable[0];
        GenScrpt->BrcCtrl.ThmQTAddr = VideoPIVQTable[1];
        GenScrpt->BrcCtrl.ScrnQTAddr = VideoPIVQTable[2];
    } else {
        AmpUT_VidoeEnc_InitMJpegDqt(VideoPIVQTable[0], VideoPIVMainQuality);
        AmpUT_VidoeEnc_InitMJpegDqt(VideoPIVQTable[1], VideoPIVThmbQuality);
        AmpUT_VidoeEnc_InitMJpegDqt(VideoPIVQTable[2], VideoPIVScrnQuality);
        GenScrpt->BrcCtrl.Tolerance = 0;
        GenScrpt->BrcCtrl.MaxEncLoop = 0;
        GenScrpt->BrcCtrl.JpgBrcCB = NULL;
        GenScrpt->BrcCtrl.TargetBitRate = 0;
        GenScrpt->BrcCtrl.MainQTAddr = VideoPIVQTable[0];
        GenScrpt->BrcCtrl.ThmQTAddr = VideoPIVQTable[1];
        GenScrpt->BrcCtrl.ScrnQTAddr = VideoPIVQTable[2];
    }

    GenScrpt->PostProc = &post_videoEncPiv_cb;
    GenScrpt->PreProc = &pre_videoEncPiv_cb;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_GENCFG_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoEncPIV]Stage_0 0x%X", StageAddr);

    //raw cap config
    StageAddr = s_scriptAddr + TotalScriptSize;
    RawCapScrpt = (AMP_SENC_SCRPT_RAWCAP_s *)StageAddr;
    memset(RawCapScrpt, 0x0, sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    RawCapScrpt->Cmd = SENC_RAWCAP;
    RawCapScrpt->SrcType = AMP_ENC_SOURCE_VIN;
    RawCapScrpt->ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING;
    RawCapScrpt->SensorMode = pivCtrl.SensorMode;
    RawCapScrpt->FvRawCapArea.VcapWidth = PivCaptureWidth;
    RawCapScrpt->FvRawCapArea.VcapHeight = PivCaptureHeight;
    RawCapScrpt->FvRawCapArea.EffectArea.X = RawCapScrpt->FvRawCapArea.EffectArea.Y = 0;
    RawCapScrpt->FvRawCapArea.EffectArea.Width = RawCapScrpt->FvRawCapArea.VcapWidth;
    RawCapScrpt->FvRawCapArea.EffectArea.Height = RawCapScrpt->FvRawCapArea.VcapHeight;
    RawCapScrpt->FvRawType = (cmpr)? AMP_STILLENC_RAW_COMPR: AMP_STILLENC_RAW_UNCOMPR;
    RawCapScrpt->FvRawBuf.Buf = s_rawBuffAddr;
    RawCapScrpt->FvRawBuf.Width = RawWidth;
    RawCapScrpt->FvRawBuf.Height = RawHeight;
    RawCapScrpt->FvRawBuf.Pitch = RawPitch;
    RawCapScrpt->FvBufRule = AMP_ENC_SCRPT_BUFF_FIXED;
    RawCapScrpt->FvRingBufSize = RawSize*1;
    RawCapScrpt->CapCB.RawCapCB = AmpUT_VideoEnc_PIV_RawCapCB;
    TotalScriptSize += ALIGN_128(sizeof(AMP_SENC_SCRPT_RAWCAP_s));
    TotalStageNum ++;
    AmbaPrint("[UT_videoEncPIV]Stage_1 0x%X", StageAddr);

    //raw2yuv config
    StageAddr = s_scriptAddr + TotalScriptSize;
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
    StageAddr = s_scriptAddr + TotalScriptSize;
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
    Scrpt.ScriptStartAddr = (UINT32)s_scriptAddr;
    Scrpt.ScriptTotalSize = TotalScriptSize;
    Scrpt.ScriptStageNum = TotalStageNum;
    AmbaPrint("[UT_videoEncPIV]Scrpt addr 0x%X, Sz %uByte, stg %d", Scrpt.ScriptStartAddr, Scrpt.ScriptTotalSize, Scrpt.ScriptStageNum);

    StillRawCaptureRunning = 1;

    /* Step4. execute script */
    AmpEnc_RunScript(StillEncPipe, &Scrpt, AMP_ENC_FUNC_FLAG_NONE);

    /* Step4. release script */
    AmbaPrint("[0x%08X] memFree", s_scriptAddr);
    if (AmbaKAL_BytePoolFree((void *)Ori_s_scriptAddr) != OK) {
        AmbaPrint("memFree Fail (scrpt)");
    }
    s_scriptAddr = NULL;

    AmbaPrint("memFree Done");

_DONE:
    return 0;
}

int AmpUT_VidoeEnc_CheckCapability(void)
{
    UINT8 enable = 1;
    INPUT_ENC_LIMIT_s *capability = NULL;

    if (TvLiveview == 0) { //single vout
        capability = &VideoEncMgt[EncModeIdx].limit[0];
        AmbaPrint("[Cap] (%d %d) (%d %d) (%d %d %d %d %d %d %d) (%d %d)", \
            capability->H264Encode, capability->MJPGEncode, capability->PIP30PYuv, capability->PIP30PEncode, \
            capability->HFlip, capability->Rotata90, capability->Rotata90VFlip, capability->HFlipVFlip, \
            capability->VFlip, capability->Rotata90HFlipVFlip, capability->Rotata90HFlip, capability->Stamp, \
            capability->DualHD);
        if (EncDualStream == 0) {
            if (EncPriSpecH264 == 0) {enable = capability->MJPGEncode;}
            if (enable == 0) {goto _done;}
        } else {
            if (EncPriSpecH264 == 0 || EncSecSpecH264 == 0) {enable = capability->MJPGEncode;}
            if (enable == 0) {goto _done;}
        }
    } else if (TvLiveview && EncDualStream == 0) { //dual vout, single stream
        capability = &VideoEncMgt[EncModeIdx].limit[1];
        AmbaPrint("[Cap] (%d %d) (%d %d) (%d %d %d %d %d %d %d) (%d %d)", \
            capability->H264Encode, capability->MJPGEncode, capability->PIP30PYuv, capability->PIP30PEncode, \
            capability->HFlip, capability->Rotata90, capability->Rotata90VFlip, capability->HFlipVFlip, \
            capability->VFlip, capability->Rotata90HFlipVFlip, capability->Rotata90HFlip, capability->Stamp, \
            capability->DualHD);

        if (EncPriSpecH264 == 0) {enable = capability->MJPGEncode;}
        if (enable == 0) {goto _done;}
    } else if (TvLiveview && EncDualStream) { //dual vout, dual stream
        capability = &VideoEncMgt[EncModeIdx].limit[2];
        AmbaPrint("[Cap] (%d %d) (%d %d) (%d %d %d %d %d %d %d) (%d %d)", \
            capability->H264Encode, capability->MJPGEncode, capability->PIP30PYuv, capability->PIP30PEncode, \
            capability->HFlip, capability->Rotata90, capability->Rotata90VFlip, capability->HFlipVFlip, \
            capability->VFlip, capability->Rotata90HFlipVFlip, capability->Rotata90HFlip, capability->Stamp, \
            capability->DualHD);

        if (EncPriSpecH264 == 0 || EncSecSpecH264 == 0) {
            enable = capability->MJPGEncode;
        }
        if (enable == 0) {
            goto _done;
        }
    }

#ifdef CONFIG_SOC_A12
    if (EncRotation) {
        if (EncRotation == AMP_ROTATE_0_HORZ_FLIP) {
            enable = capability->HFlip;
        } else if (EncRotation == AMP_ROTATE_90) {
            if (LiveViewProcMode == 0 && \
                VideoEncMgt[EncModeIdx].CaptureWidth < VideoEncMgt[EncModeIdx].MainWidth && \
                VideoEncMgt[EncModeIdx].CaptureHeight < VideoEncMgt[EncModeIdx].MainHeight) {
                enable = 0;
            } else {
                enable = capability->Rotata90;
            }
        } else if (EncRotation == AMP_ROTATE_90_VERT_FLIP) {
            if (LiveViewProcMode == 0 && \
                VideoEncMgt[EncModeIdx].CaptureWidth < VideoEncMgt[EncModeIdx].MainWidth && \
                VideoEncMgt[EncModeIdx].CaptureHeight < VideoEncMgt[EncModeIdx].MainHeight) {
                enable = 0;
            } else {
                enable = capability->Rotata90VFlip;
            }
        } else if (EncRotation == AMP_ROTATE_180) {
            enable = capability->HFlipVFlip;
        } else if (EncRotation == AMP_ROTATE_180_HORZ_FLIP) {
            enable = capability->VFlip;
        } else if (EncRotation == AMP_ROTATE_270) {
            if (LiveViewProcMode == 0 && \
                VideoEncMgt[EncModeIdx].CaptureWidth < VideoEncMgt[EncModeIdx].MainWidth && \
                VideoEncMgt[EncModeIdx].CaptureHeight < VideoEncMgt[EncModeIdx].MainHeight) {
                enable = 0;
            } else {
                enable = capability->Rotata90HFlipVFlip;
            }
        } else if (EncRotation == AMP_ROTATE_270_VERT_FLIP) {
            if (LiveViewProcMode == 0 && \
                VideoEncMgt[EncModeIdx].CaptureWidth < VideoEncMgt[EncModeIdx].MainWidth && \
                VideoEncMgt[EncModeIdx].CaptureHeight < VideoEncMgt[EncModeIdx].MainHeight) {
                enable = 0;
            } else {
                enable = capability->Rotata90HFlip;
            }
        }

        if (enable == 0) {
            goto _done;
        }
    }
#else
    if (EncRotation) {
        if (EncRotation == AMP_ROTATE_0_HORZ_FLIP) {
            enable = capability->HFlip;
        } else if (EncRotation == AMP_ROTATE_90) {
            enable = capability->Rotata90;
        } else if (EncRotation == AMP_ROTATE_90_VERT_FLIP) {
            enable = capability->Rotata90VFlip;
        } else if (EncRotation == AMP_ROTATE_180) {
            enable = capability->HFlipVFlip;
        } else if (EncRotation == AMP_ROTATE_180_HORZ_FLIP) {
            enable = capability->VFlip;
        } else if (EncRotation == AMP_ROTATE_270) {
            enable = capability->Rotata90HFlipVFlip;
        } else if (EncRotation == AMP_ROTATE_270_VERT_FLIP) {
            enable = capability->Rotata90HFlip;
        }

        if (enable == 0) {
            goto _done;
        }
    }
#endif

    if (EncDateTimeStampPri || EncDateTimeStampSec) {
        enable = capability->Stamp;
        if (enable == 0) {
            goto _done;
        }
    }

    if (EncDualHDStream) {
        enable = capability->DualHD;
        if (enable == 0) {
            goto _done;
        }
    }

#ifdef CONFIG_SOC_A12
    if (VideoEnc_EisAlgoEnable)  {
        capability = &VideoEncMgt[EncModeIdx].limit[0];
        enable = capability->EisSupport;
        if (enable == 0) {
            goto _done;
        }
    }
#endif

_done:
    return enable;
}


/**
 * Pre-processing before entering standby
 */
static void AmpUT_VideoEnc_RegisterResumeInt(void)
{
    // At this moment, all INTs are disabled by AmbaPLL, we have to enable some in order to wake up.
#ifdef CONFIG_SOC_A12
    AmbaINT_IntEnable(AMBA_VIC_INT_ID9_UART0); // Use UART INT to wake
#endif

}

int AmpUT_VideoEncTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_VideoEncTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        int SensorId = 0, LcdId = 0;
        if (Status != STATUS_IDLE) {
            return -1;
        }
        if (argc < 3) {
            UINT8 i = 0;
            AmbaPrint("Usage: t videoenc init [sensorID][LcdID]");
            AmbaPrint("               sensorID:");
            for (i = 0; i<INPUT_DEVICE_NUM; i++) {
                AmbaPrint("                        %2d -- %s", i, MWUT_GetInputDeviceName(i));
            }
            AmbaPrint("               LcdID: 0 -- WDF9648W");
            return -1;
        } else if (argc == 3) {
            SensorId = atoi(argv[2]);
            LcdId = 0;
        } else {
            SensorId = atoi(argv[2]);
            LcdId = atoi(argv[3]);
        }
        AmpUT_VideoEnc_Init(SensorId, LcdId);

    #ifdef CONFIG_SOC_A9
        {
            extern AMBA_DSP_LOG_CTRL_s DspLogCtrl;
            DspLogCtrl.SysCmdShow = 1;
            DspLogCtrl.VoutCmdShow = 1;
            DspLogCtrl.EncCmdShow = 1;
            DspLogCtrl.DecCmdShow = 0;
            //DspLogCtrl.IDspCmdShow = 1;
            DspLogCtrl.VcapCmdShow = 1;
            DspLogCtrl.VcapCmdBufInfo = 1;
        }
    #endif

        Status = STATUS_INIT;
        AmbaPrintColor(BLUE, "[AMP_UT] Init done %d", SensorId);
    } else if (strcmp(argv[1],"tv") == 0) {
        if (Status != STATUS_INIT) {
            AmbaPrint("Please set it before liveviewstart");
        }

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc tv [enable]");
        }

        TvLiveview = atoi(argv[2]);

        AmbaPrint("TV: %s",TvLiveview?"Enable":"Disable");
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
            ExtLiveViewProcMode = Proc;
            if (Proc == 0) {
                ExtLiveViewOSMode = Aux;
                ExtLiveViewAlgoMode = 0;
            } else {
                ExtLiveViewAlgoMode = Aux;
                ExtLiveViewOSMode = 0;
            }
            if (ExtLiveViewProcMode == 0) {
                AmbaPrint("Set LvProc = %s_%s", "Express", Aux?"OS":"Basic");
            } else {
                AmbaPrint("Set LvProc = %s_%s", "Hybrid",
                    (Aux==0)? "Fast": ((Aux==1)? "LISO": "HISO"));
            }
        }
    } else if ((strcmp(argv[1],"liveviewstart") == 0) || (strcmp(argv[1],"lvst") == 0)) {
        UINT32 tt;
        UINT8 PreviousEncodeMode = EncModeIdx;

        if (Status != STATUS_INIT) {
            return -1;
        }

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc liveviewstart [modeId]");

            if (VideoEncMgt == NULL) {
                AmbaPrintColor(RED, "Please hook sensor first!!!");
                return -1;
            }

            AmbaPrint("[%s] mode_id: ", MWUT_GetInputDeviceName(InputDeviceId));
            MWUT_InputVideoModePrintOutAll(VideoEncMgt);
            return -1;
        }
        EncModeIdx = tt = atoi(argv[2]);
        if (VideoEncMgt[tt].ForbidMode) {
            AmbaPrintColor(RED, "Not Suppot this Mode (%d)", tt);
            return -1;
        }
        if (VideoEncMgt[tt].PALModeOnly && EncodeSystem == 0 && \
            (EncDualHDStream || EncDualStream))  {
            AmbaPrintColor(RED, "Only Support PAL mode !!!!!");
            return -1;
        }
        if (AmpUT_VidoeEnc_CheckCapability() == 0) {
            EncModeIdx = PreviousEncodeMode;
            AmbaPrint("%s doesn't support this mode", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
            return -1;
        }

        AmpUT_VideoEnc_LiveviewStart(tt);
        Status = STATUS_LIVEVIEW;
        AmbaPrintColor(BLUE, "[AMP_UT] LVST %d", EncModeIdx);
    } else if ((strcmp(argv[1],"encstart") == 0) || (strcmp(argv[1],"enst") == 0)) {
        UINT32 Duration = atoi(argv[2]);

        if (Status != STATUS_LIVEVIEW) {
            return -1;
        }

        if (AmpUT_VidoeEnc_CheckCapability() == 0) {
            AmbaPrint("%s doesn't support this Encode Mode", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
            return -1;
        }

        AmbaPrint("Encode time: %d milliseconds", Duration);
        AmpUT_VideoEnc_EncodeStart();
        Status = STATUS_ENCODE;
        AmbaPrintColor(BLUE, "[AMP_UT] Encode start");

        if (Duration != 0) {
            while (Duration) {
                AmbaKAL_TaskSleep(1);
                Duration--;
                if (Duration == 0) {
                    AmpUT_VideoEnc_EncStop();
                    EncDateTimeStampPri = EncDateTimeStampSec = 0;
                    Status = STATUS_LIVEVIEW;
                    AmbaPrintColor(BLUE, "[AMP_UT] Encode stop");
                }
                if (Status == STATUS_LIVEVIEW) {
                    Duration = 0;
                }
            }
        }

    } else if ((strcmp(argv[1],"encstop") == 0) || (strcmp(argv[1],"ensp") == 0)) {
        if (Status != STATUS_ENCODE && Status != STATUS_PAUSE) {
            return -1;
        }
        AmpUT_VideoEnc_EncStop();
        EncDateTimeStampPri = EncDateTimeStampSec = 0;
        Status = STATUS_LIVEVIEW;
        AmbaPrintColor(BLUE, "[AMP_UT] Encode stop");
    } else if ((strcmp(argv[1],"pause") == 0)) {
        AmpUT_VideoEnc_EncPause();
        AmbaPrintColor(BLUE, "[AMP_UT] Encode pause");
    } else if ((strcmp(argv[1],"resume") == 0)) {
        AmpUT_VideoEnc_EncResume();
        AmbaPrintColor(BLUE, "[AMP_UT] Encode resume");
    } else if ((strcmp(argv[1],"liveviewstop") == 0) || (strcmp(argv[1],"lvsp") == 0)) {
        if (Status != STATUS_LIVEVIEW) {
            return -1;
        }
        /*  Make sure output file is closed */
        if (outputPriFile || outputSecFile) {
            AmbaPrint("MUXER not IDLE !!!!");
            return -1;
        }

        AmpUT_VideoEnc_LiveviewStop();
        Status = STATUS_INIT;
        AmbaPrintColor(BLUE, "[AMP_UT] LVSP");
    } else if ((strcmp(argv[1],"standby") == 0)) {
        AmbaPrint("Enter Standby mode");
        AmbaIPC_LinkCtrlSuspendLinux(AMBA_LINK_SLEEP_TO_RAM);
        if (AmbaIPC_LinkCtrlWaitSuspendLinux(5000) != OK) {
            AmbaPrint("SuspendLinux (%d) failed!\n", AMBA_LINK_SLEEP_TO_RAM);
        }
        AmbaPrintk_Flush();
        AmbaLCD_SetBacklight(0, 0);
        AmbaPLL_SetStandbyMode(0, AmpUT_VideoEnc_RegisterResumeInt, NULL);
        AmbaIPC_LinkCtrlResumeLinux(AMBA_LINK_SLEEP_TO_RAM);
        if (AmbaIPC_LinkCtrlWaitResumeLinux(5000) != OK) {
           AmbaPrint("ResumeLinux (%d) failed.\n", AMBA_LINK_SLEEP_TO_RAM);
        }
        AmbaLCD_SetBacklight(0, 1);
    } else if ((strcmp(argv[1],"initzoom") == 0)) {
        float dzx, dzy;
        AMBA_DSP_IMG_DZOOM_INFO_s DzoomInfo;
        AMBA_DSP_IMG_MODE_CFG_s Mode;

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc initzoom [factorX] [factorY]");
            AmbaPrint("                           factorX: Horizontal initial zoom in decimal format, eg: 1.5");
            AmbaPrint("                           factorY: Vertical initial zoom  in decimal format, eg: 2.0");
            return -1;
        }
    #ifdef CONFIG_SOC_A12
        if (VideoEnc_EisAlgoEnable) {
            AmbaPrint("Can't Support Eis + Dzoom now");
            return -1;
        }
    #endif

        dzx = atof(argv[2]);
        InitZoomX = (UINT32)(dzx*65536.);
        dzy = atof(argv[3]);
        InitZoomY = (UINT32)(dzy*65536.);

        if (Status == STATUS_LIVEVIEW) { // temp hack
            memset(&Mode, 0x0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
            DzoomInfo.ZoomX = InitZoomX;
            DzoomInfo.ZoomY = InitZoomY;
            DzoomInfo.ShiftX = 0;
            DzoomInfo.ShiftY = 0;
            AmbaDSP_WarpCore_SetDzoomFactor(&Mode, &DzoomInfo);

            AmbaDSP_WarpCore_CalcDspWarp(&Mode, 0);
            AmbaDSP_WarpCore_CalcDspCawarp(&Mode, 0);
            AmbaDSP_WarpCore_SetDspWarp(&Mode);
            AmbaDSP_WarpCore_SetDspCawarp(&Mode);
        }
        //FIXME: %f issue
        //AmbaPrint("Initial Dzoom for next sensor mode change: (%X %X) = (%f %f)",InitZoomX, InitZoomY, dzx, dzy);

    } else if ((strcmp(argv[1],"dual") == 0)) {
        UINT8 IsDualEnable = atoi(argv[2]);

        if (Status != STATUS_INIT) {
            AmbaPrint("Please set dualstream before liveview start");
            return -1;
        }

        AmbaPrintColor(BLUE, "[AMP_UT] DualStream: %s", IsDualEnable? "ENABLE": "DISABLE");
        EncDualStream = IsDualEnable;
    } else if ((strcmp(argv[1],"dualhd") == 0)) {
        UINT8 IsDualHDEnable = atoi(argv[2]);

        if (Status != STATUS_INIT) {
            AmbaPrint("Please set dualstream before liveview start");
            return -1;
        }

        AmbaPrintColor(BLUE,"[AMP_UT] DualHDStream: %s", IsDualHDEnable?"ENABLE":"DISABLE");
#ifdef CONFIG_SOC_A12
        AmbaPrint("Note: Only support dual 1080P60, dual 1080P30 and dual 720P60");
#endif
        AmbaPrint("Note: DualHD does NOT support dual vout!");
        EncDualHDStream = IsDualHDEnable;
        if (EncDualHDStream) {
            TvLiveview = 0;
            AmbaPrint("Tv Disable!");
        }
    }  else if ((strcmp(argv[1],"brc") == 0)) {
#ifdef CONFIG_SOC_A12
        static AMP_VIDEOENC_RUNTIME_QUALITY_CFG_s RealTimeQualityCfg[2] = {0};
        if (argc < 3) {
            AmbaPrint("Usage: t videoenc brc [option]");
            AmbaPrint("         option: config quality control or invoke");
            AmbaPrint("               config - config quality control parameters");
            AmbaPrint("               invoke - invoke command");
            return -1;
        }

        if ((strcmp(argv[2],"config") == 0)) {
            if (argc < 4) {
                AmbaPrint("Usage: t videoenc brc config [mode][channel][...]");
                AmbaPrint("      channel: encode channel");
                AmbaPrint("               0 - Primary");
                AmbaPrint("               1 - Secondary");
                AmbaPrint("         mode: bitrate control option");
                AmbaPrint("               0 - Bitrate config");
                AmbaPrint("               1 - Bitrate change");
                AmbaPrint("               2 - Gop Change");
                AmbaPrint("               3 - QP control");
                AmbaPrint("               4 - QP model control");
                AmbaPrint("               5 - ROI control");
                AmbaPrint("               6 - HQP control");
                AmbaPrint("               7 - ZMV control");
                AmbaPrint("               8 - Force Idr");
                AmbaPrint("               9 - RESET ALL");
                AmbaPrint("             Bitrate config: t videoenc brc config 0 [Channel][BrcMode][AvgBitRate][MaxBitRate][MinBitRate][iBeat]");
                AmbaPrint("                                BrcMode: 1 - CBR, 2 - SmartVBR");
                AmbaPrint("                                AvgBitRate: Average Bitrate. Unit: Mbps.");
                AmbaPrint("                                MaxBitRate: Maximum Bitrate for SmartVBR only. Unit: Mbps");
                AmbaPrint("                                MinBitRate: Minimum Bitrate for SmartVBR only. Unit: Mbps");
                AmbaPrint("                                     iBeat: 0 - off");
                AmbaPrint("                                            1 - M-1 B pictures after IDR require special quant reduction");
                AmbaPrint("                                            2 - the P picture just before IDR requires special quant reduction");
                AmbaPrint("                                            4 - M-1 M-1 B pictures before IDR require special quant reduction");
                AmbaPrint("                                            8 - the P picture just before I requires special quant reduction");
                AmbaPrint("                                <Example> t videoenc brc config 0 0 1 12 12 12 0");
                AmbaPrint("                                <Example> t videoenc brc config 0 1 2 12 18 6 0");
                AmbaPrint("             Bitrate change: t videoenc brc change 1 [Channel][AverBitRtate]");
                AmbaPrint("                                AverBitRtate: Unit in Kbps.");
                AmbaPrint("                                <Example> t videoenc brc config 1 0 6000");
                AmbaPrint("             Gop Change: t videoenc brc config 2 [Channel][M][N][Idr]");
                AmbaPrint("                                M: Distance between in P frame(simeple rc not support M>1)");
                AmbaPrint("                                N: Distance between in I frame");
                AmbaPrint("                                Idr: Distance between in Idr frame");
                AmbaPrint("             QP control: t videoenc brc config 3 [Channel][QPminI][QPmaxI][QPminP][QPmaxP][QPminB][QPmaxB]");
                AmbaPrint("                                QPmin: minimum qp value");
                AmbaPrint("                                QPmax: maximum qp value");
                AmbaPrint("             QP model control: t videoenc brc config 4 (TBD)");
                AmbaPrint("             ROI control: t videoenc brc config 5 (TBD)");
                AmbaPrint("             HQP control: t videoenc brc config 6 [Channel][QPmax][QPmin][QPreduce]");
                AmbaPrint("                                QPmin: minimum qp value");
                AmbaPrint("                                QPmax: maximum qp value");
                AmbaPrint("                                QPreduce: how much better to make Q QP relative to P QP, 1~10, , default is 6");
                AmbaPrint("             ZMV control: t videoenc brc config 7 [Channel][threshold]");
                AmbaPrint("                                threshold: ZMV threshold");
                AmbaPrint("             Force Idr: t videoenc brc config 8 [Channel]");
                AmbaPrint("             RESET ALL: t videoenc brc config 9 [Channel]");
                return -1;
            } else {
                UINT8 Mode = 0, Channel = 0;
                AMP_AVENC_HDLR_s *EncHdlr = NULL;
                AMP_VIDEOENC_RUNTIME_QUALITY_CFG_s *RealTimeCtrl = NULL;
                Mode = atoi(argv[3]);
                Channel = atoi(argv[4]);

                if (Channel == 0) {
                    EncHdlr = VideoEncPri;
                    RealTimeCtrl = &RealTimeQualityCfg[0];
                } else if (Channel == 1) {
                    EncHdlr = VideoEncSec;
                    RealTimeCtrl = &RealTimeQualityCfg[1];
                } else {
                    AmbaPrintColor(RED,"Incorrect channel %u", Channel);
                }

                switch(Mode){
                case 0: /* BitRate Range */
                {
                    if (Status != STATUS_LIVEVIEW) {
                        AmbaPrint("Please set bitrate after liveview start %d", Status);
                        return -1;
                    }
                    {
                        UINT8 BrcMode = atoi(argv[5]);
                        UINT8 AvgBitRate = atoi(argv[6]);
                        UINT8 MaxBitRate = atoi(argv[7]);
                        UINT8 MinBitRate = atoi(argv[8]);
                        UINT8 IBeatMode = atoi(argv[9]);
                        AMP_VIDEOENC_BITSTREAM_CFG_s CurrentCfg;

                        AmpVideoEnc_GetBitstreamConfig(EncHdlr, &CurrentCfg);
                        CurrentCfg.Spec.H264Cfg.BitRateControl.BrcMode = BrcMode;
                        CurrentCfg.Spec.H264Cfg.BitRateControl.AverageBitrate = (UINT32)AvgBitRate*1E6;
                        CurrentCfg.Spec.H264Cfg.QualityControl.IBeatMode = IBeatMode;
                        if (CurrentCfg.Spec.H264Cfg.BitRateControl.BrcMode == VIDEOENC_SMART_VBR) {
                            CurrentCfg.Spec.H264Cfg.BitRateControl.MaxBitrate = (UINT32)MaxBitRate*1E6;
                            CurrentCfg.Spec.H264Cfg.BitRateControl.MinBitrate = (UINT32)MinBitRate*1E6;
                        }
                        AmpVideoEnc_SetBitstreamConfig(EncHdlr, &CurrentCfg);
                        AmbaPrint("BRC mode %s Avg %uMbps Max %uMbps Min %uMbps iBeat %u", \
                            (CurrentCfg.Spec.H264Cfg.BitRateControl.BrcMode == 1)? "CBR": "SmartVBR", \
                            CurrentCfg.Spec.H264Cfg.BitRateControl.AverageBitrate/1000000, \
                            CurrentCfg.Spec.H264Cfg.BitRateControl.MaxBitrate/1000000, \
                            CurrentCfg.Spec.H264Cfg.BitRateControl.MinBitrate/1000000, \
                            CurrentCfg.Spec.H264Cfg.QualityControl.IBeatMode);

                        if (EncMonitorEnable && (VideoEncMgt[EncModeIdx].BrcMode == VIDEOENC_SMART_VBR)) {
                            AMBA_IMG_ENC_MONITOR_BRC_HDLR_CFG_s EncMonCfg = {0};
                            AMBA_IMG_ENC_MONITOR_BITRATE_HDLR_s *BrcHdlr = NULL;
                            BrcHdlr = (Channel == 0)? BrcHdlrPri: BrcHdlrSec;
                            EncMonCfg.AverageBitrate = CurrentCfg.Spec.H264Cfg.BitRateControl.AverageBitrate;
                            EncMonCfg.MinBitrate = CurrentCfg.Spec.H264Cfg.BitRateControl.MinBitrate;
                            EncMonCfg.MaxBitrate = CurrentCfg.Spec.H264Cfg.BitRateControl.MaxBitrate;
                            AmbaEncMonitorBRC_RunTimeBitRateChange(&EncMonCfg, &BrcHdlr);
                        }

                    }
                }
                    break;
                case 1: /* BitRate Change */
                    if (argc == 6) {
                        RealTimeCtrl->Cmd |= RC_BITRATE;
                        RealTimeCtrl->BitRate = (UINT32) (atoi(argv[5])*1E3);
                        AmbaPrint("[Amp_UT][brc] Cmd 0x%x (%d)", RealTimeCtrl->Cmd, RealTimeCtrl->BitRate);
                    } else {
                        return -1;
                    }
                    break;
                case 2: /* Gop Change */
                    if (argc == 8) {
                        if (Status == STATUS_ENCODE) {
                            RealTimeCtrl->Cmd |= RC_GOP;
                            RealTimeCtrl->M =  atoi(argv[5]);
                            RealTimeCtrl->N =  atoi(argv[6]);
                            RealTimeCtrl->IDR =  atoi(argv[7]);
                            AmbaPrint("[Amp_UT][brc] Cmd 0x%x (%d, %d, %d)", RealTimeCtrl->Cmd,\
                                RealTimeCtrl->M, RealTimeCtrl->N, RealTimeCtrl->IDR);
                        } else if (Status == STATUS_LIVEVIEW) {
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
                case 3: /* Qp Range */
                    if (argc == 11) {
                        if (Status == STATUS_ENCODE) {
                            RealTimeCtrl->Cmd |= RC_QP;
                            RealTimeCtrl->QpMinI = atoi(argv[5]);
                            RealTimeCtrl->QpMaxI = atoi(argv[6]);
                            RealTimeCtrl->QpMinP = atoi(argv[7]);
                            RealTimeCtrl->QpMaxP = atoi(argv[8]);
                            RealTimeCtrl->QpMinB = atoi(argv[9]);
                            RealTimeCtrl->QpMaxB = atoi(argv[10]);
                            AmbaPrint("[Amp_UT][brc] Cmd 0x%x (%d,%d,%d %d %d %d)", RealTimeCtrl->Cmd,\
                                    RealTimeCtrl->QpMinI, RealTimeCtrl->QpMaxI, RealTimeCtrl->QpMinP,\
                                    RealTimeCtrl->QpMaxP, RealTimeCtrl->QpMinB, RealTimeCtrl->QpMaxB);
                        } else if (Status == STATUS_LIVEVIEW) {
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
                case 4: /* AQp Range */
                    if (argc == 15){
                        if (Status == STATUS_ENCODE) {
                            RealTimeCtrl->Cmd |= RC_QMODEL;
                            RealTimeCtrl->AQPStrength = atoi(argv[5]);
                            RealTimeCtrl->Intra16x16Bias = atoi(argv[6]);
                            RealTimeCtrl->Intra4x4Bias = atoi(argv[7]);
                            RealTimeCtrl->Inter16x16Bias = atoi(argv[8]);
                            RealTimeCtrl->Inter8x8Bias = atoi(argv[9]);
                            RealTimeCtrl->Direct16x16Bias = atoi(argv[10]);
                            RealTimeCtrl->Direct8x8Bias = atoi(argv[11]);
                            RealTimeCtrl->MELambdaQpOffset = atoi(argv[12]);
                            RealTimeCtrl->Alpha = atoi(argv[13]);
                            RealTimeCtrl->Beta = atoi(argv[14]);
                        } else if (Status == STATUS_LIVEVIEW) {
                            AMP_VIDEOENC_BITSTREAM_CFG_s CurrentCfg;
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
                case 5:
                    break;
                case 6: /* HQP */
                    if (argc == 9) {
                        RealTimeCtrl->Cmd |= RC_HQP;
                        RealTimeCtrl->HPNumber = atoi(argv[5]);
                        RealTimeCtrl->HQpMax = atoi(argv[6]);
                        RealTimeCtrl->HQpMin = atoi(argv[7]);
                        RealTimeCtrl->HQpReduce = atoi(argv[8]);
                        AmbaPrint("[Amp_UT][brc] Cmd 0x%x (%d %d %d %d)", RealTimeCtrl->Cmd, RealTimeCtrl->HPNumber,\
                                RealTimeCtrl->HQpMax, RealTimeCtrl->HQpMin, RealTimeCtrl->HQpReduce);
                    } else {
                        return -1;
                    }
                    break;
                case 7: /* ZMV */
                    if (argc == 6) {
                        RealTimeCtrl->Cmd |= RC_ZMV;
                        RealTimeCtrl->ZmvThres = atoi(argv[5]);
                        AmbaPrint("[Amp_UT][brc] Cmd 0x%x (%d)", RealTimeCtrl->Cmd, RealTimeCtrl->ZmvThres);
                    } else {
                        return -1;
                    }
                    break;
                case 8: /* Force IDR */
                    if (argc == 5){
                        AmbaPrint("[Amp_UT] Force Idr can only invoke individually");
                        RealTimeCtrl->Cmd = RC_FORCE_IDR;
                        AmbaPrint("[Amp_UT][brc] Cmd 0x%x", RealTimeCtrl->Cmd);
                    } else {
                        return -1;
                    }
                    break;
                case 9: /* Reset all */
                    if (argc == 5) {
                        memset(RealTimeCtrl, 0x0, sizeof(AMP_VIDEOENC_RUNTIME_QUALITY_CFG_s));
                    }
                    break;
                default:
                    break;
                }
            }
        } else if ((strcmp(argv[2],"invoke") == 0)) {
            if (argc < 4) {
                AmbaPrint("Usage: t videoenc brc invoke [stream]");
                AmbaPrint("                         0: Primary");
                AmbaPrint("                         1: Secondary");
            } else {
                if (Status != STATUS_ENCODE) {
                    AmbaPrint("Wrong Status %d", Status);
                    return 0;
                }
                if (atoi(argv[3]) == 0) {
                    AmpVideoEnc_SetRuntimeQuality(VideoEncPri, &RealTimeQualityCfg[0]);
                    AmbaPrint("[Amp_UT] invoke brc to primary channel");
                } else if (atoi(argv[3]) == 1) {
                    AmpVideoEnc_SetRuntimeQuality(VideoEncSec, &RealTimeQualityCfg[1]);
                    AmbaPrint("[Amp_UT] invoke brc to secondary channel");
                }
            }
        }
#else
        if (Status != STATUS_ENCODE) {
            AmbaPrint("Wrong Status");
            return 0;
        }
        if (argc < 3) {
            AmbaPrint("Usage: t videoenc rbrc [avgBitRate]");
            AmbaPrint("                 avgBitRate: Average Bitrate. Unit: Mbps.");
            return -1;
        }
        {
            AMP_VIDEOENC_RUNTIME_BITRATE_CFG_s NewBitrateCfg;
            NewBitrateCfg.AverageBitrate = atoi(argv[2]);
            if (NewBitrateCfg.AverageBitrate > 50) {
                    AmbaPrint("Please use Mbps unit");
                    return -1;
            }
            AmbaPrint("       Change Bitrate to %dMbps", NewBitrateCfg.AverageBitrate);
            NewBitrateCfg.AverageBitrate *= 1E6;
            AmpVideoEnc_SetRuntimeBitrate(VideoEncPri, &NewBitrateCfg);
            BrcFrameCount = 0;
            BrcByteCount = 0;
        }
#endif
    } else if ((strcmp(argv[1],"brcshow") == 0)){
        AMP_VIDEOENC_ENCODING_INFO_s EncInfo;
        if (Status != STATUS_ENCODE) {
            AmbaPrint("Wrong Status %d", Status);
            return 0;
        }
        if (VideoEncPri) {
            AmpVideoEnc_GetEncodingInfo(VideoEncPri, &EncInfo);
            AmbaPrint("[AmpUT][VideoEnc][Pri]: Total Frames %d AverBitrate %dkbps TotalBytes %lldbytes", EncInfo.TotalFrames,\
                EncInfo.AverageBitrate, EncInfo.TotalBytes);
        }
        if (VideoEncSec) {
            AmpVideoEnc_GetEncodingInfo(VideoEncSec, &EncInfo);
            AmbaPrint("[AmpUT][VideoEnc][Sec]: Total Frames %d AverBitrate %dkbps TotalBytes %lldbytes", EncInfo.TotalFrames,\
                EncInfo.AverageBitrate, EncInfo.TotalBytes);
        }
        if ((strcmp(argv[2],"reset") == 0)) {
            if (VideoEncPri) AmpVideoEnc_ResetEncodingInfo(VideoEncPri);
            if (VideoEncSec) AmpVideoEnc_ResetEncodingInfo(VideoEncSec);
        }
    } else if ((strcmp(argv[1],"sc") == 0)) {
        AMBA_IMG_SCHDLR_EXP_s Exp = {0};

        if (argc < 10) {
            AmbaPrint("Usage: t videnenc sc [Type] [AGC] [Shutter] [RGain] [GGain] [BGain] [AEGain] [GlobalGain]");
            AmbaPrint("                     Type: 2 - Direct, 3 - By scheduler");
            AmbaPrint("                     Shutter: fps. Eg, 60 = 60fps");
            return -1;
        }

        Exp.Type = (AMBA_IMG_SCHDLR_SET_TYPE_e)atoi(argv[2]);
#ifdef CONFIG_SOC_A9
        Exp.Info.AGC = 1.0*atoi(argv[3]);
        Exp.Info.Shutter = 1.0/atoi(argv[4]);
#else
        Exp.Info.SubChannelNum = 0;
        Exp.Info.AGC[0] = 1.0*atoi(argv[3]);
        Exp.Info.Shutter[0] = 1.0/atoi(argv[4]);
#endif
        Exp.Info.DGain.GainR = atoi(argv[5]);
        Exp.Info.DGain.GainG = atoi(argv[6]);
        Exp.Info.DGain.GainB = atoi(argv[7]);
        Exp.Info.DGain.AeGain = atoi(argv[8]);
        Exp.Info.DGain.GlobalDGain = atoi(argv[9]);

        AmbaPrint("Type %d AGC %f Shutter %f Gain R:%d G:%d B:%d AE:%d GLOBAL:%d", Exp.Type, Exp.Info.AGC, Exp.Info.Shutter, \
            Exp.Info.DGain.GainR, Exp.Info.DGain.GainG, Exp.Info.DGain.GainB, Exp.Info.DGain.AeGain, Exp.Info.DGain.GlobalDGain);

        AmbaImgSchdlr_SetExposure((UINT32)0, &Exp);

    } else if ((strcmp(argv[1],"3a") == 0)) {
        UINT8 Is3aEnable = atoi(argv[2]);
        UINT8 i;
        UINT32 ChannelCount = 0;
        AMBA_3A_OP_INFO_s AaaOpInfo = {0};

        AmbaImg_Proc_Cmd(MW_IP_GET_TOTAL_CH_COUNT, (UINT32)&ChannelCount,0, 0);

        if (Is3aEnable == 0) {
            VideoEnc3AEnable = 0;
            AaaOpInfo.AeOp = DISABLE;
            AaaOpInfo.AfOp = DISABLE;
            AaaOpInfo.AwbOp = DISABLE;
            AaaOpInfo.AdjOp = DISABLE;
        } else if (Is3aEnable == 1) {
            VideoEnc3AEnable = 1;
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
            AmbaPrint("ChNo[%u] 3A %s: ae:%u af:%u awb:%u adj:%u", i, VideoEnc3AEnable? "Enable": "Disable", \
                AaaOpInfo.AeOp, AaaOpInfo.AfOp, AaaOpInfo.AwbOp, AaaOpInfo.AdjOp);
        }
    } else if ((strcmp(argv[1],"ta") == 0)) {
        UINT8 Enable = atoi(argv[2]);
        VideoEncTAEnable = Enable;
        AmbaPrint("TA : %s, be sure to use HybridAlgoMode", VideoEncTAEnable?"Enable":"Disable");
    } else if ((strcmp(argv[1],"rotate") == 0) || (strcmp(argv[1],"rot") == 0)) {

        if (argc != 3) {
            AmbaPrint("Usage: t videnenc rotation [Rotation] ");
        #ifdef CONFIG_SOC_A12
            AmbaPrint("       In A12 ExpressMode, PrevA will be rotated after issue encode rotate,");
            AmbaPrint("       and APP shall re-issue liveview stop/start to prevent PrevA buffer mismatch");
            AmbaPrint("       In ExpressMode, rotate is conflict to MainReSample, So rotate is forbid when RawWindow < MainWindow");
        #endif
            AmbaPrint("              Rotation: Predefined rotation value");
            AmbaPrint("                        0 - No rotation ");
            AmbaPrint("                        1 - Horizontal flip");
            AmbaPrint("                        2 - 90 clockwise");
            AmbaPrint("                        3 - 90 clockwise then verical flip");
            AmbaPrint("                        4 - 180 clockwise");
            AmbaPrint("                        5 - 180 clockwise then horizontal flip");
            AmbaPrint("                        6 - 270 clockwise");
            AmbaPrint("                        7 - 270 clockwise then vertical flip");
            AmbaPrint("                 1  2  3  4  5  6  7");
            AmbaPrint("3840x2160P30     O  X  X  O  O  X  X");
            AmbaPrint("2560x1920P30     O  X  X  O  O  X  X");
            AmbaPrint("2560x1440P30     O  X  X  O  O  X  X");
            AmbaPrint("2560x1440P60     O  X  X  O  O  X  X");
            AmbaPrint("1920x1080P60     O  O  O  O  O  O  O");
            AmbaPrint("1920x1440P60     O  O  O  O  O  O  O");
            AmbaPrint("1920x1440P30     O  O  O  O  O  O  O");
            AmbaPrint("1920x1080P30     O  O  O  O  O  O  O");
            AmbaPrint("1920x1080P120    X  X  X  X  X  X  X");
            AmbaPrint("1280x720 P240    X  X  X  X  X  X  X");
            AmbaPrint("1080P60 DualHD   O  X  X  O  O  X  X");
            AmbaPrint("1080P30 DualHD   O  O  O  O  O  O  O");
            return -1;
        }

        if (Status == STATUS_ENCODE || Status == STATUS_PAUSE) {
            AmbaPrint("Wrong Status");
            return 0;
        } else if (Status == STATUS_LIVEVIEW) {
            AMP_ROTATION_e PreviousRotate = EncRotation;
            EncRotation = (AMP_ROTATION_e)atoi(argv[2]);

            if (AmpUT_VidoeEnc_CheckCapability()) {
                AmpUT_VideoEnc_LiveviewStop();
                AmpUT_VideoEnc_LiveviewStart(EncModeIdx);
                if (EncDateTimeStampPri) {
                    AMP_VIDEOENC_BLEND_INFO_s BlendInfo = {0};
                    BlendInfo.Enable = 1;
                    BlendInfo.BufferID = 0;  // Unit Test only use one blend area, we choose ID = 0
                    BlendInfo.OffsetX = 200;
                    BlendInfo.OffsetY = 200;
                    BlendInfo.Pitch = 256;
                    BlendInfo.Width = 256;
                    BlendInfo.Height = 100;
                    BlendInfo.YAddr = EncBlendY;
                    BlendInfo.UVAddr = EncBlendUV;
                    BlendInfo.AlphaYAddr = EncBlendAlphaY;
                    BlendInfo.AlphaUVAddr = EncBlendAlphaUV;

                    AmpVideoEnc_SetEncodeBlend(VideoEncPri, &BlendInfo);
                }
                if (EncDateTimeStampSec) {
                    AMP_VIDEOENC_BLEND_INFO_s BlendInfo = {0};
                    BlendInfo.Enable = 1;
                    BlendInfo.BufferID = 0;  // Unit Test only use one blend area, we choose ID = 0
                    BlendInfo.OffsetX = 200;
                    BlendInfo.OffsetY = 200;
                    BlendInfo.Pitch = 256;
                    BlendInfo.Width = 256;
                    BlendInfo.Height = 100;
                    BlendInfo.YAddr = EncBlendY;
                    BlendInfo.UVAddr = EncBlendUV;
                    BlendInfo.AlphaYAddr = EncBlendAlphaY;
                    BlendInfo.AlphaUVAddr = EncBlendAlphaUV;

                    AmpVideoEnc_SetEncodeBlend(VideoEncSec, &BlendInfo);
                }
            } else {
                AmbaPrint("%s doesn't support Rotate", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
                EncRotation = PreviousRotate;
            }
        } else {
            EncRotation = (AMP_ROTATION_e)atoi(argv[2]);
        }

        AmbaPrintColor(BLUE, "[AMP_UT] Rotation %d ",EncRotation);

    } else if ((strcmp(argv[1],"timelapse") == 0) || (strcmp(argv[1],"tim") == 0)) {
        if (argc < 3) {
            AmbaPrint("Usage: t videnenc timelapse [enable]");
            AmbaPrint("                  enable: 1 - Enable, 0 - Disable");
            return -1;
        }
        EncTimeLapse = atoi(argv[2]);

        AmbaPrint("Time lapse %s", EncTimeLapse?"Enable":"Disable");

    } else if ((strcmp(argv[1],"tcap") == 0)) {
        if (Status != STATUS_ENCODE || !EncTimeLapse) {
            return -1;
        }
        if (EncThumbnail && encPriTotalFrames==0) {
            //issue PIV cmd to let SSP start turely video-encode
            AMP_VIDEOENC_PIV_CTRL_s PivCtrl;
            UINT32 iso = 1;
            UINT8 cmpr = 1;
            UINT32 tsize = 0;
            UINT8 loop = 0;

            PivVideoThm = 1;
            if (LiveViewProcMode) {
                PivVideoThmJpgCount = 2;
            } else {
                PivVideoThmJpgCount = 3;
            }
            PivCtrl.SensorMode.Data = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;;
            PivCtrl.CaptureWidth = VideoEncMgt[EncModeIdx].CaptureWidth;
            PivCtrl.CaptureHeight = VideoEncMgt[EncModeIdx].CaptureHeight;
            PivCtrl.MainWidth = VideoEncMgt[EncModeIdx].MainWidth;
            PivCtrl.MainHeight = VideoEncMgt[EncModeIdx].MainHeight;
            PivCtrl.AspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
            AmpUT_VideoEnc_PIV(PivCtrl, iso, cmpr, tsize, loop);
        }
        AmpVideoEnc_CaptureTimeLapsedFrame(VideoEncVinA);
    } else if ((strcmp(argv[1],"spec") == 0)) {
        UINT32 v1 = atoi(argv[2]);
        UINT32 v2 = atoi(argv[3]);

        if (Status == STATUS_ENCODE) {
            AmbaPrint("Wrong Status");
            return 0;
        }
        if (argc < 4) {
            AmbaPrint("Usage: t videoenc spec [PriSpec] [SecSpec]");
            AmbaPrint("                  PriSpec: Primary stream spec. 1 - H.264 (default), 0 - MotionJPEG");
            AmbaPrint("                  SecSpec: Secondary stream spec. 1 - H.264 (default), 0 - MotionJPEG");
        #ifdef CONFIG_SOC_A12
            AmbaPrint("                  A12 ExpressMode can not Support MJPEG in PriStream ");
        #endif
            return 0;
        }
        AmbaPrint("Primary Stream: %s",v1?"H.264":"MJPEG");
        AmbaPrint("Secondary Stream (if enabled): %s",v2?"H.264":"MJPEG");
        #ifdef CONFIG_SOC_A12
            if (LiveViewProcMode == 0 && v1 == 0) {
                AmbaPrint("A12 can not support MJpeg PriStream in ExpressMode, force as H264");
                EncPriSpecH264 = v1 = 1;
            } else {
                EncPriSpecH264 = v1;
            }
        #else
            EncPriSpecH264 = v1;
        #endif

        EncSecSpecH264 = v2;
        if (Status == STATUS_LIVEVIEW) {
            AmpUT_VideoEnc_LiveviewStop();
            AmpUT_VideoEnc_LiveviewStart(EncModeIdx);
        }
    }
    else if ((strcmp(argv[1],"chgfrate") == 0) || (strcmp(argv[1],"chgfr") == 0)) {
        int Num = (EncDualStream || EncDualHDStream)? 2: 1;
        AMP_VIDEOENC_RUNTIME_FRAMERATE_CFG_s Cfg[2];

        if (Status != STATUS_ENCODE) {
            return -1;
        }
        if (Num == 1) {
            if (argc < 3) {
                AmbaPrint("Usage: t videoenc chgfrate [priStreamDivisor]");
                AmbaPrint("                  PriStreamDivisor: Primary stream framerate divisor, Eg, 2 --> 1/2, 4 --> 1/4");
            }

            Cfg[0].Hdlr = VideoEncPri;
            Cfg[0].Divisor = atoi(argv[2]);
        } else if (Num == 2) {
            if (argc < 4) {
                AmbaPrint("Usage: t videoenc chgfrate [priStreamDivisor] [secStreamDivisor]");
                AmbaPrint("                  PriStreamDivisor: Primary stream framerate divisor, Eg, 2 --> 1/2, 4 --> 1/4");
                AmbaPrint("                  SecStreamDivisor: Secondary stream framerate divisor (if dualstream is enabled), Eg, 2 --> 1/2, 4 --> 1/4");
            }

            Cfg[0].Hdlr = VideoEncPri;
            Cfg[1].Hdlr = VideoEncSec;
            Cfg[0].Divisor = atoi(argv[2]);
            Cfg[1].Divisor = atoi(argv[3]);
        }

        AmpVideoEnc_SetRuntimeFrameRate(Num, Cfg);

    }  else if ((strcmp(argv[1],"vinencchg") == 0) || (strcmp(argv[1],"chg") == 0)) {
        if (argc < 3) {
            AmbaPrint("Usage: t videoenc liveviewstart [modeId]");
            if (VideoEncMgt == NULL) {
                AmbaPrintColor(RED, "Please hook sensor first!!!");
                return -1;
            }
            AmbaPrint("[%s] mode_id: ", MWUT_GetInputDeviceName(InputDeviceId));
            MWUT_InputVideoModePrintOutAll(VideoEncMgt);
            return -1;
        } else {
            UINT8 NewEncodeModeID;
            UINT8 PreviousEncodeMode = EncModeIdx;

            if (Status != STATUS_LIVEVIEW)
                return 0;

            EncModeIdx = NewEncodeModeID = atoi(argv[2]);
            if (VideoEncMgt[NewEncodeModeID].ForbidMode) {
                AmbaPrint("Not Suppot this Mode (%d)", NewEncodeModeID);
                return -1;
            }
            if (VideoEncMgt[NewEncodeModeID].PALModeOnly && EncodeSystem == 0 && \
                (EncDualHDStream || EncDualStream))  {
                AmbaPrintColor(RED, "Only Support PAL mode !!!!!");
                return -1;
            }
            if (AmpUT_VidoeEnc_CheckCapability() == 0) {
                EncModeIdx = PreviousEncodeMode;
                AmbaPrint("%s doesn't support this mode", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
                return -1;
            }

            AmpUT_VideoEnc_ChangeResolution(NewEncodeModeID);
            AmbaPrintColor(BLUE,"[AMP_UT] Mode change done %d", NewEncodeModeID);
        }
    } else if (strcmp(argv[1],"piv") == 0) {
        if (Status != STATUS_ENCODE) {
            return 0;
        }
    #ifdef CONFIG_SOC_A9
        if (TvLiveview == 1) {
            AmbaPrint("PIV can not support in dual Vout, please disable TV first");
            AmbaPrint("t videoenc tv 0 before liveview start");
            return 0;
        }
    #endif

        if (argc < 8) {
            AmbaPrint("Usage: t videoenc piv [encW][encH][iso][cmpr][targetSize][encodeLoop]");
            AmbaPrint("             encW: encode width");
            AmbaPrint("             encH: encode height");
            AmbaPrint("             iso: algo mode, 0:HISO(Not support) 1:LISO 2:MFHISO(hack as FastMode)");
        #ifdef CONFIG_SOC_A9
            AmbaPrint("                 frameRate>30fps can not support ISO-PIV(Hiso/Liso)");
            AmbaPrint("                 frameRate>60fps can not support PIV");
        #endif
            AmbaPrint("             cmpr: compressed raw");
            AmbaPrint("             tsize: target Size in Kbyte unit");
            AmbaPrint("                   0 : default w*h/2000 without BRC control");
            AmbaPrint("             loop: Jpeg BRC re-encode loop");
            AmbaPrint("                   0~0xFE : re-encode number + 1");
            AmbaPrint("                   0xFF : for burst capture");
            return 0;
        } else {
            /*
             * From ucoder, Mode5_4k2k, when MCTF is On, L/Hiso VideoPIV is forbid.
             * since MCTF engine resource allocate is hard when video/still execute at same time
             */
            AMP_VIDEOENC_PIV_CTRL_s PivCtrl = {0};
            UINT16 EncW = atoi(argv[2]);
            UINT16 EncH = atoi(argv[3]);
            UINT32 Iso = atoi(argv[4]);
            UINT8 Cmpr = atoi(argv[5]);
            UINT32 Tsize = atoi(argv[6]);
            UINT8 Loop = atoi(argv[7]);

            PivCtrl.SensorMode.Data = (EncodeSystem == 0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;
            PivCtrl.CaptureWidth = VideoEncMgt[EncModeIdx].CaptureWidth;
            PivCtrl.CaptureHeight = VideoEncMgt[EncModeIdx].CaptureHeight;
            PivCtrl.MainWidth = EncW;
            PivCtrl.MainHeight = EncH;
            PivCtrl.AspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
            AmpUT_VideoEnc_PIV(PivCtrl, Iso, Cmpr, Tsize, Loop);
        }
    } else if (strcmp(argv[1],"thm") == 0) {
        if (Status == STATUS_ENCODE) {
            return 0;
        }
        if (argc < 3) {
            AmbaPrint("Usage: t videoenc thm [enable]");
            AmbaPrint("             enable: enable video thumbnail");
            return 0;
        } else {
            UINT8 tt = atoi(argv[2]);
            EncThumbnail = tt;
            AmbaPrint("VideoThm %s", EncThumbnail?"Enable":"Disable");
        }
    } else if (strcmp(argv[1],"dzjump") == 0) {
        AMP_IMG_DZOOM_JUMP_s TmpParamJumpDzoom;
        int DzoomRval;

        UINT32 TmpParamJumpDzoomStep = atoi(argv[2]);
        double TmpParamJumpDzoomShiftX = atof(argv[3]);
        double TmpParamJumpDzoomShiftY = atof(argv[4]);

    #ifdef CONFIG_SOC_A12
        if (VideoEnc_EisAlgoEnable) {
            AmbaPrint("Can't Support Eis + Dzoom now");
            return -1;
        }
    #endif

        TmpParamJumpDzoom.Step = TmpParamJumpDzoomStep;
        TmpParamJumpDzoom.ShiftX = (int) (TmpParamJumpDzoomShiftX * 65536);
        TmpParamJumpDzoom.ShiftY = (int) (TmpParamJumpDzoomShiftY * 65536);

        DzoomRval = AmpImgDzoom_PresetDzoomJump(dzoomHdlr, &TmpParamJumpDzoom);
        if (DzoomRval != 0) {
            AmbaPrint("AmpImgDzoom_PresetDzoomJump Error");
        }
        AmpImgDzoom_SetDzoomJump(dzoomHdlr, &TmpParamJumpDzoom);
        if (DzoomRval != 0) {
            AmbaPrint("AmpImgDzoom_SetDzoomJump Error");
        }
    } else if (strcmp(argv[1],"dzmove") == 0) {
        AMP_IMG_DZOOM_POSITION_s TmpParamPositionDzoom;
        int DzoomRval;

        UINT32  TmpParamPositionDzoomDirection = atoi(argv[2]);
        UINT32  TmpParamPositionDzoomSpeed = atoi(argv[3]);
        UINT32  TmpParamPositionDzoomDistance = atoi(argv[4]);
        double  TmpParamPositionDzoomShiftX = atof(argv[5]);    // 16.16 format
        double  TmpParamPositionDzoomShiftY = atof(argv[6]);    // 16.16 format

    #ifdef CONFIG_SOC_A12
        if (VideoEnc_EisAlgoEnable) {
            AmbaPrint("Can't Support Eis + Dzoom now");
            return -1;
        }
    #endif

        TmpParamPositionDzoom.Direction = TmpParamPositionDzoomDirection;
        TmpParamPositionDzoom.Speed = TmpParamPositionDzoomSpeed;
        TmpParamPositionDzoom.Distance = TmpParamPositionDzoomDistance;
        TmpParamPositionDzoom.ShiftX = (int) (TmpParamPositionDzoomShiftX * 65536);
        TmpParamPositionDzoom.ShiftY = (int) (TmpParamPositionDzoomShiftY * 65536);

        DzoomRval = AmpImgDzoom_SetDzoomPosition(dzoomHdlr, &TmpParamPositionDzoom);

        if (DzoomRval != 0) {
            AmbaPrint("AmpImgDzoom_SetDzoomPosition Error");
        }
    } else if (strcmp(argv[1],"dzstop") == 0) {
        int DzoomRval;

    #ifdef CONFIG_SOC_A12
        if (VideoEnc_EisAlgoEnable) {
            AmbaPrint("Can't Support Eis + Dzoom now");
            return -1;
        }
    #endif
        DzoomRval = AmpImgDzoom_StopDzoom(dzoomHdlr);

        if (DzoomRval != 0) {
            AmbaPrint("AmpImgDzoom_StopDzoom Error");
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
    } else if (strcmp(argv[1],"mpl")==0) {
        AmbaPrint("MMPL remain %d frag %d ",G_MMPL.tx_byte_pool_available,G_MMPL.tx_byte_pool_fragments);
    } else if (strcmp(argv[1],"mpl2")==0) {
        static void *tz;
        static void *TmpRawtz;
        AmpUtil_GetAlignedPool(&G_MMPL, &tz, &TmpRawtz, 100, 32);
        AmbaPrint("MMPL remain %d frag %d ",G_MMPL.tx_byte_pool_available,G_MMPL.tx_byte_pool_fragments);
        AmbaKAL_BytePoolFree(TmpRawtz);
    } else if (strcmp(argv[1],"logmuxer")==0) {
        LogMuxer = atoi(argv[2]);
        AmbaPrint("Log muxer: %s",LogMuxer?"ON":"OFF");
    } else if (strcmp(argv[1],"freq")==0) {
        AmbaPrint("---------------------------------------------");
        AmbaPrint("Cortex freq:\t\t%d", AmbaPLL_GetCortexClk());
    #ifdef CONFIG_SOC_A9
        AmbaPrint("ARM freq:\t\t%d", AmbaPLL_GetArmClk());
    #endif
        AmbaPrint("DDR freq:\t\t%d", AmbaPLL_GetDdrClk());
        AmbaPrint("IDSP freq:\t\t%d", AmbaPLL_GetIdspClk());
        AmbaPrint("Core freq:\t\t%d", AmbaPLL_GetCoreClk());
        AmbaPrint("AXI freq:\t\t%d", AmbaPLL_GetAxiClk());
        AmbaPrint("AHB freq:\t\t%d", AmbaPLL_GetAhbClk());
        AmbaPrint("APB freq:\t\t%d", AmbaPLL_GetApbClk());
        AmbaPrint("---------------------------------------------");

        AmbaPrint("----- Version info --------------------------");
        //AmbaPrint("FlowPipeVer  : %5d", AmpMW_GetFlowPipeVer());
        //AmbaPrint("DataPipeVer  : %5d", AmpMW_GetDataPipeVer());
        //AmbaPrint("SchedulerVer : %5d", AmbaImgSchdlr_GetVer());
        AmbaPrint("---------------------------------------------");

    } else if (strcmp(argv[1],"blend")==0) {
        if (argc < 4) {
            AmbaPrint("Usage: t videoenc blend [streamID] [enable]");
            AmbaPrint("             streamID: 0 - Primary stream, 1 - Secondary stream");
            AmbaPrint("             enable: enable blend");
        #ifdef CONFIG_SOC_A12
            AmbaPrint("             A12 need Yuv422 format and 32align Pitch");
        #endif
            return -1;
        } else {
            UINT8 StreamID = atoi(argv[2]);
            UINT8 BlendEnable = atoi(argv[3]);
            AMP_VIDEOENC_BLEND_INFO_s BlendInfo;

            AmbaPrint("Encode Blend [%d] %s", StreamID, BlendEnable?"Enable":"Disable");

            BlendInfo.Enable = BlendEnable;
            BlendInfo.BufferID = 0;  // Unit Test only use one blend area, we choose ID = 0
            BlendInfo.OffsetX = 200;
            BlendInfo.OffsetY = 200;
            BlendInfo.Pitch = 256;
            BlendInfo.Width = 256;
            BlendInfo.Height = 100;
            BlendInfo.YAddr = EncBlendY;
            BlendInfo.UVAddr = EncBlendUV;
            BlendInfo.AlphaYAddr = EncBlendAlphaY;
            BlendInfo.AlphaUVAddr = EncBlendAlphaUV;

            if (StreamID == 0) {
                EncDateTimeStampPri = BlendEnable;
                AmpVideoEnc_SetEncodeBlend(VideoEncPri, &BlendInfo);
                memcpy(&PIVInfo,&BlendInfo,sizeof(AMP_VIDEOENC_BLEND_INFO_s));
            } else {
                EncDateTimeStampSec = BlendEnable;
                AmpVideoEnc_SetEncodeBlend(VideoEncSec, &BlendInfo);
            }

        }
    } else if ((strcmp(argv[1],"interlace") == 0)) {
        UINT32 v1 = atoi(argv[2]);
        UINT32 v2 = atoi(argv[3]);

        if (Status == STATUS_ENCODE) {
            AmbaPrint("Wrong Status");
            return 0;
        }
        if (argc < 4) {
            AmbaPrint("Usage: t videoenc interlace [PriInt] [SecInt]");
            AmbaPrint("                  PriInt: Primary stream interlace enable. 0 - Progressive (default), 1 - Interlace");
            AmbaPrint("                  SecInt: Secondary stream interlace enable. 0 - Progressive (default), 1 - Interlace");
            return 0;
        }
        AmbaPrint("Primary Stream: %s",v1?"Interlace":"Progressive");
        AmbaPrint("Secondary Stream (if enabled): %s",v2?"Interlace":"Progressive");
        EncPriInteralce = v1;
        EncSecInteralce = v2;
        if (Status == STATUS_LIVEVIEW) {
            AmpUT_VideoEnc_ChangeResolution(EncModeIdx);
        }
    } else if (strcmp(argv[1],"calib") == 0) {
        UINT32 v1 = atoi(argv[2]);
        UINT32 v2 = atoi(argv[3]);

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc calib [calibID] [Enable]");
            AmbaPrint("                  CalibID : calibratrion ID");
            AmbaPrint("                    SBP  : 1");
            AmbaPrint("                    Warp : 2");
            AmbaPrint("                    CA   : 3");
            AmbaPrint("                    VIG  : 4");
            return 0;
        }

        if (v1 <=4 && v1) {
            if (v2) {
                CalibEnable |= (0x1<<(v1-1));
            } else {
                CalibEnable &= ~(0x1<<(v1-1));
            }
            AmbaPrint("CalibEnable 0x%X", CalibEnable);
        }
    } else if (strcmp(argv[1],"secwin") == 0) {
        UINT16 Width = atoi(argv[2]);
        UINT16 Height = atoi(argv[3]);
        if (Width > 1920) {
            AmbaPrint("SecStrmWin %dX%d does not support width > 1920", Width, Height);
        } else if (EncSecSpecH264 && (Width != ALIGN_16(Width) || Height != ALIGN_16(Height))) {
            AmbaPrint("SecStrmWin %dX%d should be 16-alignment for H.264", Width, Height);
        } else {
            AmbaPrint("SecStrmWin %dX%d", Width, Height);
            SecStreamCustomWidth = Width;
            SecStreamCustomHeight = Height;
        }
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
    } else if (strcmp(argv[1],"system") == 0) {
        UINT8 v1 = atoi(argv[2]);
        AmbaPrint("Encode System is %s", v1?"PAL": "NTSC");
        EncodeSystem = v1;
    } else if (strcmp(argv[1],"dumpskip") == 0) {
        UINT8 v1 = atoi(argv[2]);
        AmbaPrint("DumpSkipFlag is 0x%X", v1);
        EncodeDumpSkip = v1;
    } else if (strcmp(argv[1],"ReportRate") == 0 || strcmp(argv[1],"rate") == 0) {
        UINT8 v1 = atoi(argv[2]);
        AmbaPrint("Report rate is %d", v1);
        MainviewReportRate = v1;
    } else if (strcmp(argv[1],"mjpegquality") == 0 || strcmp(argv[1],"mjpegq") == 0) {
        MjpegQuality = atoi(argv[2]);
    } else if (strcmp(argv[1],"mjpegfmt") == 0) {
        MjpegFmt = atoi(argv[2]);
#ifdef CONFIG_SOC_A12
    } else if (strcmp(argv[1],"encmonitor") == 0 || strcmp(argv[1],"emon") == 0) {
        EncMonitorEnable = atoi(argv[2]);
        AmbaPrint("Encode Monitor %s !", EncMonitorEnable?"Enable":"Disable");
    } else if (strcmp(argv[1],"encmonitoraqp") == 0 || strcmp(argv[1],"emonaqp") == 0) {
        EncMonitorAQPEnable = atoi(argv[2]);
        AmbaPrint("Encode Monitor AQP %s !", EncMonitorAQPEnable?"Enable":"Disable");
#endif
    } else if (strcmp(argv[1],"VideoOB") == 0) {
        VideoOBModeEnable = atoi(argv[2]);
        AmbaPrint("VideoOB mode %s !", VideoOBModeEnable?"Enable":"Disable");
    } else if (strcmp(argv[1],"suspend") == 0) {
        DspSuspendEnable = atoi(argv[2]);
        AmbaPrint("%s Dsp suspend in LiveveiwStop !", DspSuspendEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"wirelessmode") == 0) {
        WirelessModeEnable = atoi(argv[2]);
        AmbaPrint("%s Enter Wireless mode in LiveveiwStop !", WirelessModeEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"activeslot") == 0) {
        if(*argv[2] != 'C' && *argv[2] != 'I') {
            AmbaPrint("Active slot only supports C (SD card) or I (USB Host)!");
            return NG;
        }
        DefaultSlot[0] = *argv[2];
        AmbaPrint("Change active slot to %s !", DefaultSlot);
#ifdef CONFIG_SOC_A12
    } else if (strcmp(argv[1],"eis") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t videoenc eis [cmd] ...");
            AmbaPrint("    cmd:");
            AmbaPrint("       hook: hook imu driver");
            AmbaPrint("       devinfo: get device information");
            AmbaPrint("       deven: enable imu device or not");
            AmbaPrint("       devstatus: get device real time status");
            AmbaPrint("       algoen: enable eis algo or not");
            return 0;
        }
        if (strcmp(argv[2],"hook") == 0) {
            UINT16 EisId = atoi(argv[3]);
            UINT16 EisSensorId = atoi(argv[4]);

            if (argc < 5) {
                AmbaPrint("Usage: t videoenc eis hook [EisID] [SensorID]");
                AmbaPrint("                  EisID :");
                AmbaPrint("                    Dummy    : 0");
                AmbaPrint("                    MPU6500  : 1");
                AmbaPrint("                    ICM20608 : 2");
                AmbaPrint("                  SensorID :");
                AmbaPrint("                    Sensor_0 : 0");
                AmbaPrint("                    Sensor_1 : 1");
                return 0;
            }

            if (EisId == 0) {
                AmbaIMU_Hook(EisSensorId, &AmbaIMU_DMY0000Obj);
                AmbaPrint("Hook Dummy IMU to SensorId 0x%X", EisSensorId);
            } else if (EisId == 1) {
                AmbaIMU_Hook(EisSensorId, &AmbaIMU_MPU6500Obj);
                AmbaPrint("Hook MPU6500 IMU to SensorId 0x%X", EisSensorId);
            } else if (EisId == 1) {
                AmbaIMU_Hook(EisSensorId, &AmbaIMU_ICM20608Obj);
                AmbaPrint("Hook ICM20608 IMU to SensorId 0x%X", EisSensorId);
            } else {
                AmbaIMU_Hook(EisSensorId, &AmbaIMU_DMY0000Obj);
                AmbaPrint("Hook Dummy IMU to SensorId 0x%X", EisSensorId);
            }
        } else if (strcmp(argv[2],"devinfo") == 0) {
            AMBA_IMU_INFO_s ImuInfo = {0};
            UINT16 EisSensorId = atoi(argv[3]);

            if (argc < 4) {
                AmbaPrint("Usage: t videoenc eis devinfo [SensorID]");
                AmbaPrint("                  SensorID :");
                AmbaPrint("                    Sensor_0 : 0");
                AmbaPrint("                    Sensor_1 : 1");
                return 0;
            }

            AmbaIMU_GetDeviceInfo(EisSensorId, &ImuInfo);

            AmbaPrint("========== SensorID 0x%X ==========", EisSensorId);
            AmbaPrint("  ID      : %d", ImuInfo.GyroID);
            AmbaPrint("  GPIO    : (%d, %d, %d)", ImuInfo.GyroPwrGpio, ImuInfo.GyroHpsGpio, ImuInfo.GyroIntGpio);
            AmbaPrint("  Chan    : (%d, %d, %d, %d)", ImuInfo.GyroXChan, ImuInfo.GyroYChan, ImuInfo.GyroZChan, ImuInfo.GyroTChan);
            AmbaPrint("  Reg     : (%d, %d, %d)", ImuInfo.GyroXReg, ImuInfo.GyroYReg, ImuInfo.GyroZReg);
            AmbaPrint("  Polar   : (%d, %d, %d)", ImuInfo.GyroXPolar, ImuInfo.GyroYPolar, ImuInfo.GyroZPolar);
            AmbaPrint("  AccChan : (%d, %d, %d)", ImuInfo.AccXChan, ImuInfo.AccYChan, ImuInfo.AccZChan);
            AmbaPrint("  AccPolar: (%d, %d, %d)", ImuInfo.AccXPolar, ImuInfo.AccYPolar, ImuInfo.AccZPolar);
            AmbaPrint("==================================");
        } else if (strcmp(argv[2],"deven") == 0) {
            UINT16 Enable = atoi(argv[3]);
            UINT16 EisSensorId = atoi(argv[4]);

            if (argc < 5) {
                AmbaPrint("Usage: t videoenc eis deven [Enable][SensorID]");
                AmbaPrint("                  SensorID :");
                AmbaPrint("                    Sensor_0 : 0");
                AmbaPrint("                    Sensor_1 : 1");
                return 0;
            }

            if (Enable) {
                AmbaIMU_Open(EisSensorId);
            } else {
                AmbaIMU_Close(EisSensorId);
            }
            AmbaPrint("%s IMU of SensorId 0x%X", (Enable)?"Enable":"Disable", EisSensorId);
        } else if (strcmp(argv[2],"devstatus") == 0) {
            AMBA_IMU_DATA_s Data = {0};
            UINT16 ImuType = atoi(argv[3]);
            UINT16 EisSensorId = atoi(argv[4]);

            if (argc < 5) {
                AmbaPrint("Usage: t videoenc eis devstatus [Type][SensorID]");
                AmbaPrint("                  Type :");
                AmbaPrint("                    NULL : 0");
                AmbaPrint("                    Gyro : 1");
                AmbaPrint("                    Acc  : 2");
                AmbaPrint("                    Combo: 3");
                AmbaPrint("                  SensorID :");
                AmbaPrint("                    Sensor_0 : 0");
                AmbaPrint("                    Sensor_1 : 1");
                return 0;
            }

            AmbaIMU_Read(EisSensorId, ImuType, &Data);
            AmbaPrint("======= IMU of SensorId 0x%X =======", EisSensorId);
            AmbaPrint("  Type    : %d", ImuType);
            AmbaPrint("  GPIO    : (%d, %d, %d)", Data.GyroXData, Data.GyroYData, Data.GyroZData);
            AmbaPrint("  Thermal : %d", Data.TemperatureData);
            AmbaPrint("  Acc     : (%d, %d, %d)", Data.AccXData, Data.AccYData, Data.AccZData);
            AmbaPrint("==================================");
        } else if (strcmp(argv[2],"algoen") == 0) {
            UINT8 Enable = atoi(argv[3]);

            if (argc < 4) {
                AmbaPrint("Usage: t videoenc eis algoen [Enable]");
                return 0;
            }

            VideoEnc_EisAlgoEnable = Enable;
            AmbaPrint("%s EIS Algo", (Enable)?"Enable":"Disable", VideoEnc_EisAlgoEnable);
        }
#endif
    } else if (strcmp(argv[1],"slowshutter") == 0 || strcmp(argv[1],"ss") == 0){
        SlowShutterEnable = (UINT8)atoi(argv[2]);
        AmbaPrint("%s slowshutter", (SlowShutterEnable)?"Enable":"Disable");
        if (Status == STATUS_LIVEVIEW || \
            Status == STATUS_ENCODE || \
            Status == STATUS_PAUSE) {
            AE_CONTROL_s AeCtrlMode;
            UINT32 FrameRate = 0;
            AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};
            UINT32 MinFrameTime;

            AmbaSensor_GetStatus(VinChannel, &SensorStatus);
            MinFrameTime = SensorStatus.ModeInfo.MinFrameRate.TimeScale/SensorStatus.ModeInfo.MinFrameRate.NumUnitsInTick;
            if (EncodeSystem == 0) {
                FrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;
            } else {
                FrameRate = VideoEncMgt[EncModeIdx].TimeScalePAL/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
            }
            AmbaImg_Proc_Cmd(MW_IP_GET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);
            if (SlowShutterEnable && (FrameRate >= 50 )) {
                UINT8 MaxSSRatio;
                MaxSSRatio = EncFrameRate/MinFrameTime;
                if (MaxSSRatio>=4) {
                    AeCtrlMode.SlowShutter = 2;
                } else if (MaxSSRatio>=2){
                    AeCtrlMode.SlowShutter = 1;
                } else {
                    AeCtrlMode.SlowShutter = 0;
                }
            } else {
                AeCtrlMode.SlowShutter = 0;
            }
            AmbaImg_Proc_Cmd(MW_IP_SET_MULTI_AE_CONTROL_CAPABILITY, 0, (UINT32)&AeCtrlMode,0);
        }
    } else if (strcmp(argv[1],"dspwork") == 0) {
        UINT32 WorkSize = atoi(argv[2]);

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc dspwork [size]");
            AmbaPrint("                    size : In MByte unit");
            return 0;
        }
        VideoEncCusomtDspWorkSize = WorkSize;
        AmbaPrint("Set Dsp work size : %d MByte", WorkSize);
    } else {
        AmbaPrint("Usage: t videoenc cmd ...");
        AmbaPrint("    cmd:");
        AmbaPrint("       init: init all");
        AmbaPrint("       liveviewproc: Liveview Proc Config");
        AmbaPrint("       liveviewstart: start liveview");
        AmbaPrint("       encstart [millisecond]: recording for N milliseconds, N = 0 will do continuous encoding");
        AmbaPrint("       encstop: stop recording");
        AmbaPrint("       pause: pause recording");
        AmbaPrint("       resume: resume recording");
        AmbaPrint("       chg: change sensor resolution (when liveview)");
        AmbaPrint("       brc: set bitrate (h264 only)");
        AmbaPrint("       rbrc: set average bitrate during recording (h264 only)");
        AmbaPrint("       brcshow: show current bitrate info during recording");
        AmbaPrint("       initzoom: set initial dzoom of next liveview");
        AmbaPrint("       piv: do videoEnc PIV(only support single VOut)");
        AmbaPrint("       thm: do videoEnc with thumbnail");
        AmbaPrint("       spec: Select bitstream specification");
        AmbaPrint("       rotate: Set encode rotation");
        AmbaPrint("       dzjump: Dzoom jumping");
        AmbaPrint("       dzmove: Dzoom moving");
        AmbaPrint("       dzstop: Dzoom stop during dzjump or dzmove");
        AmbaPrint("       dualhd: DualHD mode");
        AmbaPrint("       blend: Bitstream blending");
        AmbaPrint("       chgfrate: Change framerate during encoding");
        AmbaPrint("       timelapse: Timelapse recording enable");
        AmbaPrint("       tcap: Capture+encode one frame during timelapse recording");
        AmbaPrint("       interlace: Encable interlace encode");
    }
    return 0;
}

int AmpUT_VideoEncTestAdd(void)
{
    AmbaPrint("Adding AmpUT_VideoEnc");
    // hook command
    AmbaTest_RegisterCommand("videoenc", AmpUT_VideoEncTest);
    AmbaTest_RegisterCommand("ve", AmpUT_VideoEncTest);

   // AmpUT_VideoEnc_Init(0);
   // AmpUT_VideoEnc_LiveviewStart();

    return AMP_OK;
}
