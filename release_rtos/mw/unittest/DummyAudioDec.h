#ifndef __DUMMYAUDIODEC_H__

#include "RingBuffer.h"

#define DUMMY_AUDIO_DEC_PRIORITY    45

typedef int (*DUMMY_AUDIO_DEC_DATA_REQUEST_FP)(void *);

typedef struct {
    AMBA_KAL_EVENT_FLAG_t DecStop;
    AMP_CFS_FILE_s *pRawFile;
    UINT32 nState;
    AMBA_KAL_TASK_t DecTask;
    AMBA_KAL_MUTEX_t Mutex;
    char DecStack[0x8000];
    RingBuf_s RingBuf;
    UINT8 *pBuffer;
    AMP_FIFO_HDLR_s *pFifo;
    char szBsName[MAX_FILENAME_LENGTH];
    DUMMY_AUDIO_DEC_DATA_REQUEST_FP fnDataRequest;
} DummyAudioDec_s;

extern UINT32 DummyAudioDec_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxQueuedFrame);

extern int DummyAudioDec_Init(DummyAudioDec_s *pEncoder, char *szBsName, UINT8 *pBuffer, UINT32 nBufferSize, UINT32 nMaxQueuedFrame, DUMMY_AUDIO_DEC_DATA_REQUEST_FP fnDataRequest);

extern int DummyAudioDec_Reset(DummyAudioDec_s *pEncoder, AMP_FIFO_HDLR_s *pFifo);

extern int DummyAudioDec_Start(DummyAudioDec_s *pEncoder);

extern int DummyAudioDec_Stop(DummyAudioDec_s *pEncoder);

#define __DUMMYAUDIODEC_H__
#endif

