/**
  * @file src/app/apps/flow/misc/connectedcam/misc_defsetting_func.c
  *
  *  Functions of Reset the system setting to default application
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


#include <apps/flow/misc/misc_defsetting.h>
#include <system/status.h>
#include <system/app_pref.h>
#include <system/app_util.h>
#include <apps/gui/resource/gui_settle.h>
#include <apps/flow/widget/menu/widget.h>

static int misc_defsetting_start(void)
{
    int ReturnValue = 0;

    misc_defsetting.Func(MISC_DEFSETTING_DIALOG_SHOW_DEFSETTING, 0, 0);

    return ReturnValue;
}


static int misc_defsetting_stop(void)
{
    int ReturnValue = 0;

    if (APP_CHECKFLAGS(app_misc_defsetting.GFlags, APP_AFLAGS_POPUP)) {
        APP_REMOVEFLAGS(app_misc_defsetting.GFlags, APP_AFLAGS_POPUP);
        ReturnValue = AppWidget_Off(WIDGET_DIALOG, 0);
    }

    return ReturnValue;
}

static int default_dialog_ok_handler(UINT32 sel, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;
    APP_REMOVEFLAGS(app_misc_defsetting.GFlags, APP_AFLAGS_POPUP);
    switch (sel) {
    default:
    case DIALOG_SEL_OK:
        AppUtil_SwitchApp(AppUtil_GetStartApp(0));
        break;
    }

    return ReturnValue;
}

static int default_dialog_default_handler(UINT32 sel, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;

    APP_REMOVEFLAGS(app_misc_defsetting.GFlags, APP_AFLAGS_POPUP);
    switch (sel) {
    case DIALOG_SEL_YES:
        /* Set the "initial version", system will refresh the preference when next boot.*/
        UserSetting->SystemPref.SystemVersion = 0;
        /* Save the system preference.  */
        AppPref_Save();

        ReturnValue = AppDialog_SetDialog(DIALOG_TYPE_OK, DIALOG_SUB_DEFAULT_SETTING_REBOOT, default_dialog_ok_handler);
        ReturnValue = AppWidget_On(WIDGET_DIALOG, 0);
        APP_ADDFLAGS(app_misc_defsetting.GFlags, APP_AFLAGS_POPUP);
        break;
    default:
    case DIALOG_SEL_NO:
        AppUtil_SwitchApp(AppUtil_GetStartApp(0));
        break;
    }

    return ReturnValue;
}

static int misc_defsetting_dialog_show_defsetting(void)
{
    int ReturnValue = 0;

    ReturnValue = AppDialog_SetDialog(DIALOG_TYPE_Y_N, DIALOG_SUB_DEFAULT_SETTING_YES_OR_NO, default_dialog_default_handler);
    ReturnValue = AppWidget_On(WIDGET_DIALOG, 0);
    APP_ADDFLAGS(app_misc_defsetting.GFlags, APP_AFLAGS_POPUP);

    return ReturnValue;
}

int misc_defsetting_func(UINT32 funcId, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;

    switch (funcId) {
    case MISC_DEFSETTING_START:
        misc_defsetting_start();
        break;
    case MISC_DEFSETTING_STOP:
        misc_defsetting_stop();
        break;
    case MISC_DEFSETTING_DIALOG_SHOW_DEFSETTING:
        misc_defsetting_dialog_show_defsetting();
        break;
    default:
        AmbaPrint("[app_misc_defsetting] The function is not defined");
        break;
    }

    return ReturnValue;
}
