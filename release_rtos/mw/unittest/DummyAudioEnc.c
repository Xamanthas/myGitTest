#include "DummyAudioEnc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DmyEnc_Perror(level, color, fmt, ...) {\
    char Buffer[160];\
    snprintf(Buffer, 160, fmt, ##__VA_ARGS__);\
    Buffer[159] = '\0';\
    AmbaPrintColor(color, "[Error]%s:%u %s", __FUNCTION__, __LINE__, Buffer);\
}

typedef enum {
    ENCODER_STATE_IDLE,
    ENCODER_STATE_RUNNING,
    ENCODER_STATE_PAUSING,
    ENCODER_STATE_PAUSED,
    ENCODER_STATE_RESUME
} ENCODER_STATE_e;

static int DummyAudioEnc_RingBufCB(void *hdlr, UINT32 event, void* info)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    return 0;
}

static int DummyAudioEnc_OpenFile(DummyAudioEnc_s *Encoder)
{
    AMP_CFS_FILE_PARAM_s cfsParam;
    if (AmpCFS_GetFileParam(&cfsParam) != AMP_OK) {
        DmyEnc_Perror(0, 0, "AmpCFS_GetFileParam() failed");
        return -1;
    }
    cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    strncpy(cfsParam.Filename, Encoder->szHdrName, MAX_FILENAME_LENGTH);
    cfsParam.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
    Encoder->pIdxFile = AmpCFS_fopen(&cfsParam);
    if (Encoder->pIdxFile == NULL) {
        DmyEnc_Perror(0, 0, "AmpCFS_fopen() failed");
        return -1;
    }
    // Open Raw file (to make code clean, not to close the file)
    if (AmpCFS_GetFileParam(&cfsParam) != AMP_OK) {
        DmyEnc_Perror(0, 0, "AmpCFS_GetFileParam() failed");
        return -1;
    }
    cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    strncpy(cfsParam.Filename, Encoder->szBsName, MAX_FILENAME_LENGTH);
    cfsParam.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
    Encoder->pRawFile = AmpCFS_fopen(&cfsParam);
    if (Encoder->pRawFile == NULL) {
        DmyEnc_Perror(0, 0, "AmpCfs_fopen() failed");
        return -1;
    }
    return 0;
}

static int DummyAudioEnc_CloseFile(DummyAudioEnc_s *Encoder)
{
    int rval = 0;
    if (Encoder->pIdxFile != NULL) {
        if (AmpCFS_fclose(Encoder->pIdxFile) != AMP_OK) {
            DmyEnc_Perror(0, 0, "AmpCFS_fclose() failed");
            rval = -1;
        }
        Encoder->pIdxFile = NULL;
    }
    if (Encoder->pRawFile != NULL) {
        if (AmpCFS_fclose(Encoder->pRawFile) != AMP_OK) {
            DmyEnc_Perror(0, 0, "AmpCFS_fclose() failed");
            rval = -1;
        }
        Encoder->pRawFile = NULL;
    }
    return rval;
}

