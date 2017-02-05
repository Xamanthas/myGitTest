/**
  * @file src/app/apps/flow/misc/misc_defsetting.h
  *
  * Header of Reset the system setting to default application
  *
  * History:
  *    2014/03/20 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#ifndef APP_MISC_DEFSETTING_H_
#define APP_MISC_DEFSETTING_H_

#include <apps/apps.h>
#include <apps/flow/widget/widgetmgt.h>
#include <apps/gui/misc/gui_misc_fwupdate.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * App Flags Definitions
 ************************************************************************/

/*************************************************************************
 * App General Definitions
 ************************************************************************/

/*************************************************************************
 * App Function Definitions
 ************************************************************************/
typedef enum _MISC_DEFSETTING_FUNC_ID_e_ {
    MISC_DEFSETTING_START = 0,
    MISC_DEFSETTING_STOP,
    MISC_DEFSETTING_DIALOG_SHOW_DEFSETTING
} MISC_DEFSETTING_FUNC_ID_e;

extern int misc_defsetting_func(UINT32 funcId, UINT32 param1, UINT32 param2);
/*************************************************************************
 * App Operation Definitions
 ************************************************************************/

/*************************************************************************
 * App Status Definitions
 ************************************************************************/
typedef struct misc_defsetting_s {
    int (*Func)(UINT32 funcId, UINT32 param1, UINT32 param2);
} misc_defsetting_t;

extern misc_defsetting_t misc_defsetting;

__END_C_PROTO__

#endif /* APP_MISC_DEFSETTING_H_ */
