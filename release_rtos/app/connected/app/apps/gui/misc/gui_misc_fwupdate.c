/**
  * @file src/app/apps/gui/misc/connectedcam/gui_misc_fwupdate.c
  *
  *  Implementation of Firmware Update GUI display flows
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

#include <apps/gui/misc/gui_misc_fwupdate.h>
#include <wchar.h>
#include <apps/gui/resource/gui_settle.h>
/**
 * @brief Show the progress of each partition during updating firmware.
 *
 * @param percentage - The percentage of progress.
 */
static int set_fwupdate_ratio(int percentage)
{
#if 0
    UINT16 str_ratio[5] = {0};

    if (percentage/100) {
        str_ratio[0] = 0x0030+(percentage/100);
    } else {
        str_ratio[0] = 0x0020;    //SPACE
    }
    if (percentage/10) {
        str_ratio[1] = 0x0030+((percentage/10)%10);
    } else {
        str_ratio[1] = 0x0020;    //SPACE
    }
    str_ratio[2] = 0x0030+(percentage%10);
    str_ratio[3] = 0x0025;    //%
    str_ratio[4] = 0x0000;

    AppLibGraph_UpdateStringContext(0, STR_FWUPDATE_RATIO, str_ratio);
    AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_FWUPDATE_RATIO, STR_FWUPDATE_RATIO);
#endif
    return 0;
}

/**
 * @brief Show the rest of partitions during updating firmware.
 *
 * @param present - The present updating partition.
 * @param totalPart - The total partitions.
 */
static int set_fwupdate_stage(int present, int totalPart)
{
    UINT16 str_stage[10] = {'s','t','a','g','e',' ','0','/','6','\0'};

    str_stage[6] = 0x0030+present;
    str_stage[8] = 0x0030+totalPart;

    AppLibGraph_UpdateStringContext(0, STR_FWUPDATE_STAGE, str_stage);
    AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_FWUPDATE_STAGE, STR_FWUPDATE_STAGE);

    return 0;
}

/**
 *  @brief The GUI functions of application
 *
 *  The GUI functions of application
 *
 *  @param [in] guiCmd Command ID
 *  @param [in] param1 First parameter
 *  @param [in] param2 Second parameter
 *
 *  @return >=0 success, <0 failure
 */
int gui_misc_fwupdate_func(UINT32 guiCmd, UINT32 param1, UINT32 param2)
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
    case GUI_WARNING_SHOW:
        AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_WARNING);
        break;
    case GUI_WARNING_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_WARNING);
        break;
    case GUI_WARNING_UPDATE:
        AppLibGraph_UpdateString(GRAPH_CH_DCHAN, GOBJ_WARNING, GuiWarningTable[param1].str);
        break;
    case GUI_FWUPDATE_RATIO_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_FWUPDATE_RATIO);
        break;
    case GUI_FWUPDATE_RATIO_UPDATE:
        set_fwupdate_ratio(param1);
        break;
    case GUI_FWUPDATE_RATIO_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_FWUPDATE_RATIO);
        break;
    case GUI_FWUPDATE_STAGE_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_FWUPDATE_STAGE);
        break;
    case GUI_FWUPDATE_STAGE_UPDATE:
        set_fwupdate_stage(param1, param2);
        break;
    case GUI_FWUPDATE_STAGE_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_FWUPDATE_STAGE);
        break;
    default:
        AmbaPrint("[gui_misc_fwupdate] Undefined GUI command");
        ReturnValue = -1;
        break;
    }

    return ReturnValue;
}
