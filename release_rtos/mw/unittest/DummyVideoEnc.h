#ifndef __DUMMYVIDEOENC_H__

#include "RingBuffer.h"

#define DUMMY_VIDEO_ENC_PRIORITY    40

typedef struct {
    AMBA_KAL_EVENT_FLAG_t EncStop;
    AMP_CFS_FILE_s *pIdxFile;
    AMP_CFS_FILE_s *pRawFile;
    UINT64 nHDRFlen;
    UINT32 nState;
    AMBA_KAL_TASK_t EncTask;
    char EncStack[0x8000];
    RingBuf_s RingBuf;
    UINT8 *pBuffer;
    AMP_FIFO_HDLR_s *pFifo;
    char szBsName[MAX_FILENAME_LENGTH];
    char szHdrName[MAX_FILENAME_LENGTH];
} DummyVideoEnc_s;

extern UINT32 DummyVideoEnc_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxQueuedFrame);

extern int DummyVideoEnc_Init(DummyVideoEnc_s *pEncoder, char *szBsName, char *szHdrName, UINT8 *pBuffer, UINT32 nBufferSize, UINT32 nMaxQueuedFrame);

extern int DummyVideoEnc_Reset(DummyVideoEnc_s *pEncoder, AMP_FIFO_HDLR_s *pFifo);

extern int DummyVideoEnc_Start(DummyVideoEnc_s *pEncoder);

extern int DummyVideoEnc_Stop(DummyVideoEnc_s *pEncoder);

extern int DummyVideoEnc_Pause(DummyVideoEnc_s *pEncoder);

extern int DummyVideoEnc_Resume(DummyVideoEnc_s *pEncoder);

#define __DUMMYVIDEOENC_H__
#endif

