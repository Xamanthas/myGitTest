/**
 *  @file AmpUT_ExtMuxAV.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2014/04/24 |clchan       |Created        |
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
#include "DummyAudioEnc.h"
#include "FormatLib.h"

#include <format/Muxer.h>
#include <format/ExtMux.h>
#include <util.h>
#include "ExtStream.h"

#define BS_FN   "C:\\OUT_0000.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0000.nhnt" /**< nhnt file name */
#define UDTA_FN "C:\\OUT_0000.udta" /**< user define data file name */
#define AUDIO_FN "C:\\Pri_0000.aac" /**< audio bitstream file name */
#define AUDIO_HDR_FN "C:\\Pri_0000.nhnt" /**< audio nhnt file name */
#define EXT_FN  "C:\\TEST.EXT" /**< ext file name */
#define SPLIT_PREFIX   "C:\\TEST" /**< prefix of the split file */
#define EXTMUX_VIDEO_CODEC_HDLR ((void *)0x1234) /**< video codec handler */
#define EXTMUX_VIDEO_ENC_RAW_SIZE (10 << 20) /**< 10MB raw buffer */
#define EXTMUX_AUDIO_CODEC_HDLR ((void *)0x1235) /**< audio codec handler */
#define EXTMUX_AUDIO_ENC_RAW_SIZE (10 << 20) /**< 2MB raw buffer */
#define EXTMUX_MAX_FIFO_NUM (256) /**< number of entry of FIFO */
#define SPLIT_TIME  (1000 * 60 * 30)  /**< 30 min split */
#define STACK_SIZE  0x1000 /**< stack size */

// global var for dmyenc
static void *g_ExtMuxVideoBuffer = NULL; /**< video mux buffer */
static void *g_ExtMuxAudioBuffer = NULL; /**< audio mux buffer */
static AMP_FIFO_HDLR_s *g_ExtVideoEncFifoHdlr = NULL; /**< video encode FIFO handler */
static AMP_FIFO_HDLR_s *g_ExtVideoMuxFifoHdlr = NULL; /**< video mux FIFO handler */
static AMP_FIFO_HDLR_s *g_ExtAudioEncFifoHdlr = NULL; /**< audio encode FIFO handler */
static AMP_FIFO_HDLR_s *g_ExtAudioMuxFifoHdlr = NULL; /**< audio mux FIFO handler */
static AMP_STREAM_HDLR_s *g_pStream = NULL; /**< stream handler */
static AMP_MUX_FORMAT_HDLR_s *g_pFormat = NULL; /**< format handler */
static AMP_MOVIE_INFO_s *g_pMovie = NULL; /**< movie information */
static AMP_MUXER_PIPE_HDLR_s *g_pMuxPipe = NULL; /**< muxer pipe */

static AMBA_KAL_TASK_t g_ExtMuxMuxTask = {0}; /**< muxer task */
static void *g_pExtMuxMuxStack = NULL; /**< muxer stack */
static UINT32 g_nSplitCount = 0; /**< count of split file */
static FORMAT_USER_DATA_s g_Udta; /**< user define data */
static DummyVideoEnc_s g_VideoEnc; /**< video encoder */
static DummyAudioEnc_s g_AudioEnc; /**< audio encoder */

/**
 * External muxer AV UT - video encoder event callback function.
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
        if (RingBuf_DeleteFrame(&g_VideoEnc.RingBuf) == -1)
             AmbaPrint("RingBuf_DeleteFrame Fail");
    }

    return 0;
}

/**
 * External muxer AV UT - audio encoder event callback function.
 *
 * @param [in] pHdlr audio encoder handler.
 * @param [in] nEvent event id.
 * @param [in] pInfo callback information.
 * @return 0 - OK, others - fail
 *
 */
static int DummyAudioEnc_EventCB(void *pHdlr, UINT32 nEvent, void *pInfo)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    if (nEvent == AMP_FIFO_EVENT_DATA_CONSUMED) {
        if (RingBuf_DeleteFrame(&g_AudioEnc.RingBuf) == -1)
             AmbaPrint("RingBuf_DeleteFrame Fail");
    }

    return 0;
}

/**
 * External muxer AV UT - muxer task.
 *
 * @param [in] info task information.
 *
 */
