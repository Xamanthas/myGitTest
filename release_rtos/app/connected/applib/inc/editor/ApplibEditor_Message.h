/**
 * @file src/app/connected/applib/inc/editor/ApplibEditor_Message.h
 *
 * Header of editor's message.
 *
 * History:
 *    2014/01/14 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_EDITOR_MSG_H_
#define APPLIB_EDITOR_MSG_H_

/**
* @defgroup ApplibEditor_Message
* @brief Editor related Message
*
*
*/

/**
 * @addtogroup ApplibEditor_Message
 * @ingroup Editor
 * @{
 */
#include <applibhmi.h>

__BEGIN_C_PROTO__


/**********************************************************************/
/* MDL_APPLIB_EDITOR_ID messsages                                        */
/**********************************************************************/
/**
* Partition: |31 - 27|26 - 24|23 - 16|15 -  8| 7 -  0|
*   |31 - 27|: MDL_APPLIB_EDITOR_ID
*   |26 - 24|: MSG_TYPE_HMI
*   |23 - 16|: module or interface type ID
*   |15 -  8|: Self-defined
*   | 7 -  0|: Self-defined
* Note:
*   bit 0-15 could be defined in the module itself (individual
*   header files). However, module ID should be defined here for arrangement
**/
#define HMSG_FORMAT_MODULE(x)  MSG_ID(MDL_APPLIB_FORMAT_ID, MSG_TYPE_HMI, (x))

#define HMSG_EDTMGR_SUCCESS        HMSG_FORMAT_MODULE(0x0000) /*< message of editor success*/
#define HMSG_EDTMGR_FAIL        HMSG_FORMAT_MODULE(0x0001) /*< message of editor fail*/
#define HMSG_EDTMGR_LIST_FULL        HMSG_FORMAT_MODULE(0x0002)/*< message of editor process list full*/
#define HMSG_EDTMGR_DISK_FULL        HMSG_FORMAT_MODULE(0x0003)/*< message of process disk editor*/
#define HMSG_EDTMGR_UNKNOWN_FORMAT    HMSG_FORMAT_MODULE(0x0004)/*< input unknown format message*/
#define HMSG_EDTMGR_ILLEGAL_INTERVAL    HMSG_FORMAT_MODULE(0x0005)/*< input time interval invalid*/
#define HMSG_EDTMGR_VIDEO_MISMATCH    HMSG_FORMAT_MODULE(0x0006)/*< input video mismatch*/
#define HMSG_EDTMGR_AUDIO_MISMATCH    HMSG_FORMAT_MODULE(0x0007)/*< input audio mismatch*/
#define HMSG_EDTMGR_EXT_MISMATCH    HMSG_FORMAT_MODULE(0x0008)/*< input ext mismatch*/
#define HMSG_EDTMGR_FORCE_STOP        HMSG_FORMAT_MODULE(0x0009)/*< force stop editor*/
#define HMSG_EDTMGR_MOVIE_RECOVER_BEGIN        HMSG_FORMAT_MODULE_MUXER(0x000A)/*< movie recover begin*/
#define HMSG_EDTMGR_MOVIE_RECOVER_COMPLETE    HMSG_FORMAT_MODULE_MUXER(0x000B)/*< movie recover finish*/

__END_C_PROTO__

/**
 * @}
 */

#endif /* APPLIB_EDITOR_MSG_H_ */

