/**
 *  @file AmpUT_MkvEdt.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2015/01/06 |clchan      |Created        |
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

#include <format/MkvEdt.h>
#include <index/Index.h>

#include <util.h>

#define EDT_FN1  "C:\\TEST.MKV" /**< edit file name 1 */

static AMP_EDT_FORMAT_HDLR_s *g_EdtHdlr = NULL; /**< editor handler */
static AMP_STREAM_HDLR_s *g_FileStream = NULL; /**< file stream handler */

/**
 * Editor UT - process callback function.
 *
 * @param [in] pHdlr process handler.
 * @param [in] nEvent event id.
 * @param [in] pInfo callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_MkvEdt_ProcessCB(void *hdlr, UINT32 event, void* info)
{
    AmbaPrint("%s, hdlr = 0x%x, event = %u", __FUNCTION__, hdlr, event);
    if (event == AMP_EDITOR_EVENT_OK) {
        AmbaPrint("[SUCCESS] AmpUT_MkvEdt : End");
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
static int AmpUT_MkvEdt_Init(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *FmtBuf = NULL;
    AMP_MKV_EDT_INIT_CFG_s MkvEdtInitCfg;
    void *Mp4EditorBuf = NULL;
    AMP_INDEX_INIT_CFG_s IndexBufCfg;
    void *IdxBuf = NULL;
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

    /** Init Mkv */
    AmpMkvEdt_GetInitDefaultCfg(&MkvEdtInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&MkvEdtInitCfg.Buffer, &Mp4EditorBuf, MkvEdtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    AmpMkvEdt_Init(&MkvEdtInitCfg);

    /** Create Index buffer */
    AmpIndex_GetInitDefaultCfg(&IndexBufCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&IndexBufCfg.Buffer, &IdxBuf, IndexBufCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    if (AmpIndex_Init(&IndexBufCfg) != AMP_OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
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
    /** Init Editor */
    AmpEditor_GetInitDefaultCfg(&EditorCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&EditorCfg.Buffer, &EditorBuf, EditorCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK)
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    AmpEditor_Init(&EditorCfg);
    return 0;
}

/**
 * Editor UT - recover function.
 *
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_MkvEdt_Recover(void)
{
    if (g_EdtHdlr == NULL) {
        AMP_MKV_EDT_CFG_s EdtCfg;
        AmpMkvEdt_GetDefaultCfg(&EdtCfg);
        EdtCfg.OnEvent = AmpUT_MkvEdt_ProcessCB;
        if (AmpMkvEdt_Create(&EdtCfg, &g_EdtHdlr) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    return  AmpEditor_Recover(g_EdtHdlr, FALSE, EDT_FN1);
}

/**
 * Editor UT - abort function.
 *
 * @return 0 - OK, others - fail
 *
 */
int AmpUT_MkvEdt_abort(void)
{
    AmbaPrint("%s", __FUNCTION__);
    AmpEditor_Abort(g_EdtHdlr);
    return 0;
}

/**
 * Editor UT - editor test function.
 */
static int AmpUT_MkvEdtTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_MkvEdt_Init();
    } else if (strcmp(argv[1], "recover") == 0) {
        AmpUT_MkvEdt_Recover();
    } else if (strcmp(argv[1], "abort") == 0) {
        AmpUT_MkvEdt_abort();
    }
    return 0;
}

/**
 * Editor UT - add editor test function.
 */
int AmpUT_MkvEdtTestAdd(void)
{
    AmbaTest_RegisterCommand("mkvedt", AmpUT_MkvEdtTest);
    return AMP_OK;
}

