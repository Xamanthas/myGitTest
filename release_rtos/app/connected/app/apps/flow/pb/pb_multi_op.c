/**
  * @file src/app/apps/flow/pb/connectedcam/pb_multi_op.c
  *
  * Operations of multi playback application
  *
  * History:
  *    2014/09/11 - [Annie Ting] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include <apps/flow/pb/pb_multi.h>
#include <system/app_util.h>
#include <system/status.h>
#include <wchar.h>

/*************************************************************************
 * App Function Declarations (static)
 ************************************************************************/
static int pb_multi_button_record(void);
static int pb_multi_button_focus(void);
static int pb_multi_button_focus_clr(void);
static int pb_multi_button_shutter(void);
static int pb_multi_button_shutter_clr(void);
static int pb_multi_button_zoom_in(void);
static int pb_multi_button_zoom_in_clr(void);
static int pb_multi_button_zoom_out(void);
static int pb_multi_button_zoom_out_clr(void);
static int pb_multi_button_up(void);
static int pb_multi_button_down(void);
static int pb_multi_button_left(void);
static int pb_multi_button_right(void);
static int pb_multi_button_set(void);
static int pb_multi_button_menu(void);
static int pb_multi_button_mode(void);
static int pb_multi_button_del(void);
static int pb_multi_button_power(void);

PB_MULTI_OP_s pb_multi_op = {
    pb_multi_button_record,
    pb_multi_button_focus,
    pb_multi_button_focus_clr,
    pb_multi_button_shutter,
    pb_multi_button_shutter_clr,
    pb_multi_button_zoom_in,
    pb_multi_button_zoom_in_clr,
    pb_multi_button_zoom_out,
    pb_multi_button_zoom_out_clr,
    pb_multi_button_up,
    pb_multi_button_down,
    pb_multi_button_left,
    pb_multi_button_right,
    pb_multi_button_set,
    pb_multi_button_menu,
    pb_multi_button_mode,
    pb_multi_button_del,
    pb_multi_button_power
};

/**
 *  @brief The operation of Record button.
 *
 *  The operation of Record button.
 *
 *  @return >=0 success, <0 failure
 */
