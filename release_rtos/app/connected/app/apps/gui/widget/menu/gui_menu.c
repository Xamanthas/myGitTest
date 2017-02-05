/**
  * @file src/app/apps/gui/widget/menu/gui_menu.c
  *
  *  Implementation for Menu GUI flow
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

#include <apps/flow/widget/menu/menu.h>
#include <apps/gui/widget/menu/gui_menu.h>
#include <apps/gui/resource/gui_resource.h>
#include <apps/gui/resource/gui_settle.h>

int gui_menu_func(UINT32 gui_cmd, UINT32 param1, UINT32 param2)
{
    int rval = 0;

    switch (gui_cmd) {
    case GUI_MENU_TAB_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_MENU_TAB_1+param1);
        break;
    case GUI_MENU_TAB_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_MENU_TAB_1+param1);
        break;
    case GUI_MENU_TAB_UPDATE_BITMAP:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_MENU_TAB_1+param1, param2);
        break;
    case GUI_MENU_ITEM_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_MENU_RECT_1+param1);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_MENU_STRING_1+param1);
        break;
    case GUI_MENU_ITEM_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_MENU_RECT_1+param1);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_MENU_STRING_1+param1);
        break;
    case GUI_MENU_ITEM_UPDATE_STRING:
        AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_MENU_STRING_1+param1, param2);
        break;
    case GUI_MENU_ITEM_UPDATE_BITMAP:
        //AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, gui_menu_items[param1].gobj_bmp, param2);
        break;
    case GUI_MENU_ITEM_HIGHLIGHT:
        if (param2) {
            AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_MENU_RECT_1+param1, BMP_MENU_STRIPE_HL);
        } else {
            AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, GOBJ_MENU_RECT_1+param1, BMP_MENU_STRIPE);
        }
        break;
    case GUI_MENU_ITEM_LOCK:
        if (param2) {
            AppLibGraph_UpdateColor(GRAPH_CH_DUAL, GOBJ_MENU_STRING_1+param1, COLOR_DARKGRAY, COLOR_TEXT_BORDER );
        } else {
            AppLibGraph_UpdateColor(GRAPH_CH_DUAL, GOBJ_MENU_STRING_1+param1, COLOR_LIGHTGRAY, COLOR_TEXT_BORDER );
        }
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
