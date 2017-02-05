/**
  * @file src/app/apps/flow/misc/misc_formatcard.h
  *
  * Header of format card application
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
#ifndef APP_MISC_FORMATCARD_H__
#define APP_MISC_FORMATCARD_H__

#include <apps/apps.h>
#include <apps/flow/widget/widgetmgt.h>
#include <apps/gui/misc/gui_misc_formatcard.h>

__BEGIN_C_PROTO__

//#define MISC_FORMATCARD_DEBUG
#if defined(MISC_FORMATCARD_DEBUG)
#define DBGMSG  AmbaPrint
#define DBGMSGc2    AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc2(...)
#endif


/*************************************************************************
 * App Flag Definitions
 ************************************************************************/
#define MISC_FORMATCARD_CARD_PROTECTED                (0x0001)
#define MISC_FORMATCARD_DO_FORMAT_CARD                (0x0002)
#define MISC_FORMATCARD_WARNING_MSG_RUN    (0x0004)

/*************************************************************************
 * App Function Definitions
 ************************************************************************/
typedef enum _MISC_FORMATCARD_FUNC_ID_e_ {
    MISC_FORMATCARD_INIT = 0,
    MISC_FORMATCARD_START,
    MISC_FORMATCARD_START_FLG_ON,
    MISC_FORMATCARD_STOP,
    MISC_FORMATCARD_OP_DONE,
    MISC_FORMATCARD_OP_SUCCESS,
    MISC_FORMATCARD_OP_FAILED,
    MISC_FORMATCARD_SWITCH_APP,
    MISC_FORMATCARD_CARD_REMOVED,
    MISC_FORMATCARD_DIALOG_SHOW_FORMATCARD,
    MISC_FORMATCARD_STATE_WIDGET_CLOSED,
    MISC_FORMATCARD_WARNING_MSG_SHOW_START,
    MISC_FORMATCARD_WARNING_MSG_SHOW_STOP
} MISC_FORMATCARD_FUNC_ID_e;

extern int misc_formatcard_func(UINT32 funcId, UINT32 param1, UINT32 param2);

/*************************************************************************
 * App Status Definitions
 ************************************************************************/

typedef struct _MISC_FORMATCARD_s_ {
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
    int (*Gui)(UINT32 guiCmd, UINT32 param1, UINT32 param2);
} MISC_FORMATCARD_s;

extern MISC_FORMATCARD_s misc_formatcard;

__END_C_PROTO__

#endif /* APP_MISC_FORMATCARD_H__ */
