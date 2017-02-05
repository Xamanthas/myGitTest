/**
  * @file src/app/apps/flow/thumb/connectedcam/thumb_motion_op.c
  *
  * Operations of Player Thumbnail Basic View
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
#include <apps/flow/thumb/thumb_motion.h>
#include <system/app_util.h>
#include <system/status.h>

/*************************************************************************
 * App Function Declarations (static)
 ************************************************************************/
static int thumb_motion_button_record(void);
static int thumb_motion_button_focus(void);
static int thumb_motion_button_focus_clr(void);
static int thumb_motion_button_shutter(void);
static int thumb_motion_button_shutter_clr(void);
static int thumb_motion_button_zoom_in(void);
static int thumb_motion_button_zoom_in_clr(void);
static int thumb_motion_button_zoom_out(void);
static int thumb_motion_button_zoom_out_clr(void);
static int thumb_motion_button_up(void);
static int thumb_motion_button_down(void);
static int thumb_motion_button_left(void);
static int thumb_motion_button_right(void);
static int thumb_motion_button_set(void);
static int thumb_motion_button_menu(void);
static int thumb_motion_button_mode(void);
static int thumb_motion_button_del(void);
static int thumb_motion_button_power(void);

THUMB_MOTION_OP_s thumb_motion_op = {
    thumb_motion_button_record,
    thumb_motion_button_focus,
    thumb_motion_button_focus_clr,
    thumb_motion_button_shutter,
    thumb_motion_button_shutter_clr,
    thumb_motion_button_zoom_in,
    thumb_motion_button_zoom_in_clr,
    thumb_motion_button_zoom_out,
    thumb_motion_button_zoom_out_clr,
    thumb_motion_button_up,
    thumb_motion_button_down,
    thumb_motion_button_left,
    thumb_motion_button_right,
    thumb_motion_button_set,
    thumb_motion_button_menu,
    thumb_motion_button_mode,
    thumb_motion_button_del,
    thumb_motion_button_power
};

/**
 *  @brief The operation of Record button.
 *
 *  The operation of Record button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_record(void)
{
    int ReturnValue = 0;
    int tab_switch_temp = thumb_motion.TabCur;
    /** current tab shift */
    tab_switch_temp++;
    if (tab_switch_temp == thumb_motion.TabNum) {
        tab_switch_temp = THUMB_MOTION_TAB_VIDEO;
    }
    thumb_motion.Func(THUMB_MOTION_SHIFT_TAB, tab_switch_temp, 0);
    return ReturnValue;
}

/**
 *  @brief The operation of Focus button.
 *
 *  The operation of Focus button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_focus(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Focus button release.
 *
 *  The operation of Focus button release.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_focus_clr(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Shutter button.
 *
 *  The operation of Shutter button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_shutter(void)
    {
    int ReturnValue = 0;

    if (app_status.PlaybackType == DCIM_HDLR)
        app_status.PlaybackType = EVENTRECORD_HDLR;
    else
        app_status.PlaybackType = DCIM_HDLR;

    thumb_motion.Func(THUMB_MOTION_START_DISP_PAGE, 0, 0);

    return ReturnValue;
}

/**
 *  @brief The operation of Shutter button release.
 *
 *  The operation of Shutter button release.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_shutter_clr(void)
{
    int ReturnValue = 0;


    return ReturnValue;
}

/**
 *  @brief The operation of Zoom_in button.
 *
 *  The operation of Zoom_in button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_zoom_in(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Zoom_in button release.
 *
 *  The operation of Zoom_in button release.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_zoom_in_clr(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Zoom_out button.
 *
 *  The operation of Zoom_out button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_zoom_out(void)
{
    int ReturnValue = 0;
    return ReturnValue;
}

/**
 *  @brief The operation of Zoom_out button release.
 *
 *  The operation of Zoom_out button release.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_zoom_out_clr(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Up button.
 *
 *  The operation of Up button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_up(void)
{
    int ReturnValue = 0;

    if (thumb_motion.FileInfo.TotalFileNum > 0) {
        if (thumb_motion.FileInfo.PageItemCur >= thumb_motion.DispCol) {
            ReturnValue = thumb_motion.Func(THUMB_MOTION_SHIFT_FILE_TO_PREV, thumb_motion.DispCol, 0);
            //thumb_motion.Func(THUMB_MOTION_SHOW_PAGE_INFO, 0, 0);
        }
    }

    return ReturnValue;
}

/**
 *  @brief The operation of Down button.
 *
 *  The operation of Down button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_down(void)
{
    int ReturnValue = 0;

    if (thumb_motion.FileInfo.TotalFileNum > 0) {
        if (thumb_motion.FileInfo.PageItemCur < thumb_motion.DispCol) {
            ReturnValue = thumb_motion.Func(THUMB_MOTION_SHIFT_FILE_TO_NEXT, thumb_motion.DispCol, 0);
            //thumb_motion.Func(THUMB_MOTION_SHOW_PAGE_INFO, 0, 0);
        }
    }

    return ReturnValue;
}

/**
 *  @brief The operation of Left button.
 *
 *  Left button operation
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_left(void)
{
    int ReturnValue = 0;

    /** page current item shift */
    if (thumb_motion.FileInfo.TotalFileNum > 0) {
        ReturnValue = thumb_motion.Func(THUMB_MOTION_SHIFT_FILE_TO_PREV, 1, 0);

        //thumb_motion.Func(THUMB_MOTION_SHOW_PAGE_INFO, 0, 0);
    }

    return ReturnValue;
}

