/**
  * @file src/app/sample/unittest/AmpUT_AVEnc.c
  *
  * transcoder encode unit test
  *
  * History:
  *    2014/10/08 - [Wisdom Hung] created file
  *
  * Copyright (C) 2014, Ambarella, Inc.
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
#include <recorder/AudioEnc.h>
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

#include <AmbaCalibInfo.h>
#include "AmbaLCD.h"
#include <AmbaImg_Proc.h>
#include <AmbaImg_Impl_Cmd.h>
#include <AmbaImg_VIn_Handler.h>
#include <AmbaImg_VDsp_Handler.h>
#include "AmpUT_Display.h"
#include <AmbaImg_AaaDef.h>

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
static UINT8 AvEnc_EisAlgoEnable = 0;
#endif

void *UT_AVEncodefopen(const char *pFileName, const char *pMode, BOOL8 AsyncMode)
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

int UT_AVEncodefclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_AVEncodefwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_AVEncodefsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

UINT32 UT_AVFrameRateIntConvert(UINT32 OriFrameRate)
{
    UINT32 FrameRateInt = OriFrameRate;

    if (OriFrameRate==29 || OriFrameRate==59 || OriFrameRate==119 || OriFrameRate==239) {
        FrameRateInt++;
    }

    return FrameRateInt;
}

#define AV_Audio_Calibration    8

static char DefaultSlot[] = "C";

static AMP_ENC_PIPE_HDLR_s *AvEncPipe = NULL;  // Encode pipeline  instance

// Audio side

// global var for AudioEnc
static AMP_AVENC_HDLR_s *AvAudioEncPriHdlr = NULL;
static UINT8 *AvAudioMainWorkBuf = NULL;
#define STATUS_AV_NONE     1
#define STATUS_AV_IDLE     2
#define STATUS_AV_LIVEVIEW 3
#define STATUS_AV_ENCODE   4
#define STATUS_AV_PAUSE    5

static UINT8 AvStatus = STATUS_AV_NONE;

static UINT8 *AvAudworkNCBuf = 0;
static UINT8 *OriAvAudworkNCBuf = 0;
static UINT8 *AvAudworkCacheBuf = 0;
static UINT8 *OriAvAudworkCacheBuf = 0;

static UINT8 AvAudioEncodeType = 0;
// static for audio stream encode
static AMP_AUDIOENC_INSTANCE_s AvAudioEncInstance = {0};

#define AUDENC_MULTI_AIN    0x1
#define AUDENC_MULTI_STREAM 0x2

  /* muxer */
#define AUDENC_BISFIFO_SIZE AUDIO_ENC_MAX_NEEDED_SIZE*360 // assume 10sec AAC frame
static UINT8 *AvAudPriBitsBuf = NULL;
static UINT8 *OriAvAudPriBitsBuf = NULL;
#define AUDENC_DESC_SIZE 1024*32 //every Desc is 32Byte, assume 10sec data
static UINT8 *AvAudPriDescBuf = NULL;
static UINT8 *OriAvAudPriDescBuf = NULL;

static AMP_FIFO_HDLR_s *AvAudioEncPriVirtualFifoHdlr = NULL;
static AMBA_KAL_TASK_t AvAudioEncPriMuxTask = {0};
static AMBA_KAL_SEM_t AvEncAudioPriSem = {0};

static AMP_CFS_FILE_s *AvAOutputPriFile = NULL;
static AMP_CFS_FILE_s *AvAoutputPriIdxFile = NULL;
static AMP_CFS_FILE_s *AvAOutputPriHeaderFile = NULL;
static AMP_CFS_FILE_s *AvVoutputPriFile = NULL;        // Output file pointer for primary stream
static AMP_CFS_FILE_s *AvVoutputPriIdxFile = NULL;     // Output file pointer for primary stream index (frame offset/size...etc)
static AMP_CFS_FILE_s *AvVUdtaPriFile = NULL;          // Output file pointer for primary stream UserData
static AMP_CFS_FILE_s *AvVoutputSecFile = NULL;        // Output file pointer for Secondary stream
static AMP_CFS_FILE_s *AvVoutputSecIdxFile = NULL;     // Output file pointer for Secondary stream index (frame offset/size...etc)
static AMP_CFS_FILE_s *AvVUdtaSecFile = NULL;          // Output file pointer for Secondary stream UserData

static char *AmpUT_AvAudioEncPriStack;
#define AV_ENC_AUD_MUX_TASK_STACK_SIZE (8192)

static int AV_AFno = 0;

static UINT32 AvAencPriTotalFrames = 0;    // total frames primary stream muxer received

// Video side
// Global var for VideoEnc codec
static AMP_VIN_HDLR_s *AvVideoEncVinA = NULL;        // Vin instance
static AMP_AVENC_HDLR_s *AVEncPri = NULL;            // Primary VideoEnc codec instance
static AMP_AVENC_HDLR_s *AVEncSec = NULL;            // Secondary VideoEnc codec instance
static AMBA_IMG_SCHDLR_HDLR_s *AvImgSchdlr = NULL;   // Image scheduler instance
static UINT8 *AvVinWorkBuffer = NULL;                // Vin module working buffer
static UINT8 *AvVEncWorkBuffer = NULL;               // VideoEnc working buffer
static UINT8 *AvImgSchdlrWorkBuffer = NULL;          // Img scheduler working buffer
static AMBA_DSP_CHANNEL_ID_u AvVinChannel;           // Vin channel

#ifdef CONFIG_LCD_WDF9648W
extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;
#endif

static UINT8 AvTvLiveview = 1;                          // Enable TV liveview or not
static UINT8 AvLCDLiveview = 1;                         // Enable LCD liveview or not
static UINT8 AvEnc3AEnable = 1;                         // Enable AE/AWB/ADJ algo or not
static UINT8 AvEncTAEnable = 0;                         // Enable TA algo or not in HybridAlgoMode
static UINT8 AvEncPriSpecH264 = 1;                      // Primary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 AvEncSecSpecH264 = 1;                      // Secondary codec instance output spec. 1 = H264, 0 = MJPEG
static UINT8 AvEncPriInteralce = 0;                     // Primary codec instance output type. 0 = Progressive, 1 = Interlave
static UINT8 LiveViewProcMode = 0;                  // LiveView Process Mode, 0: Express 1:Hybrid
static UINT8 LiveViewAlgoMode = 0;                  // LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
static UINT8 LiveViewOSMode = 1;                    // LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 LiveViewHdrMode = 0;                   // LiveView HDR Mode in ExpressMode, 0: Disable 1:Enable
static UINT8 ExtLiveViewProcMode = 0xFF;            // External LiveView Process Mode, 0: Express 1:Hybrid
static UINT8 ExtLiveViewAlgoMode = 0xFF;            // External LiveView Algo Mode in HybridMode, 0: Fast 1:Liso 2: Hiso
static UINT8 ExtLiveViewOSMode = 0xFF;              // External LiveView OverSampling Mode in ExpressMode, 0: Disable 1:Enable
#ifdef CONFIG_SOC_A9
static AMP_DISP_WINDOW_HDLR_s *AV_encLcdWinHdlr = NULL; // LCD video plane window handler
static AMP_DISP_WINDOW_HDLR_s *AV_encTvWinHdlr = NULL;  // TV video plane window handler
#endif
static UINT8 AvEncodeDumpSkip = 0;                      // Encode Skip dump file, write data to SD card or just lies to fifo that it has muxed pending data
#define AVENC_SKIP_PRI         (0x1<<0)
#define AVENC_SKIP_SEC         (0x1<<1)
#define AVENC_SKIP_PRI_ADO     (0x1<<4)
#define AVENC_SKIP_SEC_ADO     (0x1<<5)
#define AVENC_SKIP_JPG         (0x1<<7)
static UINT8 AvLogMuxer = 0;
static UINT8 AvEncDualStream = 0;                       // Enable dual stream or not
static UINT8 AvEncDualHDStream = 0;                     // Enable Dual HD stream or not
static UINT16 AvEncSecStreamCustomWidth = 0;
static UINT16 AvEncSecStreamCustomHeight = 0;

static UINT8 EncodeSystem = 0;                      // Encode System, 0:NTSC, 1: PAL

static INPUT_ENC_MGT_s *AV_VideoEncMgt;         // Pointer to above tables
static UINT8 InputDeviceId = 0;
static UINT8 AV_EncModeIdx = 1;        // Current mode index
static UINT8 AVEncIsIqParamInit = 0;
static UINT8 AVEncIsHdrIqParam = 0;

#define AVENC_GOP_N   8                // I-frame distance
#define AVENC_GOP_IDR 8                // IDR-frame distance

/* Simple muxer to communicate with FIFO */
#define BITSFIFO_SIZE 32*1024*1024
static UINT8 *AV_H264BitsBuf;          // H.264 bitstream buffer
static UINT8 *AV_MjpgBitsBuf;          // MJPEG bitstream buffer
#define DESC_SIZE 40*3000
static UINT8 *AV_H264DescBuf;          // H.264 descriptor buffer (physical)
static UINT8 *AV_MjpgDescBuf;          // MJPEG descriptor buffer (physical)
static AMBA_KAL_TASK_t AV_VideoEncPriMuxTask = {0};    // Primary stream muxer task
static UINT8 *AV_VideoEncPriMuxStack = NULL;           // Stack for primary stream muxer task
#define AV_ENC_PRI_MUX_TASK_STACK_SIZE (8192)
static AMBA_KAL_SEM_t AvEncPriSem = {0};         // Counting semaphore for primary stream muxer task and fifo callback
static AMBA_KAL_TASK_t AVEncSecMuxTask = {0};    // Secondary stream muxer task
#define AV_ENC_SEC_MUX_TASK_STACK_SIZE (8192)
static UINT8 *AVEncSecMuxStack = NULL;           // Stack for Secondary stream muxer task
static AMBA_KAL_SEM_t AVEncSecSem = {0};         // Counting semaphore for Secondary stream muxer task and fifo callback

static int AvEncFnoPri = 0;                // Frame number counter for primary stream muxer
static int AvEncFnoSec = 0;                // Frame number counter for secondary stream muxer
static UINT32 AV_EncFrameRate = 0;         // Remember current framerate for primary muxer to calculate actual bitrate
static UINT64 AV_encPriBytesTotal = 0;     // total bytes primary stream muxer received
static UINT32 AV_encPriTotalFrames = 0;    // total frames primary stream muxer received
static UINT32 AV_EncSecFrameRate = 0;      // Remember current framerate for secondary muxer to calculate actual bitrate
static UINT64 AV_encSecBytesTotal = 0;     // total bytes secondary stream muxer received
static UINT32 AV_encSecTotalFrames = 0;    // total frames secondary stream muxer received
static AMP_FIFO_HDLR_s *AV_VirtualPriFifoHdlr = NULL;  // Primary stream vitrual fifo
static AMP_FIFO_HDLR_s *AV_VirtualSecFifoHdlr = NULL;  // Secondary stream vitrual fifo

static UINT32 AV_BrcFrameCount = 0;
static UINT32 AV_BrcByteCount = 0;
static UINT32 AV_BrcSecFrameCount = 0;
static UINT32 AV_BrcSecByteCount = 0;

#define INIT_DZOOM_FACTOR   (1<<16)

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

#ifdef CONFIG_SOC_A12
static AMBA_IMG_ENC_MONITOR_BITRATE_HDLR_s *BrcHdlrPri = NULL;  // Pri Stream BitRateMonitorControl instance
static AMBA_IMG_ENC_MONITOR_BITRATE_HDLR_s *BrcHdlrSec = NULL;  // Sec Stream BitRateMonitorControl instance
static AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *EncMonitorStrmHdlrPri = NULL;  // Pri Stream in encode monitor instance
static AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *EncMonitorStrmHdlrSec = NULL;  // Sec Stream in encode monitor instance
static AMBA_IMG_ENC_MONITOR_AQP_HDLR_s *AqpHdlrPri = NULL;  // Pri Stream AqpMonitorControl instance
static AMBA_IMG_ENC_MONITOR_AQP_HDLR_s *AqpHdlrSec = NULL;  // Sec Stream AqpMonitorControl instance
static UINT8 EncMonitorEnable = 1;                  // Enable encode monitor or not
static UINT8 EncMonitorAQPEnable = 1;               // Enable encode monitor AQP or not
static UINT8 *EncMonitorCyclicWorkBuffer = NULL;  // Encode monitor Cyclic working buffer
static UINT8 *EncMonitorServiceWorkBuffer = NULL; // BitRateMonitorControl working buffer
static UINT8 *EncMonitorServiceAqpWorkBuffer = NULL; // AQpMonitorControl working buffer
static UINT8 *EncMonitorStrmWorkBuffer = NULL;    // Encode monitor stream working buffer


int AmpUT_AVEnc_AqpPriStream(AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *StrmHdlr)
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

int AmpUT_AVEnc_AqpSecStream(AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *StrmHdlr)
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

static int AmpUT_AVEnc_GetDayLumaThresholdCB(int mode, UINT32 *threshold)
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
static int AmpUT_AVEnc_GetSceneComplexityRangeCB(int mode, UINT32 *complexMin, UINT32 *complexMid, UINT32 *complexMax)
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
static int AmpUT_AVEnc_QpAdjustmentCB(int mode, UINT8 *isQpModify, AMBA_IMG_ENCODE_MONITOR_STRM_HDLR_s *strmHdlr, AMBA_IMG_ENC_MONITOR_BITRATE_HDLR_s *hdlr)
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