static void DummyAudioEnc_EncTask(UINT32 info)
{
    int rval = 0;
    DummyAudioEnc_s * const Encoder = (DummyAudioEnc_s *)info;
    UINT32 nFrameCount = 0;
    UINT64 TotalSize = 0;
    UINT32 nIdxLen = 0;
    RING_BUFFER_INFO_s RingBufInfo = {0};
    AMP_BITS_DESC_s tmpDesc = {0};
    AMP_FIFO_INFO_s tmpInfo = {0};

    MP4_NHNT_SAMPLE_HEADER_s nhntDesc = {0};
    MP4_NHNT_HEADER_s nhntHeader = {0};

    nIdxLen = AmpCFS_fread(&nhntHeader, 1, sizeof(nhntHeader), Encoder->pIdxFile);
    if (nIdxLen < sizeof(nhntHeader)) {
        DmyEnc_Perror(0, 0, "AmpCFS_fread() failed");
        rval = -1;
        goto Done;
    }

    AmbaPrint("Raw File sizeof(MP4_NHNT_HEADER_s) = (%d):", sizeof(MP4_NHNT_HEADER_s));
    AmbaPrint("Symbol:%c%c%c%c", nhntHeader.Signature[0], nhntHeader.Signature[1], nhntHeader.Signature[2], nhntHeader.Signature[3]);
    AmbaPrint("avgBitRate:%d", nhntHeader.avgBitRate);
    AmbaPrint("maxBitRate:%d", nhntHeader.maxBitRate);
    AmbaPrint("timeStampResolution:%d", nhntHeader.timeStampResolution);

    while (Encoder->State != ENCODER_STATE_IDLE) {
        if (AmpFifo_GetInfo(Encoder->pFifo, &tmpInfo) != AMP_OK) {
            DmyEnc_Perror(0, 0, "AmpFifo_GetInfo() failed");
            rval = -1;
            goto Done;
        }
        if (tmpInfo.AvailEntries < tmpInfo.TotalEntries) {
            if (TotalSize == Encoder->nHDRFlen) {
                AmbaPrint("Reach Audio EOS");
                memset (&tmpDesc, 0, sizeof(tmpDesc));
                tmpDesc.Size = AMP_FIFO_MARK_EOS;// EOS magic num // TBD
                if (AmpFifo_WriteEntry(Encoder->pFifo, &tmpDesc) != AMP_OK) {
                    DmyEnc_Perror(0, 0, "AmpFifo_WriteEntry() failed");
                    rval = -1;
                    goto Done;
                }
                break;
            } else {
                nIdxLen = AmpCFS_fread(&nhntDesc, 1, sizeof(nhntDesc), Encoder->pIdxFile);
                if (nIdxLen < sizeof(nhntDesc)) {
                    DmyEnc_Perror(0, 0, "AmpCFS_fread() failed");
                    rval = -1;
                    goto Done;
                }
                if (nhntDesc.dataSize == AMP_FIFO_MARK_EOS) {
                    AmbaPrint("Reach Audio EOS");
                    memset (&tmpDesc, 0, sizeof(tmpDesc));
                    tmpDesc.Size = AMP_FIFO_MARK_EOS;// EOS magic num // TBD
                    if (AmpFifo_WriteEntry(Encoder->pFifo, &tmpDesc) != AMP_OK) {
                        DmyEnc_Perror(0, 0, "AmpFifo_WriteEntry() failed");
                        rval = -1;
                        goto Done;
                    }
                    break;
                }
                tmpDesc.Size = nhntDesc.dataSize;
                tmpDesc.Type = AMP_FIFO_TYPE_AUDIO_FRAME;
                tmpDesc.SeqNum = nFrameCount;
                TotalSize += tmpDesc.Size;
                while (TRUE) {
                    if (Encoder->State == ENCODER_STATE_PAUSING) {
                        AMP_BITS_DESC_s eos = {0};
                        eos.Size = AMP_FIFO_MARK_EOS_PAUSE;
                        AmbaPrint("Reach Audio PAUSE");
                        AmpFifo_WriteEntry(Encoder->pFifo, &eos);
                        Encoder->State = ENCODER_STATE_PAUSED;
                        AmbaKAL_TaskSleep(200);
                    } else if (Encoder->State == ENCODER_STATE_RESUME) {
                        Encoder->State = ENCODER_STATE_RUNNING;
                        break;
                    } else if (Encoder->State == ENCODER_STATE_PAUSED) {
                        AmbaKAL_TaskSleep(200);
                    } else {
                        break;
                    }
                }

                //AmbaPrint("Fifo[%p] Read %u: pts:%llu, ftype:%u, size:%u, total size:%llu", Encoder->pFifo, nFrameCount, tmpDesc.Pts, nhntDesc.frameType, tmpDesc.Size, TotalSize);

                /**< Read frame from BitStream*/
                RingBuf_GetInfo(&Encoder->RingBuf, &RingBufInfo);
                tmpDesc.Align = 0;
                tmpDesc.StartAddr = RingBufInfo.pRingBufWp;
                if (AmpCFS_fseek(Encoder->pRawFile, nhntDesc.fileOffset, AMP_CFS_SEEK_START) != AMP_OK) {
                    DmyEnc_Perror(0, 0, "AmpCFS_fseek() failed");
                    rval = -1;
                    goto Done;
                }
                while (TRUE) {
                    const int r = RingBuf_CreateFrame(&Encoder->RingBuf, Encoder->pRawFile, tmpDesc.Size);
                    if (r == tmpDesc.Size)
                        break;
                    if (r == 0) {
                        AmbaKAL_TaskSleep(1000);
                    } else {
                        DmyEnc_Perror(0, 0, "RingBuf_CreateFrame() failed");
                        rval = -1;
                        goto Done;
                    }
                }
                // write to fifo
                if (AmpFifo_WriteEntry(Encoder->pFifo, &tmpDesc) != AMP_OK) {
                    DmyEnc_Perror(0, 0, "AmpFifo_WriteEntry() failed");
                    rval = -1;
                    goto Done;
                }
                nFrameCount++;
            }
            AmbaKAL_TaskSleep(10);
        } else {
            AmbaKAL_TaskSleep(1000);
        }
    }
Done:
    DummyAudioEnc_CloseFile(Encoder);
    if (AmbaKAL_EventFlagGive(&Encoder->EncStop, 0x00000001) != OK)
        DmyEnc_Perror(0, 0, "AmbaKAL_EventFlagGive() failed");
    AmbaPrint("%s() is stopped", __FUNCTION__);
    if (rval == -1)
        AmbaPrint("[FAIL]: Muxing Failed!");
}

