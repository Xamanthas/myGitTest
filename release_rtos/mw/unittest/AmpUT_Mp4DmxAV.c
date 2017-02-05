/**
 *  @file AmpUT_Mp4DmxAV.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2014/01/10 |felix       |Created        |
 *
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <player/Decode.h>
#include <player/VideoDec.h>
#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaCache_Def.h>
#include "DummyVideoDec.h"
#include "DummyAudioDec.h"

#include <format/Demuxer.h>
#include <stream/File.h>
#include <format/Mp4Dmx.h>
#include <util.h>

#define BS_FN   "C:\\OUT_0001.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0001.nhnt" /**< nhnt file name */
#define MP4_FN  "C:\\TEST.MP4" /**< mp4 file name */
#define AUDIO_FN "C:\\AUDIO.raw" /**< audio raw file name */

#define MP4DMX_DEC_RAW_SIZE (10<<20) /**< 10MB raw buffer */
#define INDEX_BUFFER_SIZE  (1<<20) /**< 1MB */
#define MAX_FIFO_ENTRY_NUM  1024 /**< number of entry of FIFO */

static AMP_STREAM_HDLR_s *g_pStream = NULL; /**< stream handler */
static AMP_DMX_FORMAT_HDLR_s *g_pFormat = NULL; /**< format handler */
static AMP_MOVIE_INFO_s *g_pMovie = NULL; /**< movie information */
static AMP_DEMUXER_PIPE_HDLR_s *g_pDmxPipe = NULL; /**< demuxer pipe */

#define MP4DMX_VIDEO_CODEC_HDLR ((void *)0x1234) /**< video codec handler */
#define MP4DMX_AUDIO_CODEC_HDLR ((void *)0x1235) /**< audio codec handler */

static void *g_Mp4DmxVideoBuf = NULL; /**< demux video buffer */
static AMP_FIFO_HDLR_s *g_Mp4DmxVideoDecFifoHdlr = NULL; /**< video decode FIFO handler */
static AMP_FIFO_HDLR_s *g_Mp4DmxVideoFifoHdlr = NULL; /**< video demux FIFO handler */

static void *g_Mp4DmxAudioBuf = NULL; /**< demux audio buffer */
static AMP_FIFO_HDLR_s *g_Mp4DmxAudioDecFifoHdlr = NULL; /**< audio decode FIFO handler */
static AMP_FIFO_HDLR_s *g_Mp4DmxAudioFifoHdlr = NULL; /**< audio demux FIFO handler */

static DummyVideoDec_s g_VideoDec; /**< video decoder */
static DummyAudioDec_s g_AudioDec; /**< audio decoder */

