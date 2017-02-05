/**
 * @file ambava_frmhdlr.h
 *
 * Amba fifo
 *
 * History:
 *    2014/12/4 - [cyweng] created file
 *
 * Copyright (C) 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef AMBAVA_FRMHDLR_H_
#define AMBAVA_FRMHDLR_H_

#include <recorder/Encode.h>

#define FRM_HDLR_MSGQUEUE_SIZE (128)
#define FRM_HDLR_MAX_CB  (16)
#define FRM_HDLR_MAX_EVENT  (16)
#define FRM_HDLR_DEF_PRIORITY  (90)
#define FRM_HDLR_DEF_TASK_STACK_SIZE  (32<<10)

typedef struct _AMBA_VA_CFG_t_ {
    UINT8 TaskPriority;
    UINT32 TaskStackSize;
    void* TaskStack;
} AMBA_VA_CFG;

typedef int (*VA_FRMHDLR_CB)(UINT32,
                             AMP_ENC_YUV_INFO_s*);

extern int AmbaVA_FrmHdlr_GetDefCfg(AMBA_VA_CFG* cfg);

extern int AmbaVA_FrmHdlr_Init(AMBA_VA_CFG* cfg);

extern int AmbaVA_FrmHdlr_Register(UINT32 event,
                                   VA_FRMHDLR_CB func);

extern int AmbaVA_FrmHdlr_UnRegister(UINT32 event,
                                     VA_FRMHDLR_CB func);

extern int AmbaVA_FrmHdlr_NewFrame(UINT32,
                                   AMP_ENC_YUV_INFO_s*);




#endif /* AMBAVA_FRMHDLR_H_ */
