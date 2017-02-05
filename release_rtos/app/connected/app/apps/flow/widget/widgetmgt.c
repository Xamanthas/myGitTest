/**
  * @file src/app/apps/flow/widget/widgetmgt.c
  *
  * Implementation of Widget Management
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

#include <apps/flow/widget/widgetmgt.h>
#include <apps/flow/widget/menu/menu.h>
#include <apps/flow/widget/dialog/dialog.h>
#include <apps/flow/widget/menu/menu_adas_calib.h>

extern WIDGET_ITEM_s* AppMenuQuick_GetWidget(void);
extern WIDGET_ITEM_s* AppMenuAdj_GetWidget(void);
extern WIDGET_ITEM_s* AppMenuTime_GetWidget(void);

/*************************************************************************
 * Widget Definitions
 ************************************************************************/
typedef struct _WIDGET_s_ {
    UINT32 Flags;
    UINT32 Cur;
    WIDGET_ITEM_s *Menu;
    WIDGET_ITEM_s *MenuQuick;
    WIDGET_ITEM_s *MenuAdj;
    WIDGET_ITEM_s *MenuTime;
    WIDGET_ITEM_s *Dialog;
    WIDGET_ITEM_s *MenuAdasCalib;
} WIDGET_s;

static WIDGET_s widget;

/*************************************************************************
 * Widget APIs
 ************************************************************************/
int AppWidget_Init(void)
{
    memset(&widget, 0, sizeof(WIDGET_s));
    widget.Flags = WIDGET_NONE;
    widget.Menu = (WIDGET_ITEM_s *)AppMenu_GetWidget();
    widget.MenuQuick = (WIDGET_ITEM_s *)AppMenuQuick_GetWidget();
    widget.MenuAdj = (WIDGET_ITEM_s *)AppMenuAdj_GetWidget();
    widget.MenuTime = (WIDGET_ITEM_s *)AppMenuTime_GetWidget();
    widget.Dialog = (WIDGET_ITEM_s *)AppDialog_GetWidget();
    widget.MenuAdasCalib = (WIDGET_ITEM_s *)AppAdasCalib_GetWidget();

    return 0;
}

int AppWidget_GetCur(void)
{
    return widget.Cur;
}

int AppWidget_On(UINT32 widgetId, UINT32 param)
{
    int rval = 0;

    if (widget.Cur) {
        switch (widget.Cur) {
        case WIDGET_MENU:
            widget.Menu->off(0);
            widget.Cur = WIDGET_NONE;
            AmbaPrint("Menu is hidden for widget %d", widgetId);
            break;
        case WIDGET_MENU_QUICK:
            widget.MenuQuick->off(0);
            widget.Cur = WIDGET_NONE;
            AmbaPrint("Quick Menu is hidden for widget %d", widgetId);
            break;
        case WIDGET_MENU_ADJ:
            widget.MenuAdj->off(0);
            widget.Cur = WIDGET_NONE;
            AmbaPrint("Adjusting Menu is hidden for widget %d", widgetId);
            break;
        case WIDGET_MENU_TIME:
            widget.MenuTime->off(0);
            widget.Cur = WIDGET_NONE;
            AmbaPrint("Time Menu is hidden for widget %d", widgetId);
            break;
        case WIDGET_DIALOG:
            AmbaPrint("Dialog should be decided!!!");
            rval = -1;
            break;
        case WIDGET_MENU_ADAS_CALIB:
            widget.MenuAdasCalib->off(0);
            widget.Cur = WIDGET_NONE;
            AmbaPrint("ADAS calibration Menu is hidden for widget %d", widgetId);
            break;
        default:
            AmbaPrint("[App - Wiget] Unknown current widget!!!");
            rval = -1;
            break;
        }
    }

    if (rval < 0) {
        return rval;
    }

    switch (widgetId) {
    case WIDGET_MENU:
        APP_ADDFLAGS(widget.Flags, WIDGET_MENU);
        widget.Menu->on(0);
        widget.Cur = widgetId;
        break;
    case WIDGET_MENU_QUICK:
        APP_ADDFLAGS(widget.Flags, WIDGET_MENU_QUICK);
        widget.MenuQuick->on(0);
        widget.Cur = widgetId;
        break;
    case WIDGET_MENU_ADJ:
        APP_ADDFLAGS(widget.Flags, WIDGET_MENU_ADJ);
        widget.MenuAdj->on(0);
        widget.Cur = widgetId;
        break;
    case WIDGET_MENU_TIME:
        APP_ADDFLAGS(widget.Flags, WIDGET_MENU_TIME);
        widget.MenuTime->on(0);
        widget.Cur = widgetId;
        break;
    case WIDGET_DIALOG:
        APP_ADDFLAGS(widget.Flags, WIDGET_DIALOG);
        widget.Dialog->on(0);
        widget.Cur = widgetId;
        break;
    case WIDGET_MENU_ADAS_CALIB:
        APP_ADDFLAGS(widget.Flags, WIDGET_MENU_ADAS_CALIB);
        widget.MenuAdasCalib->on(0);
        widget.Cur = widgetId;
        break;
    default:
        AmbaPrint("[App - Wiget] Unknown widget!!!");
        rval = -1;
        break;
    }

    return rval;
}