static void AmpUT_ExtMuxAV_MuxTask(UINT32 info)
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
    if (AmpExtMux_Delete(g_pFormat) != AMP_OK) {
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
    if (AmpExtStream_Delete(g_pStream) != AMP_OK) {
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
    if (g_ExtVideoMuxFifoHdlr != NULL) {
        AmpFifo_Delete(g_ExtVideoMuxFifoHdlr);
        g_ExtVideoMuxFifoHdlr = NULL;
    }
    if (g_ExtAudioMuxFifoHdlr != NULL) {
        AmpFifo_Delete(g_ExtAudioMuxFifoHdlr);
        g_ExtAudioMuxFifoHdlr = NULL;
    }
    if (g_ExtVideoEncFifoHdlr != NULL) {
        AmpFifo_Delete(g_ExtVideoEncFifoHdlr);
        g_ExtVideoEncFifoHdlr = NULL;
    }
    if (g_ExtAudioEncFifoHdlr != NULL) {
        AmpFifo_Delete(g_ExtAudioEncFifoHdlr);
        g_ExtAudioEncFifoHdlr = NULL;
    }
DONE:
    if (rval == -1) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
    } else {
        AmbaPrint("%s Stop", __FUNCTION__);
        AmbaPrint("[SUCCESS] AmpUT_ExtMuxAV : End");
    }
}

/**
 * External muxer AV UT - FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_FifoCB(void *hdlr, UINT32 event, void* info)
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
 * External muxer AV UT - create movie name function.
 *
 * @param [out] szName movie name.
 * @param [in] nSize length of movie name.
 *
 */
static void AmpUT_ExtMuxAV_CreateMovName(char *szName, UINT32 nSize)
{
    if (g_nSplitCount == 0) {
        strncpy(szName, EXT_FN, nSize);
        szName[nSize - 1] = '\0';
    } else {
        char file[MAX_FILENAME_LENGTH] = {0};
        snprintf(file, MAX_FILENAME_LENGTH, "%s%02u.EXT", SPLIT_PREFIX, g_nSplitCount);
        strncpy(szName, file, nSize);
        szName[nSize - 1] = '\0';
    }
    g_nSplitCount++;
}

