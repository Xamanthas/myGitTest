/**
  * @file src/app/apps/gui/widget/menu/sportcam/gui_menu_adj.c
  *
  *  Implementation for adjusting Menu GUI flow
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
#include <apps/gui/widget/menu/gui_menu_adjust.h>
#include <apps/gui/resource/gui_settle.h>

static UINT32 gui_icons[2] = {
    GOBJ_AMENU_ICON_LEFT,
    GOBJ_AMENU_ICON_RIGHT
};

#define ADJBAR_HL_TOTAL_W_0    (260)
#define ADJBAR_HL_TOTAL_H_0    (5)
#define ADJBAR_IDX_INIT_X_0    (338)
#define ADJBAR_IDX_INIT_Y_0    (366)

static int set_adjustbar(GUI_ADJ_STATUS_s *status)
{
    int Total = 0, Cur = 0;
    UINT16 gui_abar_hl_w[GUI_LAYOUT_NUM] = {0};
    UINT16 gui_abar_hl_h[GUI_LAYOUT_NUM] = {0};
    UINT16 gui_abar_idx_x[GUI_LAYOUT_NUM] = {0};
    UINT16 gui_abar_idx_y[GUI_LAYOUT_NUM] = {0};

    Total = (status->Max-status->Min)/status->Step;
    Cur = (status->Cur-status->Min)/status->Step;

    gui_abar_hl_w[0] = 0;
    gui_abar_hl_h[0] = 0;
    gui_abar_hl_w[1] = (Cur*ADJBAR_HL_TOTAL_W_0)/Total;
    gui_abar_hl_h[1] = ADJBAR_HL_TOTAL_H_0;
    AppLibGraph_UpdateSize(GRAPH_CH_FCHAN, GOBJ_AMENU_BAR_HL, gui_abar_hl_w[1], gui_abar_hl_h[1], 0);
    gui_abar_idx_x[0] = 0;
    gui_abar_idx_y[0] = 0;
    gui_abar_idx_x[1] = ADJBAR_IDX_INIT_X_0+gui_abar_hl_w[1];
    gui_abar_idx_y[1] = ADJBAR_IDX_INIT_Y_0+50;
    AppLibGraph_UpdatePosition(GRAPH_CH_FCHAN, GOBJ_AMENU_BAR_IDX, gui_abar_idx_x[1], gui_abar_idx_y[1]);

    gui_abar_hl_w[0] = (Cur*ADJBAR_HL_TOTAL_W_0)/Total;
    gui_abar_hl_h[0] = ADJBAR_HL_TOTAL_H_0;
    gui_abar_hl_w[1] = 0;
    gui_abar_hl_h[1] = 0;
    AppLibGraph_UpdateSize(GRAPH_CH_DCHAN, GOBJ_AMENU_BAR_HL, gui_abar_hl_w[0], gui_abar_hl_h[0], 0);
    gui_abar_idx_x[0] = ADJBAR_IDX_INIT_X_0+gui_abar_hl_w[0];
    gui_abar_idx_y[0] = ADJBAR_IDX_INIT_Y_0;
    gui_abar_idx_x[1] = 0;
    gui_abar_idx_y[1] = 0;
    AppLibGraph_UpdatePosition(GRAPH_CH_DCHAN, GOBJ_AMENU_BAR_IDX, gui_abar_idx_x[0], gui_abar_idx_y[0]);

    return 0;
}

int gui_menu_adj_func(UINT32 guiCmd, UINT32 param1, UINT32 param2)
{
    int rval = 0;

    switch (guiCmd) {
    case GUI_AMENU_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_AMENU_BAR_BASE);
        AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_AMENU_TITLE, param1);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_AMENU_TITLE);
        break;
    case GUI_AMENU_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_AMENU_BAR_BASE);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_AMENU_TITLE);
        break;
    case GUI_ICON_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, gui_icons[param1]);
        break;
    case GUI_ICON_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, gui_icons[param1]);
        break;
    case GUI_ICON_UPDATE:
        AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, gui_icons[param1], param2);
        break;
    case GUI_STATUS_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_AMENU_BAR_HL);
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_AMENU_BAR_IDX);
        break;
    case GUI_STATUS_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_AMENU_BAR_HL);
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_AMENU_BAR_IDX);
        break;
    case GUI_STATUS_UPDATE:
        set_adjustbar((GUI_ADJ_STATUS_s *)param1);
        break;
    case GUI_VALUE_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_AMENU_VALUE);
        break;
    case GUI_VALUE_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_AMENU_VALUE);
        break;
    case GUI_VALUE_UPDATE:
        AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_AMENU_VALUE, param1);
        break;
    case GUI_HIDE_ALL:
        AppLibGraph_HideAll(GRAPH_CH_DUAL);
        break;
    case GUI_FLUSH:
        AppLibGraph_Draw(GRAPH_CH_DUAL);
        break;
    default:
        AmbaPrint("[Gui menu adjust] Undefined GUI command");
        rval = -1;
        break;
    }

    return rval;
}
