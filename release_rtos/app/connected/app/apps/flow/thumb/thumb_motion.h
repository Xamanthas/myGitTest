/**
  * @file src/app/apps/flow/thumb/thumb_motion.h
  *
  * Header of Player Thumbnail Basic View
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
#ifndef APP_THUMB_MOTION_H__
#define APP_THUMB_MOTION_H__

#include <apps/apps.h>
#include <apps/flow/widget/widgetmgt.h>
#include <apps/gui/thumb/gui_thumb_motion.h>

__BEGIN_C_PROTO__

//#define THUMB_MOTION_DEBUG
#if defined(THUMB_MOTION_DEBUG)
#define DBGMSG  AmbaPrint
#define DBGMSGc2    AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc2(...)
#endif


/*************************************************************************
 * App Flag Definitions
 ************************************************************************/
#define THUMB_MOTION_BUF_INIT        (0x0004)
#define THUMB_MOTION_WARNING_MSG_RUN    (0x0008)
#define THUMB_MOTION_DELETE_FILE_RUN    (0x0010)
#define THUMB_MOTION_VOUT_CHG_PRE    (0x0020)
#define THUMB_MOTION_VOUT_CHG_DONE    (0x0040)
#define THUMB_MOTION_VOUT_CHG_READY_BLOCK    (0x0100)
#define THUMB_MOTION_TVOUT_FOCUS    (0x0200)


/*************************************************************************
 * App Function Definitions
 ************************************************************************/
typedef enum _THUMB_MOTION_FUNC_ID_e_ {
    THUMB_MOTION_INIT = 0,
    THUMB_MOTION_START,
    THUMB_MOTION_START_FLG_ON,
    THUMB_MOTION_STOP,
    THUMB_MOTION_APP_READY,
    THUMB_MOTION_SET_APP_ENV,
    THUMB_MOTION_START_DISP_PAGE,
    THUMB_MOTION_SHOW_ITEM,
    THUMB_MOTION_SHOW_PAGE_INFO,
    THUMB_MOTION_SHIFT_FILE_TO_PREV,
    THUMB_MOTION_SHIFT_FILE_TO_NEXT,
    THUMB_MOTION_SHIFT_TAB,
    THUMB_MOTION_STOP_PLAYING,
    THUMB_MOTION_CARD_REMOVED,
    THUMB_MOTION_CARD_ERROR_REMOVED,
    THUMB_MOTION_CARD_NEW_INSERT,
    THUMB_MOTION_CARD_STORAGE_IDLE,
    THUMB_MOTION_FILE_ID_UPDATE,
    THUMB_MOTION_SET_FILE_INDEX,
    THUMB_MOTION_SELECT_DELETE_FILE_MODE,
    THUMB_MOTION_SET_DELETE_FILE_MODE,
    THUMB_MOTION_DELETE_FILE_DIALOG_SHOW,
    THUMB_MOTION_DELETE_FILE,
    THUMB_MOTION_DELETE_FILE_COMPLETE,
    THUMB_MOTION_STATE_WIDGET_CLOSED,
    THUMB_MOTION_SET_SYSTEM_TYPE,
    THUMB_MOTION_UPDATE_FCHAN_VOUT,
    THUMB_MOTION_UPDATE_DCHAN_VOUT,
    THUMB_MOTION_CHANGE_DISPLAY,
    THUMB_MOTION_CHANGE_OSD,
    THUMB_MOTION_AUDIO_INPUT,
    THUMB_MOTION_AUDIO_OUTPUT,
    THUMB_MOTION_USB_CONNECT,
    THUMB_MOTION_GUI_INIT_SHOW,
    THUMB_MOTION_UPDATE_BAT_POWER_STATUS,
    THUMB_MOTION_WARNING_MSG_SHOW_START,
    THUMB_MOTION_WARNING_MSG_SHOW_STOP
} THUMB_MOTION_FUNC_ID_e;


typedef enum _THUMB_MOTION_TAB_TYPE_ID_e_ {
    THUMB_MOTION_TAB_VIDEO = 0,
    THUMB_MOTION_TAB_PHOTO,
    THUMB_MOTION_TAB_DCIM,
    THUMB_MOTION_TAB_NUM
} THUMB_MOTION_TAB_TYPE_ID_e;

extern int thumb_motion_func(UINT32 funcId, UINT32 param1, UINT32 param2);

/*************************************************************************
 * App Operation Definitions
 ************************************************************************/
typedef struct _THUMB_MOTION_OP_s_ {
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
} THUMB_MOTION_OP_s;

extern THUMB_MOTION_OP_s thumb_motion_op;


/*************************************************************************
 * App Status Definitions
 ************************************************************************/
typedef struct _THUMB_MOTION_FILE_INFO_s_ {
    APPLIB_DCF_MEDIA_TYPE_e MediaRoot;
    int TotalFileNum;
    int FileCur;
    int TotalPageNum;
    int PageCur;
    int PageItemNum; /** item number in current page */
    int PageItemCur; /** current item in current page */
} THUMB_MOTION_FILE_INFO_s;

typedef struct _THUMB_MOTION_s_ {
    UINT8 RedrawEventType;
#define REDRAW_NONE    (0)
#define REDRAW_SYS_TYPE_CHG    (1)
#define REDRAW_JACK_EVENT    (2)
#define REDRAW_ROTATE_SCREEN    (4)
#define REDRAW_CARD_REMOVE    (8)
    UINT8 DeleteFileMode;
    UINT8 TabNum;
    INT8 TabCur;
    THUMB_MOTION_FILE_INFO_s FileInfo;
    int MaxPageItemNum;
    int DispCol;
    int DispRow;
    char CurFn[APP_MAX_FN_SIZE];
    UINT64 CurFileObjID;
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
    int (*Gui)(UINT32 guiCmd, UINT32 param1, UINT32 param2);
    THUMB_MOTION_OP_s *Op;
} THUMB_MOTION_s;

extern THUMB_MOTION_s thumb_motion;

__END_C_PROTO__

#endif /* APP_THUMB_MOTION_H__ */
