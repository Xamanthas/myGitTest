/**
 *  @file AmpUT_Editor.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2015/01/29 |felix       |Created        |
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
#include <format/MovEdt.h>
#include <format/MkvEdt.h>
#include <index/Index.h>
#include <index/Temp.h>
#include <index/Raw.h>

#include <util.h>

//#define TEST_ABORT
#define USE_TEMP_IDX /**< define index type */

static AMP_EDT_FORMAT_HDLR_s *g_EdtHdlr = NULL; /**< editor handler */
static AMP_INDEX_HDLR_s *g_Index = NULL; /**< index handler */
static AMP_STREAM_HDLR_s *g_FileStream = NULL; /**< file stream handler */
#ifdef TEST_ABORT
static AMP_EDT_FORMAT_HDLR_s *g_EdtHdlr2 = NULL; /**< editor handler */
static AMP_INDEX_HDLR_s *g_Index2 = NULL; /**< index handler */
static AMP_STREAM_HDLR_s *g_FileStream2 = NULL; /**< file stream handler */
#endif

extern int AmpUT_Mp4Edt_Crop(AMP_INDEX_HDLR_s * index, UINT32 startTime, UINT32 endTime);
extern int AmpUT_MovEdt_Crop(AMP_INDEX_HDLR_s * index, UINT32 startTime, UINT32 endTime);
extern int AmpUT_Mp4Edt_Divide(AMP_INDEX_HDLR_s * index, UINT32 targetTime);
extern int AmpUT_MovEdt_Divide(AMP_INDEX_HDLR_s * index, UINT32 targetTime);
extern int AmpUT_Mp4Edt_Merge(AMP_INDEX_HDLR_s * index);
extern int AmpUT_MovEdt_Merge(AMP_INDEX_HDLR_s * index);
extern int AmpUT_Mp4Edt_PartialDelete(AMP_INDEX_HDLR_s * index, UINT32 startTime, UINT32 endTime);
extern int AmpUT_MovEdt_PartialDelete(AMP_INDEX_HDLR_s * index, UINT32 startTime, UINT32 endTime);
extern int AmpUT_Mp4Edt_Recover(AMP_INDEX_HDLR_s * index);
extern int AmpUT_MovEdt_Recover(AMP_INDEX_HDLR_s * index);
extern int AmpUT_MkvEdt_Recover(void);

static int AmpUT_Editor_CheckFileType(void)
{
    int FileType = -1;
    AMP_CFS_STAT FileStat;
    if (AmpCFS_Stat("C:\\TEST.MP4", &FileStat) == AMP_OK) {
        return 1;
    }
    if (AmpCFS_Stat("C:\\TEST.MOV", &FileStat) == AMP_OK) {
        return 2;
    }
    if (AmpCFS_Stat("C:\\TEST.MKV", &FileStat) == AMP_OK) {
        return 3;
    }
    return FileType;
}

/**
 * Editor UT - init function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Editor_Init(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *FmtBuf = NULL;
    AMP_MP4_EDT_INIT_CFG_s Mp4EdtInitCfg;
    void *Mp4EditorBuf = NULL;
    AMP_MOV_EDT_INIT_CFG_s MovEdtInitCfg;
    void *MovEditorBuf = NULL;
    AMP_MKV_EDT_INIT_CFG_s MkvEdtInitCfg;
    void *MkvEditorBuf = NULL;
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
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fmtInitCfg.Buffer, &FmtBuf, fmtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFormat_Init(&fmtInitCfg);

    /** Init Mp4 */
    AmpMp4Edt_GetInitDefaultCfg(&Mp4EdtInitCfg);
#ifdef TEST_ABORT
    Mp4EdtInitCfg.MaxHdlr = 2;
    Mp4EdtInitCfg.BufferSize = AmpMp4Edt_GetRequiredBufferSize(Mp4EdtInitCfg.MaxHdlr);
#endif
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&Mp4EdtInitCfg.Buffer, &Mp4EditorBuf, Mp4EdtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpMp4Edt_Init(&Mp4EdtInitCfg);

    /** Init Mov */
    AmpMovEdt_GetInitDefaultCfg(&MovEdtInitCfg);
