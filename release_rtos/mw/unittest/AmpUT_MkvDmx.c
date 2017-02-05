/**
 *  @file AmpUT_MkvDmx.c
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

#include <format/Demuxer.h>
#include <stream/File.h>
#include <format/MkvDmx.h>
#include <util.h>

#define BS_FN   "C:\\OUT_0001.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0001.nhnt" /**< nhnt file name */
#define MKV_FN  "C:\\TEST.MKV" /**< mkv file name */

#define MKVDMX_DEC_RAW_SIZE (10<<20) /**< 10MB raw buffer */
#define INDEX_BUFFER_SIZE  (1<<20) /**< 1MB */
#define MAX_FIFO_ENTRY_NUM  1024 /**< number of entry of FIFO */

static AMP_STREAM_HDLR_s *g_pStream = NULL; /**< stream handler */
static AMP_DMX_FORMAT_HDLR_s *g_pFormat = NULL; /**< format handler */
static AMP_MOVIE_INFO_s *g_pMovie = NULL; /**< movie information */
static AMP_DEMUXER_PIPE_HDLR_s *g_pDmxPipe = NULL; /**< demuxer pipe */

#define MKVDMX_CODEC_HDLR ((void *)0x1234) /**< codec handler */

static void *g_MkvDmxBuffer = NULL; /**< demux buffer */
static AMP_FIFO_HDLR_s *g_MkvDecFifoHdlr = NULL; /**< decode FIFO handler */
static AMP_FIFO_HDLR_s *g_MkvDmxFifoHdlr = NULL; /**< demux FIFO handler */
static DummyVideoDec_s g_Decoder; /**< video decoder */

