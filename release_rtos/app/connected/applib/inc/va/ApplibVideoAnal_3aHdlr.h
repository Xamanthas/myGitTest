/**
 * @file app/connected/applib/inc/va/ApplibVideoAnal_3aHdlr.h
 *
 * header of VA frame 3A info handler
 *
 * History:
 *    2015/01/09 - [Bill Chou] created file
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_VIDEO_ANAL_3AHDLR_H_
#define APPLIB_VIDEO_ANAL_3AHDLR_H_

#include <recorder/Encode.h>
#include "AmbaDSP_Event.h"

#define APPLIB_TRIA_HDLR_MSGQUEUE_SIZE (128)
#define APPLIB_TRIA_HDLR_MAX_CB  (16)
#define APPLIB_TRIA_HDLR_DEF_PRIORITY  (135)
#define APPLIB_TRIA_HDLR_DEF_TASK_STACK_SIZE  (32<<10)


typedef struct APPLIB_TRIA_TASK_CFG_t_ {
    UINT8 TaskPriority;
    UINT32 TaskStackSize;
    void* TaskStack;
} APPLIB_TRIA_TASK_CFG_t;

typedef int (*APPLIB_VA_TRIAHDLR_CB)(UINT32, AMBA_DSP_EVENT_CFA_3A_DATA_s*);

extern int AppLibVideoAnal_TriAHdlr_IsInit(void);

extern int AppLibVideoAnal_TriAHdlr_GetDefCfg(APPLIB_TRIA_TASK_CFG_t* cfg);

extern int AppLibVideoAnal_TriAHdlr_Init(void);

extern int AppLibVideoAnal_TriAHdlr_Register(UINT32 event, APPLIB_VA_TRIAHDLR_CB func);

extern int AppLibVideoAnal_TriAHdlr_UnRegister(UINT32 event, APPLIB_VA_TRIAHDLR_CB func);

extern int AppLibVideoAnal_TriAHdlr_NewFrame(UINT32, AMBA_DSP_EVENT_CFA_3A_DATA_s* pData3A);

extern int AppLibVideoAnal_DSP_EventHdlr_NewAE(AMBA_DSP_EVENT_CFA_3A_DATA_s* pData3A);


#endif /* APPLIB_VIDEO_ANAL_3AHDLR_H_ */

