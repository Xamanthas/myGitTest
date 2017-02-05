/**
 * @file src/app/connected/applib/inc/system/ApplibSys_Lcd.h
 *
 * Header of LCD panel interface.
 *
 * History:
 *    2013/07/17 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_GPS_H_
#define APPLIB_GPS_H_
/**
* @defgroup ApplibSys_Gps
* @brief GPS interface.
*
*
*/

/**
 * @addtogroup ApplibSys_Gps
 * @ingroup System
 * @{
 */
#include <mw.h>
#include <applib.h>
#include <gps_struct.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * GPS definitions
 ************************************************************************/
 
#define MPS_TO_KMPH (3.6)

/*************************************************************************
 * GPS structure
 ************************************************************************/
/**
 *  This data structure describes the interface of a LCD panel
 */
typedef struct _APPLIB_GPS_s_ {
    WCHAR Name[32];/**< Module name */
    /* UINT16 start */
    UINT16 Enable:1; /**<Enable*/
    int (*Init)(void);/**< Module init interface */
    /* Module parameter get interface */
    int (*GetGpsId)(void); /**< Get GPS Mode*/
    gps_data_t* (*GetData)(void); /**< Get GPS data*/
} APPLIB_GPS_s;


/*************************************************************************
 * GPS Internal APIs
 ************************************************************************/
/**
 *  Remove the GPS device
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysGps_Remove(void);

/**
 *  Attach the GPS device and enable the device control.
 *
 *  @param [in] dev Device information
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysGps_Attach(APPLIB_GPS_s *dev);

/*************************************************************************
 * GPS Public APIs
 ************************************************************************/
/**
 *  Clean GPS configuration
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysGps_PreInit(void);

/**
 *  GPS initiation
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysGps_Init(void);

/**
 *  To get the GPS ID
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysGps_GetGpsId(void);

/**
 *  To get the GPS data
 *
 *  @return >=0 The address of gps data, <0 failure
 */
extern gps_data_t* AppLibSysGps_GetData(void);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_LCD_H_ */
