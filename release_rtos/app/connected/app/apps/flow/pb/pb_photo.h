/**
  * @file src/app/apps/flow/pb/pb_photo.h
  *
  * Header of photo playback application
  *
  * History:
  *    2013/11/08 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#ifndef APP_PB_PHOTO_H__
#define APP_PB_PHOTO_H__

#include <apps/apps.h>
#include <apps/flow/widget/widgetmgt.h>
#include <apps/gui/pb/gui_pb_photo.h>

__BEGIN_C_PROTO__

//#define PB_PHOTO_DEBUG
#if defined(PB_PHOTO_DEBUG)
#define DBGMSG  AmbaPrint
#define DBGMSGc2    AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc2(...)
#endif


/*************************************************************************
 * App Flag Definitions
 ************************************************************************/
#define PB_PHOTO_OP_BLOCKED                (0x0001)
#define PB_PHOTO_OP_CONT                (0x0002)
#define PB_PHOTO_WARNING_MSG_RUN    (0x0004)
#define PB_PHOTO_DELETE_FILE_RUN        (0x0008)

/** Play flags */
#define PB_PHOTO_PLAYBACK_NORMAL        (0x00010000)
#define PB_PHOTO_PLAYBACK_ZOOM                (0x00020000)
#define PB_PHOTO_PLAYBACK_ROTATE                (0x00040000)

/** GET_FILE parameters */
#define GET_CURR_FILE    (0)
#define GET_NEXT_FILE    (1)
#define GET_PREV_FILE    (2)

/** Rotate dictionary */
typedef enum _PB_PHOTO_ROTATE_INDEX_e_ {
    ROTATE_0 = 0,
    ROTATE_90,
    ROTATE_180,
    ROTATE_270,
    ROTATE_NUM
} PB_PHOTO_ROTATE_INDEX_e;

/*************************************************************************
 * App Function Definitions
 ************************************************************************/
typedef enum _PB_PHOTO_FUNC_ID_e_ {
    PB_PHOTO_INIT = 0,
    PB_PHOTO_START,
    PB_PHOTO_START_FLG_ON,
    PB_PHOTO_STOP,
    PB_PHOTO_APP_READY,
    PB_PHOTO_SET_APP_ENV,
    PB_PHOTO_START_DISP_PAGE,
    PB_PHOTO_PLAY,
    PB_PHOTO_STOP_PLAYING,
    PB_PHOTO_SWITCH_APP,
    PB_PHOTO_GET_FILE,
    PB_PHOTO_CARD_REMOVED,
    PB_PHOTO_CARD_ERROR_REMOVED,
    PB_PHOTO_CARD_NEW_INSERT,
    PB_PHOTO_CARD_STORAGE_IDLE,
    PB_PHOTO_SET_FILE_INDEX,
    PB_PHOTO_DELETE_FILE_DIALOG_SHOW,
    PB_PHOTO_DELETE_FILE,
    PB_PHOTO_DELETE_FILE_COMPLETE,
    PB_PHOTO_STATE_WIDGET_CLOSED,
    PB_PHOTO_SET_SYSTEM_TYPE,
    PB_PHOTO_UPDATE_FCHAN_VOUT,
    PB_PHOTO_UPDATE_DCHAN_VOUT,
    PB_PHOTO_CHANGE_DISPLAY,
    PB_PHOTO_CHANGE_OSD,
    PB_PHOTO_AUDIO_INPUT,
    PB_PHOTO_AUDIO_OUTPUT,
    PB_PHOTO_USB_CONNECT,
    PB_PHOTO_GUI_INIT_SHOW,
    PB_PHOTO_UPDATE_BAT_POWER_STATUS,
    PB_PHOTO_WARNING_MSG_SHOW_START,
    PB_PHOTO_WARNING_MSG_SHOW_STOP
} PB_PHOTO_FUNC_ID_e;

extern int pb_photo_func(UINT32 funcId, UINT32 param1, UINT32 param2);

/*************************************************************************
 * App Operation Definitions
 ************************************************************************/
typedef struct _PB_PHOTO_OP_s_ {
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
} PB_PHOTO_OP_s;

extern PB_PHOTO_OP_s pb_photo_op;


/*************************************************************************
 * App Status Definitions
 ************************************************************************/

typedef struct _PB_PHOTO_FILE_INFO_s_ {
    APPLIB_DCF_MEDIA_TYPE_e MediaRoot;
    int TotalFileNum;
    int FileCur;
} PB_PHOTO_FILE_INFO_s;

typedef struct _PB_PHOTO_MEDIA_INFO_s_ {
    UINT32 Width;
    UINT32 Height;
    INT8 Rotate;
    INT8 RotateOri;
    UINT16 Iso;
    UINT32 Zoom;
#define PB_PHOTO_IZOOM_MIN        (100)
#define PB_PHOTO_IZOOM_MAX        (1000)
    int ZoomStep;
} PB_PHOTO_MEDIA_INFO_s;

typedef struct _PB_PHOTO_s_ {
    char CurFn[MAX_FILENAME_LENGTH];
    UINT64 CurFileObjID;
    PB_PHOTO_FILE_INFO_s FileInfo;
    PB_PHOTO_MEDIA_INFO_s MediaInfo;
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
    int (*Gui)(UINT32 guiCmd, UINT32 param1, UINT32 param2);
    PB_PHOTO_OP_s *Op;
    int (*OpBlocked)(void);
    int (*OpCont)(void);
} PB_PHOTO_s;

extern PB_PHOTO_s pb_photo;

__END_C_PROTO__

#endif /* APP_PB_PHOTO_H__ */
