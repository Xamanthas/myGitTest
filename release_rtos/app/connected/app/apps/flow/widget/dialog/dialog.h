/**
  * @file src/app/apps/flow/widget/dialog/dialog.h
  *
  * Header of Dialog
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

#ifndef APP_WIDGET_DIALOG_H_
#define APP_WIDGET_DIALOG_H_


#include <apps/flow/widget/widgetmgt.h>
#include <apps/flow/widget/dialog/dialog_table.h>


__BEGIN_C_PROTO__

/*************************************************************************
 * Dialog definitions
 ************************************************************************/
typedef enum _DIALOG_TYPE_ID_e_ {
    DIALOG_TYPE_OK = 0,
    DIALOG_TYPE_Y_N,
    DIALOG_TYPE_A_Y_N,
    DIALOG_TYPE_NUM
} DIALOG_TYPE_ID_e;

typedef int (*dialog_set_handler)(UINT32 sel, UINT32 param1, UINT32 param2);
#define DIALOG_SEL_OK    (0x00)
#define DIALOG_SEL_NO    (0x00)
#define DIALOG_SEL_YES    (0x01)
#define DIALOG_SEL_ALL    (0x02)

/*************************************************************************
 * Dialog APIs for widget management
 ************************************************************************/
extern WIDGET_ITEM_s* AppDialog_GetWidget(void);

/*************************************************************************
 * Public Dialog Widget APIs
 ************************************************************************/
extern int AppDialog_SetDialog(UINT32 type, UINT32 subject, dialog_set_handler set);
extern int AppDialog_SetDialogTable(UINT32 *table);

__END_C_PROTO__

#endif /* APP_WIDGET_DIALOG_H_ */
