/**
  * @file src/app/apps/flow/rec/rec_connected_cam.h
  *
  * Header of CAR Recorder (sensor) application
  *
  * History:
  *    2013/03/24 - [Annie Ting] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#ifndef APP_REC_CONNECTED_CAM_H__
#define APP_REC_CONNECTED_CAM_H__

#include <apps/apps.h>
#include <apps/flow/widget/widgetmgt.h>
#include <apps/gui/rec/gui_rec_connected_cam.h>

__BEGIN_C_PROTO__

#define REC_CONNECTED_CAM_DEBUG
#if defined(REC_CONNECTED_CAM_DEBUG)
#define DBGMSG  AmbaPrint
#define DBGMSGc2    AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc2(...)
#endif


/** Define this flag, it will do pause/resume flow
      when memory runout */
//#define REC_CONNECTED_CAM_MEM_RUNOUT_PAUSE


/**threshold value for free space check*/
#define FREESPACE_THRESHOLD 400*1024*1024 /**< default 400MB*/

/**Photo capture limit amount*/
#define MAX_PHOTO_COUNT 50
/*************************************************************************
 * App Flag Definitions
 ************************************************************************/
#define REC_CONNECTED_CAM_FLAGS_PAUSED            (1<<0)
#define REC_CONNECTED_CAM_FLAGS_SELFTIMER_RUN (1<<1)
#define REC_CONNECTED_CAM_FLAGS_MUXER_BUSY     (1<<2)
#define REC_CONNECTED_CAM_FLAGS_MUXER_OPEN     (1<<3)
#define REC_CONNECTED_CAM_FLAGS_MUXER_ERROR     (1<<4)
#define REC_CONNECTED_CAM_FLAGS_SHUTTER_PRESSED  (1<<5)
#define REC_CONNECTED_CAM_FLAGS_STILL_CAPTURE (1<<6)
#define REC_CONNECTED_CAM_FLAGS_CAPTURE_BG_PROCESS (1<<7)
#define REC_CONNECTED_CAM_FLAGS_WARNING_MSG_RUN (1<<8)
#define REC_CONNECTED_CAM_FLAGS_MEM_RUNOUT (1<<9)
#define REC_CONNECTED_CAM_FLAGS_EM_RECORD  (1<<10)
#define REC_CONNECTED_CAM_FLAGS_PES_DELAY   (1<<11)
#define REC_CONNECTED_CAM_FLAGS_CAPTURE_ON_VF  (1<<12)
#define REC_CONNECTED_CAM_FLAGS_NETFIFO_BUSY  (1<<13)
#define REC_CONNECTED_CAM_FLAGS_CAPTURE_FROM_NETCTRL    (1<<14)
#define REC_CONNECTED_CAM_FLAGS_CAPTURE_PIV    (1<<15)
#define REC_CONNECTED_CAM_FLAGS_BLOCK_MENU    (1<<16)
#define REC_CONNECTED_CAM_FLAGS_BLOCK_USB    (1<<17)


/*************************************************************************
 * App Function Definitions
 ************************************************************************/
