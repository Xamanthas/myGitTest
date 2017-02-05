/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaUserFirmwareUpdater.c
 *
 *  @Copyright      :: Copyright (C) 2014 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Firmware Update through SD card
 *
 *  @History        ::
 *      Date        Name        Comments
 *      03/31/2014  W.Shi       Created
 *
 *  $LastChangedDate: 2014-06-11 16:15:07 +0800 (星期三, 11 六月 2014) $
 *  $LastChangedRevision: 10542 $
 *  $LastChangedBy: kfchen $
 *  $HeadURL: http://ambtwsvn2/svn/DSC_Platform/trunk/SoC/A9/Workshop/A9EVK/FirmwareUpdater/src/AmbaFirmwareUpdater.c $
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaKAL.h"

#include "AmbaRTSL_NOR.h"
#include "AmbaNOR.h"

#include "AmbaFS.h"
#include "AmbaPrintk.h"
#include "AmbaUtility.h"
#include "AmbaNFTL.h"

#include "AmbaBuffers.h"
#include "AmbaNOR_PartitionTable.h"
#include "AmbaNAND_PartitionTable.h"
#include "AmbaFirmwareUpdater.h"
#include "AmbaFWPROG.h"

static UINT8 AmbaFwUpdaterStack[8 * 1024] __POST_ATTRIB_ALIGN__(32);
extern const char *AmbaNOR_PartitionName[AMBA_NUM_FW_PARTITION + 1];
extern const int AmbaFW_PartitionSize[AMBA_NUM_FW_PARTITION];
extern AMBA_NOR_MEDIA_PART_s AmbaNOR_MediaPartInfo;
extern AMBA_NOR_FW_PART_s AmbaNOR_FwPartInfo;
static AMBA_NAND_PART_META_s _AmbaNOR_MetaData __attribute__((section(".bss.noinit")));
static AMBA_NOR_PART_TABLE_s _AmbaNOR_PartTable __attribute__((section(".bss.noinit")));
extern int AmbaNOR_SetMeta(AMBA_NAND_PART_META_s *pMeta);
extern int AmbaNOR_GetMeta(AMBA_NAND_PART_META_s * pMeta);

extern int AmbaNOR_GetPtb(AMBA_NOR_PART_TABLE_s *pTable);
extern int AmbaNOR_SetPtb(AMBA_NOR_PART_TABLE_s * pTable);
extern int AmbaNOR_SetPartInfo(UINT32 PartId, UINT32 StartBlk, UINT32 NumBlks);
extern int AmbaNOR_GetPartInfo(UINT32 PartId, UINT32 * pStartBlk, UINT32 * pNumBlks);
static char *MediaPartString[] = {
    [MP_Storage0] = "Storage0",
    [MP_Storage1] = "Storage1",
    [MP_IndexForVideoRecording] = "IndexForVideoRecording",
    [MP_UserSetting] = "UserSetting",
    [MP_CalibrationData] = "CalibrationData"
};

extern int AmbaNOR_GetPtb(AMBA_NOR_PART_TABLE_s *pTable);
extern int AmbaNOR_SetPtb(AMBA_NOR_PART_TABLE_s * pTable);
extern int AmbaNOR_SetPartInfo(UINT32 PartId, UINT32 StartBlk, UINT32 NumBlks);
extern int AmbaNOR_GetPartInfo(UINT32 PartId, UINT32 * pStartBlk, UINT32 * pNumBlks);
extern int AmbaNOR_MediaPartWrite(int ID, UINT8 * pBuf, UINT32 Sec, UINT32 Secs);

static AMBA_NOR_MEDIA_PART_s _AmbaNOR_MediaPartOld __attribute__((section(".bss.noinit")));
static AMBA_NOR_FW_PART_s    _AmbaNOR_FwPartOld __attribute__((section(".bss.noinit")));

