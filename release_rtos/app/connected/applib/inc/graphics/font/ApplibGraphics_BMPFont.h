/**
 * @file src/app/connected/applib/inc/graphics/font/ApplibGraphics_BMPFont.h
 *
 * ApplibGraphics_BMPFont include Amba BMP font related
 *
 * History:
 *    2014/02/07 - [Eric Yen] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _APPLIB_GRAPHICS_BMPFONT_H_
#define _APPLIB_GRAPHICS_BMPFONT_H_

/**
* @defgroup ApplibGraphics_BMPFont ApplibGraphics_BMPFont
* @brief BMP fone define for graphics functions
*
* This is detailed description of BMP font
*/
/**
 * @addtogroup ApplibGraphics_BMPFont
 * @ingroup ApplibGraphics_String
 * @{
 */

#include <mw.h>
#include <graphics/font/ApplibGraphics_Font.h>

/*************************************************************************
 * BMP Font Enums
 ************************************************************************/

/*************************************************************************
 * BMP Font Structures
 ************************************************************************/
/**
 *  The definition of BMP font header in BMP font BIN.
 */
#pragma pack(4)
typedef struct _APPLIB_GRAPHIC_BMPFONT_BIN_HEADER_s_ {
    unsigned short  Magic;              /**< Magic number                                   */
    unsigned short  Version;            /**< Version                                        */
    unsigned short  HeaderSize;         /**< sizeof (APPLIB_GRAPHIC_BMPFONT_BIN_HEADER_s)   */
    unsigned short  SizeOfFont_s;       /**< sizeof (APPLIB_GRAPHIC_BMPFONT_BIN_FONT_s)     */
    char FontName[120];                 /**< Font logical name                              */
    unsigned short  Pages;              /**< number of pages (>= 1)                         */
    unsigned short  SizeOfLang_s;       /**< sizeof (APPLIB_GRAPHIC_BMPFONT_BIN_LANG_s)     */
    unsigned short  LangNum;            /**< number of languages (>= 1)                     */
    unsigned short  Reversed[6];        /**< future use                                     */
} APPLIB_GRAPHIC_BMPFONT_BIN_HEADER_s;
#pragma pack()

/**
 *  The definition of BMP font language in BMP font BIN.
 *  structure of each language MUST sync with GUI gen
 */
#pragma pack(4)
typedef struct _APPLIB_GRAPHIC_BMPFONT_BIN_LANG_s_ {
    UINT16 Height;                      /**< total height of character                      */
    UINT16 Start;                       /**< start page of the language                     */
    UINT16 End;                         /**< end page of the language                       */
    UINT16 First;                       /**< first unicode of the language                  */
    UINT16 Last;                        /**< last of the language                           */
    char Name[24];                      /**< name of the language                           */
    UINT16 Flags;                       /**< reversed for loader's flags                    */
    UINT16 Reversed[6];                 /**< reversed for future use                        */
} APPLIB_GRAPHIC_BMPFONT_BIN_LANG_s;
#pragma pack()

/**
 *  The definition of BMP font BIN.
 *  don't modify this structure MUST sync with GUI gen
 */
#pragma pack(4)
typedef struct _APPLIB_GRAPHIC_BMPFONT_BIN_FONT_s_ {
    UINT8   Type;                                   /**< bit-flags defined below                */
#define APPLIB_GRAPHIC_BMPFONT_PFT_VARIABLE 0x01    /**< Variable-Width font (1-bpp, standard)  */
#define APPLIB_GRAPHIC_BMPFONT_PFT_OUTLINE  0x02    /**< Outline font (2-bpp)                   */
#define APPLIB_GRAPHIC_BMPFONT_PFT_ALIASED  0x04    /**< Anti-aliased (4-bpp) font              */
    UINT8   Ascent;                                 /**< Ascent above baseline                  */
    UINT8   Descent;                                /**< Descent below baseline                 */
    UINT8   Height;                                 /**< total height of character              */
    UINT16  LineBytes;                              /**< total bytes (width) of one scanline    */
    UINT16  FirstChar;                              /**< first character present in font (page) */
    UINT16  LastChar;                               /**< last character present in font (page)  */
    UINT16  *Offset;                                /**< bit-offsets for variable-width font    */
    void    *Next;                                  /**< NULL unless multi-page Unicode font    */
    UINT8   *Data;                                  /**< character bitmap data array            */
} APPLIB_GRAPHIC_BMPFONT_BIN_FONT_s;
#pragma pack()

/**
 *  The definition of BMP font BIN info.
 */
typedef struct _APPLIB_GRAPHIC_BMPFONT_BIN_INFO_s_{
    APPLIB_GRAPHIC_BMPFONT_BIN_FONT_s *DescPage;    /**< descriptor to font pages               */
    APPLIB_GRAPHIC_BMPFONT_BIN_LANG_s *DescLang;    /**< descriptor to language/height          */
    UINT8 **DescPageRaw;                            /**< ptr                                    */
}APPLIB_GRAPHIC_BMPFONT_BIN_INFO_s;

/*************************************************************************
 * BMP Font Defination
 ************************************************************************/
/* Font Header MUST sync with GUI gen*/
#define APPLIB_GRAPHIC_BMPFONT_MAGIC      0x1234     /**<APPLIB_GRAPHIC_BMPFONT_MAGIC      0x1234*/
#define APPLIB_GRAPHIC_BMPFONT_VERSION    0x0100     /**<APPLIB_GRAPHIC_BMPFONT_VERSION    0x0100*/

/*************************************************************************
 * BMP Font APIs
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
extern int AppLibBMPFont_GetFunc(APPLIB_FONT_FUNCTION_s *pFunc);

#endif /* _APPLIB_GRAPHICS_BMPFONT_H_ */

/**
 * @}
 */

