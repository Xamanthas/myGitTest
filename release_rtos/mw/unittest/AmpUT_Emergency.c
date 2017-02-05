/**
 *  @file AmpUT_Emergency.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/11/19 |felix       |Created        |
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
#include <format/Editor.h>
#include <format/Mp4Edt.h>

#include <format/Muxer.h>
#include <format/Mp4Mux.h>
#include <format/Mp4Dmx.h>
#include <index/Index.h>
#include <index/Temp.h>
#include <index/Raw.h>
#include <index/Mem.h>
#include <stream/File.h>
#include <util.h>

#define BS_FN   "C:\\OUT_0000.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0000.nhnt" /**< nhnt file name */
#define UDTA_FN "C:\\OUT_0000.udta" /**< user define data file name */
#define IDX_FN  "C:\\TEST.IDX" /**< index file name */
#define MP4_FN  "C:\\TEST.MP4" /**< mp4 file name */
#define MP4_FN_PREFIX   "C:\\TEST" /**< prefix of the split file */
#define SPLIT_PREFIX   "C:\\TEST" /**< prefix of the split file */
#define EM_FN1      "C:\\EM1.MP4" /**< emergency file name 1 */
#define EM_FN1_1    "C:\\EM1_1.MP4" /**< emergency file name 1-1 */
#define EM_FN1_2    "C:\\EM1_2.MP4" /**< emergency file name 1-2 */
#define EM_FN2      "C:\\EM2.MP4" /**< emergency file name 2 */
#define EM_FN2_1    "C:\\EM2_1.MP4" /**< emergency file name 2-1 */
#define EM_FN2_2    "C:\\EM2_2.MP4" /**< emergency file name 2-2 */
#define EM_OUT      "C:\\EM.MP4" /**< emergency file name */
#define EMERGENCY_CODEC_HDLR ((void *)0x1234) /**< codec handler */
#define EMERGENCY_ENC_RAW_SIZE (10 << 20) /**< 10MB raw buffer */
#define EMERGENCY_MAX_FIFO_NUM (256) /**< number of entry of FIFO */
#define EMERGENCY_TMP_BUFFER_SIZE  (1<<20) /**< 1MB */
#define EMERGENCY_HEAD_TIME  5000 /**< length of emergency head file */
#define EMERGENCY_TAIL_TIME  5000 /**< length of emergency tail file */

#define SPLIT_TIME  (1000 * 10 * 1)  /**< 10 sec */
#define STACK_SIZE  0x10000 /**< stack size */

static void *g_EmBuffer = NULL; /**< mux buffer */
static AMP_FIFO_HDLR_s *g_EmEncFifoHdlr = NULL; /**< encode FIFO handler */
static AMP_FIFO_HDLR_s *g_EmFifoHdlr = NULL; /**< FIFO handler */
static AMP_STREAM_HDLR_s *g_pStream = NULL; /**< stream handler */
static AMP_STREAM_HDLR_s *g_pEmStream = NULL; /**< emergency stream handler */
static AMP_STREAM_HDLR_s *g_pTmpStream = NULL; /**< tempory file stream handler */

static AMP_INDEX_HDLR_s *g_pIndex = NULL; /**< index handler */
static AMP_INDEX_HDLR_s *g_pEmIndex = NULL; /**< emergency file index handler */
static AMP_MUX_FORMAT_HDLR_s *g_pFormat = NULL; /**< format handler */
static AMP_MOVIE_INFO_s *g_pMovie = NULL; /**< movie information */
static AMP_MUXER_PIPE_HDLR_s *g_pMuxPipe = NULL; /**< muxer pipe */

static AMBA_KAL_TASK_t EmTask = {0}; /**< emergency task */
static void *g_pEmStack = NULL; /**< mux stack */

static AMBA_KAL_TASK_t g_EmMuxTask = {0}; /**< mux task */
static void *g_pEmMuxStack = NULL; /**< mux stack */
static UINT32 g_nSplitCount = 0; /**< count of split file */
static FORMAT_USER_DATA_s g_Udta; /**< user define data */
static DummyVideoEnc_s g_Encoder; /**< video encoder */

static AMP_EDT_FORMAT_HDLR_s *g_pEdtHdlr = NULL; /**< format handler */

