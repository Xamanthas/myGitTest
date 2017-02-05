/**
 * @file src/app/connected/applib/inc/player/decode_utility/ApplibPlayer_VideoTask.h
 *
 * A task handling some callbacks of video player.
 *
 * History:
 *    2014/08/26 - [phcheng] Create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/**
 * @defgroup ApplibPlayer_VideoTask         ApplibPlayer_VideoTask
 * @brief A task handling some callbacks of video player.
 */

/**
 * @addtogroup ApplibPlayer_VideoTask
 * @ingroup DecodeUtility
 * @{
 */

#ifndef _APPLIB_PLYR_VIDEO_TASK_H_
#define _APPLIB_PLYR_VIDEO_TASK_H_

#include <msgqueue.h>

/**
 * The type of Vout message in "Video Vout Task"
 */
typedef enum _APPLIB_VIDEO_TASK_MSG_e_ {
    APPLIB_VIDEO_TASK_MSG_EOS = 0,      ///< The PTS of currently displayed frame has reached EOS PTS
    APPLIB_VIDEO_TASK_MSG_FEED_END,     ///< Demux has finished feeding a file
    APPLIB_VIDEO_TASK_MSG_NUMBER        ///< Total number of video Vout message type
} APPLIB_VIDEO_TASK_MSG_e;

/**
 * Message as an input to "Video Vout Task"
 */
typedef struct _APPLIB_VIDEO_TASK_MSG_s_ {
    /**
     * The type of message.
     */
    APPLIB_VIDEO_TASK_MSG_e MessageType;
    /**
     * Video decode handler.
     */
    AMP_AVDEC_HDLR_s *AvcDecHdlr;
} APPLIB_VIDEO_TASK_MSG_s;

/**
 * Resource for still decode task and still display task.
 */
typedef struct _APPLIB_VIDEO_TASK_RESOURCE_s_{
    /**
     * Whether the task is initialized.  \n
     * 0: Not initialized, 1: Initialized
     */
    UINT8 IsInit;
    /**
     * Video task.
     */
    AMBA_KAL_TASK_t VideoTask;
    /**
     * Message queue for all events.
     */
    AMP_MSG_QUEUE_HDLR_s VideoMsgQueue;
    /**
     * Task message pool.
     */
    APPLIB_VIDEO_TASK_MSG_s *VideoMsgPool;
} APPLIB_VIDEO_TASK_RESOURCE_s;

/**
 * Whether the video task is initialized.
 *
 * @return 0 - Not initialized, 1 - Initialized
 */
extern int AppLibVideoTask_IsTaskInitialized(void);

/**
 * Deinitialize video task.
 *
 * @return 0 - Success, Others - Failure
 */
extern int AppLibVideoTask_DeinitTask(void);

/**
 * Initialize video task.
 *
 * @return 0 - Success, Others - Failure
 */
extern int AppLibVideoTask_InitTask(void);

/**
 * Send a message to video task.
 *
 * @param [in] VideoMsg         Message to video task
 * @param [in] Timeout          The limit of execution time (in ms) for this function. Set a value of 0xFFFFFFFF for endless waiting.
 *
 * @return 0 - OK, -1 - Task is not initialized, Others - AMP_MSG_QUEUE_RESULT_e
 * @see AMP_MSG_QUEUE_RESULT_e
 */
extern int AppLibVideoTask_SendVideoMsg(const APPLIB_VIDEO_TASK_MSG_s *VideoMsg, const UINT32 Timeout);

#endif /* _APPLIB_PLYR_VIDEO_TASK_H_ */

/**
 * @}
 */     // End of group ApplibPlayer_VideoTask