/**
 * MP4 demuxer AV UT - data request function.
 *
 * @param [in] pDecoder decoder handler.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUt_Mp4Dmx_DataRequest(void *pDecoder)
{
    if (pDecoder == &g_VideoDec)
        return AmpDemuxer_OnDataRequest(g_Mp4DmxVideoFifoHdlr);
    if (pDecoder == &g_AudioDec)
        return AmpDemuxer_OnDataRequest(g_Mp4DmxAudioFifoHdlr);
    AmbaPrint("%s : invalid fifo!!", __FUNCTION__);
    return -1;
}

/**
 * MP4 demuxer AV UT - video decode FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_VideoDecFifoCB(void *hdlr, UINT32 event, void* info)
{
    //AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);
    AMP_FIFO_INFO_s TmpInfo = {0};
    AMP_BITS_DESC_s *pTmpDesc = NULL;
    RING_BUFFER_INFO_s RingBufInfo = {0};
    if (event == AMP_FIFO_EVENT_GET_WRITE_POINT) {
        pTmpDesc = (AMP_BITS_DESC_s *) info;
        if (RingBuf_GetInfo(&g_VideoDec.RingBuf, &RingBufInfo) != 0)
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        pTmpDesc->Size = RingBufInfo.nRingBufFreeSize;
        pTmpDesc->StartAddr = RingBufInfo.pRingBufWp;
        if (pTmpDesc->StartAddr == NULL)
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    } else if (event == AMP_FIFO_EVENT_DATA_READY) {
        if (AmpFifo_GetInfo(g_Mp4DmxVideoDecFifoHdlr, &TmpInfo) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        } else {
            if (AmpFifo_PeekEntry(g_Mp4DmxVideoDecFifoHdlr, &pTmpDesc, (TmpInfo.AvailEntries - 1)) != AMP_OK) {
                AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            } else {
                if (pTmpDesc->Size < AMP_FIFO_MARK_EOS) {
                    if (RingBuf_UpdateWp(&g_VideoDec.RingBuf, pTmpDesc->StartAddr, pTmpDesc->Size) != 0)
                        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
                }
            }
        }
    }
    return 0;
}

/**
 * MP4 demuxer AV UT - audio decode FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_AudioDecFifoCB(void *hdlr, UINT32 event, void* info)
{
    //AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);
    AMP_FIFO_INFO_s TmpInfo = {0};
    AMP_BITS_DESC_s *pTmpDesc = NULL;
    RING_BUFFER_INFO_s RingBufInfo = {0};
    if (event == AMP_FIFO_EVENT_GET_WRITE_POINT) {
        pTmpDesc = (AMP_BITS_DESC_s *) info;
        if (RingBuf_GetInfo(&g_AudioDec.RingBuf, &RingBufInfo) != 0)
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        pTmpDesc->Size = RingBufInfo.nRingBufFreeSize;
        pTmpDesc->StartAddr = RingBufInfo.pRingBufWp;
        if (pTmpDesc->StartAddr == NULL)
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    } else if (event == AMP_FIFO_EVENT_DATA_READY) {
        if (AmpFifo_GetInfo(g_Mp4DmxAudioDecFifoHdlr, &TmpInfo) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        } else {
            if (AmpFifo_PeekEntry(g_Mp4DmxAudioDecFifoHdlr, &pTmpDesc, (TmpInfo.AvailEntries - 1)) != AMP_OK) {
                AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            } else {
                if (pTmpDesc->Size < AMP_FIFO_MARK_EOS) {
                    if (RingBuf_UpdateWp(&g_AudioDec.RingBuf, pTmpDesc->StartAddr, pTmpDesc->Size) != 0)
                        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
                }
            }
        }
    }
    return 0;
}

/**
 * MP4 demuxer AV UT - demux FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_DmxFifoCB(void *hdlr, UINT32 event, void* info)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    return 0;
}

/**
 * MP4 demuxer AV UT - demux callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_DmxCB(void *hdlr, UINT32 event, void* info)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    switch (event) {
    case AMP_DEMUXER_EVENT_START:
        AmbaPrint("AMP_DEMUXER_EVENT_START");
        DummyVideoDec_Start(&g_VideoDec);
        DummyAudioDec_Start(&g_AudioDec);
        break;
    case AMP_DEMUXER_EVENT_END:
        AmbaPrint("AMP_DEMUXER_EVENT_END");
        DummyVideoDec_Stop(&g_VideoDec);
        DummyAudioDec_Stop(&g_AudioDec);
        AmbaPrint("[SUCCESS] AmpUT_Mp4DmxAV : End");
        break;
    default:
        AmbaPrint("[FAIL] %s: event = %u", __FUNCTION__, event);
        break;
    }
    return 0;
}

/**
 * MP4 demuxer AV UT - demux initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_DmxInit(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *pFmtBuf = NULL;
    AMP_DEMUXER_INIT_CFG_s demuxerInitCfg;
    void *pDemuxerBuf = NULL;
    AMP_MP4_DMX_INIT_CFG_s Mp4InitCfg;
    void *pMp4Buf = NULL;
    AMP_INDEX_INIT_CFG_s IndexBufCfg;
    void *pIdxBuf = NULL;
    AMP_FILE_STREAM_INIT_CFG_s fileInitCfg;
    void *pFileBuf = NULL;
    void *Mp4DmxVideoRawBuf = NULL;
    void *Mp4DmxAudioRawBuf = NULL;
    UINT32 BufferSize = DummyVideoDec_GetRequiredBufSize(MP4DMX_DEC_RAW_SIZE, MAX_FIFO_ENTRY_NUM);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_Mp4DmxVideoBuf, &Mp4DmxVideoRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoDec_Init(&g_VideoDec, BS_FN, HDR_FN, g_Mp4DmxVideoBuf, BufferSize, MAX_FIFO_ENTRY_NUM, AmpUt_Mp4Dmx_DataRequest);
    BufferSize = DummyAudioDec_GetRequiredBufSize(MP4DMX_DEC_RAW_SIZE, MAX_FIFO_ENTRY_NUM);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_Mp4DmxAudioBuf, &Mp4DmxAudioRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyAudioDec_Init(&g_AudioDec, AUDIO_FN, g_Mp4DmxAudioBuf, BufferSize, MAX_FIFO_ENTRY_NUM, AmpUt_Mp4Dmx_DataRequest);
    /** Initial Format */
    AmpFormat_GetInitDefaultCfg(&fmtInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fmtInitCfg.Buffer, &pFmtBuf, fmtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFormat_Init(&fmtInitCfg);
    /** Initial Demuxer */
    AmpDemuxer_GetInitDefaultCfg(&demuxerInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&demuxerInitCfg.Buffer, &pDemuxerBuf, demuxerInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpDemuxer_Init(&demuxerInitCfg);
    /** Initial Mp4 Demux */
    AmpMp4Dmx_GetInitDefaultCfg(&Mp4InitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&Mp4InitCfg.Buffer, &pMp4Buf, Mp4InitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpMp4Dmx_Init(&Mp4InitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Initial Index */
    AmpIndex_GetInitDefaultCfg(&IndexBufCfg);
    IndexBufCfg.BufferSize = INDEX_BUFFER_SIZE;
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&IndexBufCfg.Buffer, &pIdxBuf, IndexBufCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpIndex_Init(&IndexBufCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Initial File system */
    AmpFileStream_GetInitDefaultCfg(&fileInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fileInitCfg.Buffer, &pFileBuf, fileInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFileStream_Init(&fileInitCfg);

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MP4 demuxer AV UT - demux open function.
 *
 * @param [in] szName file name.
 * @param [in] Speed demux speed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_DmxOpen(char *szName, UINT32 Speed)
{
    AMP_FIFO_CFG_s fifoDefCfg = {0};
    AMP_FILE_STREAM_CFG_s fileCfg;
    AMP_DEMUXER_PIPE_CFG_s dmxCfg;
    AMP_MP4_DMX_CFG_s Mp4Cfg = {0};
    AMP_DMX_MOVIE_INFO_CFG_s MovieCfg;

    /** Create decoder fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MP4DMX_VIDEO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_Mp4DmxAV_VideoDecFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_Mp4DmxVideoDecFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Create demux fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MP4DMX_VIDEO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_Mp4DmxAV_DmxFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_Mp4DmxVideoFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Create decoder fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MP4DMX_AUDIO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_Mp4DmxAV_AudioDecFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_Mp4DmxAudioDecFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Create demux fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MP4DMX_AUDIO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_Mp4DmxAV_DmxFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_Mp4DmxAudioFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Open File system */
    AmpFileStream_GetDefaultCfg(&fileCfg);
    AmpFileStream_Create(&fileCfg, &g_pStream);
    if (g_pStream->Func->Open(g_pStream, szName, AMP_STREAM_MODE_RDONLY) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Open Media info (Mp4dmx) */
    if (AmpFormat_GetMovieInfo(szName, AmpMp4Dmx_Parse, g_pStream, &g_pMovie) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Inifial Demuxer */
    AmpDemuxer_GetDefaultMovieInfoCfg(&MovieCfg, g_pMovie);
    MovieCfg.Track[0].Fifo = g_Mp4DmxVideoFifoHdlr;
    MovieCfg.Track[0].BufferBase = g_Mp4DmxVideoBuf;
    MovieCfg.Track[0].BufferLimit = (UINT8 *)g_Mp4DmxVideoBuf + MP4DMX_DEC_RAW_SIZE;
    MovieCfg.Track[1].Fifo = g_Mp4DmxAudioFifoHdlr;
    MovieCfg.Track[1].BufferBase = g_Mp4DmxAudioBuf;
    MovieCfg.Track[1].BufferLimit = (UINT8 *)g_Mp4DmxAudioBuf + MP4DMX_DEC_RAW_SIZE;
    AmpDemuxer_InitMovieInfo(g_pMovie, &MovieCfg);
    /** Craete Mp4 demux */
    AmpMp4Dmx_GetDefaultCfg(&Mp4Cfg);
    Mp4Cfg.Stream = g_pStream;
    if (AmpMp4Dmx_Create(&Mp4Cfg, &g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Create Demuxer pipe */
    AmpDemuxer_GetDefaultCfg(&dmxCfg);
    dmxCfg.FormatCount = 1;
    dmxCfg.Format[0] = g_pFormat;
    dmxCfg.Media[0] = (AMP_MEDIA_INFO_s *)g_pMovie;
    dmxCfg.OnEvent = AmpUT_Mp4DmxAV_DmxCB;
    dmxCfg.Speed = Speed;
    if (AmpDemuxer_Create(&dmxCfg, &g_pDmxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpDemuxer_Add(g_pDmxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoDec_Reset(&g_VideoDec, g_Mp4DmxVideoDecFifoHdlr);
    DummyAudioDec_Reset(&g_AudioDec, g_Mp4DmxAudioDecFifoHdlr);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MP4 demuxer AV UT - demux close function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_DmxClose(void)
{
    if (g_pDmxPipe != NULL) {
        if (AmpDemuxer_WaitComplete(g_pDmxPipe, AMBA_KAL_WAIT_FOREVER) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
        if (AmpDemuxer_Remove(g_pDmxPipe) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
        if (AmpDemuxer_Delete(g_pDmxPipe) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
        g_pDmxPipe = NULL;
    }

    if (g_pFormat != NULL) {
        AmpMp4Dmx_Delete(g_pFormat);
        g_pFormat = NULL;
    }

    if (g_pStream != NULL) {
        g_pStream->Func->Close(g_pStream);
        AmpFileStream_Delete(g_pStream);
        g_pStream = NULL;
    }

    if (g_pMovie != NULL) {
        AmpFormat_RelMovieInfo(g_pMovie, FALSE);
        g_pMovie = NULL;
    }

    if (g_Mp4DmxVideoFifoHdlr != NULL) {
        AmpFifo_Delete(g_Mp4DmxVideoFifoHdlr);
        g_Mp4DmxVideoFifoHdlr = NULL;
    }

    if (g_Mp4DmxVideoDecFifoHdlr != NULL) {
        AmpFifo_Delete(g_Mp4DmxVideoDecFifoHdlr);
        g_Mp4DmxVideoDecFifoHdlr = NULL;
    }

    if (g_Mp4DmxAudioFifoHdlr != NULL) {
        AmpFifo_Delete(g_Mp4DmxAudioFifoHdlr);
        g_Mp4DmxAudioFifoHdlr = NULL;
    }

    if (g_Mp4DmxAudioDecFifoHdlr != NULL) {
        AmpFifo_Delete(g_Mp4DmxAudioDecFifoHdlr);
        g_Mp4DmxAudioDecFifoHdlr = NULL;
    }

    return 0;
}

/**
 * MP4 demuxer AV UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_init(void)
{
    /** Initial Mp4 demux */
    if (AmpUT_Mp4DmxAV_DmxInit() != 0)
        AmbaPrint("[FAIL] %s", __FUNCTION__);
    return 0;
}

/**
 * MP4 demuxer AV UT - start function.
 *
 * @param [in] StartTime start demux time.
 * @param [in] Direction demux direction.
 * @param [in] Speed demux speed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_start(UINT32 StartTime, UINT8 Direction, UINT32 Speed)
{
    /** Open Mp4 demux */
    int rval;
    UINT32 speed = ((Speed == 0) ? 1 : Speed);
    rval = AmpUT_Mp4DmxAV_DmxOpen(MP4_FN, speed); if (rval != AMP_OK) { goto ERR; }

    if (Direction == 0) {
        AmbaPrint("Mp4 Demux - Forward");
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 1200); if (rval != AMP_OK) { goto ERR; }
        if ((StartTime == 0) && (speed == 1))
            rval = AmpDemuxer_Start(g_pDmxPipe);
        else
            rval = AmpDemuxer_Seek(g_pDmxPipe, StartTime, Direction, speed);
        if (rval != AMP_OK) { goto ERR; }
        AmpDemuxer_OnDataRequest(g_Mp4DmxVideoFifoHdlr);
        AmpDemuxer_OnDataRequest(g_Mp4DmxAudioFifoHdlr);
        AmbaKAL_TaskSleep(100);
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 200); if (rval != AMP_OK) { goto ERR; }
    } else if (Direction == 1) {
        AmbaPrint("Mp4 Demux - Backward");
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 1200); if (rval != AMP_OK) { goto ERR; }
        rval = AmpDemuxer_Seek(g_pDmxPipe, StartTime, Direction, speed); if (rval != AMP_OK) { goto ERR; }
        AmpDemuxer_OnDataRequest(g_Mp4DmxVideoFifoHdlr);
        AmpDemuxer_OnDataRequest(g_Mp4DmxAudioFifoHdlr);
        AmbaKAL_TaskSleep(100);
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 200); if (rval != AMP_OK) { goto ERR; }
    } else {
        AmbaPrint("%s, Error Direction", __FUNCTION__);
    }
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MP4 demuxer AV UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_stop(void)
{
    AmbaPrint("%s", __FUNCTION__);
    AmpDemuxer_Stop(g_pDmxPipe);
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MP4 demuxer AV UT - exit function.
 */
static int AmpUT_Mp4DmxAV_exit(void)
{
    AmbaPrint("%s", __FUNCTION__);
    AmpUT_Mp4DmxAV_DmxClose();
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MP4 demuxer AV UT - usage describe function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_Usage(void)
{
    AmbaPrint("Mp4 Demux Test Command");
    AmbaPrint("----------------------");
    AmbaPrint("1. init - start demux");
    AmbaPrint("2. start - start demux");
    AmbaPrint("3. stop - stop demux");
    AmbaPrint("4. exit - exit demux");
    return 0;
}

/**
 * MP4 demuxer AV UT - start usage describe function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_StartUsage(void)
{
    AmbaPrint("Mp4 Demux Test Command - start");
    AmbaPrint("------------------------------");
    AmbaPrint("start $time $direction $speed");
    AmbaPrint("");
    AmbaPrint("$time      - From what time(million second) of clip to demux.");
    AmbaPrint("$direction - 0 : Forward, 1 : Backward.");
    AmbaPrint("$speed     - 1, 2, 4, 8 fold to demux, only effected in forward demux.");
    AmbaPrint("------------------------------");
    AmbaPrint("Ex. start 0 0 0");
    AmbaPrint("Ex. start 5000 1 0");
    return 0;
}

/**
 * MP4 demuxer AV UT - feed function.
 *
 * @param [in] Time time for feed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_feed(UINT32 Time)
{
    AmpUT_Mp4DmxAV_DmxOpen(MP4_FN, 1);
    AmpDemuxer_FeedFrame(g_pFormat, 0, Time, AMP_FIFO_TYPE_IDR_FRAME);
    AmpUT_Mp4DmxAV_DmxClose();
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MP4 demuxer AV UT - seek function.
 *
 * @param [in] nTime time to seek.
 * @param [in] nDirection demux direction.
 * @param [in] nSpeed demux speed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_seek(UINT32 nTime, UINT8 nDirection, UINT32 nSpeed)
{
    AmpDemuxer_Seek(g_pDmxPipe, nTime, nDirection, ((nSpeed == 0) ? 1 : nSpeed));
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MP4 demuxer AV UT - test function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4DmxAV_test(void)
{
    return 0;
}

/**
 * MP4 demuxer AV UT - MP4 demuxer test function.
 */
static int AmpUT_Mp4DmxAVTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int rval = 0;

//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_Mp4DmxAV_init();
    } else if (strcmp(argv[1], "start") == 0) {
        if (argc == 5) {
            UINT32 StartTime;
            UINT8 Direction;
            UINT32 Speed;
            StartTime = atoi(argv[2]);
            Direction = atoi(argv[3]);
            Speed = atoi(argv[4]);
            AmbaPrint("mp4dmxav start %u %u %u", StartTime, Direction, Speed);
            AmpUT_Mp4DmxAV_start(StartTime, Direction, Speed);
        } else if (argc == 2) {
            AmbaPrint("mp4dmxav start");
            AmpUT_Mp4DmxAV_start(0, 0, 1);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            AmpUT_Mp4DmxAV_StartUsage();
            rval = -1;
        }
    } else if (strcmp(argv[1], "feed") == 0) {
        if (argc == 3) {
            UINT32 Time;
            Time = atoi(argv[2]);
            AmpUT_Mp4DmxAV_feed(Time);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            rval = -1;
        }
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_Mp4DmxAV_stop();
    } else if (strcmp(argv[1], "exit") == 0) {
        AmpUT_Mp4DmxAV_exit();
    } else if (strcmp(argv[1], "seek") == 0) {
        if (argc == 5) {
            UINT8 Direction;
            UINT32 Time;
            UINT32 Speed;
            Time = atoi(argv[2]);
            Direction = atoi(argv[3]);
            Speed = atoi(argv[4]);
            AmpUT_Mp4DmxAV_seek(Time, Direction, Speed);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            rval = -1;
        }
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_Mp4DmxAV_test();
    } else {
        AmpUT_Mp4DmxAV_Usage();
    }

    return rval;
}

/**
 * MP4 demuxer AV UT - add MP4 demuxer test function.
 */
int AmpUT_Mp4DmxAVTestAdd(void)
{
    AmbaTest_RegisterCommand("mp4dmxav", AmpUT_Mp4DmxAVTest);/**< hook command */
    return AMP_OK;
}

