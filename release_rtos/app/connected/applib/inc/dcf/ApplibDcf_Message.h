/**
 * @file src/app/connected/applib/inc/dcf/ApplibDcf_Message.h
 *
 * Header of dcf message.
 *
 * History:
 *    2014/05/15 - [Annie Ting] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_DCF_MSG_H_
#define APPLIB_DCF_MSG_H_
/**
* @defgroup ApplibDcf_Message
* @brief dcf message
*
*
*/

/**
 * @addtogroup ApplibDcf_Message
 * @ingroup DCF
 * @{
 */
#include <applibhmi.h>

__BEGIN_C_PROTO__


/**********************************************************************/
/* MDL_APPLIB_DCF_ID messsages                                        */
/**********************************************************************/
/**
* Partition: |31 - 27|26 - 24|23 - 16|15 -  8| 7 -  0|
*   |31 - 27|: MDL_APPLIB_DCF_ID
*   |26 - 24|: MSG_TYPE_HMI
*   |23 - 16|: module or interface type ID
*   |15 -  8|: Self-defined
*   | 7 -  0|: Self-defined
* Note:
*   bit 0-15 could be defined in the module itself (individual
*   header files). However, module ID should be defined here for arrangement
**/
#define HMSG_DCF_MODULE(x)  MSG_ID(MDL_APPLIB_DCF_ID, MSG_TYPE_HMI, (x))
/** Sub-group:type of app library & interface events */
#define HMSG_DCF_MODULE_ID_CFS   (0x01)

/** CFS MODE MSG */
#define HMSG_DCF_MODULE_CFS(x)   HMSG_DCF_MODULE(((UINT32)HMSG_DCF_MODULE_ID_CFS << 16) | (x))
#define HMSG_DCF_FILE_CLOSE               HMSG_DCF_MODULE_CFS(0x0001) /**<HMSG_DCF_FILE_CLOSE*/





__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_DCF_MSG_H_ */

