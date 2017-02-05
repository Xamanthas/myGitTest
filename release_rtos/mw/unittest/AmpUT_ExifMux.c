/**
 *  @file AmpUT_ExifMux.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/07/01 |clchan      |Created        |
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

#include <format/Muxer.h>
#include <format/ExtMux.h>
#include <format/ExifMux.h>
#include <stream/File.h>
#include <util.h>

static AMP_STREAM_HDLR_s *g_pStream = NULL; /**< stream handler */
static AMP_MUX_FORMAT_HDLR_s *g_pFormat = NULL; /**< format handler */
static AMP_IMAGE_INFO_s *g_pImage = NULL; /**< image information */
static AMP_MUXER_PIPE_HDLR_s *g_pMuxPipe = NULL; /**< muxer pipe */

#define FULL_JPEG_FN "C:\\AMBA_MAIN.jpg" /**< file name of Full view */
#define SCRN_JPEG_FN "C:\\AMBA_SCRN.jpg" /**< file name of Screen view */
#define THMB_JPEG_FN "C:\\AMBA_THMB.jpg" /**< file name of Thumbnail */
#define JPG_FN_PREFIX   "C:\\TEST"

static UINT32 g_nFileCount = 1; /**< count of file */

//#define USE_EXT_MUX

#define FULL_JPEG_WIDTH     4000 /**< picture width of Full view */
#define FULL_JPEG_HEIGHT    3000 /**< picture height of Full view */
#define SCRN_JPEG_WIDTH     960 /**< picture width of Screen view */
#define SCRN_JPEG_HEIGHT    720 /**< picture height of Screen view */
#define THMB_JPEG_WIDTH     160 /**< picture width of Thumbnail */
#define THMB_JPEG_HEIGHT    120 /**< picture height of Thumbnail */

#define DUMMY_CODEC_HDLR ((void *)0x1234) /**< codec handler */

#define DUMMYSTILLENC_BS_SIZE (5<<20) /**< 5MB BS buffer */
#define DUMMYSTILLENC_DESC_NUM (128) /**< descriptor number = 128 */
#define STACK_SIZE  0x2000 /**< stack size */

// global var for dummyStillEnc
static void* bitsRawBuffer = NULL; /**< raw buffer */
static void* bitsBuffer = NULL; /**< buffer */
static AMP_FIFO_HDLR_s *EncFifoHdlr = NULL; /**< encode FIFO handler */
static AMP_FIFO_HDLR_s *MuxFifoHdlr = NULL; /**< mux FIFO handler */
static AMBA_KAL_TASK_t ExifMuxMuxTask = {0}; /**< mux task */
static void *g_pExifMuxStack = NULL; /**< mux stack */

/**
 * Exif mux UT - close function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifMux_Close(void)
{
    AmbaPrint("%s", __FUNCTION__);
    if (AmpMuxer_Remove(g_pMuxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        return -1;
    }
    if (AmpMuxer_Delete(g_pMuxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        return -1;
    }
    g_pMuxPipe = NULL;
#ifdef USE_EXT_MUX
    if (AmpExtMux_Delete(g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        return -1;
    }
#else
    if (AmpExifMux_Delete(g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        return -1;
    }
#endif
    g_pFormat = NULL;
    if (g_pStream->Func->Close(g_pStream) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        return -1;
    }
    if (AmpFileStream_Delete(g_pStream) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        return -1;
    }
    g_pStream= NULL;
    AmpFormat_RelImageInfo(g_pImage, TRUE);
    g_pImage = NULL;

    AmpFifo_Delete(MuxFifoHdlr);
    MuxFifoHdlr = NULL;
    AmpFifo_Delete(EncFifoHdlr);
    EncFifoHdlr = NULL;
    AmbaKAL_BytePoolFree(bitsBuffer);
    bitsBuffer = NULL;
    return 0 ;
}

/**
 * Exif mux UT - mux task.
  *
 * @param [in] info task information.
 *
 */
static void AmpUT_ExifMux_MuxTask(UINT32 info)
{
    int rval = 0;
    AmbaPrint("%s Start", __FUNCTION__);
    AmpMuxer_WaitComplete(g_pMuxPipe, AMBA_KAL_WAIT_FOREVER);
    rval = AmpUT_ExifMux_Close();
    if (rval == -1) {
        AmbaPrint("[FAIL] %s", __FUNCTION__);
    } else {
        AmbaPrint("%s Stop", __FUNCTION__);
        AmbaPrint("[SUCCESS] AmpUT_ExifMux : End");
    }
}

