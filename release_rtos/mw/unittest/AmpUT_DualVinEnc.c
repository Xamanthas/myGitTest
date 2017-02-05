 /**
  * @file src/app/sample/unittest/AmpUT_DualVinEnc.c
  *
  * Dual Vin Video Encode/Liveview unit test
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
#include "AmbaUTCalibMgr.h"

#include <AmbaImg_Proc.h>
#include <AmbaImg_Impl_Cmd.h>
#include <AmbaImg_VIn_Handler.h>
#include <AmbaImg_VDsp_Handler.h>
#include <math.h>
#include <recorder/AudioEnc.h>
#include "AmpUT_VideoIsoConfig.h"
#include "AmbaDSP_ImgHighIsoFilter.h"
#include "AmbaImgCalibItuner.h"
#include "AmbaTUNE_HdlrManager.h"

#include "encMonitor.h"
#include "encMonitorService.h"
#include "encMonitorStream.h"

#include <AmbaImg_AaaDef.h>

static char DefaultSlot[] = "C";

/**
 * message queue structure
 */
typedef struct DUAL_VIDENC_MSG_S_ {
    UINT32 Event;     /**< Equal to AMP_FIFO_EVENT_e */
    UINT32 Data[3];   /**< Data, casted according to event */
} DUAL_VIDENC_MSG_S;

/**
 * encode callback event
 */
typedef enum _DUAL_VIDENC_EVENT_e_ {
    // Codec event
    DUAL_VIDENC_MSG_ILLEGAL_SIGNAL = 0x100
} DUAL_VIDENC_EVENT_e;

static AMBA_KAL_MSG_QUEUE_t DualVideoEncMqueue;
static UINT8 *DualVideoEncMqueuePool = NULL;            // Mqueue for DualVideoEnc task
static AMBA_KAL_TASK_t DualVideoEncTask = {0};          // DualVideoEnc task
static UINT8 *DualVideoEncTaskStack = NULL;            // Stack for DualVideoEnc task

void *UT_DualVinEncodefopen(const char *pFileName, const char *pMode, BOOL8 AsyncMode)
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

int UT_DualVinEncodefclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_DualVinEncodefwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_DualVinEncodefsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

UINT32 UT_DualVinFrameRateIntConvert(UINT32 OriFrameRate)
{
    UINT32 FrameRateInt = OriFrameRate;

    if (OriFrameRate==29 || OriFrameRate==59 || OriFrameRate==119 || OriFrameRate==239) {
        FrameRateInt++;
    }

    return FrameRateInt;
}


// Global var for VideoEnc codec
static AMP_VIN_HDLR_s *VideoEncVinA = NULL;       // Vin instance Main
static AMP_VIN_HDLR_s *VideoEncVinPip = NULL;       // Vin instance Pip

static AMP_AVENC_HDLR_s *VideoEncPri = NULL;      // Primary VideoEnc codec instance
static AMP_AVENC_HDLR_s *VideoEncSec = NULL;      // Secondary VideoEnc codec instance
static AMP_AVENC_HDLR_s *VideoPipEncPri = NULL;      // Primary VideoEnc codec instance
static AMP_AVENC_HDLR_s *VideoPipEncSec = NULL;      // Secondary VideoEnc codec instance

static AMP_ENC_PIPE_HDLR_s *VideoEncPipe = NULL;  // Encode pipeline  instance
static AMBA_IMG_SCHDLR_HDLR_s *ImgSchdlr = NULL;    // Image scheduler instance
static AMBA_IMG_SCHDLR_HDLR_s *PIPImgSchdlr = NULL;    // Image scheduler instance
static AMBA_IMG_ENC_MONITOR_BITRATE_HDLR_s *BrcHdlrPri = NULL;  // Pri Stream BitRateMonitorControl instance
static AMBA_IMG_ENC_MONITOR_BITRATE_HDLR_s *BrcHdlrSec = NULL;  // Sec Stream BitRateMonitorControl instance
static AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *EncMonitorStrmHdlrPri = NULL;  // Pri Stream in encode monitor instance
static AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *EncMonitorStrmHdlrSec = NULL;  // Sec Stream in encode monitor instance
static AMBA_IMG_ENC_MONITOR_AQP_HDLR_s *AqpHdlrPri = NULL;  // Pri Stream AqpMonitorControl instance
static AMBA_IMG_ENC_MONITOR_AQP_HDLR_s *AqpHdlrSec = NULL;  // Sec Stream AqpMonitorControl instance
static UINT8 *VinWorkBuffer = NULL;               // Vin module working buffer
static UINT8 *VEncWorkBuffer = NULL;              // VideoEnc working buffer
static UINT8 *ImgSchdlrWorkBuffer = NULL;         // Img scheduler working buffer
static UINT8 *EncMonitorCyclicWorkBuffer = NULL;  // Encode monitor Cyclic working buffer
static UINT8 *EncMonitorServiceWorkBuffer = NULL; // BitRateMonitorControl working buffer
static UINT8 *EncMonitorServiceAqpWorkBuffer = NULL; // AQpMonitorControl working buffer
static UINT8 *EncMonitorStrmWorkBuffer = NULL;    // Encode monitor stream working buffer
static AMBA_DSP_CHANNEL_ID_u VinChannel;          // Vin channel
static AMBA_DSP_CHANNEL_ID_u VinPipChannel;       // Second Vin channel

#define STATUS_UNKNOWN  0
#define STATUS_IDLE     1
#define STATUS_INIT     2
#define STATUS_LIVEVIEW 3
#define STATUS_ENCODE   4
#define STATUS_PAUSE    5
static UINT8 Status = STATUS_UNKNOWN;                  // Simple state machine to avoid wrong command issuing


#ifdef CONFIG_LCD_WDF9648W
extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;
#endif


static UINT8 TvLiveview = 1;                        // Enable TV liveview or not
static UINT8 LCDLiveview = 1;                       // Enable LCD liveview or not
static UINT8 VideoEnc3AEnable = 1;                  // Enable AE/AWB/ADJ algo or not
static UINT8 VideoEncTAEnable[MAX_VIN_NUM] = {0};   // Enable TA algo or not in HybridAlgoMode
static UINT8 EncPriSpecH264 = 1;                    // Primary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 EncSecSpecH264 = 1;                    // Secondary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 EncPriInteralce = 0;                   // Primary codec instance output type. 0 = Progressive, 1 = Interlave
static UINT8 EncSecInteralce = 0;                   // Secondary codec instance output type. 0 = Progressive, 1 = Interlave
static UINT8 DualEncThumbnail[MAX_VIN_NUM] = {0};   // Enable video thumbnail or not
static UINT8 EncDualStream = 0;                     // Enable dual stream or not
static UINT8 EncDualHDStream = 0;                   // Enable Dual HD stream or not
static UINT8 EncDateTimeStampPri = 0;               // Enable Primary stream Date time stamp or not
static UINT8 EncDateTimeStampSec = 0;               // Enable Secondary stream Date time stamp or not
static UINT8 EncPipDateTimeStampPri = 0;            // Enable PIP Primary stream Date time stamp or not
static UINT8 EncPipDateTimeStampSec = 0;            // Enable PIP Secondary stream Date time stamp or not
static UINT8 EncIBeat = 0;                          // Enable I-beating RC
static UINT32 InitZoomX = 1<<16;                    // Initial Horizontal dzoom factor
static UINT32 InitZoomY = 1<<16;                    // Initial Vertical dzoom factor
static AMP_ROTATION_e EncRotation = AMP_ROTATE_0;   // Video encode rotation
static AMP_VIDEOENC_H264_STOP_METHOD_e EncStopMethod = AMP_VIDEOENC_STOP_NEXT_IP; // Video encode stop method
static UINT8 LiveViewProcMode = 0;                  // LiveView Process Mode, 0: Express 1:Hybrid
static UINT8 LiveViewAlgoMode = 0;                  // LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
static UINT8 LiveViewOSMode = 1;                    // LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 LiveViewHdrMode[2] = {0,0};              // LiveView HDR Mode in ExpressMode, 0: Disable 1:Enable,
static UINT8 ExtLiveViewProcMode = 0xFF;            // External LiveView Process Mode, 0: Express 1:Hybrid
static UINT8 ExtLiveViewAlgoMode = 0xFF;            // External LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
static UINT8 ExtLiveViewOSMode = 0xFF;              // External LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
static UINT16 CalibEnable = 0;                      // Calibration data enable or not
#define CALIB_SBP   0x1
#define CALIB_WARP  0x1<<1
#define CALIB_CA    0x1<<2
#define CALIB_VIG   0x1<<3
static UINT16 CustomIdspFreq = AMP_SYSTEM_FREQ_POWERSAVING;
static UINT16 CustomCoreFreq = AMP_SYSTEM_FREQ_POWERSAVING;
static UINT16 CustomCortexFreq = AMP_SYSTEM_FREQ_POWERSAVING;
static UINT16 CustomMaxIdspFreq = A12_IDSP_PLL_MAX;
static UINT16 CustomMaxCoreFreq = A12_CORE_PLL_MAX;
static UINT16 CustomMaxCortexFreq = A12_CORTEX_PLL_MAX;
static UINT8 EncodeSystem = 0;                      // Encode System, 0:NTSC, 1: PAL
static UINT8 EncodeDumpSkip = 0;                    // Encode Skip dump file, write data to SD card or just lies to fifo that it has muxed pending data
#define VIDEO_ENC_SKIP_PRI   0x1
#define VIDEO_ENC_SKIP_SEC   0x1<<1
#define VIDEO_ENC_SKIP_JPG   0x1<<7
static UINT8 MainviewReportRate = 0;                // vcap report interval, 1: every vcap; 2:every two vcap....

static UINT8 EncMonitorEnable = 0/*1*/;                  // Enable encode monitor or not
static UINT8 EncMonitorAQPEnable = 0/*1*/;               // Enable encode monitor AQP or not
#define VIN_MAIN_ONLY   0
#define VIN_PIP_ONLY    1
#define VIN_MAIN_PIP    2
static UINT8 DualVideoVinSelect = VIN_PIP_ONLY;          // Vin Select, 0: default main only , 1: pip only, 2: both enabled
static UINT8 DualStillVinSelect = VIN_MAIN_ONLY; // In dual vin encode, this flag is for PIV
#define B5_OV4689_PIP_SENSOR_CH_ID  4

static UINT8 VideoOBModeEnable = 0;
static UINT8 DspSuspendEnable = 0;
static UINT8 WirelessModeEnable = 0;

static UINT8 VideoEncIsIqParamInit = 0;
static UINT8 VideoEncIsHdrIqParam = 0;

static UINT8 VideoEncPipIsIqParamInit = 0;
static UINT8 VideoEncPipIsHdrIqParam = 0;
static UINT8 InputDeviceId = 0;
static UINT8 InputPipDeviceId = 0;





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
UINT8 DualVinVideoPIVQTable[3][128] = {
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


static UINT8 EncBlendY[256*100];                    // Y buffer for blending test
static UINT8 EncBlendAlphaY[256*100];               // Alpha Y buffer for blending test
static UINT8 EncBlendUV[256*100];                 // UV buffer for blending test
static UINT8 EncBlendAlphaUV[256*100];            // Alpha UV buffer for blending test

#define CAPTURE_MODE_NONE               0
static UINT8 CaptureMode = CAPTURE_MODE_NONE;

// Global var for StillEnc codec
static UINT8 *StillEncWorkBuf = NULL;
static AMP_STLENC_HDLR_s *StillEncPri = NULL;
static AMP_ENC_PIPE_HDLR_s *StillEncPipe = NULL;
static UINT8 StillIso = 0;
static UINT8 StillCodecInit = 0;
static UINT8 StillRawCaptureRunning = 0;
//static UINT8 StillBGProcessing = 0;
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

/* Still Codec Function prototype*/
UINT32 AmpUT_DualVinEnc_PIVPOSTCB(AMP_STILLENC_POSTP_INFO_s *info);
UINT32 AmpUT_DualVinEnc_PIVPRECB(AMP_STILLENC_PREP_INFO_s *info);
UINT32 AmpUT_DualVinEnc_DspWork_Calculate(UINT8 **addr, UINT32 *size);

static AMP_STILLENC_POSTP_s post_videoEncPiv_cb = {.Process = AmpUT_DualVinEnc_PIVPOSTCB};
static AMP_STILLENC_PREP_s pre_videoEncPiv_cb = {.Process = AmpUT_DualVinEnc_PIVPRECB};
int AmpUT_DualVinEnc_PIV(AMP_VIDEOENC_PIV_CTRL_s pivCtrl, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop);

static INPUT_ENC_MGT_s *VideoEncMgt = NULL;// Pointer to above tables
static INPUT_ENC_MGT_s *VideoPipEncMgt = NULL;// Pointer to above tables

static UINT8 EncModeIdx = 1;        // Current mode index
static UINT8 EncPipModeIdx = 1;     // Current mode index

#define GOP_N   8                   // I-frame distance
#define GOP_IDR 8                   // IDR-frame distance

static UINT16 SecStreamCustomWidth = 0;
static UINT16 SecStreamCustomHeight = 0;

/* Simple muxer to communicate with FIFO */
#define Max_PIV_Width       4096
#define Max_PIV_Height      2304

#define BITSFIFO_SIZE 32*1024*1024+(Max_PIV_Width*Max_PIV_Height)
static UINT8 *H264BitsBuf;          // H.264 bitstream buffer
static UINT8 *MjpgBitsBuf;          // MJPEG bitstream buffer
#define DESC_SIZE 40*3000
static UINT8 *H264DescBuf;          // H.264 descriptor buffer (physical)
static UINT8 *MjpgDescBuf;          // MJPEG descriptor buffer (physical)
static AMBA_KAL_TASK_t VideoEncPriMuxTask = {0};    // Primary stream muxer task
static AMBA_KAL_TASK_t VideoEncSecMuxTask = {0};    // Secondary stream muxer task
static AMBA_KAL_TASK_t VideoEncJpegMuxTask = {0};   // JPEG stream muxer task
static UINT8 *VideoEncPriMuxStack = NULL;           // Stack for primary stream muxer task
static UINT8 *VideoEncSecMuxStack = NULL;           // Stack for secondary stream muxer task
static UINT8 *VideoEncJpegMuxStack = NULL;          // Stack for JPEG(piv/thumbnail) stream muxer task
static AMBA_KAL_SEM_t VideoEncPriSem = {0};         // Counting semaphore for primary stream muxer task and fifo callback
static AMBA_KAL_SEM_t VideoEncSecSem = {0};         // Counting semaphore for secondary stream muxer task and fifo callback
static AMBA_KAL_SEM_t VideoEncJpegSem = {0};        // Counting semaphore for JPEG(piv/thumbnail) stream muxer task and fifo callback
static AMBA_KAL_TASK_t VideoPipEncPriMuxTask = {0};    // Primary stream muxer task
static AMBA_KAL_TASK_t VideoPipEncSecMuxTask = {0};    // Secondary stream muxer task
static AMBA_KAL_TASK_t VideoPipEncJpegMuxTask = {0};   // JPEG stream muxer task
static UINT8 *VideoEncPipPriMuxStack = NULL;           // Stack for primary stream muxer task
static UINT8 *VideoEncPipSecMuxStack = NULL;           // Stack for secondary stream muxer task
static UINT8 *VideoEncPipJpegMuxStack = NULL;          // Stack for JPEG(piv/thumbnail) stream muxer task
static AMBA_KAL_SEM_t VideoPipEncPriSem = {0};         // Counting semaphore for primary stream muxer task and fifo callback
static AMBA_KAL_SEM_t VideoPipEncSecSem = {0};         // Counting semaphore for secondary stream muxer task and fifo callback
static AMBA_KAL_SEM_t VideoPipEncJpegSem = {0};        // Counting semaphore for JPEG(piv/thumbnail) stream muxer task and fifo callback


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

static AMP_CFS_FILE_s *PIP_outputPriFile = NULL;        // Output file pointer for primary stream
static AMP_CFS_FILE_s *PIP_outputSecFile = NULL;        // Output file pointer for secondary stream
static AMP_CFS_FILE_s *PIP_outputJpegFile = NULL;       // Output file pointer for JPEG(piv/thumbnail) stream
static AMP_CFS_FILE_s *PIP_outputPriIdxFile = NULL;     // Output file pointer for primary stream index (frame offset/size...etc)
static AMP_CFS_FILE_s *PIP_outputSecIdxFile = NULL;     // Output file pointer for secondary stream index (frame offset/size...etc)
static AMP_CFS_FILE_s *PIP_UdtaPriFile = NULL;          // Output file pointer for primary stream UserData
static AMP_CFS_FILE_s *PIP_UdtaSecFile = NULL;          // Output file pointer for secondary stream UserData
static AMP_CFS_FILE_s *PIP_stillRawFile = NULL;         // raw Output file pointer
static AMP_CFS_FILE_s *PIP_stillYFile = NULL;           // y Output file pointer
static AMP_CFS_FILE_s *PIP_stillUvFile = NULL;          // uv Output file pointer


#define ClipMaxStorageSize  3E9
static int fnoPri = 0;                  // Frame number counter for primary stream muxer
static int FnoPriSplit = 0;                  // Frame number counter for primary stream muxer
static int fnoSec = 0;                  // Frame number counter for secondary stream muxer
static int FnoSecSplit = 0;                  // Frame number counter for primary stream muxer
static UINT16 fnoRaw = 1;               // Frame number counter for piv/thumbnail raw picture
static UINT16 fnoYuv = 1;               // Frame number counter for piv/thumbnail yuv picture
static UINT16 fnoPiv = 1;               // Frame number counter for piv/thumbnail jpeg picture
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

static int PIPfnoPri = 0;                  // Frame number counter for primary stream muxer
static int PIPFnoPriSplit = 0;                  // Frame number counter for primary stream muxer
static int PIPfnoSec = 0;                  // Frame number counter for secondary stream muxer
static int PIPFnoSecSplit = 0;                  // Frame number counter for primary stream muxer
static UINT16 PIPfnoRaw = 1;               // Frame number counter for piv/thumbnail raw picture
static UINT16 PIPfnoYuv = 1;               // Frame number counter for piv/thumbnail yuv picture
static UINT16 PIPfnoPiv = 1;               // Frame number counter for piv/thumbnail jpeg picture
static UINT32 PIPEncFrameRate = 0;         // Remember current framerate for primary muxer to calculate actual bitrate
static UINT64 PIPencPriBytesTotal = 0;     // total bytes primary stream muxer received
static UINT32 PIPencPriTotalFrames = 0;    // total frames primary stream muxer received
static UINT64 PIPencPriBytesStorageTotal = 0;     // total bytes primary stream storage received
static UINT64 PIPencSecBytesTotal = 0;     // total bytes secondary stream muxer received
static UINT32 PIPencSecTotalFrames = 0;    // total frames secondary stream muxer received
static UINT64 PIPencSecBytesStorageTotal = 0;     // total bytes primary stream storage received
static AMP_FIFO_HDLR_s *VirtualPIPPriFifoHdlr = NULL;  // Primary stream vitrual fifo
static AMP_FIFO_HDLR_s *VirtualPIPSecFifoHdlr = NULL;  // Secondary stream vitrual fifo
static AMP_FIFO_HDLR_s *VirtualPIPJpegFifoHdlr = NULL; // JPEG (piv/thumbnail) stream vitrual fifo


static UINT8 LogMuxer = 0;

/** UT function prototype */
int AmpUT_DualVinEnc_EncStop(void);
int AmpUT_DualVinEnc_ChangeResolution(UINT32 modeIdx);
int AmpUT_DualVinEnc_LiveviewStart(UINT32 modeIdx, UINT32 modePipIdx);
int AmpUT_DualVinEnc_ChangeMainView(UINT32 MainViewID);

/*************************************** Muxer (Data flow) implementation start ********************************************************/

/**
 * Primary muxer task
 *
 * @param [in] info initial value
 *
 */

static UINT32 BrcFrameCount = 0;
static UINT32 BrcByteCount = 0;

extern UINT32 _find_jpeg_segment(UINT8* img, UINT32 size, UINT16 marker);
void AmpUT_DualVinEnc_PriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc = NULL;
    int Er = 0;
    UINT8 *BitsLimit = NULL;

    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_DualVinEnc_PriMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&VideoEncPriSem, 10000);  // Check if there is any pending frame to be muxed
        if (Er != OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }

        if (encPriBytesStorageTotal > ClipMaxStorageSize) {
            Er = AmpFifo_PeekEntry(VirtualPriFifoHdlr, &Desc, 0);
            if (outputPriFile && (Desc->Type<=AMP_FIFO_TYPE_I_FRAME)) {
                UT_DualVinEncodefsync((void *)outputPriFile);
                UT_DualVinEncodefclose((void *)outputPriFile);
                FnoPriSplit++;
                UT_DualVinEncodefsync((void *)outputPriIdxFile);
                UT_DualVinEncodefclose((void *)outputPriIdxFile);
                outputPriFile = NULL;
                outputPriIdxFile = NULL;
                FileOffset = 0;
            }
        }

        if ((!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) || (encPriBytesStorageTotal > ClipMaxStorageSize)) {
        //if ((!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI))) {
            if (outputPriFile == NULL) { // Open files when receiving the 1st frame
                char Fn[80];
                char mdASCII[3] = {'w','+','\0'};
                FORMAT_USER_DATA_s Udta;

                encPriBytesStorageTotal = 0;

                if (FnoPriSplit) {
                    sprintf(Fn,"%s:\\OU%02d%04d.%s", DefaultSlot, FnoPriSplit,fnoPri,EncPriSpecH264?"h264":"mjpg");
                } else {
                    sprintf(Fn,"%s:\\OUT_%04d.%s", DefaultSlot, fnoPri,EncPriSpecH264?"h264":"mjpg");
                }
                outputPriFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (outputPriFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    outputPriFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                if (FnoPriSplit) {
                    sprintf(Fn,"%s:\\OU%02d%04d.nhnt", DefaultSlot, FnoPriSplit,fnoPri);
                } else {
                    sprintf(Fn,"%s:\\OUT_%04d.nhnt", DefaultSlot, fnoPri);
                }
                outputPriIdxFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (outputPriIdxFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    outputPriIdxFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                if (FnoPriSplit==0) {
                    sprintf(Fn,"%s:\\OUT_%04d.udta", DefaultSlot, fnoPri);
                    UdtaPriFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                    while (UdtaPriFile==0) {
                        AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                        AmbaKAL_TaskSleep(10);
                        UdtaPriFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                    }

                    AmbaPrint("%s opened", Fn);

                    Udta.nIdrInterval = GOP_IDR/GOP_N;
                    Udta.nTimeScale = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;
                    Udta.nTickPerPicture = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TickPerPicture: VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                    Udta.nN = GOP_N;
                    Udta.nM = VideoEncMgt[EncModeIdx].GopM;
                    Udta.nInterlaced = VideoEncMgt[EncModeIdx].Interlace;
                    if (EncRotation == AMP_ROTATE_90 || EncRotation == AMP_ROTATE_90_VERT_FLIP || \
                        EncRotation == AMP_ROTATE_270 || EncRotation == AMP_ROTATE_270_VERT_FLIP) {
                        Udta.nVideoWidth = VideoEncMgt[EncModeIdx].MainHeight;
                        Udta.nVideoHeight = VideoEncMgt[EncModeIdx].MainWidth;
                    } else {
                        Udta.nVideoWidth = VideoEncMgt[EncModeIdx].MainWidth;
                        Udta.nVideoHeight = VideoEncMgt[EncModeIdx].MainHeight;
                    }

                    UT_DualVinEncodefwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)UdtaPriFile);
                    UT_DualVinEncodefclose((void *)UdtaPriFile);
                }
                NhntHeader.Signature[0]='n';
                NhntHeader.Signature[1]='h';
                NhntHeader.Signature[2]='n';
                NhntHeader.Signature[3]='t';
                NhntHeader.TimeStampResolution = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;
                UT_DualVinEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputPriIdxFile);

                if (EncPriSpecH264) {
                    BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
                } else {
                    BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
                }
            }
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

        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) {
                if (outputPriFile) {
                    UT_DualVinEncodefsync((void *)outputPriFile);
                    UT_DualVinEncodefclose((void *)outputPriFile);
                    fnoPri++;
                    UT_DualVinEncodefsync((void *)outputPriIdxFile);
                    UT_DualVinEncodefclose((void *)outputPriIdxFile);
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

                    UT_DualVinEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)outputPriIdxFile);

                    if (EncPriSpecH264==0 && VideoEncMgt[EncModeIdx].MainHeight==1080) {
                        UINT8 *Mjpeg_bs=0,*OriMjpeg_bs=0;
                        UINT32 sof_addr=0;
                        UINT8 *sof_ptr=0;

                        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&Mjpeg_bs, (void **)&OriMjpeg_bs, ALIGN_32(Desc->Size), 32);
                        if (Er != OK) {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                               // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                               UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputPriFile);
                            } else {
                                // AmbaCache_Invalidate(Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputPriFile);
                                // AmbaCache_Invalidate(H264BitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                                UT_DualVinEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
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
                            sof_addr = _find_jpeg_segment(Mjpeg_bs,Desc->Size,0xFFC0);
                            if (sof_addr == 0xFFFFFFFF) {
                                if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                                   // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                                   UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputPriFile);
                                } else {
                                    // AmbaCache_Invalidate(Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                    UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputPriFile);
                                    // AmbaCache_Invalidate(H264BitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                                    UT_DualVinEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
                                }
                            } else {
                                sof_ptr = (UINT8*)(sof_addr + 5);
                                (*sof_ptr)    =  (UINT8)(VideoEncMgt[EncModeIdx].MainHeight >> 8);
                                *(sof_ptr+1)  = (UINT8)(VideoEncMgt[EncModeIdx].MainHeight &  0xFF);

                                UT_DualVinEncodefwrite((const void *)Mjpeg_bs, 1, Desc->Size, (void *)outputPriFile);
                            }

                            if (AmbaKAL_BytePoolFree((void *)OriMjpeg_bs) != OK) {
                                AmbaPrint("cached memory release fail for MJPEG header change flow");
                            }
                        }

                    } else {
                 //       AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                        if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                           // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                           UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputPriFile);
                        } else {
                            UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputPriFile);
                            if (EncPriSpecH264) {
                                UT_DualVinEncodefwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
                            } else {
                                UT_DualVinEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputPriFile);
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
void AmpUT_DualVinEnc_SecMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc = NULL;
    int Er = 0;
    UINT8 *BitsLimit = NULL;

    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_DualVinEnc_SecMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&VideoEncSecSem, 10000);
        if (Er!= OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }
        if (encSecBytesStorageTotal > ClipMaxStorageSize) {
            Er = AmpFifo_PeekEntry(VirtualSecFifoHdlr, &Desc, 0);
            if (outputSecFile && (Desc->Type<=AMP_FIFO_TYPE_I_FRAME)) {
                UT_DualVinEncodefsync((void *)outputSecFile);
                UT_DualVinEncodefclose((void *)outputSecFile);
                FnoSecSplit++;
                UT_DualVinEncodefsync((void *)outputSecIdxFile);
                UT_DualVinEncodefclose((void *)outputSecIdxFile);
                outputSecFile = NULL;
                outputSecIdxFile = NULL;
                FileOffset = 0;
            }
        }
        if ((!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) || (encSecBytesStorageTotal > ClipMaxStorageSize)) {
            if (outputSecFile == NULL) {
                char Fn[80];
                char mdASCII[3] = {'w','+','\0'};
                FORMAT_USER_DATA_s Udta;

                encSecBytesStorageTotal = 0;

                if (FnoSecSplit) {
                    sprintf(Fn,"%s:\\OU%02d%04d_s.%s", DefaultSlot, FnoSecSplit, fnoSec,EncSecSpecH264?"h264":"mjpg");
                } else {
                    sprintf(Fn,"%s:\\OUT_%04d_s.%s", DefaultSlot, fnoSec,EncSecSpecH264?"h264":"mjpg");
                }
                outputSecFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (outputSecFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    outputSecFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                if (FnoSecSplit) {
                    sprintf(Fn,"%s:\\OU%02d%04d_s.nhnt", DefaultSlot, FnoSecSplit, fnoSec);
                } else {
                    sprintf(Fn,"%s:\\OUT_%04d_s.nhnt", DefaultSlot, fnoSec);
                }
                outputSecIdxFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (outputSecIdxFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    outputSecIdxFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                if (FnoSecSplit==0) {
                    sprintf(Fn,"%s:\\OUT_%04d_s.udta", DefaultSlot, fnoSec);

                    UdtaSecFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                    while (UdtaSecFile==0) {
                        AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                        AmbaKAL_TaskSleep(10);
                        UdtaSecFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,0);
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
                    Udta.nInterlaced = VideoEncMgt[EncModeIdx].Interlace;
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

                    UT_DualVinEncodefwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)UdtaSecFile);
                    UT_DualVinEncodefclose((void *)UdtaSecFile);
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

                UT_DualVinEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputSecIdxFile);

                if (EncSecSpecH264) {
                    BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
                } else {
                    BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
                }
            }

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
            } else if (Desc->Type == AMP_FIFO_TYPE_EOS) {
                Ty[0] = 'E'; Ty[1] = 'O'; Ty[2] = 'S'; Ty[3] = '\0';
            } else {
                Ty[0] = 'J'; Ty[1] = 'P'; Ty[2] = 'G'; Ty[3] = '\0';
            }

            if (LogMuxer) {
                AmbaPrint("Sec[%d] %s pts:%8lld 0x%08x %d", encSecTotalFrames, Ty, Desc->Pts, Desc->StartAddr, Desc->Size);
            }
        } else {
            while (Er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(VirtualSecFifoHdlr, &Desc, 0);
            }
        }
        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_SEC)) {
                if (outputSecFile) {
                    UT_DualVinEncodefsync((void *)outputSecFile);
                    UT_DualVinEncodefclose((void *)outputSecFile);
                    UT_DualVinEncodefsync((void *)outputSecIdxFile);
                    UT_DualVinEncodefclose((void *)outputSecIdxFile);
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

                    UT_DualVinEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)outputSecIdxFile);
                //      AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                    if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                        UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputSecFile);
                    } else {
                        UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputSecFile);
                        if (EncSecSpecH264) {
                            UT_DualVinEncodefwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputSecFile);
                        } else {
                            UT_DualVinEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputSecFile);
                        }
                    }
                }
            } else {
                AmbaKAL_TaskSleep(1);
            }

            encSecBytesTotal += Desc->Size;
            encSecBytesStorageTotal += Desc->Size;
            encSecTotalFrames ++;
        }
        AmpFifo_RemoveEntry(VirtualSecFifoHdlr, 1);
    }
}


