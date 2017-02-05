/**
 *  @file ExtStream.c
 *
 *  Ext stream implementation
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2014/12/23 |yhlee       |Created        |
 *
 *
 *  @copyright 2014 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */
#include "ExtStream.h"
#include <cfs/AmpCfs.h>
#include <format/Format.h>

#define ExtStream_Perror(level, color, fmt, ...) {\
    char Buffer[160];\
    snprintf(Buffer, 160, fmt, ##__VA_ARGS__);\
    Buffer[159] = '\0';\
    AmbaPrintColor(color, "[Error]%s:%u %s", __FUNCTION__, __LINE__, Buffer);\
}

#define DEFAULT_FIO_RATE    (10 * 1024)

typedef struct {
    AMP_STREAM_HDLR_s Hdlr;
    AMBA_KAL_MUTEX_t Mutex; /**< to protect handler's internal variables */ //  TODO: if performance not good, remove it
    AMP_CFS_FILE_s *File;
    char URL[MAX_FILENAME_LENGTH];
    AMP_STREAM_EXT_MEDIA_INFO_s Info;
    UINT32 Alignment;  /**< pre-allocation */
    UINT32 BytesToSync;
    AMP_CFS_FILE_PARAM_ASYNC_s AsyncParam;
    BOOL8 Async;   /**< async mode */
    BOOL8 LowPriority;
    UINT8 Mode;
} AMP_EXT_STREAM_HDLR_s;

typedef struct {
    AMBA_KAL_MUTEX_t Mutex; /**< to protect handler */
    AMBA_KAL_BLOCK_POOL_t HdlrBpl;
    BOOL8 Init;
} AMP_EXT_STREAM_MGR_s;

static AMP_EXT_STREAM_MGR_s g_FileMgr = {0};
static AMP_STREAM_s g_File;

/**
 * Get the default file stream manager configure
 * @param [out] Config the buffer to get default configure
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
int AmpExtStream_GetInitDefaultCfg(AMP_EXT_STREAM_INIT_CFG_s *config)
{
    K_ASSERT(config != NULL);
    memset(config, 0, sizeof(AMP_EXT_STREAM_INIT_CFG_s));
    config->MaxHdlr = 1;
    config->BufferSize = AmpExtStream_GetRequiredBufferSize(config->MaxHdlr);
    return AMP_OK;
}

/**
 * Get the required buffer size
 * @param [in] MaxHdlr the number of file streams
 * @return the required buffer size
 */
UINT32 AmpExtStream_GetRequiredBufferSize(UINT8 maxHdlr)
{
    K_ASSERT(maxHdlr > 0);
    return (UINT32)maxHdlr * (sizeof(AMP_EXT_STREAM_HDLR_s) + sizeof(void *));
}

