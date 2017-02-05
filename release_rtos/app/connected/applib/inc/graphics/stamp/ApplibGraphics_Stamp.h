/**
 * @file src/app/connected/applib/inc/graphics/stamp/ApplibGraphics_Stamp.h
 *
 * Header of time stamp
 *
 * History:
 *    2014/04/14 - [Hsunying Huang] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_GRAPHICS_TIME_STAMP_H_
#define APPLIB_GRAPHICS_TIME_STAMP_H_

/**
* @defgroup ApplibGraphics_Stamp ApplibGraphics_Stamp
* @brief Object define for graphics functions
*
* This is detailed description of object
*/
/**
 * @addtogroup ApplibGraphics_Stamp
 * @ingroup GraphicsUtility
 * @{
 */

/*************************************************************************
 * Graphics definitions
 ************************************************************************/
#define GRAPH_PREVIEW_BITMAP_READY    HMSG_MODULE_GRAPH(0x0001) /**<GRAPH_PREVIEW_BITMAP_READY    HMSG_MODULE_GRAPH(0x0001)*/

/*!
 * applib graphic color format enum
 */
typedef enum _APPLIB_GRAPH_COLOR_FORMAT_e_ {
    COLOR_FORMAT_YUV422 = 0,                    /**< YUV422 Encode format           */
    COLOR_FORMAT_YUV420,                        /**< YUV420 Encode format              */
    COLOR_FORMAT_NUM                            /**< Total encdoe format number        */
} APPLIB_GRAPH_COLOR_FORMAT_e;

/**
 *  The definition of graphic blending buffer id.
 */
typedef enum _APPLIB_GRAPHIC_BLEND_BUF_ID_e_ {
    GRAPH_BLEND_BUF_0 = 0,                      /**< Blending bufer[0]                 */
    GRAPH_BLEND_BUF_1,                          /**< Blending bufer[1]                 */
    GRAPH_BLEND_BUF_2,                          /**< Blending bufer[2]                 */
    GRAPH_BLEND_BUF_3,                          /**< Blending bufer[3]                 */
    GRAPH_BLEND_BUF_4,                          /**< Blending bufer[4]                 */
    GRAPH_BLEND_BUF_5,                          /**< Blending bufer[5]                 */
    GRAPH_BLEND_BUF_6,                          /**< Blending bufer[6]                 */
    GRAPH_BLEND_BUF_7,                          /**< Blending bufer[7]                 */
    GRAPH_BLEND_BUF_8,                          /**< Blending bufer[8]                 */
    GRAPH_BLEND_BUF_NUM                         /**< Total blending bufer number       */
} APPLIB_GRAPHIC_BLEND_BUF_ID_e;

/**
 *  The definition of graphic blending buffer channel id.
 */
typedef enum _APPLIB_GRAPHIC_BLEND_CH_ID_e_ {
    GRAPH_BLEND_CH_Y = 0,                       /**< Y blending channel                 */
    GRAPH_BLEND_CH_UV,                          /**< UV blending channel                */
    GRAPH_BLEND_CH_A,                           /**< Alpha blending channel             */
    GRAPH_BLEND_CH_NUM                          /**< Total blending channel number      */
} APPLIB_GRAPHIC_BLEND_CH_ID_e;

/**
 *  The parameter of graphic blending buffer.
 */
typedef struct _APPLIB_GRAPHIC_SOURCE_BUF_INFO_s_ {
    APPLIB_GRAPHIC_RENDER_s *SourceRender;      /**< SourceRender                       */
    AMP_AREA_s SourceDisplayBox;                /**< SourceDisplayBox                   */
} APPLIB_GRAPHIC_SOURCE_BUF_INFO_s;

/**
 *  The parameter of graphic blending buffer.
 */
typedef struct _APPLIB_GRAPHIC_STAMP_BUF_CONFIG_s_ {
    UINT32 OffsetX;                             /**< Offset X                           */
    UINT32 OffsetY;                             /**< Offset Y                           */
    UINT32 Width;                               /**< Width                              */
    UINT32 Height;                              /**< Height                             */
    UINT8 *YAddr;                               /**< YAddr                              */
    UINT8 *UVAddr;                              /**< UVAddr                             */
    UINT8 *AlphaYAddr;                          /**< Alpha Address for Y Channel        */
    UINT8 *AlphaUVAddr;                         /**< Alpha Address for UV Channel       */
} APPLIB_GRAPHIC_STAMP_BUF_CONFIG_s;

/**
 *  @brief Initialize stamp module
 *
 *  Initialize stamp module
 *
 *  @param [in] *blendingBufAddress the specidif blending buffer
 *
 *  @return 0 - OK, others - AMP_ER_CODE_e
 *  @see AMP_ER_CODE_e
 */
extern void AppLibStamp_Init(void *blendingBufAddress);

/**
 *  @brief Set clut table
 *
 *  Set clut table
 *
 *  @param [in] *clutTable the clut table
 *
 *  @return
 *  @see
 */
extern void AppLibBlend_SetClutTable(UINT8 *clutTable);

/**
 *  @brief Add one new buffer for blending
 *
 *  Add one new buffer for blending
 *
 *  @param [in] stampAreaId the specific id for blending buffer
 *  @param [in] *sourceBufInfo the blending buffer
 *  @param [in] colorFormat the color format of source buffer
 *
 *  @return 0 - OK, others - AMP_ER_CODE_e
 *  @see AMP_ER_CODE_e
 */
extern void AppLibBlend_AddBlendArea(UINT8 stampAreaId, APPLIB_GRAPHIC_SOURCE_BUF_INFO_s *sourceBufInfo, APPLIB_GRAPH_COLOR_FORMAT_e colorFormat);

/**
 *  @brief Update one specific buffer for blending
 *
 *  Update one new specific for blending
 *
 *  @param [in] stampAreaId the specific id for blending buffer
 *  @param [in] *sourceBufInfo the blending buffer
 *  @param [in] colorFormat the color format of source buffer
 *
 *  @return 0 - OK, others - AMP_ER_CODE_e
 *  @see AMP_ER_CODE_e
 */
extern void AppLibBlend_UpdateBlendArea(UINT8 stampAreaId, APPLIB_GRAPHIC_SOURCE_BUF_INFO_s *sourceBufInfo, APPLIB_GRAPH_COLOR_FORMAT_e colorFormat);

/**
 *  @brief Get a buffer for blending
 *
 *  Get a buffer for blending
 *
 *  @param [in] blendBufId the specific id for blending buffer
 *  @param [out] *bufInfo the all info of blending buffer
 *
 *  @return 0 - OK, others - AMP_ER_CODE_e
 *  @see AMP_ER_CODE_e
 */
extern void AppLibStamp_GetBlendBuf(UINT32 blendBufId, APPLIB_GRAPHIC_STAMP_BUF_CONFIG_s *bufInfo);
#endif /* APPLIB_GRAPHICS_TIME_STAMP_H_ */

/**
 * @}
 */

