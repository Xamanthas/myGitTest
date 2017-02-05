/**
 *  @file AmpUT_MkvMux.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2014/4/16  |clchan      |Created        |
 *
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <recorder/Encode.h>
#include "AmpUnitTest.h"
#include "AmbaUtility.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaCache_Def.h>
#include "DummyVideoEnc.h"
#include "FormatLib.h"

#include <format/Muxer.h>
#include <format/MkvMux.h>
#include <stream/File.h>
#include <util.h>

#define BS_FN   "C:\\OUT_0000.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0000.nhnt" /**< nhnt file name */
#define UDTA_FN "C:\\OUT_0000.udta" /**< user define data file name */
#define MKV_FN  "C:\\TEST.MKV" /**< mkv file name */
#define SPLIT_PREFIX   "C:\\TEST" /**< prefix of the split file */
#define MKVMUX_CODEC_HDLR ((void *)0x1234) /**< codec handler */
#define MKVMUX_ENC_RAW_SIZE (10 << 20) /**< 10MB raw buffer */
#define MKVMUX_MAX_FIFO_NUM (256) /**< number of entry of FIFO */
#define SPLIT_TIME  (1000 * 60 * 30)  /**< 30 min split */
#define BYTE2SYNC   (10 << 20) /**< 10MB */
#define STACK_SIZE  0x8000 /**< stack size */

static void *g_MkvMuxBuffer = NULL; /**< mux buffer */
static AMP_FIFO_HDLR_s *g_MkvEncFifoHdlr = NULL; /**< encode FIFO handler */
static AMP_FIFO_HDLR_s *g_MkvMuxFifoHdlr = NULL; /**< mux FIFO handler */
static AMP_STREAM_HDLR_s *g_pStream = NULL; /**< stream handler */
static AMP_MUX_FORMAT_HDLR_s *g_pFormat = NULL; /**< format handler */
static AMP_MOVIE_INFO_s *g_pMovie = NULL; /**< movie information */
static AMP_MUXER_PIPE_HDLR_s *g_pMuxPipe = NULL; /**< muxer pipe */
static AMBA_KAL_TASK_t g_MkvMuxMuxTask = {0}; /**< mux task */
static void *g_pMkvMuxMuxStack = NULL; /**< mux stack */
static UINT32 g_nSplitCount = 0; /**< count of split file */
static FORMAT_USER_DATA_s g_Udta; /**< user define data */
static DummyVideoEnc_s g_Encoder; /**< video encoder */

/**
 * MKV mux UT - video encoder event callback function.
 *
 * @param [in] pHdlr video encoder handler.
 * @param [in] nEvent event id.
 * @param [in] pInfo callback information.
 * @return 0 - OK, others - fail
 *
 */
static int DummyVideoEnc_EventCB(void *pHdlr, UINT32 nEvent, void *pInfo)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    if (nEvent == AMP_FIFO_EVENT_DATA_CONSUMED) {
        if (RingBuf_DeleteFrame(&g_Encoder.RingBuf) == -1)
             AmbaPrint("RingBuf_DeleteFrame Fail");
    }

    return 0;
}

/**
 * MKV mux UT - mux task.
 *
 * @param [in] info task information.
 *
 */
