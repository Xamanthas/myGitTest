/**
 *  @file AmpUT_Fio.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2014/5/23  |clchan      |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include "AmpUnitTest.h"
#include <util.h>

/**
 * FIO UT - print debug message function.
 */
#define Ut_Perror(nLevel, nColor, pFmt, ...) {\
    char buffer[160];\
    snprintf(buffer, 160, pFmt, ##__VA_ARGS__);\
    buffer[159] = '\0';\
    AmbaPrintColor(nColor, "[Error]%s:%u %s", __FUNCTION__, __LINE__, buffer);\
}

#define HDR_FN  "C:\\OUT_0000.nhnt" /**< nhnt file name */
#define EXT_FN  "C:\\TEST.EXT" /**< test file name */
#define STACK_SIZE  0x200000 /**< stack size */
#define DATA_SIZE   (128 * 1024) /**< test data size */
#define MAX_QUEUE_FRAME 1024 /**< max number of queue frame */
#define FSYNC_SIZE  0x10000 /**< number of bytes to sync file */

/**
 * MP4 nhnt header
 */
#pragma pack(1)
typedef struct{
    char Signature[4];
    UINT64 version:8;
    UINT64 streamType:8;
    UINT64 objectTypeIndication:8;
    UINT64 reserved:16;
    UINT64 bufferSizeDB:24;
    UINT32 avgBitRate;
    UINT32 maxBitRate;
    UINT32 timeStampResolution;
}MP4_NHNT_HEADER_s;
#pragma pack()

/**
 * MP4 nhnt sample header
 */
#pragma pack(1)
typedef struct{
    UINT32 dataSize:24;
    UINT32 randomAccessPoint:1;
    UINT32 auStartFlag:1;
    UINT32 auEndFlag:1;
    UINT32 reserved:1;//3->1
    UINT32 frameType:4;// 2->4
//    UINT32 mixFields;
    UINT32 fileOffset;
    UINT32 compositionTimeStamp;
    UINT32 decodingTimeStamp;
}MP4_NHNT_SAMPLE_HEADER_s;
#pragma pack()

static AMBA_KAL_TASK_t g_MuxTask = {0}; /**< mux task */
static void *g_pMuxStack = NULL; /**< mux task stack */
static AMBA_KAL_TASK_t g_EncTask = {0}; /**< encode task */
static void *g_pEncStack = NULL; /**< encode task stack */
static AMBA_KAL_EVENT_FLAG_t g_FlgData = {0}; /**< event flag */
static AMBA_KAL_MUTEX_t g_Mutex = {0};/**< mutex */
static UINT64 g_PTS[MAX_QUEUE_FRAME]; /**< PTS queue */
static UINT32 g_nFrameSize[MAX_QUEUE_FRAME]; /**< frame size queue */
static UINT32 g_nFrameCount; /**< count of frame */
static UINT32 g_nQueueLength; /**< queue length */
static BOOL g_bQueueEnd = FALSE; /**< queue end flag */
static UINT32 g_nBytesNotSynced = 0; /**< number of bytes does not to sync */

/**
 * FIO UT - write frame function.
 *
 * @param [in] nSize data size
 * @param [in] PTS PTS
 * @return 0 - ok, others - fail
 */