#define CrcBufLen (0x10000)
#define IMAGE_BUF_RESERVE_SIZE 2048
UINT8 CrcBuf[CrcBufLen] __attribute__((section(".bss.noinit")));

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_GetFwImageCRC32
 *
 *  @Description:: Calculate CRC32 checksum of a image
 *
 *  @Input      ::
 *      pFile:  file pointer to firmware
 *      DataOffset: file position of the first data for CRC check
 *      DataSize: Number of data bytes
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *      UINT32  : CRC32 checksum.
\*-----------------------------------------------------------------------------------------------*/
static UINT32 FwUpdater_GetFwImageCRC32(AMBA_FS_FILE *pFile, UINT32 DataOffset, int DataSize)
{
    void *pCrcBuf = (void *) CrcBuf;
    int CrcBufSize = CrcBufLen;
    UINT32 Crc = AMBA_CRC32_INIT_VALUE;

    if (AmbaFS_fseek(pFile, DataOffset, AMBA_FS_SEEK_START) < 0)
        return 0xffffffff;

    while (DataSize > 0) {
        if (DataSize < CrcBufSize)
            CrcBufSize = DataSize;

        DataSize -= CrcBufSize;

        memset(pCrcBuf, 0, CrcBufSize);
        if (AmbaFS_fread(pCrcBuf, 1, CrcBufSize, pFile) != CrcBufSize)
            return 0xffffffff;

        Crc = AmbaUtility_Crc32Add(pCrcBuf, CrcBufSize, Crc);
    }

    return AmbaUtility_Crc32Finalize(Crc);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_ValidateImage
 *
 *  @Description:: Peform a basic sanity check to validate the image.
 *
 *  @Input      ::
 *      pImgHeader: pointer to image header
 *      pFile:  file pointer to firmware
 *      ImgDataOffset:  start file position of image data
 *      ImgSize:    image data size
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_ValidateImage(AMBA_IMG_HEADER *pImgHeader,
                                   AMBA_FS_FILE *pFile, UINT32 ImgDataOffset, UINT32 ImgSize)
{
    UINT32 ImgHeaderSize = sizeof(AMBA_IMG_HEADER);
    UINT32 Crc32;

    if (pFile == NULL || pImgHeader == NULL || ImgSize < ImgHeaderSize) {
        AmbaPrint("%s(%d)", __func__, __LINE__);
        return NG;
    }

    if (pImgHeader->VerNum == 0x0 || pImgHeader->VerDate == 0x0 ||
        (ImgSize != ImgHeaderSize + pImgHeader->ImgLen)) {
        AmbaPrint("%s(%d)", __func__, __LINE__);
        return NG;
    }

    Crc32 = FwUpdater_GetFwImageCRC32(pFile, ImgDataOffset + ImgHeaderSize, pImgHeader->ImgLen);
    if (Crc32 != pImgHeader->Crc32) {
        AmbaPrint("Verifying image CRC ... 0x%x != 0x%x failed!", Crc32, pImgHeader->Crc32);
        return NG;
    } else {
        AmbaPrint("Verifying image CRC ... done");
        return OK;
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_UpdateNorPartInfo
 *
 *  @Description:: Update NOR partition info
 *
 *  @Input      ::
 *      pFwHeader:  firmware binary header
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_UpdateNorPartInfo(AMBA_FIRMWARE_HEADER_s *pFwHeader)
{
    AMBA_NOR_DEV_s *pDev;
    UINT32 StartBlock = 0, NumBlock = 0;
    UINT32 BlockSize;
    int i;

    pDev = AmbaRTSL_NorGetDev();
    BlockSize = pDev->DevLogicInfo.EraseBlockSize;

    for (i = 0; i < TOTAL_FW_PARTS; i++) {
        NumBlock = pFwHeader->PartitionSize[i] / BlockSize;
        if ((pFwHeader->PartitionSize[i] % BlockSize) != 0x0)
            NumBlock++;
        AmbaNOR_SetPartInfo(i, StartBlock, NumBlock);
        StartBlock += NumBlock;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_UpdateNorMetaInfo
 *
 *  @Description:: Update the NOR device meta information.
 *
 *  @Input      ::
 *      pModelName  : Pointer to NOR meta model name.
 *
 *  @Output     :: none
 *
 *  @Return     :: none
 *-----------------------------------------------------------------------------------------------*/
void FwUpdater_UpdateNorMetaInfo(char *pModelName)
{
    AMBA_NOR_DEV_s *pDev;
    AMBA_NAND_PART_META_s *pNorMeta = &_AmbaNOR_MetaData;
    UINT32 BlockSize;
    UINT32 NumFwBlock = 0;
    UINT32 NumBlock, StartBlock, EndBlock;
    int i;

    pDev = AmbaRTSL_NorGetDev();
    BlockSize = pDev->DevLogicInfo.EraseBlockSize;

    /* Update NFTL partition info */
    for (i = 0; i < TOTAL_FW_PARTS; i++) {
        AmbaNOR_GetPartInfo(i, &StartBlock, &NumBlock);
        NumFwBlock += NumBlock;
    }

    EndBlock = (pDev->DevLogicInfo.TotalByteSize / pDev->DevLogicInfo.EraseBlockSize) - 1;

    for (i = AMBA_NUM_FW_PARTITION - 1; i > TOTAL_FW_PARTS; i--) {
        NumBlock = (AmbaFW_PartitionSize[i] + (BlockSize - 1)) / BlockSize;
        StartBlock = EndBlock + 1 - NumBlock;
        if (StartBlock < NumFwBlock) {
            AmbaPrint("Cannot allocate Nor  partition!");
            NumBlock = 0;
        }
        AmbaNOR_SetPartInfo(i, StartBlock, NumBlock);
        EndBlock = StartBlock - 1;
    }

    /* AmbaNAND_CalcNftlPartition(NumFwBlock); */
    /* Update NOR meta info */
    pNorMeta->Magic = PTB_META_MAGIC;

    if (pModelName != NULL && strlen(pModelName) <= FW_MODEL_NAME_SIZE)
        strcpy((char *)pNorMeta->ModelName, pModelName);
    else
        pNorMeta->ModelName[0] = '\0';

    for (i = 0; i < AMBA_NUM_FW_PARTITION; i++) {
        memcpy(pNorMeta->PartInfo[i].Name, AmbaNOR_PartitionName[i], strlen(AmbaNOR_PartitionName[i]));
        pNorMeta->PartDev[i] = PART_ON_SPINOR;

        AmbaNOR_GetPartInfo(i, &StartBlock, &NumBlock);

        pNorMeta->PartInfo[i].StartBlk = StartBlock;
        pNorMeta->PartInfo[i].NumBlks  = NumBlock;
    }

    pNorMeta->Crc32 = AmbaUtility_Crc32((void *) pNorMeta, PTB_META_ACTURAL_LEN - sizeof(UINT32));

    AmbaNOR_SetMeta(pNorMeta);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_CheckImgHeader
 *
 *  @Description:: Validate image header
 *
 *  @Input   ::
 *      pFwHeader:  pointer to firmware binary header
 *
 *  @Output  :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_CheckImgHeader(AMBA_FIRMWARE_HEADER_s *pFwHeader)
{
    AMBA_NAND_PART_META_s NorMeta;
    int i;

    if (AmbaNOR_GetMeta(&NorMeta) < 0) {
        AmbaPrint("firmware model name doesn't match!");
        return NG;
    }

    if (strcmp((char *)pFwHeader->ModelName, (char *)NorMeta.ModelName) != 0) {
        AmbaPrint("firmware model name doesn't match!");
        return NG;
    }

    for (i = 0; i < AMBA_NUM_FIRMWARE_TYPE; i++) {
        if (pFwHeader->FwInfo[i].Size > 0) {
            AmbaPrint("Image \"%s\" is found!", AmbaNOR_PartitionName[i + AMBA_PARTITION_SYS_SOFTWARE]);

            if (pFwHeader->FwInfo[i].Size < sizeof(AMBA_IMG_HEADER)) {
                AmbaPrint("Image \"%s\" is incorrect!", AmbaNOR_PartitionName[i]);
                return NG;
            }
        }
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_ParseHeader
 *
 *  @Description:: Parse firmware binary header
 *
 *  @Input   ::
 *      pFile:      file pointer to firmware
 *      pFwHeader:  pointer to firmware binary header
 *
 *  @Output  :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_ParseFwHeader(AMBA_FS_FILE *pFile, AMBA_FIRMWARE_HEADER_s *pFwHeader)
{
    UINT32 FwHeaderSize = sizeof(AMBA_FIRMWARE_HEADER_s);

    if (FwHeaderSize != AmbaFS_fread(pFwHeader, 1, FwHeaderSize, pFile)) {
        AmbaPrint("can't read firmware file!");
        return NG;
    }

    /* 2. Get old ptb & Meta from NOR. */
    if ((AmbaNOR_GetPtb(&_AmbaNOR_PartTable) != 0) || (AmbaNOR_GetMeta(&_AmbaNOR_MetaData) != 0)) {
        AmbaPrint("Can't get ptb, erase it");
    }

    memcpy(&_AmbaNOR_MediaPartOld, &AmbaNOR_MediaPartInfo, sizeof(AMBA_NOR_MEDIA_PART_s));
    memcpy(&_AmbaNOR_FwPartOld,    &AmbaNOR_FwPartInfo,    sizeof(AMBA_NOR_FW_PART_s));

    if (FwUpdater_UpdateNorPartInfo(pFwHeader) != OK) {
        AmbaPrint("no partition info in firmware!");
        return NG;
    }

    /* 3. Check Header ,CRC and update PTB. */
    if (FwUpdater_CheckImgHeader(pFwHeader) != OK) {
        AmbaPrint("firmware header is illegal!");
        return NG;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_CheckFwImage
 *
 *  @Description:: Check all the images in firmware binary
 *
 *  @Input   ::
 *      pFile:      file pointer to firmware
 *      pFwHeader:  pointer to firmware binary header
 *
 *  @Output  :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_CheckFwImage(AMBA_FS_FILE *pFile, AMBA_FIRMWARE_HEADER_s *pFwHeader)
{
    AMBA_IMG_HEADER FwImgHeader;
    UINT32 FwImgOffset = sizeof(AMBA_FIRMWARE_HEADER_s);
    int i;

    AmbaPrint("Start firmware CRC check...\n");

    for (i = 0; i < AMBA_NUM_FIRMWARE_TYPE; i++) {
        if (pFwHeader->FwInfo[i].Size == 0)
            continue;

        AmbaPrint("Checking %s ", AmbaNOR_PartitionName[i + AMBA_PARTITION_SYS_SOFTWARE]);

        /* Read partition header. */
        if ((AmbaFS_fseek(pFile, FwImgOffset, AMBA_FS_SEEK_START) < 0) ||
            (AmbaFS_fread(&FwImgHeader, 1, sizeof(FwImgHeader), pFile) != sizeof(FwImgHeader))) {
            AmbaPrint("Cannot read fw image header!");
            return NG;
        }

        /* validate the image */
        if (FwUpdater_ValidateImage(&FwImgHeader, pFile, FwImgOffset, pFwHeader->FwInfo[i].Size) < 0) {
            AmbaPrint("Invalid CRC32 code!");
            return NG;
        }

        AmbaPrint("\tlength:\t\t%d", pFwHeader->FwInfo[i].Size);
        AmbaPrint("\tcrc32:\t\t0x%08x", FwImgHeader.Crc32);
        AmbaPrint("\tver_num:\t%d.%d", (FwImgHeader.VerNum >> 16), (FwImgHeader.VerNum & 0xffff));
        AmbaPrint("\tver_date:\t%d/%d/%d", (FwImgHeader.VerDate >> 16), ((FwImgHeader.VerDate >> 8) & 0xff), (FwImgHeader.VerDate & 0xff));
        AmbaPrint("\timg_len:\t%d", FwImgHeader.ImgLen);
        AmbaPrint("\tmem_addr:\t0x%08x\r\n", FwImgHeader.MemAddr);

        /* Get offset of the next image. */
        FwImgOffset += pFwHeader->FwInfo[i].Size;

        /* Offset should not be greater than 256MB */
        if (FwImgOffset > 0x10000000)
            return NG;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_WriteFwImage
 *
 *  @Description:: Write all the images in firmware binary to NOR
 *
 *  @Input   ::
 *      pFile:      file pointer to firmware
 *      pFwHeader:  pointer to firmware binary header
 *
 *  @Output  :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_WriteFwImage(AMBA_FS_FILE *pFile, AMBA_FIRMWARE_HEADER_s *pFwHeader)
{
    extern AMBA_KAL_BYTE_POOL_t  AmbaBytePool_Cached;
    UINT8 *pImgBuf = NULL;
    UINT32 ImgBufSize = 0;

    AMBA_IMG_HEADER FwImgHeader;
    AMBA_NOR_PART_s *pNorPart;
    UINT32 FwImgOffset = sizeof(AMBA_FIRMWARE_HEADER_s);
    UINT32 PartID;
    int i, rval;
    AMBA_MEM_CTRL_s MemCtrl;

    /* allocate buffer */
    for (i = 0; i < AMBA_NUM_FIRMWARE_TYPE; i++)
        if(pFwHeader->FwInfo[i].Size > ImgBufSize)
            ImgBufSize = pFwHeader->FwInfo[i].Size;

    ImgBufSize += IMAGE_BUF_RESERVE_SIZE;
    rval = AmbaKAL_MemAllocate(&AmbaBytePool_Cached, &MemCtrl, ImgBufSize, 32);
    if (rval < 0) {
        AmbaPrint("Failed to allocate memory");
        return NG;
    }
    pImgBuf = MemCtrl.pMemAlignedBase;

    /* Program to SPINOR and update PTB. */
    for (i = 0; i < AMBA_NUM_FIRMWARE_TYPE; i++) {
        if (pFwHeader->FwInfo[i].Size == 0)
            continue;

        PartID = (i + AMBA_PARTITION_SYS_SOFTWARE);
        pNorPart = &_AmbaNOR_PartTable.Part[PartID];


        /* Read partition header. */
        if ((AmbaFS_fseek(pFile, FwImgOffset, AMBA_FS_SEEK_START) < 0) ||
            (AmbaFS_fread(&FwImgHeader, 1, sizeof(FwImgHeader), pFile) != sizeof(FwImgHeader))) {
            AmbaPrint("Cannot read fw image header!");
            return NG;
        }

        if (FwImgHeader.ImgLen == 0) {
            goto next;
        }

        if (FwImgHeader.ImgLen > ImgBufSize) {
            AmbaPrint("Cannot fw image size is larger than buffer size!");
            return NG;
        }

        if ((AmbaFS_fread(pImgBuf, 1, FwImgHeader.ImgLen, pFile) != FwImgHeader.ImgLen)) {
            AmbaPrint("firmware image read fail");
            return NG;
        }

        AmbaPrint("Program \"%s\" to NOR ...", AmbaNOR_PartitionName[PartID]);
        if (AmbaNOR_WritePartition((UINT8 *)pImgBuf, FwImgHeader.ImgLen, PartID) != OK) {
            AmbaPrint("Failed");
            return NG;
        }

        /* Update the PTB's entry */
        pNorPart->Crc32    = FwImgHeader.Crc32;
        pNorPart->VerNum   = FwImgHeader.VerNum;
        pNorPart->VerDate  = FwImgHeader.VerDate;
        pNorPart->ImgLen   = FwImgHeader.ImgLen;
        pNorPart->MemAddr  = FwImgHeader.MemAddr;

        if (AmbaNOR_SetPtb(&_AmbaNOR_PartTable) < 0)
            AmbaPrint("Unable to update ptb %s", AmbaNOR_PartitionName[PartID]);

        AmbaPrint("Done");
next:
        /* Get offset of the next image. */
        FwImgOffset += pFwHeader->FwInfo[i].Size;
    }

    AmbaKAL_MemFree(&MemCtrl);
    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FirmwareUpdater
 *
 *  @Description::  firmware update for SD card
 *
 *  @Input   ::
 *      pFileName:  filename of the firmware file
 *
 *  @Output  :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FirmwareUpdater(char *pFileName)
{
    AMBA_FIRMWARE_HEADER_s FwHeader;
    AMBA_FS_FILE *pFile = NULL;
    AMBA_FS_STAT FileStat;

    /* check if firmware file exists */
    if (AmbaFS_Stat(pFileName, &FileStat) < 0) {
        AmbaPrint("firmware file %s doesn't exist!", pFileName);
        return NG;
    }

    /* Open firmware binary */
    pFile = AmbaFS_fopen(pFileName, "r");
    if (FwUpdater_ParseFwHeader(pFile, &FwHeader) != OK) {
        AmbaFS_fclose(pFile);
        return NG;
    }

    /* Validate firmware image header */
    if (FwUpdater_CheckFwImage(pFile, &FwHeader) != OK) {
        AmbaFS_fclose(pFile);
        return NG;
    }

    /* Update NOR partition table and the content of each partition */
    if (FwUpdater_WriteFwImage(pFile, &FwHeader) != OK) {
        AmbaFS_fclose(pFile);
        return NG;
    }

    /* Update NOR meta info */
    FwUpdater_UpdateNorMetaInfo(NULL);

    AmbaNorFwUpdaterSD_ClearMagicCode();

    //AmbaPrint("erase hiber...");
    //AmbaNOR_ErasePartition(AMBA_PARTITION_LINUX_HIBERNATION_IMAGE);
    //AmbaPrint("Done");

    AmbaFS_fclose(pFile);

    return OK;
}

static UINT32 FwUpdater_CheckMediaPartInfo(AMBA_NOR_MEDIA_PART_s *pNewMediaPart, AMBA_NOR_MEDIA_PART_s *pOldMediaPart, UINT8 PartID)
{
    if ((pNewMediaPart->StartBlk[PartID]   == pOldMediaPart->StartBlk[PartID]) &&
        (pNewMediaPart->NumBlks[PartID]    == pOldMediaPart->NumBlks[PartID]))
        return 0;
    else
        return 1;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_RestoreMediaPart
 *
 *  @Description:: the main entry of firmware updater
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_RestoreMediaPart(UINT8 *Change)
{
    extern AMBA_KAL_BYTE_POOL_t AmbaBytePool_Cached;
    UINT8 *pImgBuf = NULL;
    int ImgBufSize = 0;
    int i, Rval, Sec;
    char TmpFileName[32], Drive;
    AMBA_FS_FILE *pFile;
    AMBA_MEM_CTRL_s MemCtrl;

    Drive = 'c';

    /* allocate buffer */
    for (i = 0; i < MP_MAX; i ++) {
        if ((i == MP_Storage0) || (i == MP_Storage1) || (i == MP_IndexForVideoRecording))
            continue;
        if (AmbaNOR_MediaPartInfo.NumBlks[i] > ImgBufSize)
            ImgBufSize = AmbaNOR_MediaPartInfo.NumBlks[i];
    }
    ImgBufSize = 512 * ImgBufSize + IMAGE_BUF_RESERVE_SIZE;

    Rval = AmbaKAL_MemAllocate(&AmbaBytePool_Cached, &MemCtrl, ImgBufSize, 32);
    if (Rval < 0) {
        AmbaPrint("Failed to allocate memory");
        return NG;
    }
    pImgBuf = MemCtrl.pMemAlignedBase;

    for (i = 0; i < MP_MAX; i ++) {

        if ((i == MP_Storage0) || (i == MP_Storage1) || (i == MP_IndexForVideoRecording))
            continue;

        if (Change[i] == 0)
            continue;

        /* Check status */
        sprintf(TmpFileName, "%c:\\%s.bin", Drive, MediaPartString[i]);

        pFile = AmbaFS_fopen(TmpFileName, "r");
        if (pFile == NULL) {
            AmbaPrint("AmbaFS_fopen fail %s ...", TmpFileName);
            return -1;
        }

        for (Sec = 0; Sec < AmbaNOR_MediaPartInfo.NumBlks[i]; Sec ++) {
            Rval = AmbaFS_fread((void *)pImgBuf, 1, 512, pFile);
            if (Rval != 512) {
                return -1;
            }
            Rval = AmbaNOR_MediaPartWrite(i, (UINT8 *)pImgBuf, Sec, 1);
            if (Rval < 0) {
                AmbaPrint("AmbaNOR_MediaPartWrite %d fail ...", i);
                return -1;
            }
        }
        AmbaFS_fclose(pFile);
    }
    return OK;
}

void AmbaFwUpdaterEntry(UINT32 Argv)
{
    UINT8 Change[NFTL_MAX_INSTANCE];
    int i;

    FirmwareUpdater("c:\\AmbaSysFW.bin");

    /* Get new partition info */
    for (i = 0; i < MP_MAX; i ++) {
        Change[i] = FwUpdater_CheckMediaPartInfo(&AmbaNOR_MediaPartInfo, &_AmbaNOR_MediaPartOld, i);
    }

    /* Restore Media Part data */
    FwUpdater_RestoreMediaPart(Change);
    AmbaPrint("FirmwareUpdater_NOR finish");
}

static UINT8 AmbaFwUpdaterStack[8 * 1024] __POST_ATTRIB_NOINIT__;

AMBA_KAL_TASK_t AmbaFwUpdaterTask;
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaFwUpdaterMain
 *
 *  @Description:: the main entry of firmware updater
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
int AmbaFwUpdaterMain(void)
{
    return AmbaKAL_TaskCreate(&AmbaFwUpdaterTask,                   /* pTask */
                              "AmbaFwUpdaterTask",                  /* pTaskName */
                              55,                                   /* Priority */
                              AmbaFwUpdaterEntry,                   /* void (*EntryFunction)(UINT32) */
                              0x0,                                  /* EntryArg */
                              AmbaFwUpdaterStack,                   /* pStackBase */
                              sizeof(AmbaFwUpdaterStack),           /* StackByteSize */
                              AMBA_KAL_AUTO_START);
}
