/**
 * @file src/app/connected/applib/inc/system/ApplibSys_Vout.h
 *
 *  Header of vout Utility interface.
 *
 * History:
 *    2013/07/10 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_VOUT_H_
#define APPLIB_VOUT_H_
/**
* @defgroup ApplibSys_Vout
* @brief vout Utility interface.
*
*
*/

/**
 * @addtogroup ApplibSys_Vout
 * @ingroup System
 * @{
 */
#include <applib.h>
#include <display/Display.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Vout system declaration
 ************************************************************************/
/**
 * Vout setting structure
 */
typedef struct _APPLIB_VOUT_SETTING_s_ {
    UINT8 SystemType;    /**< Vout system id */
#define VOUT_SYS_NTSC   (0)  /**<VOUT_SYS_NTSC*/
#define VOUT_SYS_PAL    (1)  /**<VOUT_SYS_PAL */
    UINT8 VoutJackHDMI; /**< Vout jack HDMI */
    UINT8 VoutJackCS; /**< Vout Jack CS */
    UINT8 Reserved; /**< Reserved */
} APPLIB_VOUT_SETTING_s;

/** Vout display mode config id */
typedef enum _VOUT_DISP_MODE_ID_e_ {
    VOUT_DISP_MODE_2160P_HALF = 0,
    VOUT_DISP_MODE_2160P24,
    VOUT_DISP_MODE_1080P,
    VOUT_DISP_MODE_1080P_HALF,
    VOUT_DISP_MODE_1080I,
    VOUT_DISP_MODE_1080P24,
    VOUT_DISP_MODE_720P,
    VOUT_DISP_MODE_720P_HALF,
    VOUT_DISP_MODE_720P24,
    VOUT_DISP_MODE_SDP,
    VOUT_DISP_MODE_SDI,
	VOUT_DISP_MODE_DMT0659,
    VOUT_DISP_MODE_NUM
} VOUT_DISP_MODE_ID_e;

/*************************************************************************
 * Vout system APIs
 ************************************************************************/

/**
 *  Set the vout system type.
 *
 *  @param [in] voutSys vout system type.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysVout_SetSystemType(int voutSys);

/**
 *  Get the system type of vout.
 *
 *  @return The system type
 */
extern int AppLibSysVout_GetSystemType(void);

/**
 *  Set the flag of HDMI jack
 *
 *  @param [in] jackState HDMI jack status
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysVout_SetJackHDMI(int jackState);

/**
 *  Get the status of HDMI jack
 *
 *  @return The status of HDMI jack
 */
extern int AppLibSysVout_CheckJackHDMI(void);

/**
 *  Set the status of composite jack
 *
 *  @param [in] jackState composite jack status
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibSysVout_SetJackCs(int jackState);

/**
 *  Get the status of composite jack
 *
 *  @return the status of composite jack
 */
extern int AppLibSysVout_CheckJackCs(void);

/**
 *  Get the vout mode.
 *
 *  @param [in] voutDispModeID Application vout mode ID
 *
 *  @return vout mode.
 */
extern int AppLibSysVout_GetVoutMode(VOUT_DISP_MODE_ID_e voutDispModeID);

/**
 *  Get the HDMI frame rate
 *
 *  @param [in] voutDispMode Vout mode
 *
 *  @return frame rate
 */
extern int AppLibSysVout_GetHDMIFrameRate(AMP_DISP_HDMI_MODE_e voutDispMode);


__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_VOUT_H_ */
