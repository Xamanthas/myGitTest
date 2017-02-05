/**
 * @file src/app/connected/applib/inc/comsvc/ApplibComSvc_MemMgr.h
 *
 *  Header of of Application Memory manager
 *
 * History:
 *    2013/08/14 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_MEM_MGR_H_
#define APPLIB_MEM_MGR_H_
/**
* @defgroup ApplibComSvc_MemMgr
* @brief Application Memory manager
*
*
*/

/**
 * @addtogroup ApplibComSvc_MemMgr
 * @ingroup CommonService
 * @{
 */
#include <applib.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Definitions for memory application manager
 ************************************************************************/
extern AMBA_KAL_BYTE_POOL_t *APPLIB_G_MMPL;        /**< Cache memory pool for applib*/
extern AMBA_KAL_BYTE_POOL_t *APPLIB_G_NC_MMPL;     /**< non Cache memory pool for applib*/
extern AMBA_KAL_BYTE_POOL_t *APPLIB_FIFO_MMPL; /**< FIFO memory pool for applib*/
extern UINT8 *ApplibDspWorkAreaResvStart;          /**< Applib Dsp Work Area Resv Start*/
extern UINT8 *ApplibDspWorkAreaResvLimit;          /**< Applib Dsp Work Area Resv Limit*/
extern UINT32 ApplibDspWorkAreaResvSize;           /**< Applib Dsp Work Area Resv Size*/


/*************************************************************************
 * Declaration: Public APIs
 ************************************************************************/
/**
 *  To set the memory pool for applib.
 *
 *  @param [in] pMPL The memory address of Cache memory pool.
 *  @param [in] pNcMPL The memory address of Non-Cache memory pool.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcMemMgr_Init(AMBA_KAL_BYTE_POOL_t *pMPL, AMBA_KAL_BYTE_POOL_t *pNcMPL);

/**
 *  To set the DSP memory address
 *
 *  @param [in] resvStart The start address
 *  @param [in] resvLimit The end address
 *  @param [in] resvSize The size
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcMemMgr_SetDspMemory(UINT8 *resvStart , UINT8 *resvLimit, UINT32 resvSize);

/**
 *  To get the DSP memory address from end
 *
 *  @param [out] Addr start address
 *  @param [in] Size wanted memory size
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcMemMgr_AllocateDSPMemory(UINT8 **Addr, UINT32 Size);

/**
 *  To free the DSP memory app used
 *
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibComSvcMemMgr_FreeDSPMemory(void);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_MEM_MGR_H_ */
