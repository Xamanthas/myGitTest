/**
 * @file src/app/connected/applib/inc/graphics/obj/ApplibGraphics_Obj.h
 *
 * ApplibGraphics_Obj.h include Amba Object related
 *
 * History:
 *    2013/12/06 - [Eric Yen] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef _APPLIB_GRAPHICS_OBJ_H_
#define _APPLIB_GRAPHICS_OBJ_H_

/**
* @defgroup ApplibGraphics_Obj ApplibGraphics_Obj
* @brief Object define for graphics functions
*
* This is detailed description of object
*/
/**
 * @addtogroup ApplibGraphics_Obj
 * @ingroup GraphicsUtility
 * @{
 */

#include <AmbaDataType.h>
#include <AmbaKAL.h>
#include <AmbaPrintk.h>
#include <common/common.h>
#include <graphics/render/ApplibGraphics_Render.h>

/*************************************************************************
 * Obj Enums
 ************************************************************************/
/**
 * Graphic content status enumerate
 */
typedef enum _APPLIB_OBJ_STAT_e_ {
    OBJ_STAT_INVALID,               /**< Graphic Obj is invalid            */
    OBJ_STAT_NORMAL,                /**< Graphic Obj is normal             */
    OBJ_STAT_UPDATING,              /**< Graphic Obj is updating content   */
    OBJ_STAT_UPDATE,                /**< Graphic Obj has been updated      */
    OBJ_STAT_DRAM,                  /**< Graphic Obj is on drawing         */
    OBJ_STAT_DELETE,                /**< Graphic Obj is going to delete    */
    OBJ_STAT_HIDING,                /**< Graphic Obj is going to hide      */
    OBJ_STAT_HIDED                  /**< Graphic Obj has been hide         */
} APPLIB_OBJ_STAT_e;

/*************************************************************************
 * Obj Structures
 ************************************************************************/
 /**
  * APPLIB_GRAPHIC_OBJ_ID_t
  */
 typedef UINT32 APPLIB_GRAPHIC_OBJ_ID_t;

 /**
  * applib graphic object
  */
typedef struct _APPLIB_GRAPHIC_OBJ_s_ {
    APPLIB_GRAPHIC_OBJ_ID_t ID;         /**< object ID                           */
    UINT8 Exist;                        /**< object exist or not                 */
    AMP_AREA_s DisplayBox;              /**< object area in a canvas             */
    AMP_AREA_s LastDisplayBox;          /**< object last area in a canvas        */
    UINT32* AlphaTable;                 /**< object alpha value                  */
    UINT32 Layer;                       /**< canvas layer which object drew on   */
    UINT32 Group;                       /**< object content group                */
    UINT8 Show;                         /**< object shown or not                 */
//  UINT32 matrix[2][2];                /**< Linear transform(rotate/translate) matrix will apply when drawing to a canvas buffer    */
    AMBA_KAL_MUTEX_t Mutex;             /**< Mutex of graphic obj                */
    APPLIB_OBJ_STAT_e Stat;             /**< object status                       */
    void *Content;                      /**< object content descriptor pointer   */
//  UINT32 ContentSize;                 /**< graphic content descriptor size     */
    int (*CalcArea_f)( struct _APPLIB_GRAPHIC_OBJ_s_ *obj);         /**< Get graphic obj area               */
    int (*Dump_f)( struct _APPLIB_GRAPHIC_OBJ_s_ *obj);             /**< Dump a graphic obj content         */
    int (*Draw_f)( APPLIB_GRAPHIC_RENDER_s *render,
                   AMP_AREA_s *drawArea,
                   struct _APPLIB_GRAPHIC_OBJ_s_ *obj);             /**< Draw a graphic obj on canvas       */
//  int (*Update_f)( const APPLIB_RENDER_t *render,                 /** update a graphic obj on canvas      */
//                   APPLIB_GRAPHIC_OBJ_t *obj);
} APPLIB_GRAPHIC_OBJ_s;

#endif /* _APPLIB_GRAPHICS_OBJ_H_ */

/**
 * @}
 */

