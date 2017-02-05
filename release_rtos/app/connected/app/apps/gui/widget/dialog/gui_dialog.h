/**
  * @file src/app/apps/gui/dialog/gui_dialog.h
  *
  * Header for Dialog GUI flow
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
#ifndef APP_GUI_WIDGET_DIALOG_H_
#define APP_GUI_WIDGET_DIALOG_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaDataType.h>
#include <applib.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Dialog GUI definitions
 ************************************************************************/
typedef enum _GUI_DIALOG_OPT_ID_e_ {
    GUI_OPT_1 = 0,
    GUI_OPT_2,
    GUI_OPT_3,
    GUI_OPT_4,
    GUI_OPT_5,
    GUI_OPT_NUM,
    GUI_OPT_HL
} GUI_DIALOG_OPT_ID_e;

typedef enum _GUI_DIALOG_BTN_ID_e_ {
    GUI_DIALOG_BTN_CANCEL = 0,
    GUI_DIALOG_BTN_SET,
    GUI_DIALOG_BTN_NUM
} GUI_DIALOG_BTN_ID_e;

typedef enum _GUI_DIALOG_CMD_ID_e_ {
    GUI_FLUSH = 0,
    GUI_HIDE_ALL,
    GUI_RESET,
    GUI_DIALOG_SHOW,
    GUI_DIALOG_HIDE,
    GUI_DIALOG_BTN_SHOW,
    GUI_DIALOG_BTN_HIDE,
    GUI_DIALOG_BTN_HIGHLIGHT,
    GUI_OPT_SHOW,
    GUI_OPT_HIDE,
    GUI_OPT_UPDATE,
    GUI_OPT_HIGHLIGHT,
    GUI_DESC_SHOW,
    GUI_DESC_HIDE,
    GUI_DESC_UPDATE,
} GUI_DIALOG_CMD_ID_e;
/*************************************************************************
 * Dialog Widget GUI functions
 ************************************************************************/
extern int gui_dialog_func(UINT32 guiCmd, UINT32 param1, UINT32 param2);

__END_C_PROTO__

#endif /* APP_GUI_WIDGET_DIALOG_H_ */
