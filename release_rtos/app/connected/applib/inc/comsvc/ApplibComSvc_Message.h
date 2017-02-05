/**
 * @file src/app/connected/applib/inc/comsvc/ApplibComSvc_Message.h
 *
 * Header of common services' message.
 *
 * History:
 *    2014/04/15 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_COMSVC_MSG_H_
#define APPLIB_COMSVC_MSG_H_
/**
* @defgroup ApplibComSvc_Message
* @brief common services' message.
*
*
*/

/**
 * @addtogroup ApplibComSvc_Message
 * @ingroup CommonService
 * @{
 */
#include <applibhmi.h>

__BEGIN_C_PROTO__

/**********************************************************************/
/* MDL_APPLIB_COMSVC_ID messsages                                        */
/**********************************************************************/
/**
* Partition: |31 - 27|26 - 24|23 - 16|15 -  8| 7 -  0|
*   |31 - 27|: MDL_APPLIB_COMSVC_ID
*   |26 - 24|: MSG_TYPE_HMI
*   |23 - 16|: module or interface type ID
*   |15 -  8|: Self-defined
*   | 7 -  0|: Self-defined
* Note:
*   bit 0-15 could be defined in the module itself (individual
*   header files). However, module ID should be defined here
*   for arrangement
**/
#define HMSG_COMSVC_MODULE(x)  MSG_ID(MDL_APPLIB_COMSVC_ID, MSG_TYPE_HMI, (x))

#define HMSG_COMSVC_MODULE_ID_ASYNC            (0x01) /**< Sub-group:type of app library & interface events */
#define HMSG_COMSVC_MODULE_ID_TIMER            (0x02) /**< Sub-group:type of app library & interface events */

/** Async op module events */
/** Details in ApplibComSvc_AsyncOp.h */
#define HMSG_COMSVC_MODULE_ASYNC(x)    HMSG_COMSVC_MODULE(((UINT32)HMSG_COMSVC_MODULE_ID_ASYNC << 16) | (x))

/** Timer module events */
/** Details in ApplibComSvc_Timer.h */
#define HMSG_COMSVC_MODULE_TIMER(x)    HMSG_COMSVC_MODULE(((UINT32)HMSG_COMSVC_MODULE_ID_TIMER << 16) | (x))


__END_C_PROTO__

#endif /* APPLIB_COMSVC_MSG_H_ */

