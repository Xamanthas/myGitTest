#ifndef __DUMMYVIDEODEC_H__

#include "RingBuffer.h"

#define DUMMY_VIDEO_DEC_PRIORITY    45

typedef int (*DUMMY_VIDEO_DEC_DATA_REQUEST_FP)(void *);

typedef struct {
    AMBA_KAL_EVENT_FLAG_t DecStop;
    AMP_CFS_FILE_s *pIdxFile;
    AMP_CFS_FILE_s *pRawFile;
    UINT32 nState;
    AMBA_KAL_TASK_t DecTask;
    char DecStack[0x8000];
    RingBuf_s RingBuf;
    UINT8 *pBuffer;
    AMP_FIFO_HDLR_s *pFifo;
    char szBsName[MAX_FILENAME_LENGTH];
    char szHdrName[MAX_FILENAME_LENGTH];
    DUMMY_VIDEO_DEC_DATA_REQUEST_FP fnDataRequest;
} DummyVideoDec_s;

extern UINT32 DummyVideoDec_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxQueuedFrame);

extern int DummyVideoDec_Init(DummyVideoDec_s *pEncoder, char *szBsName, char *szHdrName, UINT8 *pBuffer, UINT32 nBufferSize, UINT32 nMaxQueuedFrame, DUMMY_VIDEO_DEC_DATA_REQUEST_FP fnDataRequest);

extern int DummyVideoDec_Reset(DummyVideoDec_s *pEncoder, AMP_FIFO_HDLR_s *pFifo);

extern int DummyVideoDec_Start(DummyVideoDec_s *pEncoder);

extern int DummyVideoDec_Stop(DummyVideoDec_s *pEncoder);

#define __DUMMYVIDEODEC_H__
#endif

