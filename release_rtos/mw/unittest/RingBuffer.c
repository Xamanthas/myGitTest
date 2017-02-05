/**
 *  @file RingBuf.c
 *
 *  Ring Buffer function.
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/07/12 |felix       |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <stdlib.h>
#include "RingBuffer.h"
#include "AmbaDataType.h"
#include <AmbaPrintk.h>
#include "AmbaKAL.h"

#define RingBuf_Perror(level, color, fmt, ...) {\
    char Buffer[160];\
    snprintf(Buffer, 160, fmt, ##__VA_ARGS__);\
    Buffer[159] = '\0';\
    AmbaPrintColor(color, "[Error]%s:%u %s", __FUNCTION__, __LINE__, Buffer);\
}

void RingBuf_Debug(RingBuf_s *pBuffer)
{
#if 0
//    AmbaPrint("pBuffer->nRingBufFreeSize = %u", pBuffer->nRingBufFreeSize);
    AmbaPrint("pBuffer->nRingDescCount = %u", pBuffer->nRingDescCount);
    AmbaPrint("pBuffer->nRingWDescNum = %u", pBuffer->nRingWDescNum);
//    AmbaPrint("pBuffer->pRingBufWp = 0x%x", pBuffer->pRingBufWp);
    AmbaPrint("pBuffer->nRingRDescNum = %u", pBuffer->nRingRDescNum);
//    AmbaPrint("pBuffer->pRingBufRp = 0x%x", pBuffer->pRingBufRp);
#endif
}

UINT32 RingBuf_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxDesc)
{
    return (nRawBufSize + nMaxDesc * sizeof(RING_BUFFER_DESC_s));
}

int RingBuf_Init(RingBuf_s *pBuffer, UINT8 *pBufferBase, UINT32 nBufferSize, UINT32 nMaxDesc, AMP_CALLBACK_f cdRingBuf)
{
    int rval = 0;
    UINT32 nDescIdx = 0;
    const UINT32 nRawBufSize = nBufferSize - nMaxDesc * sizeof(RING_BUFFER_DESC_s);
    K_ASSERT(pBuffer != NULL);
    K_ASSERT(pBufferBase != NULL);
    K_ASSERT(nBufferSize == RingBuf_GetRequiredBufSize(nRawBufSize, nMaxDesc));

    memset(pBuffer, 0, sizeof(RingBuf_s));
    if (AmbaKAL_MutexCreate(&pBuffer->Mutex) == OK) {
        pBuffer->pRingBufBase = pBufferBase;
        pBuffer->pRingBufLimit = pBufferBase + nRawBufSize;
        pBuffer->nRingBufSize = nRawBufSize;
        pBuffer->nRingBufFreeSize = pBuffer->nRingBufSize;
        pBuffer->pRingBufWp = pBufferBase;
        pBuffer->pRingBufRp = pBufferBase;
        pBuffer->nRingWDescNum = 0;
        pBuffer->nRingRDescNum = 0;
        pBuffer->nRingDescCount = 0;
        pBuffer->nRingDescNum = nMaxDesc;
        pBuffer->Desc = (RING_BUFFER_DESC_s *)pBuffer->pRingBufLimit;
        for (nDescIdx = 0; nDescIdx < pBuffer->nRingDescNum; nDescIdx++) {
            pBuffer->Desc[nDescIdx].pDescWp = NULL;
            pBuffer->Desc[nDescIdx].nWriteSize = 0;
        }

        pBuffer->cbRingBuf = cdRingBuf;
        RingBuf_Debug(pBuffer);
        rval = 0;
    } else {
        RingBuf_Perror(0, 0, "create mutex failed");
        rval = -1;
    }

    return rval;
}

void RingBuf_Deinit(RingBuf_s *pBuffer)
{
    UINT32 nDescIdx = 0;

    if (AmbaKAL_MutexDelete(&pBuffer->Mutex) == OK) {
        pBuffer->pRingBufBase = 0;
        pBuffer->pRingBufLimit = 0;
        pBuffer->nRingBufSize = 0;
        pBuffer->nRingBufFreeSize = pBuffer->nRingBufSize;
        pBuffer->pRingBufWp = NULL;
        pBuffer->pRingBufRp = NULL;
        pBuffer->nRingWDescNum = 0;
        pBuffer->nRingRDescNum = 0;
        pBuffer->nRingDescCount = 0;

        for (nDescIdx = 0; nDescIdx < pBuffer->nRingDescNum; nDescIdx++) {
            pBuffer->Desc[nDescIdx].pDescWp = NULL;
            pBuffer->Desc[nDescIdx].nWriteSize = 0;
        }
    }
}

int RingBuf_Reset(RingBuf_s *pBuffer)
{
    int rval = -1;
    if (AmbaKAL_MutexTake(&pBuffer->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        INT32 nDescIdx = 0;
        pBuffer->nRingBufFreeSize = pBuffer->nRingBufSize;
        pBuffer->pRingBufWp = pBuffer->pRingBufBase;
        pBuffer->pRingBufRp = pBuffer->pRingBufBase;
        pBuffer->nRingWDescNum = 0;
        pBuffer->nRingRDescNum = 0;
        pBuffer->nRingDescCount = 0;

        for (nDescIdx = 0; nDescIdx < pBuffer->nRingDescNum; nDescIdx++) {
            pBuffer->Desc[nDescIdx].pDescWp = NULL;
            pBuffer->Desc[nDescIdx].nWriteSize = 0;
        }
        rval = 0;
        AmbaKAL_MutexGive(&pBuffer->Mutex);
    } else {
        RingBuf_Perror(0, 0, "take mutex failed");
    }
    return rval;
}

int RingBuf_GetInfo(RingBuf_s *pBuffer, RING_BUFFER_INFO_s *Info)
{
    int rval = 0;

    if (AmbaKAL_MutexTake(&pBuffer->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        Info->pRingBufWp = pBuffer->pRingBufWp;
        Info->nRingBufFreeSize = pBuffer->nRingBufFreeSize;
        Info->nRingDescCount = pBuffer->nRingDescCount;
        AmbaKAL_MutexGive(&pBuffer->Mutex);
    } else {
        RingBuf_Perror(0, 0, "take mutex failed");
        rval = -1;
    }

    return rval;
}

int RingBuf_UpdateWp(RingBuf_s *pBuffer, UINT8 *pWp, UINT32 nSize)
{
    int rval = -1;

    if (AmbaKAL_MutexTake(&pBuffer->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        if (nSize > pBuffer->nRingBufFreeSize) {
            AmbaPrint("%s, Buffer is Full", __FUNCTION__);
            AmbaPrint("RingBuf FreeSize = %u, Frame Size = %u", pBuffer->nRingBufFreeSize, nSize);
            rval = 0;
        } else {
            if ((pBuffer->Desc[pBuffer->nRingWDescNum].pDescWp != NULL)
                || (pBuffer->Desc[pBuffer->nRingWDescNum].nWriteSize != 0)) {
                AmbaPrint("%s, Buffer Desc is Full", __FUNCTION__);
            } else {
                RingBuf_Debug(pBuffer);
                pBuffer->Desc[pBuffer->nRingWDescNum].pDescWp = pWp;
                pBuffer->Desc[pBuffer->nRingWDescNum].nWriteSize = nSize;
                pBuffer->nRingWDescNum = (pBuffer->nRingWDescNum + 1) % pBuffer->nRingDescNum;
                pBuffer->nRingDescCount++;
                pBuffer->nRingBufFreeSize -= nSize;

                if ((UINT8 *)((UINT32)pWp + nSize) <= pBuffer->pRingBufLimit) {
                    pBuffer->pRingBufWp = (UINT8 *)((UINT32)pWp + nSize);
                } else {
                    UINT32 rear = (UINT32)pBuffer->pRingBufLimit - (UINT32)pWp;
                    //AmbaPrint("Ring buffer round ");
                    pBuffer->pRingBufWp = (UINT8 *)((UINT32)pBuffer->pRingBufBase + (nSize - rear));
                }

                if (pBuffer->pRingBufWp >= pBuffer->pRingBufLimit)
                    pBuffer->pRingBufWp = pBuffer->pRingBufBase;

                rval = 0;
            }
        }
        AmbaKAL_MutexGive(&pBuffer->Mutex);
    } else {
        RingBuf_Perror(0, 0, "take mutex failed");
        goto Done;
    }

Done:
    return rval;
}

int RingBuf_CreateFrame(RingBuf_s *pBuffer, AMP_CFS_FILE_s *pFile, UINT32 nSize)
{
    int rval = -1;

    if (AmbaKAL_MutexTake(&pBuffer->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        if (nSize > pBuffer->nRingBufFreeSize) {
            AmbaPrint("%s, Buffer is Full", __FUNCTION__);
            rval = 0;
        } else {
            if ((pBuffer->Desc[pBuffer->nRingWDescNum].pDescWp != NULL)
                || (pBuffer->Desc[pBuffer->nRingWDescNum].nWriteSize != 0)) {
                AmbaPrint("%s, Buffer Desc is Full", __FUNCTION__);
                rval = 0;
            } else {
                pBuffer->Desc[pBuffer->nRingWDescNum].pDescWp = pBuffer->pRingBufWp;
                pBuffer->Desc[pBuffer->nRingWDescNum].nWriteSize = nSize;
                pBuffer->nRingWDescNum = (pBuffer->nRingWDescNum + 1) % pBuffer->nRingDescNum;
                pBuffer->nRingDescCount++;
                pBuffer->nRingBufFreeSize -= nSize;

                if ((UINT8 *)((UINT32)pBuffer->pRingBufWp + nSize) <= pBuffer->pRingBufLimit) {
                    AmpCFS_fread(pBuffer->pRingBufWp, 1, nSize, pFile);
                    pBuffer->pRingBufWp = (UINT8 *)((UINT32)pBuffer->pRingBufWp + nSize);
                } else {
                    UINT32 rear = (UINT32)pBuffer->pRingBufLimit - (UINT32)pBuffer->pRingBufWp;
                    //AmbaPrint("Ring buffer round ");
                    AmpCFS_fread(pBuffer->pRingBufWp, 1, rear, pFile);
                    AmpCFS_fread(pBuffer->pRingBufBase, 1, (nSize - rear), pFile);
                    pBuffer->pRingBufWp = (UINT8 *)((UINT32)pBuffer->pRingBufBase + (nSize - rear));
                }

                if (pBuffer->pRingBufWp >= pBuffer->pRingBufLimit)
                    pBuffer->pRingBufWp = pBuffer->pRingBufBase;

                rval = nSize;
            }
        }
        AmbaKAL_MutexGive(&pBuffer->Mutex);
    } else {
        RingBuf_Perror(0, 0, "take mutex failed");
        goto Done;
    }

Done:
    return rval;
}

int RingBuf_DeleteFrame(RingBuf_s *pBuffer)
{
    int rval = -1;
    UINT32 nSize = 0;

    if (AmbaKAL_MutexTake(&pBuffer->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        if ((pBuffer->Desc[pBuffer->nRingRDescNum].pDescWp == NULL) || (pBuffer->Desc[pBuffer->nRingRDescNum].nWriteSize == 0)) {
            AmbaPrint("%s, Buffer Desc is Empty", __FUNCTION__);
            rval = 0;
        } else {
            nSize = pBuffer->Desc[pBuffer->nRingRDescNum].nWriteSize;

            /**< Clean Buffer */
            pBuffer->Desc[pBuffer->nRingRDescNum].pDescWp = NULL;
            pBuffer->Desc[pBuffer->nRingRDescNum].nWriteSize = 0;
            pBuffer->nRingRDescNum = (pBuffer->nRingRDescNum + 1) % pBuffer->nRingDescNum;
            if (pBuffer->nRingDescCount > 0)
                pBuffer->nRingDescCount--;
            pBuffer->nRingBufFreeSize += nSize;
            rval = 0;
        }
        AmbaKAL_MutexGive(&pBuffer->Mutex);
    } else {
        RingBuf_Perror(0, 0, "take mutex failed");
        goto Done;
    }

