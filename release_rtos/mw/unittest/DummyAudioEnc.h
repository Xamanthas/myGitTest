#ifndef __DUMMYAUDIOENC_H__

#include "RingBuffer.h"

#define DUMMY_AUDIO_ENC_PRIORITY    42

typedef struct {
    AMBA_KAL_EVENT_FLAG_t EncStop;
    AMP_CFS_FILE_s *pIdxFile;
    AMP_CFS_FILE_s *pRawFile;
    UINT64 nHDRFlen;
    UINT32 State;
    AMBA_KAL_TASK_t EncTask;
    char EncStack[0x8000];
    RingBuf_s RingBuf;
    UINT8 *pBuffer;
    AMP_FIFO_HDLR_s *pFifo;
    char szBsName[MAX_FILENAME_LENGTH];
    char szHdrName[MAX_FILENAME_LENGTH];
} DummyAudioEnc_s;

extern UINT32 DummyAudioEnc_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxQueuedFrame);

extern int DummyAudioEnc_Init(DummyAudioEnc_s *pEncoder, char *szBsName, char *szHdrName, UINT8 *pBuffer, UINT32 nBufferSize, UINT32 nMaxQueuedFrame);

extern int DummyAudioEnc_Reset(DummyAudioEnc_s *pEncoder, AMP_FIFO_HDLR_s *pFifo);

extern int DummyAudioEnc_Start(DummyAudioEnc_s *pEncoder);

extern int DummyAudioEnc_Stop(DummyAudioEnc_s *pEncoder);

extern int DummyAudioEnc_Pause(DummyAudioEnc_s *pEncoder);

extern int DummyAudioEnc_Resume(DummyAudioEnc_s *pEncoder);

#define __DUMMYAUDIOENC_H__
#endif