static UINT8 ProcessFileNo = 0; /**< emergency process file number */
static BOOL EmProcess = FALSE; /**< emergency process flag */
static BOOL bEmInit = FALSE; /**< emergency init flag */
static BOOL EmBusyFlag = FALSE; /**< emergency busy flag */
static UINT64 EmNeedDuration = 0; /**< need duration */
static UINT64 nEmMovieDuration = 0; /**< movie duration */
static UINT64 nHeadDuration = 0; /**< head file duration */
static UINT64 nTailDuration = 0; /**< tail file duration */
static UINT32 nEmFile1_1_Num = 0; /**< number of file 1-1 */
static UINT32 nEmFile1_2_Num = 0; /**< number of file 1-2 */
static UINT32 nEmFile2_1_Num = 0; /**< number of file 2-1 */
static AMP_MOVIE_INFO_s *pEmMovieInfo = NULL; /**< movie information */
static AMP_MEDIA_TRACK_INFO_s * pDefaultTrack; /**< track */
static char szName1[MAX_FILENAME_LENGTH] = {'\0'}; /**< file name 1 */
static char szName2[MAX_FILENAME_LENGTH] = {'\0'}; /**< file name 2 */

/**
 * Emergency UT - process callback function.
 *
 * @param [in] pHdlr process handler.
 * @param [in] nEvent event id.
 * @param [in] pInfo callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_ProcessCB(void *hdlr, UINT32 event, void* info);

/**
 * Emergency UT - video encode callback function.
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
 * Emergency UT - emergency task.
 *
 * @param [in] info task information.
 *
 */
