/**
 * @file src/app/sample/sspunittest/src/AmbaSSPSystemInfo.h
 *
 * SSP UnitTest System Information
 *
 * History:
 *    2013/02/25 - [Peter Weng] created file
 *
 * Copyright (C) 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef _AMBA_SSP_SYSTEM_INFO_H_
#define _AMBA_SSP_SYSTEM_INFO_H_

#include <stdio.h>
#include "AmbaDataType.h"
#include "AmbaKAL.h"
#include "AmbaDSP.h"
#include "AmbaDSP_Event.h"
#include "AmbaDSP_EventInfo.h"
#include "AmbaDSP_VideoEnc.h"
#include <AmbaFS.h>
#include <AmbaKAL.h>
#include "AmbaPrint.h"
#include "AmbaCache.h"
#include "AmbaUtility.h"
#include "AmbaSensor.h"
#include "AmbaSensor_IMX117.h"
#include "AmbaRTSL_MMU_Def.h"

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MAX2(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MIN2(a,b) ((a)<(b)?(a):(b))
#define MAX3(a,b,c) (MAX((a),MAX(b,c)))
#define MIN3(a,b,c) (MIN(MIN(a,b),c))
#define ALIGN_128(x)  (((x) + 127) & 0xFFFFFF80)
#define ALIGN_64(x) (((x) + 63) & 0xFFFFFFC0)
#define ALIGN_32(x) (((x) + 31) & 0xFFFFFFE0)
#define ALIGN_16(x) (((x) + 15) & 0xFFFFFFF0)
#define ALIGN_8(x)  (((x) + 7) & 0xFFFFFFF8)
#define ALIGN_4(x)  (((x) + 3) & 0xFFFFFFFC)
#define ALIGN_2(x)  (((x) + 1) & 0xFFFFFFFE)
#define CLIP(a, max, min) ((a) > (max)) ? (max) : (((a) < (min)) ? (min) : (a))
#define TRUNCATE_16(x)  ((x) & 0xFFFFFFF0))
#define TRUNCATE_64(x)  ((x) & 0xFFFFFFC0))
#define COMPRESSED_RAW_WIDTH(cols)    (ALIGN_32((cols*27)>>5))

#define MAX_DSP_EVENT_HANDLER (1)
#define MAX_DSP_ENC_STREAM_NUM (4)

#define MAX_STILL_AE_INFO_NUM (3)

/* SSP MuxTask and Queue */
extern UINT8 SSPSysH264DataCbInit;
extern UINT8 SSPSysStillCbInit;
extern UINT8 SSPSysMuxTaskInit;
extern AMBA_KAL_TASK_t SSPSysMuxTask;
extern char *SSPSysMuxStack;
extern AMBA_KAL_MSG_QUEUE_t SSPSysMuxMsgQueue;
#define MAX_SSP_SYS_MUX_MSG_NUM  256
typedef struct _SSP_SYS_MUX_MSG_S_ {
    UINT32 Event;
    union {
        UINT32 DataInfo[2];
        AMBA_DSP_EVENT_ENC_PIC_READY_s  PicInfo;
        AMBA_DSP_RAW_BUF_s              RawInfo;
        AMBA_DSP_YUV_IMG_BUF_s          YuvInfo;
    } Data;
} SSP_SYS_MUX_MSG_S;
extern SSP_SYS_MUX_MSG_S *SSPSysMuxMsgQPool;
extern void SSPSys_MuxTask(UINT32 info);
#define MSG_SSPSYS_MUX_H264_DATA_READY      0x0000001
#define MSG_SSPSYS_MUX_JPEG_DATA_READY      0x0000002
#define MSG_SSPSYS_MUX_H264_ENCODE_STOP     0x0000003
#define MSG_SSPSYS_MUX_STILL_YUV_DATA_READY 0x0000004
#define MSG_SSPSYS_MUX_STILL_RAW_DATA_READY 0x0000005

