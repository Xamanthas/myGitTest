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

#include "AmbaRTSL_NAND.h"
#include "AmbaNAND.h"
#include "AmbaNandBadBlockTable.h"
#include "AmbaNFTL.h"

#include "AmbaFS.h"
#include "AmbaPrintk.h"
#include "AmbaUtility.h"

#include "AmbaNAND_PartitionTable.h"
#include "AmbaFwUpdaterSD.h"
#include "AmbaFirmwareUpdater.h"

#include "AmbaBuffers.h"
#include "AmbaSysCtrl.h"

#define ENABLE_UPDATE_MEDIA_PARTITIONS
#define IMAGE_BUF_RESERVE_SIZE 2048

extern const char *AmbaNAND_PartitionName[AMBA_NUM_NAND_PARTITION + 1];
extern AMBA_NAND_NFTL_PART_s AmbaNAND_NftlPartInfo;
extern AMBA_NAND_FW_PART_s   AmbaNAND_FwPartInfo;

extern void AmbaNAND_UpdateNftlPartition(AMBA_NAND_DEV_s *pDev);
#ifdef ENABLE_UPDATE_MEDIA_PARTITIONS
static char *MediaPartString[] = {
    [MP_Storage0] = "Storage0",
    [MP_Storage1] = "Storage1",
    [MP_IndexForVideoRecording] = "IndexForVideoRecording",
    [MP_UserSetting] = "UserSetting",
    [MP_CalibrationData] = "CalibrationData"
};
#endif

static AMBA_NAND_NFTL_PART_s _AmbaNAND_NftlPartOld __POST_ATTRIB_NOINIT__;
static AMBA_NAND_FW_PART_s   _AmbaNAND_FwPartOld __POST_ATTRIB_NOINIT__;

static AMBA_NAND_PART_TABLE_s _AmbaNAND_PartTable ;
static AMBA_NAND_PART_META_s _AmbaNAND_MetaData __POST_ATTRIB_NOINIT__;

#define CrcBufLen (0x10000)
static UINT8 CrcBuf[CrcBufLen] __POST_ATTRIB_NOINIT__;

static AMBA_FIRMWARE_IMAGE_HEADER_s FwImgHeader __POST_ATTRIB_ALIGN__(512);