static void AmpUT_Em_EmTask(UINT32 info)
{
    AmbaPrint("Emergency Process Start");
    while(1) {
        if (bEmInit) {
            if (g_nSplitCount > 1) {
                if (!EmBusyFlag) {
                    switch (ProcessFileNo)
                    {
                        case 0: /** File 1-1 */
                        {
                            nEmFile2_1_Num = nEmFile1_1_Num + 1;
                            if (nEmFile1_1_Num == 0) {
                                strncpy(szName1, MP4_FN, MAX_FILENAME_LENGTH);
                                szName1[MAX_FILENAME_LENGTH - 1] = '\0';
                            } else {
                                char file[MAX_FILENAME_LENGTH] = {0};
                                snprintf(file, MAX_FILENAME_LENGTH, "%s%02u.MP4", MP4_FN_PREFIX, nEmFile1_1_Num);
                                strncpy(szName1, file, MAX_FILENAME_LENGTH);
                                szName1[MAX_FILENAME_LENGTH - 1] = '\0';
                            }
                            while (AmpCFS_FStatus(szName1) != AMP_CFS_STATUS_UNUSED)
                                AmbaKAL_TaskSleep(100);
                            AmbaPrint("%s, File 1_1 Process", __FUNCTION__);
                            if (g_pTmpStream->Func->Open(g_pTmpStream, szName1, AMP_STREAM_MODE_RDONLY) == AMP_OK) {
                                if (AmpFormat_GetMovieInfo(szName1, AmpMp4Dmx_Parse, g_pTmpStream, &pEmMovieInfo) == AMP_OK) {
                                    if (g_pTmpStream->Func->Close(g_pTmpStream) != AMP_OK)
                                        AmbaPrint("%s, %u", __FUNCTION__, __LINE__);
                                    pDefaultTrack = FormatLib_GetDefaultTrack((AMP_MEDIA_INFO_s *)pEmMovieInfo, AMP_MEDIA_TRACK_TYPE_VIDEO);
//                                    AmbaPrint("pDefaultTrack->TimeScale = %u", pDefaultTrack->TimeScale);
                                    nHeadDuration = EMERGENCY_HEAD_TIME;
                                    nEmMovieDuration = ((UINT64)pDefaultTrack->FrameCount * pDefaultTrack->TimePerFrame * 1000) / pDefaultTrack->TimeScale / pDefaultTrack->TimePerFrame * 1000;
//                                    AmbaPrint("nHeadDuration = %llu, nEmMovieDuration = %llu", nHeadDuration, nEmMovieDuration);
                                    AmpFormat_RelMovieInfo(pEmMovieInfo, TRUE);
                                    EmBusyFlag = TRUE;
                                    if (nEmMovieDuration >= EMERGENCY_HEAD_TIME) {
                                        AmpEditor_Crop2New(g_pEdtHdlr, FALSE, nEmMovieDuration - EMERGENCY_HEAD_TIME, nEmMovieDuration, TRUE, szName1, EM_FN1);
                                    } else {
                                        if (nEmFile1_1_Num == 0) {
                                            AmpEditor_Crop2New(g_pEdtHdlr, FALSE, 0, nEmMovieDuration, TRUE, szName1, EM_FN1);
                                        } else {
                                            AmpEditor_Crop2New(g_pEdtHdlr, FALSE, 0, nEmMovieDuration, TRUE, szName1, EM_FN1_1);
                                            EmNeedDuration = nHeadDuration - nEmMovieDuration;
//                                            AmbaPrint("EmNeedDuration = %llu", EmNeedDuration);
                                        }
                                    }
                                } else {
                                    AmbaPrint("%s, %u", __FUNCTION__, __LINE__);
                                    break;
                                }
                            } else {
                                AmbaPrint("%s, %u", __FUNCTION__, __LINE__);
                                break;
                            }
                            break;
                        }
                        case 1: /** File 1-2 */
                        {
                            nEmFile1_2_Num = nEmFile1_1_Num - 1;
                            if (nEmFile1_2_Num == 0) {
                                strncpy(szName2, MP4_FN, MAX_FILENAME_LENGTH);
                                szName2[MAX_FILENAME_LENGTH - 1] = '\0';
                            } else {
                                char file[MAX_FILENAME_LENGTH] = {0};
                                snprintf(file, MAX_FILENAME_LENGTH, "%s%02u.MP4", MP4_FN_PREFIX, nEmFile1_2_Num);
                                strncpy(szName2, file, MAX_FILENAME_LENGTH);
                                szName2[MAX_FILENAME_LENGTH - 1] = '\0';
                            }
                            while (AmpCFS_FStatus(szName2) != AMP_CFS_STATUS_UNUSED)
                                AmbaKAL_TaskSleep(100);
                            AmbaPrint("%s, File 1_2 Process", __FUNCTION__);
                            if (g_pTmpStream->Func->Open(g_pTmpStream, szName2, AMP_STREAM_MODE_RDONLY) == AMP_OK) {
                                if (AmpFormat_GetMovieInfo(szName2, AmpMp4Dmx_Parse, g_pTmpStream, &pEmMovieInfo) == AMP_OK) {
                                    if (g_pTmpStream->Func->Close(g_pTmpStream) != AMP_OK)
                                        AmbaPrint("%s, %u", __FUNCTION__, __LINE__);
                                    pDefaultTrack = FormatLib_GetDefaultTrack((AMP_MEDIA_INFO_s *)pEmMovieInfo, AMP_MEDIA_TRACK_TYPE_VIDEO);
                                    nEmMovieDuration = ((UINT64)pDefaultTrack->FrameCount * pDefaultTrack->TimePerFrame * 1000) / pDefaultTrack->TimeScale / pDefaultTrack->TimePerFrame * 1000;
                                    AmpFormat_RelMovieInfo(pEmMovieInfo, TRUE);
                                    if (nEmMovieDuration >= EmNeedDuration) {
                                        EmBusyFlag = TRUE;
                                        AmpEditor_Crop2New(g_pEdtHdlr, FALSE, nEmMovieDuration - EmNeedDuration, nEmMovieDuration, TRUE, szName2, EM_FN1_2);
                                    }
                                }
                            } else {
                                AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
                                break;
                            }
                            break;
                        }
                        case 2:
                        {
                            EmBusyFlag = TRUE;
                            while (AmpCFS_FStatus(EM_FN1_2) != AMP_CFS_STATUS_UNUSED)
                                AmbaKAL_TaskSleep(100);
                            while (AmpCFS_FStatus(EM_FN1_1) != AMP_CFS_STATUS_UNUSED)
                                AmbaKAL_TaskSleep(100);
                            AmpEditor_Merge(g_pEdtHdlr, FALSE, TRUE, FALSE, EM_FN1_2, EM_FN1_1);
                            break;
                        }
                        case 3: /** File 2-1 */
                        {
                            if ((g_nSplitCount - nEmFile2_1_Num) > 1) {
                                char file[MAX_FILENAME_LENGTH] = {0};
                                AmbaPrint("%s, File 2 Process", __FUNCTION__);
                                snprintf(file, MAX_FILENAME_LENGTH, "%s%02u.MP4", MP4_FN_PREFIX, nEmFile2_1_Num);
                                strncpy(szName1, file, MAX_FILENAME_LENGTH);
                                szName1[MAX_FILENAME_LENGTH - 1] = '\0';
                                while (AmpCFS_FStatus(szName1) != AMP_CFS_STATUS_UNUSED)
                                    AmbaKAL_TaskSleep(100);
                                if (g_pTmpStream->Func->Open(g_pTmpStream, szName1, AMP_STREAM_MODE_RDONLY) == AMP_OK) {
                                    if (g_pTmpStream->Func->Close(g_pTmpStream) != AMP_OK)
                                        AmbaPrint("%s, %u", __FUNCTION__, __LINE__);
                                    EmBusyFlag = TRUE;
                                    nTailDuration = EMERGENCY_TAIL_TIME;
                                    AmpEditor_Crop2New(g_pEdtHdlr, FALSE, 0, nTailDuration, TRUE, szName1, EM_FN2);
                                } else {
                                    AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
                                    break;
                                }
                            }
                            break;
                        }
                        case 4: /** Merge Em1 and Em2 */
                        {
                            EmBusyFlag = TRUE;
                            AmpEditor_Merge(g_pEdtHdlr, FALSE, TRUE, FALSE, EM_FN1, EM_FN2);
                            break;
                        }
                        default:
                        break;
                    }
                }
            }
        }
        AmbaKAL_TaskSleep(100);
    }
}