#define SSP_VIDEO_PIPE_EXPRESS_BASIC    0
#define SSP_VIDEO_PIPE_EXPRESS_OS       1
#define SSP_VIDEO_PIPE_EXPRESS_YUV      2
#define SSP_VIDEO_PIPE_HYBRID_FAST      3
#define SSP_VIDEO_PIPE_HYBRID_LISO      4
#define SSP_VIDEO_PIPE_HYBRID_HISO      5
#define SSP_VIDEO_PIPE_HYBRID_OTF       6
#define SSP_VIDEO_PIPE_HYBRID_LISO_HDR  7


typedef struct _AMBA_SSP_SYSTEM_SETTING_s_ {
    /* Clock */
    UINT32  AudioClk;
    UINT32  CoreClk;
    UINT32  IDspClk;
    UINT32  CortexClk;
    UINT32  DdrClk;

    /* DSP working buffer */
    UINT32 DspWorkSize;
    UINT8* DspWorkAddr;
    const UINT32 DspLogSize;
    UINT8* DspLogAddr;

    /* IK Config */
    UINT8 VideoCtxNum;
    UINT8 VideoCfgNum;
    UINT8 StillCtxNum;
    UINT8 StillCfgNum;
    UINT8 DecodeCtxNum;
    UINT8 DecodeCfgNum;
    UINT32 IKWorkSize;
    UINT8* IKWorkAddr;

    /* BS/Desc Config */
    UINT32 JpegBSSize;
    UINT8* JpegBSAddr;
    UINT32 JpegDescSize;
    UINT8* JpegDescAddr;
    UINT32 H264BSSize;
    UINT8* H264BSAddr;
    UINT8* H264BSStartAddr;

    /* DSP mode */
    UINT8 CurBootMode;
    UINT8 EventCfgInit;
    UINT8 VideoPipeline;

    /* vin */
    UINT16 VinCapWidth;
    UINT16 VinCapHeight;
    UINT16 VinCapX;
    UINT16 VinCapY;

    /* Vout setting */
    UINT16 PrevWidthA;
    UINT16 PrevHeightA;
    UINT16 PrevActWidthA;
    UINT16 PrevActHeightA;
    UINT16 PrevWidthB;
    UINT16 PrevHeightB;
    UINT16 PrevActWidthB;
    UINT16 PrevActHeightB;
    AMBA_DSP_FRAME_RATE_s FratePrevA;
    AMBA_DSP_FRAME_RATE_s FratePrevB;
    AMBA_DSP_ROTATE_FLIP_e PrevRotateA;
    AMBA_DSP_ROTATE_FLIP_e PrevRotateB;
    UINT8 LcdDevId;
    UINT8 LcdDevMode;
    UINT8 TVDevId;
    UINT8 HDMIDevMode;

    /* Encode setting */
    UINT8 EncodeSrc;
    UINT8 EncodePurpose;
    UINT8 EncodeStillAlgoMode;
    UINT8 EncodeStillRawCapCtrl;
    UINT8 EncodeStillRawCapNum;
    UINT8 EncodeInputFmt[8];
    UINT8 EncodeInputRawDataBits;
    UINT8 EncodeInputRawBayer;
    UINT16 EncodeInputWidth[8];
    UINT16 EncodeInputHeight[8];
    UINT8* EncodeInputAddr[8];
    UINT8 EncodeInputBufferDepth[8];
    char EncodeInputName[8][64];
    char EncodeOutputName[8][64];
    UINT8 EncodeOutputMainFmt[8];
    UINT8 EncodePreviewAEnable;
    UINT8 EncodePreviewBEnable;

    UINT8 StrmNum;
    AMBA_DSP_FRAME_RATE_s StrmFrate[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmFtm[MAX_DSP_ENC_STREAM_NUM];
    AMBA_DSP_ROTATE_FLIP_e StrmRot[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmFrateDivisor[MAX_DSP_ENC_STREAM_NUM];
    UINT16 MainWidth[MAX_DSP_ENC_STREAM_NUM];
    UINT16 MainHeight[MAX_DSP_ENC_STREAM_NUM];
    UINT16 ScrnWidth;
    UINT16 ScrnHeight;
    UINT16 ScrnWidthAct;
    UINT16 ScrnHeightAct;
    UINT16 ThmbWidth;
    UINT16 ThmbHeight;
    UINT16 ThmbWidthAct;
    UINT16 ThmbHeightAct;
    UINT16 QvLcdWidth;
    UINT16 QvLcdHeight;
    UINT16 QvHDMIWidth;
    UINT16 QvHDMIHeight;
    UINT8 MainJpegQLevel;
    UINT8 ThmbJpegQLevel;
    UINT8 ScrnJpegQLevel;
    UINT8 JpegReEncodeLoop;

    /* H264 config */
    UINT8 StrmProIDC[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmLvIDC[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmCabac[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmGopHeir[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmGopPHeir[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmGopM[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmGopN[MAX_DSP_ENC_STREAM_NUM];
    UINT32 StrmGopIDR[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmNumPRef[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmNumBRef[MAX_DSP_ENC_STREAM_NUM];
    AMBA_DSP_BITRATE_CTRL_e StrmBrateCtrl[MAX_DSP_ENC_STREAM_NUM];
    UINT32 StrmQLevel[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmVBRComplx[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmVBRPercent[MAX_DSP_ENC_STREAM_NUM];
    UINT16 StrmVBRMinRatio[MAX_DSP_ENC_STREAM_NUM];
    UINT16 StrmVBRMaxRatio[MAX_DSP_ENC_STREAM_NUM];
    UINT32 StrmBitRate[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmQpMinI[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmQpMaxI[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmQpMinP[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmQpMaxP[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmQpMinB[MAX_DSP_ENC_STREAM_NUM];
    UINT8 StrmQpMaxB[MAX_DSP_ENC_STREAM_NUM];
    AMBA_DSP_H264ENC_STOP_PAUSE_OPTION_e StrmStopOption[MAX_DSP_ENC_STREAM_NUM];

    UINT8* EncodeOutputLumaBufAddr;
    UINT8* EncodeOutputChromaBufAddr;
    UINT8* EncodeOutputScrnLumaBufAddr;
    UINT8* EncodeOutputScrnChromaBufAddr;
    UINT8* EncodeOutputThmbLumaBufAddr;
    UINT8* EncodeOutputThmbChromaBufAddr;

    /* Resource */
    UINT8  DisablePiv:1;
    UINT8  DisableH264:1;
    UINT8  DisableMJPEG:1;
    UINT8  Disable2ndStream:1;
    UINT8  DisableHDPreview:1;
    UINT8  EnableSqueezeYuvBuffer:3;
    UINT16 MaxPivWidth;
    UINT16 MaxPivHeight;
    UINT16 MaxPivThumbWidth;
    UINT16 MaxPivThumbHeight;

    /* Sensor */
    UINT8 SensorID;
    UINT8 SensorReadoutMode;
    UINT8 SensorShutterType;
} AMBA_SSP_SYSTEM_SETTING_s;

#pragma pack(1)
typedef struct _NHNT_HEADER_s_ {
    char Signature[4];
    UINT64 Version:8;
    UINT64 StreamType:8;
    UINT64 ObjectTypeIndication:8;
    UINT64 Reserved:16;
    UINT64 BufferSizeDB:24;
    UINT32 AvgBitRate;
    UINT32 MaxBitRate;
    UINT32 TimeStampResolution;
} NHNT_HEADER_s;
#pragma pack()

#pragma pack(1)
typedef struct _NHNT_SAMPLE_HEADER_s_ {
    UINT32 DataSize:24;
    UINT32 RandomAccessPoint:1;
    UINT32 AuStartFlag:1;
    UINT32 AuEndFlag:1;
    UINT32 Reserved:1;
    UINT32 FrameType:4;
    UINT32 FileOffset;
    UINT32 CompositionTimeStamp;
    UINT32 DecodingTimeStamp;
} NHNT_SAMPLE_HEADER_s;
#pragma pack()

typedef struct _FORMAT_USER_DATA_s_ {
    UINT32 nIdrInterval;
    UINT32 nTimeScale;
    UINT32 nTickPerPicture;
    UINT32 nN;
    UINT32 nM;
    UINT32 nVideoWidth;
    UINT32 nVideoHeight;
} FORMAT_USER_DATA_s;

extern AMBA_SSP_SYSTEM_SETTING_s AmbaSSPSystemInfo;
extern UINT8 SSPSysInfoJpegQTable[3][128];
extern void SSPSysInfo_initJpegDqt(UINT8 *qTable, int Quality);

extern UINT8 RawCaptureDoneNumber;
extern UINT8 UnProcessRawNumber;
extern UINT8 UnProcessYuvNumber;

extern char SSPSensorName[16][16];
extern char SSPLcdName[16][16];
extern char SSPHDMIName[16][16];

extern float SSPStillShutterTime[MAX_STILL_AE_INFO_NUM];
extern float SSPStillAgcGain[MAX_STILL_AE_INFO_NUM];

//extern int AmbaDSP_Img3aTransferCFAAaaStatData(void *In);
//static AMBA_DSP_EVENT_HANDLER_f CFATransHandler[MAX_DSP_EVENT_HANDLER] = {
//        [0] = AmbaDSP_Img3aTransferCFAAaaStatData,
//};
//
//extern int AmbaDSP_Img3aTransferRGBAaaStatData(void *In);
//static AMBA_DSP_EVENT_HANDLER_f RGBTransHandler[MAX_DSP_EVENT_HANDLER] = {
//        [0] = AmbaDSP_Img3aTransferRGBAaaStatData,
//};


extern AMBA_DSP_EVENT_HANDLER_f RgbHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f CfaHandler[MAX_DSP_EVENT_HANDLER];

extern AMBA_DSP_EVENT_HANDLER_f RawCfaStatHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f RawRgbStatHandler[MAX_DSP_EVENT_HANDLER];

extern AMBA_DSP_EVENT_HANDLER_f H264DataRdyHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f JpgDataRdyHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f MJpgDataRdyHandler[MAX_DSP_EVENT_HANDLER];

extern AMBA_DSP_EVENT_HANDLER_f VcapLiveviewRawHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f VcapLiveviewLcdHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f VcapLiveviewTvHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f VcapYuvHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f Vcap2ndYuvHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f Vcap3rdYuvHandler[MAX_DSP_EVENT_HANDLER];

extern AMBA_DSP_EVENT_HANDLER_f ScapRawCapStartHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f ScapRawHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f ScapCleanRawHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f ScapYuvHandler[MAX_DSP_EVENT_HANDLER];

extern AMBA_DSP_EVENT_HANDLER_f ScapHdrHandler[MAX_DSP_EVENT_HANDLER];

extern AMBA_DSP_EVENT_HANDLER_f JpgDecStatHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f JpgDecYuvHandler[MAX_DSP_EVENT_HANDLER];

extern AMBA_DSP_EVENT_HANDLER_f H264DecStatHandler[MAX_DSP_EVENT_HANDLER];

extern AMBA_DSP_EVENT_HANDLER_f H264EncStartHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f H264EncPauseHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f H264EncResumeHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f H264EncStopHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f EisStatusHandler[MAX_DSP_EVENT_HANDLER];
extern AMBA_DSP_EVENT_HANDLER_f DualFovVcapRawReadyHandler[MAX_DSP_EVENT_HANDLER];

extern int SSPSysInfo_ScapRawCapStartHandler(void *EventInfo);
extern int SSPSysInfo_ScapRawDataRdyHandler(void *EventInfo);
extern int SSPSysInfo_ScapYuvDataRdyHandler(void *EventInfo);
extern int SSPSysInfo_JpegDataRdyHandler(void *EventInfo);
extern int SSPSysInfo_H264DataRdyHandler(void *EventInfo);
extern int SSPSysInfo_H264EncodeStopHandler(void *EventInfo);

extern AMBA_DSP_VIDEO_ENC_STREAM_CONFIG_s SSPSysInfo_VideoEncStrmCfg[MAX_DSP_ENC_STREAM_NUM];
extern AMBA_DSP_VIDEO_ENC_START_CONFIG_u SSPSysInfo_VideoEncStartCfg[MAX_DSP_ENC_STREAM_NUM];

#endif /* _AMBA_SSP_SYSTEM_INFO_H_ */
