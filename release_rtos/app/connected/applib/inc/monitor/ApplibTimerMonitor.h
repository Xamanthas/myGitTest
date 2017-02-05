/**
 * @file src/app/connected/applib/inc/monitor/ApplibTimerMonitor.h
 *
 * Header of Timer Monitor Utility interface.
 *
 * History:
 *    2014/05/26 - [Chester Chuang] created file
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
* @defgroup ApplibMonitor_Timer
* @brief Timer Monitor Utility interface.
*
*
*/

/**
 * @addtogroup ApplibMonitor_Timer
 * @ingroup Monitor
 * @{
 */

__BEGIN_C_PROTO__


/**
 *  Timer based monitor related prototype
 */
typedef struct _APPLIB_TIMER_BASED_MONITOR_HANDLER_s_ {

    void (*MonitorInit)(void);    /**< Invoke when enable handler              */
    void (*TimeUpCallBack)(void); /**< Invoke every time when time is up       */
    UINT32 Period;                /**< Period to trigger TimeUpCallCallBack()  */

} APPLIB_TIMER_BASED_MONITOR_HANDLER_s;

/**
 *  @brief Register a timer based handler
 *
 *  Register a timer based handler.
 *
 *  @param [in] hdlr monitor handler
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibTimerBasedMonitor_RegisterHandler(APPLIB_TIMER_BASED_MONITOR_HANDLER_s *hdlr);

/**
 *  @brief Unregister a timer based handler
 *
 *  Unregister a timer based handler.
 *
 *  @param [in] id handler id
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibTimerBasedMonitor_UnregisterHandler(UINT32 id);

/**
 *  @brief Enable/Disable a timer based handler
 *
 *  Enable/Disable a timer based handler.
 *
 *  @param [in] id handler id
 *  @param [in] enable enable/disable
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibTimerBasedMonitor_EnableHandler(UINT32 id, UINT32 enable);

/**
 *  @brief Release Timer based Monitor
 *
 *  Release Timer based Monitor.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibTimerBasedMonitor_Release(void);

/**
 *  @brief Set handler period based on id
 *
 *  Set handler period based on id.
 *
 *  @param [in] id handler id
 *  @param [in] period handler period
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibTimerBasedMonitor_SetHandlerPeriod(UINT32 id, UINT32 period);

/**
 *  @brief Get handler period based on id
 *
 *  Get handler period based on id.
 *
 *  @param [in] id handler id
 *
 *  @return >=0 period, <0 failure
 */
extern int AppLibTimerBasedMonitor_GetHandlerPeriod(UINT32 id);


/**
 *  @brief Initialize Timer based Monitor
 *
 *  Initialize the Timer based Monitor
 *
 *  @param [in] taskPriority priority of the collection task
 *  @param [in] pStack stack for the task
 *  @param [in] stackSize size of the stack
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibTimerBasedMonitor_Init(UINT32 taskPriority, void *pStack, UINT32 stackSize);


__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_TIMER_MONITOR_H_ */
