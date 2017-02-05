/**
 *  @file AmpUT_Mp4Mux2s.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2014/1/9   |felix       |Created        |
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

#define USE_TEMP_IDX
//#define USE_MEM_IDX

#include <format/Muxer.h>
#include <format/Mp4Mux.h>
#include <index/Index.h>
#ifdef USE_TEMP_IDX
#include <index/Temp.h>
#else
#ifdef USE_MEM_IDX
#include <index/Mem.h>
#else
#include <index/Raw.h>
extern AMBA_NAND_DEV_INFO_s AmbaNAND_DevInfo;
#endif
#endif
#include <stream/File.h>
#include <util.h>

#define BS_FN   "C:\\OUT_0000.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0000.nhnt" /**< nhnt file name */
#define UDTA_FN "C:\\OUT_0000.udta" /**< user define data file name */
#define IDX_FN  "C:\\TEST.IDX" /**< index file name 1 */
#define IDX_FN2  "C:\\TEST2.IDX" /**< index file name 2 */
#define MP4_FN  "C:\\TEST.MP4" /**< mp4 file name 1 */
#define MP4_FN2  "C:\\TEST2.MP4" /**< mp4 file name 2 */
#define SPLIT_PREFIX   "C:\\TEST" /**< prefix of the split file 1 */
#define SPLIT_PREFIX2   "C:\\TEST2" /**< prefix of the split file 2 */
#define MP4MUX_CODEC_HDLR ((void *)0x1234) /**< coder handler */
#define MP4MUX_ENC_RAW_SIZE (10 << 20) /**< 10MB raw buffer */
#define MP4MUX_MAX_FIFO_NUM (256) /**< number of entry of FIFO */
#define SPLIT_TIME    (1000 * 60 * 30)  /**< 30 min split */
#define BYTE2SYNC   (10 << 20) /**< 10MB */
#define STACK_SIZE  0x1000 /**< stack size */

static void *g_Mp4MuxBuf = NULL; /**< mux buffer */
static AMP_FIFO_HDLR_s *g_Mp4EncFifoHdlr = NULL; /**< encoder FIFO handler */
static AMP_FIFO_HDLR_s *g_Mp4MuxFifoHdlr = NULL; /**< muxer FIFO handler 1*/
static AMP_FIFO_HDLR_s *g_Mp4MuxFifoHdlr2 = NULL; /**< muxer FIFO handler 2*/
static AMP_STREAM_HDLR_s *g_pStream = NULL; /**< stream handler 1 */
static AMP_STREAM_HDLR_s *g_pStream2 = NULL; /**< stream handler 2 */
static AMP_INDEX_HDLR_s *g_pIndex = NULL; /**< index handler 1 */
static AMP_INDEX_HDLR_s *g_pIndex2 = NULL; /**< index handler 2 */
static AMP_MUX_FORMAT_HDLR_s *g_pFormat = NULL; /**< format handler 1 */
static AMP_MUX_FORMAT_HDLR_s *g_pFormat2 = NULL; /**< format handler 2 */
static AMP_MOVIE_INFO_s *g_pMovie = NULL; /**< movie information 1 */
static AMP_MOVIE_INFO_s *g_pMovie2 = NULL; /**< movie information 2 */
static AMP_MUXER_PIPE_HDLR_s *g_pMuxPipe = NULL; /**< muxer pipe */
static AMBA_KAL_TASK_t g_Mp4MuxMuxTask = {0}; /**< muxer task */
static void *g_pMp4MuxMuxStack = NULL; /**< muxer stack */
static UINT32 g_nSplitCount = 0; /**< count of split file 1 */
static UINT32 g_nSplitCount2 = 0; /**< count of split file 2 */
static FORMAT_USER_DATA_s g_Udta; /**< user define data */
static DummyVideoEnc_s g_Encoder; /**< video encoder */

/**
 * MP4 muxer 2 stream UT - video encoder event callback function.
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
 * MP4 muxer 2 stream UT - muxer task.
 *
 * @param [in] info task information.
 *
 */