#ifdef TEST_ABORT
    MovEdtInitCfg.MaxHdlr = 2;
    MovEdtInitCfg.BufferSize = AmpMovEdt_GetRequiredBufferSize(MovEdtInitCfg.MaxHdlr);
#endif
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&MovEdtInitCfg.Buffer, &MovEditorBuf, MovEdtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpMovEdt_Init(&MovEdtInitCfg);

    /** Init Mkv */
    AmpMkvEdt_GetInitDefaultCfg(&MkvEdtInitCfg);
#ifdef TEST_ABORT
    MkvEdtInitCfg.MaxHdlr = 2;
    MkvEdtInitCfg.BufferSize = AmpMkvEdt_GetRequiredBufferSize(MkvEdtInitCfg.MaxHdlr);
#endif
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&MkvEdtInitCfg.Buffer, &MkvEditorBuf, MkvEdtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpMkvEdt_Init(&MkvEdtInitCfg);

    /** Create Index buffer */
    AmpIndex_GetInitDefaultCfg(&IndexBufCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&IndexBufCfg.Buffer, &IdxBuf, IndexBufCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpIndex_Init(&IndexBufCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#ifdef USE_TEMP_IDX
    /** Init Temp Index */
    AmpTempIdx_GetInitDefaultCfg(&tempInitCfg);
    tempInitCfg.MaxHdlr = 8;
    tempInitCfg.BufferSize = AmpTempIdx_GetRequiredBufferSize(tempInitCfg.MaxHdlr);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&tempInitCfg.Buffer, &TempIdxBuf, tempInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpTempIdx_Init(&tempInitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    /** Create Temp Index */
    AmpTempIdx_GetDefaultCfg(&tempCfg);
    if (AmpTempIdx_Create(&tempCfg, &g_Index) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#ifdef TEST_ABORT
    AmpTempIdx_GetDefaultCfg(&tempCfg);
    if (AmpTempIdx_Create(&tempCfg, &g_Index2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#endif
#else
    /** Init Raw Index */
    AmpRawIdx_GetInitDefaultCfg(&rawInitCfg);
    extern AMBA_NAND_DEV_INFO_s AmbaNAND_DevInfo;
    memcpy(&rawInitCfg.NANDInfo, &AmbaNAND_DevInfo, sizeof(AMBA_NAND_DEV_INFO_s));
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&rawInitCfg.Buffer, &RawIdxBuf, rawInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpRawIdx_Init(&rawInitCfg) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    /** Create Raw Index */
    AmpRawIdx_GetDefaultCfg(&rawCfg);
    if (AmpRawIdx_Create(&rawCfg, &g_Index) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#ifdef TEST_ABORT
    AmpRawIdx_GetDefaultCfg(&rawCfg);
    if (AmpRawIdx_Create(&rawCfg, &g_Index2) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
#endif
#endif
    /** Initial File system */
    AmpFileStream_GetInitDefaultCfg(&fileInitCfg);
    fileInitCfg.MaxHdlr = 4;
    fileInitCfg.BufferSize = AmpFileStream_GetRequiredBufferSize(fileInitCfg.MaxHdlr);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fileInitCfg.Buffer, &FileBuf, fileInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
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
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EditorCfg.Buffer, &EditorBuf, EditorCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpEditor_Init(&EditorCfg);

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * Editor UT - crop function.
 *
 * @param StartTime crop start time
 * @param EndTime crop end time
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Editor_Crop(UINT32 startTime, UINT32 endTime)
{
    int FileType;
    int rval = 0;
    FileType = AmpUT_Editor_CheckFileType();
    if (FileType > 0) {
        if (FileType == 1) {
            rval = AmpUT_Mp4Edt_Crop(g_Index, startTime, endTime);
        } else if (FileType == 2) {
            rval = AmpUT_MovEdt_Crop(g_Index, startTime, endTime);
        } else {
            AmbaPrint("Unsupported!");
        }
    } else {
        AmbaPrint("Wrong file type!");
    }
    if (rval < 0) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
        return -1;
    }
    return 0;
}

/**
 * Editor UT - divide function.
 *
 * @param targetTime divide target time
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Editor_Divide(UINT32 targetTime)
{
    int FileType;
    int rval = 0;
    FileType = AmpUT_Editor_CheckFileType();
    if (FileType > 0) {
        if (FileType == 1) {
            rval = AmpUT_Mp4Edt_Divide(g_Index, targetTime);
        } else if (FileType == 2) {
            rval = AmpUT_MovEdt_Divide(g_Index, targetTime);
        } else {
            AmbaPrint("Unsupported!");
        }
    } else {
        AmbaPrint("Wrong file type!");
    }
    if (rval < 0) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
        return -1;
    }
    return 0;
}

/**
 * Editor UT - merge function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Editor_Merge(void)
{
    int FileType;
    int rval = 0;
    FileType = AmpUT_Editor_CheckFileType();
    if (FileType > 0) {
        if (FileType == 1) {
            rval = AmpUT_Mp4Edt_Merge(g_Index);
        } else if (FileType == 2) {
            rval = AmpUT_MovEdt_Merge(g_Index);
        } else {
            AmbaPrint("Unsupported!");
        }
    } else {
        AmbaPrint("Wrong file type!");
    }
    if (rval < 0) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
        return -1;
    }
    return 0;
}

/**
 * Editor UT - partial delete function.
 *
 * @param StartTime start time
 * @param EndTime end time
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Editor_PartialDelete(UINT32 startTime, UINT32 endTime)
{
    int FileType;
    int rval = 0;
    FileType = AmpUT_Editor_CheckFileType();
    if (FileType > 0) {
        if (FileType == 1) {
            rval = AmpUT_Mp4Edt_PartialDelete(g_Index, startTime, endTime);
        } else if (FileType == 2) {
            rval = AmpUT_MovEdt_PartialDelete(g_Index, startTime, endTime);
        } else {
            AmbaPrint("Unsupported!");
        }
    } else {
        AmbaPrint("Wrong file type!");
    }
    if (rval < 0) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
        return -1;
    }
    return 0;
}

/**
 * Editor UT - recover function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Editor_Recover(void)
{
    int FileType;
    int rval = 0;
    FileType = AmpUT_Editor_CheckFileType();
    if (FileType > 0) {
        if (FileType == 1) {
            rval= AmpUT_Mp4Edt_Recover(g_Index);
        } else if (FileType == 2) {
            rval = AmpUT_MovEdt_Recover(g_Index);
        } else if (FileType == 3) {
            rval = AmpUT_MkvEdt_Recover();
        } else {
            AmbaPrint("Unsupported!");
        }
    } else {
        AmbaPrint("Wrong file type!");
    }
    if (rval < 0) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
        return -1;
    }
    return 0;
}

/**
 * Editor UT - erase function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Editor_erase(void)
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
static int AmpUT_Editor_stop(void)
{
    return 0;
}

/**
 * Editor UT - abort function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Editor_abort(void)
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
static int AmpUT_Editor_test(void)
{
    return 0;
}

/**
 * Editor UT - editor test function.
 */
static int AmpUT_EditorTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_Editor_Init();
    } else if (strcmp(argv[1], "crop") == 0) {
        if (argc == 4) {
            UINT32 StartTime;
            UINT32 EndTime;
            StartTime = atoi(argv[2]);
            EndTime = atoi(argv[3]);
            AmpUT_Editor_Crop(StartTime, EndTime);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            return -1;
        }
    } else if (strcmp(argv[1], "merge") == 0) {
        AmpUT_Editor_Merge();
    } else if (strcmp(argv[1], "divide") == 0) {
        if (argc == 3) {
            UINT32 TargetTime;
            TargetTime = atoi(argv[2]);
            AmpUT_Editor_Divide(TargetTime);
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
            AmpUT_Editor_PartialDelete(StartTime, EndTime);
        } else {
            AmbaPrint("\t Wrong Parameters!");
            AmbaPrint("");
            return -1;
        }
    } else if (strcmp(argv[1], "recover") == 0) {
        AmpUT_Editor_Recover();
    } else if (strcmp(argv[1], "erase") == 0) {
        AmpUT_Editor_erase();
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_Editor_stop();
    } else if (strcmp(argv[1], "abort") == 0) {
        AmpUT_Editor_abort();
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_Editor_test();
    }
    return 0;
}

/**
 * Editor UT - add editor test function.
 */
int AmpUT_EditorTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("edt", AmpUT_EditorTest);

    return AMP_OK;
}


