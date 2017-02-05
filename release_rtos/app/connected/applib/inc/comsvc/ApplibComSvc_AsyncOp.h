/**
 * @file src/app/connected/applib/inc/comsvc/ApplibComSvc_AsyncOp.h
 *
 * Header of Async Operation - APP level
 *
 * History:
 *    2013/09/09 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_ASYNC_OP_H_
#define APPLIB_ASYNC_OP_H_
/**
* @defgroup ApplibComSvc_AsyncOp
* @brief Async Operation - APP level
*
*
*/

/**
 * @addtogroup ApplibComSvc_AsyncOp
 * @ingroup CommonService
 * @{
 */
#include <applib.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Async OP manager command messages
 ************************************************************************/
#define ASYNC_MGR_CMD_SHUTDOWN      HMSG_COMSVC_MODULE_ASYNC(0x0001)           /**<ASYNC_MGR_CMD_SHUTDOWN    */
#define ASYNC_MGR_CMD_CARD_FORMAT   HMSG_COMSVC_MODULE_ASYNC(0x0002)           /**<ASYNC_MGR_CMD_CARD_FORMAT */
#define ASYNC_MGR_CMD_CARD_INSERT   HMSG_COMSVC_MODULE_ASYNC(0x0003)           /**<ASYNC_MGR_CMD_CARD_INSERT */
#define ASYNC_MGR_CMD_FILE_COPY     HMSG_COMSVC_MODULE_ASYNC(0x0004)           /**<ASYNC_MGR_CMD_FILE_COPY   */
#define ASYNC_MGR_CMD_FILE_MOVE     HMSG_COMSVC_MODULE_ASYNC(0x0005)           /**<ASYNC_MGR_CMD_FILE_MOVE   */
#define ASYNC_MGR_CMD_FILE_DEL      HMSG_COMSVC_MODULE_ASYNC(0x0006)           /**<ASYNC_MGR_CMD_FILE_DEL    */
#define ASYNC_MGR_CMD_DMF_FCOPY     HMSG_COMSVC_MODULE_ASYNC(0x0007)           /**<ASYNC_MGR_CMD_DMF_FCOPY   */
#define ASYNC_MGR_CMD_DMF_FMOVE     HMSG_COMSVC_MODULE_ASYNC(0x0008)           /**<ASYNC_MGR_CMD_DMF_FMOVE   */
#define ASYNC_MGR_CMD_DMF_FDEL      HMSG_COMSVC_MODULE_ASYNC(0x0009)           /**<ASYNC_MGR_CMD_DMF_FDEL    */
#define ASYNC_MGR_CMD_DMF_FAST_FDEL_ALL HMSG_COMSVC_MODULE_ASYNC(0x000a)       /**<ASYNC_MGR_CMD_DMF_FAST_FDEL_ALL*/
#define ASYNC_MGR_CMD_CALIB_LOAD_DATA   HMSG_COMSVC_MODULE_ASYNC(0x000b)       /**<ASYNC_MGR_CMD_CALIB_LOAD_DATA*/

#define ASYNC_MGR_MSG_OP_DONE(x)    ((x) | 0x8000)        /**<ASYNC_MGR_MSG_OP_DONE(x)*/
#define ASYNC_MGR_MSG_SHUTDOWN_DONE     ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_SHUTDOWN)                  /**<ASYNC_MGR_MSG_SHUTDOWN_DONE   */
#define ASYNC_MGR_MSG_CARD_FORMAT_DONE  ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_CARD_FORMAT)               /**<ASYNC_MGR_MSG_CARD_FORMAT_DONE*/
#define ASYNC_MGR_MSG_CARD_INSERT_DONE  ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_CARD_INSERT)               /**<ASYNC_MGR_MSG_CARD_INSERT_DONE*/
#define ASYNC_MGR_MSG_FILE_COPY_DONE    ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_FILE_COPY)                 /**<ASYNC_MGR_MSG_FILE_COPY_DONE  */
#define ASYNC_MGR_MSG_FILE_MOVE_DONE    ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_FILE_MOVE)                 /**<ASYNC_MGR_MSG_FILE_MOVE_DONE  */
#define ASYNC_MGR_MSG_FILE_DEL_DONE     ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_FILE_DEL)                  /**<ASYNC_MGR_MSG_FILE_DEL_DONE   */
#define ASYNC_MGR_MSG_DMF_FCOPY_DONE    ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_DMF_FCOPY)                 /**<ASYNC_MGR_MSG_DMF_FCOPY_DONE  */
#define ASYNC_MGR_MSG_DMF_FMOVE_DONE    ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_DMF_FMOVE)                 /**<ASYNC_MGR_MSG_DMF_FMOVE_DONE  */
#define ASYNC_MGR_MSG_DMF_FDEL_DONE     ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_DMF_FDEL)                  /**<ASYNC_MGR_MSG_DMF_FDEL_DONE   */
#define ASYNC_MGR_MSG_DMF_FAST_FDEL_ALL_DONE    ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_DMF_FAST_FDEL_ALL) /**<ASYNC_MGR_MSG_DMF_FAST_FDEL_ALL_DONE*/
#define ASYNC_MGR_MSG_CALIB_LOAD_DATA_DONE      ASYNC_MGR_MSG_OP_DONE(ASYNC_MGR_CMD_CALIB_LOAD_DATA)   /**<ASYNC_MGR_MSG_CALIB_LOAD_DATA_DONE*/


/**
 *  Initialization of Async operation.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_Init(void);

/**
 *  To shut down the system
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_Shutdown(void);

/**
 *  To format card
 *
 *  @param [in] slot Card slot id
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_CardFormat(int slot);

/**
 *  To insert card
 *
 *  @param [in] slot Card slot id
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_CardInsert(int slot);

/**
 *  To copy the file
 *
 *  @param [in] srcFn Source file name
 *  @param [in] dstFn destination file name
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_FileCopy(char *srcFn, char *dstFn);

/**
 *  To move the file
 *
 *  @param [in] srcFn Source file name
 *  @param [in] dstFn Destination file name
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_FileMove(char *srcFn, char *dstFn);

/**
 *  To delete the file
 *
 *  @param [in] filename File name
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_FileDel(char *filename);

/**
 *  To copy file with DMF rule
 *
 *  @param [in] dmfRootType Media root type
 *  @param [in] srcFn source file name
 *  @param [in] dstFn destination file name
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_DmfFcopy(int dmfRootType, char *srcFn, char *dstFn);

/**
 *  Move file with DMF rule
 *
 *  @param [in] dmfRootType Media root type
 *  @param [in] srcFn source file name
 *  @param [in] dstFn destination file name
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_DmfFmove(int dmfRootType, char *srcFn, char *dstFn);

/**
 *  Delete file with DMF rule
 *
 *  @param [in] dmfRootType Media root type
 *  @param [in] filename  File name
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_DmfFdel(int dmfRootType, UINT32 FileObjID, UINT32 handler);
/**
 *  Delete all files with DMF rule
 *
 *  @param [in] dmfRootType Media root type
 *  @param [in] param Parameter
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_DmfFastFdelAll(int dmfRootType, UINT32 param);

/**
 *  Load calibration data
 *
 *  @param [in] stage Stage ID
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcAsyncOp_CalibLoadData(int stage);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_ASYNC_OP_H_ */
