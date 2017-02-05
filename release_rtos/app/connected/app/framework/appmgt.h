/**
  * @file src/app/framework/appmgt.h
  *
  * Header of Application Management
  *
  * History:
  *    2013/07/09 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef APP_APPMGT_H_
#define APP_APPMGT_H_

#include <framework/appdefines.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Application Structure Definitions
 ************************************************************************/
typedef struct _APP_APP_s_ {
    int Id;
    int Tier;
    int Parent;
    int Previous;
    int Child;
    UINT32 GFlags;
    UINT32 Flags;
    int (*Start)(void);
    int (*Stop)(void);
    int (*OnMessage)(UINT32 msg, UINT32 param1, UINT32 param2);
} APP_APP_s;

extern int AppAppMgt_Init(APP_APP_s **sysApps, int appMaxNum);
extern int AppAppMgt_Register(APP_APP_s *app);
extern int AppAppMgt_GetApp(int appId, APP_APP_s **app);
extern int AppAppMgt_GetCurApp(APP_APP_s **app);
extern int AppAppMgt_GetCurT1App(APP_APP_s **app);
extern int AppAppMgt_SwitchApp(int appId);

extern int AppAppMgt_CheckBusy(void);
extern int AppAppMgt_CheckIo(void);
extern int AppAppMgt_CheckIdle(void);

__END_C_PROTO__

#endif /* APP_APPMGT_H_ */