__PRE_ATTRIB_ALIGN__(AMBA_CACHE_LINE_SIZE) __PRE_ATTRIB_NOINIT__
static UINT8 DtbTempBuf[64 * 2 * 1024]
__POST_ATTRIB_NOINIT__ __POST_ATTRIB_ALIGN__(32);

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_GetContinousNandBlocks
 *
 *  @Description:: Get valid NAND blocks
 *
 *  @Input      ::
 *      NumBlock:       number of required NAND blocks
 *      StartBlock:     the first NAND block number
 *      ArrangeOrder:   NAND block number is in ascending(0) or descending(1) order
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          UINT32 : number of required NAND blocks plus zero to more bad blocks
\*-----------------------------------------------------------------------------------------------*/
static UINT32 FwUpdater_GetContinousNandBlocks(UINT32 NumBlock, UINT32 StartBlock, UINT8 ArrangeOrder)
{
    UINT32 Block;
    int i;

    for (i = 0 ; i < NumBlock; i ++) {
        /* either ascending or descending block number order */
        if (ArrangeOrder == 0)
            Block = StartBlock + i;
        else
            Block = StartBlock - i;

        /* we only care about inital bad block */
        if (AmbaNandBBT_IsBadBlock(Block) != NAND_INIT_BAD_BLOCK)
            continue;

        NumBlock ++;
    }

    return NumBlock;
}

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

    memset(pCrcBuf, 0, CrcBufSize);

    if (AmbaFS_fseek(pFile, DataOffset, AMBA_FS_SEEK_START) < 0)
        return 0xffffffff;

    while (DataSize > 0) {
        if (DataSize < CrcBufSize)
            CrcBufSize = DataSize;

        DataSize -= CrcBufSize;

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
static int FwUpdater_ValidateImage(AMBA_FIRMWARE_IMAGE_HEADER_s *pImgHeader,
                                   AMBA_FS_FILE *pFile, UINT32 ImgDataOffset, UINT32 ImgSize)
{
    UINT32 ImgHeaderSize = sizeof(AMBA_FIRMWARE_IMAGE_HEADER_s);
    UINT32 Crc32;

    if (pFile == NULL || pImgHeader == NULL || ImgSize < ImgHeaderSize) {
        AmbaPrint("%s(%d)", __func__, __LINE__);
        return NG;
    }

    if (pImgHeader->Version == 0x0 || pImgHeader->Date == 0x0 ||
        (ImgSize != ImgHeaderSize + pImgHeader->Length)) {
        AmbaPrint("%s(%d)", __func__, __LINE__);
        return NG;
    }

    Crc32 = FwUpdater_GetFwImageCRC32(pFile, ImgDataOffset + ImgHeaderSize, pImgHeader->Length);
    if (Crc32 != pImgHeader->Crc32) {
        AmbaPrint("Verifying image CRC ... 0x%x != 0x%x failed!", Crc32, pImgHeader->Crc32);
        return NG;
    } else {
        AmbaPrint("Verifying image CRC ... done");
        return OK;
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_UpdateNandPartInfo
 *
 *  @Description:: Update NAND partition info
 *
 *  @Input      ::
 *      pFwHeader:  firmware binary header
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_UpdateNandPartInfo(AMBA_FIRMWARE_HEADER_s *pFwHeader)
{
    UINT32 StartBlock = 0, NumBlock = 0;
    UINT32 BlockSize;
    int i;

    BlockSize = AmbaNAND_GetPagesPerBlock() * AmbaNAND_GetMainSize();

    for (i = 0; i < TOTAL_FW_PARTS; i++) {
        NumBlock = (pFwHeader->PartitionSize[i] + (BlockSize - 1)) / BlockSize;
        /* Only Guarantee all blocks are good in MEDIA partition */
        /* NumBlock = FwUpdater_GetContinousNandBlocks(NumBlock, StartBlock, 0); */
        AmbaNAND_SetPartInfo(i, StartBlock, NumBlock);
        StartBlock += NumBlock;
    }

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_UpdateNandMetaInfo
 *
 *  @Description:: Update the NAND device meta information.
 *
 *  @Input      ::
 *      pModelName  : Pointer to NAND meta model name.
 *
 *  @Output     :: none
 *
 *  @Return     :: none
 *-----------------------------------------------------------------------------------------------*/
void FwUpdater_UpdateNandMetaInfo(char *pModelName, AMBA_FIRMWARE_HEADER_s *pFwHeader)
{
    AMBA_NAND_PART_META_s *pNandMeta = &_AmbaNAND_MetaData;
    AMBA_NAND_DEV_s *pNandDev = AmbaRTSL_NandGetDev();
    UINT32 BlockSize = AmbaNAND_GetPagesPerBlock() * AmbaNAND_GetMainSize();
    UINT32 NumFwBlock = 0;
    UINT32 NumBlock, StartBlock, EndBlock;
    int i;

    /* Update NFTL partition info */
    for (i = 0; i < TOTAL_FW_PARTS; i++) {
        AmbaNAND_GetPartInfo(i, &StartBlock, &NumBlock);
        NumFwBlock += NumBlock;
    }

    EndBlock = (pNandDev->DevLogicInfo.TotalBlocks / pNandDev->DevLogicInfo.Intlve) - 1;

    for (i = AMBA_NUM_NAND_PARTITION - 1; i > TOTAL_FW_PARTS; i--) {
        NumBlock = (pFwHeader->PartitionSize[i] + (BlockSize - 1)) / BlockSize;
        NumBlock = FwUpdater_GetContinousNandBlocks(NumBlock, EndBlock, 1);
        StartBlock = EndBlock + 1 - NumBlock;
        if (StartBlock < NumFwBlock) {
            AmbaPrint("Cannot allocate NAND partition!");
            NumBlock = 0;
        }
        AmbaNAND_SetPartInfo(i, StartBlock, NumBlock);
        EndBlock = StartBlock - 1;
    }

    /* Leave remaining NAND blocks for storage 0 */
    StartBlock = NumFwBlock;
    NumBlock = EndBlock + 1 - StartBlock;
    AmbaNAND_SetPartInfo(AMBA_NAND_PARTITION_STORAGE0, StartBlock, NumBlock);

    AmbaNAND_UpdateNftlPartition(pNandDev);

    /* Update NAND meta info */
    pNandMeta->Magic = PTB_META_MAGIC;

    if (pModelName != NULL && strlen(pModelName) <= FW_MODEL_NAME_SIZE)
        strcpy((char *)pNandMeta->ModelName, pModelName);
    else
        pNandMeta->ModelName[0] = '\0';

    for (i = 0; i < AMBA_NUM_NAND_PARTITION; i++) {
        memcpy(pNandMeta->PartInfo[i].Name, AmbaNAND_PartitionName[i], strlen(AmbaNAND_PartitionName[i]));
        pNandMeta->PartDev[i] = PART_ON_NAND;

        AmbaNAND_GetPartInfo(i, &StartBlock, &NumBlock);

        pNandMeta->PartInfo[i].StartBlk = StartBlock;
        pNandMeta->PartInfo[i].NumBlks  = NumBlock;
    }

    pNandMeta->PloadInfo = pFwHeader->PloadInfo;
    pNandMeta->Crc32 = AmbaUtility_Crc32((void *) pNandMeta, PTB_META_ACTURAL_LEN - sizeof(UINT32));

    AmbaNAND_SetMeta(pNandMeta);
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
    AMBA_NAND_PART_META_s NandMeta;
    int i;

    if (AmbaNAND_GetMeta(&NandMeta) < 0) {
        AmbaPrint("firmware model name doesn't match!");
        return NG;
    }

    if (strcmp((char *)pFwHeader->ModelName, (char *)NandMeta.ModelName) != 0) {
        AmbaPrint("firmware model name doesn't match!");
        return NG;
    }

    for (i = 0; i < AMBA_NUM_FIRMWARE_TYPE; i++) {
        if (pFwHeader->FwInfo[i].Size > 0) {
            AmbaPrint("Image \"%s\" is found!", AmbaNAND_PartitionName[i]);

            if (pFwHeader->FwInfo[i].Size < sizeof(AMBA_FIRMWARE_IMAGE_HEADER_s)) {
                AmbaPrint("Image \"%s\" is incorrect!", AmbaNAND_PartitionName[i]);
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

    /* 2. Get old ptb & Meta from NAND. */
    if ((AmbaNAND_GetPtb(&_AmbaNAND_PartTable) != 0) || (AmbaNAND_GetMeta(&_AmbaNAND_MetaData) != 0)) {
        AmbaPrint("Can't get ptb, erase it");
    }

    memcpy(&_AmbaNAND_NftlPartOld, &AmbaNAND_NftlPartInfo, sizeof(AMBA_NAND_NFTL_PART_s));
    memcpy(&_AmbaNAND_FwPartOld,   &AmbaNAND_FwPartInfo,   sizeof(AMBA_NAND_FW_PART_s));

    AmbaNandBBT_Scan();

    if (FwUpdater_UpdateNandPartInfo(pFwHeader) != OK) {
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
    UINT32 FwImgOffset = sizeof(AMBA_FIRMWARE_HEADER_s);
    int i;

    AmbaPrint("Start firmware CRC check...\n");

    for (i = 0; i < AMBA_NUM_FIRMWARE_TYPE; i++) {
        if (pFwHeader->FwInfo[i].Size == 0)
            continue;

        AmbaPrint("Checking %s ", AmbaNAND_PartitionName[i + AMBA_NAND_PARTITION_SYS_SOFTWARE]);

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
        AmbaPrint("\tver_num:\t%d.%d", (FwImgHeader.Version >> 16), (FwImgHeader.Version & 0xffff));
        AmbaPrint("\tver_date:\t%d/%d/%d", (FwImgHeader.Date >> 16), ((FwImgHeader.Date >> 8) & 0xff), (FwImgHeader.Date & 0xff));
        AmbaPrint("\timg_len:\t%d", FwImgHeader.Length);
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
 *  @Description:: Write all the images in firmware binary to NAND flash
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

    AMBA_FIRMWARE_IMAGE_HEADER_s FwImgHeader;
    AMBA_NAND_PART_s *pNandPart;
    UINT32 FwImgOffset = sizeof(AMBA_FIRMWARE_HEADER_s);
    UINT32 PartID;
    int i, rval;
    AMBA_MEM_CTRL_s MemCtrl;
    AMBA_NAND_DEV_s *pNandDev = AmbaRTSL_NandGetDev();
    UINT32 DtbBlock = _AmbaNAND_FwPartOld.StartBlk[AMBA_PARTITION_LINUX_KERNEL] +
                      _AmbaNAND_FwPartOld.NumBlks[AMBA_PARTITION_LINUX_KERNEL]  - 1;

    /* Backup Linux DTB data */
    if (_AmbaNAND_FwPartOld.NumBlks[AMBA_PARTITION_LINUX_KERNEL] != 0) {
        UINT32 LinuxStartBlk = _AmbaNAND_FwPartOld.StartBlk[AMBA_PARTITION_LINUX_KERNEL];
        memset(DtbTempBuf, 0x0, sizeof(DtbTempBuf));

        for (; DtbBlock > LinuxStartBlk; DtbBlock --) {
            if (AmbaNandBBT_IsBadBlock(DtbBlock) == NAND_GOOD_BLOCK) {
                AmbaPrint("Backup DtbBlock");
                AmbaNAND_Read(DtbBlock, 0, pNandDev->DevLogicInfo.PagesPerBlock, DtbTempBuf, NULL, NAND_MAIN_ECC, 5000);
                break;
            }
        }
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

    /* Program to NAND and update PTB. */
    for (i = 0; i < AMBA_NUM_FIRMWARE_TYPE; i++) {
        if (pFwHeader->FwInfo[i].Size == 0)
            continue;

        PartID = (i + AMBA_NAND_PARTITION_SYS_SOFTWARE);
        pNandPart = &_AmbaNAND_PartTable.Part[PartID];

        /* Read partition header. */
        if ((AmbaFS_fseek(pFile, FwImgOffset, AMBA_FS_SEEK_START) < 0) ||
            (AmbaFS_fread(&FwImgHeader, 1, sizeof(FwImgHeader), pFile) != sizeof(FwImgHeader))) {
            AmbaPrint("Cannot read fw image header!");
            return NG;
        }

        if (FwImgHeader.Length > ImgBufSize) {
            AmbaPrint("Cannot fw image size is larger than buffer size!");
            return NG;
        }

        if ((AmbaFS_fread(pImgBuf, 1, FwImgHeader.Length, pFile) != FwImgHeader.Length)) {
            AmbaPrint("firmware image read fail");
            return NG;
        }

        AmbaPrint("Program \"%s\" to NAND flash ...", AmbaNAND_PartitionName[PartID]);

        if (i == AMBA_FIRMWARE_LINUX_KERNEL) {
            UINT32 LinuxBlkNum, BlkSize;
            BlkSize = pNandDev->DevLogicInfo.PagesPerBlock * pNandDev->DevLogicInfo.MainSize;

            LinuxBlkNum = FwImgHeader.Length / BlkSize;
            LinuxBlkNum += (FwImgHeader.Length % BlkSize) ? 1 : 0;
            LinuxBlkNum += 1;

            if (LinuxBlkNum > AmbaNAND_FwPartInfo.NumBlks[PartID]) {
                AmbaPrint("Please reserve 1 block for Linux DTB! %s", AmbaNAND_PartitionName[PartID]);
                return NG;
            }
        }

        if (AmbaNAND_WritePartition(pImgBuf, FwImgHeader.Length, PartID) != OK) {
            AmbaPrint("Failed");
            return NG;
        }

        /* Update the PTB's entry */
        pNandPart->Crc32    = FwImgHeader.Crc32;
        pNandPart->VerNum   = FwImgHeader.Version;
        pNandPart->VerDate  = FwImgHeader.Date;
        pNandPart->ImgLen   = FwImgHeader.Length;
        pNandPart->MemAddr  = FwImgHeader.MemAddr;

        if (AmbaNAND_SetPtb(&_AmbaNAND_PartTable) < 0)
            AmbaPrint("Unable to update ptb %s", AmbaNAND_PartitionName[PartID]);
        {
            AMBA_NAND_PART_META_s *pNandMeta = &_AmbaNAND_MetaData;

            pNandMeta->PloadInfo = pFwHeader->PloadInfo;
            AmbaNAND_SetMeta(pNandMeta);
        }

        AmbaPrint("Done");

        if ((i == AMBA_FIRMWARE_SYS_SOFTWARE) || (i == AMBA_FIRMWARE_LINUX_KERNEL)) {
            AmbaPrint("erase hiber...");
            rval = AmbaNAND_ErasePartition(AMBA_PARTITION_LINUX_HIBERNATION_IMAGE);
            if (rval == 0)
                AmbaPrint("Done");
        }


        /* Get offset of the next image. */
        FwImgOffset += pFwHeader->FwInfo[i].Size;
    }

    /* Write Linux DTB back */

    if ((_AmbaNAND_FwPartOld.NumBlks[AMBA_PARTITION_LINUX_KERNEL] != AmbaNAND_FwPartInfo.NumBlks[AMBA_PARTITION_LINUX_KERNEL]) ||
        (_AmbaNAND_FwPartOld.StartBlk[AMBA_PARTITION_LINUX_KERNEL] != AmbaNAND_FwPartInfo.StartBlk[AMBA_PARTITION_LINUX_KERNEL])) {
        AmbaPrint("Erase DtbBlock");
        DtbBlock = AmbaNAND_FwPartInfo.StartBlk[AMBA_PARTITION_LINUX_KERNEL] + AmbaNAND_FwPartInfo.NumBlks[AMBA_PARTITION_LINUX_KERNEL]  - 1;

        for (; DtbBlock > AmbaNAND_FwPartInfo.StartBlk[AMBA_PARTITION_LINUX_KERNEL]; DtbBlock --) {
            rval = AmbaNAND_Erase(DtbBlock, 5000);

            if (rval == OK) {
                AmbaPrint("Write back DtbBlock");
                return AmbaNAND_Program(DtbBlock, 0, pNandDev->DevLogicInfo.PagesPerBlock, DtbTempBuf, NULL, NAND_MAIN_ECC, 5000);
            }
        }
    }

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

    /* Update NAND partition table and the content of each partition */
    if (FwUpdater_WriteFwImage(pFile, &FwHeader) != OK) {
        AmbaFS_fclose(pFile);
        return NG;
    }

#ifdef ENABLE_UPDATE_MEDIA_PARTITIONS
    /* Update NAND meta info */
    FwUpdater_UpdateNandMetaInfo(NULL, &FwHeader);
#endif

    AmbaFwUpdaterSD_ClearMagicCode();

    AmbaFS_fclose(pFile);

    return OK;
}

#ifdef ENABLE_UPDATE_MEDIA_PARTITIONS
static UINT32 FwUpdater_CheckMediaPartInfo(AMBA_NAND_NFTL_PART_s *pNewMediaPart, AMBA_NAND_NFTL_PART_s *pOldMediaPart, UINT8 PartID)
{
    if ((pNewMediaPart->StartBlk[PartID]   == pOldMediaPart->StartBlk[PartID]) &&
        (pNewMediaPart->NumBlks[PartID]    == pOldMediaPart->NumBlks[PartID])  &&
        (pNewMediaPart->RsvBlk[PartID]     == pOldMediaPart->RsvBlk[PartID])   &&
        (pNewMediaPart->NumZones[PartID]   == pOldMediaPart->NumZones[PartID]) &&
        (pNewMediaPart->NumTrlBlks[PartID] == pOldMediaPart->NumTrlBlks[PartID]))
        return 0;
    else
        return 1;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: FwUpdater_ReinitMediaPart
 *
 *  @Description::
 *
 *  @Input   ::
 *
 *  @Output  :: none
 *
 *  @Return  ::
 *
\*-----------------------------------------------------------------------------------------------*/
static int FwUpdater_ReinitMediaPart(UINT8 *Change)
{
    int i, mode, Error = 0, Max = AMBA_NUM_NAND_MEDIA_PARTITION;
    AMBA_NAND_DEV_s *pDev = AmbaRTSL_NandGetDev();

    /* Deinit all partitions first because partition may be changed. */
    for (i = 0; i < AMBA_NUM_NAND_MEDIA_PARTITION; i++) {
        if (!AmbaNFTL_IsInit(i))
            continue;

        if (AmbaNFTL_Deinit(i) < 0) {
            AmbaPrint("%i AmbaNFTL_Deinit fail");
            Error--;
        }
    }

    /* Update nand device info */
    memset(pDev->DevPartInfo, 0x0, sizeof(pDev->DevPartInfo));
    AmbaNAND_InitNftlPart();

    /* Erase (new firmware) partitions if partition changed. */
    for (i = 0; i < AMBA_NUM_NAND_MEDIA_PARTITION; i++) {
        if (pDev->DevPartInfo[i].FtlBlocks <= 0 || Change[i] == 0)
            continue;

        AmbaPrint("Erase partition \"%s\"!", MediaPartString[i]);
        if (AmbaNFTL_ErasePart(i) < 0) {
            AmbaPrint("%s AmbaNFTL_ErasePart fail", MediaPartString[i]);
            Error--;
        }
    }

    /* Reinit all partitions. */
    for (i = 0; i < Max; i++) {
        if (pDev->DevPartInfo[i].FtlBlocks <= 0)
            continue;

        /* Make sure nftl partition is not initialized. */
        if (AmbaNFTL_IsInit(i)) {
            if (AmbaNFTL_Deinit(i) < 0) {
                AmbaPrint("%s AmbaNFTL_Deinit fail", MediaPartString[i]);
                Error--;
            }
        }

        mode = NFTL_MODE_NO_SAVE_TRL_TBL;

        if (AmbaNFTL_Init(i, mode) < 0) {
            AmbaPrint("%s AmbaNFTL_Init fail", MediaPartString[i]);
            Error--;
        }
    }

    return Error;
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
    AMBA_NFTL_STATUS_s Status;
    AMBA_FS_FILE *pFile;
    AMBA_MEM_CTRL_s MemCtrl;

    Drive = 'c';

    /* allocate buffer */
    for (i = 0; i < MP_MAX; i ++) {
        if ((i == MP_Storage0) || (i == MP_Storage1) || (i == MP_IndexForVideoRecording))
            continue;
        if (AmbaNAND_NftlPartInfo.NumBlks[i] > ImgBufSize)
            ImgBufSize = AmbaNAND_NftlPartInfo.NumBlks[i];
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

        if (AmbaNFTL_GetStatus(i, &Status) < 0) {
            AmbaPrint("AmbaNFTL_GetStatus fail...");
            return -1;
        }

        sprintf(TmpFileName, "%c:\\%s.bin", Drive, MediaPartString[i]);

        pFile = AmbaFS_fopen(TmpFileName, "r");
        if (pFile == NULL) {
            AmbaPrint("AmbaFS_fopen fail %s ...", TmpFileName);
            return -1;
        }

        for (Sec = 0; Sec < Status.TotalSecs; Sec ++) {
            Rval = AmbaFS_fread((void *)pImgBuf, 1, 512, pFile);
            if (Rval != 512) {
                return -1;
            }
            Rval = AmbaNFTL_Write(i, (UINT8 *)pImgBuf, Sec, 1);
            if (Rval < 0) {
                AmbaPrint("AmbaNFTL_Write %d fail ...", i);
                return -1;
            }
        }
        AmbaFS_fclose(pFile);
    }
    return OK;
}
#endif

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaFwUpdaterEntry
 *
 *  @Description:: the  entry of firmware updater
 *
 *  @Input      ::
        Argv: Task entry Argument
 *
 *  @Output     :: none
 *
 *  @Return     :: none
 *
\*-----------------------------------------------------------------------------------------------*/
void AmbaFwUpdaterEntry(UINT32 Argv)
{
#ifdef ENABLE_UPDATE_MEDIA_PARTITIONS
    UINT8 Change[NFTL_MAX_INSTANCE];
    int i;
#endif
    extern int volatile AmbaLink_Enable;

    //Image header could occupy linux partition and casue get_IPC_Mutex failed, thus we need to disable Ambalink
    AmbaLink_Enable = 0;
    if (FirmwareUpdater("c:\\AmbaSysFW.bin") != OK) {
        AmbaPrint("AmbaFirmware update fail ...");
        return;
    }

#ifdef ENABLE_UPDATE_MEDIA_PARTITIONS
    /* Get new partition info */
    for (i = 0; i < MP_MAX; i ++) {
        Change[i] = FwUpdater_CheckMediaPartInfo(&AmbaNAND_NftlPartInfo, &_AmbaNAND_NftlPartOld, i);
    }

    FwUpdater_ReinitMediaPart(Change);


    /* Restore Media Part data */
    FwUpdater_RestoreMediaPart(Change);
#endif
    AmbaPrint("FirmwareUpdater finish");
    AmbaSysSoftReset();
}

static UINT8 AmbaFwUpdaterStack[16 * 1024] __POST_ATTRIB_NOINIT__;

AMBA_KAL_TASK_t AmbaFwUpdaterTask __POST_ATTRIB_NOINIT__;

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
    /* Create the task */
    return AmbaKAL_TaskCreate(&AmbaFwUpdaterTask,                 /* pTask */
                              "AmbaFwUpdaterTask",                  /* pTaskName */
                              55,                                   /* Priority */
                              AmbaFwUpdaterEntry,                   /* void (*EntryFunction)(UINT32) */
                              0x0,                                  /* EntryArg */
                              AmbaFwUpdaterStack,                  /* pStackBase */
                              sizeof(AmbaFwUpdaterStack),          /* StackByteSize */
                              AMBA_KAL_AUTO_START);                 /* AutoStart */
}
