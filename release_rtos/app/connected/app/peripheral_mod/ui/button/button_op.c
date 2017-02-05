/**
  * @file src/app/peripheral_mod/ui/button/button_op.c
  *
  * Implementation of Button Operation - APP level
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
#include "button_op.h"
#include <wchar.h>
#include <framework/apphmi.h>
#include <framework/appmaintask.h>

//#define DEBUG_DEMOLIB_BUTTON_OP
#if defined(DEBUG_DEMOLIB_BUTTON_OP)
#define DBGMSG AmbaPrint
#define DBGMSGc(x) AmbaPrintColor(GREEN,x)
#define DBGMSGc2 AmbaPrintColor
#else
#define DBGMSG(...)
#define DBGMSGc(...)
#define DBGMSGc2(...)
#endif

/*************************************************************************
 * Button OP definitons
 ************************************************************************/
#define BUTTON_MGR_STACK_SIZE    (0x3800)
#define BUTTON_MGR_NAME          "App_Button_Operation_Manager"
#define BUTTON_MGR_MSGQUEUE_SIZE (16)

typedef struct _APP_BUTTON_MGR_MESSAGE_s_ {
    UINT32 MessageID;
    UINT32 MessageData[2];
} APP_BUTTON_MGR_MESSAGE_s;

typedef struct _BUTTON_MGR_s_ {
    UINT8 Stack[BUTTON_MGR_STACK_SIZE];  /**< Stack */
    UINT8 MsgPool[sizeof(APP_BUTTON_MGR_MESSAGE_s)*BUTTON_MGR_MSGQUEUE_SIZE];   /**< Message memory pool. */
    AMBA_KAL_TASK_t Task;               /**< Task ID */
    AMBA_KAL_MSG_QUEUE_t MsgQueue;      /**< Message queue ID */
} BUTTON_MGR_s;

/** Global instance of Button OP manager */
static BUTTON_MGR_s G_buttonmgr = {0};
#define BUTTON_MSG(bid, offset)        (((offset) == 0) ? HMSG_KEY2_BUTTON(bid) : HMSG_KEY2_BUTTON_RELEASE(bid))

/**
 *  @brief Send the message
 *
 *  Send the message
 *
 *  @param [in] msg Meassage
 *  @param [in] param1 Parameter 1
 *  @param [in] param2 Parameter 2
 *
 *  @return >=0 success, <0 failure
 */
static int ButtonOp_SndMsg(UINT32 msg, UINT32 param1, UINT32 param2)
{
    int ReturnValue = 0;
    APP_BUTTON_MGR_MESSAGE_s t_msg = {0};

    t_msg.MessageID = msg;
    t_msg.MessageData[0] = param1;
    t_msg.MessageData[1] = param2;

    ReturnValue = AmbaKAL_MsgQueueSend(&G_buttonmgr.MsgQueue, &t_msg, AMBA_KAL_NO_WAIT);
    //AmbaPrint("SndMsg.MessageID = 0x%x ReturnValue = %d", msg->MessageID, ReturnValue);

    return ReturnValue;
}

/**
 *  @brief Receive the message
 *
 *  Receive the message
 *
 *  @param [in] msg Message
 *  @param [in] waitOption Wait option
 *
 *  @return >=0 success, <0 failure
 */
static int ButtonOp_RcvMsg(APP_BUTTON_MGR_MESSAGE_s *msg, UINT32 waitOption)
{
    int ReturnValue = 0;

    ReturnValue = AmbaKAL_MsgQueueReceive(&G_buttonmgr.MsgQueue, (void *)msg, waitOption);
    //AmbaPrint("RcvMsg.MessageID = 0x%x ReturnValue = %d", msg->MessageID, ReturnValue);

    return ReturnValue;
}


/**
 *  @brief Button manager task
 *
 *  Button manager task
 *
 *  @param [in] info information
 *
 *  @return >=0 success, <0 failure
 */
static void ButtonOp_MgrTask(UINT32 info)
{
    UINT32 param1 = 0, param2 = 0;
    APP_BUTTON_MGR_MESSAGE_s Msg = {0};
    AmbaPrint("[Button Handler] Button manager ready");

    while (1) {
        ButtonOp_RcvMsg(&Msg, AMBA_KAL_WAIT_FOREVER);
        param1 = Msg.MessageData[0];
        param2 = Msg.MessageData[1];
        AmbaPrint("[Button Handler] Received msg: 0x%X (param1 = 0x%X / param2 = 0x%X)", Msg.MessageID, param1, param2);
        switch (Msg.MessageID) {
        default:
            AppLibComSvcHcmgr_SendMsgNoWait(BUTTON_MSG(param1, Msg.MessageID), 0, 0);
            break;
        }
    }

    DBGMSG("[Button] msg 0x%X is done (ReturnValue = %d / retInfo = %d)", Msg.MessageID, ReturnValue, retInfo);
}


/**
 *  @brief Initialize the button operation
 *
 *  Initialize the button operation
 *
 *  @return >=0 success, <0 failure
 */
int AppButtonOp_Init(void)
{
    int ReturnValue = 0;

    DBGMSG("[DemoLib - Button] <AppButtonOp_init> start");

    /* Clear G_buttonmgr */
    memset(&G_buttonmgr, 0, sizeof(BUTTON_MGR_s));

    /* Create App message queue */
    ReturnValue = AmbaKAL_MsgQueueCreate(&G_buttonmgr.MsgQueue, G_buttonmgr.MsgPool, sizeof(APP_BUTTON_MGR_MESSAGE_s), BUTTON_MGR_MSGQUEUE_SIZE);
    if (ReturnValue == OK) {
        DBGMSGc2(GREEN, "[DemoLib - Button]Create Queue success = %d", ReturnValue);
    } else {
        AmbaPrintColor(RED, "[DemoLib - Button]Create Queue fail = %d", ReturnValue);
    }
    /* Create Host Control Manager task*/
    ReturnValue = AmbaKAL_TaskCreate(&G_buttonmgr.Task, /* pTask */
        BUTTON_MGR_NAME, /* pTaskName */
        APP_BUTTON_OP_PRIORITY, /* Priority */
        ButtonOp_MgrTask, /* void (*EntryFunction)(UINT32) */
        0x0, /* EntryArg */
        (void *) G_buttonmgr.Stack, /* pStackBase */
        BUTTON_MGR_STACK_SIZE, /* StackByteSize */
        AMBA_KAL_AUTO_START); /* AutoStart */
    if (ReturnValue != OK) {
        AmbaPrintColor(RED, "[DemoLib - Button]Create task fail = %d", ReturnValue);
    }

    DBGMSG("[DemoLib - Button] <AppButtonOp_init> end: ReturnValue = %d", ReturnValue);

    return ReturnValue;
}


/**
 *  @brief Update button status
 *
 *  Update button status
 *
 *  @param [in] buttonId Button id
 *  @param [in] event Event
 *
 *  @return >=0 success, <0 failure
 */
int AppButtonOp_UpdateStatus(UINT32 buttonId, UINT32 event)
{
    ButtonOp_SndMsg(event, buttonId, 0);
    return 0;
}