static UINT8 AV_MjpegQuantMatrix[128] = {              // Standard JPEG qualty 50 table.
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

extern UINT32 _find_jpeg_segment(UINT8* img, UINT32 size, UINT16 marker);
void AmpUT_AVEnc_VPriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc = NULL;
    int Er = NG;
    UINT8 *BitsLimit = NULL;
    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_VideoEnc_PriMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&AvEncPriSem, 10000);  // Check if there is any pending frame to be muxed
        if (Er != OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }

        if (!(AvEncodeDumpSkip & AVENC_SKIP_PRI)) {
            if (AvVoutputPriFile == NULL) { // Open files when receiving the 1st frame
                char Fn[80];
                char mdASCII[3] = {'w','+','\0'};
                FORMAT_USER_DATA_s Udta;

                sprintf(Fn,"%s:\\OUT_%04d.%s", DefaultSlot, AvEncFnoPri, AvEncPriSpecH264?"h264":"mjpg");
                AvVoutputPriFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (AvVoutputPriFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    AvVoutputPriFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                sprintf(Fn,"%s:\\OUT_%04d.nhnt", DefaultSlot, AvEncFnoPri);
                AvVoutputPriIdxFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (AvVoutputPriIdxFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    AvVoutputPriIdxFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                sprintf(Fn,"%s:\\OUT_%04d.udta", DefaultSlot, AvEncFnoPri);
                AvVUdtaPriFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                while (AvVUdtaPriFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    AvVUdtaPriFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                }

                AmbaPrint("%s opened", Fn);

                Udta.nIdrInterval = AVENC_GOP_IDR/AVENC_GOP_N;
                Udta.nTimeScale = (EncodeSystem==0)? AV_VideoEncMgt[AV_EncModeIdx].TimeScale: AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL;
                Udta.nTickPerPicture = (EncodeSystem==0)? AV_VideoEncMgt[AV_EncModeIdx].TickPerPicture: AV_VideoEncMgt[AV_EncModeIdx].TickPerPicturePAL;
                Udta.nN = AVENC_GOP_N;
                Udta.nM = AV_VideoEncMgt[AV_EncModeIdx].GopM;
                /*
                if (EncRotation == AMP_ROTATE_90 || EncRotation == AMP_ROTATE_90_VERT_FLIP || \
                    EncRotation == AMP_ROTATE_270 || EncRotation == AMP_ROTATE_270_VERT_FLIP) {
                    Udta.nVideoWidth = VideoEncMgt[EncModeIdx].MainHeight;
                    Udta.nVideoHeight = VideoEncMgt[EncModeIdx].MainWidth;
                } else {*/
                Udta.nVideoWidth = AV_VideoEncMgt[AV_EncModeIdx].MainWidth;
                Udta.nVideoHeight = AV_VideoEncMgt[AV_EncModeIdx].MainHeight;
                //}
                Udta.nInterlaced = AV_VideoEncMgt[AV_EncModeIdx].Interlace;
                UT_AVEncodefwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)AvVUdtaPriFile);
                UT_AVEncodefclose((void *)AvVUdtaPriFile);

                NhntHeader.Signature[0]='n';
                NhntHeader.Signature[1]='h';
                NhntHeader.Signature[2]='n';
                NhntHeader.Signature[3]='t';
                NhntHeader.TimeStampResolution = (EncodeSystem==0)? AV_VideoEncMgt[AV_EncModeIdx].TimeScale: AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL;
                UT_AVEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)AvVoutputPriIdxFile);

                if (AvEncPriSpecH264) {
                    BitsLimit = AV_H264BitsBuf + BITSFIFO_SIZE - 1;
                } else {
                    BitsLimit = AV_MjpgBitsBuf + BITSFIFO_SIZE - 1;
                }
            }
        }

        Er = AmpFifo_PeekEntry(AV_VirtualPriFifoHdlr, &Desc, 0);  // Get a pending entry
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
            if (AvLogMuxer) {
                AmbaPrint("Pri[%d] %s pts:%8lld 0x%08x 0x%X", AV_encPriTotalFrames, Ty, Desc->Pts, Desc->StartAddr, Desc->Size);
            }
        } else {
            while (Er != 0) {
                AmbaPrint("[VPri]Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(AV_VirtualPriFifoHdlr, &Desc, 0);
            }
        }
        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
            if (!(AvEncodeDumpSkip & AVENC_SKIP_PRI)) {
                if (AvVoutputPriFile) {
                    UT_AVEncodefsync((void *)AvVoutputPriFile);
                    UT_AVEncodefclose((void *)AvVoutputPriFile);
                    AvEncFnoPri++;
                    UT_AVEncodefsync((void *)AvVoutputPriIdxFile);
                    UT_AVEncodefclose((void *)AvVoutputPriIdxFile);
                    AvVoutputPriFile = NULL;
                    AvVoutputPriIdxFile = NULL;
                    FileOffset = 0;
                }
            }

            Avg = (UINT32)(AV_encPriBytesTotal*8.0*AV_EncFrameRate/AV_encPriTotalFrames/1E3);

            AmbaPrint("Video Primary Muxer met EOS, total %d frames/fields", AV_encPriTotalFrames);
            AmbaPrint("Primary Bitrate Average: %d kbps\n", Avg);


            AV_encPriBytesTotal = 0;
            AV_encPriTotalFrames = 0;
        } else if (Desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
            //Do Nothing
        } else {
            if (!(AvEncodeDumpSkip & AVENC_SKIP_PRI)) {
                if (AvVoutputPriFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;

                    UT_AVEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)AvVoutputPriIdxFile);

                    if (AvEncPriSpecH264==0 && AV_VideoEncMgt[AV_EncModeIdx].MainHeight==1080) {
                        UINT8 *Mjpeg_bs = 0,*OriMjpeg_bs = 0;
                        UINT32 sof_addr = 0;
                        UINT8 *sof_ptr = 0;

                        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&Mjpeg_bs, (void **)&OriMjpeg_bs, ALIGN_32(Desc->Size), 32);
                        if (Er != OK) {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                               // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                               UT_AVEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)AvVoutputPriFile);
                            } else {
                                // AmbaCache_Invalidate(Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                UT_AVEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)AvVoutputPriFile);
                                // AmbaCache_Invalidate(H264BitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                                UT_AVEncodefwrite((const void *)AV_MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)AvVoutputPriFile);
                            }
                            //AmbaPrint("Out of cached memory for MJPEG header change flow!!");
                        } else {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                                memcpy(Mjpeg_bs, Desc->StartAddr, Desc->Size);
                            } else {
                                UINT8 *Bs = Mjpeg_bs;
                                memcpy(Mjpeg_bs, Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                Bs += BitsLimit - Desc->StartAddr + 1;
                                memcpy(Bs, AV_MjpgBitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                            }
                            sof_addr = _find_jpeg_segment(Mjpeg_bs,Desc->Size,0xFFC0);
                            sof_ptr = (UINT8*)(sof_addr + 5);
                            (*sof_ptr)    =  (UINT8)(AV_VideoEncMgt[AV_EncModeIdx].MainHeight >> 8);
                            *(sof_ptr+1)  = (UINT8)(AV_VideoEncMgt[AV_EncModeIdx].MainHeight &  0xFF);

                            UT_AVEncodefwrite((const void *)Mjpeg_bs, 1, Desc->Size, (void *)AvVoutputPriFile);

                            if (AmbaKAL_BytePoolFree((void *)OriMjpeg_bs) != OK) {
                                AmbaPrint("cached memory release fail for MJPEG header change flow");
                            }
                        }

                    } else {
                 //       AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                        if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                           // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                           UT_AVEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)AvVoutputPriFile);
                        } else {
                            UT_AVEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)AvVoutputPriFile);
                            if (AvEncPriSpecH264) {
                                UT_AVEncodefwrite((const void *)AV_H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)AvVoutputPriFile);
                            } else {
                                UT_AVEncodefwrite((const void *)AV_MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)AvVoutputPriFile);
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


            AV_encPriBytesTotal += Desc->Size;
            AV_encPriTotalFrames++;
            AV_BrcFrameCount++;
            AV_BrcByteCount += Desc->Size;
        }
        AmpFifo_RemoveEntry(AV_VirtualPriFifoHdlr, 1);
    }
}

void AmpUT_AVEnc_VSecMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *Desc = NULL;
    int Er = NG;
    UINT8 *BitsLimit = NULL;
    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_VideoEnc_SecMuxTask Start");

    while (1) {
        Er = AmbaKAL_SemTake(&AVEncSecSem, 10000);  // Check if there is any pending frame to be muxed
        if (Er != OK) {
          //  AmbaPrint(" no sem fff");
            continue;
        }

        if (!(AvEncodeDumpSkip & AVENC_SKIP_SEC)) {
            if (AvVoutputSecFile == NULL) { // Open files when receiving the 1st frame
                char Fn[80];
                char mdASCII[3] = {'w','+','\0'};
                FORMAT_USER_DATA_s Udta;

                sprintf(Fn,"%s:\\OUT_%04d_s.%s", DefaultSlot, AvEncFnoSec, AvEncSecSpecH264?"h264":"mjpg");
                AvVoutputSecFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (AvVoutputSecFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    AvVoutputSecFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                sprintf(Fn,"%s:\\OUT_%04d_s.nhnt", DefaultSlot, AvEncFnoSec);
                AvVoutputSecIdxFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                while (AvVoutputSecIdxFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    AvVoutputSecIdxFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", Fn);

                sprintf(Fn,"%s:\\OUT_%04d_s.udta", DefaultSlot, AvEncFnoSec);
                AvVUdtaSecFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                while (AvVUdtaSecFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    AvVUdtaSecFile = UT_AVEncodefopen((const char *)Fn, (const char *)mdASCII,0);
                }
                AmbaPrint("%s opened", Fn);

                Udta.nIdrInterval = SEC_STREAM_GOP_IDR/SEC_STREAM_GOP_N;
                if (EncodeSystem == 0) {
                    Udta.nTimeScale =  SEC_STREAM_TIMESCALE;
                    Udta.nTickPerPicture = SEC_STREAM_TICK;
                } else {
                    Udta.nTimeScale = SEC_STREAM_TIMESCALE_PAL;
                    Udta.nTickPerPicture = SEC_STREAM_TICK_PAL;
                }
                Udta.nN = AVENC_GOP_N;
                Udta.nM = AV_VideoEncMgt[AV_EncModeIdx].GopM;

                if (AvEncDualHDStream) {
                    if (AvEncSecStreamCustomWidth) {
                        Udta.nVideoWidth = AvEncSecStreamCustomWidth;
                        Udta.nVideoHeight = AvEncSecStreamCustomHeight;
                    } else {
                        UINT16 OutWidth = 0, OutHeight = 0;
                        MWUT_InputSetDualHDWindow(AV_VideoEncMgt, AV_EncModeIdx, &OutWidth, &OutHeight);
                        Udta.nVideoWidth = OutWidth;
                        Udta.nVideoHeight = OutHeight;
                    }
                } else {
                    if (AvEncSecStreamCustomWidth) {
                        Udta.nVideoWidth = AvEncSecStreamCustomWidth;
                        Udta.nVideoHeight = AvEncSecStreamCustomHeight;
                    } else {
                        Udta.nVideoWidth = SEC_STREAM_WIDTH;
                        Udta.nVideoHeight = SEC_STREAM_HEIGHT;
                    }
                }
                Udta.nInterlaced = AV_VideoEncMgt[AV_EncModeIdx].Interlace;
                UT_AVEncodefwrite((const void *)&Udta, 1, sizeof(FORMAT_USER_DATA_s), (void *)AvVUdtaSecFile);
                UT_AVEncodefclose((void *)AvVUdtaSecFile);

                NhntHeader.Signature[0] = 'n';
                NhntHeader.Signature[1] = 'h';
                NhntHeader.Signature[2] = 'n';
                NhntHeader.Signature[3] = 't';
                if (EncodeSystem == 0) {
                    NhntHeader.TimeStampResolution = SEC_STREAM_TIMESCALE;
                } else {
                    NhntHeader.TimeStampResolution = SEC_STREAM_TIMESCALE_PAL;
                }
                UT_AVEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)AvVoutputSecIdxFile);

                if (AvEncSecSpecH264) {
                    BitsLimit = AV_H264BitsBuf + BITSFIFO_SIZE - 1;
                } else {
                    BitsLimit = AV_MjpgBitsBuf + BITSFIFO_SIZE - 1;
                }
            }
        }

        Er = AmpFifo_PeekEntry(AV_VirtualSecFifoHdlr, &Desc, 0);  // Get a pending entry
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
            if (AvLogMuxer) {
                AmbaPrint("Sec[%d] %s pts:%8lld 0x%08x 0x%X", AV_encSecTotalFrames, Ty, Desc->Pts, Desc->StartAddr, Desc->Size);
            }
        } else {
            while (Er != 0) {
                AmbaPrint("[VSec]Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                Er = AmpFifo_PeekEntry(AV_VirtualSecFifoHdlr, &Desc, 0);
            }
        }
        if (Desc->Size == AMP_FIFO_MARK_EOS) {
            UINT32 Avg;
            // EOS
            if (!(AvEncodeDumpSkip & AVENC_SKIP_SEC)) {
                if (AvVoutputSecFile) {
                    UT_AVEncodefsync((void *)AvVoutputSecFile);
                    UT_AVEncodefclose((void *)AvVoutputSecFile);
                    AvEncFnoSec++;
                    UT_AVEncodefsync((void *)AvVoutputSecIdxFile);
                    UT_AVEncodefclose((void *)AvVoutputSecIdxFile);
                    AvVoutputSecFile = NULL;
                    AvVoutputSecIdxFile = NULL;
                    FileOffset = 0;
                }
            }

            Avg = (UINT32)(AV_encSecBytesTotal*8.0*AV_EncSecFrameRate/AV_encSecTotalFrames/1E3);

            AmbaPrint("Video Secondary Muxer met EOS, total %d frames/fields", AV_encSecTotalFrames);
            AmbaPrint("Secondary Bitrate Average: %d kbps\n", Avg);


            AV_encSecBytesTotal = 0;
            AV_encSecTotalFrames = 0;
        } else if (Desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
            //Do Nothing
        } else {
            if (!(AvEncodeDumpSkip & AVENC_SKIP_SEC)) {
                if (AvVoutputSecFile) {
                    NhntSample.CompositionTimeStamp = Desc->Pts;
                    NhntSample.DecodingTimeStamp = Desc->Pts;
                    NhntSample.DataSize = Desc->Size;
                    NhntSample.FileOffset = FileOffset;
                    FileOffset += Desc->Size;
                    NhntSample.FrameType = Desc->Type;

                    UT_AVEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)AvVoutputSecIdxFile);

                    if (AvEncSecSpecH264==0 && AV_VideoEncMgt[AV_EncModeIdx].MainHeight==1080) {
                        UINT8 *Mjpeg_bs = 0,*OriMjpeg_bs = 0;
                        UINT32 sof_addr = 0;
                        UINT8 *sof_ptr = 0;

                        Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&Mjpeg_bs, (void **)&OriMjpeg_bs, ALIGN_32(Desc->Size), 32);
                        if (Er != OK) {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                               // AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                               UT_AVEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)AvVoutputSecFile);
                            } else {
                                // AmbaCache_Invalidate(Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                UT_AVEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)AvVoutputSecFile);
                                // AmbaCache_Invalidate(H264BitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                                UT_AVEncodefwrite((const void *)AV_MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)AvVoutputSecFile);
                            }
                            //AmbaPrint("Out of cached memory for MJPEG header change flow!!");
                        } else {
                            if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                                memcpy(Mjpeg_bs, Desc->StartAddr, Desc->Size);
                            } else {
                                UINT8 *Bs = Mjpeg_bs;
                                memcpy(Mjpeg_bs, Desc->StartAddr, BitsLimit - Desc->StartAddr + 1);
                                Bs += BitsLimit - Desc->StartAddr + 1;
                                memcpy(Bs, AV_MjpgBitsBuf, Desc->Size - (BitsLimit - Desc->StartAddr + 1));
                            }
                            sof_addr = _find_jpeg_segment(Mjpeg_bs,Desc->Size,0xFFC0);
                            sof_ptr = (UINT8*)(sof_addr + 5);
                            (*sof_ptr)    =  (UINT8)(AV_VideoEncMgt[AV_EncModeIdx].MainHeight >> 8);
                            *(sof_ptr+1)  = (UINT8)(AV_VideoEncMgt[AV_EncModeIdx].MainHeight &  0xFF);

                            UT_AVEncodefwrite((const void *)Mjpeg_bs, 1, Desc->Size, (void *)AvVoutputSecFile);

                            if (AmbaKAL_BytePoolFree((void *)OriMjpeg_bs) != OK) {
                                AmbaPrint("cached memory release fail for MJPEG header change flow");
                            }
                        }

                    } else {
                        //AmbaPrint("Write: 0x%x sz %d limit %X",Desc->StartAddr,Desc->Size, BitsLimit);
                        if (Desc->StartAddr + Desc->Size <= BitsLimit) {
                           //AmbaCache_Invalidate(Desc->StartAddr, Desc->Size); // Need to flush when using cacheable memory
                           UT_AVEncodefwrite((const void *)Desc->StartAddr, 1, Desc->Size, (void *)AvVoutputSecFile);
                        } else {
                            UT_AVEncodefwrite((const void *)Desc->StartAddr, 1, BitsLimit - Desc->StartAddr + 1, (void *)AvVoutputSecFile);
                            if (AvEncSecSpecH264) {
                                UT_AVEncodefwrite((const void *)AV_H264BitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)AvVoutputSecFile);
                            } else {
                                UT_AVEncodefwrite((const void *)AV_MjpgBitsBuf, 1, Desc->Size - (BitsLimit - Desc->StartAddr + 1), (void *)AvVoutputSecFile);
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


            AV_encSecBytesTotal += Desc->Size;
            AV_encSecTotalFrames++;
            AV_BrcSecFrameCount++;
            AV_BrcSecByteCount += Desc->Size;
        }
        AmpFifo_RemoveEntry(AV_VirtualSecFifoHdlr, 1);
    }
}

