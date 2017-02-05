/**
 * @file src/app/connected/applib/inc/format/ApplibFormat_DemuxExif.h
 *
 * Header of exif demux
 *
 * History:
 *    2014/08/25 - [phcheng] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_DEMUX_EXIF_H_
#define APPLIB_DEMUX_EXIF_H_
/**
* @defgroup ApplibFormat_DemuxExif
* @brief exif demux
*
*
*/

/**
 * @addtogroup ApplibFormat_DemuxExif
 * @ingroup Format
 * @{
 */
__BEGIN_C_PROTO__

/**
 *   Initialization of exif demuxer.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxExif_Init(void);

/**
 *  Feed image into raw buffer
 *
 *  @param [in] codecHdlr       Pointer to the still decode handler.
 *  @param [in] Fn              File name. Full path of an image.
 *  @param [in] ImageSource     The decode source of file. 0: fullview  1: thumbnail  2: screennail.
 *  @param [in] RawBuf          Address of raw buffer.
 *  @param [in] SizeRawBuf      Size of raw buffer.
 *  @param [in] MPOImage        MPO Image.
 *  @param [in] MPOIdx          MPO Index.
 *  @param [out] ImageWidth     Width of the image.
 *  @param [out] ImageHeight    Height of the image.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxExif_Feed(void* codecHdlr,
        char* Fn,
        UINT8 ImageSource,
        void* RawBuf,
        UINT32 SizeRawBuf,
        UINT8 MPOImage,
        UINT8 MPOIdx,
        UINT32 *ImageWidth,
        UINT32 *ImageHeight);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_DEMUX_EXIF_H_ */
