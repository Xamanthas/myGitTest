/**
  * @file src/app/apps/flow/widget/widgetmgt.h
  *
  * Header of Widget Management
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
#ifndef APP_WIDGETMGT_H_
#define APP_WIDGETMGT_H_

#include <apps/apps.h>
__BEGIN_C_PROTO__

/*************************************************************************
 * Widget Definitions
 ************************************************************************/
#define WIDGET_NONE        (0x00000000)
#define WIDGET_MENU        (0x00000001)
#define WIDGET_MENU_QUICK    (0x00000002)
#define WIDGET_MENU_ADJ        (0x00000004)
#define WIDGET_MENU_TIME    (0x00000008)
#define WIDGET_DIALOG    (0x00000010)
#define WIDGET_MENU_ADAS_CALIB    (0x00000020)
#define WIDGET_ALL        (0xFFFFFFFF)

typedef struct _WIDGET_ITEM_s_ {
    int (*on)(UINT32 param);
    int (*off)(UINT32 param);
    int (*OnMessage)(UINT32 msg, UINT32 param1, UINT32 param2);
} WIDGET_ITEM_s;

/*************************************************************************
 * Widget APIs
 ************************************************************************/
extern int AppWidget_Init(void);
extern int AppWidget_GetCur(void);

extern int AppWidget_On(UINT32 widgetId, UINT32 param);
extern int AppWidget_Off(UINT32 widgetId, UINT32 param);
#define WIDGET_HIDE_SILENT    (0x80000000)
extern int AppWidget_OnMessage(UINT32 msg, UINT32 param1, UINT32 param2);
#define WIDGET_PASSED_MSG    (10)
extern UINT32 AppWidget_GetFlags(void);

__END_C_PROTO__

#endif /* APP_WIDGETMGT_H_ */