void AmpUT_AVEnc_APriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *desc;
    int er;
    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT8 *bitsLimit = AvAudPriBitsBuf + AUDENC_BISFIFO_SIZE - 1;
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_AudioEnc_PriMuxTask Start");

    while (1) {
        AmbaKAL_SemTake(&AvEncAudioPriSem, AMBA_KAL_WAIT_FOREVER);

        if (!(AvEncodeDumpSkip & AVENC_SKIP_PRI_ADO)) {
            if (AvAOutputPriHeaderFile == NULL) {
                char fn[80];
                char mdASCII[3] = {'w','+','\0'};

                sprintf(fn,"%s:\\Pri_%04d.bin", DefaultSlot, AV_AFno);
                AvAOutputPriHeaderFile = UT_AVEncodefopen((const char *)fn, (const char *)mdASCII,1);
                AmbaPrint("%s opened", fn);
            }

            if (AvAOutputPriFile == NULL) {
                char fn[80];
                char mdASCII[3] = {'w','+','\0'};

                if (AvAudioEncodeType == 0 || AvAudioEncodeType == 2 || AvAudioEncodeType == 3)
                    sprintf(fn,"%s:\\Pri_%04d.aac", DefaultSlot, AV_AFno);
                else if (AvAudioEncodeType == 1 || AvAudioEncodeType == 4)
                    sprintf(fn,"%s:\\Pri_%04d.pcm", DefaultSlot, AV_AFno);
                else if (AvAudioEncodeType == 5)
                    sprintf(fn,"%s:\\Pri_%04d.ac3", DefaultSlot, AV_AFno);
                else if (AvAudioEncodeType == 6)
                    sprintf(fn,"%s:\\Pri_%04d.mp3", DefaultSlot, AV_AFno);
                else if (AvAudioEncodeType == 7)
                    sprintf(fn,"%s:\\Pri_%04d.opus", DefaultSlot, AV_AFno);

                AvAOutputPriFile = UT_AVEncodefopen((const char *)fn, (const char *)mdASCII,1);
                while (AvAOutputPriFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    AvAOutputPriFile = UT_AVEncodefopen((const char *)fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", fn);

                sprintf(fn,"%s:\\Pri_%04d.nhnt", DefaultSlot, AV_AFno);
                AvAoutputPriIdxFile = UT_AVEncodefopen((const char *)fn, (const char *)mdASCII,1);
                while (AvAoutputPriIdxFile==0) {
                    AmbaPrint("file open fail, wait 10ms then try again %d", __LINE__);
                    AmbaKAL_TaskSleep(10);
                    AvAoutputPriIdxFile = UT_AVEncodefopen((const char *)fn, (const char *)mdASCII,1);
                }
                AmbaPrint("%s opened", fn);

                NhntHeader.Signature[0]='n';
                NhntHeader.Signature[1]='h';
                NhntHeader.Signature[2]='n';
                NhntHeader.Signature[3]='t';
                UT_AVEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)AvAoutputPriIdxFile);

                AV_AFno++;
                bitsLimit = AvAudPriBitsBuf + AUDENC_BISFIFO_SIZE - 1;
            }
        }

        er = AmpFifo_PeekEntry(AvAudioEncPriVirtualFifoHdlr, &desc, 0);
        if (er == 0) {
            if (AvLogMuxer) {
                AmbaPrint("PriAudio[%d] pts:%8lld 0x%08x %d", AvAencPriTotalFrames, desc->Pts, desc->StartAddr, desc->Size);
            }
        } else {
            while (er != 0) {
                AmbaPrint("[[APri]]Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                er = AmpFifo_PeekEntry(AvAudioEncPriVirtualFifoHdlr, &desc, 0);
            }
        }

        if (!(AvEncodeDumpSkip & AVENC_SKIP_PRI_ADO)) {
            if (AvAOutputPriHeaderFile) {
                UINT32 size = desc->Size;
                UT_AVEncodefwrite((const void *)&size, 1, sizeof(UINT32), (void *)AvAOutputPriHeaderFile);

                if (desc->Size == AMP_FIFO_MARK_EOS) {
                    UT_AVEncodefsync((void *)AvAOutputPriHeaderFile);
                    UT_AVEncodefclose((void *)AvAOutputPriHeaderFile);
                    AvAOutputPriHeaderFile = NULL;
                    AmbaPrint("[Audio]Header met EOS,AencPriTotalFrames=%d",AvAencPriTotalFrames);
                    AvAencPriTotalFrames = 0;
                }
            }

            if (AvAOutputPriFile) {
                NhntSample.CompositionTimeStamp = desc->Pts;
                NhntSample.DecodingTimeStamp = desc->Pts;
                NhntSample.DataSize = desc->Size;
                NhntSample.FileOffset = FileOffset;
                FileOffset += desc->Size;
                NhntSample.FrameType = desc->Type;

                UT_AVEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)AvAoutputPriIdxFile);
                //AmbaPrint("[Pri]Write: 0x%x sz %d limit %X",  desc->StartAddr, desc->Size, bitsLimit);
                if (desc->Size == AMP_FIFO_MARK_EOS) {
                    UT_AVEncodefsync((void *)AvAOutputPriFile);
                    UT_AVEncodefclose((void *)AvAOutputPriFile);
                    AvAOutputPriFile = NULL;
                    UT_AVEncodefsync((void *)AvAoutputPriIdxFile);
                    UT_AVEncodefclose((void *)AvAoutputPriIdxFile);
                    AvAoutputPriIdxFile = NULL;
                    FileOffset = 0;
                    AmbaPrint("[Audio]Muxer met EOS");
                } else if (desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
                    //do nothing
                } else {
                    if (desc->StartAddr + desc->Size <= bitsLimit) {
                        UT_AVEncodefwrite((const void *)desc->StartAddr, 1, desc->Size, (void *)AvAOutputPriFile);
                    }else{
                        AmbaPrint("[Audio]Muxer Wrap Around");
                        UT_AVEncodefwrite((const void *)desc->StartAddr, 1, bitsLimit - desc->StartAddr + 1, (void *)AvAOutputPriFile);
                        UT_AVEncodefwrite((const void *)AvAudPriBitsBuf, 1, desc->Size - (bitsLimit - desc->StartAddr + 1), (void *)AvAOutputPriFile);
                    }
                }
            }
        } else {
            AmbaKAL_TaskSleep(1);
            if (desc->Type == AMP_FIFO_TYPE_EOS) AmbaPrint("[Pri]Muxer met EOS");
        }
        AmpFifo_RemoveEntry(AvAudioEncPriVirtualFifoHdlr, 1);
        AvAencPriTotalFrames++;
    }
}

/**
 * Display window initialization
 */
static int AmpUT_AVEnc_DisplayStart(void)
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
        window.TargetAreaOnPlane.Height = (AV_VideoEncMgt[AV_EncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
        window.TargetAreaOnPlane.X = 0;
        window.TargetAreaOnPlane.Y = (480-window.TargetAreaOnPlane.Height)/2;
        window.SourceDesc.Enc.VinCh = AvVinChannel;
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
        window.SourceDesc.Enc.VinCh = AvVinChannel;
        if (AmpUT_Display_Window_Create(1, &window) == NG) {
            return NG;
        }
    }

    if (1) {
        /** Step 2: Setup device */
        // Setup LCD & TV
        if (AvLCDLiveview) {
            AmpUT_Display_Start(0);
        } else {
            AmpUT_Display_Stop(0);
        }
        if (AvTvLiveview) {
            AmpUT_Display_Start(1);
        } else {
            AmpUT_Display_Stop(1);
        }
    }

    // Active Window 7002 cmd
    if (AvLCDLiveview) {
        AmpUT_Display_Act_Window(0);
    } else {
        AmpUT_Display_DeAct_Window(0);
    }
    if (AvTvLiveview) {
        AmpUT_Display_Act_Window(1);
    } else {
        AmpUT_Display_DeAct_Window(1);
    }
    return 0;
}


