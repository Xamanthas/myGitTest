/**
  * @file src/app/apps/flow/misc/misc_calibupdate.c
  *
  * Implementation of Firmware update application
  *
  * History:
  *    2014/06/25 - [Annie Ting] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include "misc_calibupdate.h"

/*************************************************************************
 * Declarations (static)
 ************************************************************************/
/* App structure interfaces APIs */
static int app_misc_calibupdate_start(void);
static int app_misc_calibupdate_stop(void);
static int app_misc_calibupdate_on_message(UINT32 msg, UINT32 param1, UINT32 param2);

APP_APP_s app_misc_calibupdate = {
    0,    //id
    1,    //tier
    0,    //parent
    0,    //previous
    0,    //child
    0,    //GFlags
    0,    //flags
    app_misc_calibupdate_start,    //start()
    app_misc_calibupdate_stop,    //stop()
    app_misc_calibupdate_on_message    //on_message()
};

MISC_CALIBUPDATE_s misc_calibupdate = {0};

/*************************************************************************
 * Definitions (static)
 ************************************************************************/
/* App structure interface APIs */

/**
 *  @brief The application's function that handle the message.
 *
 *  The application's function that handle the message.
 *
 *  @param [in] msg Message ID
 *  @param [in] param1 first parameter
 *  @param [in] param2 second parameter
 *
 *  @return >=0 success, <0 failure
 */
static int app_misc_calibupdate_on_message(UINT32 msg, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;

    ReturnValue = AppWidget_OnMessage(msg, param1, param2);
    if (ReturnValue != WIDGET_PASSED_MSG) {
        return ReturnValue;
    }

    switch (msg) {
    case AMSG_ERROR_CARD_REMOVED:
        misc_calibupdate.Func(MISC_CALIBUPDATE_CARD_ERROR_REMOVED, param1, param2);
        DBGMSG("[app_misc_calibupdate] Illegal operation: Remove card during IO/busy");
        break;
    case AMSG_STATE_CARD_REMOVED:
        misc_calibupdate.Func(MISC_CALIBUPDATE_CARD_REMOVED, param1, param2);
        break;
    case HMSG_HDMI_INSERT_SET:
    case HMSG_HDMI_INSERT_CLR:
    case HMSG_CS_INSERT_SET:
    case HMSG_CS_INSERT_CLR:
        if (!APP_CHECKFLAGS(app_misc_calibupdate.GFlags, APP_AFLAGS_READY)) {
            AmbaPrint("[app_misc_calibupdate] System is not ready. Jack event will be handled later");
        } else if (APP_CHECKFLAGS(app_misc_calibupdate.GFlags, APP_AFLAGS_BUSY)) {
            AmbaPrint("[app_misc_calibupdate] System is busy.  Jack event will be handled later.");
        } else {
            misc_calibupdate.Func(MISC_CALIBUPDATE_UPDATE_FCHAN_VOUT, msg, param1);
        }
        break;
    case HMSG_LINEIN_IN_SET:
    case HMSG_LINEIN_IN_CLR:
        if (!APP_CHECKFLAGS(app_misc_calibupdate.GFlags, APP_AFLAGS_READY)) {
            AmbaPrint("[app_misc_calibupdate] System is not ready. Audio jack event will be handled later");
        } else {
            misc_calibupdate.Func(MISC_CALIBUPDATE_AUDIO_INPUT, param1, param2);
        }
        break;
    case HMSG_HP_IN_SET:
    case HMSG_HP_IN_CLR:
    case HMSG_LINEOUT_IN_SET:
    case HMSG_LINEOUT_IN_CLR:
        if (!APP_CHECKFLAGS(app_misc_calibupdate.GFlags, APP_AFLAGS_READY)) {
            AmbaPrint("[app_misc_calibupdate] System is not ready. Audio jack event will be handled later");
        } else {
            misc_calibupdate.Func(MISC_CALIBUPDATE_AUDIO_OUTPUT, param1, param2);
        }
        break;
    default:
        break;
    }

    return ReturnValue;
}

static int app_misc_calibupdate_start(void)
{
    int ReturnValue = 0;

    misc_calibupdate.Func = misc_calibupdate_func;
    misc_calibupdate.Gui = gui_misc_calibupdate_func;

    if (!APP_CHECKFLAGS(app_misc_calibupdate.GFlags, APP_AFLAGS_INIT)) {
        APP_ADDFLAGS(app_misc_calibupdate.GFlags, APP_AFLAGS_INIT);
        /** Initialize application structure */
    }

    if (!APP_CHECKFLAGS(app_misc_calibupdate.GFlags, APP_AFLAGS_START)) {
        APP_ADDFLAGS(app_misc_calibupdate.GFlags, APP_AFLAGS_START);
        APP_ADDFLAGS(app_misc_calibupdate.GFlags, APP_AFLAGS_READY);

        misc_calibupdate.Func(MISC_CALIBUPDATE_START, 0, 0);
    }

    return ReturnValue;
}

static int app_misc_calibupdate_stop(void)
{
    int ReturnValue = 0;

    ReturnValue = misc_calibupdate.Func(MISC_CALIBUPDATE_STOP, 0, 0);

    return ReturnValue;
}