Done:
    return rval;
}

int RingBuf_MoveFrameToFile(RingBuf_s *pBuffer, AMP_CFS_FILE_s *pFile)
{
    int rval = -1;
    UINT32 nSize = 0;

    if (AmbaKAL_MutexTake(&pBuffer->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        if ((pBuffer->Desc[pBuffer->nRingRDescNum].pDescWp == NULL) &&
            (pBuffer->Desc[pBuffer->nRingRDescNum].nWriteSize == 0)) {
            //RingBuf_Perror(0, 0, "DescWp == NULL or write size 0");
            rval = -2; // empty
        } else {
            pBuffer->pRingBufRp = pBuffer->Desc[pBuffer->nRingRDescNum].pDescWp;
            nSize = pBuffer->Desc[pBuffer->nRingRDescNum].nWriteSize;
            RingBuf_Debug(pBuffer);
            if ((UINT8 *)((UINT32)pBuffer->pRingBufRp + nSize) <= pBuffer->pRingBufLimit) {
                if (AmpCFS_fwrite(pBuffer->pRingBufRp, 1, nSize, pFile) != nSize) {RingBuf_Perror(0, 0, "AmpCFS_fwrite() failed");}
            } else {
                UINT32 rear = (UINT32)pBuffer->pRingBufLimit - (UINT32)pBuffer->pRingBufRp;
                //AmbaPrint("Ring buffer round ");
                if (AmpCFS_fwrite(pBuffer->pRingBufRp, 1, rear, pFile) != rear) {RingBuf_Perror(0, 0, "AmpCFS_fwrite() failed");}
                if (AmpCFS_fwrite(pBuffer->pRingBufBase, 1, (nSize - rear), pFile) != (nSize - rear)) {RingBuf_Perror(0, 0, "AmpCFS_fwrite() failed");}
            }

            /**< Clean Buffer */
            pBuffer->Desc[pBuffer->nRingRDescNum].pDescWp = NULL;
            pBuffer->Desc[pBuffer->nRingRDescNum].nWriteSize = 0;
            pBuffer->nRingRDescNum = (pBuffer->nRingRDescNum + 1) % pBuffer->nRingDescNum;
            if (pBuffer->nRingDescCount > 0)
                pBuffer->nRingDescCount--;
            pBuffer->nRingBufFreeSize += nSize;
            rval = 0;
        }
        AmbaKAL_MutexGive(&pBuffer->Mutex);
    } else {
        RingBuf_Perror(0, 0, "take mutex failed");
    }

    return rval;
}