/**
 * External muxer AV UT - reset movie information function.
 *
 * @param [in,out] pMovie movie information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_ResetMovie(AMP_MOVIE_INFO_s *pMovie)
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
 * External muxer 4 stream UT - file split function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_AutoSplit(void)
{
    int rval = 0;
    AMP_MOVIE_INFO_s *pMovie;
    char szName[MAX_FILENAME_LENGTH] = {0};

    if (g_pFormat->Func->Close(g_pFormat, AMP_MUX_FORMAT_CLOSE_DEFAULT) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    if (g_pStream->Func->Close(g_pStream) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);

    AmpUT_ExtMuxAV_CreateMovName(szName, MAX_FILENAME_LENGTH);
    // replace media info
    AmpFormat_NewMovieInfo(szName, &pMovie);
    AmpFormat_CopyMovieInfo(pMovie, g_pMovie);
    AmpFormat_RelMovieInfo(g_pMovie, TRUE);
    g_pMovie = pMovie;
    FormatLib_ResetMuxMediaInfo((AMP_MEDIA_INFO_s *)g_pMovie);
    if (AmpUT_ExtMuxAV_ResetMovie(pMovie) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    if (g_pStream->Func->Open(g_pStream, g_pMovie->Name, AMP_STREAM_MODE_WRONLY) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    g_pFormat->Media = (AMP_MEDIA_INFO_s *)g_pMovie;
    if (g_pFormat->Func->Open(g_pFormat) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    return rval;
}

/**
 * External muxer AV UT - muxer callback function.
 *
 * @param [in] hdlr muxer handler.
 * @param [in] event event id.
 * @param [in] info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_MuxCB(void* hdlr, UINT32 event, void* info)
{
    int rval = 0;
    switch (event) {
    case AMP_MUXER_EVENT_START:
        AmbaPrint("AMP_MUXER_EVENT_START");
        break;
    case AMP_MUXER_EVENT_END:
        AmbaPrint("AMP_MUXER_EVENT_END");
        DummyVideoEnc_Stop(&g_VideoEnc);
        DummyAudioEnc_Stop(&g_AudioEnc);
        break;
    case AMP_MUXER_EVENT_REACH_LIMIT:
        AmbaPrint("AMP_MUXER_EVENT_REACH_LIMIT");
        rval = AmpUT_ExtMuxAV_AutoSplit();
        break;
    default:
        break;
    }
    return rval;
}

/**
 * External muxer AV UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_init(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *pFmtBuf = NULL;
    AMP_MUXER_INIT_CFG_s muxerInitCfg;
    void *pMuxerBuf = NULL;
    AMP_EXT_MUX_INIT_CFG_s EXTMuxInitCfg;
    void *pExtBuf = NULL;
    AMP_EXT_STREAM_INIT_CFG_s fileInitCfg;
    void *pFileBuf;
    AMP_CFS_STAT stat;
    AMP_CFS_FILE_s *hUdtaFile;
    AMP_CFS_FILE_PARAM_s cfsParam;
    void *ExtMuxVideoRawBuf;
    void *ExtMuxAudioRawBuf;
    void *pStkRawBuf;
    UINT32 BufferSize = DummyVideoEnc_GetRequiredBufSize(EXTMUX_VIDEO_ENC_RAW_SIZE, EXTMUX_MAX_FIFO_NUM);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_ExtMuxVideoBuffer, &ExtMuxVideoRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoEnc_Init(&g_VideoEnc, BS_FN, HDR_FN, g_ExtMuxVideoBuffer, BufferSize, EXTMUX_MAX_FIFO_NUM);
    BufferSize = DummyAudioEnc_GetRequiredBufSize(EXTMUX_AUDIO_ENC_RAW_SIZE, EXTMUX_MAX_FIFO_NUM);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_ExtMuxAudioBuffer, &ExtMuxAudioRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyAudioEnc_Init(&g_AudioEnc, AUDIO_FN, AUDIO_HDR_FN, g_ExtMuxAudioBuffer, BufferSize, EXTMUX_MAX_FIFO_NUM);

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
    /** EXT Mux Init */
    AmpExtMux_GetInitDefaultCfg(&EXTMuxInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EXTMuxInitCfg.Buffer, &pExtBuf, EXTMuxInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpExtMux_Init(&EXTMuxInitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    // init file stream
    AmpExtStream_GetInitDefaultCfg(&fileInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fileInitCfg.Buffer, &pFileBuf, fileInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpExtStream_Init(&fileInitCfg);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_pExtMuxMuxStack, &pStkRawBuf, STACK_SIZE, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    /** Create Mux task */
    AmbaKAL_TaskCreate(&g_ExtMuxMuxTask, /** pTask */
    "UT_ExtMuxAV_MuxTask", /** pTaskName */
    82, /** Priority */
    AmpUT_ExtMuxAV_MuxTask, /** void (*EntryFunction)(UINT32) */
    0x0, /** EntryArg */
    g_pExtMuxMuxStack,  /** pStackBase */
    STACK_SIZE, /** StackByteSize */
    AMBA_KAL_DO_NOT_START); /** AutoStart */

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * External muxer AV UT - reset function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_Reset(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;

    g_nSplitCount = 0;
    // create codec fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = EXTMUX_VIDEO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = EXTMUX_MAX_FIFO_NUM;
    fifoDefCfg.cbEvent = DummyVideoEnc_EventCB;
    AmpFifo_Create(&fifoDefCfg, &g_ExtVideoEncFifoHdlr);
    // create codec fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = EXTMUX_AUDIO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = EXTMUX_MAX_FIFO_NUM;
    fifoDefCfg.cbEvent = DummyAudioEnc_EventCB;
    AmpFifo_Create(&fifoDefCfg, &g_ExtAudioEncFifoHdlr);
    // create muxer fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = EXTMUX_VIDEO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = EXTMUX_MAX_FIFO_NUM;
    fifoDefCfg.cbEvent = AmpUT_ExtMuxAV_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &g_ExtVideoMuxFifoHdlr);
    DummyVideoEnc_Reset(&g_VideoEnc, g_ExtVideoEncFifoHdlr);
    // create muxer fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = EXTMUX_AUDIO_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = EXTMUX_MAX_FIFO_NUM;
    fifoDefCfg.cbEvent = AmpUT_ExtMuxAV_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &g_ExtAudioMuxFifoHdlr);
    DummyAudioEnc_Reset(&g_AudioEnc, g_ExtAudioEncFifoHdlr);
    // reset tasks
    AmbaKAL_TaskReset(&g_ExtMuxMuxTask);
    return 0;
}

/**
 * External muxer AV UT - start function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_start(void)
{
    AMP_EXT_MUX_CFG_s ExtCfg;
    AMP_EXT_STREAM_CFG_s fileCfg;
    char szName[MAX_FILENAME_LENGTH] = {'\0'};
    AMP_MUX_MOVIE_INFO_CFG_s movieCfg;
    AMP_MUXER_PIPE_CFG_s MuxPipeCfg;

    // open media info
    AmpUT_ExtMuxAV_Reset();
    AmpUT_ExtMuxAV_CreateMovName(szName, MAX_FILENAME_LENGTH);
    AmpFormat_NewMovieInfo(szName, &g_pMovie);

    // open file stream
    AmpExtStream_GetDefaultCfg(&fileCfg);
    AmpExtStream_Create(&fileCfg, &g_pStream);
    if (g_pStream->Func->Open(g_pStream, g_pMovie->Name, AMP_STREAM_MODE_WRONLY) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // set media info
    AmpMuxer_GetDefaultMovieInfoCfg(&movieCfg);
    movieCfg.TrackCount = 2;
    movieCfg.Track[0].TrackType = AMP_MEDIA_TRACK_TYPE_VIDEO;
    movieCfg.Track[0].Fifo = g_ExtVideoMuxFifoHdlr;
    movieCfg.Track[0].BufferBase = (UINT8 *)g_ExtMuxVideoBuffer;
    movieCfg.Track[0].BufferLimit = (UINT8 *)g_ExtMuxVideoBuffer + EXTMUX_VIDEO_ENC_RAW_SIZE;
    movieCfg.Track[0].Info.Video.IsDefault = TRUE;
    movieCfg.Track[0].MediaId = AMP_FORMAT_MID_AVC;
    movieCfg.Track[0].TimeScale = g_Udta.nTimeScale;//atoi(Rate);
    movieCfg.Track[0].TimePerFrame = g_Udta.nTickPerPicture;//atoi(Scale);
    movieCfg.Track[0].Info.Video.Mode = AMP_VIDEO_MODE_P;
    movieCfg.Track[0].Info.Video.M = g_Udta.nM;//atoi(M);
    movieCfg.Track[0].Info.Video.N = g_Udta.nN;//atoi(N);
    movieCfg.Track[0].Info.Video.GOPSize = (UINT32)movieCfg.Track[0].Info.Video.N * g_Udta.nIdrInterval;//atoi(idrItvl);
    movieCfg.Track[0].Info.Video.CodecTimeScale = 90000;    // TODO: should remove CodecTimeScale, but the sample clip uses 90K
    movieCfg.Track[0].Info.Video.Width = g_Udta.nVideoWidth;//atoi(Width);
    movieCfg.Track[0].Info.Video.Height = g_Udta.nVideoHeight;//atoi(Height);
    movieCfg.Track[1].TrackType = AMP_MEDIA_TRACK_TYPE_AUDIO;
    movieCfg.Track[1].Fifo = g_ExtAudioMuxFifoHdlr;
    movieCfg.Track[1].BufferBase = (UINT8 *)g_ExtMuxAudioBuffer;
    movieCfg.Track[1].BufferLimit = (UINT8 *)g_ExtMuxAudioBuffer + EXTMUX_AUDIO_ENC_RAW_SIZE;
    movieCfg.Track[1].MediaId = AMP_FORMAT_MID_AAC;
    movieCfg.Track[1].Info.Audio.SampleRate = 48000;
    movieCfg.Track[1].TimeScale = movieCfg.Track[0].TimeScale;
    movieCfg.Track[1].TimePerFrame = ((UINT64)movieCfg.Track[1].TimeScale * 1024 / movieCfg.Track[1].Info.Audio.SampleRate);
    movieCfg.Track[1].Info.Audio.Channels = 1;
    AmpMuxer_InitMovieInfo(g_pMovie, &movieCfg);
    AmbaPrint("Ext Mux Create");
    AmpExtMux_GetDefaultCfg(AMP_MEDIA_INFO_MOVIE, &ExtCfg);
    ExtCfg.Stream = g_pStream;

    if (AmpExtMux_Create(&ExtCfg, &g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // create muxer pipe
    AmpMuxer_GetDefaultCfg(&MuxPipeCfg);
    MuxPipeCfg.FormatCount = 1;
    MuxPipeCfg.Format[0] = g_pFormat;
    MuxPipeCfg.Media[0] = (AMP_MEDIA_INFO_s *)g_pMovie;
    MuxPipeCfg.MaxDuration = SPLIT_TIME;
    MuxPipeCfg.OnEvent = AmpUT_ExtMuxAV_MuxCB;
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
    if (DummyVideoEnc_Start(&g_VideoEnc) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (DummyAudioEnc_Start(&g_AudioEnc) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmbaKAL_TaskResume(&g_ExtMuxMuxTask);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * External muxer AV UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_stop(void)
{
    AmpMuxer_Stop(g_pMuxPipe);
    return 0;
}

/**
 * External muxer AV UT - force split function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_split(void)
{
    AmpMuxer_SetMaxDuration(g_pMuxPipe, 0);
    return 0;
}

/**
 * External muxer AV UT - test function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExtMuxAV_test(void)
{
    return 0;
}

/**
 * External muxer AV UT - External muxer test function.
 */
static int AmpUT_ExtMuxAVTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_ExtMuxAV_init();
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_ExtMuxAV_start();
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_ExtMuxAV_stop();
    } else if (strcmp(argv[1], "split") == 0) {
        AmpUT_ExtMuxAV_split();
    } else if (strcmp(argv[1], "end") == 0) {
        //TODO
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_ExtMuxAV_test();
    }
    return 0;
}

/**
 * External muxer AV UT - add External muxer test function.
 */
int AmpUT_ExtMuxAVTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("extmuxav", AmpUT_ExtMuxAVTest);

    return AMP_OK;
}

