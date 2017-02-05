#include "DummyVideoEnc.h"
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

static int DummyVideoEnc_RingBufCB(void *hdlr, UINT32 event, void* info)
{
//    AmbaPrint("%s on Event: 0x%x", __FUNCTION__, event);

    return 0;
}

static int DummyVideoEnc_OpenFile(DummyVideoEnc_s *pEncoder)
{
    AMP_CFS_FILE_PARAM_s cfsParam;
    if (AmpCFS_GetFileParam(&cfsParam) != AMP_OK) {
        DmyEnc_Perror(0, 0, "AmpCFS_GetFileParam() failed");
        return -1;
    }
    cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    strncpy(cfsParam.Filename, pEncoder->szHdrName, MAX_FILENAME_LENGTH);
    cfsParam.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
    pEncoder->pIdxFile = AmpCFS_fopen(&cfsParam);
    if (pEncoder->pIdxFile == NULL) {
        DmyEnc_Perror(0, 0, "AmpCFS_fopen() failed");
        return -1;
    }
    // Open Raw file (to make code clean, not to close the file)
    if (AmpCFS_GetFileParam(&cfsParam) != AMP_OK) {
        DmyEnc_Perror(0, 0, "AmpCFS_GetFileParam() failed");
        return -1;
    }
    cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    strncpy(cfsParam.Filename, pEncoder->szBsName, MAX_FILENAME_LENGTH);
    cfsParam.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
    pEncoder->pRawFile = AmpCFS_fopen(&cfsParam);
    if (pEncoder->pRawFile == NULL) {
        DmyEnc_Perror(0, 0, "AmpCFS_fopen() failed");
        return -1;
    }
    return 0;
}

static int DummyVideoEnc_CloseFile(DummyVideoEnc_s *pEncoder)
{
    int rval = 0;
    if (pEncoder->pIdxFile != NULL) {
        if (AmpCFS_fclose(pEncoder->pIdxFile) != AMP_OK) {
            DmyEnc_Perror(0, 0, "AmpCFS_fclose() failed");
            rval = -1;
        }
        pEncoder->pIdxFile = NULL;
    }
    if (pEncoder->pRawFile != NULL) {
        if (AmpCFS_fclose(pEncoder->pRawFile) != AMP_OK) {
            DmyEnc_Perror(0, 0, "AmpCFS_fclose() failed");
            rval = -1;
        }
        pEncoder->pRawFile = NULL;
    }
    return rval;
}

