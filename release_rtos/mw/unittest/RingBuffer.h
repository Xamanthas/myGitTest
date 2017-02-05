/**
 *  @file RingBuf.h
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

#ifndef __RING_BUF_H__
#define __RING_BUF_H__

#include <stdio.h>
#include <string.h>
#include <fifo/Fifo.h>

#include "AmbaDataType.h"
#include <cfs/AmpCfs.h>
#include <stream/Stream.h>

#ifndef MP4_NHNT_S
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
#define MP4_NHNT_S
#endif

typedef struct {
    UINT8 *pRingBufWp;
    UINT32 nRingDescCount;
    UINT32 nRingBufFreeSize;
}RING_BUFFER_INFO_s;

typedef struct {
    UINT8 *pDescWp;
    UINT32 nWriteSize;
}RING_BUFFER_DESC_s;

typedef struct {
    AMBA_KAL_MUTEX_t Mutex;
    UINT8 *pRingBufBase;
    UINT8 *pRingBufLimit;
    UINT8 *pRingBufWp;
    UINT8 *pRingBufRp;
    UINT32 nRingWDescNum;
    UINT32 nRingRDescNum;
    UINT32 nRingDescCount;
    UINT32 nRingDescNum;
    UINT32 nRingBufSize;
    UINT32 nRingBufFreeSize;
    RING_BUFFER_DESC_s *Desc;
    AMP_CALLBACK_f cbRingBuf;
} RingBuf_s;

extern UINT32 RingBuf_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxDesc);
extern int RingBuf_Init(RingBuf_s *pBuffer, UINT8 *pBufferBase, UINT32 nBufferSize, UINT32 nMaxDesc, AMP_CALLBACK_f cdRingBuf);
extern void RingBuf_Deinit(RingBuf_s *pBuffer);
extern int RingBuf_Reset(RingBuf_s *pBuffer);
extern int RingBuf_GetInfo(RingBuf_s *pBuffer, RING_BUFFER_INFO_s *Info);
extern int RingBuf_UpdateWp(RingBuf_s *pBuffer, UINT8 *pWp, UINT32 nSize);
extern int RingBuf_CreateFrame(RingBuf_s *pBuffer, AMP_CFS_FILE_s *pFile, UINT32 nSize);
extern int RingBuf_DeleteFrame(RingBuf_s *pBuffer);
extern int RingBuf_MoveFrameToFile(RingBuf_s *pBuffer, AMP_CFS_FILE_s *pFile);

#endif
