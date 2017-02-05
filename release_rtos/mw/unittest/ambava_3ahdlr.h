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

#ifndef AMBAVA_3AHDLR_H_
#define AMBAVA_3AHDLR_H_

#include <recorder/Encode.h>
#include "AmbaDSP_Event.h"

#define TRIA_HDLR_MSGQUEUE_SIZE (128)
#define TRIA_HDLR_MAX_CB  (16)
#define TRIA_HDLR_DEF_PRIORITY  (89)
#define TRIA_HDLR_DEF_TASK_STACK_SIZE  (32<<10)


typedef struct _AMBA_TRIA_CFG_t_ {
    UINT8 TaskPriority;
    UINT32 TaskStackSize;
    void* TaskStack;
} AMBA_TRIA_CFG;

typedef int (*VA_TRIAHDLR_CB)(UINT32,
                             AMBA_DSP_EVENT_CFA_3A_DATA_s*);

extern int AmbaVA_TriAHdlr_GetDefCfg(AMBA_TRIA_CFG* cfg);

extern int AmbaVA_TriAHdlr_Init(AMBA_TRIA_CFG* cfg);

extern int AmbaVA_TriAHdlr_Register(UINT32 event,
                                   VA_TRIAHDLR_CB func);

extern int AmbaVA_TriAHdlr_UnRegister(UINT32 event,
                                     VA_TRIAHDLR_CB func);

extern int AmbaVA_TriAHdlr_NewFrame(UINT32,
                                   AMBA_DSP_EVENT_CFA_3A_DATA_s* pData3A);

extern int AmbaVA_DSP_EventHdlr_NewAE(AMBA_DSP_EVENT_CFA_3A_DATA_s* pData3A);




#endif /* AMBAVA_FRMHDLR_H_ */

