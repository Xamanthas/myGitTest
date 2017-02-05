/**
 * @file src/app/connected/applib/inc/graphics/font/ApplibGraphics_FTFont.h
 *
 * ApplibGraphics_FTFont include Amba BMP font related
 *
 * History:
 *    2014/04/07 - [Hsunying Hunag] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _APPLIB_GRAPHICS_FTFONT_H_
#define _APPLIB_GRAPHICS_FTFONT_H_

/**
* @defgroup ApplibGraphics_FTFont ApplibGraphics_FTFont
* @brief Freetype font define for graphics functions
*
* This is detailed description of freetype font
*/
/**
 * @addtogroup ApplibGraphics_FTFont
 * @ingroup ApplibGraphics_String
 * @{
 */

#include <AmbaKAL.h>
#include <AmbaGLib.h>
#include "AmbaROM.h"
#include <common/common.h>
#include <graphics/font/ApplibGraphics_Font.h>

/*************************************************************************
 * Freetype font Enum
 ************************************************************************/

/*************************************************************************
 * Freetype font Structure
 ************************************************************************/

/*************************************************************************
 * Freetype font Functions
 ************************************************************************/
/**
 *  @brief Get Font functions
 *
 *  To Get Font functions
 *
 *  @param [in] pFunc the all functions of BMP font
 *
 *  @return AMP_OK
 */
extern int AppLibFTFont_GetFunc(APPLIB_FONT_FUNCTION_s *pFunc);

#endif /* _APPLIB_GRAPHICS_FTFONT_H_ */

/**
 * @}
 */