void AmpUT_DualVinEnc_PIP_PriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc = NULL;
    int Er = 0;
    UINT8 *BitsLimit = NULL;

    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_DualVinEnc_PIP_PriMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&VideoPipEncPriSem, 10000);  // Check if there is any pending frame to be muxed
        if (Er != OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }

        if (PIPencPriBytesStorageTotal > ClipMaxStorageSize) {
            Er = AmpFifo_PeekEntry(VirtualPIPPriFifoHdlr, &Desc, 0);
            if (PIP_outputPriFile && (Desc->Type<=AMP_FIFO_TYPE_I_FRAME)) {
                UT_DualVinEncodefsync((void *)PIP_outputPriFile);
                UT_DualVinEncodefclose((void *)PIP_outputPriFile);
                PIPFnoPriSplit++;
                UT_DualVinEncodefsync((void *)PIP_outputPriIdxFile);
                UT_DualVinEncodefclose((void *)PIP_outputPriIdxFile);
                PIP_outputPriFile = NULL;
                PIP_outputPriIdxFile = NULL;
                FileOffset = 0;
            }
        }

        if ((!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) || (PIPencPriBytesStorageTotal > ClipMaxStorageSize)) {
        //if ((!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI))) {
            if (PIP_outputPriFile == NULL) { // Open files when receiving the 1st frame
                char Fn[80];
                char mdASCII[3] = {'w','+','\0'};
                FORMAT_USER_DATA_s Udta;

                PIPencPriBytesStorageTotal = 0;

                if (PIPFnoPriSplit) {
                    sprintf(Fn,"%s:\\OU%02d%04d_PIP.%s", DefaultSlot, PIPFnoPriSplit,PIPfnoPri,EncPriSpecH264?"h264":"mjpg");
                } else {
                    sprintf(Fn,"%s:\\OUT_%04d_PIP.%s", DefaultSlot, PIPfnoPri,EncPriSpecH264?"h264":"mjpg");
                }
                PIP_outputPriFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (PIP_outputPriFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    PIP_outputPriFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                if (PIPFnoPriSplit) {
                    sprintf(Fn,"%s:\\OU%02d%04d_PIP.nhnt", DefaultSlot, PIPFnoPriSplit,PIPfnoPri);
                } else {
                    sprintf(Fn,"%s:\\OUT_%04d_PIP.nhnt", DefaultSlot, PIPfnoPri);
                }
                PIP_outputPriIdxFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (PIP_outputPriIdxFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    PIP_outputPriIdxFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                if (PIPFnoPriSplit==0) {
                    sprintf(Fn,"%s:\\OUT_%04d_PIP.udta", DefaultSlot, PIPfnoPri);
                    PIP_UdtaPriFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                    while (PIP_UdtaPriFile==0) {
                        AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                        AmbaKAL_TaskSleep(10);
                        PIP_UdtaPriFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                    }

                    AmbaPrint("%s opened", Fn);

                    Udta.nIdrInterval = GOP_IDR/GOP_N;
                    Udta.nTimeScale = (EncodeSystem==0)? VideoPipEncMgt[EncPipModeIdx].TimeScale: VideoPipEncMgt[EncPipModeIdx].TimeScalePAL;
                    Udta.nTickPerPicture = (EncodeSystem==0)? VideoPipEncMgt[EncPipModeIdx].TickPerPicture: VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
                    Udta.nN = GOP_N;
                    Udta.nM = VideoPipEncMgt[EncPipModeIdx].GopM;
                    Udta.nInterlaced = VideoPipEncMgt[EncPipModeIdx].Interlace;
                    if (EncRotation == AMP_ROTATE_90 || EncRotation == AMP_ROTATE_90_VERT_FLIP || \
                        EncRotation == AMP_ROTATE_270 || EncRotation == AMP_ROTATE_270_VERT_FLIP) {
                        Udta.nVideoWidth = VideoPipEncMgt[EncPipModeIdx].MainHeight;
                        Udta.nVideoHeight = VideoPipEncMgt[EncPipModeIdx].MainWidth;
                    } else {
                        Udta.nVideoWidth = VideoPipEncMgt[EncPipModeIdx].MainWidth;
                        Udta.nVideoHeight = VideoPipEncMgt[EncPipModeIdx].MainHeight;
                    }

                    UT_DualVinEncodefwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)PIP_UdtaPriFile);
                    UT_DualVinEncodefclose((void *)PIP_UdtaPriFile);
                }
                NhntHeader.Signature[0]='n';
                NhntHeader.Signature[1]='h';
                NhntHeader.Signature[2]='n';
                NhntHeader.Signature[3]='t';
                NhntHeader.TimeStampResolution = (EncodeSystem==0)? VideoPipEncMgt[EncPipModeIdx].TimeScale: VideoPipEncMgt[EncPipModeIdx].TimeScalePAL;
                UT_DualVinEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)PIP_outputPriIdxFile);

                if (EncPriSpecH264) {
                    BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
                } else {
                    BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
                }
            }
        }

        Er = AmpFifo_PeekEntry(VirtualPIPPriFifoHdlr, &Desc, 0);  // Get a pending entry
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
                AmbaPrint("Pri[%d] %s pts:%8lld 0x%08x 0x%X", PIPencPriTotalFrames, Ty, Desc->Pts, Desc->StartAddr, Desc->Size);
        } else {
            while (Er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(VirtualPIPPriFifoHdlr, &Desc, 0);
            }
        }

        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) {
                if (PIP_outputPriFile) {
                    UT_DualVinEncodefsync((void *)PIP_outputPriFile);
                    UT_DualVinEncodefclose((void *)PIP_outputPriFile);
                    PIPfnoPri++;
                    UT_DualVinEncodefsync((void *)PIP_outputPriIdxFile);
                    UT_DualVinEncodefclose((void *)PIP_outputPriIdxFile);
                    PIP_outputPriFile = NULL;
                    PIP_outputPriIdxFile = NULL;
                    FileOffset = 0;
                    PIPFnoPriSplit = 0;
                }
            }

            Avg = (UINT32)(PIPencPriBytesTotal*8.0*PIPEncFrameRate/PIPencPriTotalFrames/1E3);

            AmbaPrint("PIP Primary Muxer met EOS, total %d frames/fields", PIPencPriTotalFrames);
            AmbaPrint("PIP Primary Bitrate Average: %d kbps\n", Avg);

            PIPencPriBytesTotal = 0;
            PIPencPriTotalFrames = 0;
        } else if (Desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
            //Do Nothing
        } else {
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) {
                if (PIP_outputPriFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;

                    UT_DualVinEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)PIP_outputPriIdxFile);

                    if (EncPriSpecH264==0 && VideoPipEncMgt[EncPipModeIdx].MainHeight==1080) {
                        UINT8 *Mjpeg_bs=0,*OriMjpeg_bs=0;
                        UINT32 sof_addr=0;
                        UINT8 *sof_ptr=0;

                        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&Mjpeg_bs, (void **)&OriMjpeg_bs, ALIGN_32(Desc->Size), 32);
                        if (Er != OK) {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                               // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                               UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)PIP_outputPriFile);
                            } else {
                                // AmbaCache_Invalidate(Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)PIP_outputPriFile);
                                // AmbaCache_Invalidate(H264BitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                                UT_DualVinEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)PIP_outputPriFile);
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
                            sof_addr = _find_jpeg_segment(Mjpeg_bs,Desc->Size,0xFFC0);
                            if (sof_addr == 0xFFFFFFFF) {
                                if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                                   // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                                   UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)PIP_outputPriFile);
                                } else {
                                    // AmbaCache_Invalidate(Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                    UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)PIP_outputPriFile);
                                    // AmbaCache_Invalidate(H264BitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                                    UT_DualVinEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)PIP_outputPriFile);
                                }
                            } else {
                                sof_ptr = (UINT8*)(sof_addr + 5);
                                (*sof_ptr)    =  (UINT8)(VideoPipEncMgt[EncPipModeIdx].MainHeight >> 8);
                                *(sof_ptr+1)  = (UINT8)(VideoPipEncMgt[EncPipModeIdx].MainHeight &  0xFF);

                                UT_DualVinEncodefwrite((const void *)Mjpeg_bs, 1, Desc->Size, (void *)PIP_outputPriFile);
                            }

                            if (AmbaKAL_BytePoolFree((void *)OriMjpeg_bs) != OK) {
                                AmbaPrint("cached memory release fail for MJPEG header change flow");
                            }
                        }

                    } else {
                 //       AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                        if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                           // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                           UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)PIP_outputPriFile);
                        } else {
                            UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)PIP_outputPriFile);
                            if (EncPriSpecH264) {
                                UT_DualVinEncodefwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)PIP_outputPriFile);
                            } else {
                                UT_DualVinEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)PIP_outputPriFile);
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


            PIPencPriBytesTotal += Desc->Size;
            PIPencPriBytesStorageTotal += Desc->Size;
            PIPencPriTotalFrames++;
            //BrcFrameCount++;
            //BrcByteCount+=Desc->Size;
        }
        AmpFifo_RemoveEntry(VirtualPIPPriFifoHdlr, 1);
    }
}


/**
 * Secondary muxer task
 *
 * @param [in] info initial value
 *
 */
void AmpUT_DualVinEnc_PIP_SecMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc = NULL;
    int Er = 0;
    UINT8 *BitsLimit = NULL;

    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_DualVinEnc_SecMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&VideoPipEncSecSem, 10000);
        if (Er!= OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }
        if (PIPencSecBytesStorageTotal > ClipMaxStorageSize) {
            Er = AmpFifo_PeekEntry(VirtualPIPSecFifoHdlr, &Desc, 0);
            if (PIP_outputSecFile && (Desc->Type<=AMP_FIFO_TYPE_I_FRAME)) {
                UT_DualVinEncodefsync((void *)PIP_outputSecFile);
                UT_DualVinEncodefclose((void *)PIP_outputSecFile);
                PIPFnoSecSplit++;
                UT_DualVinEncodefsync((void *)PIP_outputSecIdxFile);
                UT_DualVinEncodefclose((void *)PIP_outputSecIdxFile);
                PIP_outputSecFile = NULL;
                PIP_outputSecIdxFile = NULL;
                FileOffset = 0;
            }
        }
        if ((!(EncodeDumpSkip & VIDEO_ENC_SKIP_PRI)) || (PIPencSecBytesStorageTotal > ClipMaxStorageSize)) {
            if (PIP_outputSecFile == NULL) {
                char Fn[80];
                char mdASCII[3] = {'w','+','\0'};
                FORMAT_USER_DATA_s Udta;

                PIPencSecBytesStorageTotal = 0;

                if (FnoSecSplit) {
                    sprintf(Fn,"%s:\\OU%02d%04d_PIP_s.%s", DefaultSlot, PIPFnoSecSplit, PIPfnoSec,EncSecSpecH264?"h264":"mjpg");
                } else {
                    sprintf(Fn,"%s:\\OUT_%04d_PIP_s.%s", DefaultSlot, PIPfnoSec,EncSecSpecH264?"h264":"mjpg");
                }
                PIP_outputSecFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (PIP_outputSecFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    PIP_outputSecFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                if (PIPFnoSecSplit) {
                    sprintf(Fn,"%s:\\OU%02d%04d_PIP_s.nhnt", DefaultSlot, PIPFnoSecSplit, PIPfnoSec);
                } else {
                    sprintf(Fn,"%s:\\OUT_%04d_PIP_s.nhnt", DefaultSlot, PIPfnoSec);
                }
                PIP_outputSecIdxFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (PIP_outputSecIdxFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    PIP_outputSecIdxFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                if (PIPFnoSecSplit==0) {
                    sprintf(Fn,"%s:\\OUT_%04d_PIP_s.udta", DefaultSlot, PIPfnoSec);

                    PIP_UdtaSecFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                    while (PIP_UdtaSecFile==0) {
                        AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                        AmbaKAL_TaskSleep(10);
                        PIP_UdtaSecFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,0);
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
                    Udta.nInterlaced = VideoPipEncMgt[EncPipModeIdx].Interlace;
                    if (EncDualHDStream) {
                        if (SecStreamCustomWidth) {
                            Udta.nVideoWidth = SecStreamCustomWidth;
                            Udta.nVideoHeight = SecStreamCustomHeight;
                        } else {
                            UINT16 OutWidth = 0, OutHeight = 0;
                            MWUT_InputSetDualHDWindow(VideoPipEncMgt, EncPipModeIdx, &OutWidth, &OutHeight);
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

                    UT_DualVinEncodefwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)PIP_UdtaSecFile);
                    UT_DualVinEncodefclose((void *)PIP_UdtaSecFile);
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

                UT_DualVinEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)PIP_outputSecIdxFile);

                if (EncSecSpecH264) {
                    BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
                } else {
                    BitsLimit = MjpgBitsBuf + BITSFIFO_SIZE - 1;
                }
            }

        }

        Er = AmpFifo_PeekEntry(VirtualPIPSecFifoHdlr, &Desc, 0);
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
            } else if (Desc->Type == AMP_FIFO_TYPE_EOS) {
                Ty[0] = 'E'; Ty[1] = 'O'; Ty[2] = 'S'; Ty[3] = '\0';
            } else {
                Ty[0] = 'J'; Ty[1] = 'P'; Ty[2] = 'G'; Ty[3] = '\0';
            }

            if (LogMuxer) {
                AmbaPrint("Sec[%d] %s pts:%8lld 0x%08x %d", PIPencSecTotalFrames, Ty, Desc->Pts, Desc->StartAddr, Desc->Size);
            }
        } else {
            while (Er != 0) {
                AmbaPrint("Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(VirtualPIPSecFifoHdlr, &Desc, 0);
            }
        }
        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_SEC)) {
                if (PIP_outputSecFile) {
                    UT_DualVinEncodefsync((void *)PIP_outputSecFile);
                    UT_DualVinEncodefclose((void *)PIP_outputSecFile);
                    UT_DualVinEncodefsync((void *)PIP_outputSecIdxFile);
                    UT_DualVinEncodefclose((void *)PIP_outputSecIdxFile);
                    PIP_outputSecFile = NULL;
                    PIP_outputSecIdxFile = NULL;
                    FileOffset = 0;
                }
            }

            if (EncodeSystem) {
                Avg = (UINT32)(PIPencSecBytesTotal*8.0*(SEC_STREAM_TIMESCALE_PAL/SEC_STREAM_TICK_PAL)/PIPencSecTotalFrames/1E3);
            } else {
                Avg = (UINT32)(PIPencSecBytesTotal*8.0*(SEC_STREAM_TIMESCALE/SEC_STREAM_TICK)/PIPencSecTotalFrames/1E3);
            }

            AmbaPrint("PIP Secondary Muxer met EOS, total %d frames/fields", PIPencSecTotalFrames);
            AmbaPrint("PIP Secondary Bitrate Average: %d kbps\n", Avg);

            PIPencSecBytesTotal = 0;
            PIPencSecTotalFrames = 0;
            PIPFnoSecSplit = 0;
            PIPfnoSec++;
        } else if (Desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
            //Do Nothing
        } else {
            if (!(EncodeDumpSkip & VIDEO_ENC_SKIP_SEC)) {
                if (PIP_outputSecFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;

                    UT_DualVinEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)PIP_outputSecIdxFile);
                //      AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                    if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                        UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)PIP_outputSecFile);
                    } else {
                        UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)PIP_outputSecFile);
                        if (EncSecSpecH264) {
                            UT_DualVinEncodefwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)PIP_outputSecFile);
                        } else {
                            UT_DualVinEncodefwrite((const void *)MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)PIP_outputSecFile);
                        }
                    }
                }
            } else {
                AmbaKAL_TaskSleep(1);
            }

            PIPencSecBytesTotal += Desc->Size;
            PIPencSecBytesStorageTotal += Desc->Size;
            PIPencSecTotalFrames ++;
        }
        AmpFifo_RemoveEntry(VirtualPIPSecFifoHdlr, 1);
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
static int AmpUT_DualVinEnc_FifoCB(void *hdlr, UINT32 event, void* info)
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
    } else if (hdlr == VirtualPIPPriFifoHdlr) {
        pSem = &VideoPipEncPriSem;
    } else if (hdlr == VirtualPIPSecFifoHdlr) {
        pSem = &VideoPipEncSecSem;
    } else if (hdlr == VirtualPIPJpegFifoHdlr) {
        pSem = &VideoPipEncJpegSem;
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

#define VENC_IS_ROTATE(x)  (x == AMP_ROTATE_90 || x == AMP_ROTATE_90_VERT_FLIP || \
                          x == AMP_ROTATE_270 || x == AMP_ROTATE_270_VERT_FLIP)