/**
 * Emergency UT - mux task.
 *
 * @param [in] info task information.
 *
 */
static void AmpUT_Em_MuxTask(UINT32 info)
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
    if (AmpTempIdx_Delete(g_pIndex) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        rval = -1;
        goto DONE;
    }
    g_pIndex = NULL;
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
    if (g_EmFifoHdlr != NULL) {
        AmpFifo_Delete(g_EmFifoHdlr);
        g_EmFifoHdlr = NULL;
    }
    if (g_EmEncFifoHdlr != NULL) {
        AmpFifo_Delete(g_EmEncFifoHdlr);
        g_EmEncFifoHdlr = NULL;
    }
DONE:
    if (rval == -1) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
    } else {
        AmbaPrint("%s Stop", __FUNCTION__);
        AmbaPrint("[SUCCESS] AmpUT_Em : End");
    }
}

/**
 * Emergency UT - FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_FifoCB(void *hdlr, UINT32 event, void* info)
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
 * Emergency UT - process callback function.
 *
 * @param [in] hdlr process handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_ProcessCB(void *hdlr, UINT32 event, void* info)
{
    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);
    if (event == AMP_EDITOR_EVENT_OK) {
        if (ProcessFileNo == 0) {
            ProcessFileNo++;
            if (EmNeedDuration == 0)
                ProcessFileNo+=2;
            EmBusyFlag = FALSE;
        } else if (ProcessFileNo == 1) {
            ProcessFileNo++;
            EmBusyFlag = FALSE;
        } else if (ProcessFileNo == 2) {
            ProcessFileNo++;
            if (AmpCFS_Move(EM_FN1_2, EM_FN1) == 0)
                EmBusyFlag = FALSE;
        } else if (ProcessFileNo == 3) {
            ProcessFileNo++;
            EmBusyFlag = FALSE;
        } else if (ProcessFileNo == 4) {
            if (AmpCFS_Move(EM_FN1, EM_OUT) == AMP_OK) {
                AmbaPrint("[SUCCESS] %s is closed", EM_OUT);
            } else {
                AmbaPrint("[FAIL] %s", __FUNCTION__);
            }
        }
    } else {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
    }
    return AMP_OK;
}

/**
 * Emergency UT - create movie name function.
 *
 * @param [out] szName movie name.
 * @param [in] nSize length of movie name.
 *
 */
