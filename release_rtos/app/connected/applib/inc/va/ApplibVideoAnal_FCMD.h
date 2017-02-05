/**
 * @file src/app/connected/applib/inc/va/ApplibVideoAnal_FCMD.h
 *
 * Header of VA Frontal Car Moving Depature(FCMD) APIs
 *
 * History:
 *    2015/01/06 - [Bill Chou] created file
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_VIDEO_ANAL_FCMD_H_
#define APPLIB_VIDEO_ANAL_FCMD_H_
/**
* @defgroup ApplibVideoAnal_FCMD
* @brief FCMD related function
*
*
*/

/**
 * @addtogroup ApplibVideoAnal_FCMD
 * @ingroup FCMD
 * @{
 */
#include <applib.h>
#include <recorder/Encode.h>
#include <va/ambava_adas_FCMD.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Video definitions
 ************************************************************************/

/*! applib va FCMD outout event
*applib va FCMD config
*/
typedef enum {
    APPLIB_FCMD_CROSS = 0,
    APPLIB_FCMD_MOVE,
    APPLIB_FCMD_STOP,
    APPLIB_FCMD_EVENT_NUM
} APPLIB_ADAS_FCMD_EVENT_e;
/*!
*applib va FCMD config
*/
typedef struct APPLIB_FCMD_CFG_t_ {
    AMBA_ADAS_SENSITIVITY_LEVEL_e FCMDSensitivity;
} APPLIB_FCMD_CFG_t;

typedef struct APPLIB_FCMD_PAR_t_ {
    float HoodLevel; /**< scene_param*/
    float HorizonLevel; /**< scene_param*/
    int IsUpdate;
} APPLIB_FCMD_PAR_t;
/*************************************************************************
 * FCMD Setting APIs
 ************************************************************************/

/**
 *  Initial FCMD
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibVideoAnal_FCMD_Init(UINT8 yuvSrc, APPLIB_FCMD_CFG_t pConfig, APPLIB_FCMD_PAR_t pParams);
extern int AppLibVideoAnal_FCMD_DeInit(void);
extern int AppLibVideoAnal_FCMD_GetDef_Setting( APPLIB_FCMD_CFG_t* pAppLibFCMDConfig, APPLIB_FCMD_PAR_t* pAppLibFCMDParams);
extern int AppLibVideoAnal_FCMD_SetCfg(APPLIB_FCMD_CFG_t* pConfig);
extern int AppLibVideoAnal_FCMD_SetPar( APPLIB_FCMD_PAR_t* pParams);

extern int AppLibVideoAnal_FCMD_Enable(void);
extern int AppLibVideoAnal_FCMD_Disable(void);
__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_VIDEO_ANAL_FCMD_H_ */

