/**
  * @file src/app/apps/gui/dialog/gui_dialog.c
  *
  * Implementation for Dialog GUI flow
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
#include <apps/gui/widget/dialog/gui_dialog.h>
#include <apps/gui/resource/gui_settle.h>

typedef struct _GUI_DIALOG_BTN_s_ {
    int Gobj;
} GUI_DIALOG_BTN_s;

static GUI_DIALOG_BTN_s gui_dialog_btns[GUI_DIALOG_BTN_NUM] = {
    { GOBJ_QMENU_CANCEL},
    { GOBJ_QMENU_SET}
};

static int highlight_btn(UINT32 btnId, UINT32 hl)
{
    switch (btnId) {
    case GUI_DIALOG_BTN_SET:
        if (hl) {
            AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, gui_dialog_btns[btnId].Gobj, BMP_ICN_QMENU_YES_HL);
        } else {
            AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, gui_dialog_btns[btnId].Gobj, BMP_ICN_QMENU_YES);
        }
        break;
    case GUI_DIALOG_BTN_CANCEL:
        if (hl) {
            AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, gui_dialog_btns[btnId].Gobj, BMP_ICN_QMENU_NO_HL);
        } else {
            AppLibGraph_UpdateBMP(GRAPH_CH_DUAL, gui_dialog_btns[btnId].Gobj, BMP_ICN_QMENU_NO);
        }
        break;
    default:
        break;
    }

    return 0;
}

int gui_dialog_func(UINT32 guiCmd, UINT32 param1, UINT32 param2)
{
    int rval = 0;

    switch (guiCmd) {
    case GUI_RESET:
        break;
    case GUI_DIALOG_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_QMENU_BG);
        break;
    case GUI_DIALOG_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_QMENU_BG);
        break;
    case GUI_DIALOG_BTN_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, gui_dialog_btns[param1].Gobj);
        break;
    case GUI_DIALOG_BTN_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, gui_dialog_btns[param1].Gobj);
        break;
    case GUI_DIALOG_BTN_HIGHLIGHT:
        highlight_btn(param1, param2);
        break;
    case GUI_DESC_SHOW:
        AppLibGraph_Show(GRAPH_CH_DUAL, GOBJ_QMENU_DESC);
        break;
    case GUI_DESC_HIDE:
        AppLibGraph_Hide(GRAPH_CH_DUAL, GOBJ_QMENU_DESC);
        break;
    case GUI_DESC_UPDATE:
        AppLibGraph_UpdateString(GRAPH_CH_DUAL, GOBJ_QMENU_DESC, param1);
        break;
    case GUI_HIDE_ALL:
        AppLibGraph_HideAll(GRAPH_CH_DUAL);
        break;
    case GUI_FLUSH:
        AppLibGraph_Draw(GRAPH_CH_DUAL);
        break;
    default:
        AmbaPrint("[gui_dialog] Undefined GUI command");
        rval = -1;
        break;
    }

    return rval;
}