/**
 * MKV dmx UT - data request function.
 *
 * @param [in] pDecoder decoder handler.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_DataRequest(void *pDecoder)
{
    return AmpDemuxer_OnDataRequest(g_MkvDmxFifoHdlr);
}

/**
 * MKV dmx UT - decode FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_DecFifoCB(void *hdlr, UINT32 event, void* info)
{
    //AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);
    AMP_FIFO_INFO_s TmpInfo = {0};
    AMP_BITS_DESC_s *pTmpDesc = NULL;
    RING_BUFFER_INFO_s RingBufInfo = {0};
    if (event == AMP_FIFO_EVENT_GET_WRITE_POINT) {
        pTmpDesc = (AMP_BITS_DESC_s *) info;
        if (RingBuf_GetInfo(&g_Decoder.RingBuf, &RingBufInfo) != 0)
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        pTmpDesc->Size = RingBufInfo.nRingBufFreeSize;
        pTmpDesc->StartAddr = RingBufInfo.pRingBufWp;
        if (pTmpDesc->StartAddr == NULL)
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    } else if (event == AMP_FIFO_EVENT_DATA_READY) {
        if (AmpFifo_GetInfo(g_MkvDecFifoHdlr, &TmpInfo) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        } else {
            if (AmpFifo_PeekEntry(g_MkvDecFifoHdlr, &pTmpDesc, (TmpInfo.AvailEntries - 1)) != AMP_OK) {
                AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            } else {
                if (pTmpDesc->Size < AMP_FIFO_MARK_EOS) {
                    if (RingBuf_UpdateWp(&g_Decoder.RingBuf, pTmpDesc->StartAddr, pTmpDesc->Size) != 0)
                        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
                }
            }
        }
    }
    return 0;
}

/**
 * MKV dmx UT - demux FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_DmxFifoCB(void *hdlr, UINT32 event, void* info)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    return 0;
}

/**
 * MKV dmx UT - demux callback function.
 *
 * @param [in] hdlr demuxer handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_DmxCB(void *hdlr, UINT32 event, void* info)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    switch (event) {
    case AMP_DEMUXER_EVENT_START:
        AmbaPrint("AMP_DEMUXER_EVENT_START");
        DummyVideoDec_Start(&g_Decoder);
        break;
    case AMP_DEMUXER_EVENT_END:
        AmbaPrint("AMP_DEMUXER_EVENT_END");
        DummyVideoDec_Stop(&g_Decoder);
        AmbaPrint("[SUCCESS] AmpUT_MkvDmx : End");
        break;
    default:
        break;
    }
    return 0;
}

/**
 * MKV dmx UT - demux initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_DmxInit(void)
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
    void *pFileBuf;
    void *MkvDmxRawBuf = NULL;
    const UINT32 BufferSize = DummyVideoDec_GetRequiredBufSize(MKVDMX_DEC_RAW_SIZE, MAX_FIFO_ENTRY_NUM);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_MkvDmxBuffer, &MkvDmxRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoDec_Init(&g_Decoder, BS_FN, HDR_FN, g_MkvDmxBuffer, BufferSize, MAX_FIFO_ENTRY_NUM, AmpUT_MkvDmx_DataRequest);
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
 * MKV dmx UT - demux open function.
 *
 * @param [in] szName file name.
 * @param [in] Speed demux speed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_DmxOpen(char *szName, UINT32 Speed)
{
    AMP_FIFO_CFG_s fifoDefCfg = {0};
    AMP_FILE_STREAM_CFG_s fileCfg;
    AMP_DEMUXER_PIPE_CFG_s dmxCfg;
    AMP_MKV_DMX_CFG_s MkvCfg = {0};
    AMP_DMX_MOVIE_INFO_CFG_s MovieCfg;

    /** Create decoder fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MKVDMX_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_MkvDmx_DecFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_MkvDecFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    /** Create demux fifo */
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MKVDMX_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = MAX_FIFO_ENTRY_NUM;
    fifoDefCfg.cbEvent = AmpUT_MkvDmx_DmxFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &g_MkvDmxFifoHdlr) != AMP_OK) {
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
    MovieCfg.Track[0].Fifo = g_MkvDmxFifoHdlr;
    MovieCfg.Track[0].BufferBase = g_MkvDmxBuffer;
    MovieCfg.Track[0].BufferLimit = (UINT8 *)g_MkvDmxBuffer + MKVDMX_DEC_RAW_SIZE;
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
    dmxCfg.OnEvent = AmpUT_MkvDmx_DmxCB;
    dmxCfg.Speed = Speed;
    if (AmpDemuxer_Create(&dmxCfg, &g_pDmxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpDemuxer_Add(g_pDmxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoDec_Reset(&g_Decoder, g_MkvDecFifoHdlr);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MKV dmx UT - demux close function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_DmxClose(void)
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

    if (g_MkvDmxFifoHdlr != NULL) {
        AmpFifo_Delete(g_MkvDmxFifoHdlr);
        g_MkvDmxFifoHdlr = NULL;
    }

    if (g_MkvDecFifoHdlr != NULL) {
        AmpFifo_Delete(g_MkvDecFifoHdlr);
        g_MkvDecFifoHdlr = NULL;
    }

    return 0;
}

/**
 * MKV dmx UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_init(void)
{
    /** Initial Mkv demux */
    if (AmpUT_MkvDmx_DmxInit() != 0)
        AmbaPrint("[FAIL] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV dmx UT - start function.
 *
 * @param [in] StartTime start demux time.
 * @param [in] Direction demux direction.
 * @param [in] Speed demux speed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_start(UINT32 StartTime, UINT8 Direction, UINT32 Speed)
{
    /** Open Mkv demux */
    int rval;
    UINT32 speed = ((Speed == 0) ? 1 : Speed);
    rval = AmpUT_MkvDmx_DmxOpen(MKV_FN, speed); if (rval != AMP_OK) { goto ERR; }

    if (Direction == 0) {
        AmbaPrint("Mkv Demux - Forward");
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 1200); if (rval != AMP_OK) { goto ERR; }
        if ((StartTime == 0) && (speed == 1))
            rval = AmpDemuxer_Start(g_pDmxPipe);
        else
            rval = AmpDemuxer_Seek(g_pDmxPipe, StartTime, Direction, speed);
        if (rval != AMP_OK) { goto ERR; }
        AmpDemuxer_OnDataRequest(g_MkvDmxFifoHdlr);
        AmbaKAL_TaskSleep(100);
        rval = AmpDemuxer_SetProcParam(g_pDmxPipe, 200); if (rval != AMP_OK) { goto ERR; }
    } else if (Direction == 1) {
        AmbaPrint("Mkv Demux - Backward");
        rval =  AmpDemuxer_SetProcParam(g_pDmxPipe, 1200);if (rval != AMP_OK) { goto ERR; }
        rval = AmpDemuxer_Seek(g_pDmxPipe, StartTime, Direction, speed);if (rval != AMP_OK) { goto ERR; }
        AmpDemuxer_OnDataRequest(g_MkvDmxFifoHdlr);
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
 * MKV dmx UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_stop(void)
{
    AmbaPrint("%s", __FUNCTION__);
    AmpDemuxer_Stop(g_pDmxPipe);
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV dmx UT - exit function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_exit(void)
{
    AmbaPrint("%s", __FUNCTION__);
    AmpUT_MkvDmx_DmxClose();
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV dmx UT - usage describe function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_Usage(void)
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
 * MKV dmx UT - start usage describe function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_StartUsage(void)
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
 * MKV dmx UT - feed function.
 *
 * @param [in] Time time for feed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_feed(UINT32 Time)
{
    AmpUT_MkvDmx_DmxOpen(MKV_FN, 1);
    AmpDemuxer_FeedFrame(g_pFormat, 0, Time, AMP_FIFO_TYPE_IDR_FRAME);
    AmpUT_MkvDmx_DmxClose();
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV dmx UT - seek function.
 *
 * @param [in] nTime time to seek.
 * @param [in] nDirection demux direction.
 * @param [in] nSpeed demux speed.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_seek(UINT32 nTime, UINT8 nDirection, UINT32 nSpeed)
{
    AmpDemuxer_Seek(g_pDmxPipe, nTime, nDirection, ((nSpeed == 0) ? 1 : nSpeed));
    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
}

/**
 * MKV dmx UT - test function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvDmx_test(void)
{
    return 0;
}

/**
 * MKV dmx UT - MKV dmx test function.
 */
static int AmpUT_MkvDmxTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int rval = 0;

//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_MkvDmx_init();
    } else if (strcmp(argv[1], "start") == 0) {
        if (argc == 5) {
            UINT32 StartTime;
            UINT8 Direction;
            UINT32 Speed;
            StartTime = atoi(argv[2]);
            Direction = atoi(argv[3]);
            Speed = atoi(argv[4]);
            AmbaPrint("mkvdmx start %u %u %u", StartTime, Direction, Speed);
            AmpUT_MkvDmx_start(StartTime, Direction, Speed);
        } else if (argc == 2) {
            AmbaPrint("mkvdmx start");
            AmpUT_MkvDmx_start(0, 0, 1);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            AmpUT_MkvDmx_StartUsage();
            rval = -1;
        }
    } else if (strcmp(argv[1], "feed") == 0) {
        if (argc == 3) {
            UINT32 Time;
            Time = atoi(argv[2]);
            AmpUT_MkvDmx_feed(Time);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            rval = -1;
        }
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_MkvDmx_stop();
    } else if (strcmp(argv[1], "exit") == 0) {
        AmpUT_MkvDmx_exit();
    } else if (strcmp(argv[1], "seek") == 0) {
        if (argc == 5) {
            UINT8 Direction;
            UINT32 Time;
            UINT32 Speed;
            Time = atoi(argv[2]);
            Direction = atoi(argv[3]);
            Speed = atoi(argv[4]);
            AmpUT_MkvDmx_seek(Time, Direction, Speed);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            rval = -1;
        }
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_MkvDmx_test();
    } else {
        AmpUT_MkvDmx_Usage();
    }

    return rval;
}

/**
 * MKV dmx UT - add MKV dmx test function.
 */
int AmpUT_MkvDmxTestAdd(void)
{
    AmbaTest_RegisterCommand("mkvdmx", AmpUT_MkvDmxTest);/**< hook command */
    return AMP_OK;
}

