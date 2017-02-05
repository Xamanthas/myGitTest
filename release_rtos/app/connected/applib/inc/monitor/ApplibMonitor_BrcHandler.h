/**
 * @file src/app/connected/applib/inc/monitor/ApplibMonitor_BrcHandler.h
 *
 * Header of Bitrate control handler interface.
 *
 * History:
 *    2014/07/23 - [Chester Chuang] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_TIMER_MONITOR_H_
#define APPLIB_TIMER_MONITOR_H_
/**
* @defgroup ApplibMonitor_BrcHandler
* @brief Bitrate control handler interface.
*
*
*/

/**
 * @addtogroup ApplibMonitor_BrcHandler
 * @ingroup Monitor
 * @{
 */
#include <applib.h>
__BEGIN_C_PROTO__

/**
 *  Amba bitrate control dzoom handler
 */
typedef struct _BITRATE_CONTROL_DZOOM_HANDLER_s_ {
    void (*HandlerCB)(UINT32 *targetBitRate, UINT32 currBitRate, UINT8 streamId); /**< Handler CB function */
    float DzoomFactorThres; /**< Threshold of Dzoom factor */
    UINT8 DebugPattern;  /**< Debug Pattern */
    UINT8 Reserved[3];  /**< Reserved */
} BITRATE_CONTROL_DZOOM_HANDLER_s;

/**
 *  Amba bitrate control luma handler
 */
typedef struct _BITRATE_CONTROL_LUMA_HANDLER_s_ {
    void (*HandlerCB)(UINT32 *targetBitRate, UINT32 currBitRate, UINT8 streamId); /**< Handler CB function */
    int LumaThres;       /**< Threshold of Luma */
    int LowLumaThres;    /**< Threshold of Low Luma */
    UINT8 DebugPattern; /**< Debug Pattern */
    UINT8 Reserved[3]; /**< Reserved */
} BITRATE_CONTROL_LUMA_HANDLER_s;

/**
 *  Amba bitrate control complexity handler
 */
typedef struct _BITRATE_CONTROL_COMPLEXITY_HANDLER_s_ {
    void (*HandlerCB)(UINT32 *targetBitRate, UINT32 currBitRate, UINT8 streamId);  /**< Handler CB function */
    int (*GetDayLumaThresCB)(int mode, UINT32 *threshold); /**< Get Day Luma Threshold CB */
    int (*GetComplexityRangeCB)(int mode, UINT32 *complexMin, UINT32 *complexMid, UINT32 *complexMax); /**< Get Complexity Range CB function */
    void (*GetPipeModeCB)(UINT8 *isVhdr, UINT8 *isOverSample); /**< Get Pipe Mode CB function */
    UINT8 DebugPattern; /**< Debug Pattern */
    UINT8 Reserved[3]; /**< Reserved */
} BITRATE_CONTROL_COMPLEXITY_HANDLER_s;


__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_TIMER_MONITOR_H_ */