static void AmpUT_Em_CreateMovName(char *szName, UINT32 nSize)
{
    if (g_nSplitCount == 0){
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
 * Emergency UT - reset movie information function.
 *
 * @param [in,out] pMovie movie information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_ResetMovie(AMP_MOVIE_INFO_s *pMovie)
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
 * Emergency UT - split file function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_AutoSplit(void)
{
    int rval = 0;
    AMP_MOVIE_INFO_s *pMovie;
    char szName[MAX_FILENAME_LENGTH] = {'\0'};

    if (g_pFormat->Func->Close(g_pFormat, AMP_MUX_FORMAT_CLOSE_DEFAULT) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    if (g_pStream->Func->Close(g_pStream) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);

    AmpUT_Em_CreateMovName(szName, MAX_FILENAME_LENGTH);
    // replace media info
    AmpFormat_NewMovieInfo(szName, &pMovie);
    AmpFormat_CopyMovieInfo(pMovie, g_pMovie);
    AmpFormat_RelMovieInfo(g_pMovie, TRUE);
    g_pMovie = pMovie;
    FormatLib_ResetMuxMediaInfo((AMP_MEDIA_INFO_s *)g_pMovie);
    if (AmpUT_Em_ResetMovie(pMovie) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    if (g_pStream->Func->Open(g_pStream, g_pMovie->Name, AMP_STREAM_MODE_WRONLY) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    g_pFormat->Media = (AMP_MEDIA_INFO_s *)g_pMovie;
    if (EmProcess) {
        AmpMuxer_SetMaxDuration(g_pMuxPipe, EMERGENCY_TAIL_TIME);
        bEmInit = TRUE;
        EmProcess = FALSE;
    } else {
        AmpMuxer_SetMaxDuration(g_pMuxPipe, SPLIT_TIME);
    }
    if (g_pFormat->Func->Open(g_pFormat) != AMP_OK)
        AmbaPrint("%s, %d", __FUNCTION__, __LINE__);
    return rval;
}

/**
 * Emergency UT - mux callback function.
 *
 * @param [in] hdlr muxer handler.
 * @param [in] event event id.
 * @param [in] info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_MuxCB(void* hdlr, UINT32 event, void* info)
{
    int rval = 0;
#if 0
    UINT32 i;
#endif
    switch (event) {
    case AMP_MUXER_EVENT_START:
        AmbaPrint("AMP_MUXER_EVENT_START");
        break;
    case AMP_MUXER_EVENT_END:
        AmbaPrint("AMP_MUXER_EVENT_END");
        DummyVideoEnc_Stop(&g_Encoder);
        break;
#if 0
    case AMP_MUXER_EVENT_PAUSE:
        AmbaPrint("AMP_MUXER_EVENT_PAUSE");
        break;
    case AMP_MUXER_EVENT_RESUME:
        AmbaPrint("AMP_MUXER_EVENT_RESUME");
        break;
#endif
    case AMP_MUXER_EVENT_REACH_LIMIT:
        AmbaPrint("AMP_MUXER_EVENT_REACH_LIMIT");
        rval = AmpUT_Em_AutoSplit();
        break;
#if 0
    case AMP_MUXER_EVENT_STOP_ON_PAUSED:
        AmbaPrint("AMP_MUXER_EVENT_STOP_ON_PAUSED");
        for (i=0; i<g_pMuxPipe->Format; i++) {
            AMP_MUX_FORMAT_HDLR_s * const pFmt = g_pMuxPipe->Format[i];
            if (pFmt->Func->Close(pFmt, TRUE) < 0) {
                AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
                rval = -1;
            }
        }
        break;
#endif
    default:
        break;
    }
    return rval;
}

/**
 * Emergency UT - init function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_init(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *pFmtBuf = NULL;
    AMP_MUXER_INIT_CFG_s muxerInitCfg;
    void *pMuxerBuf = NULL;
    AMP_MP4_MUX_INIT_CFG_s MP4MuxInitCfg;
    void *pMp4Buf = NULL;
    AMP_INDEX_INIT_CFG_s IndexInitCfg;
    void *pIdxBuf = NULL;
    AMP_TEMP_IDX_INIT_CFG_s TempInitCfg;
    void *pTempIdxBuf = NULL;
    AMP_FILE_STREAM_INIT_CFG_s fileInitCfg;
    void *pFileBuf;
    AMP_CFS_STAT stat;
    AMP_CFS_FILE_s *hUdtaFile;
    AMP_CFS_FILE_PARAM_s cfsParam;
    AMP_EDITOR_INIT_CFG_s EditorCfg = {0};
    void *pEditorBuf = NULL;
    void *EmRawBuf;
    void *pStkRawBuf1;
    void *pStkRawBuf2;
    UINT32 BufferSize = DummyVideoEnc_GetRequiredBufSize(EMERGENCY_ENC_RAW_SIZE, EMERGENCY_MAX_FIFO_NUM);

    // to make code clean, not to release the ring buffer
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_EmBuffer, &EmRawBuf, BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    DummyVideoEnc_Init(&g_Encoder, BS_FN, HDR_FN, g_EmBuffer, BufferSize, EMERGENCY_MAX_FIFO_NUM);

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
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&MP4MuxInitCfg.Buffer, &pMp4Buf, MP4MuxInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpMp4Mux_Init(&MP4MuxInitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

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

    /** Init Temp Index */
    AmpTempIdx_GetInitDefaultCfg(&TempInitCfg);
    TempInitCfg.MaxHdlr = 16;
    TempInitCfg.BufferSize = AmpTempIdx_GetRequiredBufferSize(TempInitCfg.MaxHdlr);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&TempInitCfg.Buffer, &pTempIdxBuf, TempInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpTempIdx_Init(&TempInitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    // init file stream
    AmpFileStream_GetInitDefaultCfg(&fileInitCfg);
    fileInitCfg.MaxHdlr = 16;
    fileInitCfg.BufferSize = AmpFileStream_GetRequiredBufferSize(fileInitCfg.MaxHdlr);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fileInitCfg.Buffer, &pFileBuf, fileInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFileStream_Init(&fileInitCfg);

    /** Init Editor */
    AmpEditor_GetInitDefaultCfg(&EditorCfg);
    EditorCfg.TaskInfo.Priority = 82;
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EditorCfg.Buffer, &pEditorBuf, EditorCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpEditor_Init(&EditorCfg);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_pEmStack, &pStkRawBuf1, STACK_SIZE, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    /** Create Em task */
    AmbaKAL_TaskCreate(&EmTask, /** pTask */
    "UT_Em_EmTask", /** pTaskName */
    82, /** Priority */
    AmpUT_Em_EmTask, /** void (*EntryFunction)(UINT32) */
    0x0, /** EntryArg */
    g_pEmStack, /** pStackBase */
    STACK_SIZE, /** StackByteSize */
    AMBA_KAL_DO_NOT_START); /** Not start */

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_pEmMuxStack, &pStkRawBuf2, STACK_SIZE, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);

    /** Create Mux task */
    AmbaKAL_TaskCreate(&g_EmMuxTask, /** pTask */
    "UT_Em_MuxTask", /** pTaskName */
    82, /** Priority */
    AmpUT_Em_MuxTask, /** void (*EntryFunction)(UINT32) */
    0x0, /** EntryArg */
    g_pEmMuxStack,  /** pStackBase */
    STACK_SIZE, /** StackByteSize */
    AMBA_KAL_DO_NOT_START); /** AutoStart */

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * Emergency UT - reset function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_Reset(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;

    RingBuf_Reset(&g_Encoder.RingBuf);
    g_nSplitCount = 0;
    // create codec fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = EMERGENCY_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = EMERGENCY_MAX_FIFO_NUM;
    fifoDefCfg.cbEvent = DummyVideoEnc_EventCB;
    AmpFifo_Create(&fifoDefCfg, &g_EmEncFifoHdlr);
    // create muxer fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = EMERGENCY_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = EMERGENCY_MAX_FIFO_NUM;
    fifoDefCfg.cbEvent = AmpUT_Em_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &g_EmFifoHdlr);
    DummyVideoEnc_Reset(&g_Encoder, g_EmEncFifoHdlr);
    // reset tasks
    AmbaKAL_TaskReset(&g_EmMuxTask);
    return 0;
}

/**
 * Emergency UT - start function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_start(void)
{
    AMP_MP4_MUX_CFG_s Mp4Cfg;
    AMP_TEMP_IDX_CFG_s TempIdxCfg;
    AMP_FILE_STREAM_CFG_s fileCfg;
    char szName[MAX_FILENAME_LENGTH] = {'\0'};
    AMP_MUX_MOVIE_INFO_CFG_s movieCfg;
    AMP_MUXER_PIPE_CFG_s MuxPipeCfg;
    AMP_MP4_EDT_INIT_CFG_s EdtInitCfg;
    AMP_MP4_EDT_CFG_s EdtCfg;
    void *Mp4EdtBuf = NULL;

    // open media info
    AmpUT_Em_Reset();
    AmpUT_Em_CreateMovName(szName, MAX_FILENAME_LENGTH);
    AmpFormat_NewMovieInfo(szName, &g_pMovie);

    // open file stream
    AmpFileStream_GetDefaultCfg(&fileCfg);
    AmpFileStream_Create(&fileCfg, &g_pStream);
    if (g_pStream->Func->Open(g_pStream, g_pMovie->Name, AMP_STREAM_MODE_WRONLY) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    AmpFileStream_GetDefaultCfg(&fileCfg);
    if (AmpFileStream_Create(&fileCfg, &g_pEmStream) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    AmpFileStream_GetDefaultCfg(&fileCfg);
    if (AmpFileStream_Create(&fileCfg, &g_pTmpStream) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // set media info
    AmpMuxer_GetDefaultMovieInfoCfg(&movieCfg);
    movieCfg.TrackCount = 1;
    movieCfg.Track[0].TrackType = AMP_MEDIA_TRACK_TYPE_VIDEO;
    movieCfg.Track[0].Fifo = g_EmFifoHdlr;
    movieCfg.Track[0].BufferBase = (UINT8 *)g_EmBuffer;
    movieCfg.Track[0].BufferLimit = (UINT8 *)g_EmBuffer + EMERGENCY_ENC_RAW_SIZE;
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
    /** Create Index */
    AmpTempIdx_GetDefaultCfg(&TempIdxCfg);
    if (AmpTempIdx_Create(&TempIdxCfg, &g_pIndex) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    AmpTempIdx_GetDefaultCfg(&TempIdxCfg);
    if (AmpTempIdx_Create(&TempIdxCfg, &g_pEmIndex) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    AmbaPrint("Mp4 Mux Create");
    AmpMp4Mux_GetDefaultCfg(&Mp4Cfg);
    Mp4Cfg.Stream = g_pStream;
    Mp4Cfg.Index = g_pIndex;

    if (AmpMp4Mux_Create(&Mp4Cfg, &g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    AmpMp4Edt_GetInitDefaultCfg(&EdtInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EdtInitCfg.Buffer, &Mp4EdtBuf, EdtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE)  != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpMp4Edt_Init(&EdtInitCfg);

    AmpMp4Edt_GetDefaultCfg(&EdtCfg);
    EdtCfg.Index = g_pEmIndex;
    EdtCfg.OnEvent = AmpUT_Em_ProcessCB;
    if (AmpMp4Edt_Create(&EdtCfg, &g_pEdtHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // create muxer pipe
    AmpMuxer_GetDefaultCfg(&MuxPipeCfg);
    MuxPipeCfg.FormatCount = 1;
    MuxPipeCfg.Format[0] = g_pFormat;
    MuxPipeCfg.Media[0] = (AMP_MEDIA_INFO_s *)g_pMovie;
    MuxPipeCfg.MaxDuration = SPLIT_TIME;
    MuxPipeCfg.OnEvent = AmpUT_Em_MuxCB;
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
    AmbaKAL_TaskResume(&g_EmMuxTask);
    AmbaKAL_TaskResume(&EmTask);

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
    return 0;
}

/**
 * Emergency UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_stop(void)
{
    AmpMuxer_Stop(g_pMuxPipe);
    return 0;
}

/**
 * Emergency UT - process function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_process(void)
{
    /** Split the file */
    AmpMuxer_SetMaxDuration(g_pMuxPipe, 0);
    if (g_nSplitCount > 0)
        nEmFile1_1_Num = g_nSplitCount - 1;
    EmProcess = TRUE;
    return 0;
}

/**
 * Emergency UT - test function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Em_test(void)
{
    return 0;
}

/**
 * Emergency UT - emergency test function.
 */
static int AmpUT_EmTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_Em_init();
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_Em_start();
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_Em_stop();
    } else if (strcmp(argv[1], "em") == 0) {
        AmpUT_Em_process();
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_Em_test();
    }
    return 0;
}

/**
 * Emergency UT - add emergency test function.
 */
int AmpUT_EmTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("em", AmpUT_EmTest);

    return AMP_OK;
}