UINT32 DummyAudioEnc_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxQueuedFrame)
{
    return RingBuf_GetRequiredBufSize(nRawBufSize, nMaxQueuedFrame);
}

int DummyAudioEnc_Init(DummyAudioEnc_s *Encoder, char *szBsName, char *szHdrName, UINT8 *pBuffer, UINT32 nBufferSize, UINT32 nMaxQueuedFrame)
{
    AMP_CFS_STAT stat;
    const UINT32 nRawBufSize = nBufferSize - nMaxQueuedFrame * sizeof(RING_BUFFER_DESC_s);
    K_ASSERT(nBufferSize == DummyAudioEnc_GetRequiredBufSize(nRawBufSize, nMaxQueuedFrame));
    memset(Encoder, 0, sizeof(DummyAudioEnc_s));
    strncpy(Encoder->szBsName, szBsName, MAX_FILENAME_LENGTH);
    Encoder->szBsName[MAX_FILENAME_LENGTH - 1] = '\0';
    strncpy(Encoder->szHdrName, szHdrName, MAX_FILENAME_LENGTH);
    Encoder->szHdrName[MAX_FILENAME_LENGTH - 1] = '\0';
    Encoder->State = ENCODER_STATE_IDLE;
    Encoder->pBuffer = pBuffer;
    if (RingBuf_Init(&Encoder->RingBuf, (UINT8 *)Encoder->pBuffer, nBufferSize, nMaxQueuedFrame, DummyAudioEnc_RingBufCB) != 0) {
        DmyEnc_Perror(0, 0, "RingBuf_Init() failed");
        return -1;
    }
    // Open Index file (to make code clean, not to close the file)
    if (AmpCFS_Stat(Encoder->szHdrName, &stat) != AMP_OK) {
        DmyEnc_Perror(0, 0, "AmpCFS_Stat() failed");
        return -1;
    }
    Encoder->nHDRFlen = stat.Size;
    if (DummyAudioEnc_OpenFile(Encoder) < 0) {
        DmyEnc_Perror(0, 0, "DummyAudioEnc_OpenFile() failed");
        return -1;
    }
    if (AmbaKAL_EventFlagCreate(&Encoder->EncStop) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_EventFlagCreate() failed");
        return -1;
    }
    /* Create Encode task */
    if (AmbaKAL_TaskCreate(&Encoder->EncTask, "DummyAudioEnc", DUMMY_AUDIO_ENC_PRIORITY, DummyAudioEnc_EncTask, (UINT32)Encoder, (void *)Encoder->EncStack, sizeof(Encoder->EncStack), AMBA_KAL_DO_NOT_START) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_TaskCreate() failed");
        return -1;
    }
    return 0;
}

int DummyAudioEnc_Reset(DummyAudioEnc_s *Encoder, AMP_FIFO_HDLR_s *pFifo)
{
    if (DummyAudioEnc_CloseFile(Encoder) < 0) {
        DmyEnc_Perror(0, 0, "DummyAudioEnc_CloseFile() failed");
        return -1;
    }
    if (DummyAudioEnc_OpenFile(Encoder) < 0) {
        DmyEnc_Perror(0, 0, "DummyAudioEnc_OpenFile() failed");
        return -1;
    }
    RingBuf_Reset(&Encoder->RingBuf);
    // reset tasks
    Encoder->pFifo = pFifo;
    if (AmbaKAL_TaskReset(&Encoder->EncTask) != OK) {
//        DmyEnc_Perror(0, 0);
//        return -1;
    }
    return 0;
}

int DummyAudioEnc_Start(DummyAudioEnc_s *Encoder)
{
    Encoder->State = ENCODER_STATE_RUNNING;
    if (AmbaKAL_EventFlagClear(&Encoder->EncStop, 0x00000001) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_EventFlagClear() failed");
        return -1;
    }
    if (AmbaKAL_TaskResume(&Encoder->EncTask) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_TaskResume() failed");
        return -1;
    }
    return 0;
}

int DummyAudioEnc_Stop(DummyAudioEnc_s *Encoder)
{
    UINT32 flag;
    Encoder->State = ENCODER_STATE_IDLE;
    if (AmbaKAL_EventFlagTake(&Encoder->EncStop, 0x00000001, AMBA_KAL_AND_CLEAR, &flag, AMBA_KAL_WAIT_FOREVER) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_EventFlagTake() failed");
        return -1;
    }
    return 0;
}

int DummyAudioEnc_Pause(DummyAudioEnc_s *Encoder)
{
    Encoder->State = ENCODER_STATE_PAUSING;
    return 0;
}

int DummyAudioEnc_Resume(DummyAudioEnc_s *Encoder)
{
    Encoder->State = ENCODER_STATE_RESUME;
    return 0;
}

