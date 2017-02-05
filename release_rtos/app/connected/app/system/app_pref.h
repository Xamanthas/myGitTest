 /**
  * @file app/connected/app/system/app_pref.h
  *
  * Header of Application Preference
  *
  * History:
  *    2013/08/16 - [Martin Lai] created file
  *    2013/12/27 - [Hsunying Huang] modified
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef APP_SYS_USR_PREF_H_
#define APP_SYS_USR_PREF_H_

#include <framework/appdefines.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * User preference enums
 ************************************************************************/
typedef enum _APP_PREF_SYSTEM_MODE_e_ {
    APP_MODE_NONE = 0,
    APP_MODE_ENC,
    APP_MODE_DEC
} APP_PREF_SYSTEM_MODE_e;

typedef enum _APP_PREF_SELF_TIMER_e_ {
    SELF_TIMER_OFF = 0,
    SELF_TIMER_3S  = 3,
    SELF_TIMER_5S  = 5,
    SELF_TIMER_10S = 10
} APP_PREF_SELF_TIMER_e;

typedef enum _APP_PREF_STREAMING_STATUS_e_ {
    STREAMING_OFF = 0,
    STREAMING_ON
} APP_PREF_STREAMING_STATUS_e;

typedef enum _APP_PREF_STREAM_TYPE_e_ {
    STREAM_TYPE_OFF = 0,
    STREAM_TYPE_RTSP,
    STREAM_TYPE_MJPEG,
    STREAM_TYPE_UVC
} APP_PREF_STREAM_TYPE_e;

typedef enum _APP_PREF_DZOOM_e_ {
    DZOOM_OFF = 0,
    DZOOM_4X,
    DZOOM_16X,
    DZOOM_120X
} APP_PREF_DZOOM_e;

typedef enum _APP_PREF_TIME_STAMP_e_ {
    STAMP_OFF = 0,
    STAMP_DATE,
    STAMP_TIME,
    STAMP_DATE_TIME
} APP_PREF_TIME_STAMP_e;

typedef enum _APP_PREF_PHOTO_TIME_LAPSE_e_ {
    PHOTO_TIME_LAPSE_OFF = 0,
    PHOTO_TIME_LAPSE_500MS = 1,
    PHOTO_TIME_LAPSE_1S = 2,
    PHOTO_TIME_LAPSE_5S = 10
} APP_PREF_PHOTO_TIME_LAPSE_e;

typedef enum _APP_PREF_VIDOE_PLAY_OPT_e_ {
    PB_OPT_VIDEO_PLAY_ONE = 0,
    PB_OPT_VIDEO_PLAY_ALL,
    PB_OPT_VIDEO_REPEAT_ONE,
    PB_OPT_VIDEO_REPEAT_ALL
} APP_PREF_VIDOE_PLAY_OPT_e;

typedef enum _APP_PREF_PHOTO_PLAY_OPT_e_ {
    PB_OPT_PHOTO_REPEAT_OFF,
    PB_OPT_PHOTO_REPEAT_ON
} APP_PREF_PHOTO_PLAY_OPT_e;

typedef enum _APP_PREF_PHOTO_SLIDESHOW_EFFECT_e_ {
    SLIDESHOW_EFFECT_FADING = 0,
    SLIDESHOW_EFFECT_FLYING
} APP_PREF_PHOTO_SLIDESHOW_EFFECT_e;

typedef enum _APP_PREF_SOUND_PLAY_OPT_e_ {
    PB_OPT_SOUND_PLAY_ONE = 0,
    PB_OPT_SOUND_PLAY_ALL,
    PB_OPT_SOUND_REPEAT_ONE,
    PB_OPT_SOUND_REPEAT_ALL
} APP_PREF_SOUND_PLAY_OPT_e;

typedef enum _APP_PREF_SOUND_BACKGROUND_TYPE_e_ {
    PB_OPT_SOUND_BACKGROUND_LOGO = 0,
    PB_OPT_SOUND_BACKGROUND_PHOTO
} APP_PREF_SOUND_BACKGROUND_TYPE_e;

