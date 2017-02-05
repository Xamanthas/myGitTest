/**
 *  @file AmpUT_Mp4Edt.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/09/26 |felix       |Created        |
 *
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <cfs/AmpCfs.h>
#include <format/Editor.h>
#include "AmpUnitTest.h"
#include "AmbaUtility.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaCache_Def.h>

#include <stream/File.h>

#include <format/Mp4Edt.h>
#include <index/Index.h>
#include <index/Temp.h>
#include <index/Raw.h>

#include <util.h>

//#define TEST_ABORT

#define EDT_FN1  "C:\\TEST.MP4" /**< edit file name 1 */
#define EDT_FN2  "C:\\TEST2.MP4" /**< edit file name 2 */
#define EDT_FN3  "C:\\TEST3.MP4" /**< edit file name 3 */
#ifdef TEST_ABORT
#define EDT_FN11  "C:\\TEST11.MP4" /**< edit file name 1 */
#define EDT_FN22  "C:\\TEST22.MP4" /**< edit file name 2 */
#define EDT_FN33  "C:\\TEST33.MP4" /**< edit file name 3 */
#endif
#define USE_TEMP_IDX /**< define index type */

static AMP_EDT_FORMAT_HDLR_s *g_EdtHdlr = NULL; /**< editor handler */
static AMP_INDEX_HDLR_s *g_Index = NULL; /**< index handler */
static AMP_STREAM_HDLR_s *g_FileStream = NULL; /**< file stream handler */
#ifdef TEST_ABORT
static AMP_EDT_FORMAT_HDLR_s *g_EdtHdlr2 = NULL; /**< editor handler */
static AMP_INDEX_HDLR_s *g_Index2 = NULL; /**< index handler */
static AMP_STREAM_HDLR_s *g_FileStream2 = NULL; /**< file stream handler */
#endif
static BOOL8 g_Abort = FALSE; /**< Abort flag */

extern AMBA_NAND_DEV_INFO_s AmbaNAND_DevInfo;

