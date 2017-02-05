/**
 * @file src/app/connected/applib/inc/system/ApplibSys_Gyro.h
 *
 * Header of Gyro interface.
 *
 * History:
 *    2014/02/13 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APP_GYRO_H_
#define APP_GYRO_H_
/**
 * @defgroup System
 * @brief Interface of utility
 *
 *
 * interface of utility
 * Ex: Vin ,Vout,Sensor, Lens
 */

/**
* @defgroup ApplibSys_Gyro
* @brief Gyro interface.
*
*
*/

/**
 * @addtogroup ApplibSys_Gyro
 * @ingroup System
 * @{
 */
#include <mw.h>
#include <applib.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Gyro structure
 ************************************************************************/
/**
 *  This data structure describes the interface of a gyro
 */
typedef struct _APPLIB_GYRO_s_ {
    /** Module ID */
    UINT32 Id;
    /** Module name */
    WCHAR Name[32];
    /** Module init interface */
    int (*Init)(void);
} APPLIB_GYRO_s;

/**
 *  Remove the gyro input device
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysGyro_Remove(void);

/**
 *  Attach the gyro input device and enable the device control.
 *
 *  @param [in] dev Device information
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysGyro_Attach(APPLIB_GYRO_s *dev);

/**
 *  Clean Gyro configuration
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysGyro_PreInit(void);

/**
 *  Initialize the gyro
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysGyro_Init(void);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APP_GYRO_H_ */

