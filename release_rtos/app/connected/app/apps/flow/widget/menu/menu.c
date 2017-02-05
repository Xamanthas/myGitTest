/**
  * @file src/app/apps/flow/widget/menu/menu.c
  *
  *  Implementation of Main Menu
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
#include <apps/flow/widget/menu/menu.h>
#include <apps/gui/widget/menu/gui_menu.h>

/*************************************************************************
 * Menu definitions
 ************************************************************************/
extern MENU_TAB_CTRL_s menu_video_ctrl;
extern MENU_TAB_CTRL_s menu_photo_ctrl;
extern MENU_TAB_CTRL_s menu_pback_ctrl;
extern MENU_TAB_CTRL_s menu_image_ctrl;
#if defined(ENABLE_SOUND_RECORDER_PIPE)
extern MENU_TAB_CTRL_s menu_sound_ctrl;
#endif
#if defined(ENABLE_MW_PICTBRIDGE)
extern MENU_TAB_CTRL_s menu_pictb_setup_ctrl;
extern MENU_TAB_CTRL_s menu_pictb_print_ctrl;
#endif
extern MENU_TAB_CTRL_s menu_setup_ctrl;



static MENU_TAB_CTRL_s *menu_tab_ctrls[MENU_TAB_NUM] = {
    &menu_setup_ctrl,    // MENU_SETUP
    &menu_video_ctrl,    // MENU_VIDEO
    &menu_photo_ctrl,    // MENU_PHOTO
    &menu_pback_ctrl,    // MENU_PBACK
//    &menu_image_ctrl    // MENU_IMAGE
};


/** Menu funcions */
typedef enum _MENU_FUNC_ID_e_ {
    MENU_CAL_TAB_DISP_PARAM = 0,
    MENU_CAL_ITEM_DISP_PARAM,
    MENU_SWITCH_TO_TAB,
    MENU_SWITCH_TO_ITEM,
    MENU_CUR_ITEM_SET
} MENU_FUNC_ID_e;

static int menu_cal_tab_disp_param(void);
#define MENU_TAB_PAGE_CHANGED    (0x01)
static int menu_cal_item_disp_param(void);
#define MENU_ITEM_PAGE_CHANGED    (0x01)
static int menu_switch_to_tab(int tabIdx);
static int menu_switch_to_item(int itemIdx);

static int menu_func(UINT32 funcId, UINT32 param1, UINT32 param2);

/** Menu operations */
static int menu_button_up(void);
static int menu_button_down(void);
static int menu_button_left(void);
static int menu_button_right(void);
static int menu_button_set(void);
static int menu_button_menu(void);

typedef struct _MENU_OP_s_ {
    int (*ButtonUp)(void);
    int (*ButtonDown)(void);
    int (*ButtonLeft)(void);
    int (*ButtonRight)(void);
    int (*ButtonSet)(void);
    int (*ButtonMenu)(void);
} MENU_OP_s;

static MENU_OP_s menu_op = {
    menu_button_up,
    menu_button_down,
    menu_button_left,
    menu_button_right,
    menu_button_set,
    menu_button_menu
};

/** Menu status */
typedef struct _MENU_s_ {
    int TabNum;
    int TabCur;
    int DispTabStart;
    int DispTabNum;
    int DispTabCur;
    int DispItemStart;
    int DispItemNum;
    int DispItemCur;
    MENU_TAB_s *tabs[MENU_TAB_NUM];
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
    int (*Gui)(UINT32 guiCmd, UINT32 param1, UINT32 param2);
    MENU_OP_s *Op;
} MENU_s;

static MENU_s menu;

static int cur_menu_tab_id = -1;

/** Menu interface */
static int app_menu_on(UINT32 param);
static int app_menu_off(UINT32 param);
static int app_menu_on_message(UINT32 msg, UINT32 param1, UINT32 param2);

WIDGET_ITEM_s widget_menu = {
    app_menu_on,
    app_menu_off,
    app_menu_on_message
};

/*************************************************************************
 * Menu APIs
 ************************************************************************/
