/**
 * @file src/app/connected/applib/inc/display/ApplibCsc.h
 *
 * Header of display csc
 *
 * History:
 *    2014/03/27 - [Eric Yen] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_CSC_H_
#define APPLIB_CSC_H_

/**
* @defgroup Csc
* @brief Csc Utilities
*
*/

/**
 * @addtogroup Csc
 * @{
 */

#include <display/Display.h>
#include <applib.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Csc declaration
 ************************************************************************/
/** Csc output quantization rage */
typedef enum _APPLIB_CSC_QUANTIZATION_RANGE_e_ {
    APPLIB_CSC_QUANTIZATION_RANGE_LIMITED = 0,    /**< Limited quantization range of 256 levels (16 to 235) */
    APPLIB_CSC_QUANTIZATION_RANGE_FULL,           /**< Full quantization range of 256 levels (0 to 255) */
} APPLIB_CSC_QUANTIZATION_RANGE_e;

/*************************************************************************
 * Csc APIs
 ************************************************************************/
/**
 *
 *  Get csc matrix
 *
 *  @param [in] VoutType Type of vout device @see AMBA_DSP_VOUT_TYPE_e
 *  @param [in] ColorSpaceIn Input color space @see AMBA_DSP_COLOR_SPACE_e
 *  @param [in] ColorSpaceOut Output color space @see AMBA_DSP_COLOR_SPACE_e
 *  @param [in] QRange Quantization range @see AMP_DISP_QUANTIZATION_RANGE_e
 *  @param [in] IsHDOutput Is output HD resolution, a.k.a. ActiveRowHeight >= 720
 *  @param [out] pOutMatrix Output csc matrix
 *
 *  @return frame rate
 */
extern int AppLibCsc_GetMatrix(AMP_DISP_DEV_IDX_e VoutType,
                               AMBA_DSP_COLOR_SPACE_e ColorSpaceIn,
                               AMBA_DSP_COLOR_SPACE_e ColorSpaceOut,
                               APPLIB_CSC_QUANTIZATION_RANGE_e QRangeIn,
                               APPLIB_CSC_QUANTIZATION_RANGE_e QRangeOut,
                               UINT8 IsHDOutput,
                               AMBA_DSP_VOUT_CSC_CONFIG_s *pOutMatrix);


__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_CSC_H_ */

