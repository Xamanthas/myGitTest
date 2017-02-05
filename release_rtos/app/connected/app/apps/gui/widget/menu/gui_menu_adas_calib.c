/**
  * @file src/app/apps/gui/widget/menu/connectedcam/gui_menu_time.c
  *
  *  Implementation for Time Menu GUI flow
  *
  * History:
  *    2013/11/22 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include <apps/apps.h>
#include <system/app_util.h>
#include <apps/gui/widget/menu/gui_menu_adas_calib.h>
#include <apps/gui/resource/gui_settle.h>

int gui_menu_adas_calib_func(UINT32 gui_cmd, UINT32 param1, UINT32 param2)
{
    int rval = 0;

    switch (gui_cmd) {
    case GUI_HIDE_ADAS_CALIB_SKY:
        AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_ADAS_CALIB_SKY);
        break;
    case GUI_HIDE_ADAS_CALIB_HOOD:
        AppLibGraph_Hide(GRAPH_CH_DCHAN, GOBJ_ADAS_CALIB_HOOD);
        break;
    case GUI_SHOW_ADAS_CALIB_SKY:
        AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_ADAS_CALIB_SKY);
        break;
    case GUI_SHOW_ADAS_CALIB_HOOD:
        AppLibGraph_Show(GRAPH_CH_DCHAN, GOBJ_ADAS_CALIB_HOOD);
        break;
    case GUI_UPDATE_ADAS_CALIB_SKY:
        AppLibGraph_UpdatePosition(GRAPH_CH_DCHAN, GOBJ_ADAS_CALIB_SKY, param1, param2);
        break;
    case GUI_UPDATE_ADAS_CALIB_HOOD:
        AppLibGraph_UpdatePosition(GRAPH_CH_DCHAN, GOBJ_ADAS_CALIB_HOOD, param1, param2);
        break;
    case GUI_HIDE_ALL:
        AppLibGraph_HideAll(GRAPH_CH_DUAL);
        break;
    case GUI_FLUSH:
        AppLibGraph_Draw(GRAPH_CH_DUAL);
        break;
    default:
        AmbaPrint("Undefined GUI command");
        rval = -1;
        break;
    }

    return rval;
}
