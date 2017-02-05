/**
 * @file src/app/connected/applib/inc/calibration/wb/ApplibCalibWB.h
 *
 * header for white balance calibration
 *
 * History:
 *    07/10/2013  Allen Chiu Created
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <calibration/ApplibCalibMgr.h>

/**
* @defgroup ApplibCalibWb
* @brief header file for white balance calibration
*
*/

/**
 * @addtogroup ApplibCalibWb
 * @{
 */


/**
*  Register calibration site
*
*  @return 0 - OK
*/
extern int AppLib_CalibSiteInit(void);

/**
*  Get vignette calibration enable flag
*
*  @return enable flag
*/
extern UINT8  AppLibCalibVignette_GetVignetteCalDataEnable(void);

