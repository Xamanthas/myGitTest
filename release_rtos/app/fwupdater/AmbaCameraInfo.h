/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaCameraInfo.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions and Constands for Camera Information Management
 *
 *  @History        ::
 *      Date        Name        Comments
 *      02/18/2013  W.Shi       Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_CAMERA_INFO_H_
#define _AMBA_CAMERA_INFO_H_


#include "AmbaDSP_StillCapture.h"
#include "AmbaUSB.h"

typedef enum _AMBA_CAMERA_OPMODE_e_ {
#ifndef _IMX122_
    AMBA_CAMERA_OPMODE_STILL_CAPTURE = 0,
    AMBA_CAMERA_OPMODE_MOVIE_RECORD,
    AMBA_CAMERA_OPMODE_VIDEO_TUNING,
    AMBA_CAMERA_OPMODE_PLAYBACK,
    AMBA_CAMERA_OPMODE_TRANSCODE,

    AMBA_NUM_CAMERA_OPMODE                                  /* Total number of Camera operation modes */
#else   /*  Support IMX122 sensor only */
    AMBA_CAMERA_OPMODE_STILL_CAPTURE = 0,
    AMBA_CAMERA_OPMODE_MOVIE_RECORD,
    AMBA_CAMERA_OPMODE_PLAYBACK,
    AMBA_CAMERA_OPMODE_TRANSCODE,

    AMBA_NUM_CAMERA_OPMODE,                                  /* Total number of Camera operation modes */
    AMBA_CAMERA_OPMODE_VIDEO_TUNING
#endif
} AMBA_CAMERA_OPMODE_e;

typedef enum _AMBA_PHOTO_SIZE_e_ {
    AMBA_PHOTO_17M_4800x3600 = 0,
    AMBA_PHOTO_12M_4000x3000,
    AMBA_PHOTO_8M_3264x2448,
    AMBA_PHOTO_5M_2592x1944,
    AMBA_PHOTO_3M_2048x1536,
    AMBA_PHOTO_XGA_1024x768,
    AMBA_PHOTO_VGA_640x480,
    AMBA_PHOTO_13M_4800x2700,
    AMBA_PHOTO_9M_3968x2232,
    AMBA_PHOTO_2M_1920x1080,

    AMBA_NUM_PHOTO_SIZE                                     /* Total Number of Still PHOTO sizes */
} AMBA_PHOTO_SIZE_e;

typedef struct _AMBA_PHOTO_INFO_s_ {
    INT8    Size;                                           /* Photo Format */
    UINT8   Quality;                                        /* Photo Quality */
    UINT8   FocusMode;                                      /* Photo Focus Mode */
    UINT8   ExposureMode;
    UINT8   Metering;
    UINT8   WhiteBalance;
} AMBA_PHOTO_INFO_s;

typedef enum _AMBA_MOVIE_FORMAT_e_ {
    AMBA_MOVIE_4K2KP30 = 0,
    AMBA_MOVIE_4K2KP24,
    AMBA_MOVIE_1520P60,
    AMBA_MOVIE_1080P120,
    AMBA_MOVIE_1080P60,
    AMBA_MOVIE_1080P30,
    AMBA_MOVIE_720P240,
    AMBA_MOVIE_720P60,
    AMBA_MOVIE_960P200,
    AMBA_MOVIE_WVGAP300,
    AMBA_MOVIE_DUAL1080P60,
    AMBA_MOVIE_DUAL1080P30,
    AMBA_MOVIE_DUAL720P60,
    AMBA_MOVIE_4K2KP2,
    AMBA_MOVIE_4096P25,
    AMBA_MOVIE_SUPERVIEW,
    AMBA_MOVIE_SUPERVIEW_4K,
    AMBA_MOVIE_12M_PES_TEST,
    AMBA_MOVIE_MULTI,           /* multi fov preview */
    AMBA_MOVIE_MULTI_FOV_ENC,   /* multi fov encode */
    AMBA_MOVIE_WARP_ROTATION,

    AMBA_NUM_MOVIE_FORMAT                                   /* Total Number of Video Formats */
} AMBA_MOVIE_FORMAT_e;

typedef enum _AMBA_VIDEO_TUNING_FORMAT_e_ {
    AMBA_4K2K_LIVEVIEW_ONLY = 0,
    AMBA_VIDEO_RAW_CAPTURE,
    AMBA_VIDEO_ITUNER,
    AMBA_4K2K_MCTF_LIVEVIEW,

    AMBA_VIDEO_TUNING_FORMAT
} AMBA_VIDEO_TUNING_FORMAT_e;

typedef enum _AMBA_XCODE_FORMAT_e_ {
    AMBA_XCODE_720P = 0,
    AMBA_XCODE_WVGA,
    AMBA_XCODE_WQVGA,

    AMBA_NUM_XCODE_FORMAT                                   /* Total Number of Xcode Video Formats */
} AMBA_XCODE_FORMAT_e;

typedef struct _AMBA_AUDIO_SETTING_s_ {
    UINT32  SampleRate;
    UINT32  Volume;
    UINT32  SampleSize;
    UINT32  Channels;
    UINT32  BitRate;
} AMBA_AUDIO_CFG_s;

typedef struct _AMBA_CPATURE_SETTING_s_ {
    UINT8 EnableQuickview;
    UINT8 EnableBackGroundProc;
    UINT8 EnableStillR2r;
} AMBA_CAPTURE_SETTING_s;

typedef struct _AMBA_USER_SETTING_s_ {
    char                        CameraName[12];
    AMBA_PHOTO_INFO_s           PhotoInfo;                         /* Photo Information */
    AMBA_MOVIE_FORMAT_e         MovieFormat;                       /* Movie Recording Format */
    AMBA_VIDEO_TUNING_FORMAT_e  VideoTuningFormat;                 /* Video tuning Foramt */
    AMBA_XCODE_FORMAT_e         XcodeFormat;                       /* Xcode Recording Format */
    AMBA_AUDIO_CFG_s            AudioCfg;
    AMBA_CAPTURE_SETTING_s      SingleCap;
    AMBA_CAPTURE_SETTING_s      BurstCap;
    UINT8                       EnableBurstCapture;
    UINT16                      TimeLapsCaptureInterval;           /* time laps capture interval in ms. 0 - disable */
    UINT8                       EnableVideoBlend;
    UINT8                       UncompressedRawCap;
    UINT8                       EnableVideoPbFade;
    UINT32                      ZoomFactor;
    INT8                        CurOpMode;                         /* For user preference */
    AMBA_USB_DEVICE_CLASS_e     UsbClass;
    UINT8                       VerticalFlip;                      /* Configure sensor vertical flip */
    AMBA_DSP_STILL_OP_MODE_e    PivOpMode;
    UINT16                      MaxHisoWidth;
    UINT16                      MaxHisoHeight;
    UINT8                       EnableDefBlc;
} AMBA_USER_SETTING_s;

/*---------------------------------------------------------------------------*\
 * Defined in AmbaCameraInfo.c
\*---------------------------------------------------------------------------*/
extern const AMBA_USER_SETTING_s  AmbaUserSettingDefault;
extern AMBA_USER_SETTING_s        AmbaUserSetting;

#endif /* _AMBA_CAMERA_INFO_H_ */
