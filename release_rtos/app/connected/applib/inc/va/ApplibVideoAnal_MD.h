/**
 * @file src/app/connected/applib/inc/va/ApplibVideoAnal_MD.h
 *
 * Header of VA Motion Detection (MD) APIs
 *
 * History:
 *    2015/01/12 - [Bill Chou] created file
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_VIDEO_ANAL_MD_H_
#define APPLIB_VIDEO_ANAL_MD_H_
/**
* @defgroup ApplibVideoAnal_MD
* @brief MD related function
*
*
*/

/**
 * @addtogroup ApplibVideoAnal_MD
 * @ingroup MD
 * @{
 */
#include <applib.h>
#include <recorder/Encode.h>
#include <va/ambava_adas_MD_yuv.h>
#include <va/ambava_adas_MD_ae.h>
__BEGIN_C_PROTO__

/*************************************************************************
 * Video definitions
 ************************************************************************/

/*! applib va MD outout event
*applib va MD config
*/
typedef enum {
    APPLIB_MD_MOVE = 0,
    APPLIB_MD_EVENT_NUM
} APPLIB_ADAS_MD_EVENT_e;

typedef enum {
    APPLIB_MD_YUV = 0,
    APPLIB_MD_AE,
    APPLIB_MD_YUV_MSE
} APPLIB_ADAS_MD_METHOD_e;

typedef struct APPLIB_MD_ROI_DATA_t_ {
    AMBA_ADAS_SENSITIVITY_LEVEL_e MDSensitivity;
    AMBA_VA_ROI_s Location;
} APPLIB_MD_ROI_DATA_t;
/*!
*applib va MD config
*/
typedef struct APPLIB_MD_CFG_t_ {
    APPLIB_MD_ROI_DATA_t RoiData[MOTION_DETECT_ROI_MAX];
    APPLIB_ADAS_MD_METHOD_e Method;
} APPLIB_MD_CFG_t;

/*************************************************************************
 * MD Setting APIs
 ************************************************************************/

/**
 *  Initial MD
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibVideoAnal_MD_Init(UINT8 yuvSrc, APPLIB_MD_CFG_t* pConfig);
extern int AppLibVideoAnal_MD_DeInit(void);

extern void AppLibVideoAnal_MD_GetDef_Setting( APPLIB_MD_CFG_t* pConfig);
extern void AppLibVideoAnal_MD_SetROI(APPLIB_MD_CFG_t* pConfig);

extern int AppLibVideoAnal_MD_Enable(void);
extern int AppLibVideoAnal_MD_Disable(void);
__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_VIDEO_ANAL_MD_H_ */