/**
 * Exif mux UT - encodec FIFO callback function.
 *
 * @param [in] hdlr encoder handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifMux_EncFifoCB(void *hdlr, UINT32 event, void* info)
{
    AmbaPrint("%s on Event: 0x%x 0x%x", __FUNCTION__, event, AMP_FIFO_EVENT_DATA_CONSUMED);
    return 0;
}

/**
 * Exif mux UT - mux FIFO callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifMux_MuxFifoCB(void *hdlr, UINT32 event, void* info)
{
    AmbaPrint("%s on Event: 0x%x 0x%x", __FUNCTION__, event, AMP_FIFO_EVENT_DATA_READY);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        AmpMuxer_OnDataReady((AMP_FIFO_HDLR_s *)hdlr);
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
        AmpMuxer_OnEOS((AMP_FIFO_HDLR_s *)hdlr);
    }
    return 0;
}

/**
 * Exif mux UT - mux callback function.
 *
 * @param [in] hdlr FIFO handler.
 * @param [in] event event id.
 * @param [in] Info callback information.
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifMux_MuxCB(void* hdlr, UINT32 event, void* info)
{
    switch (event) {
    case AMP_MUXER_EVENT_END:
        AmbaPrint("AMP_MUXER_EVENT_END");
        break;
    default:
        break;
    }
    return 0;
}

/**
 * Exif mux UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifMux_init(void)
{
    AMP_FORMAT_INIT_CFG_s fmtInitCfg;
    void *pFmtBuf;
    AMP_MUXER_INIT_CFG_s muxerInitCfg;
    void *pMuxerBuf;
#ifdef USE_EXT_MUX
    AMP_EXT_MUX_INIT_CFG_s extInitCfg;
    void *pExtBuf;
#else
    AMP_EXIF_MUX_INIT_CFG_s exifInitCfg;
    void *pExifBuf;
#endif
    AMP_FILE_STREAM_INIT_CFG_s fileInitCfg;
    void *pFileBuf;
    void *pStkRawBuf;

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

#ifdef USE_EXT_MUX
    // init external muxer
    AmpExtMux_GetInitDefaultCfg(&extInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&extInitCfg.Buffer, &pExtBuf, extInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpExtMux_Init(&extInitCfg);
#else
    AmpExifMux_GetInitDefaultCfg(&exifInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&exifInitCfg.Buffer, &pExifBuf, exifInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpExifMux_Init(&exifInitCfg);
#endif

    // init file stream
    AmpFileStream_GetInitDefaultCfg(&fileInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&fileInitCfg.Buffer, &pFileBuf, fileInitCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpFileStream_Init(&fileInitCfg);

    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_pExifMuxStack, &pStkRawBuf, STACK_SIZE, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }

    /** Create task */
    AmbaKAL_TaskCreate(&ExifMuxMuxTask, /** pTask */
    "UT_ExifMux_MuxTask", /** pTaskName */
    7, /** Priority */
    AmpUT_ExifMux_MuxTask, /** void (*EntryFunction)(UINT32) */
    0x0, /** EntryArg */
    g_pExifMuxStack,    /** pStackBase */
    STACK_SIZE, /** StackByteSize */
    AMBA_KAL_DO_NOT_START); /** AutoStart */

    AmbaPrint("[SUCCESS] %s", __FUNCTION__);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * Exif mux UT - open function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifMux_Open(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;
    AMP_FILE_STREAM_CFG_s fileCfg;
    AMP_MUX_IMAGE_INFO_CFG_s imageCfg;
    AMP_MUXER_PIPE_CFG_s muxPipeCfg;
    char szName[MAX_FILENAME_LENGTH] = {'\0'};
    char file[MAX_FILENAME_LENGTH] = {0};
#ifdef USE_EXT_MUX
    AMP_EXT_MUX_CFG_s extCfg;
#else
    AMP_EXIF_MUX_CFG_s exifCfg;
    /** Test for set tag */
    UINT8 nMake[10] = {'I','f','d','0','t','e','s','t','A','\0'};
    UINT8 nModel[10] = {'I','f','d','0','t','e','s','t','B','\0'};
    UINT8 nSoftVer[12] = {'I','f','d','0','t','e','s','t','C','D','E','\0'};
    int i;
#endif


    AmbaKAL_TaskReset(&ExifMuxMuxTask);

    // create codec fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = DUMMY_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 0;
    fifoDefCfg.NumEntries = 128;
    fifoDefCfg.cbEvent = AmpUT_ExifMux_EncFifoCB;
    AmpFifo_Create(&fifoDefCfg, &EncFifoHdlr);

    // create muxer fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = DUMMY_CODEC_HDLR;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = 128;
    fifoDefCfg.cbEvent = AmpUT_ExifMux_MuxFifoCB;
    AmpFifo_Create(&fifoDefCfg, &MuxFifoHdlr);

    // read frames
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&bitsRawBuffer, &bitsBuffer, DUMMYSTILLENC_BS_SIZE, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        return -1;
    }

    snprintf(file, MAX_FILENAME_LENGTH, "%s%01u.JPG", JPG_FN_PREFIX, g_nFileCount);
    strncpy(szName, file, MAX_FILENAME_LENGTH);
    szName[MAX_FILENAME_LENGTH - 1] = '\0';
    g_nFileCount ++;
    // open media info
    AmpFormat_NewImageInfo(szName, &g_pImage);

    // open file stream
    AmpFileStream_GetDefaultCfg(&fileCfg);
    AmpFileStream_Create(&fileCfg, &g_pStream);
    g_pStream->Func->Open(g_pStream, szName, AMP_STREAM_MODE_WRONLY);

    // set media info
    AmpMuxer_GetDefaultImageInfoCfg(&imageCfg);
    imageCfg.Fifo = MuxFifoHdlr;
    imageCfg.BufferBase = (UINT8 *)bitsRawBuffer;
    imageCfg.BufferLimit = (UINT8 *)bitsRawBuffer + DUMMYSTILLENC_BS_SIZE;
    imageCfg.UsedFrame = imageCfg.TotalFrame = 3;
    imageCfg.Frame[0].SeqNum = 0;
    imageCfg.Frame[0].Type = AMP_FIFO_TYPE_JPEG_FRAME;
    imageCfg.Frame[0].Width = FULL_JPEG_WIDTH;
    imageCfg.Frame[0].Height = FULL_JPEG_HEIGHT;

    /** Test for exifinfo */
    imageCfg.Frame[0].ExifInfo.ExposureTimeNum = 100;
    imageCfg.Frame[0].ExifInfo.ExposureTimeDen = 200;
    imageCfg.Frame[0].ExifInfo.FNumberNum = 400;
    imageCfg.Frame[0].ExifInfo.FNumberDen = 500;
    imageCfg.Frame[0].ExifInfo.ColorSpace = 66;
    imageCfg.Frame[0].ExifInfo.Contrast = 99;

    imageCfg.Frame[1].SeqNum = 0;
    imageCfg.Frame[1].Type = AMP_FIFO_TYPE_THUMBNAIL_FRAME;
    imageCfg.Frame[1].Width = THMB_JPEG_WIDTH;
    imageCfg.Frame[1].Height = THMB_JPEG_HEIGHT;
    imageCfg.Frame[2].SeqNum = 0;
    imageCfg.Frame[2].Type = AMP_FIFO_TYPE_SCREENNAIL_FRAME;
    imageCfg.Frame[2].Width = SCRN_JPEG_WIDTH;
    imageCfg.Frame[2].Height = SCRN_JPEG_HEIGHT;
    AmpMuxer_InitImageInfo(g_pImage, &imageCfg);

