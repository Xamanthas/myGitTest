/**
 * @file app/connected/app/system/app_util.h
 *
 * Header of Demo application utility
 *
 * History:
 *    2013/08/16 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APP_SYS_UTIL_H_
#define APP_SYS_UTIL_H_

#include <framework/appdefines.h>


__BEGIN_C_PROTO__

extern int AppUtil_Init(void);
extern int AppUtil_ReadyCheck(UINT32 param);
extern int AppUtil_BusyCheck(UINT32 param);
extern int AppUtil_SwitchApp(int appId);
extern int AppUtil_SwitchMode(UINT32 param);
extern int AppUtil_GetStartApp(UINT32 param);
extern int AppUtil_CheckCardParam(UINT32 param);
extern int AppUtil_GetVoutMode(int param);
extern int AppUtil_StatusInit(void);
extern int AppUtil_PollingAllSlots(void);
extern int AppUtil_AsciiToUnicode(char *ascStr, UINT16 *uniStr);

/**
 *  @brief Initialize the jack monitor
 *
 *  Initialize the jack monitor
 *
 *  @return >=0 success, <0 failure
 */
extern int AppUtil_JackMonitor(void);

/**
 *  @brief To check the USB charger
 *
 *  To check the USB charger
 *
 *  @param [in] enable Enable flag
 *
 *  @return >=0 success, <0 failure
 */
extern int AppUtil_UsbChargeCheckingSet(UINT32 enable);


__END_C_PROTO__

#endif /* APP_SYS_UTIL_H_ */
