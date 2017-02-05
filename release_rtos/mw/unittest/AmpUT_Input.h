 /**
  * @file mw/unittestapp/AmpUT_Input.h
  *
  * Unit test header
  *
  * History:
  *    2014/12/16 - [Wisdom Hung] created file
  *
  * Copyright (C) 2014, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef AMPUTINPUT_H_
#define AMPUTINPUT_H_

#include <stdio.h>
#include <string.h>

#define MAX_VIN_NUM 2               // Max Vin number

#define SEC_STREAM_MAX_WIDTH   1920 // Secondary codec Max output width
#define SEC_STREAM_MAX_HEIGHT  1080 // Secondary codec Max output height
#define SEC_STREAM_WIDTH   720      // Secondary codec output width
#define SEC_STREAM_HEIGHT  400      // Secondary codec output height
#define SEC_STREAM_TIMESCALE 30000  // Secondary codec frame rate time scale
#define SEC_STREAM_TICK 1001        // Secondary codec frame rate tick per frame
#define SEC_STREAM_TIMESCALE_PAL 25 // Secondary codec frame rate time scale in PAL mode
#define SEC_STREAM_TICK_PAL 1       // Secondary codec frame rate tick per frame in PAL mode
#define SEC_STREAM_GOP_M   1        // Secondary codec I-frame distance
#define SEC_STREAM_GOP_N   8        // Secondary codec P-frame distance
#define SEC_STREAM_GOP_IDR 8        // Secondary codec IDR-frame distance
#define SEC_STREAM_HD_WIDTH 1280    // Secondary codec HD resolution
#define SEC_STREAM_HD_HEIGHT 720    // Secondary codec HD resolution
#define SEC_STREAM_HD_BRATE_AVG 8   // Secondary codec HD Avg. BitRate in Mbps
#define SEC_STREAM_HD_BRATE_MAX 9   // Secondary codec HD Max. BitRate in Mbps
#define SEC_STREAM_HD_BRATE_MIN 6   // Secondary codec HD Min. BitRate in Mbps
#define SEC_STREAM_BRATE_AVG 4      // Secondary codec Avg. BitRate in Mbps
#define SEC_STREAM_BRATE_MAX 5      // Secondary codec Max. BitRate in Mbps
#define SEC_STREAM_BRATE_MIN 3      // Secondary codec Min. BitRate in Mbps

typedef enum _INPUT_DEVICE_e_ {
    SENSOR_OV4689 = 0,
    SENSOR_IMX117,
    SENSOR_OV2710,
    SENSOR_OV10823,
    SENSOR_IMX206,
    SENSOR_AR0330_PARALLEL,
    SENSOR_AR0230,
    SENSOR_OV9750,
    SENSOR_MN34120,
    SENSOR_B5_OV4689,
    SENSOR_IMX290,
    SENSOR_MN34222,
    SENSOR_IMX377,
    SENSOR_B5_OV9750,
    INPUT_DEVICE_NUM
} INPUT_DEVICE_e;

typedef enum _INPUT_ENC_MODE_e_ {
    FHD_AR16_9_P30 = 0,
    FHD_AR16_9_P60,
    FHD_AR16_9_P120,
    FHD_AR16_9_HDR_P30,
    FHD_AR16_9_HDR_P60,
    HD_AR16_9_P30,      /* 5 */
    HD_AR16_9_P60,
    HD_AR16_9_P120,
    HD_AR16_9_P240,
    HD_AR16_9_HDR_P30,
    HD_AR16_9_HDR_P60,  /* 10 */
    WQHD_AR16_9_P30,
    WQHD_AR16_9_P60,
    UHD_AR16_9_P30,
    UHD_AR16_9_P60,
    FHD_AR4_3_P30,      /* 15 */
    FHD_AR4_3_P60,
    FHD_AR4_3_P120,
    FHD_AR4_3_HDR_P30,
    FHD_AR4_3_HDR_P60,
    HD_AR4_3_P30,       /* 20 */
    HD_AR4_3_P60,
    HD_AR4_3_P120,
    HD_AR4_3_P240,
    HD_AR4_3_HDR_P30,
    HD_AR4_3_HDR_P60,   /* 25 */
    WQHD_AR4_3_P30,
    WQHD_AR4_3_P60,
    UHD_AR4_3_P30,
    UHD_AR4_3_P60,
    FHD_AR16_9_CFA_HDR_P30, /* 30 */
    FHD_AR16_9_CFA_HDR_P60,
    FHD_AR4_3_CFA_HDR_P30,
    FHD_AR4_3_CFA_HDR_P60,
    HD_AR16_9_CFA_HDR_P30,
    HD_AR16_9_CFA_HDR_P60, /* 35 */
    HD_AR4_3_CFA_HDR_P30,
    HD_AR4_3_CFA_HDR_P60,
    WQHD_AR16_9_HDR_P30,
    WQHD_AR16_9_HDR_P60,
    ENC_VIDEO_MODE_NUM,     /* 40 */
    STILL_AR4_3 = 100,
    STILL_AR16_9,
    STILL_AR4_3_1,
    STILL_AR16_9_1,
    ENC_STILL_MODE_NUM
} INPUT_ENC_MODE_e;

