/**
 * @file va/comsvc/ambava_frmhdlr.c
 *
 * Amba VA frame handler
 *
 * History:
 *    2014/12/02 - [Peter Weng] created file
 *
 * Copyright (C) 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <AmbaDataType.h>
#include <AmbaKAL.h>
#include <AmbaPrint.h>
#include <AmbaPrintk.h>
#include "ambava_frmhdlr.h"
#include <recorder/VideoEnc.h>

#define VA_TASK_NAMEE          "VA frame hdlr"
#define VA_MSGQUEUE_SIZE (4)

#ifndef AMP_ENC_EVENT_HACK_SDREAD
#define AMP_ENC_EVENT_HACK_SDREAD (AMP_ENC_EVENT_START_NUM+99)
#endif

typedef struct _FRM_MGRMESSAGE_t_ {
    UINT32 MessageID;
    UINT32 MessageData[2];
    AMP_ENC_YUV_INFO_s YUVInfo;
} FRM_MGRMESSAGE_t;

typedef struct _VA_CB_SLOT_t_ {
    VA_FRMHDLR_CB Func;
    UINT32 Event;
} VA_CB_SLOT_t;

typedef struct _VA_EVENT_CNT_t_ {
    UINT32 Cnt;
    UINT32 Event;
} VA_EVENT_CNT_t;

typedef struct _G_VA_t_ {
    UINT8 Init;
    AMBA_KAL_TASK_t Task;
    AMBA_KAL_MUTEX_t Mutex;
    AMBA_KAL_MSG_QUEUE_t MsgQueue;
    UINT8 MsgPool[sizeof(FRM_MGRMESSAGE_t) * VA_MSGQUEUE_SIZE];
    VA_CB_SLOT_t CbFunc[FRM_HDLR_MAX_CB];
    VA_EVENT_CNT_t FrmCnt[FRM_HDLR_MAX_EVENT];
} G_VA_t;

static G_VA_t G_VA = { 0 };

#define VA_LOC AmbaKAL_MutexTake(&G_VA.Mutex, AMBA_KAL_WAIT_FOREVER);
#define VA_UNL AmbaKAL_MutexGive(&G_VA.Mutex);

void VA_Maintask(UINT32 info)
{
    FRM_MGRMESSAGE_t Msg;
    int Rval;
    int T;
    while (1) {
        Rval = AmbaKAL_MsgQueueReceive(&G_VA.MsgQueue, (void *) &Msg, AMBA_KAL_WAIT_FOREVER );
        if (Rval == OK) {
            //new frame coming in
            VA_LOC
            ;
            for (T = 0; T < FRM_HDLR_MAX_CB; T++) {
                if (G_VA.CbFunc[T].Func != NULL && G_VA.CbFunc[T].Event == Msg.MessageID) {
                    if (Msg.MessageID == AMP_ENC_EVENT_HACK_SDREAD) {
                        G_VA.CbFunc[T].Func( Msg.MessageData[0] , &Msg.YUVInfo);
                    } else {
                        G_VA.CbFunc[T].Func(Msg.MessageID, &Msg.YUVInfo);
                    }
                }
            }
            VA_UNL
            ;
        } else {
            AmbaKAL_TaskSleep(1);
        }
    }
}

int AmbaVA_FrmHdlr_GetDefCfg(AMBA_VA_CFG* cfg)
{
    cfg->TaskPriority = FRM_HDLR_DEF_PRIORITY;
    cfg->TaskStackSize = FRM_HDLR_DEF_TASK_STACK_SIZE;
    return OK;
}

int AmbaVA_FrmHdlr_Init(AMBA_VA_CFG* cfg)
{
    int Rval;
    if (G_VA.Init != 0) {
        return NG;
    }
    // create mqueue
    Rval = AmbaKAL_MsgQueueCreate(&G_VA.MsgQueue, G_VA.MsgPool, sizeof(FRM_MGRMESSAGE_t), VA_MSGQUEUE_SIZE);
    if (Rval == OK) {
        AmbaPrintColor(GREEN, "[VA INIT] Create Queue success");
    } else {
        AmbaPrintColor(RED, "[frmhdlr INIT] Create Queue fail = %d", Rval);
        return NG;
    }
    // create mutex
    Rval = AmbaKAL_MutexCreate(&G_VA.Mutex);
    if (Rval == OK) {
        AmbaPrintColor(GREEN, "[frmhdlr INIT] Create Mutex success");
    } else {
        AmbaPrintColor(RED, "[frmhdlr INIT] Create Mutex fail = %d", Rval);
        return NG;
    }
    AmbaPrintColor(GREEN, "[frmhdlr INIT] TaskPriority %d , TaskStackSize %d", cfg->TaskPriority, cfg->TaskStackSize);
    // create task
    Rval = AmbaKAL_TaskCreate(&G_VA.Task,
                              VA_TASK_NAMEE,
                              cfg->TaskPriority,
                              VA_Maintask,
                              0x0,
                              cfg->TaskStack,
                              cfg->TaskStackSize,
                              AMBA_KAL_AUTO_START);
    if (Rval == OK) {
        AmbaPrintColor(GREEN, "[frmhdlr INIT] Create Task success");
    } else {
        AmbaPrintColor(RED, "[frmhdlr INIT] Create Task fail = %d", Rval);
        return NG;
    }

    G_VA.Init = 1;
    AmbaPrint("yuv frame handler init done");

    return OK;
}

int AmbaVA_FrmHdlr_Register(UINT32 event,
                            VA_FRMHDLR_CB func)
{
    int T;
    if (G_VA.Init == 0) {
        AmbaPrint("frmhdlr is not inited !!");
        return NG;
    }
    VA_LOC
    ;
    for (T = 0; T < FRM_HDLR_MAX_CB; T++) {
        if (G_VA.CbFunc[T].Func == NULL) {
            G_VA.CbFunc[T].Func = func;
            G_VA.CbFunc[T].Event = event;
            break;
        }
    }
    VA_UNL
    ;
    if (T == FRM_HDLR_MAX_CB) {
        return NG;
    }
    return OK;
}

int AmbaVA_FrmHdlr_UnRegister(UINT32 event,
                              VA_FRMHDLR_CB func)
{
    int T;
    if (G_VA.Init == 0) {
        AmbaPrint("frmhdlr is not inited !!");
        return NG;
    }
    VA_LOC
    ;
    for (T = 0; T < FRM_HDLR_MAX_CB; T++) {
        if (G_VA.CbFunc[T].Func == func && G_VA.CbFunc[T].Event == event) {
            G_VA.CbFunc[T].Func = NULL;
            G_VA.CbFunc[T].Event = 0;
        }
    }
    VA_UNL
    ;
    return OK;
}

int AmbaVA_FrmHdlr_NewFrame(UINT32 event,
                            AMP_ENC_YUV_INFO_s* YUVInfo)
{
    int Rval = 0;
    FRM_MGRMESSAGE_t TempMessage = { 0 };
    if (event == AMP_ENC_EVENT_HACK_SDREAD) {
        G_VA.FrmCnt[0].Cnt++;
        G_VA.FrmCnt[0].Event = AMP_ENC_EVENT_HACK_SDREAD;
        TempMessage.MessageData[0] = G_VA.FrmCnt[0].Cnt;
    }
    TempMessage.MessageID = event;
    memcpy(&TempMessage.YUVInfo, YUVInfo, sizeof(AMP_ENC_YUV_INFO_s));

    Rval = AmbaKAL_MutexTake(&G_VA.Mutex, AMBA_KAL_NO_WAIT);
    if (Rval == OK) {
        // VA task is idle or none registering cb, send msg to trigger proc
        Rval = AmbaKAL_MsgQueueSend(&G_VA.MsgQueue, &TempMessage, AMBA_KAL_NO_WAIT);
        AmbaKAL_MutexGive(&G_VA.Mutex);
    }

    return Rval;
}