/**
 * Vout calculate
 *
 *
 * @return 0
 */
static void AmpUT_DualVinEncVoutCalculate(AMP_AREA_s *area, UINT16 areaAspectRatio, AMP_DISP_CHANNEL_IDX_e Channel)
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

void AmpUT_DualVinEncCalibrationHook(AMBA_DSP_IMG_MODE_CFG_s ImgMode)
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
int AmpUT_DualVinIsoConfigSet(AMBA_DSP_IMG_ALGO_MODE_e isoMode, AMBA_DSP_IMG_MODE_CFG_s *imgMode)
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

int AmpUT_DualVinEnc_AqpPriStream(AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *StrmHdlr)
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

int AmpUT_DualVinEnc_AqpSecStream(AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *StrmHdlr)
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

#define COMPLEX_DAY_NORMAL       (0)
#define COMPLEX_NIGHT_NORMAL     (1)
#define COMPLEX_DAY_HDR          (2)
#define COMPLEX_NIGHT_HDR        (3)
#define COMPLEX_DAY_OVERSAMPLE   (4)
#define COMPLEX_NIGHT_OVERSAMPLE (5)
static int AmpUT_DualVinEnc_GetDayLumaThresholdCB(int mode, UINT32 *threshold)
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

static int AmpUT_DualVinEnc_GetSceneComplexityRangeCB(int mode, UINT32 *complexMin, UINT32 *complexMid, UINT32 *complexMax)
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

