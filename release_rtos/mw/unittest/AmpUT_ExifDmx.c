/**
 *  @file AmpUT_ExifDmx.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/06/27 |clchan      |Created        |
 *
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <player/Decode.h>
#include <player/StillDec.h>
#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <format/Demuxer.h>
#include <stream/File.h>
#include <format/ExtDmx.h>
#include <format/ExifDmx.h>
#include <util.h>
#include <AmbaCache_Def.h>

static AMP_STREAM_HDLR_s *g_pStream = NULL; /**< stream handler */
static AMP_DMX_FORMAT_HDLR_s *g_pFormat = NULL; /**< format handler */
static AMP_IMAGE_INFO_s *g_pImage = NULL; /**< image information */
static AMP_DEMUXER_PIPE_HDLR_s *g_pDmxPipe = NULL; /**< demuxer pipe */

#define JPG_FN  "C:\\TEST.JPG" /**< file name of JPG */
#define DEC_FN  "C:\\DEC.JPG" /**< decode file name */

#define DUMMY_CODEC_HDLR ((void *)0x1234) /**< coder handler */

#define STLDEC_RAW_SIZE (5<<20) /**< 5MB raw buffer */

static void* stlRawBuf = NULL; /**< raw buffer */
static void* stlRawBufOri = NULL; /**< raw buffer ori */
static AMP_FIFO_HDLR_s *DecFifoHdlr = NULL; /**< decoder FIFO handler */
static AMP_FIFO_HDLR_s *DmxFifoHdlr = NULL; /**< demuxer FIFO handler */

