/**
 * @file src/app/connected/applib/inc/graphics/render/ApplibGraphics_Render.h
 *
 * ApplibGraphics_Render include Amba Render module related
 *
 * History:
 *    2014/01/15 - [Eric Yen] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef _APPLIB_GRAPHIC_RENDER_H_
#define _APPLIB_GRAPHIC_RENDER_H_

/**
* @defgroup ApplibGraphics_Render ApplibGraphics_Render
* @brief Render define for graphics functions
*
* This is detailed description of object
*/
/**
 * @addtogroup ApplibGraphics_Render
 * @ingroup GraphicsUtility
 * @{
 */

/*************************************************************************
 * Render Enums
 ************************************************************************/

/*************************************************************************
 * Render Structures
 ************************************************************************/
/**
 * Render Format
 */
typedef struct _APPLIB_GRAPHIC_RENDER_s_ {
    void* Buf;                              /**< buffer to plot on              */
    UINT32* RowStartAddr;                   /**< cache to store start address of each row(size = sizeof(void*)*bufHeight), NULL to disable (but slow) */
    void* Cursor;                           /**< buffer to plot on              */
    UINT32 BufPitch;                        /**< buffer pitch in bytes          */
    UINT32 BufWidth;                        /**< buffer width                   */
    UINT32 BufHeight;                       /**< buffer height                  */
    UINT8 BufPixelSize;                     /**< buffer pixel size              */
    void (*MoveTo_f)     ( const struct _APPLIB_GRAPHIC_RENDER_s_ *render,
                           void **dst,
                           UINT32 x,
                           UINT32 y );      /**< move cursor to (x,y)           */
    void (*MoveNext_f)   ( void** pen );    /**< move cursor to next pixel      */
    void (*MoveNextFew_f)( void** pen,
                           UINT32 l);       /**< move cursor to next few pixel in the same line     */
    void (*GetPixel_f)   ( void *dst,
                           UINT32 *color ); /**< get a color of pixel at pen    */
    void (*PlotPixel_f)  ( void *dst,
                           UINT32 color );  /**< plot pixel at cursor, cursor move to next pixel (wont change line)     */
    void (*PlotHLine_f)  ( void *dst,
                           UINT32 w,
                           UINT32 color);   /**< plot line from cursor.x to x+w-1, cursor move to end of line pixel (wont change line)      */
    void (*PlotPixelAt_f)( const struct _APPLIB_GRAPHIC_RENDER_s_ *render,
                           UINT32 x,
                           UINT32 y,
                           UINT32 color);   /**< plot pixel at cursor, cursor move to next pixel (wont change line)     */
    void (*PlotHLineAt_f)(const struct _APPLIB_GRAPHIC_RENDER_s_ *render,
                            UINT32 x,
                            UINT32 y,
                            UINT32 w,
                            UINT32 color);  /**< plot line from cursor.x to x+w-1, cursor move to end of line pixel (wont change line)      <*/
} APPLIB_GRAPHIC_RENDER_s;

#endif /* _APPLIB_RENDER_H_ */

/**
 * @}
 */