/**
 * Initiate the core of file stream
 * @param [in] Config the initial configure
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
int AmpExtStream_Init(AMP_EXT_STREAM_INIT_CFG_s *config)
{
    K_ASSERT(config != NULL);
    K_ASSERT(config->MaxHdlr > 0);
    K_ASSERT(config->Buffer != NULL);
    K_ASSERT(config->BufferSize == AmpExtStream_GetRequiredBufferSize(config->MaxHdlr));
    if (g_FileMgr.Init == FALSE) {
        memset(&g_FileMgr, 0, sizeof(AMP_EXT_STREAM_MGR_s));
        if (AmbaKAL_MutexCreate(&g_FileMgr.Mutex) == OK) {
            if (AmbaKAL_BlockPoolCreate(&g_FileMgr.HdlrBpl, sizeof(AMP_EXT_STREAM_HDLR_s), config->Buffer, config->BufferSize) == OK) {
                g_FileMgr.Init = TRUE;
                return AMP_OK;
            }
            ExtStream_Perror(0, 0, "Create block pool failed!");
            AmbaKAL_MutexDelete(&g_FileMgr.Mutex);
        } else {
            ExtStream_Perror(0, 0, "Create mutex failed!");
        }
    } else {
        ExtStream_Perror(0, 0, "Already initiated!");
    }
    return AMP_ERROR_GENERAL_ERROR;
}

/**
 * Get the default file stream configure
 * @param [out] Config the buffer to get default configure
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
int AmpExtStream_GetDefaultCfg(AMP_EXT_STREAM_CFG_s *config)
{
    AMP_CFS_FILE_PARAM_s Param;
    int rval;
    K_ASSERT(config != NULL);
    memset(config, 0, sizeof(AMP_EXT_STREAM_CFG_s));
    rval = AmpCFS_GetFileParam(&Param);
    if (rval == AMP_OK) {
        config->BytesToSync = Param.BytesToSync;
        memcpy(&config->AsyncParam, &Param.AsyncData, sizeof(config->AsyncParam));
        return AMP_OK;
    }
    ExtStream_Perror(0, 0, "Get default configuration failed!");
    return rval;
}

static AMP_EXT_STREAM_HDLR_s *AmpExtStream_CreateImpl(AMP_EXT_STREAM_CFG_s *config)
{
    void *Buffer = NULL;
    if (AmbaKAL_BlockPoolAllocate(&g_FileMgr.HdlrBpl, &Buffer, AMBA_KAL_NO_WAIT) == OK) {
        AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)Buffer;
        memset(FileStr, 0, sizeof(AMP_EXT_STREAM_HDLR_s));
        if (AmbaKAL_MutexCreate(&FileStr->Mutex) == OK) {
            FileStr->Hdlr.Func = &g_File;
            FileStr->File = NULL;
            FileStr->Alignment = config->Alignment;
            FileStr->BytesToSync = config->BytesToSync;
            memcpy(&FileStr->AsyncParam, &config->AsyncParam, sizeof(FileStr->AsyncParam));
            FileStr->Async = config->Async;
            FileStr->LowPriority = config->LowPriority;
            return FileStr;
        }
        ExtStream_Perror(0, 0, "Create mutex failed!");
        AmbaKAL_BlockPoolFree(FileStr);
    } else {
        ExtStream_Perror(0, 0, "Allocate block pool failed!");
    }
    return NULL;
}

/**
 * Create a file stream
 * @param [in] Config the configure of the stream
 * @param [out] Hdlr the double pointer to get the handler
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
int AmpExtStream_Create(AMP_EXT_STREAM_CFG_s *config, AMP_STREAM_HDLR_s **hdlr)
{
    int rval = AMP_ERROR_GENERAL_ERROR;
    K_ASSERT(config != NULL);
    K_ASSERT(hdlr != NULL);
    if (AmbaKAL_MutexTake(&g_FileMgr.Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        AMP_EXT_STREAM_HDLR_s *const FileStr = AmpExtStream_CreateImpl(config);
        if (FileStr != NULL) {
            *hdlr = (AMP_STREAM_HDLR_s *)FileStr;
            rval = AMP_OK;
        }
        AmbaKAL_MutexGive(&g_FileMgr.Mutex);
    }
    return rval;
}

static int AmpExtStream_DeleteImpl(AMP_EXT_STREAM_HDLR_s *hdlr)
{
    int rval = AMP_OK;
    if (AmbaKAL_MutexTake(&hdlr->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        // delete mutex
        if (AmbaKAL_MutexDelete(&hdlr->Mutex) != OK) {
            ExtStream_Perror(0, 0, "Delete mutex failed!");
            rval = AMP_ERROR_GENERAL_ERROR;
        }
        if (AmbaKAL_BlockPoolFree(hdlr) != OK) {
            ExtStream_Perror(0, 0, "Release block pool failed!");
            rval = AMP_ERROR_GENERAL_ERROR;
        }
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
        rval = AMP_ERROR_GENERAL_ERROR;
    }
    return rval;
}

/**
 * Delete a file stream
 * @param [in] Hdlr the stream handler to close
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
int AmpExtStream_Delete(AMP_STREAM_HDLR_s *hdlr)
{
    int rval = AMP_ERROR_GENERAL_ERROR;
    K_ASSERT(hdlr != NULL);
    if (AmbaKAL_MutexTake(&g_FileMgr.Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        rval = AmpExtStream_DeleteImpl((AMP_EXT_STREAM_HDLR_s *)hdlr);
        AmbaKAL_MutexGive(&g_FileMgr.Mutex);
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return rval;
}

static int AmpExtStream_Open(AMP_STREAM_HDLR_s *hdlr, char *name, UINT32 mode)
{
    int rval = AMP_ERROR_GENERAL_ERROR;
    AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)hdlr;
    K_ASSERT(FileStr != NULL);
    K_ASSERT(FileStr->File == NULL);
    if (AmbaKAL_MutexTake(&FileStr->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        AMP_CFS_FILE_PARAM_s Param;
        rval = AmpCFS_GetFileParam(&Param);
        if (rval == AMP_OK) {
            strncpy(Param.Filename, name, MAX_FILENAME_LENGTH);
            Param.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
            Param.Alignment = FileStr->Alignment;
            Param.BytesToSync = FileStr->BytesToSync;
            Param.AsyncMode = FileStr->Async;
            Param.LowPriority = FileStr->LowPriority;
            memcpy(&Param.AsyncData, &FileStr->AsyncParam, sizeof(Param.AsyncData));
            switch (mode) {
            case AMP_STREAM_MODE_RDONLY:
                Param.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
                FileStr->File = AmpCFS_fopen(&Param);
                break;
            case AMP_STREAM_MODE_WRONLY:
                Param.Mode = AMP_CFS_FILE_MODE_WRITE_ONLY;
                FileStr->File = AmpCFS_fopen(&Param);
                break;
            case AMP_STREAM_MODE_RDWR:
                Param.Mode = AMP_CFS_FILE_MODE_READ_WRITE;
                FileStr->File = AmpCFS_fopen(&Param);
                break;
            case AMP_STREAM_MODE_WRRD:
                Param.Mode = AMP_CFS_FILE_MODE_WRITE_READ;
                FileStr->File = AmpCFS_fopen(&Param);
                break;
            default:
                FileStr->File = NULL;
                break;
            }
            if (FileStr->File != NULL) {
                if (AmpCFS_FSync(FileStr->File) == AMP_OK) {
                    FileStr->Mode = mode;
                    strncpy(FileStr->URL, name, MAX_FILENAME_LENGTH);
					FileStr->URL[MAX_FILENAME_LENGTH - 1] = '\0';
                    rval = AMP_OK;
                } else {
                    ExtStream_Perror(0, 0, "AmpCFS_FSync() failed!");
                    AmpCFS_fclose(FileStr->File);
                    FileStr->File = NULL;
                    AmpCFS_remove(name);
                    rval = AMP_ERROR_IO_ERROR;
                }
            } else {
                ExtStream_Perror(0, 0, "File is null!");
                rval = AMP_ERROR_IO_ERROR;
            }
        } else {
            ExtStream_Perror(0, 0, "AmpCFS_GetFileParam() failed!");
        }
        AmbaKAL_MutexGive(&FileStr->Mutex);
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return rval;
}

static int AmpExtStream_Close(AMP_STREAM_HDLR_s *hdlr)
{
    int rval = AMP_ERROR_GENERAL_ERROR;
    AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)hdlr;
    K_ASSERT(FileStr != NULL);
    if (AmbaKAL_MutexTake(&FileStr->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        if (FileStr->File)    // could be NULL due to cinsert failure
            rval = AmpCFS_fclose(FileStr->File);
        else
            rval = AMP_OK;
        FileStr->File = NULL;
        AmbaKAL_MutexGive(&FileStr->Mutex);
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return rval;
}

static int AmpExtStream_Read(AMP_STREAM_HDLR_s *hdlr, UINT32 size, UINT8 *buffer)
{
    int rval = -1;
    AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)hdlr;
    K_ASSERT(FileStr != NULL);
    K_ASSERT(FileStr->File != NULL);
    K_ASSERT(size > 0);
    K_ASSERT(buffer != NULL);
    if (AmbaKAL_MutexTake(&FileStr->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        rval = AmpCFS_fread(buffer, 1, size, FileStr->File);
        if (rval == 0) {
            const INT64 Pos = AmpCFS_ftell(FileStr->File);
            if (Pos != AmpCFS_FGetLen(FileStr->File))
                rval = -1;
        }
        AmbaKAL_MutexGive(&FileStr->Mutex);
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return rval;
}

static int AmpExtStream_Write(AMP_STREAM_HDLR_s *hdlr, UINT32 size, UINT8 *buffer)
{
    int rval = -1;
    AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)hdlr;
    K_ASSERT(FileStr != NULL);
    K_ASSERT(FileStr->File != NULL);
    K_ASSERT(size > 0);
    K_ASSERT(buffer != NULL);
    if (AmbaKAL_MutexTake(&FileStr->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        rval = AmpCFS_fwrite(buffer, 1, size, FileStr->File);
        if (rval != size)
            ExtStream_Perror(0, 0, "AmpCFS_fwrite() failed!");
        AmbaKAL_MutexGive(&FileStr->Mutex);
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return rval;
}

static INT64 AmpExtStream_GetPos(AMP_STREAM_HDLR_s *hdlr)
{
    INT64 rval = -1;
    AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)hdlr;
    K_ASSERT(FileStr != NULL);
    K_ASSERT(FileStr->File != NULL);
    if (AmbaKAL_MutexTake(&FileStr->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        rval = AmpCFS_ftell(FileStr->File);
        if (rval < 0)
            ExtStream_Perror(0, 0, "AmpCFS_ftell() failed!");
        AmbaKAL_MutexGive(&FileStr->Mutex);
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return rval;
}

static INT64 AmpExtStream_GetLength(AMP_STREAM_HDLR_s *hdlr)
{
    INT64 rval = -1;
    AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)hdlr;
    K_ASSERT(FileStr != NULL);
    K_ASSERT(FileStr->File != NULL);
    if (AmbaKAL_MutexTake(&FileStr->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        rval = AmpCFS_FGetLen(FileStr->File);
        if (rval < 0)
            ExtStream_Perror(0, 0, "AmpCFS_FGetLen() failed!");
        AmbaKAL_MutexGive(&FileStr->Mutex);
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return rval;
}

static UINT64 AmpExtStream_GetFreeSpace(AMP_STREAM_HDLR_s *hdlr)
{
    AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)hdlr;
    AMP_CFS_DEVINF DevInf;
    if (AmpCFS_GetDev(FileStr->URL[0], &DevInf) == AMP_OK) {
        const UINT64 space = (UINT64)DevInf.Ucl * DevInf.Spc * DevInf.Bps;
        UINT64 cached;
        if (AmpCFS_GetCachedDataSize(FileStr->URL[0], &cached) == AMP_OK)
            return space - cached;
        ExtStream_Perror(0, 0, "AmpCFS_GetCachedDataSize() failed!");
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return 0;
}

static int AmpExtStream_Seek(AMP_STREAM_HDLR_s *hdlr, UINT64 pos, int orig)
{
    return 0;
}

static UINT32 AmpExtStream_GetFrameCount(AMP_EXT_STREAM_HDLR_s *hdlr, UINT8 trackId)
{
    AMP_STREAM_EXT_MOVIE_INFO_s *Movie;
    AMP_STREAM_EXT_IMAGE_INFO_s *Image;
    AMP_STREAM_EXT_SOUND_INFO_s *Sound;
    switch (hdlr->Info.MediaType) {
    case AMP_MEDIA_INFO_MOVIE:
        Movie = &hdlr->Info.Info.Movie;
        if (trackId < Movie->TrackCount)
            return Movie->Track[trackId].FrameCount;
        ExtStream_Perror(0, 0, "Incorrect tracak count!");
        break;
    case AMP_MEDIA_INFO_IMAGE:
        Image = &hdlr->Info.Info.Image;
        return Image->UsedFrame;
    case AMP_MEDIA_INFO_SOUND:
        Sound = &hdlr->Info.Info.Sound;
        return Sound->Track[trackId].FrameCount;
    default:
        ExtStream_Perror(0, 0, "Incorrect media type!");
        break;
    }
    return 0;
}

char EXT_TEST_SUFFIX[20] = {'_','H','D','R','\0'};
static int AmpExtStream_SetMediaInfo(AMP_EXT_STREAM_HDLR_s *hdlr, AMP_STREAM_EXT_MEDIA_INFO_s *info)
{
    AMP_CFS_FILE_PARAM_s Param;
    int rval = AmpCFS_GetFileParam(&Param);
    if (rval == AMP_OK) {
        AMP_CFS_FILE_s *File;
        char szHdr[MAX_FILENAME_LENGTH];
        memset(szHdr, 0, sizeof(szHdr));
        strncpy(szHdr, hdlr->URL, MAX_FILENAME_LENGTH - strlen(EXT_TEST_SUFFIX));
        szHdr[MAX_FILENAME_LENGTH - strlen(EXT_TEST_SUFFIX) - 1] = '\0';
        strcat(szHdr, EXT_TEST_SUFFIX);
        strncpy(Param.Filename, szHdr, MAX_FILENAME_LENGTH);
        Param.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
        Param.Mode = AMP_CFS_FILE_MODE_READ_WRITE;
        File = AmpCFS_fopen(&Param);
        if (File != NULL) {
            if (AmpCFS_fwrite(info, sizeof(AMP_STREAM_EXT_MEDIA_INFO_s), 1, File) == 1) {
                memcpy(&hdlr->Info, info, sizeof(AMP_STREAM_EXT_MEDIA_INFO_s));
                rval = AMP_OK;
            } else {
                ExtStream_Perror(0, 0, "AmpCFS_fwrite() failed!");
                rval = AMP_ERROR_IO_ERROR;
            }
            AmpCFS_fclose(File);
        } else {
            ExtStream_Perror(0, 0, "File is null!");
            rval = AMP_ERROR_IO_ERROR;
        }
    } else {
        ExtStream_Perror(0, 0, "AmpCFS_GetFileParam() failed!");
    }
    return rval;
}

static INT64 AmpExtStream_GetFramePos(AMP_EXT_STREAM_HDLR_s *hdlr, AMP_STREAM_EXT_FRAME_INFO_s *frame)
{
    INT64 rval = -1;
    switch (hdlr->Info.MediaType) {
    case AMP_MEDIA_INFO_MOVIE:
        rval = sizeof(AMP_STREAM_EXT_MEDIA_INFO_s);
        rval += (sizeof(AMP_STREAM_EXT_FRAME_INFO_s) + sizeof(INT64)) * (hdlr->Info.Info.Movie.TrackCount * frame->FrameNo + frame->TrackId);
        break;
    case AMP_MEDIA_INFO_IMAGE:
    case AMP_MEDIA_INFO_SOUND:
        rval = sizeof(AMP_STREAM_EXT_MEDIA_INFO_s);
        rval += (sizeof(AMP_STREAM_EXT_FRAME_INFO_s) + sizeof(INT64)) * frame->FrameNo;
        break;
    default:
        ExtStream_Perror(0, 0, "Incorrect media type!");
        break;
    }
    return rval;
}

static int AmpExtStream_SetFrameInfo(AMP_EXT_STREAM_HDLR_s *hdlr, AMP_STREAM_EXT_FRAME_INFO_s *frame)
{
    AMP_CFS_FILE_PARAM_s Param;
    int rval = AmpCFS_GetFileParam(&Param);
    if (rval == AMP_OK) {
        AMP_CFS_FILE_s *File;
        char HdrFn[MAX_FILENAME_LENGTH];
        memset(HdrFn, 0, sizeof(HdrFn));
        strncpy(HdrFn, hdlr->URL, MAX_FILENAME_LENGTH - strlen(EXT_TEST_SUFFIX));
        HdrFn[MAX_FILENAME_LENGTH - strlen(EXT_TEST_SUFFIX) - 1] = '\0';
        strcat(HdrFn, EXT_TEST_SUFFIX);
        strncpy(Param.Filename, HdrFn, MAX_FILENAME_LENGTH);
        Param.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
        Param.Mode = AMP_CFS_FILE_MODE_READ_WRITE;
        File = AmpCFS_fopen(&Param);
        if (File != NULL) {
            const INT64 FramePos = AmpExtStream_GetFramePos(hdlr, frame);
            //AmbaPrint("%s, FramePos = %d", __FUNCTION__, FramePos);
            if (FramePos >= 0) {
                rval = AmpCFS_fseek(File, FramePos, AMP_CFS_SEEK_START);
                if (rval == AMP_OK) {
                    if (AmpCFS_fwrite(frame, sizeof(AMP_STREAM_EXT_FRAME_INFO_s), 1, File) == 1) {
                        const INT64 FilePos = AmpCFS_ftell(hdlr->File);
                        if (FilePos >= 0) {
                            //AmbaPrint("%s() : FrameNo=%u, FilePos=%lld", __FUNCTION__, frame->FrameNo, FilePos);
                            if (AmpCFS_fwrite(&FilePos, sizeof(INT64), 1, File) == 1) {
                                rval = AMP_OK;
                            } else {
                                ExtStream_Perror(0, 0, "AmpCFS_fwrite() failed!");
                                rval = AMP_ERROR_IO_ERROR;
                            }
                        } else {
                            ExtStream_Perror(0, 0, "FilePos error!");
                            rval = AMP_ERROR_IO_ERROR;
                        }
                    } else {
                        ExtStream_Perror(0, 0, "AmpCFS_fwrite() failed!");
                        rval = AMP_ERROR_IO_ERROR;
                    }
                } else {
                    ExtStream_Perror(0, 0, "AmpCFS_fseek() failed!");
                }
            } else {
                ExtStream_Perror(0, 0, "FilePos error!");
                rval = AMP_ERROR_GENERAL_ERROR;
            }
            AmpCFS_fclose(File);
        } else {
            ExtStream_Perror(0, 0, "File is NULL!");
            rval = AMP_ERROR_IO_ERROR;
        }
    } else {
        ExtStream_Perror(0, 0, "AmpCFS_GetFileParam() failed!");
    }
    return rval;
}

static int AmpExtStream_GetMediaInfo(AMP_EXT_STREAM_HDLR_s *hdlr, AMP_STREAM_EXT_MEDIA_INFO_s *info)
{
    AMP_CFS_FILE_PARAM_s Param;
    int rval = AmpCFS_GetFileParam(&Param);
    if (rval == AMP_OK) {
        AMP_CFS_FILE_s *File;
        char HdrFn[MAX_FILENAME_LENGTH];
        memset(HdrFn, 0, sizeof(HdrFn));
        strncpy(HdrFn, hdlr->URL, MAX_FILENAME_LENGTH - strlen(EXT_TEST_SUFFIX));
        HdrFn[MAX_FILENAME_LENGTH - strlen(EXT_TEST_SUFFIX) - 1] = '\0';
        strcat(HdrFn, EXT_TEST_SUFFIX);
        strncpy(Param.Filename, HdrFn, MAX_FILENAME_LENGTH);
        Param.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
        Param.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
        File = AmpCFS_fopen(&Param);
        if (File != NULL) {
            if (AmpCFS_fread(info, sizeof(AMP_STREAM_EXT_MEDIA_INFO_s), 1, File) == 1) {
                memcpy(&hdlr->Info, info, sizeof(AMP_STREAM_EXT_MEDIA_INFO_s));
                rval = AMP_OK;
            } else {
                ExtStream_Perror(0, 0, "AmpCFS_fread() failed!");
                rval = AMP_ERROR_IO_ERROR;
            }
            AmpCFS_fclose(File);
        } else {
            ExtStream_Perror(0, 0, "File is NULL!");
            rval = AMP_ERROR_IO_ERROR;
        }
    } else {
        ExtStream_Perror(0, 0, "AmpCFS_GetFileParam() failed!");
    }
    return rval;
}

static int AmpExtStream_GetFrameInfo(AMP_EXT_STREAM_HDLR_s *hdlr, AMP_STREAM_EXT_FRAME_INFO_s *frame)
{
    AMP_CFS_FILE_PARAM_s Param;
    int rval = AmpCFS_GetFileParam(&Param);
    if (rval == AMP_OK) {
        AMP_CFS_FILE_s *File;
        char HdrFn[MAX_FILENAME_LENGTH];
        memset(HdrFn, 0, sizeof(HdrFn));
        strncpy(HdrFn, hdlr->URL, MAX_FILENAME_LENGTH - strlen(EXT_TEST_SUFFIX));
        HdrFn[MAX_FILENAME_LENGTH - strlen(EXT_TEST_SUFFIX) - 1] = '\0';
        strcat(HdrFn, EXT_TEST_SUFFIX);
        strncpy(Param.Filename, HdrFn, MAX_FILENAME_LENGTH);
        Param.Filename[MAX_FILENAME_LENGTH - 1] = '\0';
        Param.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
        File = AmpCFS_fopen(&Param);
        if (File != NULL) {
            const UINT32 FrameCount = AmpExtStream_GetFrameCount(hdlr, frame->TrackId);
            if (FrameCount > 0) {
                if (frame->FrameNo >= FrameCount) {
                    frame->Size = AMP_FIFO_MARK_EOS;
                    rval = AmpCFS_fseek(hdlr->File, 0, AMP_CFS_SEEK_END);
                    if (rval != AMP_OK) {
                        ExtStream_Perror(0, 0, "AmpCFS_fseek() failed!");
                    }
                } else {
                    const INT64 FramePos = AmpExtStream_GetFramePos(hdlr, frame);
                    if (FramePos >= 0) {
                        rval = AmpCFS_fseek(File, FramePos, AMP_CFS_SEEK_START);
                        if (rval == AMP_OK) {
                            if (AmpCFS_fread(frame, sizeof(AMP_STREAM_EXT_FRAME_INFO_s), 1, File) == 1) {
                                INT64 FilePos;
                                if (AmpCFS_fread(&FilePos, sizeof(INT64), 1, File) == 1) {
                                    //AmbaPrint("%s() : FrameNo=%u, FilePos=%lld", __FUNCTION__, frame->FrameNo, FilePos);
                                    rval = AmpCFS_fseek(hdlr->File, FilePos, AMP_CFS_SEEK_START);
                                    if (rval != AMP_OK) {
                                        ExtStream_Perror(0, 0, "AmpCFS_fseek() failed!");
                                    }
                                } else {
                                    ExtStream_Perror(0, 0, "AmpCFS_fread() failed!");
                                    rval = AMP_ERROR_IO_ERROR;
                                }
                            } else {
                                ExtStream_Perror(0, 0, "AmpCFS_fread() failed!");
                                rval = AMP_ERROR_IO_ERROR;
                            }
                        } else {
                            ExtStream_Perror(0, 0, "AmpCFS_fseek() failed!");
                        }
                    } else {
                        ExtStream_Perror(0, 0, "Incorrect FramePos");
                        rval = AMP_ERROR_GENERAL_ERROR;
                    }
                }
            } else {
                ExtStream_Perror(0, 0, "Incorrect FrameCount");
                rval = AMP_ERROR_GENERAL_ERROR;
            }
            AmpCFS_fclose(File);
        } else {
            ExtStream_Perror(0, 0, "File is NULL!");
            rval = AMP_ERROR_IO_ERROR;
        }
    } else {
        ExtStream_Perror(0, 0, "AmpCFS_GetFileParam() failed!");
    }
    return rval;
}

static int AmpExtStream_Sync(AMP_EXT_STREAM_HDLR_s *hdlr)
{
    int rval = AMP_ERROR_GENERAL_ERROR;
    AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)hdlr;
    K_ASSERT(FileStr != NULL);
    K_ASSERT(FileStr->File != NULL);
    if (AmbaKAL_MutexTake(&FileStr->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        rval = AmpCFS_FSync(FileStr->File);
        if (rval != AMP_OK)
            ExtStream_Perror(0, 0, "AmpCFS_FSync() failed!");
        AmbaKAL_MutexGive(&FileStr->Mutex);
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return rval;
}

static int AmpExtStream_FuncImpl(AMP_EXT_STREAM_HDLR_s *hdlr, UINT32 cmd, UINT32 param)
{
    int rval = -1;
    K_ASSERT(hdlr != NULL);
    switch (cmd) {
    case AMP_STREAM_OP_SET_MEDIA_INFO:
        K_ASSERT(param != 0);
        rval = AmpExtStream_SetMediaInfo(hdlr, (AMP_STREAM_EXT_MEDIA_INFO_s *)param);
        break;
    case AMP_STREAM_OP_SET_FRAME_INFO:
        K_ASSERT(param != 0);
        rval = AmpExtStream_SetFrameInfo(hdlr, (AMP_STREAM_EXT_FRAME_INFO_s *)param);
        break;
    case AMP_STREAM_OP_GET_MEDIA_INFO:
        K_ASSERT(param != 0);
        rval = AmpExtStream_GetMediaInfo(hdlr, (AMP_STREAM_EXT_MEDIA_INFO_s *)param);
        break;
    case AMP_STREAM_OP_GET_FRAME_INFO:
        K_ASSERT(param != 0);
        rval = AmpExtStream_GetFrameInfo(hdlr, (AMP_STREAM_EXT_FRAME_INFO_s *)param);
        break;
    case AMP_STREAM_OP_SYNC:
        rval = AmpExtStream_Sync(hdlr);
        break;
    default:
        AmbaPrint("%s(%p, %x, %x) : unsupported!!", __FUNCTION__, hdlr, cmd, param);
        break;
    }
    return rval;
}

static int AmpExtStream_Func(AMP_STREAM_HDLR_s *hdlr, UINT32 cmd, UINT32 param)
{
    int rval = -1;
    AMP_EXT_STREAM_HDLR_s * const FileStr = (AMP_EXT_STREAM_HDLR_s *)hdlr;
    if (AmbaKAL_MutexTake(&FileStr->Mutex, AMBA_KAL_WAIT_FOREVER) == OK) {
        rval = AmpExtStream_FuncImpl(FileStr, cmd, param);
        AmbaKAL_MutexGive(&FileStr->Mutex);
    } else {
        ExtStream_Perror(0, 0, "Take mutex fail!");
    }
    return rval;
}

static AMP_STREAM_s g_File = {
    AmpExtStream_Open,
    AmpExtStream_Close,
    AmpExtStream_Read,
    AmpExtStream_Write,
    AmpExtStream_Seek,
    AmpExtStream_GetPos,
    AmpExtStream_GetLength,
    AmpExtStream_GetFreeSpace,
    AmpExtStream_Func
};