static void AmpUT_MkvMux_MuxTask(UINT32 info)
{
    int rval = 0;
    AmbaPrint("%s Start", __FUNCTION__);
    AmpMuxer_WaitComplete(g_pMuxPipe, AMBA_KAL_WAIT_FOREVER);
    if (AmpMuxer_Remove(g_pMuxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (AmpMuxer_Delete(g_pMuxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    g_pMuxPipe = NULL;
    if (AmpMkvMux_Delete(g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    g_pFormat = NULL;
    if (g_pStream->Func->Close(g_pStream) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (AmpFileStream_Delete(g_pStream) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    g_pStream = NULL;
    while (AmpCFS_FStatus(g_pMovie->Name) != AMP_CFS_STATUS_UNUSED)
        AmbaKAL_TaskSleep(100);
    if (AmpFormat_RelMovieInfo(g_pMovie, TRUE) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    g_pMovie = NULL;
    // delete fifo
    if (g_MkvMuxFifoHdlr != NULL) {
        AmpFifo_Delete(g_MkvMuxFifoHdlr);
        g_MkvMuxFifoHdlr = NULL;
    }
    if (g_MkvEncFifoHdlr != NULL) {
        AmpFifo_Delete(g_MkvEncFifoHdlr);
        g_MkvEncFifoHdlr = NULL;
    }
DONE:
    if (rval == -1) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
    } else {
        AmbaPrint("%s Stop", __FUNCTION__);
        AmbaPrint("[SUCCESS] AmpUT_MkvMux : End");
    }
}

/**
 * MKV mux UT - FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_FifoCB(void *hdlr, UINT32 event, void* info)
{
    //AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        AmpMuxer_OnDataReady((AMP_FIFO_HDLR_s *)hdlr);
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
        AmpMuxer_OnEOS((AMP_FIFO_HDLR_s *)hdlr);
    }

    return 0;
}

/**
 * MKV mux UT - create movie name function.
 *
 * @param [out] szName movie name.
 * @param [in] nSize length of movie name.
 *
 */
static void AmpUT_MkvMux_CreateMovName(char *szName, UINT32 nSize)
{
    if (g_nSplitCount == 0) {
        strncpy(szName, MKV_FN, nSize);
        szName[nSize - 1] = '\0';
    } else {
        char file[MAX_FILENAME_LENGTH] = {0};
        snprintf(file, MAX_FILENAME_LENGTH, "%s%02u.MKV", SPLIT_PREFIX, g_nSplitCount);
        strncpy(szName, file, nSize);
        szName[nSize - 1] = '\0';
    }
    g_nSplitCount++;
}

/**
 * MKV mux UT - reset movie information function.
 *
 * @param [in,out] pMovie movie information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_ResetMovie(AMP_MOVIE_INFO_s *pMovie)
{
    UINT8 TrackId;
    UINT64 nMinDTS;
    AMP_MEDIA_TRACK_INFO_s *pMin;
    if (FormatLib_AdjustDTS(pMovie) != AMP_OK) {
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
        return -1;
    }
    pMin = FormatLib_GetShortestTrack((AMP_MEDIA_INFO_s *)pMovie);
    K_ASSERT(pMin != NULL);
    nMinDTS = pMin->DTS;
    for (TrackId = 0; TrackId < pMovie->TrackCount; TrackId++) {
        AMP_MEDIA_TRACK_INFO_s *pTrack = &pMovie->Track[TrackId];
        pTrack->InitDTS = pTrack->NextDTS = pTrack->DTS = (pTrack->DTS - nMinDTS);
        if (pTrack->TrackType == AMP_MEDIA_TRACK_TYPE_VIDEO) {
            pTrack->Info.Video.RefDTS = pTrack->InitDTS;
            FormatLib_ResetPTS(&pMovie->Track[TrackId]);
        }
    }
    return 0;
}

/**
 * MKV mux UT - file split function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_AutoSplit(void)
{
    int rval = 0;
    AMP_MOVIE_INFO_s *pMovie;
    char szName[MAX_FILENAME_LENGTH] = {0};

    if (g_pFormat->Func->Close(g_pFormat, AMP_MUX_FORMAT_CLOSE_DEFAULT) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    if (g_pStream->Func->Close(g_pStream) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);

    AmpUT_MkvMux_CreateMovName(szName, MAX_FILENAME_LENGTH);
    // replace media info
    AmpFormat_NewMovieInfo(szName, &pMovie);
    AmpFormat_CopyMovieInfo(pMovie, g_pMovie);
    AmpFormat_RelMovieInfo(g_pMovie, TRUE);
    g_pMovie = pMovie;
    FormatLib_ResetMuxMediaInfo((AMP_MEDIA_INFO_s *)g_pMovie);
    if (AmpUT_MkvMux_ResetMovie(g_pMovie) != 0)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    // mkv would need read on close
    if (g_pStream->Func->Open(g_pStream, g_pMovie->Name, AMP_STREAM_MODE_WRRD) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    g_pFormat->Media = (AMP_MEDIA_INFO_s *)g_pMovie;
    if (g_pFormat->Func->Open(g_pFormat) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    return rval;
}

/**
 * MKV mux UT - mux callback function.
 *
 * @param [in] hdlr muxer handler.
 * @param [in] event event id.
 * @param [in] info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_MuxCB(void* hdlr, UINT32 event, void* info)
{
    int rval = 0;
    switch (event) {
    case AMP_MUXER_EVENT_START:
        AmbaPrint("AMP_MUXER_EVENT_START");
        break;
    case AMP_MUXER_EVENT_END:
        AmbaPrint("AMP_MUXER_EVENT_END");
        DummyVideoEnc_Stop(&g_Encoder);
        break;
    case AMP_MUXER_EVENT_REACH_LIMIT:
        AmbaPrint("AMP_MUXER_EVENT_REACH_LIMIT");
        rval = AmpUT_MkvMux_AutoSplit();
        break;
    default:
        break;
    }
    return rval;
}

/**
 * MKV mux UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_init(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *pFmtBuf = NULL;
    AMP_MUXER_INIT_CFG_s muxerInitCfg;
    void *pMuxerBuf = NULL;
    AMP_MKV_MUX_INIT_CFG_s MKVMuxInitCfg;
    void *pMkvBuf = NULL;
    AMP_INDEX_INIT_CFG_s IndexInitCfg;
    void *pIdxBuf = NULL;
    AMP_FILE_STREAM_INIT_CFG_s fileInitCfg;
    void *pFileBuf;
    AMP_CFS_STAT stat;
    AMP_CFS_FILE_s *hUdtaFile;
    AMP_CFS_FILE_PARAM_s cfsParam;
    void *MkvMuxRawBuf;
    void *pStkRawBuf;
    const UINT32 BufferSize = DummyVideoEnc_GetRequiredBufSize(MKVMUX_ENC_RAW_SIZE, MKVMUX_MAX_FIFO_NUM);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_MkvMuxBuffer, &MkvMuxRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoEnc_Init(&g_Encoder, BS_FN, HDR_FN, g_MkvMuxBuffer, BufferSize, MKVMUX_MAX_FIFO_NUM);

    // Read UDTA file
    if (AmpCFS_Stat(UDTA_FN, &stat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpCFS_GetFileParam(&cfsParam);
    cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    strcpy(cfsParam.Filename, UDTA_FN);
    hUdtaFile = AmpCFS_fopen(&cfsParam);
    if (hUdtaFile == NULL) {
        AmbaPrint("hIdxFile Open fail");
        goto ERR;
    } else {
        AmpCFS_fread(&g_Udta, 1, sizeof(g_Udta), hUdtaFile);
        AmpCFS_fclose(hUdtaFile);
    }

    // init format
    AmpFormat_GetInitDefaultCfg(&fmtInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fmtInitCfg.Buffer, &pFmtBuf, fmtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFormat_Init(&fmtInitCfg);

    // init muxer
    AmpMuxer_GetInitDefaultCfg(&muxerInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&muxerInitCfg.Buffer, &pMuxerBuf, muxerInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpMuxer_Init(&muxerInitCfg);

    /** Init Index Buffer Pool */
    AmpIndex_GetInitDefaultCfg(&IndexInitCfg);
    IndexInitCfg.BufferSize = AmpIndex_GetRequiredBufferSize(IndexInitCfg.MaxHdlr, 2 << 20);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&IndexInitCfg.Buffer, &pIdxBuf, IndexInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpIndex_Init(&IndexInitCfg) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);

    /** MKV Mux Init */
    AmpMkvMux_GetInitDefaultCfg(&MKVMuxInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&MKVMuxInitCfg.Buffer, &pMkvBuf, MKVMuxInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpMkvMux_Init(&MKVMuxInitCfg) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    // init file stream
    AmpFileStream_GetInitDefaultCfg(&fileInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fileInitCfg.Buffer, &pFileBuf, fileInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFileStream_Init(&fileInitCfg);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_pMkvMuxMuxStack, &pStkRawBuf, STACK_SIZE, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    /** Create Mux task */
    AmbaKAL_TaskCreate(&g_MkvMuxMuxTask, /** pTask */
    "UT_MkvMux_MuxTask", /** pTaskName */
    82, /** Priority */
    AmpUT_MkvMux_MuxTask, /** void (*EntryFunction)(UINT32) */
    0x0, /** EntryArg */
    g_pMkvMuxMuxStack,  /** pStackBase */
    STACK_SIZE, /** StackByteSize */
    AMBA_KAL_DO_NOT_START); /** AutoStart */

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MKV mux UT - reset function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_Reset(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;

    g_nSplitCount = 0;
    // create codec fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MKVMUX_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = MKVMUX_MAX_FIFO_NUM;
    fifoDefCfg.cbEvent = DummyVideoEnc_EventCB;
    AmpFifo_Create(&fifoDefCfg, &g_MkvEncFifoHdlr);
    // create muxer fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MKVMUX_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = MKVMUX_MAX_FIFO_NUM;
    fifoDefCfg.cbEvent = AmpUT_MkvMux_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &g_MkvMuxFifoHdlr);
    DummyVideoEnc_Reset(&g_Encoder, g_MkvEncFifoHdlr);
    // reset tasks
    AmbaKAL_TaskReset(&g_MkvMuxMuxTask);
    return 0;
}

/**
 * MKV mux UT - start function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_start(void)
{
    AMP_MKV_MUX_CFG_s MkvCfg;
    AMP_FILE_STREAM_CFG_s fileCfg;
    char szName[MAX_FILENAME_LENGTH] = {'\0'};
    AMP_MUX_MOVIE_INFO_CFG_s movieCfg;
    AMP_MUXER_PIPE_CFG_s MuxPipeCfg;

    // open media info
    AmpUT_MkvMux_Reset();
    AmpUT_MkvMux_CreateMovName(szName, MAX_FILENAME_LENGTH);
    AmpFormat_NewMovieInfo(szName, &g_pMovie);

    // open file stream
    AmpFileStream_GetDefaultCfg(&fileCfg);
    fileCfg.BytesToSync = BYTE2SYNC;
    AmpFileStream_Create(&fileCfg, &g_pStream);
    // mkv would need read on close
    if (g_pStream->Func->Open(g_pStream, g_pMovie->Name, AMP_STREAM_MODE_WRRD) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // set media info
    AmpMuxer_GetDefaultMovieInfoCfg(&movieCfg);
    movieCfg.TrackCount = 1;
    movieCfg.Track[0].TrackType = AMP_MEDIA_TRACK_TYPE_VIDEO;
    movieCfg.Track[0].Fifo = g_MkvMuxFifoHdlr;
    movieCfg.Track[0].BufferBase = (UINT8 *)g_MkvMuxBuffer;
    movieCfg.Track[0].BufferLimit = (UINT8 *)g_MkvMuxBuffer + MKVMUX_ENC_RAW_SIZE;
    movieCfg.Track[0].Info.Video.IsDefault = TRUE;
    movieCfg.Track[0].MediaId = AMP_FORMAT_MID_AVC;
    movieCfg.Track[0].TimeScale = g_Udta.nTimeScale;//atoi(Rate);
    movieCfg.Track[0].TimePerFrame = g_Udta.nTickPerPicture;//atoi(Scale);
    movieCfg.Track[0].Info.Video.Mode = AMP_VIDEO_MODE_P;
    movieCfg.Track[0].Info.Video.M = g_Udta.nM;//atoi(M);
    movieCfg.Track[0].Info.Video.N = g_Udta.nN;//atoi(N);
    movieCfg.Track[0].Info.Video.GOPSize = (UINT32)movieCfg.Track[0].Info.Video.N * g_Udta.nIdrInterval;//atoi(idrItvl);
    movieCfg.Track[0].Info.Video.CodecTimeScale = g_Udta.nTimeScale;    // TODO: should remove CodecTimeScale, but the sample clip uses 90K
    movieCfg.Track[0].Info.Video.Width = g_Udta.nVideoWidth;//atoi(Width);
    movieCfg.Track[0].Info.Video.Height = g_Udta.nVideoHeight;//atoi(Height);
    AmpMuxer_InitMovieInfo(g_pMovie, &movieCfg);
    AmbaPrint("Mkv Mux Create");
    AmpMkvMux_GetDefaultCfg(&MkvCfg);
    MkvCfg.Stream = g_pStream;

    if (AmpMkvMux_Create(&MkvCfg, &g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // create muxer pipe
    AmpMuxer_GetDefaultCfg(&MuxPipeCfg);
    MuxPipeCfg.FormatCount = 1;
    MuxPipeCfg.Format[0] = g_pFormat;
    MuxPipeCfg.Media[0] = (AMP_MEDIA_INFO_s *)g_pMovie;
    MuxPipeCfg.MaxDuration = SPLIT_TIME;
    MuxPipeCfg.OnEvent = AmpUT_MkvMux_MuxCB;
    if (AmpMuxer_Create(&MuxPipeCfg, &g_pMuxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpMuxer_Add(g_pMuxPipe) != AMP_OK){
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpMuxer_Start(g_pMuxPipe, AMBA_KAL_NO_WAIT) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (DummyVideoEnc_Start(&g_Encoder) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmbaKAL_TaskResume(&g_MkvMuxMuxTask);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MKV mux UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_stop(void)
{
    AmpMuxer_Stop(g_pMuxPipe);
    return 0;
}

/**
 * MKV mux UT - force split function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_split(void)
{
    AmpMuxer_SetMaxDuration(g_pMuxPipe, 0);
    return 0;
}

/**
 * MKV mux UT - sync function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_sync(void)
{
    g_pStream->Func->Func(g_pStream, AMP_STREAM_OP_SYNC, 0);
    return 0;
}

/**
 * MKV mux UT - test function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvMux_test(void)
{
    return 0;
}

/**
 * MKV mux UT - MKV mux test function.
 */
static int AmpUT_MkvMuxTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_MkvMux_init();
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_MkvMux_start();
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_MkvMux_stop();
    } else if (strcmp(argv[1], "split") == 0) {
        AmpUT_MkvMux_split();
    } else if (strcmp(argv[1], "sync") == 0) {
        AmpUT_MkvMux_sync();
    } else if (strcmp(argv[1], "end") == 0) {
        //TODO
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_MkvMux_test();
    }
    return 0;
}

/**
 * MKV mux UT - add MKV mux test function.
 */
int AmpUT_MkvMuxTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("mkvmux", AmpUT_MkvMuxTest);

    return AMP_OK;
}
