/**
 * @file src/app/connected/applib/inc/recorder/ApplibRecorder_ExtendEnc.h
 *
 * Header of extend encode Utilities
 *
 * History:
 *    2015/03/10 - [HsunYing Huang] created file
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_EXTEND_ENC_H_
#define APPLIB_EXTEND_ENC_H_

/*************************************************************************
 * Defination
 ************************************************************************/
/**
 * Ext info callback
 */
typedef int (*APPLIB_EXTENC_GETINFO_CB_f)(UINT32 *size, UINT8** ptr);

/*************************************************************************
 * APIs
 ************************************************************************/
/**
 *  @brief Int extend encode module
 *
 *  Int extend encode module
 *
 *  @return 0 success, <1 fail
 */
extern int AppLibExtendEnc_Init(void);

/**
 *  @brief Unint extend encode module
 *
 *  Unint extend encode module
 *
 *  @return success
 */
extern int AppLibExtendEnc_UnInit(void);

/**
 *  @brief Set extend encode module info callback
 *
 *  Set extend encode module info callback
 *
 *  @return success
 */
extern int AppLibExtendEnc_SetInfoCB(APPLIB_EXTENC_GETINFO_CB_f GetInfoCB);

/**
 *  @brief Set extend encode module enable/disable status
 *
 *  Set extend encode module enable/disable status
 *
 *  @return success
 */
extern int AppLibExtendEnc_SetEnableStatus(UINT8 enableExtendEnc);

/**
 *  @brief Get extend encode module enable/disable status
 *
 *  Get extend encode module enable/disable status
 *
 *  @return 0 disable, >1 enable
 */
extern UINT8 AppLibExtendEnc_GetEnableStatus(void);

/**
 *  @brief Set video bits buffer frequency for extend data (unit: ms)
 *
 *  Set video bits buffer frequency for extend data
 *  And the unit is ms
 *
 *  @return success
 */
extern int AppLibExtendEnc_SetFrequency(UINT16 period);

/**
 *  @brief Get video bits buffer information for extend data
 *
 *  Get video bits buffer information for extend data
 *
 *  @return success
 */
extern int AppLibExtendEnc_GetConfigure(APPLIB_VIDEOENC_EXTEND_BITS_BUFFER_SETTING_s *ExtDataSettings);

#endif /* APPLIB_EXTEND_ENC_H_ */