static void DummyVideoEnc_EncTask(UINT32 info)
{
    int rval = 0;
    DummyVideoEnc_s * const pEncoder = (DummyVideoEnc_s *)info;
    UINT32 nFrameCount = 0;
    UINT64 nTotalSize = 0;
    UINT32 nIdxLen = 0;
    RING_BUFFER_INFO_s RingBufInfo = {0};
    AMP_BITS_DESC_s tmpDesc = {0};
    AMP_FIFO_INFO_s tmpInfo = {0};

    MP4_NHNT_SAMPLE_HEADER_s nhntDesc = {0};
    MP4_NHNT_HEADER_s nhntHeader = {0};

    nIdxLen = AmpCFS_fread(&nhntHeader, 1, sizeof(nhntHeader), pEncoder->pIdxFile);
    if (nIdxLen < sizeof(nhntHeader)) {
        DmyEnc_Perror(0, 0, "AmpCFS_fread() failed");
        goto Done;
    }

    AmbaPrint("Raw File sizeof(MP4_NHNT_HEADER_s) = (%d):", sizeof(MP4_NHNT_HEADER_s));
    AmbaPrint("Symbol:%c%c%c%c", nhntHeader.Signature[0], nhntHeader.Signature[1], nhntHeader.Signature[2], nhntHeader.Signature[3]);
    AmbaPrint("avgBitRate:%d", nhntHeader.avgBitRate);
    AmbaPrint("maxBitRate:%d", nhntHeader.maxBitRate);
    AmbaPrint("timeStampResolution:%d", nhntHeader.timeStampResolution);

    while (pEncoder->nState != ENCODER_STATE_IDLE) {
        if (AmpFifo_GetInfo(pEncoder->pFifo, &tmpInfo) != AMP_OK) {
            DmyEnc_Perror(0, 0, "AmpFifo_GetInfo() failed");
            rval = -1;
            goto Done;
        }
        if (tmpInfo.AvailEntries < tmpInfo.TotalEntries) {
            if (AmpCFS_ftell(pEncoder->pIdxFile) == pEncoder->nHDRFlen) {
                AmbaPrint("Reach Video EOS");
                memset (&tmpDesc, 0, sizeof(tmpDesc));
                tmpDesc.Size = AMP_FIFO_MARK_EOS;// EOS magic num // TBD
                if (AmpFifo_WriteEntry(pEncoder->pFifo, &tmpDesc) != AMP_OK) {
                    DmyEnc_Perror(0, 0, "AmpFifo_WriteEntry() failed");
                    rval = -1;
                    goto Done;
                }
                break;
            } else {
                /**< Read frame header */
                nIdxLen = AmpCFS_fread(&nhntDesc, 1, sizeof(nhntDesc), pEncoder->pIdxFile);
                if (nIdxLen < sizeof(nhntDesc)) {
                    DmyEnc_Perror(0, 0, "AmpCFS_fread() failed");
                    rval = -1;
                    goto Done;
                }
                tmpDesc.Pts = nhntDesc.compositionTimeStamp;
                tmpDesc.Completed = 1;
                tmpDesc.Size = nhntDesc.dataSize;
                tmpDesc.Type = (AMP_FIFO_FRMAE_TYPE_e)nhntDesc.frameType;
                tmpDesc.SeqNum = nFrameCount;
                nTotalSize += tmpDesc.Size;
                while (TRUE) {
                    if (pEncoder->nState == ENCODER_STATE_PAUSING) {
                        AMP_BITS_DESC_s eos = {0};
                        if (tmpDesc.Type != AMP_FIFO_TYPE_IDR_FRAME)
                            break;
                        eos.Size = AMP_FIFO_MARK_EOS_PAUSE;
                        AmbaPrint("Reach Video PAUSE");
                        AmpFifo_WriteEntry(pEncoder->pFifo, &eos);
                        pEncoder->nState = ENCODER_STATE_PAUSED;
                        AmbaKAL_TaskSleep(200);
                    } else if (pEncoder->nState == ENCODER_STATE_RESUME) {
                        pEncoder->nState = ENCODER_STATE_RUNNING;
                        break;
                    } else if (pEncoder->nState == ENCODER_STATE_PAUSED) {
                        AmbaKAL_TaskSleep(200);
                    } else {
                        break;
                    }
                }

                //AmbaPrint("Fifo[%p] Read %u: pts:%llu, ftype:%u, size:%u, total size:%llu", pEncoder->pFifo, nFrameCount, tmpDesc.Pts, nhntDesc.frameType, tmpDesc.Size, nTotalSize);

                /**< Read frame from BitStream*/
                RingBuf_GetInfo(&pEncoder->RingBuf, &RingBufInfo);
                tmpDesc.Align = 0;
                tmpDesc.StartAddr = RingBufInfo.pRingBufWp;

                if (AmpCFS_fseek(pEncoder->pRawFile, nhntDesc.fileOffset, AMP_CFS_SEEK_START) != AMP_OK) {
                    DmyEnc_Perror(0, 0, "AmpCFS_fseek() failed");
                    rval = -1;
                    goto Done;
                }
                while (TRUE) {
                    const int r = RingBuf_CreateFrame(&pEncoder->RingBuf, pEncoder->pRawFile, tmpDesc.Size);
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
                if (AmpFifo_WriteEntry(pEncoder->pFifo, &tmpDesc) != AMP_OK) {
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
    DummyVideoEnc_CloseFile(pEncoder);
    if (AmbaKAL_EventFlagGive(&pEncoder->EncStop, 0x00000001) != OK)
        DmyEnc_Perror(0, 0, "AmbaKAL_EventFlagGive() failed");
    AmbaPrint("%s() is stopped", __FUNCTION__);
    if (rval == -1)
        AmbaPrint("[FAIL]: Muxing Failed!");
}

UINT32 DummyVideoEnc_GetRequiredBufSize(UINT32 nRawBufSize, UINT32 nMaxQueuedFrame)
{
    return RingBuf_GetRequiredBufSize(nRawBufSize, nMaxQueuedFrame);
}

int DummyVideoEnc_Init(DummyVideoEnc_s *pEncoder, char *szBsName, char *szHdrName, UINT8 *pBuffer, UINT32 nBufferSize, UINT32 nMaxQueuedFrame)
{
    AMP_CFS_STAT stat;
    const UINT32 nRawBufSize = nBufferSize - nMaxQueuedFrame * sizeof(RING_BUFFER_DESC_s);
    K_ASSERT(nBufferSize == DummyVideoEnc_GetRequiredBufSize(nRawBufSize, nMaxQueuedFrame));
    memset(pEncoder, 0, sizeof(DummyVideoEnc_s));
    strncpy(pEncoder->szBsName, szBsName, MAX_FILENAME_LENGTH);
    pEncoder->szBsName[MAX_FILENAME_LENGTH - 1] = '\0';
    strncpy(pEncoder->szHdrName, szHdrName, MAX_FILENAME_LENGTH);
    pEncoder->szHdrName[MAX_FILENAME_LENGTH - 1] = '\0';
    pEncoder->nState = ENCODER_STATE_IDLE;
    pEncoder->pBuffer = pBuffer;
    if (RingBuf_Init(&pEncoder->RingBuf, (UINT8 *)pEncoder->pBuffer, nBufferSize, nMaxQueuedFrame, DummyVideoEnc_RingBufCB) != 0) {
        DmyEnc_Perror(0, 0, "RingBuf_Init() failed");
        return -1;
    }
    // Open Index file (to make code clean, not to close the file)
    if (AmpCFS_Stat(pEncoder->szHdrName, &stat) != AMP_OK) {
        DmyEnc_Perror(0, 0, "AmpCFS_Stat() failed");
        return -1;
    }
    pEncoder->nHDRFlen = stat.Size;
    if (DummyVideoEnc_OpenFile(pEncoder) < 0) {
        DmyEnc_Perror(0, 0, "DummyVideoEnc_OpenFile() failed");
        return -1;
    }
    if (AmbaKAL_EventFlagCreate(&pEncoder->EncStop) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_EventFlagCreate() failed");
        return -1;
    }
    /* Create Encode task */
    if (AmbaKAL_TaskCreate(&pEncoder->EncTask, "DummyVideoEnc", DUMMY_VIDEO_ENC_PRIORITY, DummyVideoEnc_EncTask, (UINT32)pEncoder, (void *)pEncoder->EncStack, sizeof(pEncoder->EncStack), AMBA_KAL_DO_NOT_START) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_TaskCreate() failed");
        return -1;
    }
    return 0;
}

int DummyVideoEnc_Reset(DummyVideoEnc_s *pEncoder, AMP_FIFO_HDLR_s *pFifo)
{
    if (DummyVideoEnc_CloseFile(pEncoder) < 0) {
        DmyEnc_Perror(0, 0, "DummyVideoEnc_CloseFile() failed");
        return -1;
    }
    if (DummyVideoEnc_OpenFile(pEncoder) < 0) {
        DmyEnc_Perror(0, 0, "DummyVideoEnc_OpenFile() failed");
        return -1;
    }
    RingBuf_Reset(&pEncoder->RingBuf);
    // reset tasks
    pEncoder->pFifo = pFifo;
    if (AmbaKAL_TaskReset(&pEncoder->EncTask) != OK) {
//        DmyEnc_Perror(0, 0);
//        return -1;
    }
    return 0;
}

int DummyVideoEnc_Start(DummyVideoEnc_s *pEncoder)
{
    pEncoder->nState = ENCODER_STATE_RUNNING;
    if (AmbaKAL_EventFlagClear(&pEncoder->EncStop, 0x00000001) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_EventFlagClear() failed");
        return -1;
    }
    if (AmbaKAL_TaskResume(&pEncoder->EncTask) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_TaskResume() failed");
        return -1;
    }
    return 0;
}

int DummyVideoEnc_Stop(DummyVideoEnc_s *pEncoder)
{
    UINT32 flag;
    pEncoder->nState = ENCODER_STATE_IDLE;
    if (AmbaKAL_EventFlagTake(&pEncoder->EncStop, 0x00000001, AMBA_KAL_AND_CLEAR, &flag, AMBA_KAL_WAIT_FOREVER) != OK) {
        DmyEnc_Perror(0, 0, "AmbaKAL_EventFlagTake() failed");
        return -1;
    }
    return 0;
}

int DummyVideoEnc_Pause(DummyVideoEnc_s *pEncoder)
{
    pEncoder->nState = ENCODER_STATE_PAUSING;
    return 0;
}

int DummyVideoEnc_Resume(DummyVideoEnc_s *pEncoder)
{
    pEncoder->nState = ENCODER_STATE_RESUME;
    return 0;
}