/**
 *  @brief The operation of Right button.
 *
 *  The operation of Right button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_right(void)
{
    int ReturnValue = 0;

    /** page current item shift */
    if (thumb_motion.FileInfo.TotalFileNum > 0) {
        ReturnValue = thumb_motion.Func(THUMB_MOTION_SHIFT_FILE_TO_NEXT, 1, 0);

        //thumb_motion.Func(THUMB_MOTION_SHOW_PAGE_INFO, 0, 0);

    }

    return ReturnValue;
}

/**
 *  @brief The operation of Set button.
 *
 *  The operation of Set button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_set(void)
{
    int ReturnValue = 0;

    if (thumb_motion.FileInfo.TotalFileNum > 0) {
        APPLIB_MEDIA_INFO_s MediaInfo;
        ReturnValue = AppLibFormat_GetMediaInfo(thumb_motion.CurFn, &MediaInfo);
        if (ReturnValue == AMP_OK) {
#if defined (ENABLE_HDMI_TEST)
            ReturnValue = AppUtil_SwitchApp(APP_HDMI_TEST);
#else
            ReturnValue = AppUtil_SwitchApp(APP_PB_MULTI);
#endif
        }
    }

    return ReturnValue;
}

/**
 *  @brief The operation of Menu button.
 *
 *  The operation of Menu button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_menu(void)
{
    int ReturnValue = 0;
    if (!APP_CHECKFLAGS(app_thumb_motion.GFlags, APP_AFLAGS_POPUP)) {
        thumb_motion.Func(THUMB_MOTION_WARNING_MSG_SHOW_STOP, 0, 0);
        APP_ADDFLAGS(app_thumb_motion.GFlags, APP_AFLAGS_POPUP);
        ReturnValue = AppWidget_On(WIDGET_MENU, 0);
    }
    return ReturnValue;
}

/**
 *  @brief The operation of Mode button.
 *
 *  The operation of Mode button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_mode(void)
{
    int ReturnValue = 0;
    if (app_status.LockDecMode && app_status.FchanDecModeOnly) {
        AmbaPrint("[app_thumb_motion] Mode button is locked.");
    } else {
        AppUtil_SwitchMode(0);
    }

    return ReturnValue;
}

/**
 *  @brief The operation of Delete button.
 *
 *  The operation of Delete button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_del(void)
{
    int ReturnValue = 0;

    if (thumb_motion.FileInfo.TotalFileNum > 0) {
        thumb_motion.Func(THUMB_MOTION_SELECT_DELETE_FILE_MODE, 0, 0);
    }
    return ReturnValue;
}

/**
 *  @brief The operation of Power button.
 *
 *  The operation of Power button.
 *
 *  @return >=0 success, <0 failure
 */
static int thumb_motion_button_power(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}
