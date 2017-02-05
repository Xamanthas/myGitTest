/**
 * @file src/app/connected/applib/inc/recorder/ApplibRecorder_Message.h
 *
 * Header of recorder's message.
 *
 * History:
 *    2013/09/14 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_ENCODER_MSG_H_
#define APPLIB_ENCODER_MSG_H_
/**
* @defgroup ApplibRecorder_Message
* @brief Message define for recorder functions
*
*
*/

/**
 * @addtogroup ApplibRecorder_Message
 * @ingroup Recorder
 * @{
 */
#include <applibhmi.h>

__BEGIN_C_PROTO__

/**********************************************************************/
/* MDL_APPLIB_RECORDER_ID messsages                                        */
/**********************************************************************/
/**
* Partition: |31 - 27|26 - 24|23 - 16|15 -  8| 7 -  0|
*   |31 - 27|: MDL_APPLIB_RECORDER_ID
*   |26 - 24|: MSG_TYPE_HMI
*   |23 - 16|: module or interface type ID
*   |15 -  8|: Self-defined
*   | 7 -  0|: Self-defined
* Note:
*   bit 0-15 could be defined in the module itself (individual
*   header files). However, module ID should be defined here for arrangement
**/
#define HMSG_RECORDER_MODULE(x)  MSG_ID(MDL_APPLIB_RECORDER_ID, MSG_TYPE_HMI, (x))
/** Sub-group:type of interface events */
#define HMSG_RECORDER_MODULE_ID_COMMON  (0x01)
#define HMSG_RECORDER_MODULE_ID_VIDEO   (0x02)
#define HMSG_RECORDER_MODULE_ID_STILL   (0x03)
#define HMSG_RECORDER_MODULE_ID_ERROR   (0x04)


/** The recorder status.*/
#define HMSG_RECORDER_MODULE_COMMON(x)  HMSG_RECORDER_MODULE(((UINT32)HMSG_RECORDER_MODULE_ID_COMMON << 16) | (x))
#define HMSG_RECORDER_STATE_IDLE                HMSG_RECORDER_MODULE_COMMON(0x0000)
#define HMSG_RECORDER_STATE_LIVEVIEW            HMSG_RECORDER_MODULE_COMMON(0x0001)
#define HMSG_RECORDER_STATE_ILLEGAL_SIGNAL      HMSG_RECORDER_MODULE_COMMON(0x0002)

/** The recorder status about video recording.*/
#define HMSG_RECORDER_MODULE_VIDEO(x)   HMSG_RECORDER_MODULE(((UINT32)HMSG_RECORDER_MODULE_ID_VIDEO << 16) | (x))
#define HMSG_RECORDER_STATE_RECORDING           HMSG_RECORDER_MODULE_VIDEO(0x0001)
#define HMSG_RECORDER_STATE_RECORDING_PAUSE     HMSG_RECORDER_MODULE_VIDEO(0x0002)
#define HMSG_RECORDER_STATE_PRERECORD           HMSG_RECORDER_MODULE_VIDEO(0x0003)


/** The recorder status about still capture.*/
#define HMSG_RECORDER_MODULE_STILL(x)   HMSG_RECORDER_MODULE(((UINT32)HMSG_RECORDER_MODULE_ID_STILL << 16) | (x))
#define HMSG_RECORDER_STATE_FFCOUS              HMSG_RECORDER_MODULE_STILL(0x0001)
#define HMSG_RECORDER_STATE_FLOCK               HMSG_RECORDER_MODULE_STILL(0x0002)
#define HMSG_RECORDER_STATE_PREFLASH            HMSG_RECORDER_MODULE_STILL(0x0003)
#define HMSG_RECORDER_STATE_CAPTURE             HMSG_RECORDER_MODULE_STILL(0x0004)
#define HMSG_RECORDER_STATE_STILL_RAWCAP        HMSG_RECORDER_MODULE_STILL(0x0005)
#define HMSG_RECORDER_STATE_STILL_RAWENC        HMSG_RECORDER_MODULE_STILL(0x0006)
#define HMSG_RECORDER_STATE_RAW_PROC            HMSG_RECORDER_MODULE_STILL(0x0007)
#define HMSG_RECORDER_STATE_YUV_PROC            HMSG_RECORDER_MODULE_STILL(0x0008)
#define HMSG_RECORDER_STATE_PIV_COMPLETE        HMSG_RECORDER_MODULE_STILL(0x0009)
#define HMSG_RECORDER_STATE_PIV_YUV_DATA_READY  HMSG_RECORDER_MODULE_STILL(0x000A)
#define HMSG_RECORDER_STATE_PHOTO_EXPOSURE_START        HMSG_RECORDER_MODULE_STILL(0x000B)
#define HMSG_RECORDER_STATE_PHOTO_EXPOSURE_COMPLETE     HMSG_RECORDER_MODULE_STILL(0x000C)
#define HMSG_RECORDER_STATE_PHOTO_CAPTURE_START         HMSG_RECORDER_MODULE_STILL(0x000D)
#define HMSG_RECORDER_STATE_PHOTO_CAPTURE_COMPLETE      HMSG_RECORDER_MODULE_STILL(0x000E)
#define HMSG_RECORDER_STATE_PHOTO_BGPROC_COMPLETE       HMSG_RECORDER_MODULE_STILL(0x000F)
#define HMSG_RECORDER_STATE_PHOTO_JPEG_DATA_READY       HMSG_RECORDER_MODULE_STILL(0x0010)
#define HMSG_RECORDER_STATE_PHOTO_QUICKVIEW_START       HMSG_RECORDER_MODULE_STILL(0x0011)
#define HMSG_RECORDER_STATE_PHOTO_QUICKVIEW_COMPLETE    HMSG_RECORDER_MODULE_STILL(0x0012)
#define HMSG_RECORDER_STATE_PHOTO_QUICKVIEW_ZOOM_START  HMSG_RECORDER_MODULE_STILL(0x0013)

#define HMSG_RECORDER_MODULE_ERROR(x)   HMSG_RECORDER_MODULE(((UINT32)HMSG_RECORDER_MODULE_ID_ERROR << 16) | (x))
/** The recorder status about memory.*/
#define HMSG_MEMORY_FIFO_BUFFER_RUNOUT          HMSG_RECORDER_MODULE_ERROR(0x0001)
#define HMSG_MEMORY_DSP_WORKING_MEMORY_RUNOUT   HMSG_RECORDER_MODULE_ERROR(0x0002)

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_ENCODER_MSG_H_ */

