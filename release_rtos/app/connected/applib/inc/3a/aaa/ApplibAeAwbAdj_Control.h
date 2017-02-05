/**
 * @file src/app/connected/applib/inc/3a/aaa/ApplibAeAwbAdj_Control.h
 *
 * Header of Ae/Awb/Adj Algo. control.
 *
 * History:
 *    2013/09/23 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_AEAWBADJ_CONTROL_H_
#define APPLIB_AEAWBADJ_CONTROL_H_

#include <applib.h>

__BEGIN_C_PROTO__

/**
 *  To initialize the AE/AWB/ADJ algo related settings
 *
 *  @param [in] chNo Handler for multi-task.
 *  @param [in] initFlg Initial flag, All(0), Ae(1), Awb(2), Adj(3)
 *  @param [in] pMMPL Pointer to memory pool control block
 *
 */
extern void AppLibAeAwbAdj_Init(UINT32 chNo,UINT8 initFlg, AMBA_KAL_BYTE_POOL_t *pMMPL);

/* function prototype */
/**
 *  @brief Entry point of AE/AWB/ADJ algo.
 *
 *  @param [in] chNo  Handler for multi-task.
 *
 */
extern void AppLibAeAwbAdj_Control(UINT32 chNo);
/**
 *  @brief Entry point of Ae algo.
 *
 *  @param [in] chNo  Handler for multi-task.
 *
 *  @return >=0 success, <0 failure
 */
extern void AppLibAe_Ctrl(UINT32 chNo);
/**
 *  @brief Entry point of Awb algo.
 *
 *  @param [in] chNo  Handler for multi-task.
 *
 *  @return >=0 success, <0 failure
 */
extern void AppLibAwb_Ctrl(UINT32 chNo);
/**
 *  @brief Entry point of ADJ algo.
 *
 *  @param [in] chNo  Handler for multi-task.
 *
 *  @return >=0 success, <0 failure
 */
extern void AppLibAdj_Ctrl(UINT32 chNo);

__END_C_PROTO__

#endif /* APPLIB_AEAWBADJ_CONTROL_H_ */