int AppWidget_Off(UINT32 widgetId, UINT32 param)
{
    int rval = 0;

    if (widgetId == WIDGET_ALL) {
        if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU)) {
            APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU);
            if (widget.Cur == WIDGET_MENU) {
                widget.Menu->off(0);
                widget.Cur = WIDGET_NONE;
            }
        }
        if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU_QUICK)) {
            APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU_QUICK);
            if (widget.Cur == WIDGET_MENU_QUICK) {
                widget.MenuQuick->off(0);
                widget.Cur = WIDGET_NONE;
            }
        }
        if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU_ADJ)) {
            APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU_ADJ);
            if (widget.Cur == WIDGET_MENU_ADJ) {
                widget.MenuAdj->off(0);
                widget.Cur = WIDGET_NONE;
            }
        }
        if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU_TIME)) {
            APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU_TIME);
            if (widget.Cur == WIDGET_MENU_ADJ) {
                widget.MenuTime->off(0);
                widget.Cur = WIDGET_NONE;
            }
        }
        if (APP_CHECKFLAGS(widget.Flags, WIDGET_DIALOG)) {
            APP_REMOVEFLAGS(widget.Flags, WIDGET_DIALOG);
            if (widget.Cur == WIDGET_DIALOG) {
                widget.Dialog->OnMessage(DIALOG_SEL_NO, 0, 0);
                widget.Dialog->off(0);
                widget.Cur = WIDGET_NONE;
            }
        }
        if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU_ADAS_CALIB)) {
            APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU_ADAS_CALIB);
            if (widget.Cur == WIDGET_MENU_ADAS_CALIB) {
                widget.MenuAdasCalib->off(0);
                widget.Cur = WIDGET_NONE;
            }
        }
        if (!APP_CHECKFLAGS(param, WIDGET_HIDE_SILENT)) {
            AppLibComSvcHcmgr_SendMsg(AMSG_STATE_WIDGET_CLOSED, 0, 0);
        }
    } else {
        if (widgetId == widget.Cur) {
            switch (widgetId) {
            case WIDGET_MENU:
                APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU);
                widget.Menu->off(0);
                widget.Cur = WIDGET_NONE;
                break;
            case WIDGET_MENU_QUICK:
                APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU_QUICK);
                widget.MenuQuick->off(0);
                widget.Cur = WIDGET_NONE;
                break;
            case WIDGET_MENU_ADJ:
                APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU_ADJ);
                widget.MenuAdj->off(0);
                widget.Cur = WIDGET_NONE;
                break;
            case WIDGET_MENU_TIME:
                APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU_TIME);
                widget.MenuTime->off(0);
                widget.Cur = WIDGET_NONE;
                break;
            case WIDGET_DIALOG:
                APP_REMOVEFLAGS(widget.Flags, WIDGET_DIALOG);
                widget.Dialog->off(0);
                widget.Cur = WIDGET_NONE;
                break;
            case WIDGET_MENU_ADAS_CALIB:
                APP_REMOVEFLAGS(widget.Flags, WIDGET_MENU_ADAS_CALIB);
                widget.MenuAdasCalib->off(0);
                widget.Cur = WIDGET_NONE;
                break;
            default:
                AmbaPrint("[App - Wiget] Unknown widget!!!");
                rval = -1;
                break;
            }
            if (widget.Flags) {
                AmbaPrint("Some other widgets are still ON");
                if (APP_CHECKFLAGS(widget.Flags, WIDGET_DIALOG)) {
                    AppWidget_On(WIDGET_DIALOG, 0);
                } else if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU_TIME)) {
                    AppWidget_On(WIDGET_MENU_TIME, 0);
                } else if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU_ADJ)) {
                    AppWidget_On(WIDGET_MENU_ADJ, 0);
                } else if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU_QUICK)) {
                    AppWidget_On(WIDGET_MENU_QUICK, 0);
                } else if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU_ADAS_CALIB)) {
                    AppWidget_On(WIDGET_MENU_ADAS_CALIB, 0);
                } else if (APP_CHECKFLAGS(widget.Flags, WIDGET_MENU)) {
                    AppWidget_On(WIDGET_MENU, 0);
                }
            } else if (!APP_CHECKFLAGS(param, WIDGET_HIDE_SILENT)) {
                AppLibComSvcHcmgr_SendMsg(AMSG_STATE_WIDGET_CLOSED, 0, 0);
            }
        } else {
            AmbaPrint("[App - Wiget] Should not close non-current widget");
        }
    }

    return rval;
}

int AppWidget_OnMessage(UINT32 msg, UINT32 param1, UINT32 param2)
{
    int rval = WIDGET_PASSED_MSG;

    if (widget.Cur) {
    switch (widget.Cur) {
    case WIDGET_MENU:
        rval = widget.Menu->OnMessage(msg, param1, param2);
        break;
    case WIDGET_MENU_QUICK:
        rval = widget.MenuQuick->OnMessage(msg, param1, param2);
        break;
    case WIDGET_MENU_ADJ:
        rval = widget.MenuAdj->OnMessage(msg, param1, param2);
        break;
    case WIDGET_MENU_TIME:
        rval = widget.MenuTime->OnMessage(msg, param1, param2);
        break;
    case WIDGET_DIALOG:
        rval = widget.Dialog->OnMessage(msg, param1, param2);
        break;
    case WIDGET_MENU_ADAS_CALIB:
        rval = widget.MenuAdasCalib->OnMessage(msg, param1, param2);
        break;
    default:
        AmbaPrint("[App - Wiget] Unknown current widget");
        rval = -1;
        break;
    }
#if 0
    if (rval == 0) {
        AmbaPrint("[App - Widget] message was handled by widget");
    } else if (rval > 0) {
        AmbaPrint("[App - Widget] message wasn't handled by widget");
    } else if (rval < 0) {
        AmbaPrint("[App - Widget] message handling error");
    }
#endif
    }

    return rval;
}

UINT32 AppWidget_GetFlags(void)
{
    return widget.Flags;
}