/**
 * Editor UT - process callback function.
 *
 * @param [in] pHdlr process handler.
 * @param [in] nEvent event id.
 * @param [in] pInfo callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Edt_ProcessCB(void *hdlr, UINT32 event, void* info)
{
    AmbaPrint("%s, hdlr = 0x%x, event = %u", __FUNCTION__, hdlr, event);
    if (event == AMP_EDITOR_EVENT_OK) {
         AmbaPrint("[SUCCESS] AmpUT_Mp4Edt : End");
     } else {
         AmbaPrint("[FAIL] %s", __FUNCTION__);
     }
    return AMP_OK;
}

/**
 * Editor UT - init function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Mp4Edt_Init(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *FmtBuf = NULL;
    AMP_MP4_EDT_INIT_CFG_s Mp4EdtInitCfg;
    void *Mp4EditorBuf = NULL;
    AMP_INDEX_INIT_CFG_s IndexBufCfg;
    void *IdxBuf = NULL;
#ifdef USE_TEMP_IDX
    AMP_TEMP_IDX_INIT_CFG_s tempInitCfg;
    AMP_TEMP_IDX_CFG_s tempCfg;
    void *TempIdxBuf = NULL;
#else
    AMP_RAW_IDX_INIT_CFG_s rawInitCfg;
    AMP_RAW_IDX_CFG_s rawCfg;
    void *RawIdxBuf = NULL;
#endif
    AMP_EDITOR_INIT_CFG_s EditorCfg = {0};
    void *EditorBuf = NULL;
    AMP_FILE_STREAM_INIT_CFG_s fileInitCfg;
    AMP_FILE_STREAM_CFG_s FileCfg;
    void *FileBuf = NULL;

    /** init format */
    AmpFormat_GetInitDefaultCfg(&fmtInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fmtInitCfg.Buffer, &FmtBuf, fmtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    AmpFormat_Init(&fmtInitCfg);

    /** Init Mp4 */
    AmpMp4Edt_GetInitDefaultCfg(&Mp4EdtInitCfg);
#ifdef TEST_ABORT
    Mp4EdtInitCfg.MaxHdlr = 2;
    Mp4EdtInitCfg.BufferSize = AmpMp4Edt_GetRequiredBufferSize(Mp4EdtInitCfg.MaxHdlr);
#endif
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&Mp4EdtInitCfg.Buffer, &Mp4EditorBuf, Mp4EdtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    AmpMp4Edt_Init(&Mp4EdtInitCfg);

    /** Create Index buffer */
    AmpIndex_GetInitDefaultCfg(&IndexBufCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&IndexBufCfg.Buffer, &IdxBuf, IndexBufCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    if (AmpIndex_Init(&IndexBufCfg) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
#ifdef USE_TEMP_IDX
    /** Init Temp Index */
    AmpTempIdx_GetInitDefaultCfg(&tempInitCfg);
    tempInitCfg.MaxHdlr = 8;
    tempInitCfg.BufferSize = AmpTempIdx_GetRequiredBufferSize(tempInitCfg.MaxHdlr);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&tempInitCfg.Buffer, &TempIdxBuf, tempInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    if (AmpTempIdx_Init(&tempInitCfg) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);

    /** Create Temp Index */
    AmpTempIdx_GetDefaultCfg(&tempCfg);
    if (AmpTempIdx_Create(&tempCfg, &g_Index) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
#ifdef TEST_ABORT
    AmpTempIdx_GetDefaultCfg(&tempCfg);
    if (AmpTempIdx_Create(&tempCfg, &g_Index2) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
#endif
#else
    /** Init Raw Index */
    AmpRawIdx_GetInitDefaultCfg(&rawInitCfg);
    memcpy(&rawInitCfg.NANDInfo, &AmbaNAND_DevInfo, sizeof(AMBA_NAND_DEV_INFO_s));
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&rawInitCfg.Buffer, &RawIdxBuf, rawInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    if (AmpRawIdx_Init(&rawInitCfg) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);

    /** Create Raw Index */
    AmpRawIdx_GetDefaultCfg(&rawCfg);
    if (AmpRawIdx_Create(&rawCfg, &g_Index) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
#ifdef TEST_ABORT
    AmpRawIdx_GetDefaultCfg(&rawCfg);
    if (AmpRawIdx_Create(&rawCfg, &g_Index2) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
#endif
#endif
    /** Initial File system */
    AmpFileStream_GetInitDefaultCfg(&fileInitCfg);
    fileInitCfg.MaxHdlr = 4;
    fileInitCfg.BufferSize = AmpFileStream_GetRequiredBufferSize(fileInitCfg.MaxHdlr);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fileInitCfg.Buffer, &FileBuf, fileInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    AmpFileStream_Init(&fileInitCfg);

    AmpFileStream_GetDefaultCfg(&FileCfg);
    FileCfg.Async = FALSE;
    AmpFileStream_Create(&FileCfg, &g_FileStream);
#ifdef TEST_ABORT
    AmpFileStream_GetDefaultCfg(&FileCfg);
    FileCfg.Async = FALSE;
    AmpFileStream_Create(&FileCfg, &g_FileStream2);
#endif
    /** Init Editor */
    AmpEditor_GetInitDefaultCfg(&EditorCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EditorCfg.Buffer, &EditorBuf, EditorCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    AmpEditor_Init(&EditorCfg);
    return 0;
}

/**
 * Editor UT - crop function.
 *
 * @param index index handler
 * @param StartTime crop start time
 * @param EndTime crop end time
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_Mp4Edt_Crop(AMP_INDEX_HDLR_s * index, UINT32 startTime, UINT32 endTime)
{
    if (g_EdtHdlr == NULL) {
        AMP_MP4_EDT_CFG_s EdtCfg;
        AmpMp4Edt_GetDefaultCfg(&EdtCfg);
        EdtCfg.Index = index;
        EdtCfg.OnEvent = AmpUT_Mp4Edt_ProcessCB;
        if (AmpMp4Edt_Create(&EdtCfg, &g_EdtHdlr) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    return AmpEditor_Crop2New(g_EdtHdlr, FALSE, startTime, endTime, TRUE, EDT_FN1, EDT_FN2);
}

/**
 * Editor UT - divide function.
 *
 * @param index index handler
 * @param targetTime divide target time
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_Mp4Edt_Divide(AMP_INDEX_HDLR_s * index, UINT32 targetTime)
{
    if (g_EdtHdlr == NULL) {
        AMP_MP4_EDT_CFG_s EdtCfg;
        AmpMp4Edt_GetDefaultCfg(&EdtCfg);
        EdtCfg.Index = index;
        EdtCfg.OnEvent = AmpUT_Mp4Edt_ProcessCB;
        if (AmpMp4Edt_Create(&EdtCfg, &g_EdtHdlr) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    return AmpEditor_Divide(g_EdtHdlr, FALSE, targetTime, TRUE, AMP_EDITOR_ROUND_OFF, EDT_FN1, EDT_FN2);
}

/**
 * Editor UT - merge function.
 *
 * @param index index handler
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_Mp4Edt_Merge(AMP_INDEX_HDLR_s * index)
{
    if (g_EdtHdlr == NULL) {
        AMP_MP4_EDT_CFG_s EdtCfg;
        AmpMp4Edt_GetDefaultCfg(&EdtCfg);
        EdtCfg.Index = index;
        EdtCfg.OnEvent = AmpUT_Mp4Edt_ProcessCB;
        if (AmpMp4Edt_Create(&EdtCfg, &g_EdtHdlr) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    return AmpEditor_Merge(g_EdtHdlr, FALSE, TRUE, FALSE, EDT_FN1, EDT_FN2);
}

/**
 * Editor UT - partial delete function.
 *
 * @param index index handler
 * @param StartTime start time
 * @param EndTime end time
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_Mp4Edt_PartialDelete(AMP_INDEX_HDLR_s * index, UINT32 startTime, UINT32 endTime)
{
    if (g_EdtHdlr == NULL) {
        AMP_MP4_EDT_CFG_s EdtCfg;
        AmpMp4Edt_GetDefaultCfg(&EdtCfg);
        EdtCfg.Index = index;
        EdtCfg.OnEvent = AmpUT_Mp4Edt_ProcessCB;
        if (AmpMp4Edt_Create(&EdtCfg, &g_EdtHdlr) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    return AmpEditor_PartialDelete(g_EdtHdlr, FALSE, startTime, endTime, TRUE, EDT_FN1, EDT_FN1);
}

/**
 * Editor UT - recover function.
 *
 * @param index index handler
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_Mp4Edt_Recover(AMP_INDEX_HDLR_s * index)
{
    if (g_EdtHdlr == NULL) {
        AMP_MP4_EDT_CFG_s EdtCfg;
        AmpMp4Edt_GetDefaultCfg(&EdtCfg);
        EdtCfg.Index = index;
        EdtCfg.OnEvent = AmpUT_Mp4Edt_ProcessCB;
        if (AmpMp4Edt_Create(&EdtCfg, &g_EdtHdlr) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    return AmpEditor_Recover(g_EdtHdlr, FALSE, EDT_FN1);
}

/**
 * Editor UT - erase function.
 *
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_Mp4Edt_erase(void)
{
#ifdef USE_TEMP_IDX

#else
    g_Index->Func->Func(AMP_RAW_IDX_OP_ERASE_ALL, 0, 0);
#endif
    return 0;
}

/**
 * Editor UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_Mp4Edt_stop(void)
{
    return 0;
}

/**
 * Editor UT - abort function.
 *
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_Mp4Edt_abort(void)
{
    AmbaPrint("%s", __FUNCTION__);
    AmpEditor_Abort(g_EdtHdlr);
    return 0;
}

/**
 * Editor UT - test function.
 *
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_Mp4Edt_test(void)
{
    //for test
    AMP_MP4_EDT_CFG_s EdtCfg;
    AmpMp4Edt_GetDefaultCfg(&EdtCfg);
    EdtCfg.Index = g_Index;
    EdtCfg.OnEvent = AmpUT_Mp4Edt_ProcessCB;
    AmpMp4Edt_Create(&EdtCfg, &g_EdtHdlr);
#ifdef TEST_ABORT
    AmpMp4Edt_GetDefaultCfg(&EdtCfg);
    EdtCfg.Index = g_Index2;
    EdtCfg.OnEvent = AmpUT_Mp4Edt_ProcessCB;
    AmpMp4Edt_Create(&EdtCfg, &g_EdtHdlr2);
#endif
    if (g_EdtHdlr == NULL) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    } else {
        if (!g_Abort)
            AmpEditor_Crop2New(g_EdtHdlr, FALSE, 1000, 3000, TRUE, EDT_FN1, EDT_FN2);
#ifdef TEST_ABORT
        if (!g_Abort)
            AmpEditor_Crop2New(g_EdtHdlr2, FALSE, 1000, 3000, TRUE, EDT_FN11, EDT_FN22);
#endif
        if (!g_Abort)
            AmpEditor_Merge(g_EdtHdlr, FALSE, TRUE, FALSE, EDT_FN1, EDT_FN2);
#ifdef TEST_ABORT
        if (!g_Abort)
            AmpEditor_Merge(g_EdtHdlr2, FALSE, TRUE, FALSE, EDT_FN11, EDT_FN22);
#endif
        if (!g_Abort)
            AmpEditor_Crop2New(g_EdtHdlr, FALSE, 1000, 3000, TRUE, EDT_FN1, EDT_FN2);
#ifdef TEST_ABORT
        if (!g_Abort)
            AmpEditor_Crop2New(g_EdtHdlr2, FALSE, 1000, 3000, TRUE, EDT_FN11, EDT_FN22);
#endif
        if (!g_Abort)
            AmpEditor_Merge(g_EdtHdlr, FALSE, TRUE, FALSE, EDT_FN1, EDT_FN2);
#ifdef TEST_ABORT
        if (!g_Abort)
            AmpEditor_Merge(g_EdtHdlr2, FALSE, TRUE, FALSE, EDT_FN11, EDT_FN22);
#endif
    }
    return 0;
}

/**
 * Editor UT - editor test function.
 */
static int AmpUT_Mp4EdtTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_Mp4Edt_Init();
    } else if (strcmp(argv[1], "crop") == 0) {
        if (argc == 4) {
            UINT32 StartTime;
            UINT32 EndTime;
            StartTime = atoi(argv[2]);
            EndTime = atoi(argv[3]);
            AmpUT_Mp4Edt_Crop(g_Index, StartTime, EndTime);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            return -1;
        }
    } else if (strcmp(argv[1], "merge") == 0) {
        AmpUT_Mp4Edt_Merge(g_Index);
    } else if (strcmp(argv[1], "divide") == 0) {
        if (argc == 3) {
            UINT32 TargetTime;
            TargetTime = atoi(argv[2]);
            AmpUT_Mp4Edt_Divide(g_Index, TargetTime);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            return -1;
        }
    } else if (strcmp(argv[1], "pd") == 0) {
        if (argc == 4) {
            UINT32 StartTime;
            UINT32 EndTime;
            StartTime = atoi(argv[2]);
            EndTime = atoi(argv[3]);
            AmpUT_Mp4Edt_PartialDelete(g_Index, StartTime, EndTime);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            return -1;
        }
    } else if (strcmp(argv[1], "recover") == 0) {
        AmpUT_Mp4Edt_Recover(g_Index);
    } else if (strcmp(argv[1], "erase") == 0) {
        AmpUT_Mp4Edt_erase();
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_Mp4Edt_stop();
    } else if (strcmp(argv[1], "abort") == 0) {
        AmpUT_Mp4Edt_abort();
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_Mp4Edt_test();
    }
    return 0;
}

/**
 * Editor UT - add editor test function.
 */
int AmpUT_Mp4EdtTestAdd(void)
{
    AmbaTest_RegisterCommand("mp4edt", AmpUT_Mp4EdtTest);
    return AMP_OK;
}

