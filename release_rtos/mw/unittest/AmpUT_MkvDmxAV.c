/**
 *  @file AmpUT_MkvDmxAV.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2014/11/27 |clchan      |Created        |
 *
 *
 *  @copyright 2014 Ambarella Corporation. All rights reserved.
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
#include <format/MkvDmx.h>
#include <util.h>

#define BS_FN   "C:\\OUT_0001.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0001.nhnt" /**< nhnt file name */
#define MKV_FN  "C:\\TEST.MKV" /**< mkv file name */
#define AUDIO_FN "C:\\AUDIO.raw" /**< audio raw file name */

#define MKVDMX_DEC_RAW_SIZE (10<<20) /**< 10MB raw buffer */
#define INDEX_BUFFER_SIZE  (1<<20) /**< 1MB */
#define MAX_FIFO_ENTRY_NUM  1024 /**< number of entry of FIFO */

static AMP_STREAM_HDLR_s *g_pStream = NULL; /**< stream handler */
static AMP_DMX_FORMAT_HDLR_s *g_pFormat = NULL; /**< format handler */
static AMP_MOVIE_INFO_s *g_pMovie = NULL; /**< movie information */
static AMP_DEMUXER_PIPE_HDLR_s *g_pDmxPipe = NULL; /**< demuxer pipe */

#define MKVDMX_VIDEO_CODEC_HDLR ((void *)0x1234) /**< video codec handler */
#define MKVDMX_AUDIO_CODEC_HDLR ((void *)0x1235) /**< audio codec handler */

static void *g_MkvDmxVideoBuf = NULL; /**< demux video buffer */
static AMP_FIFO_HDLR_s *g_MkvDmxVideoDecFifoHdlr = NULL; /**< video decode FIFO handler */
static AMP_FIFO_HDLR_s *g_MkvDmxVideoFifoHdlr = NULL; /**< video demux FIFO handler */

static void *g_MkvDmxAudioBuf = NULL; /**< demux audio buffer */
static AMP_FIFO_HDLR_s *g_MkvDmxAudioDecFifoHdlr = NULL; /**< audio decode FIFO handler */
static AMP_FIFO_HDLR_s *g_MkvDmxAudioFifoHdlr = NULL; /**< audio demux FIFO handler */

static DummyVideoDec_s g_VideoDec; /**< video decoder */
static DummyAudioDec_s g_AudioDec; /**< audio decoder */