/** Menu functions */
static int menu_cal_tab_disp_param(void)
{
    int ReturnValue = 0;
    int disp_tab_start_old = 0, page = 0, num = 0;

    disp_tab_start_old = menu.DispTabStart;
    page = menu.TabCur / GUI_MENU_TAB_NUM;
    menu.DispTabStart = GUI_MENU_TAB_NUM * page;
    num = menu.TabNum - menu.DispTabStart;
    menu.DispTabNum = (num > GUI_MENU_TAB_NUM) ? GUI_MENU_TAB_NUM : num;
    menu.DispTabCur = menu.TabCur - menu.DispTabStart;

    if (disp_tab_start_old != menu.DispTabStart) {
        ReturnValue = MENU_TAB_PAGE_CHANGED;
    }
    return ReturnValue;
}

static int menu_cal_item_disp_param(void)
{
    int ReturnValue = 0;
    int DispItemStartOld = 0, page = 0, num = 0;
    MENU_TAB_s *CurTab = menu.tabs[menu.TabCur];

    DispItemStartOld = menu.DispItemStart;
    page = CurTab->ItemCur / GUI_MENU_ITEM_NUM;
    menu.DispItemStart = GUI_MENU_ITEM_NUM * page;
    num = CurTab->ItemNum - menu.DispItemStart;
    menu.DispItemNum = (num > GUI_MENU_ITEM_NUM) ? GUI_MENU_ITEM_NUM : num;
    menu.DispItemCur = CurTab->ItemCur - menu.DispItemStart;

    if (DispItemStartOld != menu.DispItemStart) {
        ReturnValue = MENU_ITEM_PAGE_CHANGED;
    }
    return ReturnValue;
}