typedef enum _APP_PREF_AUTO_POWEROFF_e_ {
    AUTO_POWEROFF_OFF = 0,
    AUTO_POWEROFF_TIME_3M = 180,
    AUTO_POWEROFF_TIME_5M = 300
} APP_PREF_AUTO_POWEROFF_e;

typedef enum _APP_PREF_POWER_SAVING_e_ {
    POWERSAVING_OFF = 0,
    POWERSAVING_ON
} APP_PREF_POWER_SAVING_e;

typedef enum _APP_PREF_HDMI_SUPPORT_e_ {
    HDMI_SUPPORT_OFF = 0,
    HDMI_SUPPORT_ON
} APP_PREF_HDMI_SUPPORT_e;

typedef enum _APP_PREF_USB_MODE_e_ {
    USB_MODE_MSC = 0,
    USB_MODE_AMAGE,
    USB_MODE_RS232
} APP_PREF_USB_MODE_e;

typedef enum _APP_PREF_WIFI_STATUS_e_ {
    WIFI_OFF = 0,
    WIFI_ON,
    WIFI_AP
} APP_PREF_WIFI_STATUS_e;

typedef enum _APP_PREF_RET_STATUS_e_ {
    PREF_RET_STATUS_LOAD_NORMAL = 0,
    PREF_RET_STATUS_LOAD_RESET,
} APP_PREF_RET_STATUS_e;

typedef enum _APP_PREF_ADAS_STATUS_e_ {
    ADAS_OFF = 0,
    ADAS_ON
} APP_PREF_ADAS_STATUS_e;

/*************************************************************************
 * User preference structures
 ************************************************************************/
typedef struct _APP_PREF_USER_SYSTEM_s_ {
    UINT16 SystemVersion;
#define GEN_VER(m,n,p)      ((((m) & 0xF) << 12) | (((n) & 0x3F) << 6) | ((p) & 0x3F))
#define GET_VER_MAJOR(ver)  (((ver) >> 12) & 0xF)
#define GET_VER_MINOR(ver)  (((ver) >> 6) & 0x3F)
#define GET_VER_PATCH(ver)  ((ver) & 0x3F)
    APP_PREF_SYSTEM_MODE_e SystemMode:8;
    UINT8 Reserved1;
} APP_PREF_USER_SYSTEM_s;

typedef struct _APP_PREF_USER_VIDEO_s_ {
    UINT8 SensorVideoRes;
    UINT8 SensorVideoResSecond;
    UINT8 YUVVideoRes;
    UINT8 VideoQuality;
    APP_PREF_SELF_TIMER_e VideoSelftimer:8;
    UINT8 PreRecord;
    UINT8 TimeLapse;
    UINT8 DualStreams;
    APP_PREF_STREAMING_STATUS_e Streaming:8;
    APP_PREF_STREAM_TYPE_e StreamType:8;
    APP_PREF_DZOOM_e VideoDZoom:8;
    APP_PREF_TIME_STAMP_e VideoDateTimeStamp:8;
    UINT8 StillPriorityEN;
    UINT8 UnsavingData;
    UINT8 Reserved1;
    UINT8 Reserved2;
} APP_PREF_USER_VIDEO_s;

typedef struct _APP_PREF_USER_PHOTO_s_ {
    UINT8 PhotoMultiCap:3;
    UINT8 PhotoCapMode:3;
    UINT8 PhotoFastAf:2;
    UINT8 PhotoSize;
    UINT8 PhotoQuality;
    APP_PREF_SELF_TIMER_e PhotoSelftimer:8;
    UINT32 QuickviewDelay;
    APP_PREF_DZOOM_e PhotoDZoom:8;
    APP_PREF_TIME_STAMP_e PhotoTimeStamp:8;
    UINT16 TimeLapse;
} APP_PREF_USER_PHOTO_s;

typedef struct _APP_PREF_USER_IMAGE_s_ {
    UINT8 Flicker;
    UINT8 SlowShutter;
    UINT8 SceneMode;
    UINT8 WhiteBalance;
    UINT32 WbRedGain;
    UINT32 WbGreenGain;
    UINT32 WbBlueGain;
    UINT8 DigitalEffect;
    UINT8 Contrast;
    UINT8 Sharpness;
    UINT8 MeterMode;
    UINT8 ISO;
    UINT8 AFSetting;
    UINT8 ISSetting;
    UINT8 EVBiasIndex;
    UINT8 BLC;
    UINT8 Reserved;
    UINT8 Reserved1;
    UINT8 Reserved2;
    UINT32 Reserved3;
} APP_PREF_USER_IMAGE_s;