typedef enum _REC_CONNECTED_CAM_FUNC_ID_e_ {
    REC_CONNECTED_CAM_INIT,
    REC_CONNECTED_CAM_START,
    REC_CONNECTED_CAM_START_FLAG_ON,
    REC_CONNECTED_CAM_STOP,
    REC_CONNECTED_CAM_SET_APP_ENV,
    REC_CONNECTED_CAM_LIVEVIEW_STATE,
    REC_CONNECTED_CAM_LIVEVIEW_POST_ACTION,
    REC_CONNECTED_CAM_SELFTIMER_START,
    REC_CONNECTED_CAM_SELFTIMER_STOP,
    REC_CONNECTED_CAM_FOCUS,
    REC_CONNECTED_CAM_PREVIEW,
    REC_CONNECTED_CAM_CAPTURE,
    REC_CONNECTED_CAM_CAPTURE_PIV,
    REC_CONNECTED_CAM_CAPTURE_COMPLETE,
    REC_CONNECTED_CAM_CAPTURE_BG_PROCESS_DONE,
    REC_CONNECTED_CAM_PIV,
    REC_CONNECTED_CAM_RECORD_START,
    REC_CONNECTED_CAM_RECORD_STOP,
    REC_CONNECTED_CAM_RECORD_PAUSE,
    REC_CONNECTED_CAM_RECORD_RESUME,
    REC_CONNECTED_CAM_RECORD_AUTO_START,
    REC_CONNECTED_CAM_RECORD_PRE_RECORD,
    REC_CONNECTED_CAM_RECORD_EMERGENCY_START,
    REC_CONNECTED_CAM_MUXER_START,
    REC_CONNECTED_CAM_MUXER_OPEN,
    REC_CONNECTED_CAM_MUXER_END,
    REC_CONNECTED_CAM_MUXER_REACH_LIMIT,
    REC_CONNECTED_CAM_EVENTRECORD_START,
    REC_CONNECTED_CAM_MUXER_END_EVENTRECORD,
    REC_CONNECTED_CAM_MUXER_REACH_LIMIT_EVENTRECORD,
    REC_CONNECTED_CAM_MUXER_STREAM_ERROR,
    REC_CONNECTED_CAM_ERROR_MEMORY_RUNOUT,
    REC_CONNECTED_CAM_ERROR_STORAGE_RUNOUT,
    REC_CONNECTED_CAM_ERROR_STORAGE_IO,
    REC_CONNECTED_CAM_ERROR_LOOP_ENC_ERR,
    REC_CONNECTED_CAM_LOOP_ENC_DONE,
    REC_CONNECTED_CAM_EDTMGR_DONE,
    REC_CONNECTED_CAM_EDTMGR_FAIL,
    REC_CONNECTED_CAM_EM_RECORD_RETURN,
    REC_CONNECTED_CAM_SWITCH_APP,
    REC_CONNECTED_CAM_SET_VIDEO_RES,
    REC_CONNECTED_CAM_SET_VIDEO_QUALITY,
    REC_CONNECTED_CAM_SET_VIDEO_PRE_RECORD,
    REC_CONNECTED_CAM_SET_VIDEO_TIME_LAPSE,
    REC_CONNECTED_CAM_SET_VIDEO_DUAL_STREAMS,
    REC_CONNECTED_CAM_SET_VIDEO_RECORD_MODE,
    REC_CONNECTED_CAM_SET_PHOTO_SIZE,
    REC_CONNECTED_CAM_SET_PHOTO_QUALITY,
    REC_CONNECTED_CAM_SET_ENC_MODE,
    REC_CONNECTED_CAM_SET_DMF_MODE,
    REC_CONNECTED_CAM_SET_SELFTIMER,
    REC_CONNECTED_CAM_CARD_REMOVED,
    REC_CONNECTED_CAM_CARD_ERROR_REMOVED,
    REC_CONNECTED_CAM_CARD_NEW_INSERT,
    REC_CONNECTED_CAM_CARD_STORAGE_IDLE,
    REC_CONNECTED_CAM_CARD_STORAGE_BUSY,
    REC_CONNECTED_CAM_CARD_CHECK_STATUS,
    REC_CONNECTED_CAM_CARD_FULL_HANDLE,
    REC_CONNECTED_CAM_CARD_FULL_HANDLE_EVENT,
    REC_CONNECTED_CAM_SET_FILE_INDEX,
    REC_CONNECTED_CAM_FILE_ID_UPDATE,
    REC_CONNECTED_CAM_WIDGET_CLOSED,
    REC_CONNECTED_CAM_SET_SYSTEM_TYPE,
    REC_CONNECTED_CAM_UPDATE_FCHAN_VOUT,
    REC_CONNECTED_CAM_UPDATE_DCHAN_VOUT,
    REC_CONNECTED_CAM_CHANGE_DISPLAY,
    REC_CONNECTED_CAM_CHANGE_OSD,
    REC_CONNECTED_CAM_AUDIO_INPUT,
    REC_CONNECTED_CAM_AUDIO_OUTPUT,
    REC_CONNECTED_CAM_USB_CONNECT,
    REC_CONNECTED_CAM_GUI_INIT_SHOW,
    REC_CONNECTED_CAM_UPDATE_BAT_POWER_STATUS,
    REC_CONNECTED_CAM_WARNING_MSG_SHOW_START,
    REC_CONNECTED_CAM_WARNING_MSG_SHOW_STOP,
    REC_CONNECTED_CAM_CARD_FMT_NONOPTIMUM,
    REC_CONNECTED_CAM_ADAS_EVENT,
    REC_CONNECTED_CAM_ADAS_FUNCTION_INIT,
    REC_CONNECTED_CAM_ADAS_UPDATE_PARAM,
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    REC_CONNECTED_CAM_BOSS_BOOTED,
    REC_CONNECTED_CAM_VF_START,
    REC_CONNECTED_CAM_VF_STOP,
    REC_CONNECTED_CAM_VF_SWITCH_TO_RECORD,
    REC_CONNECTED_CAM_CAPTURE_ON_VF,
    REC_CONNECTED_CAM_NETFIFO_EVENT_START,
    REC_CONNECTED_CAM_NETFIFO_EVENT_STOP,
    REC_CONNECTED_CAM_NETCTRL_CAPTURE_DONE,
    REC_CONNECTED_CAM_NETCTRL_PIV_DONE
#endif
} REC_CONNECTED_CAM_FUNC_ID_e;

