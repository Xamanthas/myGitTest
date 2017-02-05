/**
  * @file src/app/apps/gui/widget/menu/sportcam/gui_menu_time.h
  *
  * Header for Time Menu GUI flow
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

#ifndef APP_GUI_WIDGET_MENU_TIME_H_
#define APP_GUI_WIDGET_MENU_TIME_H_

__BEGIN_C_PROTO__

/*************************************************************************
 * Time Menu GUI definitions
 ************************************************************************/
typedef enum _GUI_MENU_TIME_CMD_ID_e_ {
    GUI_FLUSH = 0,
    GUI_HIDE_ALL,
    GUI_TMENU_SHOW,
    GUI_TMENU_HIDE,
    GUI_VALUE_SHOW,
    GUI_VALUE_HIDE,
    GUI_VALUE_HL,
    GUI_VALUE_UPDATE
} GUI_MENU_TIME_CMD_ID_e;

typedef enum _GUI_MENU_TIME_VAL_ID_e_ {
    GUI_YEAR = 0,
    GUI_MONTH,
    GUI_DAY,
    GUI_HOUR,
    GUI_MINUTE,
    GUI_VALUE_NUM
} GUI_MENU_TIME_VAL_ID_e;

/*************************************************************************
 * Time Menu Widget GUI functions
 ************************************************************************/
extern int gui_menu_time_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_WIDGET_MENU_TIME_H_ */