/**
 * MKV demuxer AV UT - data request function.
 *
 * @param [in] pDecoder decoder handler.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUt_MkvDmx_DataRequest(void *pDecoder)
{
    if (pDecoder == &g_VideoDec)
        return AmpDemuxer_OnDataRequest(g_MkvDmxVideoFifoHdlr);
    if (pDecoder == &g_AudioDec)
        return AmpDemuxer_OnDataRequest(g_MkvDmxAudioFifoHdlr);
    AmbaPrint("%s : invalid fifo!!", __FUNCTION__);
    return -1;
}

/**
 * MKV demuxer AV UT - video decode FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_VideoDecFifoCB(void *hdlr, UINT32 event, void* info)
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
        if (AmpFifo_GetInfo(g_MkvDmxVideoDecFifoHdlr, &TmpInfo) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        } else {
            if (AmpFifo_PeekEntry(g_MkvDmxVideoDecFifoHdlr, &pTmpDesc, (TmpInfo.AvailEntries - 1)) != AMP_OK) {
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
 * MKV demuxer AV UT - audio decode FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_AudioDecFifoCB(void *hdlr, UINT32 event, void* info)
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
        if (AmpFifo_GetInfo(g_MkvDmxAudioDecFifoHdlr, &TmpInfo) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        } else {
            if (AmpFifo_PeekEntry(g_MkvDmxAudioDecFifoHdlr, &pTmpDesc, (TmpInfo.AvailEntries - 1)) != AMP_OK) {
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
 * MKV demuxer AV UT - demux FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_DmxFifoCB(void *hdlr, UINT32 event, void* info)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    return 0;
}

/**
 * MKV demuxer AV UT - demux callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_DmxCB(void *hdlr, UINT32 event, void* info)
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
        AmbaPrint("[SUCCESS] AmpUT_MkvDmxAV : End");
        break;
    default:
        break;
    }
    return 0;
}

/**
 * MKV demuxer AV UT - demux initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_DmxInit(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *pFmtBuf = NULL;
    AMP_DEMUXER_INIT_CFG_s demuxerInitCfg;
    void *pDemuxerBuf = NULL;
    AMP_MKV_DMX_INIT_CFG_s MkvInitCfg;
    void *pMkvBuf = NULL;
    AMP_INDEX_INIT_CFG_s IndexBufCfg;
    void *pIdxBuf = NULL;
    AMP_FILE_STREAM_INIT_CFG_s fileInitCfg;
    void *pFileBuf = NULL;
    void *MkvDmxVideoRawBuf = NULL;
    void *MkvDmxAudioRawBuf = NULL;
    UINT32 BufferSize = DummyVideoDec_GetRequiredBufSize(MKVDMX_DEC_RAW_SIZE, MAX_FIFO_ENTRY_NUM);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_MkvDmxVideoBuf, &MkvDmxVideoRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoDec_Init(&g_VideoDec, BS_FN, HDR_FN, g_MkvDmxVideoBuf, BufferSize, MAX_FIFO_ENTRY_NUM, AmpUt_MkvDmx_DataRequest);
    BufferSize = DummyAudioDec_GetRequiredBufSize(MKVDMX_DEC_RAW_SIZE, MAX_FIFO_ENTRY_NUM);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_MkvDmxAudioBuf, &MkvDmxAudioRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyAudioDec_Init(&g_AudioDec, AUDIO_FN, g_MkvDmxAudioBuf, BufferSize, MAX_FIFO_ENTRY_NUM, AmpUt_MkvDmx_DataRequest);
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
    /** Initial Mkv Demux */
    AmpMkvDmx_GetInitDefaultCfg(&MkvInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&MkvInitCfg.Buffer, &pMkvBuf, MkvInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpMkvDmx_Init(&MkvInitCfg) != AMP_OK) {
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
 * MKV demuxer AV UT - demux open function.
 *
 * @param [in] szName file name.
 * @param [in] Speed demux speed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_DmxOpen(char *szName, UINT32 Speed)
{
    AMP_FIFO_CFG_s fifoDefCfg = {0};
    AMP_FILE_STREAM_CFG_s fileCfg;
    AMP_DEMUXER_PIPE_CFG_s dmxCfg;
    AMP_MKV_DMX_CFG_s MkvCfg = {0};
    AMP_DMX_MOVIE_INFO_CFG_s MovieCfg;

    /** Create decoder fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MKVDMX_VIDEO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_MkvDmxAV_VideoDecFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_MkvDmxVideoDecFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Create demux fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MKVDMX_VIDEO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_MkvDmxAV_DmxFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_MkvDmxVideoFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Create decoder fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MKVDMX_AUDIO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_MkvDmxAV_AudioDecFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_MkvDmxAudioDecFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Create demux fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MKVDMX_AUDIO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_MkvDmxAV_DmxFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_MkvDmxAudioFifoHdlr) != AMP_OK) {
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
    /** Open Media info (Mkvdmx) */
    if (AmpFormat_GetMovieInfo(szName, AmpMkvDmx_Parse, g_pStream, &g_pMovie) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Inifial Demuxer */
    AmpDemuxer_GetDefaultMovieInfoCfg(&MovieCfg, g_pMovie);
    MovieCfg.Track[0].Fifo = g_MkvDmxVideoFifoHdlr;
    MovieCfg.Track[0].BufferBase = g_MkvDmxVideoBuf;
    MovieCfg.Track[0].BufferLimit = (UINT8 *)g_MkvDmxVideoBuf + MKVDMX_DEC_RAW_SIZE;
    MovieCfg.Track[1].Fifo = g_MkvDmxAudioFifoHdlr;
    MovieCfg.Track[1].BufferBase = g_MkvDmxAudioBuf;
    MovieCfg.Track[1].BufferLimit = (UINT8 *)g_MkvDmxAudioBuf + MKVDMX_DEC_RAW_SIZE;
    AmpDemuxer_InitMovieInfo(g_pMovie, &MovieCfg);
    /** Craete Mkv demux */
    AmpMkvDmx_GetDefaultCfg(&MkvCfg);
    MkvCfg.Stream = g_pStream;
    if (AmpMkvDmx_Create(&MkvCfg, &g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Create Demuxer pipe */
    AmpDemuxer_GetDefaultCfg(&dmxCfg);
    dmxCfg.FormatCount = 1;
    dmxCfg.Format[0] = g_pFormat;
    dmxCfg.Media[0] = (AMP_MEDIA_INFO_s *)g_pMovie;
    dmxCfg.OnEvent = AmpUT_MkvDmxAV_DmxCB;
    dmxCfg.Speed = Speed;
    if (AmpDemuxer_Create(&dmxCfg, &g_pDmxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpDemuxer_Add(g_pDmxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoDec_Reset(&g_VideoDec, g_MkvDmxVideoDecFifoHdlr);
    DummyAudioDec_Reset(&g_AudioDec, g_MkvDmxAudioDecFifoHdlr);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MKV demuxer AV UT - demux close function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_DmxClose(void)
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
        AmpMkvDmx_Delete(g_pFormat);
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

    if (g_MkvDmxVideoFifoHdlr != NULL) {
        AmpFifo_Delete(g_MkvDmxVideoFifoHdlr);
        g_MkvDmxVideoFifoHdlr = NULL;
    }

    if (g_MkvDmxVideoDecFifoHdlr != NULL) {
        AmpFifo_Delete(g_MkvDmxVideoDecFifoHdlr);
        g_MkvDmxVideoDecFifoHdlr = NULL;
    }

    if (g_MkvDmxAudioFifoHdlr != NULL) {
        AmpFifo_Delete(g_MkvDmxAudioFifoHdlr);
        g_MkvDmxAudioFifoHdlr = NULL;
    }

    if (g_MkvDmxAudioDecFifoHdlr != NULL) {
        AmpFifo_Delete(g_MkvDmxAudioDecFifoHdlr);
        g_MkvDmxAudioDecFifoHdlr = NULL;
    }

    return 0;
}

/**
 * MKV demuxer AV UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_init(void)
{
    /** Initial Mkv demux */
    if (AmpUT_MkvDmxAV_DmxInit() != 0)
        AmbaPrint("[FAIL] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV demuxer AV UT - start function.
 *
 * @param [in] StartTime start demux time.
 * @param [in] Direction demux direction.
 * @param [in] Speed demux speed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_start(UINT32 StartTime, UINT8 Direction, UINT32 Speed)
{
    /** Open Mkv demux */
    int rval;
    UINT32 speed = ((Speed == 0) ? 1 : Speed);
    rval = AmpUT_MkvDmxAV_DmxOpen(MKV_FN, speed); if (rval != AMP_OK) { goto ERR; }

    if (Direction == 0) {
        AmbaPrint("Mkv Demux - Forward");
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 1200); if (rval != AMP_OK) { goto ERR; }
        if ((StartTime == 0) && (speed == 1))
            rval = AmpDemuxer_Start(g_pDmxPipe);
        else
            rval = AmpDemuxer_Seek(g_pDmxPipe, StartTime, Direction, speed);
        if (rval != AMP_OK) { goto ERR; }
        AmpDemuxer_OnDataRequest(g_MkvDmxVideoFifoHdlr);
        AmpDemuxer_OnDataRequest(g_MkvDmxAudioFifoHdlr);
        AmbaKAL_TaskSleep(100);
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 200); if (rval != AMP_OK) { goto ERR; }
    } else if (Direction == 1) {
        AmbaPrint("Mkv Demux - Backward");
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 1200); if (rval != AMP_OK) { goto ERR; }
        rval = AmpDemuxer_Seek(g_pDmxPipe, StartTime, Direction, speed); if (rval != AMP_OK) { goto ERR; }
        AmpDemuxer_OnDataRequest(g_MkvDmxVideoFifoHdlr);
        AmpDemuxer_OnDataRequest(g_MkvDmxAudioFifoHdlr);
        AmbaKAL_TaskSleep(100);
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 200);if (rval != AMP_OK) { goto ERR; }
    } else {
        AmbaPrint("%s, Error Direction", __FUNCTION__);
    }
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MKV demuxer AV UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_stop(void)
{
    AmbaPrint("%s", __FUNCTION__);
    AmpDemuxer_Stop(g_pDmxPipe);
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV demuxer AV UT - exit function.
 */
static int AmpUT_MkvDmxAV_exit(void)
{
    AmbaPrint("%s", __FUNCTION__);
    AmpUT_MkvDmxAV_DmxClose();
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV demuxer AV UT - usage describe function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_Usage(void)
{
    AmbaPrint("Mkv Demux Test Command");
    AmbaPrint("----------------------");
    AmbaPrint("1. init - start demux");
    AmbaPrint("2. start - start demux");
    AmbaPrint("3. stop - stop demux");
    AmbaPrint("4. exit - exit demux");
    return 0;
}

/**
 * MKV demuxer AV UT - start usage describe function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_StartUsage(void)
{
    AmbaPrint("Mkv Demux Test Command - start");
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
 * MKV demuxer AV UT - feed function.
 *
 * @param [in] Time time for feed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_feed(UINT32 Time)
{
    AmpUT_MkvDmxAV_DmxOpen(MKV_FN, 1);
    AmpDemuxer_FeedFrame(g_pFormat, 0, Time, AMP_FIFO_TYPE_IDR_FRAME);
    AmpUT_MkvDmxAV_DmxClose();
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV demuxer AV UT - seek function.
 *
 * @param [in] nTime time to seek.
 * @param [in] nDirection demux direction.
 * @param [in] nSpeed demux speed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_seek(UINT32 nTime, UINT8 nDirection, UINT32 nSpeed)
{
    AmpDemuxer_Seek(g_pDmxPipe, nTime, nDirection, ((nSpeed == 0) ? 1 : nSpeed));
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV demuxer AV UT - test function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmxAV_test(void)
{
    return 0;
}

/**
 * MKV demuxer AV UT - MKV demuxer test function.
 */
static int AmpUT_MkvDmxAVTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int rval = 0;

//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_MkvDmxAV_init();
    } else if (strcmp(argv[1], "start") == 0) {
        if (argc == 5) {
            UINT32 StartTime;
            UINT8 Direction;
            UINT32 Speed;
            StartTime = atoi(argv[2]);
            Direction = atoi(argv[3]);
            Speed = atoi(argv[4]);
            AmbaPrint("mkvdmxav start %u %u %u", StartTime, Direction, Speed);
            AmpUT_MkvDmxAV_start(StartTime, Direction, Speed);
        } else if (argc == 2) {
            AmbaPrint("mkvdmxav start");
            AmpUT_MkvDmxAV_start(0, 0, 1);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            AmpUT_MkvDmxAV_StartUsage();
            rval = -1;
        }
    } else if (strcmp(argv[1], "feed") == 0) {
        if (argc == 3) {
            UINT32 Time;
            Time = atoi(argv[2]);
            AmpUT_MkvDmxAV_feed(Time);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            rval = -1;
        }
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_MkvDmxAV_stop();
    } else if (strcmp(argv[1], "exit") == 0) {
        AmpUT_MkvDmxAV_exit();
    } else if (strcmp(argv[1], "seek") == 0) {
        if (argc == 5) {
            UINT8 Direction;
            UINT32 Time;
            UINT32 Speed;
            Time = atoi(argv[2]);
            Direction = atoi(argv[3]);
            Speed = atoi(argv[4]);
            AmpUT_MkvDmxAV_seek(Time, Direction, Speed);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            rval = -1;
        }
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_MkvDmxAV_test();
    } else {
        AmpUT_MkvDmxAV_Usage();
    }

    return rval;
}

/**
 * MKV demuxer AV UT - add MKV demuxer test function.
 */
int AmpUT_MkvDmxAVTestAdd(void)
{
    AmbaTest_RegisterCommand("mkvdmxav", AmpUT_MkvDmxAVTest);/**< hook command */
    return AMP_OK;
}