static int menu_switch_to_tab(int tabIdx)
{
    int ReturnValue = 0;
    int i = 0;
    MENU_TAB_s *CurTab;

    /** Stop current tab */
    CurTab = menu.tabs[menu.TabCur];
    CurTab->Stop();

    /** Change current GUI */
    menu.Gui(GUI_MENU_TAB_UPDATE_BITMAP, menu.DispTabCur, CurTab->Bmp);
    menu.Gui(GUI_MENU_ITEM_HIGHLIGHT, menu.DispItemCur, 0);

    /** Current tab cursor change */
    menu.TabCur = tabIdx;

    /** Initialize new current tab */
    CurTab = menu.tabs[menu.TabCur];
    cur_menu_tab_id = CurTab->Id;
    CurTab->Init();
    /** Initialize Items under new current tab */
    for (i=0; i<CurTab->ItemNum; i++) {
        CurTab->Items[i]->Init();
    }

    /** Start new current tab */
    CurTab->Start();

    /** Get new menu tab display parameters */
    ReturnValue = menu.Func(MENU_CAL_TAB_DISP_PARAM, 0, 0);

    /** If menu tab content changed, refresh all tab GUI */
    if (ReturnValue == MENU_TAB_PAGE_CHANGED) {
        for (i=0; i<GUI_MENU_TAB_NUM; i++) {
            if (i < menu.DispTabNum) {
                menu.Gui(GUI_MENU_TAB_UPDATE_BITMAP, i, menu.tabs[menu.DispTabStart+i]->Bmp);
                menu.Gui(GUI_MENU_TAB_SHOW, i, 0);
            } else {
                menu.Gui(GUI_MENU_TAB_HIDE, i, 0);
            }
        }
    }

    /** Highlight current tab */
    menu.Gui(GUI_MENU_TAB_UPDATE_BITMAP, menu.DispTabCur, CurTab->BmpHl);

    /** Get new menu item display parameters */
    ReturnValue = menu.Func(MENU_CAL_ITEM_DISP_PARAM, 0, 0);

    /** Since tab page is changed, item content should be loaded */
    for (i=0; i<GUI_MENU_ITEM_NUM; i++) {
        if (i < menu.DispItemNum) {
            menu.Gui(GUI_MENU_ITEM_UPDATE_STRING, i, CurTab->Items[menu.DispItemStart+i]->GetTabStr());
            if (APP_CHECKFLAGS(CurTab->Flags, MENU_TAB_FLAGS_LOCKED) ||
                APP_CHECKFLAGS(CurTab->Items[menu.DispItemStart+i]->Flags, MENU_ITEM_FLAGS_LOCKED)) {
                menu.Gui(GUI_MENU_ITEM_LOCK, i, 1);
            } else {
                menu.Gui(GUI_MENU_ITEM_LOCK, i, 0);
            }
            menu.Gui(GUI_MENU_ITEM_SHOW, i, 0);
        } else {
            menu.Gui(GUI_MENU_ITEM_HIDE, i, 0);
        }
    }

    /** Highlight current item */
    menu.Gui(GUI_MENU_ITEM_HIGHLIGHT, menu.DispItemCur, 1);
    /** Flush GUI */
    menu.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int menu_switch_to_item(int itemIdx)
{
    int ReturnValue = 0;
    int i = 0;
    MENU_TAB_s *CurTab = menu.tabs[menu.TabCur];

    /** Unhighlight current item */
    menu.Gui(GUI_MENU_ITEM_HIGHLIGHT, menu.DispItemCur, 0);

    /** Change item cursor */
    CurTab->ItemCur = itemIdx;

    /** Get new menu item display parameters */
    ReturnValue = menu.Func(MENU_CAL_ITEM_DISP_PARAM, 0, 0);

    /** If menu item content changed, refresh all item GUI */
    if (ReturnValue == MENU_ITEM_PAGE_CHANGED) {
        for (i=0; i< GUI_MENU_ITEM_NUM; i++) {
            if (i < menu.DispItemNum) {
                menu.Gui(GUI_MENU_ITEM_UPDATE_STRING, i, CurTab->Items[menu.DispItemStart+i]->GetTabStr());
                if (APP_CHECKFLAGS(CurTab->Flags, MENU_TAB_FLAGS_LOCKED) ||
                    APP_CHECKFLAGS(CurTab->Items[menu.DispItemStart+i]->Flags, MENU_ITEM_FLAGS_LOCKED)) {
                    menu.Gui(GUI_MENU_ITEM_LOCK, i, 1);
                } else {
                    menu.Gui(GUI_MENU_ITEM_LOCK, i, 0);
                }
                menu.Gui(GUI_MENU_ITEM_SHOW, i, 0);
            } else {
                menu.Gui(GUI_MENU_ITEM_HIDE, i, 0);
            }
        }
    }

    /** Highlight current item */
    menu.Gui(GUI_MENU_ITEM_HIGHLIGHT, menu.DispItemCur, 1);
    /** Flush GUI */
    menu.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int menu_func(UINT32 funcId, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;
    MENU_TAB_s *CurTab;

    switch (funcId) {
    case MENU_CAL_TAB_DISP_PARAM:
        ReturnValue = menu_cal_tab_disp_param();
        break;
    case MENU_CAL_ITEM_DISP_PARAM:
        ReturnValue = menu_cal_item_disp_param();
        break;
    case MENU_SWITCH_TO_TAB:
        ReturnValue = menu_switch_to_tab(param1);
        break;
    case MENU_SWITCH_TO_ITEM:
        ReturnValue = menu_switch_to_item(param1);
        break;
    case MENU_CUR_ITEM_SET:
        CurTab = menu.tabs[menu.TabCur];
        if (APP_CHECKFLAGS(CurTab->Flags, MENU_TAB_FLAGS_LOCKED) ||
            APP_CHECKFLAGS(CurTab->Items[CurTab->ItemCur]->Flags, MENU_ITEM_FLAGS_LOCKED)) {
            AmbaPrint("[app_menu] Tab or item is locked");
            ReturnValue = -1;
        } else {
            ReturnValue = CurTab->Items[CurTab->ItemCur]->Set();
        }
        break;
    default:
        AmbaPrint("[app_menu] The function is undefined");
        break;
    }

    return ReturnValue;
}

static int menu_button_up(void)
{
    int ItemTarget = 0;
    MENU_TAB_s *CurTab = menu.tabs[menu.TabCur];

    if (CurTab->ItemNum > 1) {
        ItemTarget = CurTab->ItemCur-1;
        if (ItemTarget < 0) {
            ItemTarget = CurTab->ItemNum-1;
        }
        menu.Func(MENU_SWITCH_TO_ITEM, ItemTarget, 0);
    }

    return 0;
}

static int menu_button_down(void)
{
    int ItemTarget = 0;
    MENU_TAB_s *CurTab = menu.tabs[menu.TabCur];

    if (CurTab->ItemNum > 1) {
        ItemTarget = CurTab->ItemCur+1;
        if (ItemTarget == CurTab->ItemNum) {
            ItemTarget = 0;
        }
        menu.Func(MENU_SWITCH_TO_ITEM, ItemTarget, 0);
    }

    return 0;
}

static int menu_button_left(void)
{
    int TabTarget = 0;

    if (menu.TabNum > 1) {
        TabTarget = menu.TabCur+1;
        if (TabTarget == menu.TabNum) {
            TabTarget = 0;
        }
        menu.Func(MENU_SWITCH_TO_TAB, TabTarget, 0);
    }

    return 0;
}

static int menu_button_right(void)
{
    int TabTarget = 0;

    if (menu.TabNum > 1) {
        TabTarget = menu.TabCur-1;
        if (TabTarget < 0) {
            TabTarget = menu.TabNum-1;
        }
        menu.Func(MENU_SWITCH_TO_TAB, TabTarget, 0);
    }

    return 0;
}

static int menu_button_set(void)
{
    //app_beep_play_beep(BEEP_OPERATION, 0);
    return menu.Func(MENU_CUR_ITEM_SET, 0, 0);
}

static int menu_button_menu(void)
{
    return AppWidget_Off(WIDGET_MENU, 0);
}

static int menu_init = 0;

static int app_menu_on(UINT32 param)
{
    int ReturnValue = 0;
    int i = 0;
    MENU_TAB_s *CurTab;

    //AmbaPrint("[Menu] <app_menu_show> Start");

    menu.Func = menu_func;
    menu.Gui = gui_menu_func;
    menu.Op = &menu_op;

    menu_init = 1;

    if ((cur_menu_tab_id < 0) ||
        (cur_menu_tab_id != menu.tabs[menu.TabCur]->Id)) {
        menu.TabCur = menu.TabNum-1;
        cur_menu_tab_id = menu.tabs[menu.TabCur]->Id;
    }

    CurTab = menu.tabs[menu.TabCur];

    /** Initialize current tab */
    CurTab->Init();

    /** Initialize Items under current tab */
    for (i=0; i<CurTab->ItemNum; i++) {
        CurTab->Items[i]->Init();
    }
    /** Start current tab */
    CurTab->Start();

    /** Get menu tab display parameters */
    menu.Func(MENU_CAL_TAB_DISP_PARAM, 0, 0);

    /** refresh all tab GUI */
    for (i=0; i<GUI_MENU_TAB_NUM; i++) {
        if (i < menu.DispTabNum) {
            menu.Gui(GUI_MENU_TAB_UPDATE_BITMAP, i, menu.tabs[menu.DispTabStart+i]->Bmp);
            menu.Gui(GUI_MENU_TAB_SHOW, i, 0);
        } else {
            menu.Gui(GUI_MENU_TAB_HIDE, i, 0);
        }
    }
    /** Highlight current tab */
    menu.Gui(GUI_MENU_TAB_UPDATE_BITMAP, menu.DispTabCur, CurTab->BmpHl);
    /** Get menu item display parameters */
    menu.Func(MENU_CAL_ITEM_DISP_PARAM, 0, 0);
    /** Item content should be loaded */
    for (i=0; i<GUI_MENU_ITEM_NUM; i++) {
        if (i < menu.DispItemNum) {
            menu.Gui(GUI_MENU_ITEM_UPDATE_STRING, i, CurTab->Items[menu.DispItemStart+i]->GetTabStr());
            if (APP_CHECKFLAGS(CurTab->Flags, MENU_TAB_FLAGS_LOCKED) ||
                APP_CHECKFLAGS(CurTab->Items[menu.DispItemStart+i]->Flags, MENU_ITEM_FLAGS_LOCKED)) {
                menu.Gui(GUI_MENU_ITEM_LOCK, i, 1);
            } else {
                menu.Gui(GUI_MENU_ITEM_LOCK, i, 0);
            }
            menu.Gui(GUI_MENU_ITEM_SHOW, i, 0);
        } else {
            menu.Gui(GUI_MENU_ITEM_HIDE, i, 0);
        }
    }
    /** Highlight current item */
    menu.Gui(GUI_MENU_ITEM_HIGHLIGHT, menu.DispItemCur, 1);
    /** Flush GUI */
    menu.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

static int app_menu_off(UINT32 param)
    {
    int i = 0;

    menu.tabs[menu.TabCur]->Stop();

    menu.Gui(GUI_MENU_ITEM_HIGHLIGHT, menu.DispItemCur, 0);
    for (i=0; i<GUI_MENU_TAB_NUM; i++) {
        menu.Gui(GUI_MENU_TAB_HIDE, i, 0);
    }
    for (i=0; i<GUI_MENU_ITEM_NUM; i++) {
        menu.Gui(GUI_MENU_ITEM_HIDE, i, 0);
    }
    menu.Gui(GUI_FLUSH, 0, 0);

    return 0;
}

static int app_menu_on_message(UINT32 msg, UINT32 param1, UINT32 param2)
{
    int ReturnValue = WIDGET_PASSED_MSG;

    switch (msg) {
    case HMSG_USER_UP_BUTTON:
    case HMSG_USER_IR_UP_BUTTON:
        ReturnValue = menu.Op->ButtonUp();
        break;
    case HMSG_USER_DOWN_BUTTON:
    case HMSG_USER_IR_DOWN_BUTTON:
        ReturnValue = menu.Op->ButtonDown();
        break;
    case HMSG_USER_LEFT_BUTTON:
    case HMSG_USER_IR_LEFT_BUTTON:
        ReturnValue = menu.Op->ButtonLeft();
        break;
    case HMSG_USER_RIGHT_BUTTON:
    case HMSG_USER_IR_RIGHT_BUTTON:
        ReturnValue = menu.Op->ButtonRight();
        break;
    case HMSG_USER_SET_BUTTON:
    case HMSG_USER_IR_SET_BUTTON:
        ReturnValue = menu.Op->ButtonSet();
        break;
    case HMSG_USER_MENU_BUTTON:
    case HMSG_USER_IR_MENU_BUTTON:
        ReturnValue = menu.Op->ButtonMenu();
        break;
    case AMSG_CMD_UPDATE_TIME_DISPLAY:
        {
            int i = 0;
            MENU_TAB_s *CurTab = menu.tabs[menu.TabCur];
            /** Reload items */
            for (i=0; i<menu.DispItemNum; i++) {
                menu.Gui(GUI_MENU_ITEM_UPDATE_STRING, i, CurTab->Items[menu.DispItemStart+i]->GetTabStr());
            }
            menu.Gui(GUI_FLUSH, 0, 0);
        }
        break;
    default:
        ReturnValue = WIDGET_PASSED_MSG;
        break;
    }

    return ReturnValue;
}

/*************************************************************************
 * Menu APIs for widget management
 ************************************************************************/
WIDGET_ITEM_s* AppMenu_GetWidget(void)
{
    return &widget_menu;
}

/*************************************************************************
 * Public Menu Widget APIs
 ************************************************************************/
int AppMenu_Reset(void)
{
    memset(&menu, 0, sizeof(MENU_s));
    menu_init = 0;
    return 0;
}

int AppMenu_RegisterTab(UINT32 tabId)
{
    if (menu.TabNum < MENU_TAB_NUM) {
        menu.tabs[menu.TabNum] = menu_tab_ctrls[tabId]->GetTab();
        if (cur_menu_tab_id == menu.tabs[menu.TabNum]->Id) {
            menu.TabCur = menu.TabNum;
        }
        menu.TabNum++;
    } else {
        return -1;
    }
    return 0;
}

int AppMenu_ReflushItem(void)
{
    int ReturnValue = 0;
    int i = 0;
    MENU_TAB_s *CurTab;

    if (menu_init == 0) {
        return -1;
    }

    CurTab = menu.tabs[menu.TabCur];
    for (i=0; i<GUI_MENU_ITEM_NUM; i++) {
        if (i < menu.DispItemNum) {
            menu.Gui(GUI_MENU_ITEM_UPDATE_STRING, i, CurTab->Items[menu.DispItemStart+i]->GetTabStr());
            if (APP_CHECKFLAGS(CurTab->Flags, MENU_TAB_FLAGS_LOCKED) ||
                APP_CHECKFLAGS(CurTab->Items[menu.DispItemStart+i]->Flags, MENU_ITEM_FLAGS_LOCKED)) {
                menu.Gui(GUI_MENU_ITEM_LOCK, i, 1);
            } else {
                menu.Gui(GUI_MENU_ITEM_LOCK, i, 0);
            }
            menu.Gui(GUI_MENU_ITEM_SHOW, i, 0);
        } else {
            menu.Gui(GUI_MENU_ITEM_HIDE, i, 0);
        }
    }
    /** Highlight current item */
    menu.Gui(GUI_MENU_ITEM_HIGHLIGHT, menu.DispItemCur, 1);
    /** Flush GUI */
    menu.Gui(GUI_FLUSH, 0, 0);

    return ReturnValue;
}

MENU_TAB_s* AppMenu_GetTab(UINT32 tabId)
{
    return menu_tab_ctrls[tabId]->GetTab();
}

MENU_ITEM_s* AppMenu_GetItem(UINT32 tabId, UINT32 itemId)
{
    return menu_tab_ctrls[tabId]->GetItem(itemId);
}

MENU_SEL_s* AppMenu_GetSel(UINT32 tabId, UINT32 itemId, UINT32 selId)
{
    return menu_tab_ctrls[tabId]->GetSel(itemId, selId);
}

int AppMenu_SetSelTable(UINT32 tabId, UINT32 itemId, MENU_SEL_s *selTbl)
{
    menu_tab_ctrls[tabId]->SetSelTable(itemId, selTbl);
    return 0;
}

int AppMenu_LockTab(UINT32 tabId)
{
    menu_tab_ctrls[tabId]->LockTab();
    return 0;
}

int AppMenu_UnlockTab(UINT32 tabId)
{
    menu_tab_ctrls[tabId]->UnlockTab();
    return 0;
}

int AppMenu_EnableItem(UINT32 tabId, UINT32 itemId)
{
    menu_tab_ctrls[tabId]->EnableItem(itemId);
    return 0;
}

int AppMenu_DisableItem(UINT32 tabId, UINT32 itemId)
{
    menu_tab_ctrls[tabId]->DisableItem(itemId);
    return 0;
}

int AppMenu_LockItem(UINT32 tabId, UINT32 itemId)
{
    menu_tab_ctrls[tabId]->LockItem(itemId);
    return 0;
}

int AppMenu_UnlockItem(UINT32 tabId, UINT32 itemId)
{
    menu_tab_ctrls[tabId]->UnlockItem(itemId);
    return 0;
}

int AppMenu_EnableSel(UINT32 tabId, UINT32 itemId, UINT32 selId)
{
    menu_tab_ctrls[tabId]->EnableSel(itemId, selId);
    return 0;
}

int AppMenu_DisableSel(UINT32 tabId, UINT32 itemId, UINT32 selId)
{
    menu_tab_ctrls[tabId]->DisableSel(itemId, selId);
    return 0;
}

int AppMenu_LockSel(UINT32 tabId, UINT32 itemId, UINT32 selId)
{
    menu_tab_ctrls[tabId]->LockSel(itemId, selId);
    return 0;
}

int AppMenu_UnlockSel(UINT32 tabId, UINT32 itemId, UINT32 selId)
{
    menu_tab_ctrls[tabId]->UnlockSel(itemId, selId);
    return 0;
}

