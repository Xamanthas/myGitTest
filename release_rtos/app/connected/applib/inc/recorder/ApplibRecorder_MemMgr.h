/**
 * @file src/app/connected/applib/inc/recorder/ApplibRecorder_MemMgr.h
 *
 * Header of recorder's buffer manager
 *
 * History:
 *    2014/05/27 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_RECORD_MEMMGR_H_
#define APPLIB_RECORD_MEMMGR_H_
/**
* @defgroup ApplibRecorder_MemMgr
* @brief recorder's buffer manager
*
*
*/

/**
 * @addtogroup ApplibRecorder_MemMgr
 * @ingroup Recorder
 * @{
 */
#include <applib.h>
__BEGIN_C_PROTO__

/**
 *
 * Allocate buffer for recorder
 *
 * @return >=0 success, <0 failure.
 */
extern int AppLibRecorderMemMgr_BufAllocate(void);

/**
 *
 * Free buffer for recorder
 *
 * @return >=0 success, <0 failure.
 */
extern int AppLibRecorderMemMgr_BufFree(void);

/**
 *  Set the buffer size
 *
 *  @param [in] bitsBufSize Bits fifo buffer size
 *  @param [in] descBufSize Fifo description buffer size
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibRecorderMemMgr_SetBufSize(UINT32 bitsBufSize, UINT32 descBufSize);

/**
 *
 *Get buffer address for recorder
 *
 * @return >=0 success, <0 failure.
 */
extern int AppLibRecorderMemMgr_GetBufAddr(UINT8 **bitsBufAddr, UINT8 **descBufAddr);


/**
 *
 *Get buffer size for recorder
 *
 * @return >=0 success, <0 failure.
 */
 extern int AppLibRecorderMemMgr_GetBufSize(UINT32 *bitsBufSize, UINT32 *descBufSize);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_RECORD_MEMMGR_H_ */