/**
 * Exif Dmx UT - decoder FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifDmx_DecFifoCB(void *hdlr, UINT32 event, void* info)
{
    AMP_BITS_DESC_s * const tmpDesc = (AMP_BITS_DESC_s *)info;
    if (event == AMP_FIFO_EVENT_DATA_READY) {

    } else if (event == AMP_FIFO_EVENT_GET_WRITE_POINT) {
        // provide write space
        tmpDesc->Size = STLDEC_RAW_SIZE;
        tmpDesc->StartAddr = (UINT8 *)stlRawBuf;
    } else if (event == AMP_FIFO_EVENT_RESET_FIFO) {

    }
    return 0;
}

/**
 * Exif Dmx UT - demuxer FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifDmx_DmxFifoCB(void *hdlr, UINT32 event, void* info)
{
    return 0;
}

/**
 * Exif Dmx UT - demuxer callback function.
 *
 * @param [in] hdlr demuxer handler.
 * @param [in] event event id.
 * @param [in] info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifDmx_DmxCB(void* hdlr, UINT32 event, void* info)
{
    switch (event) {
    case AMP_DEMUXER_EVENT_START:
        AmbaPrint("AMP_DEMUXER_EVENT_START");
        break;
    case AMP_DEMUXER_EVENT_END:
        AmbaPrint("AMP_DEMUXER_EVENT_END");
        break;
    default:
        break;
    }
    return 0;
}

/**
 * Exif Dmx UT - open function.
 *
 * @param [in] szName file name.
 * @param [in] argv parameter.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifDmx_DmxOpen(char *szName, char **argv)
{
    //void *pTmpRawBuf = NULL;
    AMP_FIFO_CFG_s fifoDefCfg;
    AMP_FILE_STREAM_CFG_s fileCfg;
    AMP_DEMUXER_PIPE_CFG_s dmxCfg;
    AMP_EXIF_DMX_CFG_s exifCfg;
    AMP_DMX_IMAGE_INFO_CFG_s imgCfg;

    AmbaPrint("%s: %s Opened!", __FUNCTION__, szName);
    // alloc memory
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&stlRawBuf, &stlRawBufOri, STLDEC_RAW_SIZE, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // create codec fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = DUMMY_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = 64;
    fifoDefCfg.cbEvent = AmpUT_ExifDmx_DecFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &DecFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    /// create fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = DUMMY_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = 64;
    fifoDefCfg.cbEvent = AmpUT_ExifDmx_DmxFifoCB;
    if (AmpFifo_Create(&fifoDefCfg, &DmxFifoHdlr) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    // open file stream
    AmpFileStream_GetDefaultCfg(&fileCfg);
    AmpFileStream_Create(&fileCfg, &g_pStream);
    if (g_pStream->Func->Open(g_pStream, szName, AMP_STREAM_MODE_RDONLY) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    // open media info (exifdmx)
    if (AmpFormat_GetImageInfo(szName, AmpExifDmx_Parse, g_pStream, &g_pImage) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpDemuxer_GetDefaultImageInfoCfg(&imgCfg, g_pImage);
    imgCfg.Fifo = DmxFifoHdlr;
    imgCfg.BufferBase = stlRawBuf;
    imgCfg.BufferLimit = (UINT8 *)stlRawBuf + STLDEC_RAW_SIZE;
    AmpDemuxer_InitImageInfo(g_pImage, &imgCfg);

    // open exifdmx
    AmpExifDmx_GetDefaultCfg(&exifCfg);
    exifCfg.Stream = g_pStream;
    if (AmpExifDmx_Create(&exifCfg, &g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    // create demuxer pipe
    AmpDemuxer_GetDefaultCfg(&dmxCfg);
    dmxCfg.FormatCount = 1;
    dmxCfg.Format[0] = g_pFormat;
    dmxCfg.Media[0] = (AMP_MEDIA_INFO_s *)g_pImage;
    dmxCfg.OnEvent = AmpUT_ExifDmx_DmxCB;
    dmxCfg.ProcParam = atoi(argv[2]);  // feed the n frame

    AmbaPrint("%s: Feed the %u frame", __FUNCTION__, dmxCfg.ProcParam);
    dmxCfg.TaskMode = AMP_DEMUXER_TASK_MODE_CALLER;    // for the codec that does not send under threshold, e.g., still decoder
    if (AmpDemuxer_Create(&dmxCfg, &g_pDmxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpDemuxer_Add(g_pDmxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * Exif Dmx UT - close function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifDmx_DmxClose(void)
{
    if (g_pDmxPipe != NULL) {
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
        if (AmpExifDmx_Delete(g_pFormat) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
        g_pFormat = NULL;
    }
    if (g_pStream != NULL) {
        if (g_pStream->Func->Close(g_pStream) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
        if (AmpFileStream_Delete(g_pStream) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
        g_pStream = NULL;
    }
    if (g_pImage != NULL) {
        AmpFormat_RelImageInfo(g_pImage, FALSE);
        g_pImage = NULL;
    }

    if (DmxFifoHdlr != NULL) {
        if (AmpFifo_Delete(DmxFifoHdlr) != AMP_OK) {
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
            return -1;
        }
        DmxFifoHdlr = NULL;
    }
    if (DecFifoHdlr != NULL) {
       if (AmpFifo_Delete(DecFifoHdlr) != AMP_OK) {
           AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
           return -1;
       }
       DecFifoHdlr = NULL;
    }
    if (stlRawBufOri != NULL) {
        AmbaKAL_BytePoolFree(stlRawBufOri);
        stlRawBuf = NULL;
        stlRawBufOri = NULL;
    }
    return 0;
}

/**
 * Exif Dmx UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifDmx_init(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *pFmtBuf;
    AMP_DEMUXER_INIT_CFG_s demuxerInitCfg;
    void *pDemuxerBuf;
    AMP_EXIF_DMX_INIT_CFG_s exifInitCfg;
    void *pExifinitBuf;
    AMP_FILE_STREAM_INIT_CFG_s fileInitCfg;
    void *pFileBuf;

    // init format
    AmpFormat_GetInitDefaultCfg(&fmtInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fmtInitCfg.Buffer, &pFmtBuf, fmtInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFormat_Init(&fmtInitCfg);
    // init demuxer
    AmpDemuxer_GetInitDefaultCfg(&demuxerInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&demuxerInitCfg.Buffer, &pDemuxerBuf, demuxerInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpDemuxer_Init(&demuxerInitCfg);
    // init exif demuxer
    AmpExifDmx_GetInitDefaultCfg(&exifInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&exifInitCfg.Buffer, &pExifinitBuf, exifInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpExifDmx_Init(&exifInitCfg);
    // init file stream
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
 * Exif Dmx UT - feed function.
 *
 * @param [in] argc feed frame parameter
 * @param [in] argv frame buffer
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifDmx_feed(int argc, char **argv)
{
    //Open ExifDmx
    if (AmpUT_ExifDmx_DmxOpen(JPG_FN, argv) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpDemuxer_Start(g_pDmxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpDemuxer_OnDataRequest(DmxFifoHdlr);

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * Exif Dmx UT - decode function.
 *
 * @param [in] argc frame parameter
 * @param [in] argv frame buffer
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifDmx_dec(int argc, char **argv)
{
    AMP_CFS_FILE_s *pFile;
    AMP_CFS_FILE_PARAM_s cfsParam;
    AMP_BITS_DESC_s *pFrame = NULL;
    UINT64 Len;
    AmbaPrint("%s: DecFifoHdlr = 0x%x", __FUNCTION__, DecFifoHdlr);
    if (AmpFifo_PeekEntry(DecFifoHdlr, &pFrame, 0) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpCFS_GetFileParam(&cfsParam);
    cfsParam.Mode = AMP_CFS_FILE_MODE_WRITE_ONLY;
    strcpy(cfsParam.Filename, DEC_FN);
    pFile = AmpCFS_fopen(&cfsParam);
    if (pFile == NULL) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    Len = AmpCFS_fwrite(pFrame->StartAddr, 1, pFrame->Size, pFile);
    if (Len != pFrame->Size) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpCFS_fclose(pFile);

    if (AmpFifo_RemoveEntry(DecFifoHdlr, 1) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    if (AmpUT_ExifDmx_DmxClose() != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * Exif Dmx UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifDmx_stop(void)
{
    //AmpUT_ExifDmx_DmxClose();
    return 0;
}

/**
 * Exif Dmx UT - test function.
 */
static int AmpUT_ExifDmxTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    AmbaKAL_TaskSleep(5);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_ExifDmx_init();
    } else if (strcmp(argv[1], "feed") == 0) {
        AmpUT_ExifDmx_feed(argc, argv);
    } else if (strcmp(argv[1], "dec") == 0) {
        AmpUT_ExifDmx_dec(argc, argv);
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_ExifDmx_stop();
    } else {

    }
    return 0;
}

/**
 * Exif Dmx UT - add test function.
 */
int AmpUT_ExifDmxTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("exifdmx", AmpUT_ExifDmxTest);

    return AMP_OK;
}

