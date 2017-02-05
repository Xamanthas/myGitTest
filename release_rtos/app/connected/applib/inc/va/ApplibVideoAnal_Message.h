/**
 * @file
 *       src/app/connected/applib/inc/va/ApplibVideoAnal_Message.h
 *
 * Header of recorder's message.
 *
 * History:
 *    2015/01/07 - [Bill Chou] created file
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_VIDEO_ANAL_MSG_H_
#define APPLIB_VIDEO_ANAL_MSG_H_
/**
* @defgroup ApplibVideoAnal_Message
* @brief Message define for VideoAnal functions
*
*
*/

/**
 * @addtogroup ApplibVideoAnal_Message
 * @ingroup Recorder
 * @{
 */
#include <applibhmi.h>

__BEGIN_C_PROTO__

/**********************************************************************/
/* MDL_APPLIB_VA_ID messsages                                        */
/**********************************************************************/
/**
* Partition: |31 - 27|26 - 24|23 - 16|15 -  8| 7 -  0|
*   |31 - 27|: MDL_APPLIB_VA_ID
*   |26 - 24|: MSG_TYPE_HMI
*   |23 - 16|: module or interface type ID
*   |15 -  8|: Self-defined
*   | 7 -  0|: Self-defined
* Note:
*   bit 0-15 could be defined in the module itself (individual
*   header files). However, module ID should be defined here for arrangement
**/
#define HMSG_VA_MODULE(x)  MSG_ID(MDL_APPLIB_VA_ID, MSG_TYPE_HMI, (x))
/** Sub-group:type of interface events */
#define HMSG_VA_MODULE_ID_TASK  (0x01)
#define HMSG_VA_MODULE_ID_FCMD  (0x02)
#define HMSG_VA_MODULE_ID_LLWS  (0x03)
#define HMSG_VA_MODULE_ID_MD    (0x04)
#define HMSG_VA_MODULE_ID_LDWS  (0x05)
#define HMSG_VA_MODULE_ID_FCWS  (0x06)
#define HMSG_VA_MODULE_ID_ADAS  (0x07)
#define HMSG_VA_MODULE_ID_STMP  (0x08)
#define HMSG_VA_MODULE_ID_ERROR (0x0F)

/** The  status TASK.*/
#define HMSG_VA_MODULE_TASK(x)  HMSG_VA_MODULE(((UINT32)HMSG_VA_MODULE_ID_TASK << 16) | (x))
#define HMSG_VA_TASKYUV_ON          HMSG_VA_MODULE_TASK(0x0001)
#define HMSG_VA_TASK3A_ON           HMSG_VA_MODULE_TASK(0x0002)

/** The  status FCMD.*/
#define HMSG_VA_MODULE_FCMD(x)  HMSG_VA_MODULE(((UINT32)HMSG_VA_MODULE_ID_FCMD << 16) | (x))
#define HMSG_VA_FCAR_DEPARTURE      HMSG_VA_MODULE_FCMD(0x0001)

/** The  status about LLWS.*/
#define HMSG_VA_MODULE_LLWS(x)  HMSG_VA_MODULE(((UINT32)HMSG_VA_MODULE_ID_LLWS << 16) | (x))
#define HMSG_VA_LOW_LIGHT           HMSG_VA_MODULE_LLWS(0x0001)


/** The  status about MD.*/
#define HMSG_VA_MODULE_MD(x)    HMSG_VA_MODULE(((UINT32)HMSG_VA_MODULE_ID_MD << 16) | (x))
#define HMSG_VA_MD_Y                HMSG_VA_MODULE_MD(0x0001)
#define HMSG_VA_MD_AE               HMSG_VA_MODULE_MD(0x0002)

/** The  status LDWS.*/
#define HMSG_VA_MODULE_LDW(x)   HMSG_VA_MODULE(((UINT32)HMSG_VA_MODULE_ID_LDWS << 16) | (x))
#define HMSG_VA_LDW                 HMSG_VA_MODULE_LDW(0x0001)

/** The  status FCWS.*/
#define HMSG_VA_MODULE_FCW(x)   HMSG_VA_MODULE(((UINT32)HMSG_VA_MODULE_ID_FCWS << 16) | (x))
#define HMSG_VA_FCW                 HMSG_VA_MODULE_FCW(0x0001)

#define HMSG_VA_MODULE_ADAS(x)  HMSG_VA_MODULE(((UINT32)HMSG_VA_MODULE_ID_ADAS << 16) | (x))
#define HMSG_VA_CLIBRATION_DONE     HMSG_VA_MODULE_ADAS(0x0001)

#define HMSG_VA_MODULE_STMP(x)  HMSG_VA_MODULE(((UINT32)HMSG_VA_MODULE_ID_STMP << 16) | (x))
#define HMSG_VA_UPDATE              HMSG_VA_MODULE_STMP(0x0001)

#define HMSG_VA_MODULE_ERROR(x)   HMSG_VA_MODULE(((UINT32)HMSG_VA_MODULE_ID_ERROR << 16) | (x))

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_VIDEO_ANAL_MSG_H_ */

