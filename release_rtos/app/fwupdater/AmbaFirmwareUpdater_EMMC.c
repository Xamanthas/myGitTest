/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaFirmwareUpdater_EMMC.c
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: MW for EMMC
 *
 *  @History        ::
 *      Date        Name        Comments
 *      09/05/2012  Evan Chen   Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaKAL.h"

#include "AmbaRTSL_NAND.h"
#include "AmbaNAND.h"
#include "AmbaNFTL.h"

#include "AmbaFS.h"
#include "AmbaPrintk.h"
#include "AmbaUtility.h"

#include "AmbaNAND_PartitionTable.h"
#include "AmbaEMMC_Def.h"
#include "AmbaRTSL_SD.h"
#include "AmbaSD.h"
#include "AmbaFirmwareUpdater.h"
#include "AmbaFwUpdaterSD.h"
#include "AmbaFWPROG.h"
#include "AmbaEMMC_Def.h"

#include "AmbaSysCtrl.h"

extern const char *AmbaEMMC_PartitionName[AMBA_NUM_FW_PARTITION + 1];
extern const int AmbaFW_PartitionSize[AMBA_NUM_FW_PARTITION];
extern AMBA_EMMC_MEDIA_PART_s AmbaEMMC_MediaPartInfo;
extern AMBA_EMMC_FW_PART_s   AmbaEMMC_FwPartInfo;

static char *MediaPartString[] = {
    [MP_Storage0] = "Storage0",
    [MP_Storage1] = "Storage1",
    [MP_IndexForVideoRecording] = "IndexForVideoRecording",
    [MP_UserSetting] = "UserSetting",
    [MP_CalibrationData] = "CalibrationData"
};

extern int AmbaEMMC_GetPtb(AMBA_NAND_PART_TABLE_s *pTable);
extern int AmbaEMMC_SetPtb(AMBA_NAND_PART_TABLE_s * pTable);
extern int AmbaEMMC_SetPartInfo(UINT32 PartId, UINT32 StartBlk, UINT32 NumBlks);
extern int AmbaEMMC_GetPartInfo(UINT32 PartId, UINT32 * pStartBlk, UINT32 * pNumBlks);
extern int AmbaEMMC_MediaPartWrite(int ID, UINT8 * pBuf, UINT32 Sec, UINT32 Secs);

static AMBA_EMMC_MEDIA_PART_s _AmbaEMMC_MediaPartOld __attribute__((section(".bss.noinit")));
static AMBA_EMMC_FW_PART_s    _AmbaEMMC_FwPartOld __attribute__((section(".bss.noinit")));

static AMBA_NAND_PART_TABLE_s _AmbaEMMC_PartTable __attribute__((section(".bss.noinit")));
static AMBA_NAND_PART_META_s _AmbaEMMC_MetaData __attribute__((section(".bss.noinit")));

extern int AmbaEMMC_SetMeta(AMBA_NAND_PART_META_s *pMeta);
extern int AmbaEMMC_GetMeta(AMBA_NAND_PART_META_s * pMeta);

#define CrcBufLen (0x10000)
#define IMAGE_BUF_RESERVE_SIZE 2048
UINT8 CrcBuf[CrcBufLen] __attribute__((section(".bss.noinit")));

