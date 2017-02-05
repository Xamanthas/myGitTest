/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIPC_RpcProg_R_Msg.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions for AmbaIPC Message Framework.
 *
 *  @History        ::
 *      Date        Name        			Comments
 *      12/12/2013  Keny       				Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/
#ifndef _AMBA_IPC_RPC_PROG_R_MSG_H_
#define _AMBA_IPC_RPC_PROG_R_MSG_H_

#include "AmbaIPC_Rpc_Def.h"

//RPC_INFO definition
#define AMBA_RPC_PROG_R_MSG_PROG_ID	0x10000002
#define AMBA_RPC_PROG_R_MSG_HOST AMBA_IPC_HOST_THREADX
#define AMBA_RPC_PROG_R_MSG_VER (1)
#define AMBA_RPC_PROG_R_MSG_DEFULT_TIMEOUT (500)

//RPC_FUNC definition
#define AMBA_RPC_PROG_R_MSG_FUNC_SEND (1)

typedef struct _AMBA_RPC_PROG_R_MSG_MSGBLK_s_ {
	unsigned char priority;
	unsigned int src_mqid;
	unsigned int param_size;
	unsigned char param[256]; /* Message contents */
} AMBA_RPC_PROG_R_MSG_MSGBLK_s;

AMBA_IPC_REPLY_STATUS_e AmbaRpcProg_R_Msg_Send_Clnt(AMBA_RPC_PROG_R_MSG_MSGBLK_s *pArg, int *pResult, int Clnt );
void AmbaRpcProg_R_Msg_Send_Svc(AMBA_RPC_PROG_R_MSG_MSGBLK_s *pArg, AMBA_IPC_SVC_RESULT_s *pRet);

#endif /* _AMBA_IPC_RPC_PROG_R_MSG_H_ */