extern int rec_connected_cam_func(UINT32 funcId, UINT32 param1, UINT32 param2);

/*************************************************************************
 * App Operation Definitions
 ************************************************************************/
typedef struct _REC_CONNECTED_CAM_OP_s_ {
    int (*ButtonRecord)(void);
    int (*ButtonFocus)(void);
    int (*ButtonFocusClr)(void);
    int (*ButtonShutter)(void);
    int (*ButtonShutterClr)(void);
    int (*ButtonZoomIn)(void);
    int (*ButtonZoomInClr)(void);
    int (*ButtonZoomOut)(void);
    int (*ButtonZoomOutClr)(void);
    int (*ButtonUp)(void);
    int (*ButtonDown)(void);
    int (*ButtonLeft)(void);
    int (*ButtonRight)(void);
    int (*ButtonSet)(void);
    int (*ButtonMenu)(void);
    int (*ButtonMode)(void);
    int (*ButtonDel)(void);
    int (*ButtonPower)(void);
    int (*ButtonF4)(void);
} REC_CONNECTED_CAM_OP_s;

extern REC_CONNECTED_CAM_OP_s rec_connected_cam_op;

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
typedef struct _REC_CONNECTED_CAM_NETCTRL_s_ {
    int (*NetCtrlStopSession)(void);
    int (*NetCtrlRecordStart)(void);
    int (*NetCtrlRecordStop)(void);
    int (*NetCtrlGetRecordTime)(void);
    int (*NetCtrlCapture)(void);
    int (*NetCtrlContinueCaptureStop)(void);
    int (*NetCtrlRefreshPrefTable)(void);
    int (*NetCtrlGetAllCurSetting)(void);
    int (*NetCtrlGetSettingOptions)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlGetSetting)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlSetSetting)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlGetNumbFiles)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlGetDeviceInfo)(void);
    int (*NetCtrlVFStop)(void);
    int (*NetCtrlVFReset)(void);
    int (*NetCtrlGetThumb)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlGetMediaInfo)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlFormat)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlFormatDone)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlGetSpace)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlBurninFw)(UINT32 Param1, UINT32 Param2);
    int (*NetCtrlCustomCmd)(UINT32 Param1, UINT32 Param2);
} REC_CONNECTED_CAM_NETCTRL_s;

extern REC_CONNECTED_CAM_NETCTRL_s rec_connected_cam_netctrl_op;

#define APP_NETCTRL_FLAGS_VF_DISABLE    (1<<0)
#define APP_NETCTRL_FLAGS_VF_RESET_DONE (1<<1)
#define APP_NETCTRL_FLAGS_VF_STOP_DONE  (1<<2)



#endif


/*************************************************************************
 * App Status Definitions
 ************************************************************************/
typedef struct _REC_CONNECTED_CAM_s_ {
    UINT8 RecCapState;
#define REC_CAP_STATE_PREVIEW   (0x00)
#define REC_CAP_STATE_RECORD    (0x01)
#define REC_CAP_STATE_PRE_RECORD    (0x02)
#define REC_CAP_STATE_FOCUS     (0x03)
#define REC_CAP_STATE_CAPTURE   (0x04)
#define REC_CAP_STATE_VF   (0x05)
#define REC_CAP_STATE_TRANSIT_TO_VF   (0x06)
#define REC_CAP_STATE_RESET     (0xFF)
    UINT8 RecCurrMode;
    UINT8 RecNextMode;
#define REC_CAP_MODE_STOP        (0x00)
#define REC_CAP_MODE_VIEWFINDER    (0x01)
#define REC_CAP_MODE_RECORD        (0x02)
#define REC_CAP_MODE_CAPTURE        (0x03)
    UINT8 SelfTimerType;
#define SELF_TIMER_TYPE_PHOTO   (0)
#define SELF_TIMER_TYPE_VIDEO   (1)
    INT32 MuxerNum;
    int RecTime;
    int SelfTimerTime;
    int TimeLapseTime;
    int QuickViewFileType;
#define MEDIA_TYPE_UNKNOWN  (0x00)
#define MEDIA_TYPE_VIDEO  (0x01)
#define MEDIA_TYPE_PHOTO  (0x02)
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
    int (*Gui)(UINT32 guiCmd, UINT32 param1, UINT32 param2);
    REC_CONNECTED_CAM_OP_s *Op;
#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
    UINT32 NetCtrlFlags;
    REC_CONNECTED_CAM_NETCTRL_s *NetCtrl;
#endif
} REC_CONNECTED_CAM_s;

extern REC_CONNECTED_CAM_s rec_connected_cam;

__END_C_PROTO__

#endif /* APP_REC_CONNECTED_CAM_H__ */