static int AmpUT_Fio_WriteFrame(UINT32 nSize, UINT64 PTS)
{
    if (AmbaKAL_MutexTake(&g_Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        const UINT32 idx = g_nFrameCount % MAX_QUEUE_FRAME;
        while (g_nQueueLength == MAX_QUEUE_FRAME) {
            AmbaKAL_MutexGive(&g_Mutex);
            AmbaKAL_TaskSleep(10);
            if (AmbaKAL_MutexTake(&g_Mutex, AMBA_KAL_WAIT_FOREVER) != OK) {
                Ut_Perror(0, 0, "can not lock mutex");
                return -1;
            }
        }
        K_ASSERT(g_nQueueLength < MAX_QUEUE_FRAME);
        g_nFrameSize[idx] = nSize;
        g_PTS[idx] = PTS;
        if (g_nFrameCount > 0) {
            const UINT32 prev = (g_nFrameCount - 1) % MAX_QUEUE_FRAME;
            if (g_PTS[prev] >= g_PTS[idx])
                Ut_Perror(0, 0, "Frame(%u) %llu, prev=%llu", g_nFrameCount, PTS, g_PTS[prev]);
        }
        g_nFrameCount++;
        g_nQueueLength++;
        AmbaKAL_MutexGive(&g_Mutex);
    } else {
        Ut_Perror(0, 0, "can not lock mutex");
        return -1;
    }
    return 0;
}

/**
 * FIO UT - read frame function.
 *
 * @return 0 - ok, others - fail
 *
 */
static int AmpUT_Fio_ReadFrame(void)
{
    int rval = -1;
    if (AmbaKAL_MutexTake(&g_Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        while (g_nQueueLength == 0) {
            if (g_bQueueEnd) {
                AmbaKAL_MutexGive(&g_Mutex);
                return 0;
            }
            AmbaKAL_MutexGive(&g_Mutex);
            AmbaKAL_TaskSleep(10);
            if (AmbaKAL_MutexTake(&g_Mutex, AMBA_KAL_WAIT_FOREVER) != OK) {
                Ut_Perror(0, 0, "can not lock mutex");
                return -1;
            }
        }
        K_ASSERT(g_nQueueLength > 0);
        rval = g_nFrameSize[(g_nFrameCount - g_nQueueLength) % MAX_QUEUE_FRAME];
        g_nQueueLength--;
        AmbaKAL_MutexGive(&g_Mutex);
    } else {
        Ut_Perror(0, 0, "can not lock mutex");
        return -1;
    }
    return rval;
}

/**
 * FIO UT - close queue function.
 *
 * @return 0 - ok, others - fail
 *
 */
static int AmpUT_Fio_CloseQueue(void)
{
    if (AmbaKAL_MutexTake(&g_Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        g_bQueueEnd = TRUE;
        AmbaKAL_MutexGive(&g_Mutex);
    } else {
        Ut_Perror(0, 0, "can not lock mutex");
        return -1;
    }
    return 0;
}

/**
 * FIO UT - encode task.
 *
 * @param [in] info task information.
 *
 */
static void AmpUT_Fio_EncTask(UINT32 info)
{
    UINT32 nIdxLen = 0;
    void *pFioBuf;
    void *pFioRawBuf;
    MP4_NHNT_SAMPLE_HEADER_s nhntDesc = {0};
    MP4_NHNT_HEADER_s nhntHeader = {0};
    AMBA_FS_FILE *pFile;
    AmbaPrint("%s Start", __FUNCTION__);
    pFile = AmbaFS_fopen(HDR_FN, "r");
    if (pFile == NULL) {
        Ut_Perror(0, 0, "can not open file");
        goto Done;
    }
    K_ASSERT(DATA_SIZE >= sizeof(nhntHeader));
    K_ASSERT(DATA_SIZE >= sizeof(nhntDesc));
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&pFioBuf, &pFioRawBuf, DATA_SIZE, 32) != OK) {
        Ut_Perror(0, 0, "can not allocate memory");
        goto Done;
    }
    nIdxLen = AmbaFS_fread(pFioBuf, 1, sizeof(nhntHeader), pFile);
    if (nIdxLen < sizeof(nhntHeader)) {
        Ut_Perror(0, 0, "read header failed");
        goto Done;
    }
    memcpy(&nhntHeader, pFioBuf, sizeof(nhntHeader));
    while (TRUE) {
        if (AmbaFS_feof(pFile)) {
            AmbaPrint("Reach Video EOS");
            break;
        } else {
            /** Read frame header */
            nIdxLen = AmbaFS_fread(pFioBuf, 1, sizeof(nhntDesc), pFile);
            if (nIdxLen < sizeof(nhntDesc)) {
                Ut_Perror(0, 0, "data not enough");
                goto Done;
            }
            memcpy(&nhntDesc, pFioBuf, sizeof(nhntDesc));
            AmpUT_Fio_WriteFrame(nhntDesc.dataSize, nhntDesc.compositionTimeStamp);
            if (AmbaKAL_EventFlagGive(&g_FlgData, 0x00000001) != OK) {
                Ut_Perror(0, 0, "can not give flag");
                goto Done;
            }
        }
        AmbaKAL_TaskSleep(10);
    }

Done:
    AmpUT_Fio_CloseQueue();
    if (AmbaKAL_EventFlagGive(&g_FlgData, 0x80000000) != OK) {
        Ut_Perror(0, 0, "can not give flag");
        goto Done;
    }
    AmbaPrint("%s() Stop", __FUNCTION__);
}

/**
 * FIO UT - write function.
 *
 * @param [in] pBuffer data buffer.
 * @param [in] size data size
 * @param [in] count byte count of data
 * @param [in] pFile file
 * @return number of byte of write data
 *
 */
static UINT32 AmpUT_Fio_fwrite(const void *pBuffer, UINT64 Size, UINT64 Count, AMBA_FS_FILE *pFile)
{
    UINT32 rval = 0;
    UINT32 n;
    UINT32 r;
    while (Count > 0) {
        n = Count;
        if (n > DATA_SIZE)
            n = DATA_SIZE;
        r = AmbaFS_fwrite((void *)((UINT32)pBuffer + (UINT32)(rval * Size)), Size, n, pFile);
        if (r != n) {
            Ut_Perror(0, 0, "write failed");
            break;
        }
        g_nBytesNotSynced += (Size * r);
        rval += r;
        Count -= r;
        if (g_nBytesNotSynced > FSYNC_SIZE) {
            AmbaFS_FSync(pFile);
            g_nBytesNotSynced = 0;
        }
    }
    return rval;
}

/**
 * FIO UT - mux task.
 *
 * @param [in] info task information.
 *
 */
static void AmpUT_Fio_MuxTask(UINT32 info)
{
    AMBA_FS_FILE * const pFile = AmbaFS_fopen(EXT_FN, "w+");
    AmbaPrint("%s Start", __FUNCTION__);
    g_nBytesNotSynced = 0;
    if (pFile != NULL) {
        UINT32 flag;
        UINT8 t[1];
        int size;
        while (1) {
            if (AmbaKAL_EventFlagTake(&g_FlgData, 0xFFFFFFFF, AMBA_KAL_OR, &flag, AMBA_KAL_WAIT_FOREVER) != OK) {
                Ut_Perror(0, 0, "can not take flag");
                break;
            }
            if (AmbaKAL_EventFlagClear(&g_FlgData, 0x00000001) != OK) {
                Ut_Perror(0, 0, "can not clear flag");
                break;
            }
            size = AmpUT_Fio_ReadFrame();
            if (size <= 0)
                break;
            if (AmpUT_Fio_fwrite(&t, 1, size, pFile) != size) {
                Ut_Perror(0, 0, "can not write file");
                break;
            }
        }
        AmbaFS_fclose(pFile);
    } else {
        Ut_Perror(0, 0, "can not open file");
    }
    AmbaPrint("%s Stop", __FUNCTION__);
}

/**
 * FIO UT - initiate function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Fio_init(void)
{
    void *pStkRawBuf;
    if (AmbaKAL_MutexCreate(&g_Mutex) != OK)
        Ut_Perror(0, 0, "can not create mutex");
    if (AmbaKAL_EventFlagCreate(&g_FlgData) != OK)
        Ut_Perror(0, 0, "can not create flag");
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_pMuxStack, &pStkRawBuf, STACK_SIZE, 32) != OK)
        Ut_Perror(0, 0, "can not allocate memory");
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_pEncStack, &pStkRawBuf, STACK_SIZE, 32) != OK)
        Ut_Perror(0, 0, "can not allocate memory");
    if (AmbaKAL_TaskCreate(&g_EncTask, "UT_Fio_EncTask", 40, AmpUT_Fio_EncTask, 0x0, g_pEncStack, STACK_SIZE, AMBA_KAL_DO_NOT_START) != OK)
        Ut_Perror(0, 0, "can not create task");
    if (AmbaKAL_TaskCreate(&g_MuxTask, "UT_Fio_MuxTask", 81, AmpUT_Fio_MuxTask, 0x0, g_pMuxStack, STACK_SIZE, AMBA_KAL_DO_NOT_START) != OK)
        Ut_Perror(0, 0, "can not create task");
    return 0;
}

/**
 * FIO UT - start function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Fio_start(void)
{
    AmbaKAL_TaskResume(&g_EncTask);
    AmbaKAL_TaskResume(&g_MuxTask);
    return 0;
}

/**
 * FIO UT - test function.
 *
 * @return 0 - OK, others - fail
 *
 */
static int AmpUT_Fio_test(void)
{
// AmbaFS can only open 10 files concurrently, no matter read or write
#define MAX_FILE_R  12
#define MAX_FILE_W  12
    UINT32 i;
    AMBA_FS_FILE *pFileR[MAX_FILE_R];
    AMBA_FS_FILE *pFileW[MAX_FILE_W];
    char buffer[64];
    for (i=0; i<MAX_FILE_R; i++) {
        snprintf(buffer, 64, "C:\\%02u", i);
        pFileR[i] = AmbaFS_fopen((char *)buffer, (char *)"r");
        if (pFileR[i] == NULL)
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    }
    for (i=0; i<MAX_FILE_W; i++) {
        snprintf(buffer, 64, "C:\\%02u", i);
        pFileW[i] = AmbaFS_fopen((char *)buffer, (char *)"w");
        if (pFileW[i] == NULL)
            AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    }
    for (i=0; i<MAX_FILE_R; i++) {
        if (pFileR[i] != NULL)
            AmbaFS_fclose(pFileR[i]);
    }
    for (i=0; i<MAX_FILE_R; i++) {
        if (pFileW[i] != NULL)
            AmbaFS_fclose(pFileW[i]);
    }
    return 0;
}

/**
 * FIO UT - FIO test function.
 */
static int AmpUT_FioTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
//    AmbaPrint("%s cmd: %s", __FUNCTION__, argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_Fio_init();
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_Fio_start();
    } else if (strcmp(argv[1], "test") == 0) {
        AmpUT_Fio_test();
    }
    return 0;
}

/**
 * FIO UT - add FIO test function.
 */
int AmpUT_FioTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("fio", AmpUT_FioTest);
    return AMP_OK;
}