#ifdef USE_EXT_MUX
    // create external muxer
    AmpExtMux_GetDefaultCfg(AMP_MEDIA_INFO_IMAGE, &extCfg);
    extCfg.Stream = g_pStream;
    AmpExtMux_Create(&extCfg, &g_pFormat);
#else
    AmpExifMux_GetDefaultCfg(&exifCfg);

    for(i = 0; i < exifCfg.SetTagInfo.Ifd0Tags; i++) {
        switch (exifCfg.SetTagInfo.Ifd0[i].Tag) {
        case TIFF_Software:
            exifCfg.SetTagInfo.Ifd0[i].Set = TAG_CONFIGURED;
            exifCfg.SetTagInfo.Ifd0[i].Count = 12;
            exifCfg.SetTagInfo.Ifd0[i].Data = nSoftVer;
            break;
        case TIFF_Make:
            exifCfg.SetTagInfo.Ifd0[i].Set = TAG_CONFIGURED;
            exifCfg.SetTagInfo.Ifd0[i].Count = 10;
            exifCfg.SetTagInfo.Ifd0[i].Data = nMake;
            break;
        case TIFF_Model:
            exifCfg.SetTagInfo.Ifd0[i].Set = TAG_CONFIGURED;
            exifCfg.SetTagInfo.Ifd0[i].Count = 10;
            exifCfg.SetTagInfo.Ifd0[i].Data = nModel;
            break;
        case TIFF_Orientation:
            exifCfg.SetTagInfo.Ifd0[i].Set = TAG_DISABLED;
            break;
        case TIFF_XResolution:
            exifCfg.SetTagInfo.Ifd0[i].Set = TAG_DISABLED;
            break;
        case TIFF_YResolution:
            exifCfg.SetTagInfo.Ifd0[i].Set = TAG_DISABLED;
            break;
        case TIFF_GPSInfoIFDPointer:
            exifCfg.SetTagInfo.Ifd0[i].Set = TAG_ENABLED;
            break;
        default:
            break;

        }
    }

    /** Create */
    exifCfg.Stream = g_pStream;
    if (AmpExifMux_Create(&exifCfg, &g_pFormat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        return -1;
    }
#endif

    // create muxer pipe
    AmpMuxer_GetDefaultCfg(&muxPipeCfg);
    muxPipeCfg.FormatCount = 1;
    muxPipeCfg.Format[0] = g_pFormat;
    muxPipeCfg.OnEvent = AmpUT_ExifMux_MuxCB;
    muxPipeCfg.Media[0] = (AMP_MEDIA_INFO_s *)g_pImage;
    if (AmpMuxer_Create(&muxPipeCfg, &g_pMuxPipe) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        return -1;
    }
    if (AmpMuxer_Add(g_pMuxPipe) != AMP_OK) {
         AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
         return -1;
     }
    if (AmpMuxer_Start(g_pMuxPipe, 0) != AMP_OK) {
         AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
         return -1;
     }
    AmbaKAL_TaskResume(&ExifMuxMuxTask);

    return 0 ;
}

