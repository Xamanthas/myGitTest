/**
 * \va\inc\ambava_adas_LLWS.h
 *
 * Header of Implementation of Low Light Warning System(LLWS).
 *
 * History:
 *    2014/11/24 - [Bill Chou] created file
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_VIDEO_ANAL_LLWS_H_
#define APPLIB_VIDEO_ANAL_LLWS_H_

/**
* @defgroup ApplibVideoAnal_LLWS
* @brief LLWS related function
*
*
*/

/**
 * @addtogroup ApplibVideoAnal_LLWS
 * @ingroup LLWS
 * @{
 */

#include <applib.h>
#include "AmbaDSP_Event.h"
#include <va/ambava_adas_LLWS.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Video definitions
 ************************************************************************/

/*! applib va LLWS outout event
*applib va LLWS config
*/
typedef enum {
    APPLIB_LLWS_CROSS = 0,
    APPLIB_LLWS_MOVE,
    APPLIB_LLWS_STOP,
    APPLIB_LLWS_EVENT_NUM
} APPLIB_ADAS_LLWS_EVENT_e;
/*!
*applib va LLWS config
*/
typedef struct APPLIB_LLWS_CFG_t_ {
    AMBA_ADAS_SENSITIVITY_LEVEL_e LLWSSensitivity;
} APPLIB_LLWS_CFG_t;

typedef struct APPLIB_LLWS_PAR_t_ {
    float HoodLevel; /**< scene_param*/
    float HorizonLevel; /**< scene_param*/
    int IsUpdate;
} APPLIB_LLWS_PAR_t;
/*************************************************************************
 * LLWS Setting APIs
 ************************************************************************/

/**
 *  Initial LLWS
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibVideoAnal_LLWS_Init(APPLIB_LLWS_CFG_t config, APPLIB_LLWS_PAR_t params);
extern int AppLibVideoAnal_LLWS_DeInit(void);

extern void AppLibVideoAnal_LLWS_GetDef_Setting( APPLIB_LLWS_CFG_t* pConfig, APPLIB_LLWS_PAR_t* pParams);
extern int AppLibVideoAnal_LLWS_SetCfg( APPLIB_LLWS_CFG_t* pConfig);
extern int AppLibVideoAnal_LLWS_SetPar( APPLIB_LLWS_PAR_t* pParams);

extern int AppLibVideoAnal_LLWS_Enable(void);
extern int AppLibVideoAnal_LLWS_Disable(void);
__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_VIDEO_ANAL_LLWS_H_ */