typedef struct _APP_PREF_USER_AUDIO_s_ {
    UINT8 AudioVolume;
    UINT8 Reserved;
    UINT8 Reserved1;
    UINT8 Reserved2;
} APP_PREF_USER_AUDIO_s;

typedef struct _APP_PREF_USER_PLAYBACK_s_ {
    UINT8 FileProtect;
    APP_PREF_VIDOE_PLAY_OPT_e VideoPlayOpt:8;
    APP_PREF_PHOTO_PLAY_OPT_e PhotoPlayOpt:8;
    UINT8 SlideshowMode;
    APP_PREF_PHOTO_SLIDESHOW_EFFECT_e SlideshowEffectType:8;
    UINT8 RealMovieEdit;
    UINT8 DeleteFile;
    UINT8 PostProc;
    UINT8 PostProcSave;
    UINT8 UnsavingData;
    UINT8 EdtrFunc;
    APP_PREF_SOUND_PLAY_OPT_e SoundPlayOpt:8;
    APP_PREF_SOUND_BACKGROUND_TYPE_e BackgroundSoundType:8;
    UINT32 Reserved1;
} APP_PREF_USER_PLAYBACK_s;

typedef struct _APP_PREF_USER_SETUP_s_ {
    UINT8 VinSystem;
    UINT8 VoutSystem;
    UINT8 DMFMode;
    APP_PREF_HDMI_SUPPORT_e EnableHDMI:8;
    UINT32 DmfPhotoLastIdx;    /**< last recorded photo object id*/
    UINT32 DmfSoundLastIdx;    /**< last recorded sound object id*/
    UINT32 DmfVideoLastIdx;    /**< last recorded video object id*/
    UINT32 DmfMixLastIdx;    /**< last recorded dcim object id */
    UINT8 LcdBrightness;           /** Default: 65 = 0x41 */
    UINT8 LcdContrast;             /** Default: 65 = 0x41 */
    UINT8 LcdColorBalance;         /** Default: 125 = 0x7D */
    APP_PREF_USB_MODE_e USBMode:8;
    APP_PREF_AUTO_POWEROFF_e AutoPoweroff:16;
    APP_PREF_POWER_SAVING_e PowerSaving:8;
    UINT8 InputDimension;
    UINT8 OutputDimension;
    APP_PREF_WIFI_STATUS_e Wifi:8;
    UINT16 Reserved1;
} APP_PREF_USER_SETUP_s;

typedef struct _APP_PREF_USER_VA_s_ {
    APP_PREF_ADAS_STATUS_e AdasDetection:8;
    UINT8 AdasCalibSky;     /** Default: 55% = 0x37 */
    UINT8 AdasCalibHood;    /** Default: 85% = 0x55 */
    UINT8 Reserved1;
    UINT32 Reserved2;
} APP_PREF_USER_VA_s;

typedef struct _APP_PREF_USER_s_ {
    /** System */
    APP_PREF_USER_SYSTEM_s SystemPref;
    /** Video */
    APP_PREF_USER_VIDEO_s VideoPref;
    /** Photo */
    APP_PREF_USER_PHOTO_s PhotoPref;
    /** Image */
    APP_PREF_USER_IMAGE_s ImagePref;
    /** Audio*/
    APP_PREF_USER_AUDIO_s AudioPref;
    /** Playback */
    APP_PREF_USER_PLAYBACK_s PlaybackPref;
    /** Setup */
    APP_PREF_USER_SETUP_s SetupPref;
    /** VA */
    APP_PREF_USER_VA_s VAPref;
} APP_PREF_USER_s;

extern APP_PREF_USER_s *UserSetting;

/*************************************************************************
 * User preference API
 ************************************************************************/
extern INT8 AppPref_InitPref(void);
extern APP_PREF_RET_STATUS_e AppPref_Load(void);
extern INT8 AppPref_Save(void);


__END_C_PROTO__

#endif /* APP_SYS_USR_PREF_H_ */
