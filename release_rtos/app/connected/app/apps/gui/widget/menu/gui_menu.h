/**
  * @file src/app/apps/gui/widget/menu/gui_menu.h
  *
  *  Header for Menu GUI flow
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

#ifndef APP_GUI_WIDGET_MENU_H_
#define APP_GUI_WIDGET_MENU_H_

__BEGIN_C_PROTO__

/*************************************************************************
 * Menu GUI definitions
 ************************************************************************/
typedef enum _GUI_MENU_TAB_ID_e_ {
    GUI_MENU_TAB_1 = 0,
    GUI_MENU_TAB_2,
    GUI_MENU_TAB_3,
    GUI_MENU_TAB_4,
    GUI_MENU_TAB_NUM
} GUI_MENU_TAB_ID_e;

typedef enum _GUI_MENU_ITEM_ID_e_ {
    GUI_MENU_ITEM_1 = 0,
    GUI_MENU_ITEM_2,
    GUI_MENU_ITEM_3,
    GUI_MENU_ITEM_4,
    GUI_MENU_ITEM_NUM
} GUI_MENU_ITEM_ID_e;

typedef enum _GUI_MENU_CMD_ID_e_ {
    GUI_FLUSH = 0,
    GUI_HIDE_ALL,
    GUI_MENU_TAB_SHOW,
    GUI_MENU_TAB_HIDE,
    GUI_MENU_TAB_UPDATE_BITMAP,
    GUI_MENU_ITEM_SHOW,
    GUI_MENU_ITEM_HIDE,
    GUI_MENU_ITEM_UPDATE_STRING,
    GUI_MENU_ITEM_UPDATE_BITMAP,
    GUI_MENU_ITEM_HIGHLIGHT,
    GUI_MENU_ITEM_LOCK
} GUI_MENU_CMD_ID_e;

/*************************************************************************
 * Menu Widget GUI functions
 ************************************************************************/
extern int gui_menu_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_WIDGET_MENU_H_ */