static int AmpUT_AVEnc_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *numFrames = info;
    AMBA_KAL_SEM_t *pSem = NULL;

    if (hdlr == AvAudioEncPriVirtualFifoHdlr) {
        pSem = &AvEncAudioPriSem;
    } else if (hdlr == AV_VirtualPriFifoHdlr) {
        pSem = &AvEncPriSem;
    } else if (hdlr == AV_VirtualSecFifoHdlr) {
        pSem = &AVEncSecSem;
    }

    //AmbaPrint("AmpUT_AudioEnc_FifoCB on Event: 0x%x 0x%x", event, *numFrames);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for(i = 0; i < *numFrames; i++) {
            AmbaKAL_SemGive(pSem);
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            AmbaKAL_SemGive(pSem);
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
static int AmpUT_AVEncPipeCallback(void *hdlr,UINT32 event, void *info)
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
 * VIN sensor mode switch callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_AVEncVinSwitchCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {
        case AMP_VIN_EVENT_INVALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_INVALID info: %X %X", info, AvImgSchdlr);

            if (AvImgSchdlr) AmbaImgSchdlr_Enable(AvImgSchdlr, 0);
            Amba_Img_VIn_Invalid(hdlr, (UINT32 *)NULL);
            break;
        case AMP_VIN_EVENT_VALID:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_VALID info: %X %X", info, AvImgSchdlr);
            if (AvEnc3AEnable) {
                Amba_Img_VIn_Valid(hdlr, (UINT32 *)NULL);
            }

            if (AvImgSchdlr) AmbaImgSchdlr_Enable(AvImgSchdlr, 1);
            break;
        case AMP_VIN_EVENT_CHANGED_PRIOR:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_PRIOR info: %X", info);
            if (AvEnc3AEnable) {
                Amba_Img_VIn_Changed_Prior(hdlr, (UINT32 *)NULL);
            }
        #ifdef CONFIG_SOC_A12
            if (AvStatus == STATUS_AV_LIVEVIEW) { // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window = {0}, DefWindow = {0};

                if (1) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;
                    window.TargetAreaOnPlane.Width = 960;
                    window.TargetAreaOnPlane.Height = (AV_VideoEncMgt[AV_EncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                    window.TargetAreaOnPlane.X = 0;
                    window.TargetAreaOnPlane.Y = (480 - window.TargetAreaOnPlane.Height)/2;
                    if(AmpUT_Display_GetWindowCfg(0, &DefWindow) != OK) {
                        AmpUT_Display_Window_Create(0, &window);
                    } else {
                        AmpUT_Display_SetWindowCfg(0, &window);
                    }

                    if (AvLCDLiveview) {
                        AmpUT_Display_Act_Window(0);
                    } else {
                        AmpUT_Display_DeAct_Window(0);
                    }
                }
            }

            if (AvStatus == STATUS_AV_LIVEVIEW) {
                AMP_DISP_WINDOW_CFG_s window = {0}, DefWindow = {0};

                if (1) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;
                    window.TargetAreaOnPlane.Width = (AV_VideoEncMgt[AV_EncModeIdx].AspectRatio == VAR_16x9)? 1920: 1440;
                    window.TargetAreaOnPlane.Height = 1080;
                    window.TargetAreaOnPlane.X = (1920 - window.TargetAreaOnPlane.Width)/2;
                    window.TargetAreaOnPlane.Y = 0;
                    if(AmpUT_Display_GetWindowCfg(1, &DefWindow) != OK) {
                        AmpUT_Display_Window_Create(1, &window);
                    } else {
                        AmpUT_Display_SetWindowCfg(1, &window);
                    }

                    if (AvTvLiveview) {
                        AmpUT_Display_Act_Window(0);
                    } else {
                        AmpUT_Display_DeAct_Window(0);
                    }
                }
            }
        #endif
            break;
        case AMP_VIN_EVENT_CHANGED_POST:
            AmbaPrint("VinSWCB: AMP_VIN_EVENT_CHANGED_POST info: %X", info);

            if (AvEnc3AEnable) {
                extern void _Set_AdjTableNo(UINT32 chNo, int AdjTableNo);
                UINT8 IsPhotoLiveView = 0;
                UINT16 PipeMode = IP_EXPERSS_MODE;
                UINT16 AlgoMode = IP_MODE_LISO_VIDEO;
                AMBA_SENSOR_MODE_INFO_s SensorModeInfo;
                AMBA_SENSOR_MODE_ID_u SensorMode = {0};

                SensorMode.Data = (EncodeSystem==0)? AV_VideoEncMgt[AV_EncModeIdx].InputMode: AV_VideoEncMgt[AV_EncModeIdx].InputPALMode;
                AmbaSensor_GetModeInfo(AvVinChannel, SensorMode, &SensorModeInfo);

                if (AvImgSchdlr) {
                    AMBA_IMG_SCHDLR_UPDATE_CFG_s SchdlrCfg = {0};

                    AmbaImgSchdlr_GetConfig(AvImgSchdlr, &SchdlrCfg);
                    if (LiveViewProcMode && LiveViewAlgoMode) {
                        SchdlrCfg.VideoProcMode = 1;
                        if (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
                            SchdlrCfg.VideoProcMode |= 0x10;
                        }
                    }
                    SchdlrCfg.AAAStatSampleRate = AV_VideoEncMgt[AV_EncModeIdx].ReportRate;
                    AmbaImgSchdlr_SetConfig(AvImgSchdlr, &SchdlrCfg);  // One MainViewID (not vin) need one scheduler.
                }

            #ifdef CONFIG_SOC_A12
                { // Load IQ params
                    extern UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID);
                    UINT8 IsSensorHdrMode;

                    // Inform 3A LV sensor mode is Hdr or not
                    IsSensorHdrMode = (SensorModeInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE)? 1: 0;
                    AmbaImg_Proc_Cmd(MW_IP_SET_VIDEO_HDR_ENABLE, 0/*ChNo*/, (UINT32)IsSensorHdrMode, 0);

                    if (AVEncIsIqParamInit == 0 || IsSensorHdrMode != AVEncIsHdrIqParam) {
                        App_Image_Init_Iq_Params(0, InputDeviceId);
                        AVEncIsIqParamInit = 1;
                        AVEncIsHdrIqParam = IsSensorHdrMode;
                    }
                }
            #endif

                //inform 3A LiveView pipeline
                if (LiveViewProcMode && LiveViewAlgoMode) {
                    PipeMode = IP_HYBRID_MODE;
                    //Change Adj Table to TA one if necessary
                    if (AvEncTAEnable) {
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
                    LiveViewInfo.MainW = AV_VideoEncMgt[AV_EncModeIdx].MainWidth;
                    LiveViewInfo.MainH = AV_VideoEncMgt[AV_EncModeIdx].MainHeight;
                    if (EncodeSystem == 0) {
                        LiveViewInfo.FrameRateInt = AV_VideoEncMgt[AV_EncModeIdx].TimeScale/AV_VideoEncMgt[AV_EncModeIdx].TickPerPicture;
                    } else {
                        LiveViewInfo.FrameRateInt = AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL/AV_VideoEncMgt[AV_EncModeIdx].TickPerPicturePAL;
                    }
                    LiveViewInfo.FrameRateInt = UT_AVFrameRateIntConvert(LiveViewInfo.FrameRateInt);
                    AmbaSensor_GetStatus(AvVinChannel, &SsrStatus);
                    LiveViewInfo.BinningHNum = InputInfo->HSubsample.FactorNum;
                    LiveViewInfo.BinningHDen = InputInfo->HSubsample.FactorDen;
                    LiveViewInfo.BinningVNum = InputInfo->VSubsample.FactorNum;
                    LiveViewInfo.BinningVDen = InputInfo->VSubsample.FactorDen;
                    AmbaImg_Proc_Cmd(MW_IP_SET_LIVEVIEW_INFO, 0/*ChIndex*/, (UINT32)&LiveViewInfo, 0);
                }

                //inform 3A FrameRate info
                {
                    UINT32 FrameRate = 0, FrameRatex1000 = 0;

                    if (EncodeSystem == 0) {
                        FrameRate = AV_VideoEncMgt[AV_EncModeIdx].TimeScale/AV_VideoEncMgt[AV_EncModeIdx].TickPerPicture;
                        FrameRatex1000 = AV_VideoEncMgt[AV_EncModeIdx].TimeScale*1000/AV_VideoEncMgt[AV_EncModeIdx].TickPerPicture;
                    } else {
                        FrameRate = AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL/AV_VideoEncMgt[AV_EncModeIdx].TickPerPicturePAL;
                        FrameRatex1000 = AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL*1000/AV_VideoEncMgt[AV_EncModeIdx].TickPerPicturePAL;
                    }
                    FrameRate = UT_AVFrameRateIntConvert(FrameRate);
                    AmbaImg_Proc_Cmd(MW_IP_SET_FRAME_RATE, 0/*ChIndex*/, FrameRate, FrameRatex1000);
                }

                Amba_Img_VIn_Changed_Post(hdlr, (UINT32 *)NULL);
            }
        #ifdef CONFIG_SOC_A12
            if (AvEnc_EisAlgoEnable) {
                AmbaEis_Inactive();
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_DISABLE, 0, 0, 0);
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_SET_FOCAL_LENGTH, 360, 0, 0);
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_INIT, 0, 0, 0);
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_MODE_INIT, 0, 0, 0);
                AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_ENABLE, 0, 0, 0);
            }
        #endif

        #ifdef CONFIG_SOC_A9
            if (AvStatus == STATUS_AV_LIVEVIEW) { // When runtime switching sensor mode, display should change according to the new aspect ratio
                AMP_DISP_WINDOW_CFG_s window;
                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

                if (AV_encLcdWinHdlr) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;
                    window.TargetAreaOnPlane.Width = 960;
                    window.TargetAreaOnPlane.Height = (AV_VideoEncMgt[AV_EncModeIdx].AspectRatio == VAR_16x9)? 360: 480;
                    window.TargetAreaOnPlane.X = 0;
                    window.TargetAreaOnPlane.Y = (480 - window.TargetAreaOnPlane.Height)/2;
                    AmpDisplay_SetWindowCfg(AV_encLcdWinHdlr, &window);
                    if (AvLCDLiveview) {
                        AmpDisplay_SetWindowActivateFlag(AV_encLcdWinHdlr, 1);
                    } else {
                        AmpDisplay_SetWindowActivateFlag(AV_encLcdWinHdlr, 0);
                    }
                }
                AmpDisplay_Update(LCDHdlr);
            }

            if (AvStatus == STATUS_AV_LIVEVIEW) {
                AMP_DISP_WINDOW_CFG_s window;

                memset(&window, 0x0, sizeof(AMP_DISP_WINDOW_CFG_s));

                if (AV_encTvWinHdlr) {
                    window.Source = AMP_DISP_ENC;
                    window.CropArea.Width = 0;
                    window.CropArea.Height = 0;
                    window.CropArea.X = 0;
                    window.CropArea.Y = 0;
                    window.TargetAreaOnPlane.Width = (AV_VideoEncMgt[AV_EncModeIdx].AspectRatio == VAR_16x9)? 1920: 1440;
                    window.TargetAreaOnPlane.Height = 1080;
                    window.TargetAreaOnPlane.X = (1920 - window.TargetAreaOnPlane.Width)/2;
                    window.TargetAreaOnPlane.Y = 0;
                    AmpDisplay_SetWindowCfg(AV_encTvWinHdlr, &window);
                    if (AvTvLiveview) {
                        AmpDisplay_SetWindowActivateFlag(AV_encTvWinHdlr, 1);
                    } else {
                        AmpDisplay_SetWindowActivateFlag(AV_encTvWinHdlr, 0);
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
static int AmpUT_AVEncVinEventCallback(void *hdlr,UINT32 event, void *info)
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
 * Generic VideoEnc ImgSchdlr callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
static int AmpUT_AVEncImgSchdlrCallback(void *hdlr,UINT32 event, void *info)
{
//    static int cfacnt = 0;

    switch (event) {
        case AMBA_IMG_SCHDLR_EVENT_CFA_STAT_READY:
            if (AvEnc3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspCfa_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_RGB_STAT_READY:
            if (AvEnc3AEnable) {
                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_MAIN_CFA_HIST_READY:
            if (AvEnc3AEnable) {
//                AMBA_IMG_SCHDLR_META_CFG_s *Meta = (AMBA_IMG_SCHDLR_META_CFG_s *)info;
                //Amba_Img_VDspRgb_Handler(hdlr, (UINT32 *)Meta);
            }
            break;
        case AMBA_IMG_SCHDLR_EVENT_HDR_CFA_HIST_READY:
            if (AvEnc3AEnable) {
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
 * Generic AVEnc codec callback
 *
 * @param [in] hdlr  The event belongs to which vin
 * @param [in] event The event
 * @param [in] info Information the event brings
 *
 * @return 0
 */
extern int AmpUT_AVEnc_EncStop(void);
static int AmpUT_AVEncCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            break;
        case AMP_ENC_EVENT_RAW_CAPTURE_DONE:
            // Next raw capture is allowed from now on
            //StillRawCaptureRunning = 0;
            break;
        case AMP_ENC_EVENT_BACK_TO_LIVEVIEW:
            break;
        case AMP_ENC_EVENT_BG_PROCESS_DONE:
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
            //AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VCAP_2ND_YUV_READY:
            //AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_2ND_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VCAP_ME1_Y_READY:
            //AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! AMP_ENC_EVENT_VCAP_ME1_Y_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_HYBRID_YUV_READY:
            //AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! AMP_ENC_EVENT_HYBRID_YUV_READY !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_VDSP_ASSERT:
            //AmbaPrintColor(RED, "AmpUT_AVEnc: !!!!!!!!!!! AMP_ENC_EVENT_VDSP_ASSERT !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! (Video)AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            AmpUT_AVEnc_EncStop();
            //EncDateTimeStampPri = EncDateTimeStampSec = 0;
            AvStatus = STATUS_AV_LIVEVIEW;
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! (Video)AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DESC_OVERRUN:
            AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! (Video)AMP_ENC_EVENT_DESC_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY:
            if (0) {
                UINT32 *Percetage = (UINT32 *) info;
                AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY, Percetage=%d!!!!!!!!!!",Percetage);
            }
            break;
        default:
            AmbaPrint("AmpUT_AVEnc: Unknown %X info: %x", event, info);
            break;
    }
    return 0;
}

static int AmpUT_AVEncAudioCallback(void *hdlr, UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_STATE_CHANGED:
            break;
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! (Audio)AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            AmpUT_AVEnc_EncStop();
            //EncDateTimeStampPri = EncDateTimeStampSec = 0;
            AvStatus = STATUS_AV_LIVEVIEW;
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! (Audio)AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DESC_OVERRUN:
            AmbaPrint("AmpUT_AVEnc: !!!!!!!!!!! (Audio)AMP_ENC_EVENT_DESC_OVERRUN !!!!!!!!!!");
            break;
        default:
            AmbaPrint("AmpUT_AVEnc: Unknown %X info: %x", event, info);
            break;
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
UINT32 AmpUT_AVEnc_DspWork_Calculate(UINT8 **addr, UINT32 *size)
{
    extern UINT8 *DspWorkAreaResvStart;
    extern UINT32 DspWorkAreaResvSize;


    //only allocate MJPEG Bits
    (*addr) = DspWorkAreaResvStart;
    (*size) = DspWorkAreaResvSize - 15*1024*1024;

    AmbaPrint("[DspWork_Calculate] Addr 0x%X, Sz %u", *addr, *size);
    return 0;
}


/**
 *  Unit Test Initialization
 */
int AmpUT_AVEnc_Init(int sensorID, int LcdID)
{
    int er;
    void *TmpbufRaw = NULL;
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

    // Video side
    InputDeviceId = sensorID;
    if (sensorID >= 0) {
        AvVinChannel.Bits.VinID = 0;
        AvVinChannel.Bits.SensorID = 0x1;
        AV_VideoEncMgt = RegisterMWUT_Sensor_Driver(AvVinChannel,sensorID);
        {
        #ifdef CONFIG_SOC_A12 // Temporary disable for A9
            extern int App_Image_Init(UINT32 ChCount, int sensorID);
            AMBA_SENSOR_DEVICE_INFO_s SensorDeviceInfo = {0};
            AmbaSensor_GetDeviceInfo(AvVinChannel, &SensorDeviceInfo);
            if (SensorDeviceInfo.HdrIsSupport == 1) {
                App_Image_Init(2, sensorID);
            } else {
                App_Image_Init(1, sensorID);
            }
        #endif
        }
    }

    // Create semaphores for muxers
    if (AmbaKAL_SemCreate(&AvEncPriSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Pri Mux Semaphore creation failed");
    }
    if (AmbaKAL_SemCreate(&AVEncSecSem, 0) != OK) {
        AmbaPrint("VideoEnc UnitTest: Sec Mux Semaphore creation failed");
    }

    // Prepare stacks for muxer tasks
    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AV_VideoEncPriMuxStack, (void **)&TmpbufRaw, AV_ENC_PRI_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for Pri muxer stack!!");
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AVEncSecMuxStack, (void **)&TmpbufRaw, AV_ENC_SEC_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for Sec muxer stack!!");
    }

    // Create muxer tasks
    if (AmbaKAL_TaskCreate(&AV_VideoEncPriMuxTask, "AV Encoder UnitTest Primary Muxing Task", 50, \
         AmpUT_AVEnc_VPriMuxTask, 0x0, AV_VideoEncPriMuxStack, AV_ENC_PRI_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("AVEnc UnitTest: Pri Muxer task creation failed");
    }
    if (AmbaKAL_TaskCreate(&AVEncSecMuxTask, "AV Encoder UnitTest Secondary Muxing Task", 50, \
         AmpUT_AVEnc_VSecMuxTask, 0x0, AVEncSecMuxStack, AV_ENC_SEC_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("AVEnc UnitTest: Sec Muxer task creation failed");
    }

    // Initialize VIN module
    {
        AMP_VIN_INIT_CFG_s vinInitCfg;

        AmpVin_GetInitDefaultCfg(&vinInitCfg);

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AvVinWorkBuffer, (void **)&TmpbufRaw, vinInitCfg.MemoryPoolSize, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for vin!!");
        }
        vinInitCfg.MemoryPoolAddr = AvVinWorkBuffer;
        AmpVin_Init(&vinInitCfg);
    }

    // Initialize VIDEOENC module
    {
        AMP_VIDEOENC_INIT_CFG_s encInitCfg;

        AmpVideoEnc_GetInitDefaultCfg(&encInitCfg);

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AvVEncWorkBuffer, (void **)&TmpbufRaw, encInitCfg.MemoryPoolSize, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for enc!!");
        }
        encInitCfg.MemoryPoolAddr = AvVEncWorkBuffer;
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

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AvImgSchdlrWorkBuffer, (void **)&TmpbufRaw, ISInitCfg.MemoryPoolSize, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for imgschdlr!!");
        }

        ISInitCfg.IsoCfgNum = AmpResource_GetIKIsoConfigNumber(Pipe);
        ISInitCfg.MemoryPoolAddr = AvImgSchdlrWorkBuffer;
        er = AmbaImgSchdlr_Init(&ISInitCfg);
        if (er != AMP_OK) {
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
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorCyclicWorkBuffer, (void **)&TmpbufRaw, MemSize + 32, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for encMonitorCyclic!!");
        }
        EncMonitorCyclicWorkBuffer = (UINT8 *)ALIGN_32((UINT32)EncMonitorCyclicWorkBuffer);
        MonitorCyclicCfg.MemoryPoolSize = MemSize;
        MonitorCyclicCfg.MemoryPoolAddr = EncMonitorCyclicWorkBuffer;
        er = AmbaEncMonitor_Init(&MonitorCyclicCfg);
        if (er != AMP_OK) {
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

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorServiceWorkBuffer, (void **)&TmpbufRaw, MemSize + 32, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for encMonitorService!!");
        }
        EncMonitorServiceWorkBuffer = (UINT8 *)ALIGN_32((UINT32)EncMonitorServiceWorkBuffer);
        MonitorBRateCfg.MemoryPoolSize = MemSize;
        MonitorBRateCfg.MemoryPoolAddr = EncMonitorServiceWorkBuffer;
        er = AmbaEncMonitorBRC_init(&MonitorBRateCfg);
        if (er != AMP_OK) {
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

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorServiceAqpWorkBuffer, (void **)&TmpbufRaw, MemSize + 32, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for encMonitorAqpService!!");
        }
        EncMonitorServiceAqpWorkBuffer = (UINT8 *)ALIGN_32((UINT32)EncMonitorServiceAqpWorkBuffer);
        MonitorAqpCfg.MemoryPoolSize = MemSize;
        MonitorAqpCfg.MemoryPoolAddr = EncMonitorServiceAqpWorkBuffer;
        er= AmbaEncMonitorAQP_init(&MonitorAqpCfg);
        if (er != AMP_OK) {
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

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EncMonitorStrmWorkBuffer, (void **)&TmpbufRaw, MemSize + 32, 32);
        if (er != OK) {
            AmbaPrint("Out of memory for encMonitorStream!!");
        }
        EncMonitorStrmWorkBuffer = (UINT8 *)ALIGN_32((UINT32)EncMonitorStrmWorkBuffer);
        MonitorStrmCfg.MemoryPoolSize = MemSize;
        MonitorStrmCfg.MemoryPoolAddr = EncMonitorStrmWorkBuffer;
        er = AmbaEncMonitorStream_Init(&MonitorStrmCfg);
        if (er != AMP_OK) {
            AmbaPrint("AmbaEncMonitorStream_Init Fail!");
        }
    }
#endif

    // Allocate bitstream buffers
    {
    #ifdef CONFIG_SOC_A9
        extern UINT8 *DspWorkAreaResvLimit;
    #endif

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AV_H264BitsBuf, (void **)&TmpbufRaw, BITSFIFO_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }

        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AV_H264DescBuf, (void **)&TmpbufRaw, DESC_SIZE, 32);
        if (er != OK) {
            AmbaPrint("Out of cached memory for bitsFifo!!");
        }

#ifdef CONFIG_SOC_A12
        AV_MjpgDescBuf = AV_H264DescBuf;
        AV_MjpgBitsBuf = AV_H264BitsBuf;
#else
        // This is an example how to use DSP working memory when APP knows these memory area is not used.
        // We steal 15MB here
        AV_MjpgDescBuf = DspWorkAreaResvLimit + 1 - 1*1024*1024;
        AV_MjpgBitsBuf = AV_MjpgDescBuf - BITSFIFO_SIZE;
#endif

    }

    // Generate quantization tables for MJPEG
    if (1) {
        int i, temp, quality = 20, scale;

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
            temp = ((long) AV_MjpegQuantMatrix[i] * scale + 50L) / 100L;
            /* limit the values to the valid range */
            if (temp <= 0L) temp = 1L;
            if (temp > 255L) temp = 255L; /* max quantizer needed for baseline */
            AV_MjpegQuantMatrix[i] = temp;
        }

    }

    // Audio side
    // Create simple muxer semophore
    if (AmbaKAL_SemCreate(&AvEncAudioPriSem, 0) != OK) {
        AmbaPrint("AudioEnc UnitTest: Semaphore creation failed");
    }

    // Create simple muxer task
    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_AvAudioEncPriStack, (void **)&TmpbufRaw, AV_ENC_AUD_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }

    if (AmbaKAL_TaskCreate(&AvAudioEncPriMuxTask, "Audio Encoder UnitTest Primary Muxing Task", 50, \
         AmpUT_AVEnc_APriMuxTask, 0x0, AmpUT_AvAudioEncPriStack, AV_ENC_AUD_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("AudioEnc UnitTest: Primary Muxer task creation failed");
    }

    // Init AUDIOENC module
    {
        AMP_AUDIOENC_INIT_CFG_s encInitCfg;

        AmpAudioEnc_GetInitDefaultCfg(&encInitCfg);
        if (AvAudioMainWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AvAudioMainWorkBuf, (void **)&TmpbufRaw, encInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for audio!!");
            }
        }
        encInitCfg.MemoryPoolAddr = AvAudioMainWorkBuf;
        AmpAudioEnc_Init(&encInitCfg);
    }

    if (AmpUT_Display_Init() == NG) {
        return NG;
    }

#ifdef CONFIG_SOC_A12
    if (AvEnc_EisAlgoEnable){
        AmbaEisTask_Init(12, 0);
    }
#endif

    return 0;
}


/* add audio encode task, which share same audio encode engine */
/* flag definition */
#define AV_AUDIOENC_TASK_ADD_ALL           0x0
#define AV_AUDIOENC_TASK_ADD_EXCLUDE_AIN   0x1
#define AV_AUDIOENC_TASK_ADD_EXCLUDE_AENC  (0x1<<1)
int AmpUT_AVEnc_Task_Add(UINT8 *workCacheBuf, UINT8 *workNCBuf, UINT32 cacheSize, UINT32 NCacheSize, UINT32 *inputHdlr, UINT32 *encodeHdlr, UINT32 *encBuffHdlr, UINT8 flag)
{
    UINT32 *retHdlr;
    AMBA_AUDIO_IO_CREATE_INFO_s inputInfo;
    AMBA_AUDIO_TASK_CREATE_INFO_s encInfo;
    AMBA_ABU_CREATE_INFO_s abuInfo;
    AMBA_AUDIO_BUF_INFO_s inC, inNonC;
    UINT32 inputCachedSize, inputNonCachedSize, encSize, abuSize;
    UINT32 *encAddr, *abuAddr;
    AMBA_AUDIO_COMBINE_INFO_s combine;
    UINT8 *Caddr, *NCaddr, *CaddrEnd, *NCaddrEnd;

    Caddr = (UINT8 *) ALIGN_4((UINT32)workCacheBuf);
    NCaddr = (UINT8 *) ALIGN_8((UINT32)workNCBuf);
    CaddrEnd = Caddr + cacheSize;
    NCaddrEnd = NCaddr + NCacheSize;

    if (flag&AV_AUDIOENC_TASK_ADD_EXCLUDE_AIN) {
    //share ain
    } else {
        inputInfo.I2sIndex = 0; // depends on project?
        inputInfo.MaxChNum = 2;
        inputInfo.MaxDmaDescNum = 16; // depends on project?
        inputInfo.MaxDmaSize = 1024; // depends on project?
        inputInfo.MaxSampleFreq = 48000;
        inputCachedSize = AmbaAudio_InputCachedSizeQuery(&inputInfo);
        inputNonCachedSize = AmbaAudio_InputNonCachedSizeQuery(&inputInfo);
        inC.pHead = (UINT32 *)Caddr;
        inC.MaxSize = inputCachedSize;
        Caddr += inputCachedSize;
        if (Caddr > CaddrEnd) {
            AmbaPrint("AIN input Cache buffer fail");
        }

        inNonC.pHead = (UINT32 *)NCaddr;
        inNonC.MaxSize = inputNonCachedSize;
        NCaddr += inputNonCachedSize;
        if (NCaddr > NCaddrEnd) {
            AmbaPrint("AIN input NonCache buffer fail");
        }
        retHdlr = AmbaAudio_InputCreate(&inputInfo, &inC, &inNonC);
        if ((int)retHdlr == NG) {
            AmbaPrint("AIN cre fail");
        } else *inputHdlr = (UINT32)retHdlr;
    }

    if (flag&AV_AUDIOENC_TASK_ADD_EXCLUDE_AENC) {
        //share aenc
    } else {
        encInfo.MaxSampleFreq = 48000;
        encInfo.MaxChNum = 2;
        encInfo.MaxFrameSize = 2048;
        encSize = AmbaAudio_EncSizeQuery(&encInfo);
        Caddr = (UINT8 *) ALIGN_4((UINT32)Caddr);
        encAddr = (UINT32 *)Caddr;
        Caddr += encSize;
        if (Caddr > CaddrEnd) {
            AmbaPrint("AENC buffer fail");
        }
        retHdlr = AmbaAudio_EncCreate(&encInfo, encAddr, encSize);
        if ((int)retHdlr == NG) {
            AmbaPrint("AENC cre fail");
        } else *encodeHdlr = (UINT32)retHdlr;
    }

    abuInfo.MaxSampleFreq = 48000;
    abuInfo.MaxChNum = 2;
    abuInfo.MaxChunkNum = 16; // depends on project?
    abuSize = AmbaAudio_BufferSizeQuery(&abuInfo);
    Caddr = (UINT8 *) ALIGN_4((UINT32)Caddr);
    abuAddr = (UINT32 *)Caddr;
    Caddr += abuSize;
    if (Caddr > CaddrEnd) {
        AmbaPrint("ABU buffer fail");
    }
    retHdlr = AmbaAudio_BufferCreate(&abuInfo, abuAddr, abuSize);
    if ((INT32)retHdlr == NG) {
        AmbaPrint("ABU cre fail");
    } else *encBuffHdlr = (UINT32)retHdlr;

    combine.pAbu = (UINT32 *)(*encBuffHdlr);
    combine.pSrcApu = (UINT32 *)(*inputHdlr);
    combine.pDstApu = (UINT32 *)(*encodeHdlr);
    if (AmbaAudio_Combine(&combine) != OK) {
        AmbaPrint("ACOMB fail");
    }

    return 0;
}


/**
 * Liveview start
 *
 * @param [in]
 *
 */
int AmpUT_AVEnc_LiveviewStart(UINT32 modeIdx)
{
    AMBA_SENSOR_MODE_INFO_s VinInfo;
    AV_EncModeIdx = modeIdx;


//#define KEEP_ALL_INSTANCES    // When defined, LiveviewStop won't delete all instances, so the next LiveviewStart is equal to change resolution.

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" AmbaUnitTest: Liveview at %s", MWUT_GetInputVideoModeName(AV_EncModeIdx, EncodeSystem));
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
            LiveViewOSMode = AV_VideoEncMgt[AV_EncModeIdx].OSMode;
        }
    }


    // Create Vin instance
    if (AvVideoEncVinA == 0) {
        AMP_VIN_HDLR_CFG_s VinCfg;
        AMP_VIN_LAYOUT_CFG_s Layout[2]; // Dualstream from same vin/vcapwindow
        AMBA_SENSOR_MODE_ID_u Mode = {0};

        memset(&VinCfg, 0x0, sizeof(AMP_VIN_HDLR_CFG_s));
        memset(&Layout, 0x0, sizeof(AMP_VIN_LAYOUT_CFG_s)*2);

        Mode.Data = (EncodeSystem==0)? AV_VideoEncMgt[AV_EncModeIdx].InputMode: AV_VideoEncMgt[AV_EncModeIdx].InputPALMode;
        AmbaSensor_GetModeInfo(AvVinChannel, Mode, &VinInfo);

        AmpVin_GetDefaultCfg(&VinCfg);

#define CONFIG_WHEN_CREATE  // When defined, setup everything when creation. If not defined, configurations can be set after creation and before liveviewStart
#ifdef CONFIG_WHEN_CREATE
        VinCfg.Channel = AvVinChannel;
        VinCfg.Mode = Mode;
        VinCfg.LayoutNumber = 1;
        VinCfg.HwCaptureWindow.Width = AV_VideoEncMgt[AV_EncModeIdx].CaptureWidth;
        VinCfg.HwCaptureWindow.Height = AV_VideoEncMgt[AV_EncModeIdx].CaptureHeight;
        VinCfg.HwCaptureWindow.X = VinInfo.OutputInfo.RecordingPixels.StartX +
            (((VinInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
        VinCfg.HwCaptureWindow.Y = (VinInfo.OutputInfo.RecordingPixels.StartY +
            ((VinInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;
        Layout[0].Width = AV_VideoEncMgt[AV_EncModeIdx].MainWidth;
        Layout[0].Height = AV_VideoEncMgt[AV_EncModeIdx].MainHeight;
        Layout[0].EnableSourceArea = 0; // Get all capture window to main
        Layout[0].DzoomFactorX = INIT_DZOOM_FACTOR;
        Layout[0].DzoomFactorY = INIT_DZOOM_FACTOR;
        Layout[0].DzoomOffsetX = 0;
        Layout[0].DzoomOffsetY = 0;
        Layout[0].MainviewReportRate = AV_VideoEncMgt[AV_EncModeIdx].ReportRate;

        if (AvEncDualHDStream) {
            if (AvEncSecStreamCustomWidth) {
                Layout[1].Width = AvEncSecStreamCustomWidth;
                Layout[1].Height = AvEncSecStreamCustomHeight;
            } else {
                MWUT_InputSetDualHDWindow(AV_VideoEncMgt, AV_EncModeIdx, \
                    &Layout[1].Width, &Layout[1].Height);
            }
        } else {
            if (AvEncSecStreamCustomWidth) {
                Layout[1].Width = AvEncSecStreamCustomWidth;
                Layout[1].Height = AvEncSecStreamCustomHeight;
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
        VinCfg.Layout = Layout;
#endif
        VinCfg.cbEvent = AmpUT_AVEncVinEventCallback;
        VinCfg.cbSwitch= AmpUT_AVEncVinSwitchCallback;

        AmpVin_Create(&VinCfg, &AvVideoEncVinA);
    }

    // Remember frame/field rate for muxers storing frame rate info
    if (EncodeSystem == 0) {
        AV_EncFrameRate = AV_VideoEncMgt[AV_EncModeIdx].TimeScale/AV_VideoEncMgt[AV_EncModeIdx].TickPerPicture;
    } else {
        AV_EncFrameRate = AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL/AV_VideoEncMgt[AV_EncModeIdx].TickPerPicturePAL;
    }

    if (EncodeSystem == 0) {
        AV_EncSecFrameRate = SEC_STREAM_TIMESCALE/SEC_STREAM_TICK;
    } else {
        AV_EncSecFrameRate = SEC_STREAM_TIMESCALE_PAL/SEC_STREAM_TICK_PAL;
    }

    // Create ImgSchdlr instance
    {
        AMBA_IMG_SCHDLR_CFG_s ImgSchdlrCfg;

        AmbaImgSchdlr_GetDefaultCfg(&ImgSchdlrCfg);

        ImgSchdlrCfg.MainViewID = 0; //single channle have one MainView
        ImgSchdlrCfg.Channel = AvVinChannel;
        ImgSchdlrCfg.Vin = AvVideoEncVinA;
        ImgSchdlrCfg.cbEvent = AmpUT_AVEncImgSchdlrCallback;
        if (LiveViewProcMode && LiveViewAlgoMode) {
            ImgSchdlrCfg.VideoProcMode = 1;
        }
        ImgSchdlrCfg.AAAStatSampleRate = AV_VideoEncMgt[AV_EncModeIdx].ReportRate;
        AmbaImgSchdlr_Create(&ImgSchdlrCfg, &AvImgSchdlr);  // One MainViewID (not vin) need one scheduler.
    }


#ifdef CONFIG_SOC_A12
    // Create encMonitor Stream instance
    {
        AMBA_DSP_EVENT_VIDEO_ENC_STATUS_s Stream = {0};

        Stream.StreamId = 0; //Pri
        Stream.ChannelId = 0; //TBD
        AmbaEncMonitor_StreamRegister(Stream, &EncMonitorStrmHdlrPri);

        if (AvEncDualStream || AvEncDualHDStream) {
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
            BrcCfg.CmplxHdlr.GetDayLumaThresCB = AmpUT_AVEnc_GetDayLumaThresholdCB;
            BrcCfg.CmplxHdlr.GetComplexityRangeCB = AmpUT_AVEnc_GetSceneComplexityRangeCB;
            BrcCfg.CmplxHdlr.AdjustQpCB = AmpUT_AVEnc_QpAdjustmentCB;
            BrcCfg.AverageBitrate = (UINT32)AV_VideoEncMgt[AV_EncModeIdx].AverageBitRate*1000*1000;
            BrcCfg.MaxBitrate = (UINT32)AV_VideoEncMgt[AV_EncModeIdx].MaxBitRate*1000*1000;
            BrcCfg.MinBitrate = (UINT32)AV_VideoEncMgt[AV_EncModeIdx].MinBitRate*1000*1000;
            BrcCfg.emonStrmHdlr = EncMonitorStrmHdlrPri;
            BrcCfg.VideoOSMode = LiveViewOSMode;
            BrcCfg.VideoProcMode = LiveViewProcMode;
            BrcCfg.VideoHdrMode = (VinInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE)? 1: 0;
            AmbaEncMonitorBRC_RegisterService(&BrcCfg, &BrcHdlrPri);
        }

        if (AvEncSecSpecH264 && (AvEncDualStream || AvEncDualHDStream) && BrcHdlrSec == NULL) {
            if (AvEncDualHDStream) {
                if (AvEncSecStreamCustomWidth) {
                    //TBD, temp follow MainStrm
                    BrcCfg.AverageBitrate = (UINT32)AV_VideoEncMgt[AV_EncModeIdx].AverageBitRate*1000*1000;
                    BrcCfg.MaxBitrate = (UINT32)AV_VideoEncMgt[AV_EncModeIdx].MaxBitRate*1000*1000;
                    BrcCfg.MinBitrate = (UINT32)AV_VideoEncMgt[AV_EncModeIdx].MinBitRate*1000*1000;
                } else if (AV_VideoEncMgt[AV_EncModeIdx].MainWidth < SEC_STREAM_HD_WIDTH) {
                    BrcCfg.AverageBitrate = (UINT32)AV_VideoEncMgt[AV_EncModeIdx].AverageBitRate*1000*1000;
                    BrcCfg.MaxBitrate = (UINT32)AV_VideoEncMgt[AV_EncModeIdx].MaxBitRate*1000*1000;
                    BrcCfg.MinBitrate = (UINT32)AV_VideoEncMgt[AV_EncModeIdx].MinBitRate*1000*1000;
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
            AqpCfg.AqpCB = AmpUT_AVEnc_AqpPriStream;
            AmbaEncMonitorAQP_RegisterService(&AqpCfg, &AqpHdlrPri);
        }

        if ((AvEncDualStream || AvEncDualHDStream) && AqpHdlrSec == NULL) {
            AqpCfg.AqpCB = AmpUT_AVEnc_AqpSecStream;
            AqpCfg.emonStrmHdlr = EncMonitorStrmHdlrSec;
            AmbaEncMonitorAQP_RegisterService(&AqpCfg, &AqpHdlrSec);
        }
    }
#endif


    // Create video encoder instances
    if (AVEncPri == 0 && AVEncSec == 0) {
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
        EncCfg.cbEvent = AmpUT_AVEncCallback;

#ifdef CONFIG_WHEN_CREATE
        // Assign main layout in single chan
        EncCfg.MainLayout.Width = AV_VideoEncMgt[AV_EncModeIdx].MainWidth;
        EncCfg.MainLayout.Height = AV_VideoEncMgt[AV_EncModeIdx].MainHeight;
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = AvEncPriInteralce; //TBD
        EncCfg.MainTimeScale = (EncodeSystem==0)? AV_VideoEncMgt[AV_EncModeIdx].TimeScale: AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL;
        EncCfg.MainTickPerPicture = (EncodeSystem==0)? AV_VideoEncMgt[AV_EncModeIdx].TickPerPicture: AV_VideoEncMgt[AV_EncModeIdx].TickPerPicturePAL;
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
    #endif
        EncLayer.SourceType = AMP_ENC_SOURCE_VIN;
        EncLayer.Source = AvVideoEncVinA;
        EncLayer.SourceLayoutId = 0;
        EncLayer.EnableSourceArea = 0;  // No source cropping
        EncLayer.EnableTargetArea = 0;  // No target pip
        EncCfg.EventDataReadySkipNum = 0;  // File data ready every frame
        EncCfg.StreamId = AMP_VIDEOENC_STREAM_PRIMARY;

        {
            UINT8 *dspWorkAddr;
            UINT32 dspWorkSize;
            AmpUT_AVEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
            EncCfg.DspWorkBufAddr = dspWorkAddr;
            EncCfg.DspWorkBufSize = dspWorkSize;
        }
 #endif

        if (LiveViewProcMode == 0) {
            if (AV_VideoEncMgt[AV_EncModeIdx].MainWidth > 1920 || \
                AV_VideoEncMgt[AV_EncModeIdx].CaptureWidth > 1920) {
                if (LiveViewOSMode == 0) {
                    AmbaPrint("[UT_Video]Force to use NonOS Mode?? (VcapW %d,MainW %d)", AV_VideoEncMgt[AV_EncModeIdx].MainWidth, AV_VideoEncMgt[AV_EncModeIdx].CaptureWidth);
                }
            }
        }

        EncCfg.LiveViewProcMode = LiveViewProcMode;
        EncCfg.LiveViewAlgoMode = LiveViewAlgoMode;
        EncCfg.LiveViewOSMode = LiveViewOSMode;
        if (VinInfo.HdrInfo.HdrType == AMBA_SENSOR_HDR_TYPE_MULTI_SLICE) {
            EncCfg.LiveViewHdrMode = LiveViewHdrMode = 1;
        } else {
            EncCfg.LiveViewHdrMode = LiveViewHdrMode = 0;
        }

       // Create primary stream handler
        AmpVideoEnc_Create(&EncCfg, &AVEncPri); // Don't have to worry about h.264 spec settings when liveview

#ifndef CONFIG_WHEN_CREATE // An example if a user doesn't want to configure in detail when handler creation, but configure only before liveview start
        {
            AMP_VIDEOENC_MAIN_CFG_s MainCfg = {0};
            AMP_VIDEOENC_LAYER_DESC_s NewPriLayer = {0, 0, 0, AMP_ENC_SOURCE_VIN, 0, 0, {0,0,0,0},{0,0,0,0}};
            AMP_VIN_RUNTIME_CFG_s VinCfg = {0};
            AMBA_SENSOR_MODE_INFO_s VinInfo;
            AMP_VIN_LAYOUT_CFG_s Layout; // Dualstream from same vin/vcapwindow
            AMBA_SENSOR_MODE_ID_u Mode = {0};

            Mode.Data = AV_VideoEncMgt[AV_EncModeIdx].SensorMode;
            AmbaSensor_GetModeInfo(AvVinChannel, Mode, &VinInfo);

            VinCfg.Hdlr = AvVideoEncVinA;
            VinCfg.Mode = Mode;
            VinCfg.LayoutNumber = 1;
            VinCfg.HwCaptureWindow.Width = AV_VideoEncMgt[AV_EncModeIdx].CaptureWidth;
            VinCfg.HwCaptureWindow.Height = AV_VideoEncMgt[AV_EncModeIdx].CaptureHeight;
            VinCfg.HwCaptureWindow.X = VinInfo.OutputInfo.RecordingPixels.StartX +
                (((VinInfo.OutputInfo.RecordingPixels.Width - VinCfg.HwCaptureWindow.Width)/2)&0xFFF8);
            VinCfg.HwCaptureWindow.Y = (VinInfo.OutputInfo.RecordingPixels.StartY +
                ((VinInfo.OutputInfo.RecordingPixels.Height - VinCfg.HwCaptureWindow.Height)/2)) & 0xFFFE;
            Layout.Width = AV_VideoEncMgt[AV_EncModeIdx].MainWidth;
            Layout.Height = AV_VideoEncMgt[AV_EncModeIdx].MainHeight;
            Layout.EnableSourceArea = 0; // Get all capture window to main
            Layout.DzoomFactorX = INIT_DZOOM_FACTOR;
            Layout.DzoomFactorY = INIT_DZOOM_FACTOR;
            Layout.DzoomOffsetX = 0;
            Layout.DzoomOffsetY = 0;
            Layout.MainviewReportRate = AV_VideoEncMgt[EncModeIdx].ReportRate;
            VinCfg.Layout = &Layout;

            MainCfg.Hdlr = AVEncPri;
            MainCfg.MainLayout.LayerNumber = 1;
            MainCfg.MainLayout.Layer = &NewPriLayer;
            MainCfg.MainLayout.Width = AV_VideoEncMgt[AV_EncModeIdx].MainWidth;
            MainCfg.MainLayout.Height = AV_VideoEncMgt[AV_EncModeIdx].MainHeight;
            MainCfg.Interlace = AvEncPriInteralce; //TBD
            MainCfg.MainTickPerPicture = (EncodeSystem==0)? AV_VideoEncMgt[AV_EncModeIdx].TickPerPicture: AV_VideoEncMgt[AV_EncModeIdx].TickPerPicturePAL;
            MainCfg.MainTimeScale = (EncodeSystem==0)? AV_VideoEncMgt[AV_EncModeIdx].TimeScale: AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL;
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
        #endif
            {
                UINT8 *dspWorkAddr;
                UINT32 dspWorkSize;
                AmpUT_AVEnc_DspWork_Calculate(&dspWorkAddr, &dspWorkSize);
                MainCfg.DspWorkBufAddr = dspWorkAddr;
                MainCfg.DspWorkBufSize = dspWorkSize;
            }

            if (LiveViewProcMode == 0) {
                if (AV_VideoEncMgt[AV_EncModeIdx].MainWidth > 1920 || \
                    AV_VideoEncMgt[AV_EncModeIdx].CaptureWidth > 1920)
                    if (LiveViewOSMode == 0) {
                        AmbaPrint("[UT_Video]Force to use NonOS Mode?? (VcapW %d,MainW %d)", AV_VideoEncMgt[AV_EncModeIdx].MainWidth, AV_VideoEncMgt[AV_EncModeIdx].CaptureWidth);
                    }
            }

            MainCfg.LiveViewProcMode = LiveViewProcMode;
            MainCfg.LiveViewAlgoMode = LiveViewAlgoMode;
            MainCfg.LiveViewOSMode = LiveViewOSMode;
            if (VinInfo.HdrIsSupport/* && HdrType==MULTI_SLICE */) {
                MainCfg.LiveViewHdrMode = LiveViewHdrMode = 1;
            } else {
                MainCfg.LiveViewHdrMode = LiveViewHdrMode = 0;
            }
            NewPriLayer.EnableSourceArea = 0;
            NewPriLayer.EnableTargetArea = 0;
            NewPriLayer.LayerId = 0;
            NewPriLayer.SourceType = AMP_ENC_SOURCE_VIN;
            NewPriLayer.Source = AvVideoEncVinA;
            NewPriLayer.SourceLayoutId = 0;
            AmpVideoEnc_ConfigVinMain(1, &VinCfg, 1, &MainCfg);
            AmbaPrint_Flush();
        }
#endif
        // Assign Secondary main layout
        if (AvEncDualHDStream) {
            if (AvEncSecStreamCustomWidth) {
                EncCfg.MainLayout.Width = AvEncSecStreamCustomWidth;
                EncCfg.MainLayout.Height = AvEncSecStreamCustomHeight;
            } else {
                MWUT_InputSetDualHDWindow(AV_VideoEncMgt, AV_EncModeIdx, \
                    &EncCfg.MainLayout.Width, &EncCfg.MainLayout.Height);
            }
        } else {
            if (AvEncSecStreamCustomWidth) {
                EncCfg.MainLayout.Width = AvEncSecStreamCustomWidth;
                EncCfg.MainLayout.Height = AvEncSecStreamCustomHeight;
            } else {
                EncCfg.MainLayout.Width = SEC_STREAM_WIDTH;
                EncCfg.MainLayout.Height = SEC_STREAM_HEIGHT;
            }
        }
        EncCfg.MainLayout.LayerNumber = 1;
        EncCfg.Interlace = AvEncPriInteralce; //TBD
        if (EncodeSystem == 0) {
            EncCfg.MainTickPerPicture = SEC_STREAM_TICK;
            EncCfg.MainTimeScale = SEC_STREAM_TIMESCALE;
        } else {
            EncCfg.MainTickPerPicture = SEC_STREAM_TICK_PAL;
            EncCfg.MainTimeScale = SEC_STREAM_TIMESCALE_PAL;
        }
        EncLayer.SourceType = AMP_ENC_SOURCE_VIN;
        EncLayer.Source = AvVideoEncVinA;
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
    #endif

        // Create secondary stream handler
        AmpVideoEnc_Create(&EncCfg, &AVEncSec); // Don't have to worry about h.264 spec settings when liveview
    }

#ifdef CONFIG_SOC_A9
    if (strcmp(MWUT_GetInputVideoModeName(AV_EncModeIdx, EncodeSystem), "2560X1440  60P\0") == 0) {
        AmbaPrint("Turn OFF Tv");
        AvTvLiveview = 0;
    } else
#endif
    {
        AmbaPrint("Turn %s Tv",AvTvLiveview?"ON":"OFF");
    }

    {
        AmbaPrint("Turn ON LCD");
        AvLCDLiveview = 1;
    }

    // Setup display windows
    AmpUT_AVEnc_DisplayStart();

    //
    // Note: For A9, Bitstream-specific configs can be assigned here (before encode start), or when codec instance creation.
    //       But for A7L/A12 family, it shall be assigned when codec instance creation and before liveview start.
    //
#ifdef CONFIG_SOC_A12
    {
        AMP_VIDEOENC_H264_CFG_s *H264Cfg;
        AMP_VIDEOENC_MJPEG_CFG_s *MjpegCfg;
        AMP_VIDEOENC_H264_HEADER_INFO_s HeaderInfo;
        AMP_VIDEOENC_BITSTREAM_CFG_s BitsCfg;

        memset(&BitsCfg, 0x0, sizeof(AMP_VIDEOENC_BITSTREAM_CFG_s));
        BitsCfg.Rotation = 0;//EncRotation;
        BitsCfg.TimeLapse = 0;//EncTimeLapse;
        BitsCfg.VideoThumbnail = 0;//EncThumbnail;
        BitsCfg.PIVMaxWidth = AV_VideoEncMgt[AV_EncModeIdx].MaxPivWidth;

        // Assign bitstream-specific configs
        if (AVEncPri) {
            if (AvEncPriSpecH264) {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
                H264Cfg = &BitsCfg.Spec.H264Cfg;
                H264Cfg->GopM = AV_VideoEncMgt[AV_EncModeIdx].GopM;
                H264Cfg->GopN = AVENC_GOP_N;
                H264Cfg->GopIDR = AVENC_GOP_IDR;
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
                H264Cfg->Interlace = AvEncPriInteralce;
                H264Cfg->TimeScale = (EncodeSystem == 0)? AV_VideoEncMgt[AV_EncModeIdx].TimeScale: AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL;
                H264Cfg->TickPerPicture = (EncodeSystem == 0)? AV_VideoEncMgt[AV_EncModeIdx].TickPerPicture: AV_VideoEncMgt[AV_EncModeIdx].TickPerPicturePAL;

                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
                H264Cfg->StopMethod = AMP_VIDEOENC_STOP_NEXT_IP;
                HeaderInfo.GopM = H264Cfg->GopM;
                HeaderInfo.Width = AV_VideoEncMgt[AV_EncModeIdx].MainWidth;
                HeaderInfo.Height = AV_VideoEncMgt[AV_EncModeIdx].MainHeight;
                HeaderInfo.Interlace = AvEncPriInteralce;
                HeaderInfo.Rotation = 0;//EncRotation;

                // Use default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = AV_VideoEncMgt[AV_EncModeIdx].BrcMode;
                H264Cfg->BitRateControl.AverageBitrate = (UINT32)(AV_VideoEncMgt[AV_EncModeIdx].AverageBitRate * 1E6);
                if (AV_VideoEncMgt[AV_EncModeIdx].BrcMode == VIDEOENC_SMART_VBR) {
                    H264Cfg->BitRateControl.MaxBitrate = (UINT32)(AV_VideoEncMgt[AV_EncModeIdx].MaxBitRate * 1E6);
                    H264Cfg->BitRateControl.MinBitrate = (UINT32)(AV_VideoEncMgt[AV_EncModeIdx].MinBitRate * 1E6);
                }
                H264Cfg->QualityControl.IBeatMode = 0;//EncIBeat;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                MjpegCfg->QuantMatrixAddr = AV_MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(AVEncPri, &BitsCfg);
        }

        if (AVEncSec) {
            if (AvEncSecSpecH264) {
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
                H264Cfg->Interlace = AvEncPriInteralce;
                if (EncodeSystem == 0) {
                    H264Cfg->TimeScale = SEC_STREAM_TIMESCALE;
                    H264Cfg->TickPerPicture = SEC_STREAM_TICK;
                } else {
                    H264Cfg->TimeScale = SEC_STREAM_TIMESCALE_PAL;
                    H264Cfg->TickPerPicture = SEC_STREAM_TICK_PAL;
                }
                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
                H264Cfg->StopMethod = AMP_VIDEOENC_STOP_NEXT_IP;

                HeaderInfo.GopM = SEC_STREAM_GOP_M;
                if (AvEncDualHDStream) {
                    if (AvEncSecStreamCustomWidth) {
                        HeaderInfo.Width = AvEncSecStreamCustomWidth;
                        HeaderInfo.Height = AvEncSecStreamCustomHeight;
                    } else {
                        MWUT_InputSetDualHDWindow(AV_VideoEncMgt, AV_EncModeIdx, \
                            &HeaderInfo.Width, &HeaderInfo.Height);
                    }
                } else {
                    if (AvEncSecStreamCustomWidth) {
                        HeaderInfo.Width = AvEncSecStreamCustomWidth;
                        HeaderInfo.Height = AvEncSecStreamCustomHeight;
                    } else {
                        HeaderInfo.Width = SEC_STREAM_WIDTH;
                        HeaderInfo.Height = SEC_STREAM_HEIGHT;
                    }
                }
                HeaderInfo.Interlace = AvEncPriInteralce;
                HeaderInfo.Rotation = 0;//EncRotation;

                // Get default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->SPS.level_idc = 42;
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = VIDEOENC_SMART_VBR;
                H264Cfg->BitRateControl.AverageBitrate = (UINT32)(AvEncDualHDStream?6E6:2E6);
                H264Cfg->QualityControl.IBeatMode = 0;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                MjpegCfg->QuantMatrixAddr = AV_MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(AVEncSec, &BitsCfg);
        }
    }
#endif

    return 0;
}

/* create audio pipe */
int AmpUT_AVEnc_AudioCreate(UINT8 PriEncType)
{
    int er;
    AMP_AUDIOENC_HDLR_CFG_s encCfg={0};
    AMBA_AUDIO_AACENC_CONFIG_s aacCfg = { AAC_BS_ADTS, 128000 }; // Encode a directly playable ADTS file
    AMBA_AUDIO_PCM_CONFIG_s pcmCfg = { .BitsPerSample = 16, // Encode a Intel 16bit PCM
                                    .DataFormat = 0,
                                    .FrameSize = 1024 };
    AMBA_AUDIO_ADPCM_CONFIG_s adpcmCfg = { .AdpcmFrameSize = 2048 };
    AMBA_AUDIO_AC3ENC_CONFIG_s ac3Cfg = { .Ac3EncAcmod = 2,
                                        .Ac3EncBitrate = 192000,
                                        .Ac3EncAgcEnable = 0,
                                        .Ac3EncAgcCh2Enable = 0,
                                        .Ac3EncDrcMode = 0,
                                        .Ac3EncLfeEnable = 0,
                                        .Ac3EncLfeFilterEnable = 0,
                                        .Ac3EncTestMode = 0,
                                        .Ac3EncSurroundDelayEnable = 0,
                                        .Ac3EncBsEndian = Audio_BS_Intel };
    AMBA_AUDIO_OPUSENC_CONFIG_s opusCfg = { .Bitrate = 128000,
                                        .FrameSize = (48000*20/1000),
                                        .BitstreamType = OPUS_BS_RTP };
    UINT32 Csize, NCsize;
    UINT32 retInputHdlr, retEncodeHdlr, retEncBuffHdlr;

    // Create Audio encoder object
    AmpAudioEnc_GetDefaultCfg(&encCfg);

    // Encoder setup
    encCfg.SrcChannelMode = 2; // currently audio_lib treat this as channel number
    encCfg.SrcSampleRate = 48000;
    encCfg.DstChannelMode = 2; // currently audio_lib treat this as channel number
    encCfg.DstSampleRate = 48000;
    if (PriEncType == 0) {
        encCfg.EncType = AMBA_AUDIO_AAC;
        encCfg.Spec.AACCfg = aacCfg;
    } else if (PriEncType == 1) {
        encCfg.EncType = AMBA_AUDIO_PCM;
        encCfg.Spec.PCMCfg = pcmCfg;
    } else if (PriEncType == 2) {
        encCfg.EncType = AMBA_AUDIO_AAC_PLUS;
        encCfg.Spec.AACCfg = aacCfg;
    } else if (PriEncType == 3) {
        encCfg.EncType = AMBA_AUDIO_AAC_PLUS_V2;
        encCfg.Spec.AACCfg = aacCfg;
    } else if (PriEncType == 4) {
        encCfg.EncType = AMBA_AUDIO_ADPCM;
        encCfg.Spec.ADPCMCfg = adpcmCfg;
    } else if (PriEncType == 5) {
        encCfg.EncType = AMBA_AUDIO_AC3;
        encCfg.Spec.AC3Cfg = ac3Cfg;
    } else if (PriEncType == 6) {
        encCfg.EncType = AMBA_AUDIO_MPEG;
    } else if (PriEncType == 7) {
        encCfg.EncType = AMBA_AUDIO_OPUS;
        encCfg.Spec.OpusCfg = opusCfg;
    }
    AvAudioEncodeType = PriEncType;

    // Task priority, input task should have higher priority than encode task
    encCfg.EncoderTaskPriority = 15;
    encCfg.EncoderTaskCoreSelection = 1; // single core
    encCfg.InputTaskPriority = 12;
    encCfg.InputTaskCoreSelection = 1; // single core
    encCfg.EventDataReadySkipNum = 0;
    encCfg.FadeInTime = 0;
    encCfg.FadeOutTime = 0;

    // Query working size
    {
        AMBA_AUDIO_IO_CREATE_INFO_s input;
        AMBA_AUDIO_TASK_CREATE_INFO_s enc;
        AMBA_ABU_CREATE_INFO_s abu;

        // Cache need 4_align, NonCache need 8_align
        // audio input
        memset(&input, 0x0, sizeof(AMBA_AUDIO_IO_CREATE_INFO_s));
        input.I2sIndex = 0; // depends on HW design
        input.MaxChNum = 2;
        input.MaxDmaDescNum = 16; // depends chip/project, means more buffer
        input.MaxDmaSize = 1024; // depends chip/project, means more buffer
        input.MaxSampleFreq = 48000;
        Csize = ALIGN_4(AmbaAudio_InputCachedSizeQuery(&input));
        NCsize = ALIGN_8(AmbaAudio_InputNonCachedSizeQuery(&input));

        // audio encode
        memset(&enc, 0x0, sizeof(AMBA_AUDIO_TASK_CREATE_INFO_s));
        enc.MaxSampleFreq = 48000;
        enc.MaxChNum = 2;
        enc.MaxFrameSize = 2048;
        Csize += ALIGN_4(AmbaAudio_EncSizeQuery(&enc));

        memset(&abu, 0x0, sizeof(AMBA_ABU_CREATE_INFO_s));
        abu.MaxSampleFreq = 48000;
        abu.MaxChNum = 2;
        abu.MaxChunkNum = 16; // depends preject, means more buffer
        Csize += ALIGN_4(AmbaAudio_BufferSizeQuery(&abu));

        // Assign working buffer
        if (AvAudworkCacheBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AvAudworkCacheBuf, (void **)&OriAvAudworkCacheBuf, Csize, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for audio working!!");
            }
        }

        if (AvAudworkNCBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_NC_MMPL, (void **)&AvAudworkNCBuf, (void **)&OriAvAudworkNCBuf, NCsize, 32);
            if (er != OK) {
                AmbaPrint("Out of NC memory for audio working!!");
            }
        }

        // Assign callback
        encCfg.cbEvent = AmpUT_AVEncAudioCallback;

        // Assign bitstream/descriptor buffer
        if (AvAudPriBitsBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AvAudPriBitsBuf, (void **)&OriAvAudPriBitsBuf, AUDENC_BISFIFO_SIZE, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for bitsFifo!!");
            }
        }
        encCfg.BitsBufCfg.BitsBufAddr = AvAudPriBitsBuf;
        encCfg.BitsBufCfg.BitsBufSize = AUDENC_BISFIFO_SIZE;

        if (AvAudPriDescBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AvAudPriDescBuf, (void **)&OriAvAudPriDescBuf, AUDENC_DESC_SIZE, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for descFifo!!");
            }
        }

        encCfg.BitsBufCfg.DescBufAddr = AvAudPriDescBuf;
        encCfg.BitsBufCfg.DescBufSize = AUDENC_DESC_SIZE;
        encCfg.BitsBufCfg.BitsRunoutThreshold = AUDENC_BISFIFO_SIZE*80/100; //80%

        AmbaPrint("Bits 0x%X size %d Desc 0x%X size %d", AvAudPriBitsBuf, AUDENC_BISFIFO_SIZE, AvAudPriDescBuf, AUDENC_DESC_SIZE);

        //Create Audio Input, Encode and EncodeBuffer resource
        encCfg.NumInstance = 1;
        AmpUT_AVEnc_Task_Add(AvAudworkCacheBuf, AvAudworkNCBuf, Csize, NCsize, &retInputHdlr, &retEncodeHdlr, &retEncBuffHdlr, AV_AUDIOENC_TASK_ADD_ALL);
        AvAudioEncInstance.InputHdlr = (UINT32 *)retInputHdlr;
        AvAudioEncInstance.EncodeHdlr = (UINT32 *)retEncodeHdlr;
        AvAudioEncInstance.EncBuffHdlr = (UINT32 *)retEncBuffHdlr;

        encCfg.AudioInstance = &AvAudioEncInstance;

        // Assign bitstream-specific configs
        AmpAudioEnc_Create(&encCfg, &AvAudioEncPriHdlr);
    }

    return 0;
}

int AmpUT_AVEnc_create(UINT32 modeIdx,UINT8 PriEncType)
{
    AmpUT_AVEnc_LiveviewStart(modeIdx);
    if(PriEncType == AV_Audio_Calibration) {
        AvAudioEncodeType = PriEncType;
    } else {
        AmpUT_AVEnc_AudioCreate(PriEncType);
    }
    // Register pipeline
    if (PriEncType == AV_Audio_Calibration) {
        AMP_ENC_PIPE_CFG_s pipeCfg;
        // Register pipeline
        AmpEnc_GetDefaultCfg(&pipeCfg);
        //pipeCfg.cbEvent
        pipeCfg.encoder[0] = AVEncPri;
        pipeCfg.numEncoder = 1;

        pipeCfg.cbEvent = AmpUT_AVEncPipeCallback;
        pipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&pipeCfg, &AvEncPipe);

        AmpEnc_Add(AvEncPipe);
    } else {
        AMP_ENC_PIPE_CFG_s pipeCfg;
        // Register pipeline
        AmpEnc_GetDefaultCfg(&pipeCfg);
        //pipeCfg.cbEvent
        pipeCfg.encoder[0] = AvAudioEncPriHdlr;
        pipeCfg.encoder[1] = AVEncPri;
        pipeCfg.encoder[2] = AVEncSec;
        if (AvEncDualStream || AvEncDualHDStream ) {
            pipeCfg.numEncoder = 3;
        } else {
            pipeCfg.numEncoder = 2;
        }
        pipeCfg.cbEvent = AmpUT_AVEncPipeCallback;
        pipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&pipeCfg, &AvEncPipe);

        AmpEnc_Add(AvEncPipe);
    }
    AmpEnc_StartLiveview(AvEncPipe, 0);

    return 0;
}

int AmpUT_AVEnc_EncodeStart(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;
    AMP_ENC_BITSBUFFER_CFG_s  BitsBufCfg;
    AMP_VIDEOENC_BITSTREAM_CFG_s CurrentCfg;

    if (AvStatus != STATUS_AV_LIVEVIEW)
        return -1;

    AmbaPrint(" ========================================================= ");
    AmbaPrint(" VideoEncode : %s", MWUT_GetInputVideoModeName(AV_EncModeIdx, EncodeSystem));
    AmbaPrint(" VideoEncode : CaptureWin[%d, %d]", AV_VideoEncMgt[AV_EncModeIdx].CaptureWidth, AV_VideoEncMgt[AV_EncModeIdx].CaptureHeight);
    AmbaPrint(" VideoEncode : PriStream EncWin [%dx%d]", AV_VideoEncMgt[AV_EncModeIdx].MainWidth, AV_VideoEncMgt[AV_EncModeIdx].MainHeight);
    if (AvEncPriSpecH264) {
        AmpVideoEnc_GetBitstreamConfig(AVEncPri, &CurrentCfg);
        AmbaPrint(" VideoEncode : AvgBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.AverageBitrate/1000000);
        AmbaPrint(" VideoEncode : MaxBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.MaxBitrate/1000000);
        AmbaPrint(" VideoEncode : MinBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.MinBitrate/1000000);
        AmbaPrint(" VideoEncode : PriStream H264 GopM   %u", CurrentCfg.Spec.H264Cfg.GopM);
        AmbaPrint(" VideoEncode : PriStream H264 GopN   %u", CurrentCfg.Spec.H264Cfg.GopN);
        AmbaPrint(" VideoEncode : PriStream H264 GopIDR %u", CurrentCfg.Spec.H264Cfg.GopIDR);
    } else {
        AmbaPrint(" VideoEncode : PriStream MJpeg");
    }
    if (AvEncDualStream || AvEncDualHDStream) {
        UINT16 SecEncWidth, SecEncHeight;
        if (AvEncDualHDStream) {
            if (AvEncSecStreamCustomWidth) {
                SecEncWidth = AvEncSecStreamCustomWidth;
                SecEncHeight = AvEncSecStreamCustomHeight;
            } else {
                MWUT_InputSetDualHDWindow(AV_VideoEncMgt, AV_EncModeIdx, \
                    &SecEncWidth, &SecEncHeight);
            }
        } else {
            if (AvEncSecStreamCustomWidth) {
                SecEncWidth = AvEncSecStreamCustomWidth;
                SecEncHeight = AvEncSecStreamCustomHeight;
            } else {
                SecEncWidth = SEC_STREAM_WIDTH;
                SecEncHeight = SEC_STREAM_HEIGHT;
            }
        }
        AmbaPrint(" VideoEncode : SecStream EncWin [%dx%d]", SecEncWidth, SecEncHeight);
    }
    if (AvEncSecSpecH264 && (AvEncDualStream || AvEncDualHDStream)) {
        AmpVideoEnc_GetBitstreamConfig(AVEncSec, &CurrentCfg);
        AmbaPrint(" VideoEncode : AvgBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.AverageBitrate/1000000);
        AmbaPrint(" VideoEncode : MaxBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.MaxBitrate/1000000);
        AmbaPrint(" VideoEncode : MinBrate %uMbyte", CurrentCfg.Spec.H264Cfg.BitRateControl.MinBitrate/1000000);
        AmbaPrint(" VideoEncode : SecStream H264 GopM   %u", CurrentCfg.Spec.H264Cfg.GopM);
        AmbaPrint(" VideoEncode : SecStream H264 GopN   %u", CurrentCfg.Spec.H264Cfg.GopN);
        AmbaPrint(" VideoEncode : SecStream H264 GopIDR %u", CurrentCfg.Spec.H264Cfg.GopIDR);
    } else {
        if (AvEncDualStream || AvEncDualHDStream) {
            AmbaPrint(" VideoEncode : SecStream MJpeg");
        }
    }
    AmbaPrint(" =========================================================");

#ifdef CONFIG_SOC_A12
    { // retrieve current QP settings for QP_adjustment
        if (AvEncPriSpecH264) {
            VideoEncPriQpIsZero = 0;
            AmpVideoEnc_GetBitstreamConfig(AVEncPri, &CurrentCfg);
            VideoEncPriCurrQpMinI = CurrentCfg.Spec.H264Cfg.QPControl.QpMinI;
            VideoEncPriCurrQpMaxI = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxI;
            VideoEncPriCurrQpMinP = CurrentCfg.Spec.H264Cfg.QPControl.QpMinP;
            VideoEncPriCurrQpMaxP = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxP;
            VideoEncPriCurrQpMinB = CurrentCfg.Spec.H264Cfg.QPControl.QpMinB;
            VideoEncPriCurrQpMaxB = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxB;
        }

        if (AvEncSecSpecH264 && (AvEncDualStream || AvEncDualHDStream)) {
            VideoEncSecQpIsZero = 0;
            AmpVideoEnc_GetBitstreamConfig(AVEncSec, &CurrentCfg);
            VideoEncSecCurrQpMinI = CurrentCfg.Spec.H264Cfg.QPControl.QpMinI;
            VideoEncSecCurrQpMaxI = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxI;
            VideoEncSecCurrQpMinP = CurrentCfg.Spec.H264Cfg.QPControl.QpMinP;
            VideoEncSecCurrQpMaxP = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxP;
            VideoEncSecCurrQpMinB = CurrentCfg.Spec.H264Cfg.QPControl.QpMinB;
            VideoEncSecCurrQpMaxB = CurrentCfg.Spec.H264Cfg.QPControl.QpMaxB;
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
        // Assign bitstream-specific configs
        if (AVEncPri) {
            if (AvEncPriSpecH264) {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_AVCC;
                H264Cfg = &BitsCfg.Spec.H264Cfg;
                H264Cfg->GopM = AV_VideoEncMgt[AV_EncModeIdx].GopM;
                H264Cfg->GopN = AVENC_GOP_N;
                H264Cfg->GopIDR = AVENC_GOP_IDR;
                H264Cfg->GopHierarchical = 0;
                H264Cfg->Cabac = 1;
                H264Cfg->QualityControl.LoopFilterEnable = 1;
                H264Cfg->QualityControl.LoopFilterAlpha = 0;
                H264Cfg->QualityControl.LoopFilterBeta = 0;
                H264Cfg->StartFromBFrame = (H264Cfg->GopM > 1);
                H264Cfg->Interlace = AvEncPriInteralce;
                H264Cfg->TimeScale = (EncodeSystem == 0)? AV_VideoEncMgt[AV_EncModeIdx].TimeScale: AV_VideoEncMgt[AV_EncModeIdx].TimeScalePAL;
                H264Cfg->TickPerPicture = (EncodeSystem == 0)? AV_VideoEncMgt[AV_EncModeIdx].TickPerPicture: AV_VideoEncMgt[AV_EncModeIdx].TickPerPicturePAL;
                H264Cfg->AuDelimiterType = 1;
                H264Cfg->QualityLevel = 0x94; // Suggested value: 1080P: 0x94, 1008i: 0x9B, 720P: 0x9A
                H264Cfg->StopMethod = AMP_VIDEOENC_STOP_NEXT_IP;
                HeaderInfo.GopM = H264Cfg->GopM;
                HeaderInfo.Width = AV_VideoEncMgt[AV_EncModeIdx].MainWidth;
                HeaderInfo.Height = AV_VideoEncMgt[AV_EncModeIdx].MainHeight;
                HeaderInfo.Interlace = AvEncPriInteralce;

                // Use default SPS/VUI
                AmpVideoEnc_GetDefaultH264Header(&HeaderInfo, &H264Cfg->SPS, &H264Cfg->VUI);
                H264Cfg->VUI.video_full_range_flag = 0; //follow old spec.

                // Bitrate control
                H264Cfg->BitRateControl.BrcMode = AV_VideoEncMgt[AV_EncModeIdx].BrcMode;
                H264Cfg->BitRateControl.AverageBitrate = (UINT32)(AV_VideoEncMgt[AV_EncModeIdx].AverageBitRate * 1E6);
                if (AV_VideoEncMgt[AV_EncModeIdx].BrcMode == VIDEOENC_SMART_VBR) {
                    H264Cfg->BitRateControl.MaxBitrate = (UINT32)(AV_VideoEncMgt[AV_EncModeIdx].MaxBitRate * 1E6);
                    H264Cfg->BitRateControl.MinBitrate = (UINT32)(AV_VideoEncMgt[AV_EncModeIdx].MinBitRate * 1E6);
                }
                H264Cfg->QualityControl.IBeatMode = 0;//EncIBeat;
            } else {
                BitsCfg.StreamSpec = AMP_VIDEOENC_CODER_MJPEG;
                MjpegCfg = &BitsCfg.Spec.MjpgCfg;

                MjpegCfg->FrameRateDivisionFactor = 1;
                MjpegCfg->QuantMatrixAddr = AV_MjpegQuantMatrix;
            }
            AmpVideoEnc_SetBitstreamConfig(AVEncPri, &BitsCfg);
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
        AmbaPrint("H.264 Bits 0x%x size %d Desc 0x%x size %d", AV_H264BitsBuf, BITSFIFO_SIZE, AV_H264DescBuf, DESC_SIZE);
        AmbaPrint("MJPEG Bits 0x%x size %d Desc 0x%x size %d", AV_MjpgBitsBuf, BITSFIFO_SIZE, AV_MjpgDescBuf, DESC_SIZE);
    }

    memset(&BitsBufCfg, 0x0, sizeof(AMP_ENC_BITSBUFFER_CFG_s));
    if (AV_VirtualPriFifoHdlr == NULL) {
        //Set BaseInformation only once
        if (AvEncPriSpecH264) {
            BitsBufCfg.BitsBufAddr = AV_H264BitsBuf;
            BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
            BitsBufCfg.DescBufAddr = AV_H264DescBuf;
            BitsBufCfg.DescBufSize = DESC_SIZE;
            BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        } else {
            BitsBufCfg.BitsBufAddr = AV_MjpgBitsBuf;
            BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
            BitsBufCfg.DescBufAddr = AV_MjpgDescBuf;
            BitsBufCfg.DescBufSize = DESC_SIZE;
            BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
        }
        AmpVideoEnc_SetBitstreamBuffer(AVEncPri, &BitsBufCfg);

        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = AVEncPri;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = DESC_SIZE/sizeof(AMBA_DSP_EVENT_ENC_PIC_READY_s);;
        fifoDefCfg.cbEvent = AmpUT_AVEnc_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &AV_VirtualPriFifoHdlr);
    } else {
        //Following are follow previous
        if (AvEncPriSpecH264) {
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
        AmpVideoEnc_SetBitstreamBuffer(AVEncPri, &BitsBufCfg);
        AmpFifo_EraseAll(AV_VirtualPriFifoHdlr);
    }

    if (AV_VirtualSecFifoHdlr == NULL) {
        if (AVEncSec) {
            if (AvEncSecSpecH264) {
                BitsBufCfg.BitsBufAddr = AV_H264BitsBuf;
                BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
                BitsBufCfg.DescBufAddr = AV_H264DescBuf;
                BitsBufCfg.DescBufSize = DESC_SIZE;
                BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            } else {
                BitsBufCfg.BitsBufAddr = AV_MjpgBitsBuf;
                BitsBufCfg.BitsBufSize = BITSFIFO_SIZE;
                BitsBufCfg.DescBufAddr = AV_MjpgDescBuf;
                BitsBufCfg.DescBufSize = DESC_SIZE;
                BitsBufCfg.BitsRunoutThreshold = BITSFIFO_SIZE - 4*1024*1024; // leave 4MB
            }
            AmpVideoEnc_SetBitstreamBuffer(AVEncSec, &BitsBufCfg);
        }

        // create a virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = AVEncSec;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = DESC_SIZE/sizeof(AMBA_DSP_EVENT_ENC_PIC_READY_s);;
        fifoDefCfg.cbEvent = AmpUT_AVEnc_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &AV_VirtualSecFifoHdlr);
    } else {
        //Following are follow previous
        if (AVEncSec) {
            if (AvEncSecSpecH264) {
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
            AmpVideoEnc_SetBitstreamBuffer(AVEncSec, &BitsBufCfg);
            AmpFifo_EraseAll(AV_VirtualSecFifoHdlr);
        }
    }

    if (AvAudioEncPriVirtualFifoHdlr == NULL) {
        // create Primary virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = AvAudioEncPriHdlr;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = 1024;
        fifoDefCfg.cbEvent = AmpUT_AVEnc_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &AvAudioEncPriVirtualFifoHdlr);
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

        if (AvEncSecSpecH264 && (AvEncDualStream || AvEncDualHDStream)) {
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

    AmpEnc_StartRecord(AvEncPipe, 0);
    AvStatus = STATUS_AV_ENCODE;
    return 0;
}

int AmpUT_AVEnc_EncStop(void)
{
    if (AvStatus != STATUS_AV_ENCODE && AvStatus != STATUS_AV_PAUSE)
        return -1;

    AmpEnc_StopRecord(AvEncPipe, 0);
#ifdef CONFIG_SOC_A12
    if (EncMonitorEnable || EncMonitorAQPEnable) {
        AmbaEncMonitor_EnableStreamHandler(EncMonitorStrmHdlrPri, 0);
        if (EncMonitorEnable) {
            AmbaEncMonitorBRC_EnableService(BrcHdlrPri, 0);
        }
        if (EncMonitorAQPEnable) {
            AmbaEncMonitorAQP_EnableService(AqpHdlrPri, 0);
        }

        if (AvEncSecSpecH264 && (AvEncDualStream || AvEncDualHDStream)) {
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
    AvStatus = STATUS_AV_LIVEVIEW;

    return 0;
}


int AmpUT_AVEnc_EncPause(void)
{
    if (AvStatus != STATUS_AV_ENCODE)
        return -1;

    AmpEnc_PauseRecord(AvEncPipe, 0);
#ifdef CONFIG_SOC_A12
    if (EncMonitorEnable || EncMonitorAQPEnable) {
        if (EncMonitorEnable) {
            AmbaEncMonitorBRC_EnableService(BrcHdlrPri, 0);
        }
        if (EncMonitorAQPEnable) {
            AmbaEncMonitorAQP_EnableService(AqpHdlrPri, 0);
        }

        if (AvEncSecSpecH264 && (AvEncDualStream || AvEncDualHDStream)) {
            if (EncMonitorEnable) {
                AmbaEncMonitorBRC_EnableService(BrcHdlrSec, 0);
            }
            if (EncMonitorAQPEnable) {
                AmbaEncMonitorAQP_EnableService(AqpHdlrSec, 0);
            }
        }
    }
#endif
    AvStatus = STATUS_AV_PAUSE;

    return 0;
}

int AmpUT_AVEnc_EncResume(void)
{
    if (AvStatus != STATUS_AV_PAUSE)
        return -1;

    AmpEnc_ResumeRecord(AvEncPipe, 0);
#ifdef CONFIG_SOC_A12
    if (EncMonitorEnable || EncMonitorAQPEnable) {
        if (EncMonitorEnable) {
            AmbaEncMonitorBRC_EnableService(BrcHdlrPri, 1);
        }
        if (EncMonitorAQPEnable) {
            AmbaEncMonitorAQP_EnableService(AqpHdlrPri, 1);
        }

        if (AvEncSecSpecH264 && (AvEncDualStream || AvEncDualHDStream)) {
            if (EncMonitorEnable) {
                AmbaEncMonitorBRC_EnableService(BrcHdlrSec, 1);
            }
            if (EncMonitorAQPEnable) {
                AmbaEncMonitorAQP_EnableService(AqpHdlrSec, 1);
            }
        }
    }
#endif
    AvStatus = STATUS_AV_ENCODE;
    return 0;
}

/* delete pipe */
int AmpUT_AVEnc_Delete(void)
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

        if (AvEncSecSpecH264 && (AvEncDualStream || AvEncDualHDStream)) {
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

    if (AvImgSchdlr) {
        AmbaImgSchdlr_Enable(AvImgSchdlr, 0);
    }
    Amba_Img_VIn_Invalid(AVEncPri, (UINT32 *)NULL);

#ifdef CONFIG_SOC_A12
    if (AvEnc_EisAlgoEnable) {
        AmbaEis_Inactive();
        AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_DISABLE, 0, 0, 0);
        AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_SET_FOCAL_LENGTH, 360, 0, 0);
        AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_INIT, 0, 0, 0);
        AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_MODE_INIT, 0, 0, 0);
        AmbaImg_Proc_Cmd(MW_IP_AMBA_EIS_ENABLE, 0, 0, 0);
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
    AmpEnc_StopLiveview(AvEncPipe, Flag);

#ifndef KEEP_ALL_INSTANCES
    if (AvEncPipe) {
        AmpEnc_Delete(AvEncPipe);
        AvEncPipe = NULL;
    }
    if (AvVideoEncVinA) {
        AmpVin_Delete(AvVideoEncVinA);
        AvVideoEncVinA = NULL;
    }
    if (AvImgSchdlr) {
        AmbaImgSchdlr_Delete(AvImgSchdlr);
        AvImgSchdlr = NULL;
    }
    if (AVEncPri) {
        AmpVideoEnc_Delete(AVEncPri);
        AVEncPri = NULL;
    }
    if (AVEncSec) {
        AmpVideoEnc_Delete(AVEncSec);
        AVEncSec = NULL;
    }

    if(AvAudioEncPriHdlr) {
        AmpAudioEnc_Delete(AvAudioEncPriHdlr);
        AvAudioEncPriHdlr = NULL;
    }

    if (AV_VirtualPriFifoHdlr) {
        AmpFifo_Delete(AV_VirtualPriFifoHdlr);
        AV_VirtualPriFifoHdlr = NULL;
    }
    if (AV_VirtualSecFifoHdlr) {
        AmpFifo_Delete(AV_VirtualSecFifoHdlr);
        AV_VirtualSecFifoHdlr = NULL;
    }

    if (AmpResource_GetVoutSeamlessEnable() == 0) {
        if (AvLCDLiveview) {
            AmpUT_Display_Stop(0);
            AmpUT_Display_Window_Delete(0);
        }
    }

    if (AvTvLiveview) {
        AmpUT_Display_Stop(1);
        AmpUT_Display_Window_Delete(1);
    }
#endif

    /* detach audio handler */
    if (AvAudioEncInstance.InputHdlr && \
        AvAudioEncInstance.EncodeHdlr && \
        AvAudioEncInstance.EncBuffHdlr) {
        AMBA_AUDIO_COMBINE_INFO_s Combine;
        memset(&Combine, 0x0, sizeof(AMBA_AUDIO_COMBINE_INFO_s));
        Combine.pAbu = AvAudioEncInstance.EncBuffHdlr;
        Combine.pSrcApu = AvAudioEncInstance.InputHdlr;
        Combine.pDstApu = AvAudioEncInstance.EncodeHdlr;
        AmbaAudio_Detach(&Combine);
    }

    /* delete audio handler */
    if (AvAudioEncInstance.InputHdlr) {
        AmbaAudio_InputDelete(AvAudioEncInstance.InputHdlr);
        AvAudioEncInstance.InputHdlr = NULL;
    }
    if (AvAudioEncInstance.EncodeHdlr) {
        AmbaAudio_EncDelete(AvAudioEncInstance.EncodeHdlr);
        AvAudioEncInstance.EncodeHdlr = NULL;
    }
    if (AvAudioEncInstance.EncBuffHdlr) {
        AmbaAudio_BufferDelete(AvAudioEncInstance.EncBuffHdlr);
        AvAudioEncInstance.EncBuffHdlr = NULL;
    }

    if (AvAudioEncodeType != AV_Audio_Calibration) {
        /* Free audio working memory */
        if (AvAudworkCacheBuf) {
            AmbaKAL_BytePoolFree((void *)OriAvAudworkCacheBuf);
            AvAudworkCacheBuf = NULL;
        }
        if (AvAudworkNCBuf) {
            AmbaKAL_BytePoolFree((void *)OriAvAudworkNCBuf);
            AvAudworkNCBuf = NULL;
        }

        /* Free bits */
        if (AvAudPriBitsBuf) {
            AmbaKAL_BytePoolFree((void *)OriAvAudPriBitsBuf);
            AvAudPriBitsBuf = NULL;
        }
        if (AvAudPriDescBuf) {
            AmbaKAL_BytePoolFree((void *)OriAvAudPriDescBuf);
            AvAudPriDescBuf = NULL;
        }
    }

    return 0;
}

int AmpUT_AVEncTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_AVEncTest cmd: %s", argv[1]);

    if (strcmp(argv[1],"init") == 0) {
        int ss = 0, dd = 0;
        if (argc < 3) {
            UINT8 i = 0;
            AmbaPrint("Usage: t videoenc init [sensorID][LcdID]");
            AmbaPrint("               sensorID:");
            for (i = 0; i<INPUT_DEVICE_NUM; i++) {
                AmbaPrint("                        %2d -- %s", i, MWUT_GetInputDeviceName(i));
            }
            AmbaPrint("               LcdID: 0 -- WDF9648W");
            return -1;
        }
        ss = atoi(argv[2]);
        if (argc == 4) {
            dd = atoi(argv[3]);
        }
        AmpUT_AVEnc_Init(ss, dd);
        AvStatus = STATUS_AV_IDLE;
    } else if ((strcmp(argv[1],"create") == 0) || (strcmp(argv[1],"cre") == 0)) {
        if (AvStatus != STATUS_AV_IDLE && \
            AvStatus != STATUS_AV_ENCODE) {
            AmbaPrint("[AVEnc] Non Init");
            return -1;
        }
        if (argc < 4) {
            AmbaPrint("Usage: t avenc liveviewstart [modeId]");
            if (AV_VideoEncMgt == NULL) {
                AmbaPrintColor(RED, "Please hook sensor first!!!");
                return -1;
            }
            AmbaPrint("[%s] mode_id: ", MWUT_GetInputDeviceName(InputDeviceId));
            MWUT_InputVideoModePrintOutAll(AV_VideoEncMgt);
            return -1;
        } else {
            UINT32 priEncType = atoi(argv[3]);
            UINT32 tt = atoi(argv[2]);

            if (AV_VideoEncMgt[tt].ForbidMode) {
                AmbaPrintColor(RED, "Not Support this Mode (%u)", tt);
                return -1;
            }
            AmpUT_AVEnc_create(tt,priEncType);
            AvStatus = STATUS_AV_LIVEVIEW;
        }
    } else if ((strcmp(argv[1],"encstart") == 0) || (strcmp(argv[1],"enst") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (AvStatus == STATUS_AV_ENCODE) {
            AmbaPrint("[AVEnc] encode start during Encoding");
            return -1;
        }

        AmbaPrint("Encode for %d milliseconds", tt);
        AmpUT_AVEnc_EncodeStart();
        if (tt != 0) {
        while (tt) {
            AmbaKAL_TaskSleep(1);
            tt--;
            if (tt == 0) {
                AmpUT_AVEnc_EncStop();
            }
            if (AvStatus == STATUS_AV_LIVEVIEW)
                tt = 0;
            }
        }
    } else if ((strcmp(argv[1],"encstop") == 0) || (strcmp(argv[1],"ensp") == 0)) {
        if (AvStatus != STATUS_AV_ENCODE && AvStatus != STATUS_AV_PAUSE) {
            AmbaPrint("[AVEnc] encode stop during IDLE");
            return -1;
        }

        AmpUT_AVEnc_EncStop();
    } else if ((strcmp(argv[1],"encpause") == 0) || (strcmp(argv[1],"enps") == 0)) {
        if (AvStatus != STATUS_AV_ENCODE) {
            AmbaPrint("[AVEnc] encode Pause during IDLE");
            return -1;
        }

        AmpUT_AVEnc_EncPause();
    } else if ((strcmp(argv[1],"encresume") == 0) || (strcmp(argv[1],"enrs") == 0)) {
        if (AvStatus != STATUS_AV_PAUSE) {
            AmbaPrint("[AVEnc] encode Resume during BUSY/IDLE");
            return -1;
        }

        AmpUT_AVEnc_EncResume();
    } else if ((strcmp(argv[1],"delete") == 0) || (strcmp(argv[1],"del") == 0)) {
        if (AvStatus == STATUS_AV_ENCODE) {
            AmbaPrint("[AVEnc] delete during Encodeing !!");
            return -1;
        }
        if (AvStatus == STATUS_AV_IDLE) {
            AmbaPrint("[AVEnc] delete during IDLE !!");
            return -1;
        }
        /*  Make sure output file is closed */
        if (AvVoutputPriFile || AvVoutputSecFile || AvAOutputPriFile) {
            AmbaPrint("MUXER not IDLE !!!!");
            return -1;
        }

        AmpUT_AVEnc_Delete();
        AvStatus = STATUS_AV_IDLE;
    } else if ((strcmp(argv[1],"3a") == 0)) {
        UINT8 Is3aEnable = atoi(argv[2]);
        UINT8 i;
        UINT32 ChannelCount = 0;
        AMBA_3A_OP_INFO_s AaaOpInfo = {0};

        AmbaImg_Proc_Cmd(MW_IP_GET_TOTAL_CH_COUNT, (UINT32)&ChannelCount,0, 0);

        if (Is3aEnable == 0) {
            AvEnc3AEnable = 0;
            AaaOpInfo.AeOp = DISABLE;
            AaaOpInfo.AfOp = DISABLE;
            AaaOpInfo.AwbOp = DISABLE;
            AaaOpInfo.AdjOp = DISABLE;
        } else if (Is3aEnable == 1) {
            AvEnc3AEnable = 1;
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
            AmbaPrint("ChNo[%u] 3A %s: ae:%u af:%u awb:%u adj:%u", i, AvEnc3AEnable? "Enable": "Disable", \
                AaaOpInfo.AeOp, AaaOpInfo.AfOp, AaaOpInfo.AwbOp, AaaOpInfo.AdjOp);
        }
    } else if ((strcmp(argv[1],"ta") == 0)) {
        UINT8 Enable = atoi(argv[2]);
        AvEncTAEnable = Enable;
        AmbaPrint("TA : %s, be sure to use HybridAlgoMode", AvEncTAEnable?"Enable":"Disable");
    } else if ((strcmp(argv[1],"logmuxer") == 0)) {
        AvLogMuxer = (UINT8)atoi(argv[2]);
        AmbaPrint("LogMuxer %s",(AvLogMuxer)? "Enable": "Disable");
    } else if (strcmp(argv[1],"dumpskip") == 0) {
        UINT8 v1 = atoi(argv[2]);

        AmbaPrint("DumpSkipFlag is 0x%X", v1);
        AvEncodeDumpSkip = v1;
    } else if (strcmp(argv[1],"secwin") == 0) {
        UINT16 Width = atoi(argv[2]);
        UINT16 Height = atoi(argv[3]);

        if (Width > 1920) {
            AmbaPrint("SecStrmWin %dX%d does not support width > 1920", Width, Height);
        } else {
            AmbaPrint("SecStrmWin %dX%d", Width, Height);
            AvEncSecStreamCustomWidth = Width;
            AvEncSecStreamCustomHeight = Height;
        }
    } else if ((strcmp(argv[1],"dual") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (AvStatus != STATUS_AV_IDLE) {
            AmbaPrint("Please set dualstream before liveview start");
            return -1;
        }

        AmbaPrint("DualStream: %s", tt?"ENABLE":"DISABLE");
        AvEncDualStream = tt;
    } else if ((strcmp(argv[1],"dualhd") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (AvStatus != STATUS_AV_IDLE) {
            AmbaPrint("Please set dualstream before liveview start");
            return -1;
        }

        AmbaPrint("DualHDStream: %s", tt?"ENABLE":"DISABLE");
        AvEncDualHDStream = tt;
        if (AvEncDualHDStream) {
            AvTvLiveview = 0;
            AmbaPrint("Tv Disable!");
        }
    } else if (strcmp(argv[1],"audiodbg") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t avenc audiodbg [lvl]");
            return -1;
        } else {
            UINT32 debugLevel = (UINT32)atoi(argv[2]);
            AmbaAudio_SetDebugLevel(debugLevel);
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
    } else if (strcmp(argv[1],"lvproc") == 0 || strcmp(argv[1],"liveviewproc") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t avenc liveviewproc [proc][Algo/OS]");
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
    } else if (strcmp(argv[1],"suspend") == 0) {
        DspSuspendEnable = atoi(argv[2]);
        AmbaPrint("%s Dsp suspend in LiveveiwStop !", DspSuspendEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"wirelessmode") == 0) {
        WirelessModeEnable = atoi(argv[2]);
        AmbaPrint("%s Enter Wireless mode in LiveveiwStop !", WirelessModeEnable? "Enable": "Disable");
    } else if (strcmp(argv[1],"EncStatus") == 0) {
        if (AvStatus == STATUS_AV_ENCODE) {
            AmbaPrint("===== Encode Status =====");
            AmbaPrint("PriStrmFrm : %u", AV_encPriTotalFrames);
            AmbaPrint("SecStrmFrm : %u", AV_encSecTotalFrames);
            AmbaPrint("AudioFrm   : %u", AvAencPriTotalFrames);
            AmbaPrint("=========================");
        } else {
            AmbaPrint("===== Encode Idle =====");
        }
#ifdef CONFIG_SOC_A12
    } else if (strcmp(argv[1],"eis") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t avenc eis [cmd] ...");
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
                AmbaPrint("Usage: t avenc eis hook [EisID] [SensorID]");
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
                AmbaPrint("Usage: t avenc eis devinfo [SensorID]");
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
                AmbaPrint("Usage: t avenc eis deven [Enable][SensorID]");
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
                AmbaPrint("Usage: t avenc eis devstatus [Type][SensorID]");
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
        } else if (strcmp(argv[2],"algoen")) {
            UINT8 Enable = atoi(argv[3]);

            if (argc < 4) {
                AmbaPrint("Usage: t avenc eis algoen [Enable]");
                return 0;
            }

            AvEnc_EisAlgoEnable = Enable;
            AmbaPrint("%s EIS Algo", (Enable)?"Enable":"Disable", AvEnc_EisAlgoEnable);
        }
#endif
#ifdef CONFIG_SOC_A12
    } else if (strcmp(argv[1],"encmonitor") == 0 || strcmp(argv[1],"emon") == 0) {
        EncMonitorEnable = atoi(argv[2]);
        AmbaPrint("Encode Monitor %s !", EncMonitorEnable?"Enable":"Disable");
    } else if (strcmp(argv[1],"encmonitoraqp") == 0 || strcmp(argv[1],"emonaqp") == 0) {
        EncMonitorAQPEnable = atoi(argv[2]);
        AmbaPrint("Encode Monitor AQP %s !", EncMonitorAQPEnable?"Enable":"Disable");
#endif
    } else {
        AmbaPrint("Usage: t avenc init|create|encstart|encstop");
        AmbaPrint("       init: init all");
        AmbaPrint("       create [VideoModeID] [AudioEncType]: create video and audio instance");
        AmbaPrint("       encstart [millisecond]: recording for N milliseconds");
        AmbaPrint("                               N = 0 will do continuous encode");
        AmbaPrint("       encstop: stop recording");
        AmbaPrint("       encpause: pause recording");
        AmbaPrint("       encresume: resume recording");
        AmbaPrint("       delete: delete av instance");
}

return 0;
}

int AmpUT_AVEncTestAdd(void)
{
    AmbaPrint("Adding AmpUT_AVEnc");

    AmbaTest_RegisterCommand("avenc", AmpUT_AVEncTest);

    return AMP_OK;
}