static void AmpUT_Mp4Mux2s_MuxTask(UINT32 info)
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
    if (AmpMp4Mux_Delete(g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (AmpMp4Mux_Delete(g_pFormat2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
#ifdef USE_TEMP_IDX
    if (AmpTempIdx_Delete(g_pIndex) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (AmpTempIdx_Delete(g_pIndex2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
#else
#ifdef USE_MEM_IDX
    if (AmpMemIdx_Delete(g_pIndex) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (AmpMemIdx_Delete(g_pIndex2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
#else
    if (AmpRawIdx_Delete(g_pIndex) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (AmpRawIdx_Delete(g_pIndex2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
#endif
#endif
    g_pIndex = NULL;
    g_pIndex2 = NULL;
    g_pFormat = NULL;
    g_pFormat2 = NULL;
    if (g_pStream->Func->Close(g_pStream) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (g_pStream2->Func->Close(g_pStream2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (AmpFileStream_Delete(g_pStream) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (AmpFileStream_Delete(g_pStream2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    g_pStream = NULL;
    g_pStream2 = NULL;
    while (AmpCFS_FStatus(g_pMovie->Name) != AMP_CFS_STATUS_UNUSED)
        AmbaKAL_TaskSleep(100);
    while (AmpCFS_FStatus(g_pMovie2->Name) != AMP_CFS_STATUS_UNUSED)
        AmbaKAL_TaskSleep(100);
    if (AmpFormat_RelMovieInfo(g_pMovie, TRUE) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    if (AmpFormat_RelMovieInfo(g_pMovie2, TRUE) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    g_pMovie = NULL;
    g_pMovie2 = NULL;
    // delete fifo
    if (g_Mp4MuxFifoHdlr != NULL) {
        AmpFifo_Delete(g_Mp4MuxFifoHdlr);
        g_Mp4MuxFifoHdlr = NULL;
    }
    if (g_Mp4MuxFifoHdlr2 != NULL) {
        AmpFifo_Delete(g_Mp4MuxFifoHdlr2);
        g_Mp4MuxFifoHdlr2 = NULL;
    }
    if (g_Mp4EncFifoHdlr != NULL) {
        AmpFifo_Delete(g_Mp4EncFifoHdlr);
        g_Mp4EncFifoHdlr = NULL;
    }
DONE:
    if (rval == -1) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
    } else {
        AmbaPrint("%s Stop", __FUNCTION__);
        AmbaPrint("[SUCCESS] AmpUT_Mp4Mux2s : End");
    }
}

/**
 * MP4 muxer 2 stream UT - FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_FifoCB(void *hdlr, UINT32 event, void* info)
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
 * MP4 muxer 2 stream UT - create movie name function.
 *
 * @param [out] szName movie name.
 * @param [in] nSize length of movie name.
 *
 */
static void AmpUT_Mp4Mux2s_CreateMovName(char *szName, UINT32 nSize)
{
    if (g_nSplitCount == 0) {
        strncpy(szName, MP4_FN, nSize);
        szName[nSize - 1] = '\0';
    } else {
        char file[MAX_FILENAME_LENGTH] = {0};
        snprintf(file, MAX_FILENAME_LENGTH, "%s%02u.MP4", SPLIT_PREFIX, g_nSplitCount);
        strncpy(szName, file, nSize);
        szName[nSize - 1] = '\0';
    }
    g_nSplitCount++;
}

/**
 * MP4 muxer 2 stream UT - create movie name function 2.
 *
 * @param [out] szName movie name.
 * @param [in] nSize length of movie name.
 *
 */
static void AmpUT_Mp4Mux2s_CreateMovName2(char *szName, UINT32 nSize)
{
    if (g_nSplitCount2 == 0) {
        strncpy(szName, MP4_FN2, nSize);
        szName[nSize - 1] = '\0';
    } else {
        char file[MAX_FILENAME_LENGTH] = {0};
        snprintf(file, MAX_FILENAME_LENGTH, "%s%02u.MP4", SPLIT_PREFIX2, g_nSplitCount2);
        strncpy(szName, file, nSize);
        szName[nSize - 1] = '\0';
    }
    g_nSplitCount2++;
}

/**
 * MP4 muxer 2 stream UT - reset movie information function.
 *
 * @param [in,out] pMovie movie information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_ResetMovie(AMP_MOVIE_INFO_s *pMovie)
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
 * MP4 muxer 2 stream UT - file split function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_AutoSplit(void)
{
    int rval = 0;
    AMP_MOVIE_INFO_s *pMovie;
    char szName[MAX_FILENAME_LENGTH] = {0};

    if (g_pFormat->Func->Close(g_pFormat, AMP_MUX_FORMAT_CLOSE_DEFAULT) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    if (g_pStream->Func->Close(g_pStream) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);

    AmpUT_Mp4Mux2s_CreateMovName(szName, MAX_FILENAME_LENGTH);
    // replace media info
    AmpFormat_NewMovieInfo(szName, &pMovie);
    AmpFormat_CopyMovieInfo(pMovie, g_pMovie);
    AmpFormat_RelMovieInfo(g_pMovie, TRUE);
    g_pMovie = pMovie;
    FormatLib_ResetMuxMediaInfo((AMP_MEDIA_INFO_s *)g_pMovie);
    if (AmpUT_Mp4Mux2s_ResetMovie(pMovie) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    if (g_pStream->Func->Open(g_pStream, g_pMovie->Name, AMP_STREAM_MODE_WRONLY) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    g_pFormat->Media = (AMP_MEDIA_INFO_s *)g_pMovie;
    if (g_pFormat->Func->Open(g_pFormat) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);

    if (g_pFormat2->Func->Close(g_pFormat2, AMP_MUX_FORMAT_CLOSE_DEFAULT) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    if (g_pStream2->Func->Close(g_pStream2) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);

    AmpUT_Mp4Mux2s_CreateMovName2(szName, MAX_FILENAME_LENGTH);
    // replace media info
    AmpFormat_NewMovieInfo(szName, &pMovie);
    AmpFormat_CopyMovieInfo(pMovie, g_pMovie2);
    AmpFormat_RelMovieInfo(g_pMovie2, TRUE);
    g_pMovie2 = pMovie;
    FormatLib_ResetMuxMediaInfo((AMP_MEDIA_INFO_s *)g_pMovie2);
    if (AmpUT_Mp4Mux2s_ResetMovie(pMovie) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    if (g_pStream2->Func->Open(g_pStream2, g_pMovie2->Name, AMP_STREAM_MODE_WRONLY) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    g_pFormat2->Media = (AMP_MEDIA_INFO_s *)g_pMovie2;
    if (g_pFormat2->Func->Open(g_pFormat2) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);

    return rval;
}

/**
 * MP4 muxer 2 stream UT - muxer callback function.
 *
 * @param [in] hdlr muxer handler.
 * @param [in] event event id.
 * @param [in] info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_MuxCB(void* hdlr, UINT32 event, void* info)
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
        rval = AmpUT_Mp4Mux2s_AutoSplit();
        break;
    default:
        break;
    }
    return rval;
}

/**
 * MP4 muxer 2 stream UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_init(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *pFmtBuf = NULL;
    AMP_MUXER_INIT_CFG_s muxerInitCfg;
    void *pMuxerBuf = NULL;
    AMP_MP4_MUX_INIT_CFG_s MP4MuxInitCfg;
    void *pMp4Buf = NULL;
    AMP_INDEX_INIT_CFG_s IndexInitCfg;
    void *pIdxBuf = NULL;
#ifdef USE_TEMP_IDX
    AMP_TEMP_IDX_INIT_CFG_s TempInitCfg;
    void *pTempIdxBuf = NULL;
#else
#ifdef USE_MEM_IDX
    AMP_MEM_IDX_INIT_CFG_s MemInitCfg;
    void *pMemIdxBuf = NULL;
#else
    AMP_RAW_IDX_INIT_CFG_s RawInitCfg;
    void *pRawIdxBuf = NULL;
#endif
#endif
    AMP_FILE_STREAM_INIT_CFG_s fileInitCfg;
    void *pFileBuf;
    AMP_CFS_STAT stat;
    AMP_CFS_FILE_s *hUdtaFile;
    AMP_CFS_FILE_PARAM_s cfsParam;
    void *Mp4MuxRawBuf;
    void *pStkRawBuf;
    const UINT32 BufferSize = DummyVideoEnc_GetRequiredBufSize(MP4MUX_ENC_RAW_SIZE, MP4MUX_MAX_FIFO_NUM);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_Mp4MuxBuf, &Mp4MuxRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoEnc_Init(&g_Encoder, BS_FN, HDR_FN, g_Mp4MuxBuf, BufferSize, MP4MUX_MAX_FIFO_NUM);

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

    /** MP4 Mux Init */
    AmpMp4Mux_GetInitDefaultCfg(&MP4MuxInitCfg);
    MP4MuxInitCfg.MaxHdlr = 2;
    MP4MuxInitCfg.BufferSize = AmpMp4Mux_GetRequiredBufferSize(MP4MuxInitCfg.MaxHdlr);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&MP4MuxInitCfg.Buffer, &pMp4Buf, MP4MuxInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpMp4Mux_Init(&MP4MuxInitCfg) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);

    /** Init Index Buffer Pool */
    AmpIndex_GetInitDefaultCfg(&IndexInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&IndexInitCfg.Buffer, &pIdxBuf, IndexInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpIndex_Init(&IndexInitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

#ifdef USE_TEMP_IDX
    /** Init Temp Index */
    AmpTempIdx_GetInitDefaultCfg(&TempInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&TempInitCfg.Buffer, &pTempIdxBuf, TempInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpTempIdx_Init(&TempInitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#else
#ifdef USE_MEM_IDX
    /** Init Mem Index */
    AmpMemIdx_GetInitDefaultCfg(&MemInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&MemInitCfg.Buffer, &pMemIdxBuf, MemInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpMemIdx_Init(&MemInitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#else
    /** Init Raw Index */
#ifdef CONFIG_ENABLE_EMMC_BOOT
    AmpRawIdx_GetInitDefaultCfg(&RawInitCfg, AMP_RAW_IDX_DEV_TYPE_EMMC);
#else
    AmpRawIdx_GetInitDefaultCfg(&RawInitCfg, AMP_RAW_IDX_DEV_TYPE_NAND);
    memcpy(&RawInitCfg.DevInfo.Nand.Info, &AmbaNAND_DevInfo, sizeof(AMBA_NAND_DEV_INFO_s));
#endif
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&RawInitCfg.Buffer, &pRawIdxBuf, RawInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpRawIdx_Init(&RawInitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#endif
#endif
    // init file stream
    AmpFileStream_GetInitDefaultCfg(&fileInitCfg);
    fileInitCfg.MaxHdlr = 8;
    fileInitCfg.BufferSize = AmpFileStream_GetRequiredBufferSize(fileInitCfg.MaxHdlr);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fileInitCfg.Buffer, &pFileBuf, fileInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFileStream_Init(&fileInitCfg);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_pMp4MuxMuxStack, &pStkRawBuf, STACK_SIZE, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    /** Create Mux task */
    AmbaKAL_TaskCreate(&g_Mp4MuxMuxTask, /** pTask */
    "UT_Mp4Mux_MuxTask", /** pTaskName */
    82, /** Priority */
    AmpUT_Mp4Mux2s_MuxTask, /** void (*EntryFunction)(UINT32) */
    0x0, /** EntryArg */
    g_pMp4MuxMuxStack,  /** pStackBase */
    STACK_SIZE, /** StackByteSize */
    AMBA_KAL_DO_NOT_START); /** AutoStart */

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MP4 muxer 2 stream UT - reset function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_Reset(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;

    g_nSplitCount = 0;
    g_nSplitCount2 = 0;
    // create codec fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = MP4MUX_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = MP4MUX_MAX_FIFO_NUM;
    fifoDefCfg.cbEvent = DummyVideoEnc_EventCB;
    AmpFifo_Create(&fifoDefCfg, &g_Mp4EncFifoHdlr);
    // create muxer fifo
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.cbEvent = AmpUT_Mp4Mux2s_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &g_Mp4MuxFifoHdlr);
    // create muxer fifo
    AmpFifo_Create(&fifoDefCfg, &g_Mp4MuxFifoHdlr2);
    DummyVideoEnc_Reset(&g_Encoder, g_Mp4EncFifoHdlr);
    // reset tasks
    AmbaKAL_TaskReset(&g_Mp4MuxMuxTask);
    return 0;
}

/**
 * MP4 muxer 2 stream UT - start function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_start(void)
{
    AMP_MP4_MUX_CFG_s Mp4Cfg;
#ifdef USE_TEMP_IDX
    AMP_TEMP_IDX_CFG_s TempIdxCfg;
#else
#ifdef USE_MEM_IDX
    AMP_MEM_IDX_CFG_s MemIdxCfg;
#else
    AMP_RAW_IDX_CFG_s RawIdxCfg;
#endif
#endif
    AMP_FILE_STREAM_CFG_s fileCfg;
    char szName[MAX_FILENAME_LENGTH] = {'\0'};
    AMP_MUX_MOVIE_INFO_CFG_s movieCfg;
    AMP_MUXER_PIPE_CFG_s MuxPipeCfg;

    // open media info
    AmpUT_Mp4Mux2s_Reset();
    AmpUT_Mp4Mux2s_CreateMovName(szName, MAX_FILENAME_LENGTH);
    AmpFormat_NewMovieInfo(szName, &g_pMovie);
    AmpUT_Mp4Mux2s_CreateMovName2(szName, MAX_FILENAME_LENGTH);
    AmpFormat_NewMovieInfo(szName, &g_pMovie2);

    // open file stream
    AmpFileStream_GetDefaultCfg(&fileCfg);
    fileCfg.BytesToSync = BYTE2SYNC;
    AmpFileStream_Create(&fileCfg, &g_pStream);
    if (g_pStream->Func->Open(g_pStream, g_pMovie->Name, AMP_STREAM_MODE_WRONLY) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFileStream_GetDefaultCfg(&fileCfg);
    fileCfg.BytesToSync = BYTE2SYNC;
    AmpFileStream_Create(&fileCfg, &g_pStream2);
    if (g_pStream2->Func->Open(g_pStream2, g_pMovie2->Name, AMP_STREAM_MODE_WRONLY) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // set media info
    AmpMuxer_GetDefaultMovieInfoCfg(&movieCfg);
    movieCfg.TrackCount = 1;
    movieCfg.Track[0].TrackType = AMP_MEDIA_TRACK_TYPE_VIDEO;
    movieCfg.Track[0].Fifo = g_Mp4MuxFifoHdlr;
    movieCfg.Track[0].BufferBase = (UINT8 *)g_Mp4MuxBuf;
    movieCfg.Track[0].BufferLimit = (UINT8 *)g_Mp4MuxBuf + MP4MUX_ENC_RAW_SIZE;
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
    movieCfg.Track[0].Fifo = g_Mp4MuxFifoHdlr2;
    AmpMuxer_InitMovieInfo(g_pMovie2, &movieCfg);

#ifdef USE_TEMP_IDX
    /** Create Temp Index */
    AmpTempIdx_GetDefaultCfg(&TempIdxCfg);
    if (AmpTempIdx_Create(&TempIdxCfg, &g_pIndex) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpTempIdx_Create(&TempIdxCfg, &g_pIndex2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#else
#ifdef USE_MEM_IDX
    /** Create Mem Index */
    AmpMemIdx_GetDefaultCfg(&MemIdxCfg);
    if (AmpMemIdx_Create(&MemIdxCfg, &g_pIndex) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpMemIdx_Create(&MemIdxCfg, &g_pIndex2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#else
    /** Create Raw Index */
    AmpRawIdx_GetDefaultCfg(&RawIdxCfg);
    if (AmpRawIdx_Create(&RawIdxCfg, &g_pIndex) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpRawIdx_Create(&RawIdxCfg, &g_pIndex2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#endif
#endif
    AmbaPrint("Mp4 Mux Create");
    AmpMp4Mux_GetDefaultCfg(&Mp4Cfg);
    Mp4Cfg.Stream = g_pStream;
    Mp4Cfg.Index = g_pIndex;
    if (AmpMp4Mux_Create(&Mp4Cfg, &g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    Mp4Cfg.Stream = g_pStream2;
    Mp4Cfg.Index = g_pIndex2;
    if (AmpMp4Mux_Create(&Mp4Cfg, &g_pFormat2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // create muxer pipe
    AmpMuxer_GetDefaultCfg(&MuxPipeCfg);
    MuxPipeCfg.FormatCount = 2;
    MuxPipeCfg.Format[0] = g_pFormat;
    MuxPipeCfg.Media[0] = (AMP_MEDIA_INFO_s *)g_pMovie;
    MuxPipeCfg.Format[1] = g_pFormat2;
    MuxPipeCfg.Media[1] = (AMP_MEDIA_INFO_s *)g_pMovie2;
    MuxPipeCfg.MaxDuration = SPLIT_TIME;
    MuxPipeCfg.OnEvent = AmpUT_Mp4Mux2s_MuxCB;
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
    AmbaKAL_TaskResume(&g_Mp4MuxMuxTask);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * MP4 muxer 2 stream UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_stop(void)
{
    AmpMuxer_Stop(g_pMuxPipe);
    return 0;
}

/**
 * MP4 muxer 2 stream UT - force split function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_split(void)
{
    AmpMuxer_SetMaxDuration(g_pMuxPipe, 0);
    return 0;
}

/**
 * MP4 muxer 2 stream UT - test function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Mux2s_test(void)
{
    return 0;
}

/**
 * MP4 muxer 2 stream UT - MP4 muxer test function.
 */
static int AmpUT_Mp4Mux2sTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_Mp4Mux2s_init();
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_Mp4Mux2s_start();
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_Mp4Mux2s_stop();
    } else if (strcmp(argv[1], "split") == 0) {
        AmpUT_Mp4Mux2s_split();
    } else if (strcmp(argv[1], "end") == 0) {
        //TODO
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_Mp4Mux2s_test();
    }
    return 0;
}

/**
 * MP4 muxer 2 stream UT - add MP4 muxer test function.
 */
int AmpUT_Mp4Mux2sTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("mp4mux2s", AmpUT_Mp4Mux2sTest);

    return AMP_OK;
}

