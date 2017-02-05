/**
 * @file src/app/connected/applib/inc/va/ApplibVideoAnal_StmpHdlr.h
 *
 * Header of VA Stamp Handler APIs
 *
 * History:
 *    2015/01/06 - [Bill Chou] created file
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_VIDEO_ANAL_STMPHDLR_H_
#define APPLIB_VIDEO_ANAL_STMPHDLR_H_

#include <applib.h>
__BEGIN_C_PROTO__

/*************************************************************************
 * Video definitions
 ************************************************************************/
//#define ENABLE_VA_STAMP

#define VA_STMPHDLR_LDWS    (0x00000001)
#define VA_STMPHDLR_FCWS    (0x00000002)
#define VA_STMPHDLR_FCMD    (0x00000004)
#define VA_STMPHDLR_LLWS    (0x00000008)
#define VA_STMPHDLR_MD      (0x00000010)

#define VA_STMPHDLR_CALIB   (0x00000100)

#define VA_STMPHDLR_KEEP_TIME   (2) // in second

/*************************************************************************
 * Stamp Handler Setting APIs
 ************************************************************************/

extern int AppLibVideoAnal_StmpHdlr_AddEvent(UINT32 VaEvent);
extern UINT32 AppLibVideoAnal_StmpHdlr_GetEventFlag(void);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_VIDEO_ANAL_STMPHDLR_H_ */

