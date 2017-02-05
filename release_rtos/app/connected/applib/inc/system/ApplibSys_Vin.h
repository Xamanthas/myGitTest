/**
 * @file src/app/connected/applib/inc/system/ApplibSys_Vin.h
 *
 *  Header of vin Utility interface.
 *
 * History:
 *    2013/08/14 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_VIN_H_
#define APPLIB_VIN_H_
/**
* @defgroup ApplibSys_Vin
* @brief vin Utility interface.
*
*
*/

/**
 * @addtogroup ApplibSys_Vin
 * @ingroup System
 * @{
 */
#include <applib.h>
#include <vin/vin.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Vin system declaration
 ************************************************************************/
/**
 *
 * applib vin setting
 *
 */
 typedef struct _APPLIB_VIN_SETTING_s_{
    UINT8 Source; /**< Vin source config id */
#define VIN_SRC_SENSOR    (0) /**<VIN_SRC_SENSOR    (0)*/
    UINT8 System; /**< Vin system config id */
#define VIN_SYS_NTSC    (0)      /**<VIN_SYS_NTSC    (0)*/
#define VIN_SYS_PAL        (1)   /**<VIN_SYS_PAL     (1)*/
    UINT8 Dimension; /**< Vin Dimension config id */
#define INPUT_2D_R      (0)  /**<INPUT_2D_R      (0)*/
#define INPUT_2D_L      (1)  /**<INPUT_2D_L      (1)*/
#define INPUT_3D        (2)  /**<INPUT_3D        (2)*/
    UINT8 Reserved; /**< Reserved */
} APPLIB_VIN_SETTING_s;

extern AMP_VIN_HDLR_s *AppVinA; /**<AppVinA handler*/

/*************************************************************************
 * Vin APIs
 ************************************************************************/

/**
 *  Initialize the vin module.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysVin_Init(void);

/**
 *  Configure the vin module
 *
 *  @param [in] vinCfg The vin setting.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysVin_Config(AMP_VIN_CFG_s *vinCfg);

/**
 *  Set the vout system type NTSC/PAL.
 *
 *  @param [in] vinSys system type
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysVin_SetSystemType(int vinSys);

/**
 *  Get the vin system type.
 *
 *  @return The vin system type.
 */
extern int AppLibSysVin_GetSystemType(void);

/**
 *  Set the vin dimension
 *
 *  @param [in] vinDimension Vin dimension
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysVin_SetDimension(int vinDimension);

/**
 *  Get the vin dimension
 *
 *  @return The vin dimension
 */
extern int AppLibSysVin_GetDimension(void);

/**
 *  Get the vin source type.
 *
 *  @param [in] vinSrc The vin source type
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysVin_SetSourceType(int vinSrc);

/**
 *  Get the vin source type.
 *
 *  @return The vin source type.
 */
extern int AppLibSysVin_GetSourceType(void);

/**
 *  Get the total setting of vin.
 *
 *  @param [out] setting The vin setting
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysVin_GetSetting(APPLIB_VIN_SETTING_s *setting);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_VIN_H_ */
