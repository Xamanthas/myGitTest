/**
 * @file src/app/connected/applib/inc/recorder/ApplibRecorder_LoopEnc.h
 *
 * Header of Loop Encoder manager
 *
 * History:
 *    2014/04/11 - [Annie Ting] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_LOOP_ENC_MGR_H_
#define APPLIB_LOOP_ENC_MGR_H_

/**
* @defgroup ApplibRecorder_LoopEnc
* @brief Loop encode related function
*
*
*/

/**
 * @addtogroup ApplibRecorder_LoopEnc
 * @ingroup Recorder
 * @{
 */
#include <applib.h>

__BEGIN_C_PROTO__



/**info[1:0] represnt step finish*/
#define LOOP_ENC_SEARCH_DONE          0x01       /**<LOOP ENCODE STATUS MSG*/
#define LOOP_ENC_HANDLE_DONE          0x02       /**<LOOP ENCODE STATUS MSG*/
#define LOOP_ENC_SEARCH_ERROR        0x04        /**<LOOP ENCODE STATUS MSG*/
#define LOOP_ENC_HANDLE_ERROR        0x08        /**<LOOP ENCODE STATUS MSG*/
#define LOOP_ENC_CHECK_SEARCH        0x10        /**<LOOP ENCODE STATUS MSG*/
#define LOOP_ENC_CHECK_HANDLE        0x20        /**<LOOP ENCODE STATUS MSG*/

/**
 *  @brief Send message to Loop Encoder Mgr.
 *
 *  Send message to  Loop Encoder Mgr.
 *
 *  @param [in] msg Message ID
 *  @param [in] param1 first parameter
 *  @param [in] param2 second parameter
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibLoopEnc_SndMsg(UINT32 msg, UINT32 param1, UINT32 param2);



/**
 *  @brief Initialization of Loop Encoder manager.
 *
 *   Initialization of Loop Encoder manager.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibLoopEnc_Init(void);

/**
 *  @brief feed back loop enc function status
 *
 *  return loop enc result to app
 *
 *
 *  @return 0:do nothing 1:Loop enc all done 2:search file done
 *                       -1:Search file error  -2:Delete file error
 *
 */
extern int AppLibLoopEnc_StepCheck(void);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_LOOP_ENC_MGR_H_ */
