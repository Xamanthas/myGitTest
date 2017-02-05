/**
 * @file src/app/connected/applib/inc/va/ApplibVideoAnal_FrmHdlr.h
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

#ifndef APPLIB_VIDEO_ANAL_FRMHDLR_H_
#define APPLIB_VIDEO_ANAL_FRMHDLR_H_

#include <recorder/Encode.h>

#define APPLIB_FRM_HDLR_MSGQUEUE_SIZE (128)
#define APPLIB_FRM_HDLR_MAX_CB  (16)
#define APPLIB_FRM_HDLR_DEF_PRIORITY  (136)
#define APPLIB_FRM_HDLR_DEF_TASK_STACK_SIZE  (32<<10)

typedef enum APPLIB_FRM_HDLR_YUV_CNT_e_ {
    APPLIB_FRM_HDLR_2ND_YUV = 0,            /* 2nd Stream */
    APPLIB_FRM_HDLR_DCHAN_YUV,          /* Dchan */

    APPLIB_FRM_HDLR_YUV_NUM             /* Number of YUV for va */
} APPLIB_FRM_HDLR_YUV_CNT_e;

typedef struct APPLIB_YUV_TASK_CFG_t_ {
    UINT8 TaskPriority;
    UINT32 TaskStackSize;
    void* TaskStack;
} APPLIB_YUV_TASK_CFG_t;

typedef int (*APPLIB_VA_FRMHDLR_CB)(UINT32, AMP_ENC_YUV_INFO_s*);

extern int AppLibVideoAnal_FrmHdlr_GetFrmInfo(UINT8 yuvSrc, AMP_ENC_YUV_INFO_s* yuvInfo, int* frmSizeIsChanged);

extern int AppLibVideoAnal_FrmHdlr_IsInit(void);

extern int AppLibVideoAnal_FrmHdlr_GetDefCfg(APPLIB_YUV_TASK_CFG_t* cfg);

extern int AppLibVideoAnal_FrmHdlr_Init(void);

extern int AppLibVideoAnal_FrmHdlr_Register(UINT8 yuvSrc, APPLIB_VA_FRMHDLR_CB func);

extern int AppLibVideoAnal_FrmHdlr_UnRegister(UINT8 yuvSrc, APPLIB_VA_FRMHDLR_CB func);

extern int AppLibVideoAnal_FrmHdlr_NewFrame(UINT32 event, AMP_ENC_YUV_INFO_s* yuvInfo);




#endif /* APPLIB_VIDEO_ANAL_FRMHDLR_H_ */