__PRE_ATTRIB_ALIGN__(AMBA_CACHE_LINE_SIZE) __PRE_ATTRIB_NOINIT__
static UINT8 DtbTempBuf[64 * 2 * 1024]
__POST_ATTRIB_NOINIT__ __POST_ATTRIB_ALIGN__(32);

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
 *  @RoutineName:: FwUpdater_UpdateEmmcPartInfo
 *
 *  @Description:: Update EMMC partition info
 *
 *  @Input      ::
 *      pFwHeader:  firmware binary header
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_UpdateEmmcPartInfo(AMBA_FIRMWARE_HEADER_s *pFwHeader)
{
    UINT32 StartBlock = 0, NumBlock = 0;
    UINT32 BlockSize;
    int i;

    BlockSize = SEC_SIZE;

    for (i = 0; i < TOTAL_FW_PARTS; i++) {
        NumBlock = pFwHeader->PartitionSize[i] / BlockSize;
        if ((pFwHeader->PartitionSize[i] % BlockSize) != 0x0)
            NumBlock++;
        AmbaEMMC_SetPartInfo(i, StartBlock, NumBlock);
        StartBlock += NumBlock;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_UpdateEmmcMetaInfo
 *
 *  @Description:: Update the EMMC device meta information.
 *
 *  @Input      ::
 *      pModelName  : Pointer to EMMC meta model name.
 *
 *  @Output     :: none
 *
 *  @Return     :: none
 *-----------------------------------------------------------------------------------------------*/
void FwUpdater_UpdateEmmcMetaInfo(char *pModelName, AMBA_FIRMWARE_HEADER_s *pFwHeader)
{
    AMBA_SD_HOST *pHost;
    AMBA_SD_CARD *pCard;

    AMBA_NAND_PART_META_s *pEmmcMeta = &_AmbaEMMC_MetaData;
    UINT32 BlockSize = SEC_SIZE;
    UINT32 NumFwBlock = 0;
    UINT32 NumBlock, StartBlock, EndBlock;
    int i;

    pHost = AmbaRTSL_SdGetHost(SD_HOST_0);
    pCard = &pHost->Card;

    /* Update NFTL partition info */
    for (i = 0; i < TOTAL_FW_PARTS; i++) {
        AmbaEMMC_GetPartInfo(i, &StartBlock, &NumBlock);
        NumFwBlock += NumBlock;
    }

    EndBlock = (pCard->Capacity >> 9) - 1;

    for (i = AMBA_NUM_FW_PARTITION - 1; i > TOTAL_FW_PARTS; i--) {
        NumBlock = (AmbaFW_PartitionSize[i] + (BlockSize - 1)) / BlockSize;
        StartBlock = EndBlock + 1 - NumBlock;
        if (StartBlock < NumFwBlock) {
            AmbaPrint("Cannot allocate EMMC partition!");
            NumBlock = 0;
        }
        AmbaEMMC_SetPartInfo(i, StartBlock, NumBlock);
        EndBlock = StartBlock - 1;
    }

    /* Leave remaining EMMC Sectors for storage 0 */
    StartBlock = NumFwBlock;
    NumBlock = EndBlock + 1 - StartBlock;
    AmbaEMMC_SetPartInfo(AMBA_PARTITION_STORAGE0, StartBlock, NumBlock);

    /* AmbaNAND_CalcNftlPartition(NumFwBlock); */

    /* Update EMMC meta info */
    pEmmcMeta->Magic = PTB_META_MAGIC;

    if (pModelName != NULL && strlen(pModelName) <= FW_MODEL_NAME_SIZE)
        strcpy((char *)pEmmcMeta->ModelName, pModelName);
    else
        pEmmcMeta->ModelName[0] = '\0';

    for (i = 0; i < AMBA_NUM_FW_PARTITION; i++) {
        memcpy(pEmmcMeta->PartInfo[i].Name, AmbaEMMC_PartitionName[i], strlen(AmbaEMMC_PartitionName[i]));
        pEmmcMeta->PartDev[i] = PART_ON_EMMC;

        AmbaEMMC_GetPartInfo(i, &StartBlock, &NumBlock);

        pEmmcMeta->PartInfo[i].StartBlk = StartBlock;
        pEmmcMeta->PartInfo[i].NumBlks  = NumBlock;
    }

    pEmmcMeta->PloadInfo = pFwHeader->PloadInfo;
    pEmmcMeta->Crc32 = AmbaUtility_Crc32((void *) pEmmcMeta, PTB_META_ACTURAL_LEN - sizeof(UINT32));

    AmbaEMMC_SetMeta(pEmmcMeta);
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
    AMBA_NAND_PART_META_s EmmcMeta;
    int i;

    if (AmbaEMMC_GetMeta(&EmmcMeta) < 0) {
        AmbaPrint("firmware model name doesn't match!");
        return NG;
    }

    if (strcmp((char *)pFwHeader->ModelName, (char *)EmmcMeta.ModelName) != 0) {
        AmbaPrint("firmware model name doesn't match!");
        return NG;
    }

    for (i = 0; i < AMBA_NUM_FIRMWARE_TYPE; i++) {
        if (pFwHeader->FwInfo[i].Size > 0) {
            AmbaPrint("Image \"%s\" is found!", AmbaEMMC_PartitionName[i + AMBA_PARTITION_SYS_SOFTWARE]);

            if (pFwHeader->FwInfo[i].Size < sizeof(AMBA_IMG_HEADER)) {
                AmbaPrint("Image \"%s\" is incorrect!", AmbaEMMC_PartitionName[i]);
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

    /* 2. Get old ptb & Meta from EMMC. */
    if ((AmbaEMMC_GetPtb(&_AmbaEMMC_PartTable) != 0) || (AmbaEMMC_GetMeta(&_AmbaEMMC_MetaData) != 0)) {
        AmbaPrint("Can't get ptb, erase it");
    }

    memcpy(&_AmbaEMMC_MediaPartOld, &AmbaEMMC_MediaPartInfo, sizeof(AMBA_EMMC_MEDIA_PART_s));
    memcpy(&_AmbaEMMC_FwPartOld,    &AmbaEMMC_FwPartInfo,    sizeof(AMBA_EMMC_FW_PART_s));

    if (FwUpdater_UpdateEmmcPartInfo(pFwHeader) != OK) {
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

        AmbaPrint("Checking %s ", AmbaEMMC_PartitionName[i + AMBA_PARTITION_SYS_SOFTWARE]);

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
 *  @Description:: Write all the images in firmware binary to EMMC
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
    AMBA_NAND_PART_s *pNandPart;
    UINT32 FwImgOffset = sizeof(AMBA_FIRMWARE_HEADER_s);
    UINT32 PartID;
    int i, rval;
    AMBA_MEM_CTRL_s MemCtrl;
    UINT32 DtbBlock = _AmbaEMMC_FwPartOld.StartBlk[AMBA_PARTITION_LINUX_KERNEL] +
                      _AmbaEMMC_FwPartOld.NumBlks[AMBA_PARTITION_LINUX_KERNEL]  - 128;

    /* Backup Linux DTB data */
    if (_AmbaEMMC_FwPartOld.NumBlks[AMBA_PARTITION_LINUX_KERNEL] != 0) {
        memset(DtbTempBuf, 0x0, sizeof(DtbTempBuf));
        AmbaPrint("Backup DtbBlock");
        rval = AmbaSD_ReadSector(SD_HOST_0, DtbTempBuf, DtbBlock, 128);
        if (rval < 0)
            AmbaPrint("Failed to read DTB from Emmc");
    }

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

    /* Program to EMMC and update PTB. */
    for (i = 0; i < AMBA_NUM_FIRMWARE_TYPE; i++) {
        if (pFwHeader->FwInfo[i].Size == 0)
            continue;

        PartID = (i + AMBA_PARTITION_SYS_SOFTWARE);
        pNandPart = &_AmbaEMMC_PartTable.Part[PartID];


        /* Read partition header. */
        if ((AmbaFS_fseek(pFile, FwImgOffset, AMBA_FS_SEEK_START) < 0) ||
            (AmbaFS_fread(&FwImgHeader, 1, sizeof(FwImgHeader), pFile) != sizeof(FwImgHeader))) {
            AmbaPrint("Cannot read fw image header!");
            return NG;
        }

        if (FwImgHeader.ImgLen == 0) { // FIXME : check with Evan
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

        AmbaPrint("Program \"%s\" to eMMC ...", AmbaEMMC_PartitionName[PartID]);
        if (AmbaEMMC_WritePartition((UINT8 *)pImgBuf, FwImgHeader.ImgLen, PartID) != OK) {
            AmbaPrint("Failed");
            return NG;
        }

        /* Update the PTB's entry */
        pNandPart->Crc32    = FwImgHeader.Crc32;
        pNandPart->VerNum   = FwImgHeader.VerNum;
        pNandPart->VerDate  = FwImgHeader.VerDate;
        pNandPart->ImgLen   = FwImgHeader.ImgLen;
        pNandPart->MemAddr  = FwImgHeader.MemAddr;

        if (AmbaEMMC_SetPtb(&_AmbaEMMC_PartTable) < 0)
            AmbaPrint("Unable to update ptb %s", AmbaEMMC_PartitionName[PartID]);

        {
            AMBA_NAND_PART_META_s *pEmmcMeta = &_AmbaEMMC_MetaData;

            pEmmcMeta->PloadInfo = pFwHeader->PloadInfo;
            AmbaEMMC_SetMeta(pEmmcMeta);
        }

        AmbaPrint("Done");

        if ((i == AMBA_FIRMWARE_SYS_SOFTWARE) || (i == AMBA_FIRMWARE_LINUX_KERNEL)) {
            AmbaPrint("erase hiber...");
            rval = AmbaEMMC_ErasePartition(AMBA_PARTITION_LINUX_HIBERNATION_IMAGE);
            if (rval == 0)
                AmbaPrint("Done");
        }
next:
        /* Get offset of the next image. */
        FwImgOffset += pFwHeader->FwInfo[i].Size;
    }

    if ((_AmbaEMMC_FwPartOld.NumBlks[AMBA_PARTITION_LINUX_KERNEL] != AmbaEMMC_FwPartInfo.NumBlks[AMBA_PARTITION_LINUX_KERNEL]) ||
        (_AmbaEMMC_FwPartOld.StartBlk[AMBA_PARTITION_LINUX_KERNEL] != AmbaEMMC_FwPartInfo.StartBlk[AMBA_PARTITION_LINUX_KERNEL])) {
        DtbBlock = AmbaEMMC_FwPartInfo.StartBlk[AMBA_PARTITION_LINUX_KERNEL] + AmbaEMMC_FwPartInfo.NumBlks[AMBA_PARTITION_LINUX_KERNEL]  - 128;

        rval = AmbaSD_WriteSector(SD_HOST_0, DtbTempBuf, DtbBlock, 128);
        if (rval != OK) {
            AmbaPrint("Failed to write back DTB from Emmc");
            return NG;
        }
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

    /* Update EMMC partition table and the content of each partition */
    if (FwUpdater_WriteFwImage(pFile, &FwHeader) != OK) {
        AmbaFS_fclose(pFile);
        return NG;
    }

    /* Update EMMC meta info */
    FwUpdater_UpdateEmmcMetaInfo(NULL, &FwHeader);

    AmbaEmmcFwUpdaterSD_ClearMagicCode();

    AmbaFS_fclose(pFile);

    return OK;
}

static UINT32 FwUpdater_CheckMediaPartInfo(AMBA_EMMC_MEDIA_PART_s *pNewMediaPart, AMBA_EMMC_MEDIA_PART_s *pOldMediaPart, UINT8 PartID)
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
        if (AmbaEMMC_MediaPartInfo.NumBlks[i] > ImgBufSize)
            ImgBufSize = AmbaEMMC_MediaPartInfo.NumBlks[i];
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

        for (Sec = 0; Sec < AmbaEMMC_MediaPartInfo.NumBlks[i]; Sec ++) {
            Rval = AmbaFS_fread((void *)pImgBuf, 1, 512, pFile);
            if (Rval != 512) {
                return -1;
            }
            Rval = AmbaEMMC_MediaPartWrite(i, (UINT8 *)pImgBuf, Sec, 1);
            if (Rval < 0) {
                AmbaPrint("AmbaEMMC_MediaPartWrite %d fail ...", i);
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
    extern int volatile AmbaLink_Enable;

    //Image header could occupy linux partition and casue get_IPC_Mutex failed, thus we need to disable Ambalink
    AmbaLink_Enable = 0;
    FirmwareUpdater("c:\\AmbaSysFW.bin");

    /* Get new partition info */
    for (i = 0; i < MP_MAX; i ++) {
        Change[i] = FwUpdater_CheckMediaPartInfo(&AmbaEMMC_MediaPartInfo, &_AmbaEMMC_MediaPartOld, i);
    }

    /* Restore Media Part data */
    FwUpdater_RestoreMediaPart(Change);
    AmbaPrint("FirmwareUpdater_EMMC finish");
    AmbaSysSoftReset();
}

static UINT8 AmbaFwUpdaterStack[8 * 1024] __POST_ATTRIB_ALIGN__(32);

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
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmbaFwUpdaterMain(void)
{
    /* Create the task */
    return AmbaKAL_TaskCreate(&AmbaFwUpdaterTask,                   /* pTask */
                              "AmbaFwUpdaterTask",                  /* pTaskName */
                              55,                                   /* Priority */
                              AmbaFwUpdaterEntry,                   /* void (*EntryFunction)(UINT32) */
                              0x0,                                  /* EntryArg */
                              AmbaFwUpdaterStack,                   /* pStackBase */
                              sizeof(AmbaFwUpdaterStack),           /* StackByteSize */
                              AMBA_KAL_AUTO_START);                 /* AutoStart */
}