static int pb_multi_button_record(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Focus button.
 *
 *  The operation of Focus button.
 *
 *  @return >=0 success, <0 failure
 */
static int pb_multi_button_focus(void)
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
static int pb_multi_button_focus_clr(void)
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
static int pb_multi_button_shutter(void)
    {
    int ReturnValue = 0;

    return ReturnValue;
}

/**
 *  @brief The operation of Shutter button release.
 *
 *  The operation of Shutter button release.
 *
 *  @return >=0 success, <0 failure
 */
static int pb_multi_button_shutter_clr(void)
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
static int pb_multi_button_zoom_in(void)
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
static int pb_multi_button_zoom_in_clr(void)
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
static int pb_multi_button_zoom_out(void)
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
static int pb_multi_button_zoom_out_clr(void)
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
static int pb_multi_button_up(void)
{
    int ReturnValue = 0;

    if (APP_CHECKFLAGS(app_pb_multi.GFlags, APP_AFLAGS_BUSY)) {
        /** video : pause */
        if (pb_multi.CurFileType == PB_MULTI_MEDIA_VIDEO) {
            pb_multi.MediaInfo.State = PB_MULTI_PLAY_PAUSED;
            pb_multi.Func(PB_MULTI_PLAY, 0, 1);
            if (!APP_CHECKFLAGS(app_pb_multi.GFlags, APP_AFLAGS_BUSY)) {
                /* The busy flag will be removed when the flow stop the video player. */
                /* To excute the functions that system block them when the Busy flag is enabled. */
                AppUtil_BusyCheck(0);
            }
            if (APP_CHECKFLAGS(app_pb_multi.GFlags, APP_AFLAGS_READY)) {
                /* The system could switch the current app to other in the function "AppUtil_BusyCheck". */
                pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_PAUSE, 0);
                pb_multi.Gui(GUI_FLUSH, 0, 0);
            }
        } else {
            APP_ADDFLAGS(app_pb_multi.Flags, PB_MULTI_OP_BLOCKED);
            pb_multi.OpBlocked = pb_multi_button_up;
        }
    } else {
        if (pb_multi.FileInfo.TotalFileNum > 0) {
            if (pb_multi.CurFileType == PB_MULTI_MEDIA_VIDEO) {
                ReturnValue = AppLibVideoDec_Step();
                if (pb_multi.MediaInfo.Direction == PB_MULTI_PLAY_REV) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_STEP, 0);
                } else {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_STEP, 0);
                }
                pb_multi.Gui(GUI_FLUSH, 0, 0);
                AmbaKAL_TaskSleep(200);
                pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_PAUSE, 0);
                pb_multi.Gui(GUI_FLUSH, 0, 0);
            } else {
                /**Wait photo show end*/
                AppLibStillSingle_WaitShow(pb_multi.StillDecWaitFlag);
                ReturnValue = pb_multi.Func(PB_MULTI_SWITCH_APP, 0, 0);
            }
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
static int pb_multi_button_down(void)
{
    int ReturnValue = 0;
    if (pb_multi.CurFileType == PB_MULTI_MEDIA_VIDEO) {
        if (APP_CHECKFLAGS(app_pb_multi.GFlags, APP_AFLAGS_BUSY)) {
            strcpy(pb_multi.FirstFn, pb_multi.CurFn);
            if (pb_multi.MediaInfo.Direction == PB_MULTI_PLAY_FWD) {
                if (pb_multi.CurPlayTime <= 0) {
                    AmbaPrint("[pb_multi] <button_down> Current play time equal 0. Can not do backward play.");
                    return ReturnValue;
                }
                pb_multi.MediaInfo.Direction = PB_MULTI_PLAY_REV;
            } else {
                if (pb_multi.CurPlayTime >= pb_multi.MediaInfo.TotalTime) {
                    AmbaPrint("[pb_multi] <button_down> Current play time equal total time. Can not do forward play.");
                    return ReturnValue;
                }
                pb_multi.MediaInfo.Direction = PB_MULTI_PLAY_FWD;
            }
            pb_multi.MediaInfo.State = PB_MULTI_PLAY_PLAY;
            ReturnValue = pb_multi.Func(PB_MULTI_PLAY, 0, 1);
            if (pb_multi.MediaInfo.Direction == PB_MULTI_PLAY_REV) {
                if (pb_multi.MediaInfo.Speed == PBACK_SPEED_NORMAL) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_NORMAL, 0);
                } else if (pb_multi.MediaInfo.Speed > PBACK_SPEED_NORMAL) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_FAST, pb_multi.MediaInfo.Speed);
                } else {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_SLOW, pb_multi.MediaInfo.Speed);
                }
            } else {
                if (pb_multi.MediaInfo.Speed == PBACK_SPEED_NORMAL) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_NORMAL, 0);
                } else if (pb_multi.MediaInfo.Speed > PBACK_SPEED_NORMAL) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_FAST, pb_multi.MediaInfo.Speed);
                } else {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_SLOW, pb_multi.MediaInfo.Speed);
                }
            }
            pb_multi.Gui(GUI_FLUSH, 0, 0);
        } else {
            if (pb_multi.FileInfo.TotalFileNum > 0) {
                strcpy(pb_multi.FirstFn, pb_multi.CurFn);
                if (pb_multi.MediaInfo.Direction == PB_MULTI_PLAY_FWD) {
                    pb_multi.MediaInfo.Direction = PB_MULTI_PLAY_REV;
                } else {
                    pb_multi.MediaInfo.Direction = PB_MULTI_PLAY_FWD;
                }
                pb_multi.MediaInfo.State = PB_MULTI_PLAY_PLAY;
                pb_multi.MediaInfo.Speed = PBACK_SPEED_NORMAL;
                pb_multi.Func(PB_MULTI_PLAY, 0, 1);
                if (pb_multi.MediaInfo.Direction == PB_MULTI_PLAY_REV) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_NORMAL, 0);
                } else {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_NORMAL, 0);
                }
                pb_multi.Gui(GUI_FLUSH, 0, 0);
            }
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
static int pb_multi_button_left(void)
{
    int ReturnValue = 0;

    if (APP_CHECKFLAGS(app_pb_multi.GFlags, APP_AFLAGS_BUSY)) {
        if (pb_multi.CurFileType == PB_MULTI_MEDIA_VIDEO) {
            pb_multi.MediaInfo.Speed >>= 1;
            AppLibVideoDec_SpeedDown(&pb_multi.MediaInfo.Speed);
            if (pb_multi.MediaInfo.Direction == PB_MULTI_PLAY_REV) {
                if (pb_multi.MediaInfo.Speed == PBACK_SPEED_NORMAL) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_NORMAL, 0);
                } else if (pb_multi.MediaInfo.Speed > PBACK_SPEED_NORMAL) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_FAST, pb_multi.MediaInfo.Speed);
                } else {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_SLOW, pb_multi.MediaInfo.Speed);
                }
            } else {
                if (pb_multi.MediaInfo.Speed == PBACK_SPEED_NORMAL) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_NORMAL, 0);
                } else if (pb_multi.MediaInfo.Speed > PBACK_SPEED_NORMAL) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_FAST, pb_multi.MediaInfo.Speed);
                } else {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_SLOW, pb_multi.MediaInfo.Speed);
                }
            }
            pb_multi.Gui(GUI_FLUSH, 0, 0);
        }
    } else {
        if (pb_multi.FileInfo.TotalFileNum > 0) {
            if (ReturnValue == 0) {
                pb_multi.Func(PB_MULTI_STOP_PLAYING, 0, 0);
                ReturnValue = pb_multi.Func(PB_MULTI_GET_FILE, GET_PREV_FILE, pb_multi.FileInfo.MediaRoot);
                if (pb_multi.CurFileType == PB_MULTI_MEDIA_VIDEO) {
                    strcpy(pb_multi.FirstFn, pb_multi.CurFn);
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_PAUSE, 0);
                    pb_multi.Gui(GUI_PLAY_TIMER_UPDATE, 0, pb_multi.MediaInfo.TotalTime/1000);
                    pb_multi.Gui(GUI_PLAY_STATE_SHOW, 0, 0);
                    pb_multi.Gui(GUI_PLAY_SPEED_SHOW, 0, 0);
                    ReturnValue = pb_multi.Func(PB_MULTI_OPEN, PB_MULTI_OPEN_RESET, 0);
                    pb_multi.MediaInfo.State = PB_MULTI_PLAY_PLAY;
                    pb_multi.Func(PB_MULTI_PLAY, 0, 1);/**<param2 : 0 photo : 1 video */
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_NORMAL, 0);
                } else if (pb_multi.CurFileType == PB_MULTI_MEDIA_IMAGE) {
                    pb_multi.Func(PB_MULTI_PLAY, 0, 0);
                }
            } else {
                ReturnValue = pb_multi.Func(PB_MULTI_SWITCH_APP, 0, 0);
            }

        }
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
static int pb_multi_button_right(void)
{
        int ReturnValue = 0;

        if (APP_CHECKFLAGS(app_pb_multi.GFlags, APP_AFLAGS_BUSY)) {
            if (pb_multi.CurFileType == PB_MULTI_MEDIA_VIDEO) {
                pb_multi.MediaInfo.Speed <<= 1;
                AppLibVideoDec_SpeedUp(&pb_multi.MediaInfo.Speed);
                if (pb_multi.MediaInfo.Direction == PB_MULTI_PLAY_REV) {
                    if (pb_multi.MediaInfo.Speed == PBACK_SPEED_NORMAL) {
                        pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_NORMAL, 0);
                    } else if (pb_multi.MediaInfo.Speed > PBACK_SPEED_NORMAL) {
                        pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_FAST, pb_multi.MediaInfo.Speed);
                    } else {
                        pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_SLOW, pb_multi.MediaInfo.Speed);
                    }
                } else {
                    if (pb_multi.MediaInfo.Speed == PBACK_SPEED_NORMAL) {
                        pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_NORMAL, 0);
                    } else if (pb_multi.MediaInfo.Speed > PBACK_SPEED_NORMAL) {
                        pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_FAST, pb_multi.MediaInfo.Speed);
                    } else {
                        pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_SLOW, pb_multi.MediaInfo.Speed);
                    }
                }
                pb_multi.Gui(GUI_FLUSH, 0, 0);
            }
        } else {
            if (pb_multi.FileInfo.TotalFileNum > 0) {
                pb_multi.Func(PB_MULTI_STOP_PLAYING, 0, 0);
                ReturnValue = pb_multi.Func(PB_MULTI_GET_FILE, GET_NEXT_FILE, pb_multi.FileInfo.MediaRoot);
                if (ReturnValue == 0) {
                    if (pb_multi.CurFileType == PB_MULTI_MEDIA_VIDEO) {
                        strcpy(pb_multi.FirstFn, pb_multi.CurFn);
                        pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_PAUSE, 0);
                        pb_multi.Gui(GUI_PLAY_TIMER_UPDATE, 0, pb_multi.MediaInfo.TotalTime/1000);
                        pb_multi.Gui(GUI_PLAY_STATE_SHOW, 0, 0);
                        pb_multi.Gui(GUI_PLAY_SPEED_SHOW, 0, 0);
                        ReturnValue = pb_multi.Func(PB_MULTI_OPEN, PB_MULTI_OPEN_RESET, 0);
                        pb_multi.MediaInfo.State = PB_MULTI_PLAY_PLAY;
                        pb_multi.Func(PB_MULTI_PLAY, 0, 1);/**<param2 : 0 photo : 1 video */
                        pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_NORMAL, 0);
                    } else if (pb_multi.CurFileType == PB_MULTI_MEDIA_IMAGE) {
                        pb_multi.Func(PB_MULTI_PLAY, 0, 0);

                    }
                } else {
                    ReturnValue = pb_multi.Func(PB_MULTI_SWITCH_APP, 0, 0);
                }
            }
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
static int pb_multi_button_set(void)
{
    int ReturnValue = 0;
    APPLIB_VIDEO_START_MULTI_INFO_s VideoStartInfo;
    APPLIB_VIDEO_FILE_INFO_s File;

    if (pb_multi.CurFileType == PB_MULTI_MEDIA_VIDEO) {
        if (APP_CHECKFLAGS(app_pb_multi.GFlags, APP_AFLAGS_BUSY)) {
            if (pb_multi.MediaInfo.Speed == PBACK_SPEED_NORMAL) {
                pb_multi.MediaInfo.State = PB_MULTI_PLAY_PAUSED;
                APP_REMOVEFLAGS(app_pb_multi.GFlags, APP_AFLAGS_BUSY);
                AppLibVideoDec_Stop();
                ReturnValue = pb_multi.Func(PB_MULTI_SWITCH_APP, 0, 0);
            } else {
                pb_multi.MediaInfo.Speed = PBACK_SPEED_NORMAL;
                File.Filename = pb_multi.CurFn;
                VideoStartInfo.File = &File;
                VideoStartInfo.FileNum= 1;
                VideoStartInfo.AutoPlay = 1;
                VideoStartInfo.Direction = (APPLIB_VIDEO_PLAY_DIRECTION_e)pb_multi.MediaInfo.Direction;
                VideoStartInfo.ResetSpeed = 1;
                VideoStartInfo.ResetZoom = 0;
                VideoStartInfo.StartTime = pb_multi.MediaInfo.PlayTime;
                VideoStartInfo.ReloadFile = 0;
                ReturnValue = AppLibVideoDec_StartMultiple(&VideoStartInfo);
                if (pb_multi.MediaInfo.Direction == PB_MULTI_PLAY_REV) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_NORMAL, 0);
                } else {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_NORMAL, 0);
                }
                pb_multi.Gui(GUI_FLUSH, 0, 0);
            }
        } else {
            if (pb_multi.FileInfo.TotalFileNum > 0) {
                strcpy(pb_multi.FirstFn, pb_multi.CurFn);
                pb_multi.MediaInfo.State = PB_MULTI_PLAY_PLAY;
                pb_multi.MediaInfo.Speed = PBACK_SPEED_NORMAL;
                ReturnValue = pb_multi.Func(PB_MULTI_PLAY, 1, 1);
                if (pb_multi.MediaInfo.Direction == PB_MULTI_PLAY_REV) {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_REW_NORMAL, 0);
                } else {
                    pb_multi.Gui(GUI_PLAY_STATE_UPDATE, GUI_FWD_NORMAL, 0);
                }
                pb_multi.Gui(GUI_FLUSH, 0, 0);
            }
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
static int pb_multi_button_menu(void)
{
    int ReturnValue = 0;

    if (!APP_CHECKFLAGS(app_pb_multi.GFlags, APP_AFLAGS_POPUP)) {
        pb_multi.Func(PB_MULTI_WARNING_MSG_SHOW_STOP, 0, 0);
        APP_ADDFLAGS(app_pb_multi.GFlags, APP_AFLAGS_POPUP);
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
static int pb_multi_button_mode(void)
{
    int ReturnValue = 0;
    if (app_status.LockDecMode && app_status.FchanDecModeOnly) {
        AmbaPrint("[app_pb_multi] Mode button is locked.");
    } else {
        if (pb_multi.CurFileType == PB_MULTI_MEDIA_VIDEO) {
            APP_REMOVEFLAGS(app_pb_multi.GFlags, APP_AFLAGS_BUSY);
            AppLibVideoDec_Stop();
        }
        pb_multi.Func(PB_MULTI_SWITCH_APP, 1, 0);
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
static int pb_multi_button_del(void)
{
    int ReturnValue = 0;
    if (APP_CHECKFLAGS(app_pb_multi.GFlags, APP_AFLAGS_BUSY)) {
        APP_ADDFLAGS(app_pb_multi.Flags, PB_MULTI_OP_BLOCKED);
        pb_multi.OpBlocked = pb_multi_button_del;
    } else {
        if (pb_multi.FileInfo.TotalFileNum > 0) {
            pb_multi.Func(PB_MULTI_DELETE_FILE_DIALOG_SHOW, 0, 0);
        }
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
static int pb_multi_button_power(void)
{
    int ReturnValue = 0;

    return ReturnValue;
}
