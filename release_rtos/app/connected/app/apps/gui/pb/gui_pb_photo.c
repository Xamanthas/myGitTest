/**
  * @file src/app/apps/gui/pb/connectedcam/gui_pb_photo.c
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
#include <apps/gui/pb/gui_pb_photo.h>
#include <wchar.h>
#include <apps/gui/resource/gui_settle.h>


static int set_filename(WCHAR *fn, GUI_UTILITY_FILENAME_STYLE_e style)
{
    UINT16 GuiFilename[GUI_FILENAME_SIZE] = {0};

    AppGuiUtil_GetFilenameStrings((WCHAR*)GuiFilename, fn, style);
    AppLibGraph_UpdateStringContext(0, STR_FILENAME, GuiFilename);
    AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_PB_FILENAME, STR_FILENAME);

    return 0;
}

/**
 *  @brief The GUI functions of video playback application
 *
 *  The GUI functions of video playback application
 *
 *  @param [in] guiCmd Command ID
 *  @param [in] param1 First parameter
 *  @param [in] param2 Second parameter
 *
 *  @return >=0 success, <0 failure
 */
int gui_pb_photo_func(UINT32 guiCmd, UINT32 param1, UINT32 param2)
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
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_APP_ICON, BMP_BTN_MODE_PLAY);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_APP_ICON);
        break;
    case GUI_APP_ICON_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_APP_ICON);
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
    case GUI_FILENAME_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_PB_FILENAME);
        break;
    case GUI_FILENAME_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_PB_FILENAME);
        break;
    case GUI_FILENAME_UPDATE:
        set_filename((WCHAR *)param1, (GUI_UTILITY_FILENAME_STYLE_e)param2);
        break;
    case GUI_MEDIA_INFO_SHOW:
        break;
    case GUI_MEDIA_INFO_HIDE:
        break;
    case GUI_MEDIA_INFO_UPDATE:
        break;
    case GUI_ZOOMBAR_SHOW:
    case GUI_ZOOMBAR_HIDE:
    case GUI_ZOOMBAR_UPDATE:
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
    default:
        ReturnValue = -1;
        break;
    }

    return ReturnValue;
}
