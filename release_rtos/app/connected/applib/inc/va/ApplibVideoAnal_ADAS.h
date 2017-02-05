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
#ifndef APPLIB_VIDEO_ANAL_ADAS_H_
#define APPLIB_VIDEO_ANAL_ADAS_H_
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
#include <va/ambava_adas.h>
__BEGIN_C_PROTO__

/*************************************************************************
 * Video definitions
 ************************************************************************/

/*! applib va ADAS outout event
*applib va ADAS config
*/

/*!
*applib va ADAS config
*/
typedef struct APPLIB_LDWS_CFG_t_ {
    AMBA_ADAS_SENSITIVITY_LEVEL_e LDWSSensitivity;
} APPLIB_LDWS_CFG_t;

typedef struct APPLIB_FCWS_CFG_t_ {
    AMBA_ADAS_SENSITIVITY_LEVEL_e FCWSSensitivity;
} APPLIB_FCWS_CFG_t;

typedef struct APPLIB_ADAS_PAR_t_ {
    float HoodLevel; /**< scene_param*/
    float HorizonLevel; /**< scene_param*/
} APPLIB_ADAS_PAR_t;

typedef struct _APPLIB_ADAS_VIEWANGLE_t_ {
    float HorizAngle;
    float VertAngle;
} APPLIB_ADAS_VIEWANGLE_t;
/*************************************************************************
 * FCMD Setting APIs
 ************************************************************************/

/**
 *  Initial ADAS
 *
 *  @return >=0 success, <0 failure
 */
extern void AppLibVideoAnal_ADAS_GetDef_Setting(APPLIB_LDWS_CFG_t* pLdwsConfig, APPLIB_FCWS_CFG_t* pFcwsConfig, APPLIB_ADAS_PAR_t* pParams);
extern int AppLibVideoAnal_ADAS_Init(UINT8 YuvSrc, APPLIB_LDWS_CFG_t ldwsConfig, APPLIB_FCWS_CFG_t fcwsConfig, APPLIB_ADAS_PAR_t params);
extern int AppLibVideoAnal_ADAS_Process(UINT32 event, AMP_ENC_YUV_INFO_s* img);
extern int AppLibVideoAnal_ADAS_LDWS_SetCfg( APPLIB_LDWS_CFG_t* pParams);
extern int AppLibVideoAnal_ADAS_FCWS_SetCfg( APPLIB_FCWS_CFG_t* pParams);
extern int AppLibVideoAnal_ADAS_SetPar( APPLIB_ADAS_PAR_t* pParams);
extern int AppLibVideoAnal_ADAS_Enable(void);
extern int AppLibVideoAnal_ADAS_Disable(void);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_VIDEO_ANAL_FCMD_H_ */

