/**
  * @file src/app/apps/flow/misc/misc_mvrecover.h
  *
  * Header of Movie Recover application
  *
  * History:
  *    2014/01/14 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#ifndef APP_MISC_MVRECOVER_H__
#define APP_MISC_MVRECOVER_H__

#include <apps/apps.h>
#include <apps/flow/widget/widgetmgt.h>
#include <apps/gui/misc/gui_misc_mvrecover.h>

__BEGIN_C_PROTO__

//#define MISC_MVRECOVER_DEBUG
#if defined(MISC_MVRECOVER_DEBUG)
#define DBGMSG  AmbaPrint
#define DBGMSGc2    AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc2(...)
#endif


/*************************************************************************
 * App Flag Definitions
 ************************************************************************/
#define MISC_MVRECOVER_WARNING_MSG_RUN    (0x0004)



/*************************************************************************
 * App Function Definitions
 ************************************************************************/
typedef enum _MISC_MVRECOVER_FUNC_ID_e_ {
    MISC_MVRECOVER_INIT = 0,
    MISC_MVRECOVER_START,
    MISC_MVRECOVER_START_FLG_ON,
    MISC_MVRECOVER_STOP,
    MISC_MVRECOVER_APP_READY,
    MISC_MVRECOVER_SET_APP_ENV,
    MISC_MVRECOVER_SWITCH_APP,
    MISC_MVRECOVER_DIALOG_SHOW_MVRECOVER,
    MISC_MVRECOVER_OP_SUCCESS,
    MISC_MVRECOVER_OP_FAILED,
    MISC_MVRECOVER_CARD_REMOVED,
    MISC_MVRECOVER_CARD_ERROR_REMOVED,
    MISC_MVRECOVER_UPDATE_FCHAN_VOUT,
    MISC_MVRECOVER_UPDATE_DCHAN_VOUT,
    MISC_MVRECOVER_CHANGE_DISPLAY,
    MISC_MVRECOVER_CHANGE_OSD,
    MISC_MVRECOVER_AUDIO_INPUT,
    MISC_MVRECOVER_AUDIO_OUTPUT,
    MISC_MVRECOVER_USB_CONNECT,
    MISC_MVRECOVER_GUI_INIT_SHOW,
    MISC_MVRECOVER_WARNING_MSG_SHOW_START,
    MISC_MVRECOVER_WARNING_MSG_SHOW_STOP
} MISC_MVRECOVER_FUNC_ID_e;

extern int misc_mvrecover_func(UINT32 funcId, UINT32 param1, UINT32 param2);

/*************************************************************************
 * App Status Definitions
 ************************************************************************/

typedef struct _MISC_MVRECOVER_s_ {
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
    int (*Gui)(UINT32 guiCmd, UINT32 param1, UINT32 param2);
} MISC_MVRECOVER_s;

extern MISC_MVRECOVER_s misc_mvrecover;

__END_C_PROTO__

#endif /* APP_MISC_MVRECOVER_H__ */
