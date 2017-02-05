/**
 * @file va/comsvc/ambava_3ahdlr.c
 *
 * Amba VA frame 3A info handler
 *
 * History:
 *    2014/12/15 - [Bill Chou] created file
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
#include "ambava_3ahdlr.h"
#include <recorder/VideoEnc.h>


#define TRIA_TASK_NAMEE          "VA 3A adata hdlr"
#define TRIA_MSGQUEUE_SIZE (4)


typedef struct _TRIA_MGRMESSAGE_t_ {
    UINT32 MessageID;
    UINT32 MessageData[2];
    AMBA_DSP_EVENT_CFA_3A_DATA_s* pData3A;
} TRIA_MGRMESSAGE_t;

typedef struct _TRIA_CB_SLOT_t_ {
    VA_TRIAHDLR_CB Func;
    UINT32 Event;
} TRIA_CB_SLOT_t;

typedef struct _G_TRIA_t_ {
    UINT8 Init;
    AMBA_KAL_TASK_t Task;
    AMBA_KAL_MUTEX_t Mutex;
    AMBA_KAL_MSG_QUEUE_t MsgQueue;
    UINT8 MsgPool[sizeof(TRIA_MGRMESSAGE_t) * TRIA_MSGQUEUE_SIZE];
    TRIA_CB_SLOT_t CbFunc[TRIA_HDLR_MAX_CB];
} G_TRIA_t;

static G_TRIA_t G_TRIA = { 0 };
AMBA_KAL_MUTEX_t MutexTriA;

#define TRIA_LOC AmbaKAL_MutexTake(&G_TRIA.Mutex, AMBA_KAL_WAIT_FOREVER);
#define TRIA_UNL AmbaKAL_MutexGive(&G_TRIA.Mutex);

void VA_TriA_Maintask(UINT32 info)
{
    TRIA_MGRMESSAGE_t Msg;
    int Rval;
    int T;
    while (1) {
        Rval = AmbaKAL_MsgQueueReceive(&G_TRIA.MsgQueue, (void *) &Msg, AMBA_KAL_WAIT_FOREVER );
        if (Rval == OK) {
            //new frame coming in
            TRIA_LOC
            ;
            for (T = 0; T < TRIA_HDLR_MAX_CB; T++) {
                if (G_TRIA.CbFunc[T].Func != NULL && G_TRIA.CbFunc[T].Event == Msg.MessageID) {
                    G_TRIA.CbFunc[T].Func(Msg.MessageID, Msg.pData3A);
                }
            }
            TRIA_UNL
            ;
        } else {
            AmbaKAL_TaskSleep(1);
        }
    }
}

int AmbaVA_TriAHdlr_GetDefCfg(AMBA_TRIA_CFG* cfg)
{
    cfg->TaskPriority = TRIA_HDLR_DEF_PRIORITY;
    cfg->TaskStackSize = TRIA_HDLR_DEF_TASK_STACK_SIZE;
    return OK;
}

int AmbaVA_TriAHdlr_Init(AMBA_TRIA_CFG* cfg)
{
    int Rval;
    if (G_TRIA.Init != 0) {
        return NG;
    }
    AmbaPrint("VA 3A Handler init...... ");
    // create mqueue
    Rval = AmbaKAL_MsgQueueCreate(&G_TRIA.MsgQueue, G_TRIA.MsgPool, sizeof(TRIA_MGRMESSAGE_t), TRIA_MSGQUEUE_SIZE);
    if (Rval == OK) {
        AmbaPrintColor(GREEN, "[VA 3A INIT] Create Queue success");
    } else {
        AmbaPrintColor(RED, "[VA 3A INIT] Create Queue fail = %d", Rval);
        return NG;
    }



    // create mutex
    Rval = AmbaKAL_MutexCreate(&G_TRIA.Mutex);
    if (Rval == OK) {
        AmbaPrintColor(GREEN, "[VA 3A INIT] Create Mutex success");
    } else {
        AmbaPrintColor(RED, "[VA 3A INIT] Create Mutex fail = %d", Rval);
        return NG;
    }


    AmbaPrintColor(GREEN, "[VA 3A INIT] TaskPriority %d , TaskStackSize %d", cfg->TaskPriority, cfg->TaskStackSize);
    // create task
    Rval = AmbaKAL_TaskCreate(&G_TRIA.Task,
                              TRIA_TASK_NAMEE,
                              cfg->TaskPriority,
                              VA_TriA_Maintask,
                              0x0,
                              cfg->TaskStack,
                              cfg->TaskStackSize,
                              AMBA_KAL_AUTO_START);
    if (Rval == OK) {
        AmbaPrintColor(GREEN, "[VA 3A INIT] Create Task success");
    } else {
        AmbaPrintColor(RED, "[VA 3A INIT] Create Task fail = %d", Rval);
        return NG;
    }
    /// register AmbaDSP Event Handler
    Rval = AmbaDSP_RegisterEventHandler(AMBA_DSP_EVENT_CFA_3A_DATA_READY, (AMBA_DSP_EVENT_HANDLER_f)AmbaVA_DSP_EventHdlr_NewAE);
    if (Rval != OK){
        AmbaPrint("Register Event Handler fail");
    }

    G_TRIA.Init = 1;
    AmbaPrint("VA 3A init done");

    return Rval;
}

int AmbaVA_TriAHdlr_Register(UINT32 event,
                            VA_TRIAHDLR_CB func)
{
    int T;
    if (G_TRIA.Init == 0) {
        AmbaPrint("3Ahdlr is not inited !!");
        return NG;
    }
    TRIA_LOC
    ;
    for (T = 0; T < TRIA_HDLR_MAX_CB; T++) {
        if (G_TRIA.CbFunc[T].Func == NULL) {
            G_TRIA.CbFunc[T].Func = func;
            G_TRIA.CbFunc[T].Event = event;
            break;
        }
    }
    TRIA_UNL
    ;
    if (T == TRIA_HDLR_MAX_CB) {
        return NG;
    }
    return OK;
}

int AmbaVA_TriAHdlr_UnRegister(UINT32 event,
                              VA_TRIAHDLR_CB func)
{
    int T;
    if (G_TRIA.Init == 0) {
        AmbaPrint("3Ahdlr is not inited !!");
        return NG;
    }
    TRIA_LOC
    ;
    for (T = 0; T < TRIA_HDLR_MAX_CB; T++) {
        if (G_TRIA.CbFunc[T].Func == func && G_TRIA.CbFunc[T].Event == event) {
            G_TRIA.CbFunc[T].Func = NULL;
            G_TRIA.CbFunc[T].Event = 0;
        }
    }
    TRIA_UNL
    ;
    return OK;
}

int AmbaVA_TriAHdlr_NewAE(UINT32 event,
                            AMBA_DSP_EVENT_CFA_3A_DATA_s* pData3A)
{
    int Rval = 0;
    TRIA_MGRMESSAGE_t TempMessage = { 0 };

    TempMessage.MessageID = event;
    TempMessage.pData3A = pData3A;
    Rval = AmbaKAL_MutexTake(&G_TRIA.Mutex, AMBA_KAL_NO_WAIT);
    if (Rval == OK) {
        // VA 3A task is idle or none registering cb, send msg to trigger proc
        Rval = AmbaKAL_MsgQueueSend(&G_TRIA.MsgQueue, &TempMessage, AMBA_KAL_NO_WAIT);
        AmbaKAL_MutexGive(&G_TRIA.Mutex);
    }

    return Rval;
}

int AmbaVA_DSP_EventHdlr_NewAE(AMBA_DSP_EVENT_CFA_3A_DATA_s* pData3A)
{
   int Rval = 0;
    TRIA_MGRMESSAGE_t TempMessage = { 0 };

    TempMessage.MessageID = AMBA_DSP_EVENT_CFA_3A_DATA_READY;
    TempMessage.pData3A = pData3A;

    Rval = AmbaKAL_MutexTake(&G_TRIA.Mutex, AMBA_KAL_NO_WAIT);
    if (Rval == OK) {
        // VA 3A task is idle or none registering cb, send msg to trigger proc
        Rval = AmbaKAL_MsgQueueSend(&G_TRIA.MsgQueue, &TempMessage, AMBA_KAL_NO_WAIT);
        AmbaKAL_MutexGive(&G_TRIA.Mutex);
    }

    return Rval;
}