// video encode limitation table
typedef struct _INPUT_ENC_LIMIT_s {
    UINT8 H264Encode;
    UINT8 MJPGEncode;
    UINT8 PIP30PYuv;
    UINT8 PIP30PEncode;
    UINT8 HFlip;
    UINT8 Rotata90;
    UINT8 Rotata90VFlip;
    UINT8 HFlipVFlip;
    UINT8 VFlip;
    UINT8 Rotata90HFlipVFlip;
    UINT8 Rotata90HFlip;
    UINT8 Stamp;
    UINT8 DualHD;
    UINT8 EisSupport;
} INPUT_ENC_LIMIT_s;

// video input mode management table
typedef struct _INPUT_ENC_MGT_s {
    UINT32 InputMode;      // Input mode
    UINT32 InputPALMode;   // Input PAL mode
    UINT32 InputStillMode;   // Input Still mode
    UINT16 CaptureWidth;    // Vin capture window width
    UINT16 CaptureHeight;   // Vin capture window height
    UINT16 MainWidth;       // Main window width
    UINT16 MainHeight;      // Main window height

    UINT16 StillCaptureWidth;  // Still capture window width
    UINT16 StillCaptureHeight; // Still capture window width
    UINT16 StillMainWidth;     // Still Main window width
    UINT16 StillMainHeight;    // Still Main window width
    UINT16 ScrnWidth;       // Screenail window width
    UINT16 ScrnHeight;      // Screenail window Height
    UINT16 ScrnWidthAct;    // Screenail window act width
    UINT16 ScrnHeightAct;   // Screenail window act Height
    UINT16 ThmWidth;        // Thumbnail window width
    UINT16 ThmHeight;       // Thumbnail window height
    UINT16 ThmWidthAct;     // Thumbnail window act width
    UINT16 ThmHeightAct;    // Thumbnail window act height
    UINT16 MaxPivWidth;     // Max PIV interpolation width, height will follow AR

    UINT32 TimeScale;       // Time scale of frame rate
    UINT32 TickPerPicture;  // Tick per frame
    UINT32 TimeScalePAL;       // Time scale of frame rate in PAL mode
    UINT32 TickPerPicturePAL;  // Tick per frame in PAL mode
    UINT8  Interlace;       // 0: progressive 1: interlace
    UINT16 AspectRatio;     // Main window aspect ratio
    UINT8 GopM;             // P frame distance
    UINT8 DualValid;        // This mode can do dual stream or not
    UINT8 BrcMode;          // Bitrate control mode
    UINT8 MaxBitRate;       // Maximum bitrate
    UINT8 MinBitRate;       // Minimum bitrate
    UINT8 AverageBitRate;   // Average bitrate
    UINT8 ReportRate;       // how many frame to report 3A stat
    UINT8 OSMode;           // defauting OverSamplingMode or not
    char Name[32];          // Name of this index
    char Desc[64];          // Description of this index
    INPUT_ENC_LIMIT_s limit[3];      // encode liit
    UINT8 ForbidMode:1;       // Indicate this Mode is Forbid to enter
    UINT8 PALModeOnly:1;      // Indicate this Mode is Forbid to use NTSC mode for DualStream
    UINT8 DualStrmUptoHD:1;   // Indicate this Mode is can suport second stream upto HD resolution
    UINT8 Reserved:5;
} INPUT_ENC_MGT_s;

/**
 *  @brief Brief description (optional)
 *
 *  Detailed description *
 *  @param[in] AMBA_DSP_CHANNEL_ID
 *  @param[in] Vin_SensorID
 *
 *  @return INPUT_ENC_MGT_s
 */
extern INPUT_ENC_MGT_s *RegisterMWUT_Sensor_Driver(AMBA_DSP_CHANNEL_ID_u Chan, int InputID);

/**
 *  @brief Brief description (optional)
 *
 *  Detailed description *
 *  @param[in] AMBA_DSP_CHANNEL_ID
 *  @param[in] Vin_YUVID
 *
 *  @return INPUT_ENC_MGT_s
 */
extern INPUT_ENC_MGT_s *RegisterMWUT_YUV_Driver(AMBA_DSP_CHANNEL_ID_u Chan, int YUVID);

extern char* MWUT_GetInputDeviceName(UINT16 index);
extern void MWUT_InputVideoModePrintOutAll(INPUT_ENC_MGT_s *encMgt);
extern void MWUT_InputStillModePrintOutAll(INPUT_ENC_MGT_s *encMgt);
extern char* MWUT_GetInputVideoModeName(UINT16 index, UINT8 system);
extern char* MWUT_GetInputStillModeName(UINT16 index, UINT8 system);
extern void MWUT_InputSetDualHDWindow(INPUT_ENC_MGT_s *encMgt, UINT8 mode, UINT16 *outWidth, UINT16 *outHeight);

#endif /* AMPUT_INPUT_H_ */
