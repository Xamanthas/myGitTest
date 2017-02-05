/**
 * @file src/app/connected/applib/inc/image/ApplibImage.h
 *
 * Header of Image Utility interface.
 *
 * History:
 *    2013/09/03 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_IMAGE_H_
#define APPLIB_IMAGE_H_
/**
 * @defgroup Image
 * @brief Image Utility interface.
 *
 *
 */

/**
 * @addtogroup Image
 * @{
 */
#include <applib.h>
#include <imgschdlr/scheduler.h>

__BEGIN_C_PROTO__


/**
 *  Vin changed prior callback function.
 *
 *  @param [in] hdlr Handler
 *  @param [in] event Event
 *  @param [in] info Information
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_VinChangedPriorCallbackFunc(void *hdlr, UINT32 event, void *info);

/**
 *  Vin changed post callback function.
 *
 *  @param [in] hdlr Handler
 *  @param [in] event Event
 *  @param [in] info Information
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_VinChangedPostCallbackFunc(void *hdlr, UINT32 event, void *info);

/**
 *  Vin invalid callback function.
 *
 *  @param [in] hdlr Handler
 *  @param [in] event Event
 *  @param [in] info Information
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_VinInvalidCallbackFunc(void *hdlr, UINT32 event, void *info);

/**
 *  Vin valid callback function.
 *
 *  @param [in] hdlr Handler
 *  @param [in] event Event
 *  @param [in] info Information
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_VinValidCallbackFunc(void *hdlr, UINT32 event, void *info);

/**
 *  Image CFA handler
 *
 *  @param [in] hdlr Handler
 *  @param [in] event Event
 *  @param [in] info Information
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_CfaHandler(void *hdlr, UINT32 event, void *info);

/**
 *  Image RGB handler
 *
 *  @param [in] hdlr Handler
 *  @param [in] event Event
 *  @param [in] info Information
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_VDspRgbHandler(void *hdlr, UINT32 event, void *info);

/**
 *  @brief Applib image module initialization
 *
 *  Applib image module initialization
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_Init(void);

/**
 *  @brief Create image scheduler
 *
 *  Create image scheduler
 *
 *  @param [in] param Scheduler parameter
 *  @param [in] index Scheduler index
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_CreateImgSchdlr(AMBA_IMG_SCHDLR_CFG_s *param, UINT32 index);

/**
 *  @brief Delete image scheduler
 *
 *  Delete image scheduler
 *  @param [in] index Scheduler index
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_DeleteImgSchdlr(UINT32 index);

/**
 *  Stop image scheduler
 *
 *  @param [in] index Scheduler index
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_StopImgSchdlr(UINT32 index);

/**
 *  Enable image scheduler
 *
 *  @param [in] index Scheduler index
 *  @param [in] enable enable/disable
 *  @return The setting of flash light
 */
extern int AppLibImage_EnableImgSchdlr(UINT32 index, UINT32 enable);

/**
 *  Stop the 3a image.
 *
 *  @param [in] hdlr Handler
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_Stop3A(void *hdlr);

/**
 *  @brief Set the photo mode of 3A.
 *
 *  Set the photo mode of 3A.
 *
 *  @param [in] photoMode mode
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_Set3APhotoMode(UINT32 photoMode);

/**
 *  Lock AE
 *
 *
 *  @return >=0 success, <0 failure
 */
extern UINT32 AppLibImage_Lock3A(void);

/**
 *  Unlock AE
 *
 *
 *  @return >=0 success, <0 failure
 */
extern UINT32 AppLibImage_UnLock3A(void);


/**
 *   Set 3A enabled
 *
 *  @param [in] enable Enable parameter
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_Set3A(int enable);


/**
 *  Enable the anit-flicker
 *
 *  @param [in] enable Enable parameter
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_EnableAntiFlicker(int enable);

/**
 *  Update image scheduler handler
 *
 *  @param [in] param IS configurations
 *  @param [in] index IS handler index
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibImage_UpdateImgSchdlr(AMBA_IMG_SCHDLR_UPDATE_CFG_s *param, UINT32 index);


__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_IMAGE_H_ */