/* Initial JPEG DQT */
void AmpUT_DualVinEnc_InitMJpegDqt(UINT8 *qTable, int quality)
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
static int AmpUT_DualVinEncImgSchdlrCallback(void *hdlr,UINT32 event, void *info)
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
static int AmpUT_DualVinEncVinSwitchCallback(void *hdlr, UINT32 event, void *info)
{
    AMBA_DSP_CHANNEL_ID_u *Channel;

    if (hdlr==VideoEncVinA) {
        Channel = &VinChannel;
    } else {
        Channel = &VinPipChannel;
    }
    switch (event) {
        case AMP_VIN_EVENT_INVALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_INVALID info: %X %X", info, ImgSchdlr);
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_INVALID info: %X %X", info, PIPImgSchdlr);
            if (ImgSchdlr) AmbaImgSchdlr_Enable(ImgSchdlr, 0);
            if (PIPImgSchdlr) AmbaImgSchdlr_Enable(PIPImgSchdlr, 0);

            Amba_Img_VIn_Invalid(hdlr, (UINT32 *)NULL);
            break;
        case AMP_VIN_EVENT_VALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_VALID info: %X %X", info, ImgSchdlr);
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_VALID info: %X %X", info, PIPImgSchdlr);
            if (VideoEnc3AEnable) {
                Amba_Img_VIn_Valid(hdlr, (UINT32 *)NULL);
            }

            if (ImgSchdlr) AmbaImgSchdlr_Enable(ImgSchdlr, 1);
            if (PIPImgSchdlr) AmbaImgSchdlr_Enable(PIPImgSchdlr, 1);
            break;
        case AMP_VIN_EVENT_CHANGED_PRIOR:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_PRIOR info: %X", info);
            if (VideoEnc3AEnable) {
                Amba_Img_VIn_Changed_Prior(hdlr, (UINT32 *)NULL);
            }

            if (Status == STATUS_LIVEVIEW) { // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window = {0}, DefWindow = {0};
                AMP_AREA_s WindowArea = {0};
                UINT16 FinalAspectRatio = 0;

                if (1) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;

                    WindowArea.Width = 960;
                    if (DualVideoVinSelect == VIN_PIP_ONLY) {
                        WindowArea.Height = (VideoPipEncMgt[EncPipModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                    } else {
                        WindowArea.Height = (VideoEncMgt[EncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                    }
                    WindowArea.X = 0;
                    WindowArea.Y = (480-WindowArea.Height)/2;
                    if (VENC_IS_ROTATE(EncRotation) && LiveViewProcMode == 0) {
                        if (DualVideoVinSelect == VIN_PIP_ONLY) {
                            FinalAspectRatio = GET_VAR_X(VideoPipEncMgt[EncPipModeIdx].AspectRatio) |
                                (GET_VAR_Y(VideoPipEncMgt[EncPipModeIdx].AspectRatio)<<8);
                        } else {
                            FinalAspectRatio = GET_VAR_X(VideoEncMgt[EncModeIdx].AspectRatio) |
                                (GET_VAR_Y(VideoEncMgt[EncModeIdx].AspectRatio)<<8);
                        }
                    } else {
                        if (DualVideoVinSelect == VIN_PIP_ONLY) {
                            FinalAspectRatio = VideoPipEncMgt[EncPipModeIdx].AspectRatio;
                        } else {
                            FinalAspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
                        }
                    }
                    AmpUT_DualVinEncVoutCalculate(&WindowArea, FinalAspectRatio, AMP_DISP_CHANNEL_DCHAN);

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
                AMP_AREA_s WindowArea = {0};
                UINT16 FinalAspectRatio = 0;

                if (1) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;
                    if (DualVideoVinSelect == VIN_PIP_ONLY) {
                        WindowArea.Width = (VideoPipEncMgt[EncPipModeIdx].AspectRatio == VAR_16x9)? 1920: 1440;
                    } else {
                        WindowArea.Width = (VideoEncMgt[EncModeIdx].AspectRatio == VAR_16x9)? 1920: 1440;
                    }
                    WindowArea.Height = 1080;
                    WindowArea.X = (1920 - WindowArea.Width)/2;
                    WindowArea.Y = 0;
                    if (VENC_IS_ROTATE(EncRotation) && LiveViewProcMode == 0) {
                        if (DualVideoVinSelect == VIN_PIP_ONLY) {
                            FinalAspectRatio = GET_VAR_X(VideoPipEncMgt[EncPipModeIdx].AspectRatio) |
                                (GET_VAR_Y(VideoPipEncMgt[EncPipModeIdx].AspectRatio)<<8);
                        } else {
                            FinalAspectRatio = GET_VAR_X(VideoEncMgt[EncModeIdx].AspectRatio) |
                                (GET_VAR_Y(VideoEncMgt[EncModeIdx].AspectRatio)<<8);
                        }
                    } else {
                        if (DualVideoVinSelect == VIN_PIP_ONLY) {
                            FinalAspectRatio = VideoPipEncMgt[EncPipModeIdx].AspectRatio;
                        } else {
                            FinalAspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
                        }
                    }
                    AmpUT_DualVinEncVoutCalculate(&WindowArea, FinalAspectRatio, AMP_DISP_CHANNEL_FCHAN);

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
            break;
        case AMP_VIN_EVENT_CHANGED_POST:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_POST info: %X", info);

            if (VideoEnc3AEnable) {
                extern void _Set_AdjTableNo(UINT32 chNo, int AdjTableNo);
                UINT8 IsPhotoLiveView = 0;
                UINT16 PipeMode = IP_EXPERSS_MODE;
                UINT16 AlgoMode = IP_MODE_LISO_VIDEO;
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo;
                AMBA_SENSOR_MODE_ID_u SensorMode = {0};
                UINT32 ChNo = Channel->Bits.VinID;
                AMBA_SENSOR_STATUS_INFO_s SensorStatus = {0};

                if (ChNo == 0) {
                    SensorMode.Data = (EncodeSystem == 0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;
                } else {
                    SensorMode.Data = (EncodeSystem == 0)? VideoPipEncMgt[EncPipModeIdx].InputMode: VideoPipEncMgt[EncPipModeIdx].InputPALMode;
                }
                AmbaSensor_GetModeInfo((*Channel), SensorMode, &SensorModeInfo);
                AmbaSensor_GetStatus((*Channel), &SensorStatus);
                AmbaPrintColor(YELLOW,"(*Channel)=%x,SensorStatus.ModeInfo.HdrInfo.HdrType=%d,SensorModeInfo.HdrInfo.HdrType=%d",(*Channel),SensorStatus.ModeInfo.HdrInfo.HdrType,SensorModeInfo.HdrInfo.HdrType);
                #ifdef CONFIG_SOC_A12
                if(ChNo == 0) { // Load IQ params for Main
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
                } else { // Load IQ params for PIP
                    extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
                    UINT8 IsSensorHdrMode;
                    // Inform 3A LV sensor mode is Hdr or not
                    IsSensorHdrMode = (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE)? 1: 0;
                    AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_HDR_ENABLE, ChNo+LiveViewHdrMode[0], (UINT32)IsSensorHdrMode, 0);

                    if (VideoEncPipIsIqParamInit == 0 || IsSensorHdrMode != VideoEncIsHdrIqParam) {
                        if (IsSensorHdrMode == 0) {
                            App_Image_Init_Iq_Params(0, InputDeviceId); // FIXME: 3a need this
                            App_Image_Init_Iq_Params(ChNo+LiveViewHdrMode[0], InputPipDeviceId); /* PIP sensor id*/
                        } else if (IsSensorHdrMode == 1) {
                            App_Image_Init_Iq_Params(0, InputDeviceId); // FIXME: 3a need this
                            App_Image_Init_Iq_Params(ChNo+LiveViewHdrMode[0], InputPipDeviceId);
                            App_Image_Init_Iq_Params(ChNo+LiveViewHdrMode[0], InputPipDeviceId);
                        }
                        VideoEncPipIsIqParamInit = 1;
                        VideoEncPipIsHdrIqParam = IsSensorHdrMode;
                    }
                }
                #endif
                //inform 3A LiveView pipeline
                if (LiveViewProcMode && LiveViewAlgoMode) {
                    PipeMode = IP_HYBRID_MODE;

                    //Change Adj Table to TA one if necessary
                    if (VideoEncTAEnable[ChNo]) {
                        _Set_AdjTableNo(ChNo, 1);
                    } else {
                        _Set_AdjTableNo(ChNo, -1);
                    }
                } else {
                    PipeMode = IP_EXPERSS_MODE;
                    _Set_AdjTableNo(ChNo, -1);
                }
                AmbaImg_Proc_Cmd(MW_IP_SET_PIPE_MODE, ChNo, (UINT32)PipeMode, 0);

                //inform 3A LiveView Algo
                if (LiveViewAlgoMode == 0 || LiveViewAlgoMode == 1) {
                    AlgoMode = IP_MODE_LISO_VIDEO;
                } else {
                    AlgoMode = IP_MODE_HISO_VIDEO;
                }
                AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_ALGO_MODE, ChNo, (UINT32)AlgoMode, 0);

                //inform 3A LiveView type
                AmbaImg_Proc_Cmd(MW_IP_SET_PHOTO_PREVIEW, (UINT32)&IsPhotoLiveView, 0, 0);

                //inform 3A Liveview info
                if (DualVideoVinSelect == VIN_MAIN_ONLY) {
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
                    LiveViewInfo.FrameRateInt = UT_DualVinFrameRateIntConvert(LiveViewInfo.FrameRateInt);
                    AmbaSensor_GetStatus(VinChannel, &SsrStatus);
                    LiveViewInfo.BinningHNum = InputInfo->HSubsample.FactorNum;
                    LiveViewInfo.BinningHDen = InputInfo->HSubsample.FactorDen;
                    LiveViewInfo.BinningVNum = InputInfo->VSubsample.FactorNum;
                    LiveViewInfo.BinningVDen = InputInfo->VSubsample.FactorDen;
                    AmbaImg_Proc_Cmd(MW_IP_SET_LIVEVIEW_INFO, 0/*ChIndex*/, (UINT32)&LiveViewInfo, 0);
                } else if (DualVideoVinSelect == VIN_PIP_ONLY) {
                    LIVEVIEW_INFO_s LiveViewInfo = {0};
                    AMBA_SENSOR_STATUS_INFO_s SsrStatus = {0};
                    AMBA_SENSOR_INPUT_INFO_s *InputInfo = &SsrStatus.ModeInfo.InputInfo;

                    LiveViewInfo.OverSamplingEnable = 1; //HybridMode use OverSampling
                    LiveViewInfo.MainW = VideoPipEncMgt[EncPipModeIdx].MainWidth;
                    LiveViewInfo.MainH = VideoPipEncMgt[EncPipModeIdx].MainHeight;
                    if (EncodeSystem == 0) {
                        LiveViewInfo.FrameRateInt = VideoPipEncMgt[EncPipModeIdx].TimeScale/VideoPipEncMgt[EncPipModeIdx].TickPerPicture;
                    } else {
                        LiveViewInfo.FrameRateInt = VideoPipEncMgt[EncPipModeIdx].TimeScalePAL/VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
                    }
                    LiveViewInfo.FrameRateInt = UT_DualVinFrameRateIntConvert(LiveViewInfo.FrameRateInt);
                    AmbaSensor_GetStatus(VinPipChannel, &SsrStatus);
                    LiveViewInfo.BinningHNum = InputInfo->HSubsample.FactorNum;
                    LiveViewInfo.BinningHDen = InputInfo->HSubsample.FactorDen;
                    LiveViewInfo.BinningVNum = InputInfo->VSubsample.FactorNum;
                    LiveViewInfo.BinningVDen = InputInfo->VSubsample.FactorDen;

                    AmbaImg_Proc_Cmd(MW_IP_SET_LIVEVIEW_INFO, 1/*ChIndex*/, (UINT32)&LiveViewInfo, 0);
                } else if (DualVideoVinSelect == VIN_MAIN_PIP) {
                    LIVEVIEW_INFO_s LiveViewInfo = {0};
                    AMBA_SENSOR_STATUS_INFO_s SsrStatus = {0};
                    AMBA_SENSOR_INPUT_INFO_s *InputInfo = &SsrStatus.ModeInfo.InputInfo;

                    LiveViewInfo.OverSamplingEnable = 1; //HybridMode use OverSampling
                    LiveViewInfo.MainW = VideoEncMgt[EncModeIdx].MainWidth;
                    LiveViewInfo.MainH = VideoEncMgt[EncModeIdx].MainHeight;
                    if (EncodeSystem == 0) {
                        LiveViewInfo.FrameRateInt = VideoEncMgt[EncPipModeIdx].TimeScale/VideoEncMgt[EncPipModeIdx].TickPerPicture;
                    } else {
                        LiveViewInfo.FrameRateInt = VideoEncMgt[EncPipModeIdx].TimeScalePAL/VideoEncMgt[EncPipModeIdx].TickPerPicturePAL;
                    }
                    LiveViewInfo.FrameRateInt = UT_DualVinFrameRateIntConvert(LiveViewInfo.FrameRateInt);
                    AmbaSensor_GetStatus(VinChannel, &SsrStatus);
                    LiveViewInfo.BinningHNum = InputInfo->HSubsample.FactorNum;
                    LiveViewInfo.BinningHDen = InputInfo->HSubsample.FactorDen;
                    LiveViewInfo.BinningVNum = InputInfo->VSubsample.FactorNum;
                    LiveViewInfo.BinningVDen = InputInfo->VSubsample.FactorDen;
                    AmbaImg_Proc_Cmd(MW_IP_SET_LIVEVIEW_INFO, 0/*ChIndex*/, (UINT32)&LiveViewInfo, 0);

                    LiveViewInfo.MainW = VideoPipEncMgt[EncPipModeIdx].MainWidth;
                    LiveViewInfo.MainH = VideoPipEncMgt[EncPipModeIdx].MainHeight;
                    if (EncodeSystem == 0) {
                        LiveViewInfo.FrameRateInt = VideoPipEncMgt[EncPipModeIdx].TimeScale/VideoPipEncMgt[EncPipModeIdx].TickPerPicture;
                    } else {
                        LiveViewInfo.FrameRateInt = VideoPipEncMgt[EncPipModeIdx].TimeScalePAL/VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
                    }
                    LiveViewInfo.FrameRateInt = UT_DualVinFrameRateIntConvert(LiveViewInfo.FrameRateInt);
                    AmbaSensor_GetStatus(VinPipChannel, &SsrStatus);
                    LiveViewInfo.BinningHNum = InputInfo->HSubsample.FactorNum;
                    LiveViewInfo.BinningHDen = InputInfo->HSubsample.FactorDen;
                    LiveViewInfo.BinningVNum = InputInfo->VSubsample.FactorNum;
                    LiveViewInfo.BinningVDen = InputInfo->VSubsample.FactorDen;
                    AmbaImg_Proc_Cmd(MW_IP_SET_LIVEVIEW_INFO, 1/*ChIndex*/, (UINT32)&LiveViewInfo, 0);
                }

                //inform 3A Liveview info
                if (DualVideoVinSelect == VIN_MAIN_ONLY) {
                    UINT32 FrameRate = 0, FrameRatex1000 = 0;

                    if (EncodeSystem == 0) {
                        FrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;
                        FrameRatex1000 = VideoEncMgt[EncModeIdx].TimeScale*1000/VideoEncMgt[EncModeIdx].TickPerPicture;
                    } else {
                        FrameRate = VideoEncMgt[EncModeIdx].TimeScalePAL/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                        FrameRatex1000 = VideoEncMgt[EncModeIdx].TimeScalePAL*1000/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                    }
                    FrameRate = UT_DualVinFrameRateIntConvert(FrameRate);
                    AmbaImg_Proc_Cmd(MW_IP_SET_FRAME_RATE, 0/*ChIndex*/, FrameRate, FrameRatex1000);
                } else if (DualVideoVinSelect == VIN_PIP_ONLY) {
                    UINT32 FrameRate = 0, FrameRatex1000 = 0;

                    if (EncodeSystem == 0) {
                        FrameRate = VideoPipEncMgt[EncPipModeIdx].TimeScale/VideoPipEncMgt[EncPipModeIdx].TickPerPicture;
                        FrameRatex1000 = VideoPipEncMgt[EncPipModeIdx].TimeScale*1000/VideoPipEncMgt[EncPipModeIdx].TickPerPicture;
                    } else {
                        FrameRate = VideoPipEncMgt[EncPipModeIdx].TimeScalePAL/VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
                        FrameRatex1000 = VideoPipEncMgt[EncPipModeIdx].TimeScalePAL*1000/VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
                    }
                    FrameRate = UT_DualVinFrameRateIntConvert(FrameRate);
                    AmbaImg_Proc_Cmd(MW_IP_SET_FRAME_RATE, 1/*ChIndex*/, FrameRate, FrameRatex1000);

                } else if (DualVideoVinSelect == VIN_MAIN_PIP) {
                    UINT32 FrameRate = 0, FrameRatex1000 = 0;

                    if (EncodeSystem == 0) {
                        FrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;
                        FrameRatex1000 = VideoEncMgt[EncModeIdx].TimeScale*1000/VideoEncMgt[EncModeIdx].TickPerPicture;
                    } else {
                        FrameRate = VideoEncMgt[EncModeIdx].TimeScalePAL/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                        FrameRatex1000 = VideoEncMgt[EncModeIdx].TimeScalePAL*1000/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
                    }
                    FrameRate = UT_DualVinFrameRateIntConvert(FrameRate);
                    AmbaImg_Proc_Cmd(MW_IP_SET_FRAME_RATE, 0/*ChIndex*/, FrameRate, FrameRatex1000);

                    if (EncodeSystem == 0) {
                        FrameRate = VideoPipEncMgt[EncPipModeIdx].TimeScale/VideoPipEncMgt[EncPipModeIdx].TickPerPicture;
                        FrameRatex1000 = VideoPipEncMgt[EncPipModeIdx].TimeScale*1000/VideoPipEncMgt[EncPipModeIdx].TickPerPicture;
                    } else {
                        FrameRate = VideoPipEncMgt[EncPipModeIdx].TimeScalePAL/VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
                        FrameRatex1000 = VideoPipEncMgt[EncPipModeIdx].TimeScalePAL*1000/VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
                    }
                    FrameRate = UT_DualVinFrameRateIntConvert(FrameRate);
                    AmbaImg_Proc_Cmd(MW_IP_SET_FRAME_RATE, 1/*ChIndex*/, FrameRate, FrameRatex1000);
                }
                Amba_Img_VIn_Changed_Post(hdlr, &ChNo);
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
                        AmbaImgSchdlr_GetExposureInfo(Channel->Bits.VinID/*MainViewID*/, &CurExpInfo);
                        memcpy(&ExpInfo.Info, &CurExpInfo, sizeof(AMBA_IMG_SCHDLR_EXP_INFO_s));

                        ExpInfo.Type = AMBA_IMG_SCHDLR_SET_TYPE_DIRECT;
                        ExpInfo.Info.AdjUpdated = 1;
                        memcpy(&ExpInfo.Info.Mode, &ItunerInfo.TuningAlgoMode, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
                        CtxId = AmbaImgSchdlr_GetIsoCtxIndex(Channel->Bits.VinID/*MainViewID*/, ExpInfo.Info.Mode.AlgoMode);
                        if (CtxId >= 0) {
                            ExpInfo.Info.Mode.ContextId = CtxId;
                        }

                        AmbaImgSchdlr_SetExposure(Channel->Bits.VinID/*MainViewID*/, &ExpInfo);
                    } else {
                        AMBA_DSP_IMG_MODE_CFG_s ImgMode;

                        ImgMode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
                        ImgMode.AlgoMode = (LiveViewAlgoMode == 1)? AMBA_DSP_IMG_ALGO_MODE_LISO: AMBA_DSP_IMG_ALGO_MODE_HISO;
                        ImgMode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_FV;
                        ImgMode.ContextId = AmbaImgSchdlr_GetIsoCtxIndex(Channel->Bits.VinID/*TBD*/, ImgMode.AlgoMode);

                        //Set Adj
                        AmpUT_DualVinIsoConfigSet(ImgMode.AlgoMode, &ImgMode);
                    }
                }
            }

            if (CalibEnable) {
                AMBA_DSP_IMG_MODE_CFG_s ImgMode;

                memset(&ImgMode, 0x0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
                ImgMode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
                ImgMode.AlgoMode = (LiveViewAlgoMode == 2)? AMBA_DSP_IMG_ALGO_MODE_HISO: AMBA_DSP_IMG_ALGO_MODE_LISO;
                ImgMode.ContextId = AmbaImgSchdlr_GetIsoCtxIndex(Channel->Bits.VinID/*TBD*/, ImgMode.AlgoMode);
                AmpUT_DualVinEncCalibrationHook(ImgMode);
            }
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
 * DualVin task dealing with general events
 *
 * @param [in] Input input
 *
 * @return none
 */
static void DualVideoEncTaskHandler(UINT32 Input)
{
    DUAL_VIDENC_MSG_S Msg = {0};
    int er = OK;
    AmbaPrint("DualVideoEncTaskHandler started");

    while (1) {
        er = AmbaKAL_MsgQueueReceive(&DualVideoEncMqueue, (void *)&Msg, 3000);
        if (er != OK) {
            continue;
        }

        switch (Msg.Event) {
        case DUAL_VIDENC_MSG_ILLEGAL_SIGNAL:
            AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! DUAL_VIDENC_MSG_ILLEGAL_SIGNAL (%d)!!!!!!!!!!",Msg.Data[0]);
            AmpUT_DualVinEnc_LiveviewStop();
            Status = STATUS_INIT;
            if (Msg.Data[0] == 1) {
                DualVideoVinSelect = VIN_MAIN_ONLY;
            } else {
                DualVideoVinSelect = VIN_PIP_ONLY;
            }
            AmpUT_DualVinEnc_LiveviewStart(EncModeIdx, EncPipModeIdx);
            Status = STATUS_LIVEVIEW;
            break;
        default:
            AmbaPrint("DualVideoEncTaskHandler: Unknown %x", Msg.Event);
            break;
        }
    }

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
static int AmpUT_DualVinEncVinEventCallback(void *hdlr,UINT32 event, void *info)
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
static int AmpUT_DualVinEncCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_DONE:
            // Next raw capture is allowed from now on
            StillRawCaptureRunning = 0;
            break;
        case AMP_ENC_EVENT_BACK_TO_LIVEVIEW:
            break;
        case AMP_ENC_EVENT_BG_PROCESS_DONE:
            break;
        case AMP_ENC_EVENT_LIVEVIEW_RAW_READY:
             if (VideoEnc3AEnable) {
                extern int Amba_Img_VDspRaw_Handler(void *hdlr, UINT32 *pRgbData)  __attribute__((weak));
                AMP_ENC_RAW_INFO_s *ptr = info;
                AMBA_IMG_RAW_INFO_s RawBufInfo;
                AMBA_SENSOR_MODE_ID_u Mode = {0};
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo;

                if (DualVideoVinSelect == VIN_PIP_ONLY) {
                    Mode.Data = (EncodeSystem==0)? VideoPipEncMgt[EncPipModeIdx].InputMode: VideoPipEncMgt[EncPipModeIdx].InputPALMode;
                    AmbaSensor_GetModeInfo(VinPipChannel, Mode, &SensorModeInfo);
                } else {
                    Mode.Data = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;
                    AmbaSensor_GetModeInfo(VinChannel, Mode, &SensorModeInfo);
                }

                memset(&RawBufInfo, 0x0, sizeof(AMBA_IMG_RAW_INFO_s));
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
            //AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VCAP_2ND_YUV_READY:
            //AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_2ND_YUV_READY !!!!!!!!!!");
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
            break;
        case AMP_ENC_EVENT_VCAP_ME1_Y_READY:
            //AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_ME1_Y_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_HYBRID_YUV_READY:
            //AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_HYBRID_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_ILLEGAL_SIGNAL:
            {
                UINT32 *ChNo = (UINT32 *) info;
                DUAL_VIDENC_MSG_S Msg = {DUAL_VIDENC_MSG_ILLEGAL_SIGNAL, ChNo, 0, 0};
                AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_ILLEGAL_SIGNAL (%d)!!!!!!!!!!",ChNo);
                AmbaKAL_MsgQueueSend(&DualVideoEncMqueue, &Msg, AMBA_KAL_WAIT_FOREVER);
            }
            break;
        case AMP_ENC_EVENT_VDSP_ASSERT:
            //AmbaPrintColor(RED, "AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_VDSP_ASSERT !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            AmpUT_DualVinEnc_EncStop();
            EncDateTimeStampPri = EncDateTimeStampSec = 0;
            Status = STATUS_LIVEVIEW;
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DESC_OVERRUN:
            AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_DESC_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY:
            if (0) {
                UINT32 *Percetage = (UINT32 *) info;
                AmbaPrint("AmpUT_DualVinEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY, Percetage=%d!!!!!!!!!!",Percetage);
            }
            break;
        default:
            AmbaPrint("AmpUT_DualVinEnc: Unknown %X info: %x", event, info);
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
static int AmpUT_DualVinEncPipeCallback(void *hdlr,UINT32 event, void *info)
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
static int AmpUT_DualVinEnc_DisplayStart(void)
{
    AMP_DISP_WINDOW_CFG_s window = {0};
    AMP_AREA_s WindowArea = {0};
    UINT16 FinalAspectRatio = 0;

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
        if (DualVideoVinSelect == VIN_PIP_ONLY) {
            WindowArea.Height = (VideoPipEncMgt[EncPipModeIdx].AspectRatio == VAR_16x9)? 360: 480;
        } else {
            WindowArea.Height = (VideoEncMgt[EncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
        }
        WindowArea.X = 0;
        WindowArea.Y = (480-WindowArea.Height)/2;
        if (VENC_IS_ROTATE(EncRotation) && LiveViewProcMode == 0) {
            if (DualVideoVinSelect == VIN_PIP_ONLY) {
                FinalAspectRatio = GET_VAR_X(VideoPipEncMgt[EncPipModeIdx].AspectRatio) |
                    (GET_VAR_Y(VideoPipEncMgt[EncPipModeIdx].AspectRatio)<<8);
            } else {
                FinalAspectRatio = GET_VAR_X(VideoEncMgt[EncModeIdx].AspectRatio) |
                    (GET_VAR_Y(VideoEncMgt[EncModeIdx].AspectRatio)<<8);
            }
        } else {
            if (DualVideoVinSelect == VIN_PIP_ONLY) {
                FinalAspectRatio = VideoPipEncMgt[EncPipModeIdx].AspectRatio;
            } else {
                FinalAspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
            }
        }
        AmpUT_DualVinEncVoutCalculate(&WindowArea, FinalAspectRatio, AMP_DISP_CHANNEL_DCHAN);

        window.TargetAreaOnPlane.Width = WindowArea.Width;
        window.TargetAreaOnPlane.Height = WindowArea.Height;
        window.TargetAreaOnPlane.X = WindowArea.X;
        window.TargetAreaOnPlane.Y = WindowArea.Y;
        window.SourceDesc.Enc.VinCh = (DualVideoVinSelect == VIN_PIP_ONLY)? VinPipChannel: VinChannel;
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
        if (VENC_IS_ROTATE(EncRotation) && LiveViewProcMode == 0) {
            if (DualVideoVinSelect == VIN_PIP_ONLY) {
                FinalAspectRatio = GET_VAR_X(VideoPipEncMgt[EncPipModeIdx].AspectRatio) |
                    (GET_VAR_Y(VideoPipEncMgt[EncPipModeIdx].AspectRatio)<<8);
            } else {
                FinalAspectRatio = GET_VAR_X(VideoEncMgt[EncModeIdx].AspectRatio) |
                    (GET_VAR_Y(VideoEncMgt[EncModeIdx].AspectRatio)<<8);
            }
        } else {
            if (DualVideoVinSelect == VIN_PIP_ONLY) {
                FinalAspectRatio = VideoPipEncMgt[EncPipModeIdx].AspectRatio;
            } else {
                FinalAspectRatio = VideoEncMgt[EncModeIdx].AspectRatio;
            }
        }
        AmpUT_DualVinEncVoutCalculate(&WindowArea, FinalAspectRatio, AMP_DISP_CHANNEL_FCHAN);

        window.TargetAreaOnPlane.Width = WindowArea.Width;
        window.TargetAreaOnPlane.Height = WindowArea.Height;
        window.TargetAreaOnPlane.X = WindowArea.X;
        window.TargetAreaOnPlane.Y = WindowArea.Y;
        window.SourceDesc.Enc.VinCh = (DualVideoVinSelect == VIN_PIP_ONLY)? VinPipChannel: VinChannel;
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

int AmpUT_DualVinEnc_CfaOut_Callback(AMBA_DSP_IMG_WARP_CALC_CFA_INFO_s *pCfaInfo)
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
int AmpUT_DualVinEnc_Init(int sensorID, int sensorPipID, int LcdID)
{
    int Er;
    UINT32 ChannelNumber = 0;
    void *TmpbufRaw = NULL;
    {
        extern int AmbaLoadDSPuCode(void);
        AmbaLoadDSPuCode(); /* Load uCodes from NAND */
    }

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
    ChannelNumber = 2;
    InputDeviceId = sensorID;
    memset(&VinChannel, 0x0, sizeof(AMBA_DSP_CHANNEL_ID_u));
    memset(&VinPipChannel, 0x0, sizeof(AMBA_DSP_CHANNEL_ID_u));
    VinChannel.Bits.VinID = 0;
    VinChannel.Bits.SensorID = 0x1;
    VideoEncMgt = RegisterMWUT_Sensor_Driver(VinChannel, sensorID);
    InputDeviceId = sensorID;

    VinPipChannel.Bits.VinID = 1;
    VinPipChannel.Bits.SensorID = B5_OV4689_PIP_SENSOR_CH_ID;
    VideoPipEncMgt = RegisterMWUT_Sensor_Driver(VinPipChannel, sensorPipID);
    InputPipDeviceId = sensorPipID;

    if (DualVideoVinSelect == VIN_MAIN_ONLY) {
        extern int App_Image_Init(UINT32 ChCount, int sensorID);
        extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
        AMBA_3A_OP_INFO_s AaaOpInfo = {DISABLE, DISABLE, DISABLE, DISABLE};

        App_Image_Init(2, sensorPipID);

        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, 1/*ChIndex*/, (UINT32)&AaaOpInfo, 0);
        App_Image_Init_Iq_Params(0, sensorID);
        App_Image_Init_Iq_Params(1, sensorID/*sensorPipID*/);
    } else if (DualVideoVinSelect == VIN_PIP_ONLY) {
        extern int App_Image_Init(UINT32 ChCount, int sensorID);
        extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
        AMBA_3A_OP_INFO_s AaaOpInfo = {DISABLE, DISABLE, DISABLE, DISABLE};

        App_Image_Init(2, sensorID);

        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, 0/*ChIndex*/, (UINT32)&AaaOpInfo, 0);
        App_Image_Init_Iq_Params(0, sensorID);
        App_Image_Init_Iq_Params(1, sensorID/*sensorPipID*/);
    } else if (DualVideoVinSelect == VIN_MAIN_PIP) {
        UINT32 MainImgChCount = 0, PIPImgChCount = 0, i = 0;
        extern int App_Image_Init(UINT32 ChCount, int sensorID);
        extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
        AMBA_SENSOR_DEVICE_INFO_s SensorDeviceInfo = {0};
        AmbaSensor_GetDeviceInfo(VinChannel, &SensorDeviceInfo);
        if (SensorDeviceInfo.HdrIsSupport == 1) {
            MainImgChCount = 2;
        } else {
            MainImgChCount = 1;
        }
        AmbaSensor_GetDeviceInfo(VinPipChannel, &SensorDeviceInfo);
        if (SensorDeviceInfo.HdrIsSupport == 1) {
            PIPImgChCount = 2;
        } else {
            PIPImgChCount = 1;
        }
        App_Image_Init((MainImgChCount+PIPImgChCount), sensorID);
        for (i = 0;i<MainImgChCount;i++) {
            App_Image_Init_Iq_Params(i, sensorID);
        }
        for (i = 0;i<PIPImgChCount;i++) {
            App_Image_Init_Iq_Params(i+MainImgChCount, sensorPipID);
        }
    }

    // Create semaphores for muxers
    if (AmbaKAL_SemCreate(&VideoEncPriSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Semaphore creation failed");
    }
    if (AmbaKAL_SemCreate(&VideoEncSecSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Semaphore creation failed");
    }
    if (AmbaKAL_SemCreate(&VideoPipEncPriSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Semaphore creation failed");
    }
    if (AmbaKAL_SemCreate(&VideoPipEncSecSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Semaphore creation failed");
    }

    // Prepare stacks for muxer tasks
    Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoEncPriMuxStack, (void **)&TmpbufRaw, 6400 + 32, 32);
    if (Er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    VideoEncPriMuxStack = (UINT8 *)ALIGN_32((UINT32)VideoEncPriMuxStack);
    Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoEncSecMuxStack, (void **)&TmpbufRaw, 6400 + 32, 32);
    if (Er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    VideoEncPipSecMuxStack = (UINT8 *)ALIGN_32((UINT32)VideoEncPipSecMuxStack);
    Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoEncPipPriMuxStack, (void **)&TmpbufRaw, 6400 + 32, 32);
    if (Er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    VideoEncPipPriMuxStack = (UINT8 *)ALIGN_32((UINT32)VideoEncPipPriMuxStack);
    Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoEncPipSecMuxStack, (void **)&TmpbufRaw, 6400 + 32, 32);
    if (Er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    VideoEncPipSecMuxStack = (UINT8 *)ALIGN_32((UINT32)VideoEncPipSecMuxStack);

    Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&DualVideoEncMqueuePool, (void **)&TmpbufRaw, sizeof(DUAL_VIDENC_MSG_S)*128, 32);
    if (Er != OK) {
        AmbaPrint("Out of memory for DualVideoEncMqueue!!");
    }
    DualVideoEncMqueuePool = (UINT8 *)ALIGN_32((UINT32)DualVideoEncMqueuePool);
    Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&DualVideoEncTaskStack, (void **)&TmpbufRaw, 2048, 32);
    if (Er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }
    DualVideoEncTaskStack = (UINT8 *)ALIGN_32((UINT32)DualVideoEncTaskStack);

    // Create muxer tasks
    if (AmbaKAL_TaskCreate(&VideoEncPriMuxTask, "Video Encoder UnitTest Primary Muxing Task", 50, \
         AmpUT_DualVinEnc_PriMuxTask, 0x0, VideoEncPriMuxStack, 6400, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoEnc UnitTest: Muxer task creation failed");
    }
    if (AmbaKAL_TaskCreate(&VideoEncSecMuxTask, "Video Encoder UnitTest Secondary stream Muxing Task", 50, \
         AmpUT_DualVinEnc_SecMuxTask, 0x0, VideoEncSecMuxStack, 6400, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoEnc UnitTest: Muxer task creation failed");
    }
    if (AmbaKAL_TaskCreate(&VideoPipEncPriMuxTask, "Video Encoder UnitTest PIP Primary Muxing Task", 50, \
         AmpUT_DualVinEnc_PIP_PriMuxTask, 0x0, VideoEncPipPriMuxStack, 6400, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoEnc UnitTest: Muxer task creation failed");
    }
    if (AmbaKAL_TaskCreate(&VideoPipEncSecMuxTask, "Video Encoder UnitTest PIP Secondary stream Muxing Task", 50, \
         AmpUT_DualVinEnc_PIP_SecMuxTask, 0x0, VideoEncPipSecMuxStack, 6400, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("VideoEnc UnitTest: Muxer task creation failed");
    }
    // Create mQueue
    if (AmbaKAL_MsgQueueCreate(&DualVideoEncMqueue, DualVideoEncMqueuePool, sizeof(DUAL_VIDENC_MSG_S), 128)!=OK ){
        AmbaPrint("VideoEnc UnitTest: DualVideoEncMqueue creation failed");
    }
        // Create task
    if ( AmbaKAL_TaskCreate(&DualVideoEncTask, "Dual Vin Video Enc Task", 60, \
         DualVideoEncTaskHandler, 0x0, DualVideoEncTaskStack,2048, AMBA_KAL_AUTO_START)!=OK){
         AmbaPrint("VideoEnc UnitTest: DualVideoEncTask creation failed");
    }
    // Initialize VIN module
    {
        AMP_VIN_INIT_CFG_s VinInitCfg = {0};

        AmpVin_GetInitDefaultCfg(&VinInitCfg);

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VinWorkBuffer, (void **)&TmpbufRaw, VinInitCfg.MemoryPoolSize + 32, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for vin!!");
        }
        VinWorkBuffer = (UINT8 *)ALIGN_32((UINT32)VinWorkBuffer);

        VinInitCfg.MemoryPoolAddr = VinWorkBuffer;
        AmpVin_Init(&VinInitCfg);
    }

    // Initialize VIDEOENC module
    {
        AMP_VIDEOENC_INIT_CFG_s EncInitCfg = {0};

        AmpVideoEnc_GetInitDefaultCfg(&EncInitCfg);

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VEncWorkBuffer, (void **)&TmpbufRaw, EncInitCfg.MemoryPoolSize + 32, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for enc!!");
        }
        VEncWorkBuffer = (UINT8 *)ALIGN_32((UINT32)VEncWorkBuffer);
        EncInitCfg.MemoryPoolAddr = VEncWorkBuffer;
        AmpVideoEnc_Init(&EncInitCfg);
    }

    // Initialize Image scheduler module
    {
        AMBA_IMG_SCHDLR_INIT_CFG_s ISInitCfg = {0};
        UINT32 ISPoolSize = 0;
        UINT32 mainViewNum = 2;  // Dual
        AMBA_DSP_IMG_PIPE_e Pipe = AMBA_DSP_IMG_PIPE_VIDEO;

        AmbaImgSchdlr_GetInitDefaultCfg(&ISInitCfg);
        ISInitCfg.MainViewNum = mainViewNum;
        AmbaImgSchdlr_QueryMemsize(mainViewNum, &ISPoolSize);
        ISInitCfg.MemoryPoolSize = ISPoolSize + ISInitCfg.MsgTaskStackSize;

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&ImgSchdlrWorkBuffer, (void **)&TmpbufRaw, ISInitCfg.MemoryPoolSize + 32, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for imgschdlr!!");
        }
        ImgSchdlrWorkBuffer = (UINT8 *)ALIGN_32((UINT32)ImgSchdlrWorkBuffer);

        ISInitCfg.IsoCfgNum = AmpResource_GetIKIsoConfigNumber(Pipe);
        ISInitCfg.MemoryPoolAddr = ImgSchdlrWorkBuffer;
        Er = AmbaImgSchdlr_Init(&ISInitCfg);
        if (Er != AMP_OK) {
            AmbaPrint("AmbaImgSchdlr_Init Fail!");
        }
    }

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
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorCyclicWorkBuffer, (void **)&TmpbufRaw, MemSize + 32, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for encMonitorCyclic!!");
        }
        EncMonitorCyclicWorkBuffer = (UINT8 *)ALIGN_32((UINT32)EncMonitorCyclicWorkBuffer);
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

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorServiceWorkBuffer, (void **)&TmpbufRaw, MemSize + 32, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for encMonitorService!!");
        }
        EncMonitorServiceWorkBuffer = (UINT8 *)ALIGN_32((UINT32)EncMonitorServiceWorkBuffer);
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

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorStrmWorkBuffer, (void **)&TmpbufRaw, MemSize + 32, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for encMonitorStream!!");
        }
        EncMonitorStrmWorkBuffer = (UINT8 *)ALIGN_32((UINT32)EncMonitorStrmWorkBuffer);
        MonitorStrmCfg.MemoryPoolSize = MemSize;
        MonitorStrmCfg.MemoryPoolAddr = EncMonitorStrmWorkBuffer;
        Er = AmbaEncMonitorStream_Init(&MonitorStrmCfg);
        if (Er != AMP_OK) {
            AmbaPrint("AmbaEncMonitorStream_Init Fail!");
        }
    }

    // Allocate bitstream buffers
    {
        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&H264BitsBuf, (void **)&TmpbufRaw, BITSFIFO_SIZE + 32, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }
        H264BitsBuf = (UINT8 *)ALIGN_32((UINT32)H264BitsBuf);

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&H264DescBuf, (void **)&TmpbufRaw, DESC_SIZE + 32, 32);
        if (Er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }
        H264DescBuf = (UINT8 *)ALIGN_32((UINT32)H264DescBuf);

        MjpgDescBuf=H264DescBuf;
        MjpgBitsBuf=H264BitsBuf;
    }

    // Initialize blending buffers
    {
        int j, k;

        memset(EncBlendY, 255, 256*100);
        memset(EncBlendAlphaY, 0, 256*100); // 0 means 100% use OSD layer
        memset(EncBlendUV, 128, 256*100);
        memset(EncBlendAlphaUV, 0, 256*100); // 0 means 100% use OSD layer

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

    }

    {   //init display
        if (AmpUT_Display_Init() == NG) {
            return NG;
        }
    }
    return 0;
}

/**
 * Liveview start
 *
 * @param [in]
 *
 */
int AmpUT_DualVinEnc_LiveviewStart(UINT32 modeIdx, UINT32 modePipIdx)
{
    AMBA_SENSOR_MODE_INFO_s MainSensorModeInfo = {0}, PipSensorModeInfo = {0};
    AMBA_SENSOR_MODE_ID_u MainMode = {0};
    AMBA_SENSOR_MODE_ID_u PipMode = {0};

    EncModeIdx = modeIdx;
    EncPipModeIdx = modePipIdx;

    MainMode.Data = (EncodeSystem == 0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;
    AmbaSensor_GetModeInfo(VinChannel, MainMode, &MainSensorModeInfo);

    PipMode.Data = (EncodeSystem == 0)? VideoPipEncMgt[EncPipModeIdx].InputMode: VideoPipEncMgt[EncPipModeIdx].InputPALMode;
    AmbaSensor_GetModeInfo(VinPipChannel, PipMode, &PipSensorModeInfo);

    /* Operate AAAOp */
    if (DualVideoVinSelect == VIN_MAIN_ONLY) {
        AMBA_3A_OP_INFO_s AaaOpEnableInfo = {ENABLE, ENABLE, DISABLE, ENABLE};
        AMBA_3A_OP_INFO_s AaaOpDisableInfo = {DISABLE, DISABLE, DISABLE, DISABLE};

        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, 0/*ChIndex*/, (UINT32)&AaaOpEnableInfo, 0);
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, 1/*ChIndex*/, (UINT32)&AaaOpDisableInfo, 0);
    } else if (DualVideoVinSelect == VIN_PIP_ONLY) {
        AMBA_3A_OP_INFO_s AaaOpEnableInfo = {ENABLE, ENABLE, DISABLE, ENABLE};
        AMBA_3A_OP_INFO_s AaaOpDisableInfo = {DISABLE, DISABLE, DISABLE, DISABLE};

        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, 0/*ChIndex*/, (UINT32)&AaaOpDisableInfo, 0);
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, 1/*ChIndex*/, (UINT32)&AaaOpEnableInfo, 0);
    } else if (DualVideoVinSelect == VIN_MAIN_PIP) {
        AMBA_3A_OP_INFO_s AaaOpEnableInfo = {ENABLE, ENABLE, DISABLE, ENABLE};

        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, 0/*ChIndex*/, (UINT32)&AaaOpEnableInfo, 0);
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, 1/*ChIndex*/, (UINT32)&AaaOpEnableInfo, 0);
    }


//#define KEEP_ALL_INSTANCES    // When defined, LiveviewStop won't delete all instances, so the next LiveviewStart is equal to change resolution.
#ifdef KEEP_ALL_INSTANCES
    if (VideoEncVinA) {
        AmpUT_DualVinEnc_ChangeResolution(modeIdx);
        AmpEnc_StartLiveview(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
        return 0;
    }
#endif

    AmbaPrint(" ========================================================= ");
    if (DualVideoVinSelect != VIN_PIP_ONLY)
        AmbaPrint(" AmbaUnitTest: Liveview at %s", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
    if (DualVideoVinSelect != VIN_MAIN_ONLY)
        AmbaPrint(" AmbaUnitTest: LiveviewPIP at %s", MWUT_GetInputVideoModeName(EncPipModeIdx, EncodeSystem));
    AmbaPrint(" =========================================================");

    // Force Liveview Process Mode
    {
        if (ExtLiveViewProcMode != 0xFF) {
            LiveViewProcMode = ExtLiveViewProcMode;
            ExtLiveViewProcMode = 0xFF;
        }

        if (ExtLiveViewAlgoMode != 0xFF) {
            LiveViewAlgoMode = ExtLiveViewAlgoMode;
            ExtLiveViewAlgoMode = 0xFF;
        }

        if (ExtLiveViewOSMode != 0xFF) {
            LiveViewOSMode = ExtLiveViewOSMode;
            ExtLiveViewOSMode = 0xFF;
        } else {
            if (DualVideoVinSelect == VIN_PIP_ONLY) {
                LiveViewOSMode = VideoPipEncMgt[EncPipModeIdx].OSMode;
            } else {
                LiveViewOSMode = VideoEncMgt[EncModeIdx].OSMode;
            }
        }
    }

    // Create Vin instance
    if (VideoEncVinA == 0){// && DualVideoVinSelect != VIN_PIP_ONLY) {
        AMP_VIN_HDLR_CFG_s VinCfg = {0};
        AMP_VIN_LAYOUT_CFG_s Layout[2]; // Dualstream from same vin/vcapwindow

        memset(&Layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s)*2);


        AmpVin_GetDefaultCfg(&VinCfg);

#define CONFIG_WHEN_CREATE  // When defined, setup everything when creation. If not defined, configurations can be set after creation and before liveviewStart
#ifdef CONFIG_WHEN_CREATE
        VinCfg.Channel = VinChannel;
        VinCfg.Mode = MainMode;
        VinCfg.LayoutNumber = 2;

        if (VideoOBModeEnable && MainSensorModeInfo.OutputInfo.OpticalBlackPixels.Width && MainSensorModeInfo.OutputInfo.OpticalBlackPixels.Height) {
            INT16 ActStartX = MIN2(MainSensorModeInfo.OutputInfo.OpticalBlackPixels.StartX, MainSensorModeInfo.OutputInfo.RecordingPixels.StartX);
            INT16 ActStartY = MIN2(MainSensorModeInfo.OutputInfo.OpticalBlackPixels.StartY, MainSensorModeInfo.OutputInfo.RecordingPixels.StartY);

            VinCfg.HwCaptureWindow.Width = MainSensorModeInfo.OutputInfo.OutputWidth - ActStartX;
            VinCfg.HwCaptureWindow.Height = MainSensorModeInfo.OutputInfo.OutputHeight - ActStartY;
            VinCfg.HwCaptureWindow.X = ActStartX;
            VinCfg.HwCaptureWindow.Y = ActStartY;
            Layout[0].ActiveArea.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
            Layout[0].ActiveArea.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
            Layout[0].ActiveArea.X = MainSensorModeInfo.OutputInfo.RecordingPixels.StartX - ActStartX;
            Layout[0].ActiveArea.Y = MainSensorModeInfo.OutputInfo.RecordingPixels.StartY - ActStartY;
            Layout[0].OBArea.Width = MainSensorModeInfo.OutputInfo.OpticalBlackPixels.Width;
            Layout[0].OBArea.Height = MainSensorModeInfo.OutputInfo.OpticalBlackPixels.Height;
            Layout[0].OBArea.X = MainSensorModeInfo.OutputInfo.OpticalBlackPixels.StartX - ActStartX;
            Layout[0].OBArea.Y = MainSensorModeInfo.OutputInfo.OpticalBlackPixels.StartY - ActStartY;
            Layout[0].EnableOBArea = 1;
        } else {
            Layout[0].ActiveArea.Width = VinCfg.HwCaptureWindow.Width = VideoEncMgt[EncModeIdx].CaptureWidth;
            Layout[0].ActiveArea.Height = VinCfg.HwCaptureWindow.Height = VideoEncMgt[EncModeIdx].CaptureHeight;
            Layout[0].ActiveArea.X = VinCfg.HwCaptureWindow.X = MainSensorModeInfo.OutputInfo.RecordingPixels.StartX + \
                (((MainSensorModeInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
            Layout[0].ActiveArea.Y = VinCfg.HwCaptureWindow.Y = MainSensorModeInfo.OutputInfo.RecordingPixels.StartY + \
                (((MainSensorModeInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)&0xFFFE);
            Layout[0].EnableOBArea = 0;
        }

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
        Layout[1].DzoomFactorX = 1<<16;
        Layout[1].DzoomFactorY = 1<<16;
        Layout[1].DzoomOffsetX = 0;
        Layout[1].DzoomOffsetY = 0;
        Layout[1].MainviewReportRate = 1;
        VinCfg.Layout = Layout;
#endif
        VinCfg.cbEvent = AmpUT_DualVinEncVinEventCallback;
        VinCfg.cbSwitch= AmpUT_DualVinEncVinSwitchCallback;

        AmpVin_Create(&VinCfg, &VideoEncVinA);
    }
    if (VideoEncVinPip == NULL){// && DualVideoVinSelect != VIN_MAIN_ONLY) {
        AMP_VIN_HDLR_CFG_s VinCfg = {0};
        AMP_VIN_LAYOUT_CFG_s Layout[1]; // Dualstream from same vin/vcapwindow

        memset(&Layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s));
        AmpVin_GetDefaultCfg(&VinCfg);
        VinCfg.Channel = VinPipChannel;
        VinCfg.Mode = PipMode;
        VinCfg.LayoutNumber = 1;

        Layout[0].ActiveArea.Width = VinCfg.HwCaptureWindow.Width = VideoPipEncMgt[EncPipModeIdx].CaptureWidth;
        Layout[0].ActiveArea.Height = VinCfg.HwCaptureWindow.Height = VideoPipEncMgt[EncPipModeIdx].CaptureHeight;
        Layout[0].ActiveArea.X = VinCfg.HwCaptureWindow.X = PipSensorModeInfo.OutputInfo.RecordingPixels.StartX + \
            (((PipSensorModeInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        Layout[0].ActiveArea.Y = VinCfg.HwCaptureWindow.Y = PipSensorModeInfo.OutputInfo.RecordingPixels.StartY + \
            (((PipSensorModeInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)&0xFFFE);
        Layout[0].EnableOBArea = 0;
        Layout[0].Width = VideoPipEncMgt[EncPipModeIdx].MainWidth;
        Layout[0].Height = VideoPipEncMgt[EncPipModeIdx].MainHeight;
        Layout[0].EnableSourceArea = 0; // Get all capture window to main
        Layout[0].DzoomFactorX = InitZoomX; // 16.16 format
        Layout[0].DzoomFactorY = InitZoomY;
        Layout[0].DzoomOffsetX = 0;
        Layout[0].DzoomOffsetY = 0;
        Layout[0].MainviewReportRate = (MainviewReportRate)? MainviewReportRate: VideoPipEncMgt[EncPipModeIdx].ReportRate;
        VinCfg.Layout = Layout;

        VinCfg.cbEvent = AmpUT_DualVinEncVinEventCallback;
        VinCfg.cbSwitch= AmpUT_DualVinEncVinSwitchCallback;

        AmpVin_Create(&VinCfg, &VideoEncVinPip);
    }
    // Remember frame/field rate for muxers storing frame rate info
    if (EncodeSystem == 0) {
        PIPEncFrameRate = VideoPipEncMgt[EncPipModeIdx].TimeScale/VideoPipEncMgt[EncPipModeIdx].TickPerPicture;
    } else {
        PIPEncFrameRate = VideoPipEncMgt[EncPipModeIdx].TimeScalePAL/VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
    }

    if (EncodeSystem == 0) {
        EncFrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;
    } else {
        EncFrameRate = VideoEncMgt[EncModeIdx].TimeScalePAL/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
    }


    // Create ImgSchdlr instance
    if (ImgSchdlr == NULL) {// && DualVideoVinSelect != VIN_PIP_ONLY) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg;

        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

        ImgSchdlrCfg.MainViewID = 0; //single channel have one MainView
        ImgSchdlrCfg.Channel = VinChannel;
        ImgSchdlrCfg.Vin = VideoEncVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_DualVinEncImgSchdlrCallback;
        if (LiveViewProcMode && LiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
            if (MainSensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                ImgSchdlrCfg.VideoProcMode |= 0x10;
            }
        }
        ImgSchdlrCfg.AAAStatSampleRate = (MainviewReportRate)? MainviewReportRate: VideoEncMgt[EncModeIdx].ReportRate;
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &ImgSchdlr);  // One MainViewID (not vin) need one scheduler.
    }

    if (PIPImgSchdlr == NULL) {// && DualVideoVinSelect != VIN_MAIN_ONLY) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg;

        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

        ImgSchdlrCfg.MainViewID = 1; //single channel have one MainView
        ImgSchdlrCfg.Channel = VinPipChannel;
        ImgSchdlrCfg.Vin = VideoEncVinPip;
        ImgSchdlrCfg.cbEvent = AmpUT_DualVinEncImgSchdlrCallback;
        if (LiveViewProcMode && LiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
            if (PipSensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                ImgSchdlrCfg.VideoProcMode |= 0x10;
            }
        }
        ImgSchdlrCfg.AAAStatSampleRate = (MainviewReportRate)? MainviewReportRate: VideoPipEncMgt[EncPipModeIdx].ReportRate;
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &PIPImgSchdlr);  // One MainViewID (not vin) need one scheduler.
    }

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
            BrcCfg.CmplxHdlr.GetDayLumaThresCB = AmpUT_DualVinEnc_GetDayLumaThresholdCB;
            BrcCfg.CmplxHdlr.GetComplexityRangeCB = AmpUT_DualVinEnc_GetSceneComplexityRangeCB;
            BrcCfg.AverageBitrate = (UINT32)VideoEncMgt[EncModeIdx].AverageBitRate*1000*1000;
            BrcCfg.MaxBitrate = (UINT32)VideoEncMgt[EncModeIdx].MaxBitRate*1000*1000;
            BrcCfg.MinBitrate = (UINT32)VideoEncMgt[EncModeIdx].MinBitRate*1000*1000;
            BrcCfg.emonStrmHdlr = EncMonitorStrmHdlrPri;
            BrcCfg.VideoOSMode = LiveViewOSMode;
            BrcCfg.VideoProcMode = LiveViewProcMode;
            BrcCfg.VideoHdrMode = (MainSensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE)? 1: 0;
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
        AMBA_IMG_ENC_MONITOR_AQP_HDLR_CFG_s AqpCfg = {0};
        AmbaEncMonitorAQP_GetDefaultCfg(&AqpCfg);

        if (AqpHdlrPri == NULL) {
            AqpCfg.Period = 1000; // 1sec
            AqpCfg.emonStrmHdlr = EncMonitorStrmHdlrPri;
            AqpCfg.AqpCB = AmpUT_DualVinEnc_AqpPriStream;
            AmbaEncMonitorAQP_RegisterService(&AqpCfg, &AqpHdlrPri);
        }

        if ((EncDualStream || EncDualHDStream) && AqpHdlrSec == NULL) {
            AqpCfg.AqpCB = AmpUT_DualVinEnc_AqpSecStream;
            AqpCfg.emonStrmHdlr = EncMonitorStrmHdlrSec;
            AmbaEncMonitorAQP_RegisterService(&AqpCfg, &AqpHdlrSec);
        }
    }

    // Create video encoder instances
    // we treat Pri for MainVin and Sec for PipVin
    if (VideoEncPri == 0 && DualVideoVinSelect != VIN_PIP_ONLY) {
        AMP_VIDEOENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s EncLayer = {0};

        EncCfg.MainLayout.Layer = &EncLayer;
        AmpVideoEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_DualVinEncCallback;

#ifdef CONFIG_WHEN_CREATE
        // Assign main layout in single chan
        EncCfg.MainLayout.Width = VideoEncMgt[EncModeIdx].MainWidth;
        EncCfg.MainLayout.Height = VideoEncMgt[EncModeIdx].MainHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = EncPriInteralce; //TBD
        EncCfg.MainTimeScale = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TimeScale: VideoEncMgt[EncModeIdx].TimeScalePAL;
        EncCfg.MainTickPerPicture = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].TickPerPicture: VideoEncMgt[EncModeIdx].TickPerPicturePAL;
        EncCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
        EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
        EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
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
            AmpUT_DualVinEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
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
        if (MainSensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
            EncCfg.LiveViewHdrMode = LiveViewHdrMode[0] = 1;
        } else {
            EncCfg.LiveViewHdrMode = LiveViewHdrMode[0] = 0;
        }

        EncCfg.LiveviewOBModeEnable = VideoOBModeEnable;
        EncCfg.VinSelect = DualVideoVinSelect;
       // Create primary stream handler
        AmpVideoEnc_Create(&EncCfg, &VideoEncPri); // Don't have to worry about h.264 spec settings when liveview

        if (EncDualStream) {
            if (SecStreamCustomWidth) {
                EncCfg.MainLayout.Width = SecStreamCustomWidth;
                EncCfg.MainLayout.Height = SecStreamCustomHeight;
            } else {
                EncCfg.MainLayout.Width = SEC_STREAM_WIDTH;
                EncCfg.MainLayout.Height = SEC_STREAM_HEIGHT;
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
            if (DualVideoVinSelect == VIN_MAIN_PIP) {
                EncCfg.StreamId = AMP_VIDEOENC_STREAM_TERTIARY;
            } else {
                EncCfg.StreamId = AMP_VIDEOENC_STREAM_SECONDARY;
            }
            EncCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
            EncCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
            EncCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
            EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
            EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
            EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
            EncCfg.LiveviewOBModeEnable = VideoOBModeEnable;
            // Create secondary stream handler
            AmpVideoEnc_Create(&EncCfg, &VideoEncSec); // Don't have to worry about h.264 spec settings when liveview
        }
    } else {
        // reset some global parame when PIP only
        LiveViewHdrMode[0] = 0;
    }

    if (VideoPipEncPri == 0 && DualVideoVinSelect != VIN_MAIN_ONLY) {
        AMP_VIDEOENC_HDLR_CFG_s EncCfg = {0};
        AMP_VIDEOENC_LAYER_DESC_s EncLayer = {0};

        EncCfg.MainLayout.Layer = &EncLayer;
        AmpVideoEnc_GetDefaultCfg(&EncCfg);

        // Assign callback
        EncCfg.cbEvent = AmpUT_DualVinEncCallback;

        // Assign Secondary main layout
        EncCfg.MainLayout.Width = VideoPipEncMgt[EncPipModeIdx].MainWidth;
        EncCfg.MainLayout.Height = VideoPipEncMgt[EncPipModeIdx].MainHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = EncPriInteralce; //TBD
        EncCfg.MainTimeScale = (EncodeSystem==0)? VideoPipEncMgt[EncPipModeIdx].TimeScale: VideoPipEncMgt[EncPipModeIdx].TimeScalePAL;
        EncCfg.MainTickPerPicture = (EncodeSystem==0)? VideoPipEncMgt[EncPipModeIdx].TickPerPicture: VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
        EncCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
        EncCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
        EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
        EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
        EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
        EncLayer.SourceType = AMP_ENC_SOURCE_VIN;
        EncLayer.Source = VideoEncVinPip;
        EncLayer.EnableSourceArea = 0;  // No source cropping
        EncLayer.EnableTargetArea = 0;  // No target pip
        EncLayer.SourceLayoutId = (DualVideoVinSelect == VIN_PIP_ONLY)? 0: 1;
        EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
        EncCfg.StreamId = (DualVideoVinSelect == VIN_PIP_ONLY)?
            AMP_VIDEOENC_STREAM_PRIMARY: AMP_VIDEOENC_STREAM_SECONDARY;
        EncCfg.LiveviewOBModeEnable = VideoOBModeEnable;
        EncCfg.VinSelect = DualVideoVinSelect;

        if (DualVideoVinSelect == VIN_PIP_ONLY) {
            UINT8 *dspWorkAddr;
            UINT32 dspWorkSize;
            AmpUT_DualVinEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
            EncCfg.DspWorkBufAddr = dspWorkAddr;
            EncCfg.DspWorkBufSize = dspWorkSize;
            if (LiveViewProcMode == 0) {
                if (VideoPipEncMgt[EncPipModeIdx].MainWidth > 1920 || \
                    VideoPipEncMgt[EncPipModeIdx].CaptureWidth > 1920) {
                    if (LiveViewOSMode == 0) {
                        AmbaPrint("[UT_Video]Force to use NonOS Mode?? (VcapW %d,MainW %d)", VideoPipEncMgt[EncPipModeIdx].MainWidth, VideoPipEncMgt[EncPipModeIdx].CaptureWidth);
                    }
                }
                if ((VideoPipEncMgt[EncPipModeIdx].TimeScale/VideoPipEncMgt[EncPipModeIdx].TickPerPicture)>60 && \
                    LiveViewOSMode == 1) {
                    LiveViewOSMode = 0;
                    AmbaPrint("High Fr (>60fps) Force to use Basic in the Express mode");
                }
            }

            EncCfg.LiveViewProcMode = LiveViewProcMode;
            EncCfg.LiveViewAlgoMode = LiveViewAlgoMode;
            EncCfg.LiveViewOSMode = LiveViewOSMode;
            if (PipSensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                EncCfg.LiveViewHdrMode = LiveViewHdrMode[1] = 1;
            } else {
                EncCfg.LiveViewHdrMode = LiveViewHdrMode[1] = 0;
            }
        }

        // Create secondary stream handler
        AmpVideoEnc_Create(&EncCfg, &VideoPipEncPri); // Don't have to worry about h.264 spec settings when liveview
        if (EncDualStream) {
            if (SecStreamCustomWidth) {
                EncCfg.MainLayout.Width = SecStreamCustomWidth;
                EncCfg.MainLayout.Height = SecStreamCustomHeight;
            } else {
                EncCfg.MainLayout.Width = SEC_STREAM_WIDTH;
                EncCfg.MainLayout.Height = SEC_STREAM_HEIGHT;
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
            EncLayer.Source = VideoEncVinPip;
            EncLayer.EnableSourceArea = 0;  // No source cropping
            EncLayer.EnableTargetArea = 0;  // No target pip
            EncLayer.SourceLayoutId = 1; // from 2nd layout of VideoEncVinA
            EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
            if (DualVideoVinSelect == VIN_MAIN_PIP) {
                EncCfg.StreamId = AMP_VIDEOENC_STREAM_QUATERNARY;
            } else {
                EncCfg.StreamId = AMP_VIDEOENC_STREAM_SECONDARY;
            }
            EncCfg.SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
            EncCfg.SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_POWERSAVING;
            EncCfg.SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
            EncCfg.SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
            EncCfg.SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
            EncCfg.SysFreq.MaxCoreFreq = CustomMaxCoreFreq;
            EncCfg.LiveviewOBModeEnable = VideoOBModeEnable;
            // Create secondary stream handler
            AmpVideoEnc_Create(&EncCfg, &VideoPipEncSec); // Don't have to worry about h.264 spec settings when liveview
        }
    } else {
        // reset some global parame when Main only
        LiveViewHdrMode[0] = 0;
    }

    AmbaPrint("Turn %s Tv", TvLiveview? "ON": "OFF");
    AmbaPrint("Turn ON LCD");
    LCDLiveview = 1;

    // config window and start display
    AmpUT_DualVinEnc_DisplayStart();

    // Register pipeline
    {
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};
        INT Idx=0;
        // Register pipeline
        AmpEnc_GetDefaultCfg(&PipeCfg);
        //pipeCfg.cbEvent
        if (DualVideoVinSelect == VIN_MAIN_ONLY) {
            PipeCfg.encoder[0] = VideoEncPri;
            PipeCfg.numEncoder = 1;
            if (EncDualStream) {
                PipeCfg.encoder[1] = VideoEncSec;
                PipeCfg.numEncoder = 2;
            }
        } else if (DualVideoVinSelect == VIN_PIP_ONLY) {
            PipeCfg.encoder[0] = VideoPipEncPri;
            PipeCfg.numEncoder = 1;
            if (EncDualStream) {
                PipeCfg.encoder[1] = VideoPipEncSec;
                PipeCfg.numEncoder = 2;
            }
        } else if (DualVideoVinSelect == VIN_MAIN_PIP){
            PipeCfg.encoder[0] = VideoEncPri;
            Idx++;
            if (EncDualStream) {
                PipeCfg.encoder[Idx] = VideoEncSec;
                Idx++;
            }
            PipeCfg.encoder[Idx] = VideoPipEncPri;
            Idx++;
            if (EncDualStream) {
                PipeCfg.encoder[Idx] = VideoPipEncSec;
                Idx++;
            }
            PipeCfg.numEncoder = Idx;
        }
        PipeCfg.cbEvent = AmpUT_DualVinEncPipeCallback;
        PipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&PipeCfg, &VideoEncPipe);

        AmpEnc_Add(VideoEncPipe);
    }

    //
    // Note: For A9, Bitstream-specific configs can be assigned here (before encode start), or when codec instance creation.
    //       But for A7L/A12 family, it shall be assigned when codec instance creation and before liveview start.
    //
    {
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
        AMP_VIDEOENC_BITSTREAM_CFG_s BitsCfg;

        memset(&BitsCfg, 0x0, sizeof(AMP_VIDEOENC_BITSTREAM_CFG_s));
        BitsCfg.Rotation = EncRotation;
        BitsCfg.TimeLapse = 0;
        BitsCfg.VideoThumbnail = DualEncThumbnail[0];
        if (DualVideoVinSelect == VIN_PIP_ONLY) {
            BitsCfg.PIVMaxWidth = VideoPipEncMgt[EncPipModeIdx].MaxPivWidth;
        } else {
            BitsCfg.PIVMaxWidth = VideoEncMgt[EncModeIdx].MaxPivWidth;
        }

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
                AmpUT_DualVinEnc_InitMJpegDqt(MjpegQuantMatrix, MjpegQuality);
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
        if (VideoPipEncPri) {
            if (EncSecSpecH264) {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
                H264Cfg = &BitsCfg.Spec.H264Cfg;
                H264Cfg->GopM = VideoPipEncMgt[EncPipModeIdx].GopM;
                H264Cfg->GopN = GOP_N;
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
                H264Cfg->TimeScale = (EncodeSystem==0)? VideoPipEncMgt[EncPipModeIdx].TimeScale: VideoPipEncMgt[EncPipModeIdx].TimeScalePAL;
                H264Cfg->TickPerPicture = (EncodeSystem==0)? VideoPipEncMgt[EncPipModeIdx].TickPerPicture: VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
                H264Cfg->StopMethod = EncStopMethod;

                HeaderInfo.GopM = H264Cfg->GopM;
                HeaderInfo.Width = VideoPipEncMgt[EncPipModeIdx].MainWidth;
                HeaderInfo.Height = VideoPipEncMgt[EncPipModeIdx].MainHeight;
                HeaderInfo.Interlace = EncSecInteralce;
                HeaderInfo.Rotation = EncRotation;

                // Get default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->SPS.level_idc = 42;
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = VideoPipEncMgt[EncPipModeIdx].BrcMode;
                H264Cfg->BitRateControl.AverageBitrate = (UINT32)(VideoPipEncMgt[EncPipModeIdx].AverageBitRate * 1E6);
                if (VideoPipEncMgt[EncPipModeIdx].BrcMode == VIDEOENC_SMART_VBR) {
                    H264Cfg->BitRateControl.MaxBitrate = (UINT32)(VideoPipEncMgt[EncPipModeIdx].MaxBitRate * 1E6);
                    H264Cfg->BitRateControl.MinBitrate = (UINT32)(VideoPipEncMgt[EncPipModeIdx].MinBitRate * 1E6);
                }
                H264Cfg->QualityControl.IBeatMode = EncIBeat;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                AmpUT_DualVinEnc_InitMJpegDqt(MjpegQuantMatrix, MjpegQuality);
                MjpegCfg->QuantMatrixAddr = MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(VideoPipEncPri, &BitsCfg);
        }
        if (VideoPipEncSec) {
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
            AmpVideoEnc_SetBitstreamConfig(VideoPipEncSec, &BitsCfg);
        }
    }

    AmpEnc_StartLiveview(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);

    return 0;
}

int AmpUT_DualVinEnc_EncodeStart(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;
    AMP_ENC_BITSBUFFER_CFG_s  BitsBufCfg;
    UINT16 SecEncWidth, SecEncHeight;

    if (VideoEncPri) {
        AmbaPrint(" ========================================================= ");
        AmbaPrint(" VideoEncode : %s", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
        AmbaPrint(" VideoEncode : CaptureWin[%dx%d]", VideoEncMgt[EncModeIdx].CaptureWidth, VideoEncMgt[EncModeIdx].CaptureHeight);
        AmbaPrint(" VideoEncode : AvgBrate %dMbyte", VideoEncMgt[EncModeIdx].AverageBitRate);
        AmbaPrint(" VideoEncode : PriStream EncWin [%dx%d]", VideoEncMgt[EncModeIdx].MainWidth, VideoEncMgt[EncModeIdx].MainHeight);
        if (EncPriSpecH264) {
            AmbaPrint(" VideoEncode : PriStream H264 GopM   %d", VideoEncMgt[EncModeIdx].GopM);
            AmbaPrint(" VideoEncode : PriStream H264 GopN   %d", GOP_N);
            AmbaPrint(" VideoEncode : PriStream H264 GopIDR %d", GOP_IDR);
        } else {
            AmbaPrint(" VideoEncode : PriStream MJpeg");
        }

        if (EncDualStream || EncDualHDStream) {
            if (EncDualHDStream) {
                if (SecStreamCustomWidth) {
                    SecEncWidth = SecStreamCustomWidth;
                    SecEncHeight = SecStreamCustomHeight;
                } else {
                    MWUT_InputSetDualHDWindow(VideoEncMgt, EncModeIdx, \
                        &SecEncWidth, &SecEncHeight);
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
    }
    if (VideoPipEncPri) {
        AmbaPrint(" ========================================================= ");
        AmbaPrint(" VideoEncode : %s", MWUT_GetInputVideoModeName(EncPipModeIdx, EncodeSystem));
        AmbaPrint(" VideoEncode : CaptureWin[%dx%d]", VideoPipEncMgt[EncPipModeIdx].CaptureWidth, VideoPipEncMgt[EncPipModeIdx].CaptureHeight);
        AmbaPrint(" VideoEncode : AvgBrate %dMbyte", VideoPipEncMgt[EncPipModeIdx].AverageBitRate);
        AmbaPrint(" VideoEncode : PriStream EncWin [%dx%d]", VideoPipEncMgt[EncPipModeIdx].MainWidth, VideoPipEncMgt[EncPipModeIdx].MainHeight);
        if (EncPriSpecH264) {
            AmbaPrint(" VideoEncode : PriStream H264 GopM   %d", VideoPipEncMgt[EncPipModeIdx].GopM);
            AmbaPrint(" VideoEncode : PriStream H264 GopN   %d", GOP_N);
            AmbaPrint(" VideoEncode : PriStream H264 GopIDR %d", GOP_IDR);
        } else {
            AmbaPrint(" VideoEncode : PriStream MJpeg");
        }

        if (EncDualStream || EncDualHDStream) {
            if (EncDualHDStream) {
                if (SecStreamCustomWidth) {
                    SecEncWidth = SecStreamCustomWidth;
                    SecEncHeight = SecStreamCustomHeight;
                } else {
                    MWUT_InputSetDualHDWindow(VideoEncMgt, EncModeIdx, \
                        &SecEncWidth, &SecEncHeight);
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
    }

    if (EncSecSpecH264 && (EncDualStream || EncDualHDStream)) {
        AmbaPrint(" VideoEncode : SecStream H264 GopM   %d", SEC_STREAM_GOP_M);
        AmbaPrint(" VideoEncode : SecStream H264 GopN   %d", SEC_STREAM_GOP_N);
        AmbaPrint(" VideoEncode : SecStream H264 GopIDR %d", SEC_STREAM_GOP_IDR);
    } else {
        if (EncDualStream || EncDualHDStream) {
            AmbaPrint(" VideoEncode : SecStream MJpeg");
        }
    }
    AmbaPrint(" =========================================================");

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
    if (VideoEncPri) {
        if (VirtualPriFifoHdlr == NULL) {
            //Set BaseInformation only once
            if (VideoEncPri) {
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
            }

            // create a virtual fifo
            AmpFifo_GetDefaultCfg(&fifoDefCfg);
            fifoDefCfg.hCodec = VideoEncPri;
            fifoDefCfg.IsVirtual = 1;
            fifoDefCfg.NumEntries = DESC_SIZE/sizeof(AMBA_DSP_EVENT_ENC_PIC_READY_s);;
            fifoDefCfg.cbEvent = AmpUT_DualVinEnc_FifoCB;
            AmpFifo_Create(&fifoDefCfg, &VirtualPriFifoHdlr);
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
    }
    if (VideoEncSec) {
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
            AmpFifo_GetDefaultCfg(&fifoDefCfg);
            fifoDefCfg.hCodec = VideoEncSec;
            fifoDefCfg.IsVirtual = 1;
            fifoDefCfg.NumEntries = DESC_SIZE/sizeof(AMBA_DSP_EVENT_ENC_PIC_READY_s);;
            fifoDefCfg.cbEvent = AmpUT_DualVinEnc_FifoCB;
            AmpFifo_Create(&fifoDefCfg, &VirtualSecFifoHdlr);
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
    }
    if (VideoPipEncPri) {
        if (VirtualPIPPriFifoHdlr == NULL) {
            //Set BaseInformation only once
            if (VideoPipEncPri) {
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
                AmpVideoEnc_SetBitstreamBuffer(VideoPipEncPri, &BitsBufCfg);
            }

            // create a virtual fifo
            AmpFifo_GetDefaultCfg(&fifoDefCfg);
            fifoDefCfg.hCodec = VideoPipEncPri;
            fifoDefCfg.IsVirtual = 1;
            fifoDefCfg.NumEntries = DESC_SIZE/sizeof(AMBA_DSP_EVENT_ENC_PIC_READY_s);;
            fifoDefCfg.cbEvent = AmpUT_DualVinEnc_FifoCB;
            AmpFifo_Create(&fifoDefCfg, &VirtualPIPPriFifoHdlr);
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
            AmpVideoEnc_SetBitstreamBuffer(VideoPipEncPri, &BitsBufCfg);
            AmpFifo_EraseAll(VirtualPIPPriFifoHdlr);
        }
    }
    if (VideoPipEncSec) {
        if (VirtualPIPSecFifoHdlr == NULL) {
            AMP_ENC_BITSBUFFER_CFG_s  BitsBufCfg = {0};

            if (VideoPipEncSec) {
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
                AmpVideoEnc_SetBitstreamBuffer(VideoPipEncSec, &BitsBufCfg);
            }

            // create a virtual fifo
            AmpFifo_GetDefaultCfg(&fifoDefCfg);
            fifoDefCfg.hCodec = VideoPipEncSec;
            fifoDefCfg.IsVirtual = 1;
            fifoDefCfg.NumEntries = DESC_SIZE/sizeof(AMBA_DSP_EVENT_ENC_PIC_READY_s);;
            fifoDefCfg.cbEvent = AmpUT_DualVinEnc_FifoCB;
            AmpFifo_Create(&fifoDefCfg, &VirtualPIPSecFifoHdlr);
        } else {
            //Following are follow previous
            if (VideoPipEncSec) {
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
                AmpVideoEnc_SetBitstreamBuffer(VideoPipEncSec, &BitsBufCfg);
                AmpFifo_EraseAll(VirtualPIPSecFifoHdlr);
            }
        }
    }
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
    if (DualEncThumbnail[0]) {
        //issue PIV cmd to let SSP start turely video-encode
        AMP_VIDEOENC_PIV_CTRL_s PivCtrl;
        UINT32 iso = 2;
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
        if (LiveViewProcMode) { /* HybridMode shall only using LISO */
            iso = 1;
        }
        AmpEnc_StartRecord(VideoEncPipe, AMP_ENC_FUNC_FLAG_WAIT);
        AmpUT_DualVinEnc_PIV(PivCtrl, iso, cmpr, tsize, loop);
    } else {
        AmpEnc_StartRecord(VideoEncPipe, 0);
    }


    return 0;
}
int AmpUT_DualVinEnc_EncStop()
{
    AmpEnc_StopRecord(VideoEncPipe, 0);

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

    return 0;
}

int AmpUT_DualVinEnc_LiveviewStop(void)
{
    UINT32 Flag = AMP_ENC_FUNC_FLAG_WAIT;

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

    if (ImgSchdlr) {
        AmbaImgSchdlr_Enable(ImgSchdlr, 0);
    }
    if (PIPImgSchdlr) {
        AmbaImgSchdlr_Enable(PIPImgSchdlr, 0);
    }
    Amba_Img_VIn_Invalid(VideoEncPri, (UINT32 *)NULL);
    Amba_Img_VIn_Invalid(VideoPipEncPri, (UINT32 *)NULL);

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

    if (VideoEncVinA) {

        AmpVin_Delete(VideoEncVinA);
        VideoEncVinA = NULL;
    }

    if (VideoEncVinPip) {

        AmpVin_Delete(VideoEncVinPip);
        VideoEncVinPip = NULL;
    }
    if (ImgSchdlr) {
        AmbaImgSchdlr_Delete(ImgSchdlr);
        ImgSchdlr = NULL;
    }
    if (PIPImgSchdlr) {
        AmbaImgSchdlr_Delete(PIPImgSchdlr);
        PIPImgSchdlr = NULL;
    }
    if (VideoEncPri) {
        AmpVideoEnc_Delete(VideoEncPri);
        VideoEncPri = NULL;
    }
    if (VideoPipEncPri) {
        AmpVideoEnc_Delete(VideoPipEncPri);
        VideoPipEncPri = NULL;
    }
    if (VideoPipEncSec) {
        AmpVideoEnc_Delete(VideoPipEncSec);
        VideoPipEncSec = NULL;
    }

    if (LCDLiveview) {
        AmpUT_Display_Stop(0);
        AmpUT_Display_Window_Delete(0);
    }
    if (TvLiveview) {
        AmpUT_Display_Stop(1);
        AmpUT_Display_Window_Delete(1);
    }

    if (VirtualPriFifoHdlr) {
        AmpFifo_Delete(VirtualPriFifoHdlr);
        VirtualPriFifoHdlr = NULL;
    }
    if (VirtualPIPPriFifoHdlr) {
        AmpFifo_Delete(VirtualPIPPriFifoHdlr);
        VirtualPIPPriFifoHdlr = NULL;
    }
    if (VirtualSecFifoHdlr) {
        AmpFifo_Delete(VirtualSecFifoHdlr);
        VirtualSecFifoHdlr = NULL;
    }
    if (VirtualPIPSecFifoHdlr) {
        AmpFifo_Delete(VirtualPIPSecFifoHdlr);
        VirtualPIPSecFifoHdlr = NULL;
    }

#endif

    return 0;
}


int AmpUT_DualVinEnc_EncPause(void)
{
    if (Status == STATUS_ENCODE) {
        AmpEnc_PauseRecord(VideoEncPipe, 0);
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
        Status = STATUS_PAUSE;
    }
    return 0;
}

int AmpUT_DualVinEnc_EncResume(void)
{
    AMP_ENC_BITSBUFFER_CFG_s  BitsBufCfg = {0};
    if (VideoEncPri) {
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
    }

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
    if (VideoPipEncPri) {
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
        AmpVideoEnc_SetBitstreamBuffer(VideoPipEncPri, &BitsBufCfg);
    }

    if (VideoPipEncSec) {
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
        AmpVideoEnc_SetBitstreamBuffer(VideoPipEncSec, &BitsBufCfg);
    }
    AmbaPrint("H.264 Bits 0x%x size %d Desc 0x%x size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
    AmbaPrint("MJPEG Bits 0x%x size %d Desc 0x%x size %d", MjpgBitsBuf, BITSFIFO_SIZE, MjpgDescBuf, DESC_SIZE);

    if (Status == STATUS_PAUSE) {
        AmpEnc_ResumeRecord(VideoEncPipe, 0);
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
        Status = STATUS_ENCODE;
    }

    return 0;
}

int AmpUT_DualVinEnc_ChangeResolution(UINT32 modeIdx)
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
            ExtLiveViewProcMode = 0xFF;
        }

        if (ExtLiveViewAlgoMode != 0xFF) {
            LiveViewAlgoMode = ExtLiveViewAlgoMode;
            ExtLiveViewAlgoMode = 0xFF;
        }

        if (ExtLiveViewOSMode != 0xFF) {
            LiveViewOSMode = ExtLiveViewOSMode;
            ExtLiveViewOSMode = 0xFF;
        } else {
            LiveViewOSMode = VideoEncMgt[EncModeIdx].OSMode;
        }
    }

    memset(&Layout, 0x0, 2*sizeof(AMP_VIN_LAYOUT_CFG_s));

    Mode.Data = (EncodeSystem==0)? VideoEncMgt[EncModeIdx].InputMode: VideoEncMgt[EncModeIdx].InputPALMode;;
    memset(&SensorModeInfo, 0x0, sizeof(AMBA_SENSOR_MODE_INFO_s));
    AmbaSensor_GetModeInfo(VinChannel, Mode, &SensorModeInfo);

    {
        AmbaPrint("Turn %s Tv", TvLiveview? "ON": "OFF");
    }

    // Update VIN information
    VinCfg.Hdlr = VideoEncVinA;
    VinCfg.Mode = Mode;
    VinCfg.LayoutNumber = 2;
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
    Layout[1].DzoomFactorX = 1<<16;
    Layout[1].DzoomFactorY = 1<<16;
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
        ImgSchdlrCfg.cbEvent = AmpUT_DualVinEncImgSchdlrCallback;
        if (LiveViewProcMode && LiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &ImgSchdlr);  // One sensor (not vin) need one scheduler.
    }
    if (PIPImgSchdlr == NULL) {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg;

        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

        ImgSchdlrCfg.MainViewID = 1; //single channle have one MainView
        ImgSchdlrCfg.Channel = VinPipChannel;
        ImgSchdlrCfg.Vin = VideoEncVinPip;
        ImgSchdlrCfg.cbEvent = AmpUT_DualVinEncImgSchdlrCallback;
        if (LiveViewProcMode && LiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
        }
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &PIPImgSchdlr);  // One sensor (not vin) need one scheduler.
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
        MainCfg[0].LiveViewHdrMode = LiveViewHdrMode[0] = 1;
    } else {
        MainCfg[0].LiveViewHdrMode = LiveViewHdrMode[1] = 0;
    }

    MainCfg[0].SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[0].SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
    MainCfg[0].SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
    MainCfg[0].SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
    MainCfg[0].SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
    MainCfg[0].SysFreq.MaxCoreFreq = CustomMaxCoreFreq;

    {
        UINT8 *dspWorkAddr;
        UINT32 dspWorkSize;
        AmpUT_DualVinEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
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
    MainCfg[1].SysFreq.ArmCortexFreq = (CustomCortexFreq)? CustomCortexFreq: AMP_SYSTEM_FREQ_KEEPCURRENT;
    MainCfg[1].SysFreq.IdspFreq = (CustomIdspFreq)? CustomIdspFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
    MainCfg[1].SysFreq.CoreFreq = (CustomCoreFreq)? CustomCoreFreq: AMP_SYSTEM_FREQ_PERFORMANCE;
    MainCfg[1].SysFreq.MaxArmCortexFreq = CustomMaxCortexFreq;
    MainCfg[1].SysFreq.MaxIdspFreq = CustomMaxIdspFreq;
    MainCfg[1].SysFreq.MaxCoreFreq = CustomMaxCoreFreq;

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
        MainCfg[1].LiveViewHdrMode = LiveViewHdrMode[1] = 1;
    } else {
        MainCfg[1].LiveViewHdrMode = LiveViewHdrMode[1] = 0;
    }

    MainCfg[1].DspWorkBufAddr = 0;  // Don't want to change dsp buffer
    MainCfg[1].DspWorkBufSize = 0;  // Don't want to change dsp buffer
    NewSecLayer.EnableSourceArea = 0;
    NewSecLayer.EnableTargetArea = 0;
    NewSecLayer.LayerId = 0;
    NewSecLayer.SourceType = AMP_ENC_SOURCE_VIN;
    NewSecLayer.Source = VideoEncVinA;
    NewSecLayer.SourceLayoutId = 1;

    AmpVideoEnc_ConfigVinMain(1, &VinCfg, 2, &MainCfg[0]);

    // Remember new frame/field rate for muxer
    if (EncodeSystem == 0) {
        EncFrameRate = VideoEncMgt[EncModeIdx].TimeScale/VideoEncMgt[EncModeIdx].TickPerPicture;
    } else {
        EncFrameRate = VideoEncMgt[EncModeIdx].TimeScalePAL/VideoEncMgt[EncModeIdx].TickPerPicturePAL;
    }
    if (EncodeSystem == 0) {
        PIPEncFrameRate = VideoPipEncMgt[EncPipModeIdx].TimeScale/VideoPipEncMgt[EncPipModeIdx].TickPerPicture;
    } else {
        PIPEncFrameRate = VideoPipEncMgt[EncPipModeIdx].TimeScalePAL/VideoPipEncMgt[EncPipModeIdx].TickPerPicturePAL;
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
UINT32 AmpUT_DualVinEnc_DspWork_Calculate(UINT8 **addr, UINT32 *size)
{
    extern UINT8 *DspWorkAreaResvStart;
    extern UINT32 DspWorkAreaResvSize;

    if (CaptureMode == CAPTURE_MODE_NONE) {
        //only allocate MJPEG Bits
        (*addr) = DspWorkAreaResvStart;
        (*size) = DspWorkAreaResvSize - 15*1024*1024;
    }
    AmbaPrint("[DspWork_Calculate] Addr 0x%X, Sz %u", *addr, *size);
    return 0;
}

void AmpUT_DualVinEnc_JpegMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc;
    int Er;
    // A12 express PIV will use H264 BS
    UINT8 *BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;

    UINT8 dumpSkip = 0;

    AmbaPrint("AmpUT_DualVinEnc_JpegMuxTask Start");

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
            static UINT32 prevSegNum = 0;

            if (prevSegNum != Desc->SeqNum) {
                if (PivProcess) {
                    fnoPiv++;
                }
                prevSegNum = Desc->SeqNum;
            }

            if (PivProcess) {
                if (Desc->Type == AMP_FIFO_TYPE_JPEG_FRAME) {
                    sprintf(Fn,"%s:\\%04d_m_%02d.jpeg", DefaultSlot, fnoPri, fnoPiv);
                } else if (Desc->Type == AMP_FIFO_TYPE_THUMBNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d_t_%02d.jpeg", DefaultSlot, fnoPri, fnoPiv);
                } else if (Desc->Type == AMP_FIFO_TYPE_SCREENNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d_s_%02d.jpeg", DefaultSlot, fnoPri, fnoPiv);
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
                    sprintf(Fn,"%s:\\%04d_m.jpeg", DefaultSlot, fnoPri);
                } else if (Desc->Type == AMP_FIFO_TYPE_THUMBNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d_t.jpeg", DefaultSlot, fnoPri);
                } else if (Desc->Type == AMP_FIFO_TYPE_SCREENNAIL_FRAME) {
                    sprintf(Fn,"%s:\\%04d_s.jpeg", DefaultSlot, fnoPri);
                }
            }

            if (dumpSkip == 0) {
                outputJpegFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                AmbaPrint("%s opened", Fn);
            }

            // A12 express PIV will use H264 BS
            BitsLimit = H264BitsBuf + BITSFIFO_SIZE - 1;
        }
        #endif

        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            // EOS, stillenc do not prodice EOS anymore...
            #ifdef AMPUT_FILE_DUMP
                if (dumpSkip == 0) {
                    UT_DualVinEncodefsync((void *)outputJpegFile);
                    UT_DualVinEncodefclose((void *)outputJpegFile);
                    outputJpegFile = NULL;
                }
            #endif
            AmbaPrint("Muxer met EOS");
        } else {
            #ifdef AMPUT_FILE_DUMP
                {
                    static UINT8 i = 0;
                    if (dumpSkip == 0) {
                        AmbaPrint("Write: 0x%x sz %d limit %X",  Desc->StartAddr, Desc->Size, BitsLimit);
                        if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                            UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)outputJpegFile);
                        }else{
                            UT_DualVinEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)outputJpegFile);
                            // A12 express PIV will use H264 BS
                            UT_DualVinEncodefwrite((const void *)H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)outputJpegFile);

                        }
                        UT_DualVinEncodefsync((void *)outputJpegFile);
                        UT_DualVinEncodefclose((void *)outputJpegFile);
                    }
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
UINT32 AmpUT_DualVinEnc_PIVPOSTCB(AMP_STILLENC_POSTP_INFO_s *info)
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
            stillRawFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
            AmbaPrint("[Amp_UT]Dump Raw 0x%X %d %d %d  to %s Start!", \
                info->media.RawInfo.RawAddr, \
                info->media.RawInfo.RawPitch, \
                info->media.RawInfo.RawWidth, \
                info->media.RawInfo.RawHeight, Fn);
            UT_DualVinEncodefwrite((const void *)info->media.RawInfo.RawAddr, 1, info->media.RawInfo.RawPitch*info->media.RawInfo.RawHeight, (void *)stillRawFile);
            UT_DualVinEncodefsync((void *)stillRawFile);
            UT_DualVinEncodefclose((void *)stillRawFile);
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

            stillYFile = UT_DualVinEncodefopen((const char *)Fn, (const char *)mdASCII,1);
            UT_DualVinEncodefwrite((const void *)LumaAddr, 1, Pitch*Height, (void *)stillYFile);
            UT_DualVinEncodefsync((void *)stillYFile);
            UT_DualVinEncodefclose((void *)stillYFile);

            stillUvFile = UT_DualVinEncodefopen((const char *)fn1, (const char *)mdASCII,1);
            UT_DualVinEncodefwrite((const void *)ChromaAddr, 1, Pitch*Height, (void *)stillUvFile);
            UT_DualVinEncodefsync((void *)stillUvFile);
            UT_DualVinEncodefclose((void *)stillUvFile);
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
UINT32 AmpUT_DualVinEnc_PIVPRECB(AMP_STILLENC_PREP_INFO_s *info)
{
    if (info->StageCnt == 3) {
        UINT8 ItunerMode = 0;
        AmpUT_TuneGetItunerMode(&ItunerMode);
        if (ItunerMode == 0/*ITUNER_BASIC_TUNING_MODE*/) {
            /* set still idsp param */
            if (StillIso != 2 && VideoEnc3AEnable == 1){ // comes from AmbaSample_AdjPivControl()
                AMBA_DSP_IMG_MODE_CFG_s ImgMode = {0};
                ADJ_STILL_CONTROL_s AdjPivCtrl = {0};
                float BaseStillBlcShtTime = 60.0;
                UINT16 ShutterIndex = 0;
                UINT32 ChNo = (DualStillVinSelect == VIN_MAIN_ONLY)? 0: 1;
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
                AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, ChNo, IP_MODE_VIDEO, (UINT32)&VideoAeInfo);
                if (LiveViewHdrMode[0]) {
                    // HDR mode use UnitGain
                } else {
                    AmbaImg_Proc_Cmd(MW_IP_GET_PIPE_WB_GAIN, ChNo, IP_MODE_VIDEO, (UINT32)&VideoWbGain);
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

                AmbaImg_Proc_Cmd(MW_IP_ADJ_STILL_CONTROL, ChNo , (UINT32)&AdjPivCtrl , 0);
                AmbaImg_Proc_Cmd(MW_IP_SET_STILL_PIPE_CTRL_PARAMS, ChNo, (UINT32)&ImgMode , 0);

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
UINT32 AmpUT_DualVinEnc_PIV_RawCapCB(AMP_STILLENC_RAWCAP_FLOW_CTRL_s *ctrl)
{
    AMP_STILLENC_RAWCAP_DSP_CTRL_s DspCtrl = {0};
    DspCtrl.VinChan = ctrl->VinChan;
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
int AmpUT_DualVinEnc_PIV(AMP_VIDEOENC_PIV_CTRL_s pivCtrl, UINT32 iso, UINT32 cmpr, UINT32 targetSize, UINT8 encodeLoop)
{
    int Er;
    void *TempPtr;
    void *TmpbufRaw = NULL;
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
    UINT8 DspBackgroundProcMode = AmpResource_GetDspBackgroundProcMode();

    StillIso = iso;

    /* Pre-Phase */
    /* Error handle */
    if (LiveViewProcMode == 0 && iso == 0) {
        AmbaPrint("Can not Support HISO-PIV @ ExpressMode!!");
        return NG;
    }

    /* Phase I */
    /* check still codec Status */
    if (StillCodecInit == 0) { //not init yet
         int Er = OK;

        // Create simple muxer semophore
        if (AmbaKAL_SemCreate(&VideoEncJpegSem, 0) != OK) {
            AmbaPrint("StillEnc UnitTest: Semaphore creation failed");
        }

        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&VideoEncJpegMuxStack, (void **)&TmpbufRaw, 6400+32, 32);
        if (Er != OK) {
            AmbaPrint("Out of memory for Jpeg muxer stack!!");
        }
        // Create simple muxer task
        if (AmbaKAL_TaskCreate(&VideoEncJpegMuxTask, "Video Encoder UnitTest Jpeg Muxing Task", 10, \
             AmpUT_DualVinEnc_JpegMuxTask, 0x0, VideoEncJpegMuxStack, 6400, AMBA_KAL_AUTO_START) != OK) {
             AmbaPrint("jpeg Muxer task creation failed");
        }

        // Create Still encoder object
        {
            AMP_STILLENC_INIT_CFG_s EncInitCfg = {0};

            // Init STILLENC module
            AmpStillEnc_GetInitDefaultCfg(&EncInitCfg);
            if (StillEncWorkBuf == NULL) {
                Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&StillEncWorkBuf, (void **)&TmpbufRaw, EncInitCfg.MemoryPoolSize, 32);
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
        EncCfg.cbEvent = AmpUT_DualVinEncCallback;

        Elayer.Source = (DualStillVinSelect == VIN_MAIN_ONLY)? VideoEncVinA: ((DualStillVinSelect == VIN_PIP_ONLY)? VideoEncVinPip: NULL);

        // Assign bitstream/descriptor buffer
        // A12 express PIV will use H264 BS
        EncCfg.BitsBufCfg.BitsBufAddr = H264BitsBuf;
        EncCfg.BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
        EncCfg.BitsBufCfg.DescBufAddr = H264DescBuf;
        EncCfg.BitsBufCfg.DescBufSize = DESC_SIZE;
        EncCfg.BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        AmbaPrint("Bits 0x%X size %x Desc 0x%X size %d", H264BitsBuf, BITSFIFO_SIZE, H264DescBuf, DESC_SIZE);
        AmpStillEnc_Create(&EncCfg, &StillEncPri);

        // create a virtual fifo
        if (VirtualJpegFifoHdlr == NULL) {
            AMP_FIFO_CFG_s FifoDefCfg = {0};

            AmpFifo_GetDefaultCfg(&FifoDefCfg);
            FifoDefCfg.hCodec = StillEncPri;
            FifoDefCfg.IsVirtual = 1;
            FifoDefCfg.NumEntries = 1024;
            FifoDefCfg.cbEvent = AmpUT_DualVinEnc_FifoCB;
            AmpFifo_Create(&FifoDefCfg, &VirtualJpegFifoHdlr);
        }
    }

    //create pipeline
    if (StillEncPipe == NULL) {
        AMP_ENC_PIPE_CFG_s PipeCfg = {0};

        AmpEnc_GetDefaultCfg(&PipeCfg);
        PipeCfg.encoder[0] = StillEncPri;
        PipeCfg.numEncoder = 1;
        PipeCfg.cbEvent = AmpUT_DualVinEncPipeCallback;
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
    RawPitch = (cmpr)? \
        AMP_COMPRESSED_RAW_WIDTH(PivCaptureWidth): \
        PivCaptureWidth*2;
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
        AmbaPrint("[UT_videoEncPIV]scrn(%d %d %u) thm(%d %d %u)!", \
            ScrnW, ScrnH, ScrnSize, \
            ThmW, ThmH, ThmSize);
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

    Er = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, (void **)&TmpbufRaw, 128*10, 32); //TBD
    if (Er != OK) {
        AmbaPrint("[UT_videoEncPIV]Cache_DDR alloc scriptAddr fail (%u)!", 128*10);
    } else {
        s_scriptAddr = (UINT8*)TempPtr;
        Ori_s_scriptAddr = (UINT8*)TmpbufRaw;
        AmbaPrint("[UT_videoEncPIV]scriptAddr (0x%08X) (%d)!", s_scriptAddr, 128*10);
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
        GenScrpt->QVConfig.DisableLCDQV = 1;
        GenScrpt->QVConfig.DisableHDMIQV = 1;
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

    GenScrpt->MainBuf.ColorFmt = AMP_YUV_420;
    GenScrpt->MainBuf.Width = GenScrpt->MainBuf.Pitch = YuvWidth;
    GenScrpt->MainBuf.Height = YuvHeight;
    GenScrpt->MainBuf.LumaAddr = s_yuvBuffAddr;
    GenScrpt->MainBuf.ChromaAddr = 0; // Behind Luma
    GenScrpt->MainBuf.AOI.X = 0;
    GenScrpt->MainBuf.AOI.Y = 0;
    GenScrpt->MainBuf.AOI.Width = PivMainWidth;
    GenScrpt->MainBuf.AOI.Height = PivMainHeight;

    GenScrpt->ScrnBuf.ColorFmt = AMP_YUV_420;
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

    GenScrpt->ThmBuf.ColorFmt = AMP_YUV_420;
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

    if (targetSize) {
        extern UINT32 AmpUT_JpegBRCPredictCB(UINT16 targetBpp, UINT32 stillProc, UINT8* predictQ, UINT8 *curveNum, UINT32 *curveAddr);
        AmbaPrint("[UT_videoEncPIV]Target Size %u Kbyte", targetSize);
        AmpUT_DualVinEnc_InitMJpegDqt(DualVinVideoPIVQTable[0], -1);
        AmpUT_DualVinEnc_InitMJpegDqt(DualVinVideoPIVQTable[1], -1);
        AmpUT_DualVinEnc_InitMJpegDqt(DualVinVideoPIVQTable[2], -1);
        GenScrpt->BrcCtrl.Tolerance = 10;
        GenScrpt->BrcCtrl.MaxEncLoop = encodeLoop;
        GenScrpt->BrcCtrl.JpgBrcCB = AmpUT_JpegBRCPredictCB;
        GenScrpt->BrcCtrl.TargetBitRate = \
           (((targetSize<<13)/PivMainWidth)<<12)/PivMainHeight;
        GenScrpt->BrcCtrl.MainQTAddr = DualVinVideoPIVQTable[0];
        GenScrpt->BrcCtrl.ThmQTAddr = DualVinVideoPIVQTable[1];
        GenScrpt->BrcCtrl.ScrnQTAddr = DualVinVideoPIVQTable[2];
    } else {
        AmpUT_DualVinEnc_InitMJpegDqt(DualVinVideoPIVQTable[0], VideoPIVMainQuality);
        AmpUT_DualVinEnc_InitMJpegDqt(DualVinVideoPIVQTable[1], VideoPIVThmbQuality);
        AmpUT_DualVinEnc_InitMJpegDqt(DualVinVideoPIVQTable[2], VideoPIVScrnQuality);
        GenScrpt->BrcCtrl.Tolerance = 0;
        GenScrpt->BrcCtrl.MaxEncLoop = 0;
        GenScrpt->BrcCtrl.JpgBrcCB = NULL;
        GenScrpt->BrcCtrl.TargetBitRate = 0;
        GenScrpt->BrcCtrl.MainQTAddr = DualVinVideoPIVQTable[0];
        GenScrpt->BrcCtrl.ThmQTAddr = DualVinVideoPIVQTable[1];
        GenScrpt->BrcCtrl.ScrnQTAddr = DualVinVideoPIVQTable[2];
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
    RawCapScrpt->CapCB.RawCapCB = AmpUT_DualVinEnc_PIV_RawCapCB;
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
    //StillBGProcessing = 1;

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

int AmpUT_DualVinEnc_CheckCapability(void)
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
            enable = capability->Rotata90HFlip; }
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

_done:
    return enable;
}


/**
 * Pre-processing before entering standby
 */
static void AmpUT_DualVinEnc_RegisterResumeInt(void)
{
    // At this moment, all INTs are disabled by AmbaPLL, we have to enable some in order to wake up.
    AmbaINT_IntEnable(AMBA_VIC_INT_ID9_UART0); // Use UART INT to wake

}

int AmpUT_DualVinEncTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_DualVinEncTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        int SensorId = 0, LcdId = 0;
        int SensorPipId = 0;

        if (Status != STATUS_UNKNOWN) {
            return -1;
        }

        if (argc < 3) {
            UINT8 i = 0;
            AmbaPrint("Usage: t dualvinenc init [sensorID][SensorPipId][LcdID]");
            AmbaPrint("               sensorID:");
            for (i = 0; i<INPUT_DEVICE_NUM; i++) {
                AmbaPrint("                        %2d -- %s", i, MWUT_GetInputDeviceName(i));
            }
            AmbaPrint("               LcdID: 0 -- WDF9648W");
            return -1;
        } else if (argc < 4 && DualVideoVinSelect == VIN_MAIN_PIP) {
            UINT8 i = 0;
            AmbaPrint("Dual Vin");
            AmbaPrint("Usage: t dualvinenc init [sensorID][SensorPipId][LcdID]");
            AmbaPrint("               sensorID:");
            for (i = 0; i<INPUT_DEVICE_NUM; i++) {
                AmbaPrint("                        %2d -- %s", i, MWUT_GetInputDeviceName(i));
            }
            AmbaPrint("               SensorPipId: must select B5_XXXX sensor id");
            AmbaPrint("               LcdID: 0 -- WDF9648W");
        }
        SensorId = atoi(argv[2]);
        SensorPipId = atoi(argv[3]);
        LcdId = atoi(argv[4]);
        AmpUT_DualVinEnc_Init(SensorId, SensorPipId, LcdId);

        Status = STATUS_INIT;
    } else if (strcmp(argv[1],"tv") == 0) {
        if (Status != STATUS_INIT) {
            AmbaPrint("Please set it before liveviewstart");
        }

        if (argc < 3) {
            AmbaPrint("Usage: t dualvinenc tv [enable]");
        }

        TvLiveview = atoi(argv[2]);

        AmbaPrint("TV: %s",TvLiveview? "Enable": "Disable");
    } else if (strcmp(argv[1],"lvproc") == 0 || strcmp(argv[1],"liveviewproc") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t dualvinenc liveviewproc [proc][Algo/OS]");
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
        UINT32 MainSensorMode, PipSensorMode;

        if (Status != STATUS_INIT) {
            return -1;
        }

        if (argc < 3) {
            AmbaPrint("Usage: t dualvinenc liveviewstart [modeId] [modePipId]");
            return -1;
        }
        MainSensorMode = atoi(argv[2]);
        PipSensorMode = atoi(argv[3]);

        AmpUT_DualVinEnc_LiveviewStart(MainSensorMode, PipSensorMode);
        Status = STATUS_LIVEVIEW;
    } else if ((strcmp(argv[1],"encstart") == 0) || (strcmp(argv[1],"enst") == 0)) {
        UINT32 Duration = atoi(argv[2]);

        if (Status != STATUS_LIVEVIEW) {
            return -1;
        }

        if (AmpUT_DualVinEnc_CheckCapability() == 0) {
            AmbaPrint("%s doesn't support this Encode Mode", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
            return -1;
        }

        AmbaPrint("Encode for %d milliseconds", Duration);
        AmpUT_DualVinEnc_EncodeStart();
        Status = STATUS_ENCODE;

        if (Duration != 0) {
            while (Duration) {
                AmbaKAL_TaskSleep(1);
                Duration--;
                if (Duration == 0) {
                    AmpUT_DualVinEnc_EncStop();
                    EncDateTimeStampPri = EncDateTimeStampSec = 0;
                    Status = STATUS_LIVEVIEW;
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

        AmpUT_DualVinEnc_EncStop();
        EncDateTimeStampPri = EncDateTimeStampSec = 0;
        Status = STATUS_LIVEVIEW;
    } else if ((strcmp(argv[1],"pause") == 0)) {
        AmpUT_DualVinEnc_EncPause();
    } else if ((strcmp(argv[1],"resume") == 0)) {
        AmpUT_DualVinEnc_EncResume();
    } else if ((strcmp(argv[1],"liveviewstop") == 0) || (strcmp(argv[1],"lvsp") == 0)) {
        if (Status != STATUS_LIVEVIEW) {
            return -1;
        }
        /*  Make sure output file is closed */
        if (outputPriFile || outputSecFile) {
            AmbaPrint("MUXER not IDLE !!!!");
            return -1;
        }

        AmpUT_DualVinEnc_LiveviewStop();
        Status = STATUS_INIT;
    } else if ((strcmp(argv[1],"standby") == 0)) {
        AmbaPrint("Enter Standby mode");
        AmbaIPC_LinkCtrlSuspendLinux(AMBA_LINK_SLEEP_TO_RAM);
        if (AmbaIPC_LinkCtrlWaitSuspendLinux(5000) != OK) {
            AmbaPrint("SuspendLinux (%d) failed!\n", AMBA_LINK_SLEEP_TO_RAM);
        }
        AmbaPrintk_Flush();
        AmbaLCD_SetBacklight(0, 0);
        AmbaPLL_SetStandbyMode(0, AmpUT_DualVinEnc_RegisterResumeInt, NULL);
        AmbaIPC_LinkCtrlResumeLinux(AMBA_LINK_SLEEP_TO_RAM);
        if (AmbaIPC_LinkCtrlWaitResumeLinux(5000) != OK) {
           AmbaPrint("ResumeLinux (%d) failed.\n", AMBA_LINK_SLEEP_TO_RAM);
        }
        AmbaLCD_SetBacklight(0, 1);
    } else if ((strcmp(argv[1],"dual") == 0)) {
        UINT32 DualEnable = atoi(argv[2]);

        if (Status != STATUS_INIT) {
            AmbaPrint("Please set dualstream before liveview start");
            return -1;
        }

        AmbaPrint("DualStream: %s", DualEnable? "ENABLE": "DISABLE");
        EncDualStream = DualEnable;
    } else if ((strcmp(argv[1],"dualhd") == 0)) {
        UINT32 DualHDEnable = atoi(argv[2]);

        if (Status != STATUS_INIT) {
            AmbaPrint("Please set dualstream before liveview start");
            return -1;
        }

        AmbaPrint("DualHDStream: %s", DualHDEnable? "ENABLE": "DISABLE");
        AmbaPrint("Note: Only support dual 1080P60, dual 1080P30 and dual 720P60");
        AmbaPrint("Note: DualHD does NOT support dual vout!");
        EncDualHDStream = DualHDEnable;
    }  else if ((strcmp(argv[1],"brc") == 0)) {
        static AMP_VIDEOENC_RUNTIME_QUALITY_CFG_s DualVinQualityCfg[4] = {0};
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
                AmbaPrint("               0 - Main Primary");
                AmbaPrint("               1 - Main Secondary");
                AmbaPrint("               2 - PIP Primary");
                AmbaPrint("               3 - PIP Secondary");
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
                    RealTimeCtrl = &DualVinQualityCfg[0];
                } else if (Channel == 1) {
                    EncHdlr = VideoEncSec;
                    RealTimeCtrl = &DualVinQualityCfg[1];
                } else if (Channel == 2) {
                    EncHdlr = VideoPipEncPri;
                    RealTimeCtrl = &DualVinQualityCfg[2];
                } else if (Channel == 3) {
                    EncHdlr = VideoPipEncSec;
                    RealTimeCtrl = &DualVinQualityCfg[3];
                } else {
                    AmbaPrintColor(RED, "Incorrect channel %u", Channel);
                }

                switch(Mode){
                case 0:
                {
                    if (Status != STATUS_LIVEVIEW) {
                        AmbaPrint("Please set bitrate after liveview start %d", Status);
                        return -1;
                    }
                    {
                        UINT8 BrcMode = atoi(argv[5]);
                        AMP_VIDEOENC_BITSTREAM_CFG_s CurrentCfg = {0};
                        VideoEncMgt[EncModeIdx].BrcMode = BrcMode;
                        VideoEncMgt[EncModeIdx].AverageBitRate = atoi(argv[6]);
                        VideoEncMgt[EncModeIdx].MaxBitRate = atoi(argv[7]);
                        VideoEncMgt[EncModeIdx].MinBitRate = atoi(argv[8]);
                        AmpVideoEnc_GetBitstreamConfig(EncHdlr, &CurrentCfg);
                        CurrentCfg.Spec.H264Cfg.BitRateControl.BrcMode = VideoEncMgt[EncModeIdx].BrcMode;
                        CurrentCfg.Spec.H264Cfg.BitRateControl.AverageBitrate = (UINT32)(VideoEncMgt[EncModeIdx].AverageBitRate * 1E6);
                        CurrentCfg.Spec.H264Cfg.QualityControl.IBeatMode = atoi(argv[9]);
                        if (CurrentCfg.Spec.H264Cfg.BitRateControl.BrcMode == VIDEOENC_SMART_VBR) {
                            CurrentCfg.Spec.H264Cfg.BitRateControl.MaxBitrate = (UINT32)(VideoEncMgt[EncModeIdx].MaxBitRate * 1E6);
                            CurrentCfg.Spec.H264Cfg.BitRateControl.MinBitrate = (UINT32)(VideoEncMgt[EncModeIdx].MinBitRate * 1E6);
                        }
                        AmpVideoEnc_SetBitstreamConfig(EncHdlr, &CurrentCfg);
                        AmbaPrint("BRC mode %s Avg %dMbps Max %dMbps Min %dMbps iBeat %d",VideoEncMgt[EncModeIdx].BrcMode==1?"CBR":"SmartVBR", \
                            VideoEncMgt[EncModeIdx].AverageBitRate,VideoEncMgt[EncModeIdx].MaxBitRate,VideoEncMgt[EncModeIdx].MinBitRate, \
                            CurrentCfg.Spec.H264Cfg.QualityControl.IBeatMode);
                    }
                }
                    break;
                case 1:
                    if (argc == 6) {
                        RealTimeCtrl->Cmd |= RC_BITRATE;
                        RealTimeCtrl->BitRate = (UINT32) (atoi(argv[5])*1E3);
                        AmbaPrint("[Amp_UT][brc] Cmd 0x%x (%d)", RealTimeCtrl->Cmd, RealTimeCtrl->BitRate);
                    } else {
                        return -1;
                    }
                    break;
                case 2:
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
                case 3:
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
                case 4:
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
                case 6:
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
                case 7:
                    if (argc == 6) {
                        RealTimeCtrl->Cmd |= RC_ZMV;
                        RealTimeCtrl->ZmvThres = atoi(argv[5]);
                        AmbaPrint("[Amp_UT][brc] Cmd 0x%x (%d)", RealTimeCtrl->Cmd, RealTimeCtrl->ZmvThres);
                    } else {
                        return -1;
                    }
                    break;
                case 8:
                    if (argc == 5){
                        AmbaPrint("[Amp_UT] Force Idr can only invoke individually");
                        RealTimeCtrl->Cmd = RC_FORCE_IDR;
                        AmbaPrint("[Amp_UT][brc] Cmd 0x%x", RealTimeCtrl->Cmd);
                    } else {
                        return -1;
                    }
                    break;
                case 9:
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
                AmbaPrint("                         0: Main Primary");
                AmbaPrint("                         1: Main Secondary");
                AmbaPrint("                         2: PIP Primary");
                AmbaPrint("                         3: PIP Secondary");
            } else {
                if (Status != STATUS_ENCODE) {
                    AmbaPrint("Wrong Status %d", Status);
                    return 0;
                }
                if (atoi(argv[3]) == 0) {
                    AmpVideoEnc_SetRuntimeQuality(VideoEncPri, &DualVinQualityCfg[0]);
                } else if (atoi(argv[3]) == 1) {
                    AmpVideoEnc_SetRuntimeQuality(VideoEncSec, &DualVinQualityCfg[1]);
                } else if (atoi(argv[3]) == 2) {
                    AmpVideoEnc_SetRuntimeQuality(VideoPipEncPri, &DualVinQualityCfg[2]);
                } else if (atoi(argv[3]) == 3) {
                    AmpVideoEnc_SetRuntimeQuality(VideoPipEncSec, &DualVinQualityCfg[3]);
                }
            }
        }
    } else if ((strcmp(argv[1],"brcshow") == 0)){
        AMP_VIDEOENC_ENCODING_INFO_s EncInfo = {0};
        if (Status != STATUS_ENCODE) {
            AmbaPrint("Wrong Status %d", Status);
            return 0;
        }
        if (VideoEncPri) {
            AmpVideoEnc_GetEncodingInfo(VideoEncPri, &EncInfo);
            AmbaPrint("[AmpUT][Dual][Main][Pri]: Total Frames %d AverBitrate %dkbps TotalBytes %lldbytes", EncInfo.TotalFrames,\
                EncInfo.AverageBitrate, EncInfo.TotalBytes);
        }
        if (VideoEncSec) {
            AmpVideoEnc_GetEncodingInfo(VideoEncSec, &EncInfo);
            AmbaPrint("[AmpUT][Dual][Main][Sec]: Total Frames %d AverBitrate %dkbps TotalBytes %lldbytes", EncInfo.TotalFrames,\
                EncInfo.AverageBitrate, EncInfo.TotalBytes);
        }
        if (VideoPipEncPri) {
            AmpVideoEnc_GetEncodingInfo(VideoPipEncPri, &EncInfo);
            AmbaPrint("[AmpUT][Dual][Pip][Pri]: Total Frames %d AverBitrate %dkbps TotalBytes %lldbytes", EncInfo.TotalFrames,\
                EncInfo.AverageBitrate, EncInfo.TotalBytes);
        }
        if (VideoPipEncSec) {
            AmpVideoEnc_GetEncodingInfo(VideoPipEncSec, &EncInfo);
            AmbaPrint("[AmpUT][Dual][Pip][Sec]: Total Frames %d AverBitrate %dkbps TotalBytes %lldbytes", EncInfo.TotalFrames,\
                EncInfo.AverageBitrate, EncInfo.TotalBytes);
        }
        if ((strcmp(argv[2],"reset") == 0)) {
            if (VideoEncPri) AmpVideoEnc_ResetEncodingInfo(VideoEncPri);
            if (VideoEncSec) AmpVideoEnc_ResetEncodingInfo(VideoEncSec);
            if (VideoPipEncPri) AmpVideoEnc_ResetEncodingInfo(VideoPipEncPri);
            if (VideoPipEncSec) AmpVideoEnc_ResetEncodingInfo(VideoPipEncSec);
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
        Exp.Info.SubChannelNum = 0;
        Exp.Info.AGC[0] = 1.0*atoi(argv[3]);
        Exp.Info.Shutter[0] = 1.0/atoi(argv[4]);
        Exp.Info.DGain.GainR = atoi(argv[5]);
        Exp.Info.DGain.GainG = atoi(argv[6]);
        Exp.Info.DGain.GainB = atoi(argv[7]);
        Exp.Info.DGain.AeGain = atoi(argv[8]);
        Exp.Info.DGain.GlobalDGain = atoi(argv[9]);

        AmbaPrint("Type %d AGC %f Shutter %f Gain R:%d G:%d B:%d AE:%d GLOBAL:%d",Exp.Type, Exp.Info.AGC,Exp.Info.Shutter, \
            Exp.Info.DGain.GainR ,Exp.Info.DGain.GainG, Exp.Info.DGain.GainB, Exp.Info.DGain.AeGain, Exp.Info.DGain.GlobalDGain);

        AmbaImgSchdlr_SetExposure((UINT32)0, &Exp);

    } else if ((strcmp(argv[1],"3a") == 0)) {
        UINT8 Is3aEnable = atoi(argv[2]);
        UINT32 ChannelIndex = atoi(argv[3]);
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
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChannelIndex, (UINT32)&AaaOpInfo, 0);

        for (i=0; i <ChannelCount; i++) {
            AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, i, (UINT32)&AaaOpInfo, 0);
            AmbaPrint("ChNo[%u] 3A %s: ae:%u af:%u awb:%u adj:%u", i, VideoEnc3AEnable? "Enable": "Disable", \
                AaaOpInfo.AeOp, AaaOpInfo.AfOp, AaaOpInfo.AwbOp, AaaOpInfo.AdjOp);
        }
    } else if ((strcmp(argv[1],"ta") == 0)) {
        UINT8 Enable = atoi(argv[2]);
        UINT8 VinId = atoi(argv[3]);
        VideoEncTAEnable[VinId] = Enable;
        AmbaPrint("TA[%d] : %s, be sure to use HybridAlgoMode", VinId, VideoEncTAEnable?"Enable":"Disable");
    } else if ((strcmp(argv[1],"rotate") == 0) || (strcmp(argv[1],"rot") == 0)) {

        if (argc != 3) {
            AmbaPrint("Usage: t videnenc rotation [Rotation] ");
            AmbaPrint("       In A12 ExpressMode, PrevA will be rotated after issue encode rotate,");
            AmbaPrint("       and APP shall re-issue liveview stop/start to prevent PrevA buffer mismatch");
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

            if (AmpUT_DualVinEnc_CheckCapability()) {
                AmpUT_DualVinEnc_LiveviewStop();
                AmpUT_DualVinEnc_LiveviewStart(EncModeIdx, EncPipModeIdx);
            } else {
                AmbaPrint("%s doesn't support Rotate", MWUT_GetInputVideoModeName(EncModeIdx, EncodeSystem));
                EncRotation = PreviousRotate;
            }
        } else {
            EncRotation = (AMP_ROTATION_e)atoi(argv[2]);
        }

        AmbaPrint("Rotation %d ",EncRotation);
    } else if ((strcmp(argv[1],"spec") == 0)) {
        UINT32 PriSpec = atoi(argv[2]);
        UINT32 SecPec = atoi(argv[3]);

        if (Status == STATUS_ENCODE) {
            AmbaPrint("Wrong Status");
            return 0;
        }
        if (argc < 4) {
            AmbaPrint("Usage: t videoenc spec [PriSpec] [SecSpec]");
            AmbaPrint("                  PriSpec: Primary stream spec. 1 - H.264 (default), 0 - MotionJPEG");
            AmbaPrint("                  SecSpec: Secondary stream spec. 1 - H.264 (default), 0 - MotionJPEG");
            AmbaPrint("                  A12 ExpressMode can not Support MJPEG in PriStream ");
            return 0;
        }
        AmbaPrint("Primary Stream: %s",PriSpec? "H.264": "MJPEG");
        AmbaPrint("Secondary Stream (if enabled): %s",SecPec? "H.264": "MJPEG");
            if (LiveViewProcMode == 0 && PriSpec == 0) {
                AmbaPrint("A12 can not support MJpeg PriStream in ExpressMode, force as H264");
                EncPriSpecH264 = PriSpec = 1;
            } else {
                EncPriSpecH264 = PriSpec;
            }

        EncSecSpecH264 = SecPec;
        if (Status == STATUS_LIVEVIEW) {
            AmpUT_DualVinEnc_LiveviewStop();
            AmpUT_DualVinEnc_LiveviewStart(EncModeIdx, EncPipModeIdx);
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
            UINT32 SensorMode;

            if (Status != STATUS_LIVEVIEW)
                return 0;

            SensorMode = atoi(argv[2]);
            if (VideoEncMgt[SensorMode].ForbidMode) {
                AmbaPrint("Not Suppot this Mode (%d)", SensorMode);
                return -1;
            }
            if (VideoEncMgt[SensorMode].PALModeOnly && EncodeSystem == 0 && \
                (EncDualHDStream || EncDualStream))  {
                AmbaPrintColor(RED, "Only Support PAL mode !!!!!");
                return -1;
            }
            AmpUT_DualVinEnc_ChangeResolution(SensorMode);
        }
    } else if (strcmp(argv[1],"piv") == 0) {
        if (Status != STATUS_ENCODE) {
            return 0;
        }

        if (argc < 8) {
            AmbaPrint("Usage: t videoenc piv [encW][encH][iso][cmpr][targetSize][encodeLoop]");
            AmbaPrint("             encW: encode width");
            AmbaPrint("             encH: encode height");
            AmbaPrint("             iso: algo mode, 0:HISO(Not support) 1:LISO 2:MFHISO(hack as FastMode)");
            AmbaPrint("                 frameRate>30fps can not support ISO-PIV(Hiso/Liso)");
            AmbaPrint("                 frameRate>60fps can not support PIV");
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
            UINT8 ModeIdx = (DualStillVinSelect == VIN_MAIN_ONLY)? EncModeIdx: EncPipModeIdx;
            INPUT_ENC_MGT_s *EncMgt = (DualStillVinSelect == VIN_MAIN_ONLY)? VideoEncMgt: VideoPipEncMgt;

            if (EncodeSystem == 0) {
                if (EncMgt[ModeIdx].TimeScale/EncMgt[ModeIdx].TickPerPicture > 60) {
                    AmbaPrint("HFR can not support PIV");
                    return 0;
                }
            } else {
                if (EncMgt[ModeIdx].TimeScalePAL/EncMgt[ModeIdx].TickPerPicturePAL > 50) {
                    AmbaPrint("HFR can not support PIV");
                    return 0;
                }
            }

            PivCtrl.SensorMode.Data = (EncodeSystem == 0)? EncMgt[ModeIdx].InputMode: EncMgt[ModeIdx].InputPALMode;;
            PivCtrl.CaptureWidth = EncMgt[ModeIdx].CaptureWidth;
            PivCtrl.CaptureHeight = EncMgt[ModeIdx].CaptureHeight;
            PivCtrl.MainWidth = EncW;
            PivCtrl.MainHeight = EncH;
            PivCtrl.AspectRatio = EncMgt[ModeIdx].AspectRatio;
            AmpUT_DualVinEnc_PIV(PivCtrl, Iso, Cmpr, Tsize, Loop);
        }
    } else if (strcmp(argv[1],"thm") == 0) {
        if (Status == STATUS_ENCODE) {
            return 0;
        }

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc thm [enable][VinID]");
            AmbaPrint("             enable: enable video thumbnail");
            return 0;
        } else {
            UINT8 ThmEnable = atoi(argv[2]);
            UINT8 VinID = atoi(argv[3]);
            DualEncThumbnail[VinID] = ThmEnable;
            AmbaPrint("VideoThm[%d] %s", VinID, DualEncThumbnail? "Enable": "Disable");
        }
    } else if (strcmp(argv[1], "debug") == 0) {
        extern int AmbaDSP_CmdSetDebugLevel(UINT8 Module, UINT8 Level, UINT8 Mask);
        UINT8 Module = atoi(argv[2]);
        UINT8 Level = atoi(argv[3]);
        UINT8 Mask = atoi(argv[4]);
        AmbaDSP_CmdSetDebugLevel(Module, Level, Mask);
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
            return -1;
        } else {
            UINT8 StreamID = atoi(argv[2]);
            UINT8 BlendEnable = atoi(argv[3]);
            AMP_VIDEOENC_BLEND_INFO_s BlendInfo = {0};

            AmbaPrint("Encode Blend [%d] %s", StreamID, BlendEnable? "Enable": "Disable");

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

            if (DualVideoVinSelect == VIN_MAIN_PIP) {
                if (StreamID == AMP_VIDEOENC_STREAM_PRIMARY) {
                    EncDateTimeStampPri = 1;
                    if (VideoEncPri) {
                        AmpVideoEnc_SetEncodeBlend(VideoEncPri, &BlendInfo);
                    }
                } else if (StreamID == AMP_VIDEOENC_STREAM_TERTIARY){
                    EncDateTimeStampSec = 1;
                    if (VideoEncSec) {
                        AmpVideoEnc_SetEncodeBlend(VideoEncSec, &BlendInfo);
                    }
                } else if (StreamID == AMP_VIDEOENC_STREAM_SECONDARY){
                    EncPipDateTimeStampPri = 1;
                    if (VideoPipEncPri) {
                        AmpVideoEnc_SetEncodeBlend(VideoPipEncPri, &BlendInfo);
                    }
                } else if (StreamID == AMP_VIDEOENC_STREAM_QUATERNARY){
                    EncPipDateTimeStampSec = 1;
                    if (VideoPipEncSec) {
                        AmpVideoEnc_SetEncodeBlend(VideoPipEncSec, &BlendInfo);
                    }
                }
            } else {
                if (StreamID == 0) {
                    EncDateTimeStampPri = 1;
                    AmpVideoEnc_SetEncodeBlend(VideoEncPri, &BlendInfo);
                } else {
                    EncDateTimeStampSec = 1;
                    AmpVideoEnc_SetEncodeBlend(VideoEncSec, &BlendInfo);
                }
            }

        }
    } else if ((strcmp(argv[1],"interlace") == 0)) {
        UINT32 PriInterlaced = atoi(argv[2]);
        UINT32 SecInterlaced = atoi(argv[3]);

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
        AmbaPrint("Primary Stream: %s", PriInterlaced? "Interlace": "Progressive");
        AmbaPrint("Secondary Stream (if enabled): %s", SecInterlaced? "Interlace": "Progressive");
        EncPriInteralce = PriInterlaced;
        EncSecInteralce = SecInterlaced;
        if (Status == STATUS_LIVEVIEW) {
            AmpUT_DualVinEnc_ChangeResolution(EncModeIdx);
        }
    } else if (strcmp(argv[1],"calib") == 0) {
        UINT32 CalibId = atoi(argv[2]);
        UINT32 Enable = atoi(argv[3]);

        if (argc < 3) {
            AmbaPrint("Usage: t videoenc calib [calibID] [Enable]");
            AmbaPrint("                  CalibID : calibratrion ID");
            AmbaPrint("                    SBP  : 1");
            AmbaPrint("                    Warp : 2");
            AmbaPrint("                    CA   : 3");
            AmbaPrint("                    VIG  : 4");
            return 0;
        }

        if (CalibId <=4 && CalibId) {
            if (Enable) {
                CalibEnable |= (0x1<<(CalibId-1));
            } else {
                CalibEnable &= ~(0x1<<(CalibId-1));
            }
            AmbaPrint("CalibEnable 0x%X", CalibEnable);
        }
    } else if (strcmp(argv[1],"secwin") == 0) {
        UINT16 SecWinWidth = atoi(argv[2]);
        UINT16 SecWinHeight = atoi(argv[3]);

        AmbaPrint("SecStrmWin %dX%d", SecWinWidth, SecWinHeight);
        SecStreamCustomWidth = SecWinWidth;
        SecStreamCustomHeight = SecWinHeight;
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
        UINT8 MainReportRate = atoi(argv[2]);

        AmbaPrint("Report rate is %d", MainReportRate);
        MainviewReportRate = MainReportRate;
    } else if (strcmp(argv[1],"mjpegquality") == 0 || strcmp(argv[1],"mjpegq") == 0) {
        MjpegQuality = atoi(argv[2]);
    } else if (strcmp(argv[1],"encmonitor") == 0 || strcmp(argv[1],"emon") == 0) {
        EncMonitorEnable = atoi(argv[2]);
        AmbaPrint("Encode Monitor %s !", EncMonitorEnable?"Enable":"Disable");
    } else if (strcmp(argv[1],"encmonitoraqp") == 0 || strcmp(argv[1],"emonaqp") == 0) {
        EncMonitorAQPEnable = atoi(argv[2]);
        AmbaPrint("Encode Monitor AQP %s !", EncMonitorAQPEnable?"Enable":"Disable");
    } else if (strcmp(argv[1],"VideoOB") == 0) {
        VideoOBModeEnable = atoi(argv[2]);
        AmbaPrint("VideoOB mode %s !", VideoOBModeEnable?"Enable":"Disable");
    } else if (strcmp(argv[1],"suspend") == 0) {
        DspSuspendEnable = atoi(argv[2]);
        AmbaPrint("%s Dsp suspend in LiveveiwStop !", DspSuspendEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"wirelessmode") == 0) {
        WirelessModeEnable = atoi(argv[2]);
        AmbaPrint("%s Enter Wireless mode in LiveveiwStop !", WirelessModeEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"dualvin") == 0) {
        DualVideoVinSelect = atoi(argv[2]);
        AmbaPrint("DualVin %s", (DualVideoVinSelect == VIN_MAIN_ONLY)? "Main": (DualVideoVinSelect == VIN_PIP_ONLY)? "Pip": "Main+Pip");
    } else if (strcmp(argv[1], "stillvin") == 0) {
        UINT8 VinSelect = atoi(argv[2]);
        if (VinSelect == VIN_MAIN_ONLY || VinSelect == VIN_PIP_ONLY) {
            DualStillVinSelect = VinSelect;
            AmbaPrint("Still Vin %s", (VinSelect == VIN_MAIN_ONLY)? "Main": "Pip");
        } else {
            AmbaPrint("Incorrect still vin %u", VinSelect);
        }
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
        AmbaPrint("       piv: do videoEnc PIV(only support single VOut)");
        AmbaPrint("       thm: do videoEnc with thumbnail");
        AmbaPrint("       spec: Select bitstream specification");
        AmbaPrint("       rotate: Set encode rotation");
        AmbaPrint("       dualhd: DualHD mode");
        AmbaPrint("       blend: Bitstream blending");
        AmbaPrint("       chgfrate: Change framerate during encoding");
        AmbaPrint("       tcap: Capture+encode one frame during timelapse recording");
        AmbaPrint("       interlace: Encable interlace encode");
        AmbaPrint("       dualvin: Select Video DualVin mode 0:Main only, 1: Pip only 2:Dual");
        AmbaPrint("       stillvin: Select Piv Vin mode 0:Main, 1: Pip");
    }
    return 0;
}

int AmpUT_DualVinEncTestAdd(void)
{
    AmbaPrint("Adding AmpUT_DualVinEnc");
    // hook command
    AmbaTest_RegisterCommand("dualvinenc", AmpUT_DualVinEncTest);
    AmbaTest_RegisterCommand("duve", AmpUT_DualVinEncTest);

    return AMP_OK;
}
