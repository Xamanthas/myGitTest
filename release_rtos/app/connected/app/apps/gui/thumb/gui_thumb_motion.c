/**
  * @file src/app/apps/gui/pb/connectedcam/gui_thumb_motion.c
  *
  *  Implementation of photo playback GUI display flows
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
#include <apps/gui/thumb/gui_thumb_motion.h>
#include <apps/gui/resource/gui_settle.h>
#include <wchar.h>

#define GOBJ_MEDIA_INFO_RES        (GOBJ_FV_ICON_LEFT_1)
#define GOBJ_MEDIA_INFO_FRATE    (GOBJ_FV_ICON_LEFT_2)


static int set_filename(WCHAR *fn, GUI_UTILITY_FILENAME_STYLE_e style)
{
    UINT16 GuiFilename[GUI_FILENAME_SIZE] = {0};

    AppGuiUtil_GetFilenameStrings((WCHAR*)GuiFilename, fn, style);
    AppLibGraph_UpdateStringContext(0, STR_FILENAME, GuiFilename);
    AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_THUMB_FILENAME, STR_FILENAME);

    return 0;
}

#if 0
static int set_media_info(dec_media_info_t *media_info)
{
    switch (media_info->type) {
    case MEDIA_TYPE_MOV:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_MEDIA_INFO_RES,
                               gutil_get_video_res_bmp(media_info->media.movi.vid_width,
                                                       media_info->media.movi.vid_height));
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_MEDIA_INFO_FRATE,
                               gutil_get_video_ar_frate_bmp(media_info->media.movi.aspect_ratio,
                                                            media_info->media.movi.mode,
                                                            media_info->media.movi.vid_frame_rate));
        break;
    case MEDIA_TYPE_IMG:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_MEDIA_INFO_RES, BMP_0_NULL);
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_MEDIA_INFO_FRATE, BMP_0_NULL);
        break;
    default:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_MEDIA_INFO_RES, BMP_0_NULL);
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_MEDIA_INFO_FRATE, BMP_0_NULL);
        break;
    }
    return 0;
}
#endif

/**
 *  @brief The GUI functions of video playback application
 *
 *  The GUI functions of video playback application
 *
 *  @param [in] guiCmd Command ID
 *  @param [in] param1 first parameter
 *  @param [in] param2 second parameter
 *
 *  @return >=0 success, <0 failure
 */
static int TabHighlight = -1;

int gui_thumb_motion_func(UINT32 guiCmd, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;
    switch (guiCmd) {
    case GUI_FLUSH:
        AppLibGraph_Draw(GRAPH_CH_DUAL);
        break;
    case GUI_HIDE_ALL:
        AppLibGraph_HideAll(GRAPH_CH_DUAL);
        break;
    case GUI_SET_LAYOUT:
        AppLibGraph_SetGUILayout(GRAPH_CH_DCHAN, Gui_Resource_Dchan_Id, Gui_Table_Dchan, 0);
        AppLibGraph_SetGUILayout(GRAPH_CH_FCHAN, Gui_Resource_Fchan_Id, Gui_Table_Fchan, 0);
        break;
    case GUI_APP_ICON_SHOW:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_APP_ICON, BMP_BTN_MODE_THUMBNAIL);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_APP_ICON);
        break;
    case GUI_APP_ICON_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_APP_ICON);
        break;
    case GUI_FRAME_SHOW:
        AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_THUMB_SEP_UP);
        AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_THUMB_SEP_LEFT);
        AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_THUMB_GLOW_LEFT);
        AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_THUMB_GLOW_RIGHT);
        break;
    case GUI_FRAME_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_THUMB_SEP_UP);
        AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_THUMB_SEP_LEFT);
        AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_THUMB_GLOW_LEFT);
        AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_THUMB_GLOW_RIGHT);
        break;
    case GUI_POWER_STATE_SHOW:
        AppGuiUtil_PowerIconShow(param1, param2);
        break;
    case GUI_POWER_STATE_HIDE:
        AppGuiUtil_PowerIconHide(param1, param2);
        break;
    case GUI_POWER_STATE_UPDATE:
        AppGuiUtil_PowerIconUpdate(param1, param2);
        break;
    case GUI_CARD_SHOW:
        AppGuiUtil_CardIconShow(param1, param2);
        break;
    case GUI_CARD_HIDE:
        AppGuiUtil_CardIconHide(param1, param2);
        break;
    case GUI_CARD_UPDATE:
        AppGuiUtil_CardIconUpdate(param1);
        break;
    case GUI_WARNING_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_WARNING);
        break;
    case GUI_WARNING_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_WARNING);
        break;
    case GUI_WARNING_UPDATE:
        AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_WARNING, GuiWarningTable[param1].str);
        break;
    case GUI_TAB_SHOW:
        AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_THUMB_3_TAB_BASE);
        if (TabHighlight > 0) {
            AppLibGraph_Show(GRAPH_CH_DCHAN, TabHighlight);
        }
        break;
    case GUI_TAB_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_THUMB_3_TAB_BASE);
        if (TabHighlight > 0) {
            AppLibGraph_Hide(GRAPH_CH_DCHAN, TabHighlight);
        }
        break;
    case GUI_TAB_UPDATE:
        if (TabHighlight > 0) {
            AppLibGraph_Hide(GRAPH_CH_DCHAN, TabHighlight);
        }
        TabHighlight = GOBJ_THUMB_3_TAB_HL_1 + param1;
        if (TabHighlight > 0) {
            AppLibGraph_Show(GRAPH_CH_DCHAN, TabHighlight);
        }
        break;
    case GUI_DEL_SHOW:
    case GUI_DEL_HIDE:
    case GUI_DEL_UPDATE:
        break;
    case GUI_PROTECT_SHOW:
    case GUI_PROTECT_HIDE:
    case GUI_PROTECT_UPDATE:
        break;
    case GUI_FILENAME_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_THUMB_FILENAME);
        break;
    case GUI_FILENAME_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_THUMB_FILENAME);
        break;
    case GUI_FILENAME_UPDATE:
        set_filename((WCHAR *)param1, (GUI_UTILITY_FILENAME_STYLE_e)param2);
        break;
    case GUI_MEDIA_INFO_SHOW:
        AppLibGraph_Show(GRAPH_CH_FCHAN, GOBJ_MEDIA_INFO_RES);
        AppLibGraph_Show(GRAPH_CH_FCHAN, GOBJ_MEDIA_INFO_FRATE);
        break;
    case GUI_MEDIA_INFO_HIDE:
        AppLibGraph_Hide(GRAPH_CH_FCHAN, GOBJ_MEDIA_INFO_RES);
        AppLibGraph_Hide(GRAPH_CH_FCHAN, GOBJ_MEDIA_INFO_FRATE);
        break;
    case GUI_MEDIA_INFO_UPDATE:
        //set_media_info(param2);
        break;
    case GUI_PHOTO_SIZE_SHOW:
    case GUI_PHOTO_SIZE_HIDE:
    case GUI_PHOTO_SIZE_UPDATE:
    case GUI_VIDEO_SENSOR_RES_SHOW:
    case GUI_VIDEO_SENSOR_RES_HIDE:
    case GUI_VIDEO_SENSOR_RES_UPDATE:
        break;
    default:
        ReturnValue = -1;
        break;
    }

    return ReturnValue;
}
