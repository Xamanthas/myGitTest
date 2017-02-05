/**
 * @file src/app/connected/applib/inc/format/ApplibFormat_DemuxMp4.h
 *
 * Header of mp4 demux
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

#ifndef APPLIB_DEMUX_MP4_H_
#define APPLIB_DEMUX_MP4_H_
/**
* @defgroup ApplibFormat_DemuxMp4
* @brief mp4 demux
*
*
*/

/**
 * @addtogroup ApplibFormat_DemuxMp4
 * @ingroup Format
 * @{
 */
__BEGIN_C_PROTO__

/**
 *  Initialization of mp4 demuxer.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_Init(void);

/**
 *  Set information of codec handler.
 *
 *  @param [in] vidCodecHdlr    Pointer to the video or still decode handler
 *  @param [in] audCodecHdlr    Pointer to the audio decode handler
 *  @param [in] vidRawBuf       Address of video raw buffer.
 *  @param [in] szVidRawBuf     Size of video raw buffer.
 *  @param [in] audRawBuf       Address of audio raw buffer.
 *  @param [in] szAudRawBuf     Size of audio raw buffer.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_SetCodecHdlrInfo(void* vidCodecHdlr,
                                                 void* audCodecHdlr,
                                                 void* vidRawBuf,
                                                 UINT32 szVidRawBuf,
                                                 void* audRawBuf,
                                                 UINT32 szAudRawBuf);

/**
 *  Set information of codec handler.
 *
 *  @param [out] width          Width of the video
 *  @param [out] height         Height of the video
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_GetMovieSize(UINT32 *width,
                                             UINT32 *height);

/**
 *  Open the mp4 demuxer.
 *
 *  @param [in] filename        File name
 *  @param [in] startTime       Start time
 *  @param [in] direction       Play direction
 *  @param [in] speed           Play speed
 *  @param [in] isErase         Whether erase the data processed before and reset read/write pointer of raw buffer
 *  @param [in] timeOffset      Time offset (in ms) for each frame
 *  @param [in] isFeedEos       Feed EOS to DSP at the end of the file
 *  @param [in] cbEventHdlr     Callback event handler
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_Open(char* filename,
                                     UINT32 startTime,
                                     UINT32 speed,
                                     UINT8 isErase,
                                     UINT32 timeOffset,
                                     UINT8 isFeedEos,
                                     AMP_CALLBACK_f cbEventHdlr);

/**
 *  Open and start the mp4 demuxer.
 *
 *  @param [in] filename        File name
 *  @param [in] startTime       Start time
 *  @param [in] direction       Play direction
 *  @param [in] speed           Play speed
 *  @param [in] isErase         Whether erase the data processed before and reset read/write pointer of raw buffer
 *  @param [in] timeOffset      Time offset (in ms) for each frame
 *  @param [in] isFeedEos       Feed EOS to DSP at the end of the file
 *  @param [in] cbEventHdlr     Callback event handler
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_OpenStart(char* filename,
                                          UINT32 startTime,
                                          UINT8 direction,
                                          UINT32 speed,
                                          UINT8 isErase,
                                          UINT32 timeOffset,
                                          UINT8 isFeedEos,
                                          AMP_CALLBACK_f cbEventHdlr);

/**
 *  Start the mp4 demuxer.
 *
 *  @param [in] startTime       Start time
 *  @param [in] direction       Play direction
 *  @param [in] speed           Play speed
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_Start(UINT32 startTime,
                                      UINT8 direction,
                                      UINT32 speed);

/**
 *  Stop the demuxer.
 *
 *  @param [in] isEraseFifo     Whether erase the data in video fifo and reset read/write pointer
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_Stop(void);

/**
 *  Close the mp4 demuxer without erasing fifo data.
 *  For still decode only.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_StillDec_Close(void);

/**
 *  Close the mp4 demuxer
 *
 *  @param [in] isEraseFifo     Whether erase the data in video fifo and reset read/write pointer
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_Close(const UINT8 isEraseFifo);

/**
 *  Whether the demuxer is able to request data.
 *
 *  @return 0 - Cannot request data, 1 - OK
 */
extern UINT8 AppLibFormatDemuxMp4_CanRequestData(void);

/**
 *  Feed data into raw buffer
 *
 *  @param [in] codecHdlr       Pointer to the video or still decode handler.
 *  @param [in] filename        File name. Full path of a video or image.
 *  @param [in] rawBuf          Address of raw buffer.
 *  @param [in] sizeRawBuf      Size of raw buffer.
 *  @param [out] imageWidth     Width of the image.
 *  @param [out] imageHeight    Height of the image.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_Feed(void* codecHdlr,
                                     char* filename,
                                     void* rawBuf,
                                     UINT32 sizeRawBuf,
                                     UINT32 *imageWidth,
                                     UINT32 *imageHeight);

/**
 *  Request demuxer to feed more data.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_DemuxOnDataRequest(void);

/**
 *  Request demuxer to feed more audio data.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibFormatDemuxMp4_DemuxOnDataRequestAudio(void);


__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_DEMUX_MP4_H_ */
