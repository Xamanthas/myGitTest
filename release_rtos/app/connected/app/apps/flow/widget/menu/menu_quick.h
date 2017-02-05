/**
  * @file src/app/apps/flow/widget/menu/connectedcam/menu_quick.h
  *
  * Header of Quick Menu
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
#ifndef APP_WIDGET_MENU_QUICK_H_
#define APP_WIDGET_MENU_QUICK_H_


#include <apps/flow/widget/widgetmgt.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Quick Menu definitions
 ************************************************************************/

/*************************************************************************
 * Quick Menu APIs for widget management
 ************************************************************************/
extern WIDGET_ITEM_s* AppMenuQuick_GetWidget(void);

/*************************************************************************
 * Public Quick Menu Widget APIs
 ************************************************************************/
extern int AppMenuQuick_SetItem(UINT32 tab_id, UINT32 item_id);

__END_C_PROTO__

#endif /* APP_WIDGET_MENU_QUICK_H_ */
