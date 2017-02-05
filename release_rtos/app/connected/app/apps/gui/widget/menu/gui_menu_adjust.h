/**
  * @file src/app/apps/gui/widget/menu/sportcam/gui_menu_adj.h
  *
  *  Header for adjusting Menu GUI flow
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
#ifndef APP_GUI_WIDGET_MENU_ADJUST_H_
#define APP_GUI_WIDGET_MENU_ADJUST_H_

__BEGIN_C_PROTO__

/*************************************************************************
 * Adjusting Menu GUI definitions
 ************************************************************************/
typedef enum _GUI_MENU_ADJ_CMD_ID_e_ {
    GUI_FLUSH = 0,
    GUI_HIDE_ALL,
    GUI_RESET,
    GUI_AMENU_SHOW,
    GUI_AMENU_HIDE,
    GUI_ICON_SHOW,
    GUI_ICON_HIDE,
    GUI_ICON_UPDATE,
    GUI_STATUS_SHOW,
    GUI_STATUS_HIDE,
    GUI_STATUS_UPDATE,
    GUI_VALUE_SHOW,
    GUI_VALUE_HIDE,
    GUI_VALUE_UPDATE
} GUI_MENU_ADJ_CMD_ID_e;

typedef struct _GUI_ADJ_STATUS_s_ {
    int Cur;
    int Step;
    int Max;
    int Min;
} GUI_ADJ_STATUS_s;

/*************************************************************************
 * Adjusting Menu Widget GUI functions
 ************************************************************************/
extern int gui_menu_adj_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_WIDGET_MENU_ADJUST_H_ */
