 /**
  * @file src/app/framework/appdefines.h
  *
  * Application definition header file.
  *
  * History:
  *    2013/07/05 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef APP_DEFINES_H_
#define APP_DEFINES_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaDataType.h>
#include <mw.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Application Global Flags
 ************************************************************************/
#define APP_AFLAGS_INIT         (0x00000001)
#define APP_AFLAGS_START      (0x00000002)
#define APP_AFLAGS_READY     (0x00000004)
#define APP_AFLAGS_OVERLAP (0x00000008)
#define APP_AFLAGS_BUSY       (0x00000010)
#define APP_AFLAGS_IO           (0x00000020)
#define APP_AFLAGS_ERROR     (0x00000040)
#define APP_AFLAGS_POPUP     (0x00000080)

#define APP_ADDFLAGS(x, y)          ((x) |= (y))
#define APP_REMOVEFLAGS(x, y)    ((x) &= (~(y)))
#define APP_CHECKFLAGS(x, y)       ((x) & (y))

__END_C_PROTO__

#endif /* APP_DEFINES_H_ */
