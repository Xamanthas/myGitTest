/**
 * @file src/app/connected/applib/inc/graphics/font/ApplibGraphics_Font.h
 *
 * ApplibGraphics_Font include Amba font related
 *
 * History:
 *    2014/04/07 - [Hsunying Huang] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _APPLIB_GRAPHICS_FONT_H_
#define _APPLIB_GRAPHICS_FONT_H_

/**
* @defgroup ApplibGraphics_Font ApplibGraphics_Font
* @brief Font define for graphics functions
*
* This is detailed description of Font
*/
/**
 * @addtogroup ApplibGraphics_Font
 * @ingroup ApplibGraphics_String
 * @{
 */
#include <AmbaDataType.h>
#include <common/common.h>
#include <graphics/render/ApplibGraphics_Render.h>
/*************************************************************************
 * Font enum
 ************************************************************************/
/**
 *  The type of font.
 */
typedef enum _APPLIB_FONT_TYPE_e_ {
    TYPE_BMP = 0,          /**< BMP font               */
    TYPE_CUSTOMIZED,       /**< Customized font        */
    TYPE_NUM,              /**< total font number      */
} APPLIB_FONT_TYPE_e;

/*************************************************************************
 * Font struct
 ************************************************************************/
/**
 * applib graphic font draw config
 */
typedef struct _APPLIB_FONT_DRAW_CONFIG_s_ {
    UINT32 h;                   /**< height                 */
    UINT16 *str;                /**< string                 */
    UINT32 colorFore;           /**< Fore color             */
    UINT32 colorBack;           /**< Back color             */
    AMP_AREA_s updateArea;      /**< updated area           */
    UINT8 drawingUVPair;        /**< drawing UV Pair        */
} APPLIB_FONT_DRAW_CONFIG_s;

/**
 * Initial struct for graphics
 */
typedef struct _APPLIB_FONT_FUNCTION_s_ {
    int (*Init_f)(void);                                        /**< Init Font                              */
    UINT32 (*GetFontSize_f)(const char* FontFn);                /**< Get Font size                          */
    int (*Load_f)(UINT8 *FontBuffer);                           /**< Load Font data to FontBuffer           */
    int (*Draw_f)( const APPLIB_GRAPHIC_RENDER_s *render,
                   APPLIB_FONT_DRAW_CONFIG_s drawConfig);       /**< Draw function                          */
    UINT32 (*GetStrWidth_f)(UINT32 StrHeight, UINT16* Str);     /**< Get String Width function              */
    UINT32 (*GetStrHeight_f)(UINT32 StrHeight, UINT16 *Str);    /**< Get String Height function             */
} APPLIB_FONT_FUNCTION_s;

/**
 * Initial struct for graphics
 */
typedef struct _APPLIB_FONT_s_ {
    const char* FileName;                                       /**< the binary filename of font             */
    UINT32 Size;                                                /**< Total size of the font module                      */
    void* Buffer;                                               /**< Buffer address to a static buffer to load font     */
    APPLIB_FONT_TYPE_e Type;                                    /**< the type of font                        */
    APPLIB_FONT_FUNCTION_s Func;
} APPLIB_FONT_s;

/*************************************************************************
 * Font APIs
 ************************************************************************/
/**
 *  @brief Set font type
 *
 *  To Set font type
 *
 *  @param [in] Type the specific font type
 *
 *  @return AMP_OK
 */
extern int AppLibFont_SetType(APPLIB_FONT_TYPE_e Type);

/**
 *  @brief Get Font functions
 *
 *  To Get Font functions
 *
 *  @param [in] pFontFunc the all functions of the specific font
 *
 *  @return AMP_OK
 */
extern int AppLibFont_GetFunc(APPLIB_FONT_FUNCTION_s *pFontFunc);

#endif /* _APPLIB_GRAPHICS_FONT_H_ */

/**
 * @}
 */
