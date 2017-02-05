/**
 * @file src/app/connected/applib/inc/transcoder/still_decode/ApplibTranscoder_Thumb_Basic.h
 *
 * The apis provide basic function for retrieving IDR frame
 *
 * History:
 *    2015/03/17 - [cichen] Create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/**
 * @defgroup ApplibTranscoder_Thumb_Basic       ApplibTranscoder_Thumb_Basic
 * @brief to retrieve IDR frame.
 *
 */

/**
 * @addtogroup ApplibTranscoder_Thumb_Basic
 * @ingroup StillDecode
 * @{
 */

#ifndef APPLIB_TRANSCODER_THUMB_BASIC_H_
#define APPLIB_TRANSCODER_THUMB_BASIC_H_

/**
 * Image source type
 */
typedef enum _TRANS_STILL_IMAGE_SOURCE_TYPE_e_{
    TRANS_STILL_IMAGE_SOURCE_FULL,         /**< image source type for full resolution */
    TRANS_STILL_IMAGE_SOURCE_THUMBNAIL,    /**< image source type for thumbnail */
    TRANS_STILL_IMAGE_SOURCE_SCREENNAIL,   /**< image source type for screennail */
    TRANS_STILL_IMAGE_SOURCE_NUM           /**< image source type boundary */
} TRANS_STILL_IMAGE_SOURCE_TYPE_e;

/**
 * Applib Data Buffer Structure Definitions
 */
typedef struct _TRANS_STILL_DATA_BUF_s_ {
    UINT8 *Buf;             /**< buffer address*/
    UINT32 BufSize;         /**< size of buffer */
    UINT32 RetDataSize;    /**< size of data that are actually written into Buf */
} TRANS_STILL_DATA_BUF_s;

/**
 *  @brief Initialize basic thumbnail module
 *
 *  @return 0 success, <0 failure
 */
int AppLibTranscoderThmBasic_Init(void);

/**
 * Un-initialize basic thumbnail module.
 *
 * @return 0 - success, others - failure
 */
int AppLibTranscoderThmBasic_Uninit(void);

/**
 * Get IDR frame from specified file
 *
 * @param [in] Filename    File name
 * @param [in] DataBuf     Information of data buffer
 *
 * @return 0 - OK, success - failure
 */
int AppLibTranscoderThmBasic_GetIdrFrame(char *Filename, TRANS_STILL_DATA_BUF_s *DataBuf);

/**
 * Get image (full image/thumbnail/screennail) from specified file
 *
 * @param [in] Filename File name
 * @param [in] Type         Image source selction
 * @param [in] DataBuf  Information of data buffer
 *
 * @return 0 - OK, success - failure
 */
int AppLibTranscoderThmBasic_GetImage(char *Filename, TRANS_STILL_IMAGE_SOURCE_TYPE_e Type, TRANS_STILL_DATA_BUF_s *DataBuf);

#endif /* APPLIB_TRANSCODER_THUMB_BASIC_H_ */

/**
 * @}
 */     // End of group ApplibTranscoder_Thumb_Basic