/**
 * Exif mux UT - start function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifMux_start(void)
{
    AMP_CFS_FILE_s *pFile;
    AMP_CFS_FILE_PARAM_s cfsParam;
    AMP_CFS_STAT stat;
    UINT32 len;
    AMP_BITS_DESC_s tmpDesc = {0};
    UINT8 *wp;
    if (AmpUT_ExifMux_Open() != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    wp = bitsRawBuffer;
    AmpCFS_GetFileParam(&cfsParam);
    cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    if (AmpCFS_Stat(FULL_JPEG_FN, &stat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    strcpy(cfsParam.Filename, FULL_JPEG_FN);
    pFile = AmpCFS_fopen(&cfsParam);
    if (pFile == NULL) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    len = AmpCFS_fread(wp, 1, stat.Size, pFile);
    if (len != stat.Size) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpCFS_fclose(pFile);
    tmpDesc.SeqNum = 0;
    tmpDesc.StartAddr = wp;
    tmpDesc.Size = len;
    tmpDesc.Type = AMP_FIFO_TYPE_JPEG_FRAME;
    tmpDesc.Completed = 1;
    if (AmpFifo_WriteEntry(EncFifoHdlr, &tmpDesc) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmbaPrint("feed %u %u %u", tmpDesc.SeqNum, tmpDesc.Type, tmpDesc.Size);
    AmbaKAL_TaskSleep(20);
    if (AmpCFS_Stat(THMB_JPEG_FN, &stat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    wp += len;
    strcpy(cfsParam.Filename, THMB_JPEG_FN);
    pFile = AmpCFS_fopen(&cfsParam);
    if (pFile == NULL) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    len = AmpCFS_fread(wp, 1, stat.Size, pFile);
    if (len != stat.Size) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpCFS_fclose(pFile);
    tmpDesc.SeqNum = 0;
    tmpDesc.StartAddr = wp;
    tmpDesc.Size = len;
    tmpDesc.Type = AMP_FIFO_TYPE_THUMBNAIL_FRAME;
    tmpDesc.Completed = 1;
    if (AmpFifo_WriteEntry(EncFifoHdlr, &tmpDesc) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmbaPrint("feed %u %u %u", tmpDesc.SeqNum, tmpDesc.Type, tmpDesc.Size);
    AmbaKAL_TaskSleep(20);
    if (AmpCFS_Stat(SCRN_JPEG_FN, &stat) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    wp += len;
    strcpy(cfsParam.Filename, SCRN_JPEG_FN);
    pFile = AmpCFS_fopen(&cfsParam);
    if (pFile == NULL) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    len = AmpCFS_fread(wp, 1, stat.Size, pFile);
    if (len != stat.Size) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmpCFS_fclose(pFile);
    tmpDesc.SeqNum = 0;
    tmpDesc.StartAddr = wp;
    tmpDesc.Size = len;
    tmpDesc.Type = AMP_FIFO_TYPE_SCREENNAIL_FRAME;
    tmpDesc.Completed = 1;
    if (AmpFifo_WriteEntry(EncFifoHdlr, &tmpDesc) != AMP_OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
        goto ERR;
    }
    AmbaPrint("feed %u %u %u", tmpDesc.SeqNum, tmpDesc.Type, tmpDesc.Size);
    return 0;
ERR:
    AmbaPrint("[FAIL] %s", __FUNCTION__);
    return -1;
}

/**
 * Exif mux UT - stop function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_ExifMux_stop(void)
{
    //AmpUT_ExifMux_Close();
    return 0;
}

/**
 * Exif mux UT - Exif mux test function.
 */
static int AmpUT_ExifMuxTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("%s: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_ExifMux_init();
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_ExifMux_start();
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_ExifMux_stop();
    } else {

    }
    return 0;
}

/**
 * Exif mux UT - Exif mux test function.
 */
int AmpUT_ExifMuxTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("exifmux", AmpUT_ExifMuxTest);

    return AMP_OK;
}
