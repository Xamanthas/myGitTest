/**
  * @file src/app/apps/flow/misc/misc_fwupdate.h
  *
  * Header of Firmware update application
  *
  * History:
  *    2014/03/20 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef APP_MISC_FWUPDATE_H_
#define APP_MISC_FWUPDATE_H_

#include <apps/apps.h>
#include <apps/flow/widget/widgetmgt.h>
#include <apps/gui/misc/gui_misc_fwupdate.h>

__BEGIN_C_PROTO__

//#define MISC_FWUPDATE_DEBUG
#if defined(MISC_FWUPDATE_DEBUG)
#define DBGMSG  AmbaPrint
#define DBGMSGc2    AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc2(...)
#endif

/*************************************************************************
 * App Flags Definitions
 ************************************************************************/
#define MISC_FWUPDATE_WARNING_MSG_RUN    (0x0001)

/*************************************************************************
 * App General Definitions
 ************************************************************************/

/*************************************************************************
 * App Function Definitions
 ************************************************************************/
typedef enum _MISC_FWUPDATE_FUNC_ID_e_ {
    MISC_FWUPDATE_INIT = 0,
    MISC_FWUPDATE_START,
    MISC_FWUPDATE_START_FLG_ON,
    MISC_FWUPDATE_STOP,
    MISC_FWUPDATE_APP_READY,
    MISC_FWUPDATE_SET_APP_ENV,
    MISC_FWUPDATE_SWITCH_APP,
    MISC_FWUPDATE_DIALOG_SHOW_FWUPDATE,
    MISC_FWUPDATE_CARD_REMOVED,
    MISC_FWUPDATE_CARD_ERROR_REMOVED,
    MISC_FWUPDATE_UPDATE_FCHAN_VOUT,
    MISC_FWUPDATE_UPDATE_DCHAN_VOUT,
    MISC_FWUPDATE_CHANGE_DISPLAY,
    MISC_FWUPDATE_CHANGE_OSD,
    MISC_FWUPDATE_AUDIO_INPUT,
    MISC_FWUPDATE_AUDIO_OUTPUT,
    MISC_FWUPDATE_USB_CONNECT,
    MISC_FWUPDATE_GUI_INIT_SHOW,
    MISC_FWUPDATE_WARNING_MSG_SHOW_START,
    MISC_FWUPDATE_WARNING_MSG_SHOW_STOP,
    MISC_FWUPDATE_CANCEL_BY_NETCTRL
} MISC_FWUPDATE_FUNC_ID_e;

extern int misc_fwupdate_func(UINT32 funcId, UINT32 param1, UINT32 param2);

/*************************************************************************
 * App Operation Definitions
 ************************************************************************/

/*************************************************************************
 * App Status Definitions
 ************************************************************************/
typedef struct _MISC_FWUPDATE_s_ {
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
    int (*Gui)(UINT32 guiCmd, UINT32 param1, UINT32 param2);
} MISC_FWUPDATE_s;

extern MISC_FWUPDATE_s misc_fwupdate;

__END_C_PROTO__

#endif /* APP_MISC_FWUPDATE_H_ */
