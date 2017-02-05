/**
  * @file src/app/apps/gui/usb/connectedcam/gui_usb_msc.c
  *
  *  Implementation of USB MSC GUI display flows
  *
  * History:
  *    2013/12/02 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include <apps/gui/usb/gui_usb_msc.h>
#include <apps/gui/resource/gui_settle.h>
#include <wchar.h>

/**
 *  @brief The GUI functions of application
 *
 *  The GUI functions of application
 *
 *  @param [in] guiCmd Command ID
 *  @param [in] param1 first parameter
 *  @param [in] param2 second parameter
 *
 *  @return >=0 success, <0 failure
 */
int gui_usb_msc_func(UINT32 guiCmd, UINT32 param1, UINT32 param2)
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
        {
            WCHAR str[20] = {'U','S','B',' ','M','a','s','s',' ','S','t','o','r','a','g','e','\0'};
            AppLibGraph_UpdateStringContext(0, STR_USB_CONNECTING, (UINT16*)str);
            AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_USB_MSG, STR_USB_CONNECTING);
            AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_USB_MSG);
        }
        break;
    case GUI_APP_ICON_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_USB_MSG);
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
