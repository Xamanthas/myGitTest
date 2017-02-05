/**
 * @file mw/unittest/extSrc/sample_calib/AmbaUTCalibMgr.c
 *
 * sample code for Calibration Manager control
 *
 * History:
 *    07/10/2013  Allen Chiu Created
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */


#include "AmbaUTCalibMgr.h"
#include "AmbaUtility.h"
#include <AmbaCardManager.h>
#include "AmbaNAND_Def.h"
#include "AmbaNFTL.h"
#include "ca/AmbaUTCalibCA.h"
#include "warp/AmbaUTCalibWarp.h"
#include "vignette/AmbaUTCalibVig.h"
#include "bpc/AmbaUTCalibBPC.h"
#include "AmbaROM.h"
#ifdef CONFIG_ENABLE_EMMC_BOOT
#include <AmbaPartition_Def.h>
extern int AmbaEMMC_MediaPartRead(int ID, UINT8 *pBuf, UINT32 Sec, UINT32 Secs);
extern int AmbaEMMC_MediaPartWrite(int ID, UINT8 *pBuf, UINT32 Sec, UINT32 Secs);
#endif

#define CALIB_MGR_DEBUG

#ifdef CALIB_MGR_DEBUG
#define CalMgrPrint     AmbaPrint
#else
#define CalMgrPrint(...)
#endif

AMBA_DSP_IMG_MODE_CFG_s CalibImgMode;


int AmpUT_CalibNandSave(UINT32 CalId , UINT32 SubId);

// define WARP table head loaded flag
static UINT8 GWarpTableHeadLoaded = 0;
static UINT8 GCATableHeadLoaded = 0;
static UINT8 GVignetteTableHeadLoaded = 0;
static UINT8 GBPCTableHeadLoaded = 0;

// define WARP table loaded flag
UINT8 GWarpTableLoaded[MAX_WARP_TABLE_COUNT] = {0};
UINT8 GCATableLoaded[MAX_CA_TABLE_COUNT] = {0};
UINT8 GVignetteTableLoaded[MAX_VIGNETTE_GAIN_TABLE_COUNT] = {0};
UINT8 GBPCTableLoaded[BPC_MAX_PAGES] = {0};
UINT8 GWBTableLoaded = 0,GBLCTableLoaded = 0, GAFTableLoaded = 0, GGyroTableLoaded = 0, GMShutterTableLoaded = 0;
UINT8 GIrisTableLoaded = 0,GISOTableLoaded = 0, GFlashTableLoaded = 0, GAudioTableLoaded = 0;

/* --------- */
UINT8 CalFormat[SITE_NUM+1] = {
    CAL_STATUS_LOAD_FORMAT,
    CAL_AF_LOAD_FORMAT,
    CAL_GYRO_LOAD_FORMAT,
    CAL_MSHUTTER_LOAD_FORMAT,
    CAL_IRIS_LOAD_FORMAT,
    CAL_VIGNETTE_LOAD_FORMAT,
    CAL_WARP_LOAD_FORMAT,
    CAL_FPN_LOAD_FORMAT,
    CAL_WB_LOAD_FORMAT,
    CAL_ISO_LOAD_FORMAT,
    CAL_BLC_LOAD_FORMAT,
    CAL_FLASH_LOAD_FORMAT,
    CAL_AUDIO_LOAD_FORMAT,
    CAL_CA_LOAD_FORMAT,
    CAL_LENSSHIFT_LOAD_FORMAT
};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibGetObj
 *
 *  @Description:: get calibration object
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          return calibration object table
\*-----------------------------------------------------------------------------------------------*/
Cal_Obj_s* AmpUT_CalibGetObj(UINT32 CalId)
{
    return &(CalObjTable[CalId]);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibGetNandSecNum
 *
 *  @Description:: calculate sectors occupied in nand
 *
 *  @Input      ::
 *          Size: Number of bytes
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          return sector number
\*-----------------------------------------------------------------------------------------------*/
UINT32 AmpUT_CalibGetNandSecNum(UINT32 Size)
{
    return (Size+511)>>9;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibGetNandSecStart
 *
 *  @Description:: calculate start sector in nand
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          start sector id
\*-----------------------------------------------------------------------------------------------*/
UINT32 AmpUT_CalibGetNandSecStart(UINT32 CalId)
{
    UINT8 i;
    Cal_Obj_s *CalObj;
    UINT32 Sec = 0;

    for (i = 0; i < CalId; i++) {
        CalObj = AmpUT_CalibGetObj(i);
        Sec += (CalObj->Size+511)>>9;
    }

    return Sec;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibGetDriverLetter
 *
 *  @Description:: get SD card slot
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          return slot driver
\*-----------------------------------------------------------------------------------------------*/
char AmpUT_CalibGetDriverLetter(void)
{
    AMBA_SCM_STATUS_s ScmStatus;
    char driver = { 'c' };

    AmbaSCM_GetSlotStatus(SCM_SLOT_SD0, &ScmStatus);
    if (ScmStatus.Format > 0) {
        driver = AmbaUtility_Slot2Drive(SCM_SLOT_SD0);
    } else {
        AmbaSCM_GetSlotStatus(SCM_SLOT_SD1, &ScmStatus);
        if (ScmStatus.Format > 0)
            driver = AmbaUtility_Slot2Drive(SCM_SLOT_SD1);
        else {
            AmbaPrint("%s() %d, Please insert SD Card", __func__, __LINE__);
            driver = 'z';
        }
    }
    return driver;
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibGetNandStatus
 *
 *  @Description:: get calibration object
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          return calibration object table
\*-----------------------------------------------------------------------------------------------*/
Cal_Obj_s* AmpUT_CalibGetNandStatus(UINT32 CalId,UINT32 *Offset, UINT32 *SectorNum)
{
    UINT8 i;
    Cal_Obj_s *CalObj;
    UINT32 SiteSize;

    *Offset = 0;
    *SectorNum = 0;

    for (i = 0; i < CalId; i++) {
        CalObj = AmpUT_CalibGetObj(i);
        SiteSize = (((CalObj->Size+511)>>9)<<9);
        *Offset += SiteSize;
    }
    CalObj = AmpUT_CalibGetObj(CalId);
    *SectorNum = (CalObj->Size+511)>>9;
    *Offset = *Offset>>9;
    return 0;
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibGetSiteStatus
 *
 *  @Description:: get calibration site status
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          return calibration site status
\*-----------------------------------------------------------------------------------------------*/
Cal_Stie_Status_s* AmpUT_CalibGetSiteStatus(UINT32 CalId)
{
    Cal_Status_s *CalStatus;

    CalStatus = (Cal_Status_s*)( AmpUT_CalibGetObj(CAL_STATUS_ID)->DramShadow );

    return &( CalStatus->Site[CalId] );
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibResetSiteStatus
 *
 *  @Description:: reset calibration site status
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibResetSiteStatus(UINT32 CalId)
{
    Cal_Stie_Status_s *CalSite;

    CalSite = AmpUT_CalibGetSiteStatus(CalId);
    memset(CalSite, 0, sizeof(Cal_Stie_Status_s));

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSaveStatus
 *
 *  @Description:: Write Status data to NAND flash
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibSaveStatus(void)
{
    return AmpUT_CalibNandSave(CAL_STATUS_ID,0);
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_MultiGetline
 *
 *  @Description:: get multi lines from the text file
 *
 *  @Input      ::
 *          Fp:  file pointer
 *          Buf: buffer
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK:0, NG: return NG message
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_MultiGetline(AMBA_FS_FILE *Fp, char Buf[])
{
    unsigned char ch;
    // Normal state
    do {
        if (AmbaFS_fread(&ch, 1, 1, Fp) == 0) { // read 1 byte
            return -1;
        }
        if ( (ch == '\n') || (ch == '\r') ) {
            break;  // go to end-of-line status
        }
        *Buf = ch;
        Buf++;
    } while (1);
    // End of line state
    do {
        if (AmbaFS_fread(&ch, 1, 1, Fp) == 0) { // read 1 byte
            break;
        }
        if ( (ch == '\n') || (ch == '\r') ) {
            /* do nothing */
        } else {
            // Reach to next line, roll back 1 byte
            AmbaFS_fseek(Fp, -1, SEEK_CUR);
            break;
        }
    } while (1);
    *Buf = '\0';
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibMemReset
 *
 *  @Description:: reset calibration data to zero
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibMemReset(UINT32 CalId)
{
    Cal_Obj_s *CalObj;

    CalObj = AmpUT_CalibGetObj(CalId);

    memset(CalObj->DramShadow, 0, CalObj->Size);
        AmpUT_CalibResetSiteStatus(CalId);
    return 0;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibNandReset
 *
 *  @Description:: Clear NVD partition with zero initialized
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibNandReset(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    UINT32 Offset=0,SectorNum=0;
    //UINT8 *CalDramShadow;
    int Rval;

    AmpUT_CalibMemReset(CalId);
    CalObj = AmpUT_CalibGetObj(CalId);
    AmpUT_CalibGetNandStatus(CalId,&Offset, &SectorNum);

    Rval = AmpUT_CalibNandSave( CalId, 0);
    if (Rval == OK) {
        CalMgrPrint("[CAL] Reset site %s %d success", CalObj->Name, CalId);
        return 0;
    } else {
        CalMgrPrint("[CAL] Reset site %s %d fail", CalObj->Name, CalId);
        return -1;
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSDCardReset
 *
 *  @Description:: Clear SD card calibration partition with zero initialized
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibSDCardReset(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    AMBA_FS_FILE *Fid = NULL;
    char FileName[20] = {'c',':','\\','c','a','l','i','b'};
    char tmp[10] = {0};

    AmpUT_CalibMemReset(CalId);
    CalObj = AmpUT_CalibGetObj(CalId);

    //write to SD card
    sprintf(tmp,"%d.bin",(int)CalId);
    strcat(FileName, tmp);
    FileName[0] = AmpUT_CalibGetDriverLetter();
    Fid = AmbaFS_fopen(FileName,"w");
    if (Fid == NULL) {
        AmbaPrint("fopen %s fail.",FileName);
        return -1;
    }
    CalMgrPrint("CalObj->Size = %d",CalObj->Size);
    AmbaFS_fwrite(CalObj->DramShadow,CalObj->Size, 1, Fid);
    AmbaFS_fclose(Fid);
    AmpUT_CalibRestCheckFlag(CalId);
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibResetPartial
 *
 *  @Description:: clear calibration data for partial load mode
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibResetPartial(UINT32 CalId) //for vignette partial load
{
    Cal_Obj_s *CalObj;
    char *CalName;
    int CalSubId; //the ID has been recognize in nand
    //UINT32 CalSize;
    //UINT8 *CalDramShadow;
    CalObj = AmpUT_CalibGetObj(CalId);
    CalName = CalObj->Name;

    memset(CalObj->DramShadow, 0, CalObj->Size);
    AmpUT_CalibResetSiteStatus(CalId);

    if (CalId == CAL_VIGNETTE_ID) {
        for (CalSubId = CAL_VIGNETTE_PARTIAL_ID_(0); CalSubId <CAL_VIGNETTE_PARTIAL_ID_(MAX_VIGNETTE_GAIN_TABLE_COUNT); CalSubId++) {
            // The first vig table ID in NAND is CAL_VIGNETTE_PARTIAL_LOAD_START_ID (20)
            //CalDramShadow = CalObj->DramShadow + ((CalSubId-CAL_VIGNETTE_PARTIAL_ID_(0)) * sizeof(Vignette_Pack_Storage_s)) + CAL_VIGNETTE_TABLE_BASE;
            //the shift 4 byte is vignette Enable & table count, that are only appear in the start addr
            //CalSize = sizeof(Vignette_Pack_Storage_s);//for one vignette table Size
            //bug
            //if(AmbaNVD_Save(NVD_ID_CALIB, CalSubId, CalDramShadow, CalSize) == 0) {
            if (0) {
                CalMgrPrint("[CAL] Reset site %s %d partial success", CalName,CalSubId);
            } else {
                CalMgrPrint("[CAL] Reset site %s %d  partial fail", CalName,CalSubId);
                return -1;
            }
        }
    }

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibCheckSize
 *
 *  @Description:: Check the calibration data size with the defined size
 *
 *  @Input      ::
 *          CalId: calibration ID
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibCheckSize(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    char *CalName;
    int Rval = OK;
    UINT32 TotalSize;

    CalObj = AmpUT_CalibGetObj(CalId);
    CalName = CalObj->Name;

    switch (CalId) {
        case CAL_VIGNETTE_ID:
            TotalSize = CAL_VIGNETTE_TABLE_BASE + sizeof(Vignette_Pack_Storage_s)*MAX_VIGNETTE_GAIN_TABLE_COUNT;
            if (TotalSize > CalObj->Size) {
                Rval = NG;
            }
            break;
        case CAL_WARP_ID: 
            TotalSize = CAL_WARP_TABLE_BASE + sizeof(Warp_Storage_s)*MAX_WARP_TABLE_COUNT;
            if (TotalSize > CalObj->Size) {
                Rval = NG;
            }
            break;
        case CAL_CA_ID: 
            TotalSize = CAL_CA_TABLE_BASE + sizeof(CA_Storage_s)*MAX_CA_TABLE_COUNT;
            if (TotalSize > CalObj->Size) {
                Rval = NG;
            }
            break;
        default:
            break;
    }
    if(Rval != OK) {
        AmbaPrintColor(RED,"[CAL]Please check the size of calibration site %s, wanted size = %d defined size = %d",CalName,TotalSize, CalObj->Size);
    }
    return Rval;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibNandLoadTableHead
 *
 *  @Description:: Load calibration table head data from NAND to DRAM
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibNandLoadTableHead(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    char *CalName;
    UINT32 CalSize;
    UINT8 *CalDramShadow;
    int Rval;
    UINT32 Sec, SecNum;


    CalObj = AmpUT_CalibGetObj(CalId);
    CalName = CalObj->Name;
    CalSize = CalObj->Size;
    CalDramShadow = CalObj->DramShadow;


    Sec = AmpUT_CalibGetNandSecStart(CalId);
    switch (CalId) {
        case CAL_VIGNETTE_ID:
            SecNum = 1;
            break;
        case CAL_BPC_ID:
            SecNum = (sizeof(BPC_Nand_s) -4 + 511) >>9 ; //temp for head should be less than one nand block (512 bytes)
            break;
        default:
            AmbaPrint("Only BPC and VNC load from nand");
            return -1;
            break;
    }
    //AmbaPrint("(sizeof(BPC_Nand_s) = %d ",sizeof(BPC_Nand_s));
    CalMgrPrint("Head: Sec = %d , SecNum = %d",Sec,SecNum);
#ifdef CONFIG_ENABLE_EMMC_BOOT
    Rval = AmbaEMMC_MediaPartRead(MP_CalibrationData, CalDramShadow, Sec, SecNum);
#else
    Rval = AmbaNFTL_Read(NFTL_ID_CAL, CalDramShadow, Sec, SecNum);
#endif

    AmbaPrintColor(RED,"Rval = %d",Rval);
    if (Rval == OK) {
        CalMgrPrint("[CAL] Site %s %d load success, Size=%d", CalName, CalId, CalSize);
        return 0;
    } else {
        CalMgrPrint("[CAL] Site %s %d load fail, Size=%d", CalName, CalId, CalSize);
        return -1;
    }
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibNandLoadTable
 *
 *  @Description:: Load calibration table data from NAND to DRAM
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          TableIdx: entry table id for loading
 *          TableNums: number of tables for loading
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibNandLoadTable(UINT32 CalId, UINT32 TableIdx, UINT32 TableNums)
{
    Cal_Obj_s *CalObj;
    char *CalName;
    UINT32 CalSize;
    UINT8 *CalDramShadow;
    UINT32 CalDramStart;
    UINT32 TotalSecNums=0;
    UINT32 SecStart;
    //UINT8 CalSubId = CalId; //the ID has been recognize in nand
    int Rval;
    UINT32 Sec, SecNum;
    UINT8 StartTable,Count;


    CalObj = AmpUT_CalibGetObj(CalId);
    CalName = CalObj->Name;
    CalSize = CalObj->Size;
    CalDramShadow = CalObj->DramShadow;

    if (AmpUT_CalibCheckHeadFlag(CalId) == 0) {
        AmpUT_CalibNandLoadTableHead(CalId);
    }

    Sec = AmpUT_CalibGetNandSecStart(CalId);

    StartTable = TableIdx;
    Count = TableNums;    
    switch (CalId) {
        case CAL_VIGNETTE_ID:
            SecStart = Sec + 1;
            if (TableIdx == CALIB_FULL_LOAD) {
                StartTable = 0;
                Count = MAX_VIGNETTE_GAIN_TABLE_COUNT;
            } else if ((TableIdx+TableNums) > MAX_VIGNETTE_GAIN_TABLE_COUNT) {
                Count = MAX_VIGNETTE_GAIN_TABLE_COUNT-TableIdx;
            } else if (TableIdx > MAX_VIGNETTE_GAIN_TABLE_COUNT) {
                AmbaPrint("VNC data out of range, please check the parameter");
                return -1;
            }
            SecNum = AmpUT_CalibGetNandSecNum(sizeof(Vignette_Pack_Storage_s));
            SecStart += StartTable * SecNum;
            TotalSecNums = SecNum* Count;
            CalDramStart = (SecStart -Sec)* 512;
            break;
        case CAL_BPC_ID: {
            BPC_Nand_s *BPCNand =  (BPC_Nand_s *) CalObj->DramShadow;
            SecStart = Sec +((sizeof(BPC_Nand_s) -4 + 511) >>9) ;
            if (TableIdx == CALIB_FULL_LOAD) {
                StartTable = 0;
                Count = BPC_MAX_PAGES;
            } else if ((TableIdx+TableNums) > BPC_MAX_PAGES) {
                Count = BPC_MAX_PAGES-TableIdx;
            } else if (TableIdx > BPC_MAX_PAGES) {
                AmbaPrint("BPC data out of range, please check the parameter");
                return -1;
            }
            
            for (int i=0; i<StartTable; i++) {
                SecStart += AmpUT_CalibGetNandSecNum(BPCNand->BPCPage[i].PageSize);
                AmbaPrint("Pre  Table:%d PageSize:%d", i, BPCNand->BPCPage[i].PageSize);
            }
            for (int i=0; i<Count; i++) {
                TotalSecNums += AmpUT_CalibGetNandSecNum(BPCNand->BPCPage[StartTable+i].PageSize);
                CalMgrPrint("Post Table:%d PageSize:%d", StartTable+i, BPCNand->BPCPage[StartTable+i].PageSize);
            }
            CalDramStart = (SecStart -Sec)* 512;
            break;
        }
        default:
            SecStart = Sec;
            StartTable = 0;
            Count = 1;
            SecNum = AmpUT_CalibGetNandSecNum(CalObj->Size);
            TotalSecNums = SecNum;
            CalDramStart = 0;
            break;
    }

#ifdef CONFIG_ENABLE_EMMC_BOOT
    Rval = AmbaEMMC_MediaPartRead(MP_CalibrationData, CalDramShadow + CalDramStart , SecStart, TotalSecNums);
#else
    Rval = AmbaNFTL_Read(NFTL_ID_CAL, CalDramShadow + CalDramStart , SecStart, TotalSecNums);
#endif

    if (Rval == OK) {
        CalMgrPrint("[CAL] Site %s %d load success, Size=%d", CalName, CalId, CalSize);
        Rval = 0;
    } else {
        CalMgrPrint("[CAL] Site %s %d load fail, Size=%d", CalName, CalId, CalSize);
        Rval = -1;
    }

//CheckSum
    switch (CalId) {
        case CAL_VIGNETTE_ID: {
            for (int i=StartTable; i < StartTable + Count; i++) {
                Vignette_Pack_Storage_s *VNCNand = (Vignette_Pack_Storage_s *)(CalDramShadow+ CAL_VIGNETTE_TABLE(i)) ;
                if (i >= MAX_VIGNETTE_GAIN_TABLE_COUNT) {
                    AmbaPrint("VNC data out of range, please check the parameter");
                    break;
                } 
                if ( VNCNand->Enable == 1 ) {
                    UINT32 TmpCheckSum = 0;
                    
                    if(AmpUT_CalibCheckFlag(CalId, i) == 1) {
                        continue;
                    }
                    TmpCheckSum = AmbaUtility_Crc32((UINT32 *) (VNCNand), sizeof(Vignette_Pack_Storage_s));
                    if (memcmp(CalDramShadow + CAL_VIGNETTE_CHECKSUM(i), &TmpCheckSum, sizeof(UINT32)) == 0) {
                        AmpUT_CalibMarkFlag(CalId, i, 1);
                        CalMgrPrint("CheckSum Pass. checksum nand:%x checksum ram:%x ",*(UINT32 *)(CalDramShadow + CAL_VIGNETTE_CHECKSUM(i)),TmpCheckSum);
                        Rval =  0;
                    } else {
                        CalMgrPrint("CheckSum Fail %x." , TmpCheckSum);
                        Rval =  -1;
                    }
                } else {
                    CalMgrPrint("Vig Page:%d not Enable", i );
                }
            }
            break;
        }
        case CAL_BPC_ID: {
            BPC_Nand_s *BPCNand =  (BPC_Nand_s *) CalObj->DramShadow;
            for (int i=StartTable; i<StartTable+Count; i++) {
                if (i >= BPC_MAX_PAGES) {
                    AmbaPrint("BPC data out of range, please check the parameter");
                    break;
                }                
                if ( BPCNand->BPCPage[i].PageValid == 1 ) {
                    UINT32 TmpCheckSum =  0;
                    if(AmpUT_CalibCheckFlag(CalId, i) == 1) {
                        continue;
                    }
                    TmpCheckSum = AmbaUtility_Crc32( (UINT8 *)&BPCNand-> Data +  BPCNand->BPCPage[i].Offset  , BPCNand->BPCPage[i].PageSize);
                    CalMgrPrint("BPCNandDataAddr: %08x Offset:%d LastAddr:%08x Pagesize: %d",   &BPCNand-> Data , BPCNand->BPCPage[i].Offset, (UINT32 *)&BPCNand-> Data +  BPCNand->BPCPage[i].Offset,BPCNand->BPCPage[i].PageSize);
                    if ( BPCNand->BPCPage[i].CheckSum == TmpCheckSum) {
                        AmpUT_CalibMarkFlag(CalId, i, 1);
                        CalMgrPrint("CheckSum Pass. checksum nand:%x checksum ram:%x ",BPCNand->BPCPage[i].CheckSum,TmpCheckSum );
                        Rval =  0;
                    } else {
                        CalMgrPrint("CheckSum Fail %x." , TmpCheckSum);
                        Rval =  -1;
                    }
                } else {
                    CalMgrPrint("BPC Page:%d not Enable",i );
                }
            }
            break;
        }
        default: {
            UINT32 NandCheckSum = *(UINT32 *)(CalDramShadow+ (CalObj->Size-4));
            UINT32 TmpCheckSum = 0;
            if(AmpUT_CalibCheckFlag(CalId, 0) == 1) {
                break;
            }
            TmpCheckSum = AmbaUtility_Crc32(CalDramShadow, CalObj->Size - 4);
            if (NandCheckSum == TmpCheckSum) {
                CalMgrPrint("CalID:%d CheckSum Pass:%x", CalId, TmpCheckSum);
                AmpUT_CalibMarkFlag(CalId, 0, 1);
                Rval =  0;
            } else {
                AmbaPrint("CalID:%d CheckSum Fail: CalDram:%x Tmp:%x ", CalId , NandCheckSum, TmpCheckSum);
                Rval =  -1;
            }
            break;
    }
    }
    return Rval;

}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibNandLoad
 *
 *  @Description:: Load calibration data from NAND to DRAM
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibNandLoad(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    char *CalName;
    UINT32 CalSize;
    UINT8 *CalDramShadow;
    //UINT8 CalSubId = CalId; //the ID has been recognize in nand
    UINT32 Offset=0,SectorNum=0;
    int Rval;

    CalObj = AmpUT_CalibGetObj(CalId);
    CalName = CalObj->Name;
    CalSize = CalObj->Size;
    CalDramShadow = CalObj->DramShadow;

    AmpUT_CalibGetNandStatus(CalId,&Offset, &SectorNum);

#ifdef CONFIG_ENABLE_EMMC_BOOT
    Rval = AmbaEMMC_MediaPartRead(MP_CalibrationData, CalDramShadow, Offset, SectorNum);
#else
    Rval = AmbaNFTL_Read(NFTL_ID_CAL, CalDramShadow, Offset, SectorNum);
#endif
    if (Rval == OK) {
        CalMgrPrint("[CAL] Site %s %d load success, Size=%d", CalName, CalId, CalSize);
        Rval = 0;
    } else {
        CalMgrPrint("[CAL] Site %s %d load fail, Size=%d", CalName, CalId, CalSize);
        Rval = -1;
    }

//CheckSum
    switch (CalId) {
        case CAL_VIGNETTE_ID: {
            for (int i=0; i < MAX_VIGNETTE_GAIN_TABLE_COUNT; i++) {
                Vignette_Pack_Storage_s *VNCNand = (Vignette_Pack_Storage_s *)(CalDramShadow+ CAL_VIGNETTE_TABLE(i)) ;
                if ( VNCNand->Enable == 1 ) {
                    UINT32 TmpCheckSum = AmbaUtility_Crc32((UINT32 *) (VNCNand), sizeof(Vignette_Pack_Storage_s));
                    if (memcmp(CalDramShadow + CAL_VIGNETTE_CHECKSUM(i), &TmpCheckSum, sizeof(UINT32)) == 0) {
                        CalMgrPrint("Vig Page:%d CheckSum Pass. checksum nand:%x checksum ram:%x ", i, *(UINT32 *)(CalDramShadow + CAL_VIGNETTE_CHECKSUM(i)),TmpCheckSum);
                    } else {
                        CalMgrPrint("Vig Page:%d CheckSum Fail %x." , i, TmpCheckSum);
                        Rval =  -1;
                    }
                } else {
                    CalMgrPrint("Vig Page:%d not Enable", i );
                }
            }
            break;
        }
        case CAL_BPC_ID: {
            BPC_Nand_s *BPCNand =  (BPC_Nand_s *) CalObj->DramShadow;
            for (int i=0; i<BPC_MAX_PAGES; i++) {
                if ( BPCNand->BPCPage[i].PageValid == 1 ) {
                    UINT32 TmpCheckSum = AmbaUtility_Crc32( (UINT8 *)&BPCNand-> Data +  BPCNand->BPCPage[i].Offset  , BPCNand->BPCPage[i].PageSize);
                    if ( BPCNand->BPCPage[i].CheckSum == TmpCheckSum) {
                        CalMgrPrint("BPC Page:%d CheckSum Pass. checksum nand:%x checksum ram:%x ",i,BPCNand->BPCPage[i].CheckSum,TmpCheckSum );
                    } else {
                        CalMgrPrint("BPC Page:%d CheckSum Fail %x." ,i, TmpCheckSum);
                        Rval =  -1;
                    }
                } else {
                    CalMgrPrint("BPC Page:%d not Enable",i );
                }
            }
            break;
        }
        default: {
            UINT32 NandCheckSum = *(UINT32 *)(CalDramShadow+ (CalObj->Size-4));
            UINT32 TmpCheckSum = AmbaUtility_Crc32(CalDramShadow, CalObj->Size - 4);
            if (NandCheckSum == TmpCheckSum) {
                CalMgrPrint("CalID:%d CheckSum Pass:%x", CalId, TmpCheckSum);
            } else {
                CalMgrPrint("CalID:%d CheckSum Fail: CalDram:%x Tmp:%x ", CalId , NandCheckSum, TmpCheckSum);
                Rval =  -1;
            }
            break;
        }
    }

    return Rval;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibNandInit
 *
 *  @Description:: Init Calibration Nand
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibNandInit(void)
{
    int Rval;
    AmpUT_CalibRestCheckAllFlag();
#ifdef CONFIG_ENABLE_EMMC_BOOT
    Rval = 0;
#else
    AmbaNFTL_InitLock(NFTL_ID_CAL);
    Rval = AmbaNFTL_Init(NFTL_ID_CAL,0);
#endif
    return Rval;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSDCardLoad
 *
 *  @Description:: Load calibration data from SD card to DRAM
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibSDCardLoad(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    UINT32 CalSize;
    UINT8 *CalDramShadow;
    UINT8 CalSubId = CalId; //the ID has been recognize in nand
    char FileName[16] = {'c',':','\\','c','a','l','i','b'};
    char tmp[16] = {0};
    AMBA_FS_FILE *Fid = NULL;

    Cal_Stie_Status_s *PCalSite;//modify vig Status to 0xAAFFAAFF

    if (CalId >=CAL_VIGNETTE_PARTIAL_LOAD_START_ID) {
        CalSubId = CalId;
        CalId = CAL_VIGNETTE_ID;
    }
    CalMgrPrint("init load");
    CalObj = AmpUT_CalibGetObj(CalId);
    CalSize = CalObj->Size;
    CalDramShadow = CalObj->DramShadow;
    PCalSite = AmpUT_CalibGetSiteStatus(CalId);

    if (PCalSite->Status == CAL_SITE_DONE_VIGNETTE_LOAD_PARTIAL && CalId == CAL_VIGNETTE_ID) {
        CalObj->DramShadow[CAL_VIGNETTE_ENABLE] = PCalSite->Reserved[0]; //set the vignette Enable from status0(NAND) to dram
        CalObj->DramShadow[CAL_VIGNETTE_TABLE_COUNT] = PCalSite->Reserved[1]; // set the table count to from status0(NAND) to dram
        // The first vig table ID in NAND is CAL_VIGNETTE_PARTIAL_LOAD_START_ID (20)
        CalDramShadow = CalObj->DramShadow + ((CalSubId-CAL_VIGNETTE_PARTIAL_LOAD_START_ID) * sizeof(Vignette_Pack_Storage_s)) + CAL_VIGNETTE_TABLE_BASE;
        //the shift 4 byte is vignette Enable & table count, that are only appear in the start addr
        CalSize = sizeof(Vignette_Pack_Storage_s);
    } else {
        CalSubId = CalId;
    }

    sprintf(tmp,"%d.bin",CalSubId);
    strcat(FileName, tmp);
    FileName[0] = AmpUT_CalibGetDriverLetter();
    Fid = AmbaFS_fopen(FileName,"r");
    if (Fid == NULL) {
        AmbaPrint("fopen %s fail.",FileName);
        return -1;
    }
    AmbaFS_fread(CalDramShadow,CalSize, 1, Fid);
    AmbaFS_fclose(Fid);
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibROMLoadTableHead
 *
 *  @Description:: Load calibration table head data from ROM to DRAM
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibROMLoadTableHead(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    char FileName[16] = "";
    int FPOS = 0;
    int Rval = -1;
    int TableBase = 0;
    UINT8 *CalAddr;

    CalObj = AmpUT_CalibGetObj(CalId);
    CalAddr = CalObj->DramShadow;

    sprintf(FileName, "calib%d.bin", (int)CalId);
    switch (CalId) {
        case CAL_CA_ID:
            TableBase = CAL_CA_TABLE_BASE;
            break;
        case CAL_WARP_ID:
            TableBase = CAL_WARP_TABLE_BASE;
            break;
        case CAL_VIGNETTE_ID:
            TableBase = CAL_VIGNETTE_TABLE_BASE;
            break;
        default:
            break;
    }

    Rval = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, FileName, CalAddr, TableBase, FPOS);
    if (Rval == -1) {
        AmbaPrint("Load ROM file TableHead %s failed", FileName);
    } else {
        AmpUT_CalibMarkHeadFlag(CalId, 1);
        CalMgrPrint("Load ROM file TableHead %s success", FileName);
    }

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibROMLoadTable
 *
 *  @Description:: Load calibration table data from ROM to DRAM
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          TableIdx: entry table id for loading
 *          TableNums: number of tables for loading
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibROMLoadTable(UINT32 CalId, UINT32 TableIdx, UINT32 TableNums)
{
    Cal_Obj_s *CalObj;
    char FileName[16] = "";
    int FPOS = 0;
    int TableSize = 0;
    int Rval = -1;
    int TableBase = 0;
    UINT8 *CalAddr;
    UINT8 i;
    UINT8 StartTable,Count;

    if(TableNums == 1) {
        if (AmpUT_CalibCheckFlag(CalId, TableIdx) == 1) {
            AmbaPrint("Calibration site [%d] was already loaded",CalId);
            return 0;
        }
    }

    CalObj = AmpUT_CalibGetObj(CalId);
    CalAddr = CalObj->DramShadow;
    if (AmpUT_CalibCheckHeadFlag(CalId) == 0) {
        AmpUT_CalibROMLoadTableHead(CalId);
        AmpUT_CalibMarkHeadFlag(CalId, 1);
    }

    StartTable = TableIdx;
    Count = TableNums;
    switch (CalId) {
        case CAL_CA_ID:
            TableBase = CAL_CA_TABLE_BASE;
            TableSize = sizeof(CA_Storage_s);
            if (TableIdx == CALIB_FULL_LOAD) {
                StartTable = 0;
                Count = MAX_CA_TABLE_COUNT;
            }
            break;
        case CAL_WARP_ID:
            TableBase = CAL_WARP_TABLE_BASE;
            TableSize = sizeof(Warp_Storage_s);
            if (TableIdx == CALIB_FULL_LOAD) {
                StartTable = 0;
                Count = MAX_WARP_TABLE_COUNT;
            }
            break;
        case CAL_VIGNETTE_ID:
            TableBase = CAL_VIGNETTE_TABLE_BASE;
            TableSize = sizeof(Vignette_Pack_Storage_s);
            if (TableIdx == CALIB_FULL_LOAD) {
                StartTable = 0;
                Count = MAX_VIGNETTE_GAIN_TABLE_COUNT;
            }
            break;
        default:
            TableBase = 0;
            TableSize = CalObj->Size;
            StartTable = 0;
            Count = 1;
            break;
    }

    sprintf(FileName, "calib%d.bin", (int)CalId);
    FPOS = TableBase + StartTable * TableSize;
    CalAddr += TableBase + StartTable * TableSize;
    Rval = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, FileName, CalAddr, TableSize*Count, FPOS);
    if (Rval == -1) {
        AmbaPrint("Load ROM file %s failed", FileName);
    } else {
        for(i = StartTable; i < Count; i++) {
            AmpUT_CalibMarkFlag(CalId, i, 1);
        }
    }

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibROMLoad
 *
 *  @Description:: Load calibration data from ROM to DRAM
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibROMLoad(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    UINT32 CalSize;
    char FileName[16] = "";
    int FPOS = 0;
    int Rval;
    UINT8 *CalAddr = NULL;

    CalObj = AmpUT_CalibGetObj(CalId);
    CalAddr = CalObj->DramShadow;

    sprintf(FileName, "calib%d.bin", (int)CalId);
    CalSize = CalObj -> Size;

    Rval = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, FileName, CalAddr, CalSize, FPOS);
    if (Rval == -1) {
        AmbaPrint("Load ROM file %s failed", FileName);
    } else {
        CalMgrPrint("Load ROM file %s success", FileName);
    }

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibNandSaveTableHead
 *
 *  @Description:: Save calibration table head from DRAM to NAND
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibNandSaveTableHead(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    char *CalName;
    UINT32 CalSize;
    //int CalSubId; //the ID has been recognize in nand
    //Cal_Stie_Status_s *PCalSite;//The vignette calibration Status: 1.partial save: 0xAAFFAAFF 2.the original : 0X55FF55FF
    UINT8 *CalDramShadow;
    int Rval;
    UINT32 Sec, SecNum;

    CalObj = AmpUT_CalibGetObj(CalId);
    CalName = CalObj->Name;
    CalSize = CalObj->Size;
    CalDramShadow = CalObj->DramShadow;

    Sec = AmpUT_CalibGetNandSecStart(CalId);
    switch (CalId) {
        case CAL_BPC_ID:
            SecNum = ((sizeof(BPC_Page_s) * BPC_MAX_PAGES) + 511) >>9; //temp for head should be less than one nand block (512 bytes)
            break;
        default:
            SecNum = 1;
            break;
    }

    CalMgrPrint("Head: Sec = %d , SecNum = %d",Sec,SecNum);
#ifdef CONFIG_ENABLE_EMMC_BOOT
    Rval = AmbaEMMC_MediaPartWrite(MP_CalibrationData , CalDramShadow, Sec, SecNum);
#else
    Rval = AmbaNFTL_Write(NFTL_ID_CAL, CalDramShadow, Sec, SecNum);
#endif    
    CalMgrPrint("Rval = %d",Rval);

    if (Rval == OK) {
        CalMgrPrint("[CAL] Site %s %d save success, Size=%d", CalName, CalId, CalSize);
        return 0;
    } else {
        CalMgrPrint("[CAL] Site %s %d save fail, Size=%d", CalName, CalId, CalSize);
        return -1;
    }
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibNandSaveTable
 *
 *  @Description:: Save calibration table head from DRAM to NAND
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          TableIdx: entry table id for loading
 *          TableNums: number of tables for loading
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibNandSaveTable(UINT32 CalId, UINT32 TableIdx, UINT32 TableNums )
{
    Cal_Obj_s *CalObj;
    char *CalName;
    UINT32 CalSize;
    //int CalSubId; //the ID has been recognize in nand
    //Cal_Stie_Status_s *PCalSite;//The vignette calibration Status: 1.partial save: 0xAAFFAAFF 2.the original : 0X55FF55FF
    UINT8 *CalDramShadow;
    int Rval;
    UINT32 Sec, SecNum;
    UINT32 CalDramAdd;

    CalObj = AmpUT_CalibGetObj(CalId);
    CalName = CalObj->Name;
    CalSize = CalObj->Size;
    CalDramShadow = CalObj->DramShadow;

    Sec = AmpUT_CalibGetNandSecStart(CalId);

    switch (CalId) {
        case CAL_CA_ID:
            Sec++;
            SecNum = AmpUT_CalibGetNandSecNum(sizeof(CA_Storage_s));
            CalDramAdd = (1+TableIdx * SecNum)*512 ;
            break;
        case CAL_WARP_ID:
            Sec++;
            SecNum = AmpUT_CalibGetNandSecNum(sizeof(Warp_Storage_s));
            CalDramAdd = (1+TableIdx * SecNum)*512 ;
            break;
        case CAL_BPC_ID: {
            BPC_Nand_s *BPCNand =  (BPC_Nand_s *) CalObj->DramShadow;
            Sec+= ((sizeof(BPC_Page_s) * BPC_MAX_PAGES) + 511) >>9;
            SecNum = AmpUT_CalibGetNandSecNum(BPCNand->BPCPage[TableIdx].PageSize);
            CalDramAdd = BPCNand->BPCPage[TableIdx].Offset;
            break;
        }
        case CAL_VIGNETTE_ID:
            Sec++;
            SecNum = AmpUT_CalibGetNandSecNum(sizeof(Vignette_Pack_Storage_s));
            CalDramAdd = (1+TableIdx * SecNum)*512 ;
            break;
        default:
            SecNum = AmpUT_CalibGetNandSecNum(CalSize);
            CalDramAdd = (1+TableIdx * SecNum)*512 ;
            break;
    }


    CalMgrPrint("Table Sec = %d , SecNum = %d",Sec,SecNum);
#ifdef CONFIG_ENABLE_EMMC_BOOT
    Rval = AmbaEMMC_MediaPartWrite(MP_CalibrationData , CalDramShadow+ CalDramAdd, Sec+TableIdx * SecNum, SecNum* TableNums); 
#else
    Rval = AmbaNFTL_Write(NFTL_ID_CAL, CalDramShadow+ CalDramAdd, Sec+TableIdx * SecNum, SecNum* TableNums);
#endif
    if (Rval == OK) {
        CalMgrPrint("[CAL] Site %s %d save success, Size=%d", CalName, CalId, CalSize);
        AmpUT_CalibRestCheckFlag(CalId);
        return 0;
    } else {
        CalMgrPrint("[CAL] Site %s %d save fail, Size=%d", CalName, CalId, CalSize);
        return -1;
    }

}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibNandSave
 *
 *  @Description:: Save calibration from DRAM to NAND
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          SubId: sub-channel ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibNandSave(UINT32 CalId , UINT32 SubId)
{
    Cal_Obj_s *CalObj;
    char *CalName;
    UINT32 CalSize;
    //int CalSubId; //the ID has been recognize in nand
    //Cal_Stie_Status_s *PCalSite;//The vignette calibration Status: 1.partial save: 0xAAFFAAFF 2.the original : 0X55FF55FF
    UINT32 Offset,SectorNum;
    UINT8 *CalDramShadow;
    int Rval;

    CalObj = AmpUT_CalibGetObj(CalId);
    CalName = CalObj->Name;
    CalSize = CalObj->Size;
    CalDramShadow = CalObj->DramShadow;
    //PCalSite = AmpUT_CalibGetSiteStatus(CalId);
    AmpUT_CalibGetNandStatus(CalId,&Offset, &SectorNum);

//CheckSum
    switch (CalId) {
        case CAL_VIGNETTE_ID: {
            for (int i=0; i < MAX_VIGNETTE_GAIN_TABLE_COUNT; i++) {
                Vignette_Pack_Storage_s *VNCNand = (Vignette_Pack_Storage_s *)(UINT8 *)(CalDramShadow+ CAL_VIGNETTE_TABLE(i)) ;
                if ( VNCNand->Enable == 1 ) {
                    UINT32 TmpCheckSum = AmbaUtility_Crc32((UINT32 *) (VNCNand), sizeof(Vignette_Pack_Storage_s));
                    memcpy(CalDramShadow + CAL_VIGNETTE_CHECKSUM(i), &TmpCheckSum, sizeof(UINT32));
                    CalMgrPrint("Vig Page:%d CheckSum:%x TmpCheckSum:%x", i,  *(UINT32*)(CalDramShadow + CAL_VIGNETTE_CHECKSUM(i)), TmpCheckSum);
                } else {
                    CalMgrPrint("Vig Page:%d not Enable",i );
                }
            }
            break;
        }
        case CAL_BPC_ID: {
            BPC_Nand_s *BPCNand =  (BPC_Nand_s *) CalObj->DramShadow;
            for (int i=0; i<BPC_MAX_PAGES; i++) {
                if ( BPCNand->BPCPage[i].PageValid == 1 ) {
                    UINT32 TmpCheckSum = AmbaUtility_Crc32( (UINT8 *)&BPCNand-> Data +  BPCNand->BPCPage[i].Offset  , BPCNand->BPCPage[i].PageSize);
                    //AmbaPrint("BPCNandDataAddr: %08x Offset:%d LastAddr:%08x Pagesize: %d",   &BPCNand-> Data , BPCNand->BPCPage[i].Offset, (UINT32 *)&BPCNand-> Data +  BPCNand->BPCPage[i].Offset,BPCNand->BPCPage[i].PageSize);
                    BPCNand->BPCPage[i].CheckSum =TmpCheckSum;
                    CalMgrPrint("BPC Page:%d CheckSum:%x",i,BPCNand->BPCPage[i].CheckSum);
                } else {
                    CalMgrPrint("BPC Page:%d not Enable",i );
                }
            }
            break;
        }
        default: {
            UINT32 TmpCheckSum = AmbaUtility_Crc32(CalDramShadow, CalObj->Size - 4);
            memcpy(CalDramShadow + (CalObj->Size-4), &TmpCheckSum, sizeof(UINT32));
            CalMgrPrint("CalID:%d CheckSum:%x", CalId, TmpCheckSum);
            break;
        }
    }
    //CheckSum done

    //AmbaPrint("Offset = %d , SectorNum = %d",Offset,SectorNum);
#ifdef CONFIG_ENABLE_EMMC_BOOT
    Rval = AmbaEMMC_MediaPartWrite(MP_CalibrationData , CalDramShadow, Offset, SectorNum); 
#else
    Rval = AmbaNFTL_Write(NFTL_ID_CAL, CalDramShadow, Offset, SectorNum);
#endif    
    if (Rval == OK) {
        CalMgrPrint("[CAL] Site %s %d save success, Size=%d", CalName, CalId, CalSize);
        AmpUT_CalibRestCheckFlag(CalId);
        Rval =  0;
    } else {
        CalMgrPrint("[CAL] Site %s %d save fail, Size=%d", CalName, CalId, CalSize);
        Rval = -1;
    }

    return Rval;

}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSDCardSave
 *
 *  @Description:: Save calibration from DRAM to SD card
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          SubId: sub-channel ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibSDCardSave(UINT32 CalId , UINT32 SubId)
{
    Cal_Obj_s *CalObj;
    //char *CalName;
    UINT32 CalSize;
    UINT8 *CalDramShadow;
    int CalSubId; //the ID has been recognize in nand
    Cal_Stie_Status_s *PCalSite;//The vignette calibration Status: 1.partial save: 0xAAFFAAFF 2.the original : 0X55FF55FF
    AMBA_FS_FILE *Fid = NULL;
    char FileName[16] = {'c',':','\\','c','a','l','i','b'};
    char tmp[16] = {0};

    CalObj = AmpUT_CalibGetObj(CalId);
    //CalName = CalObj->Name;
    CalSize = CalObj->Size;
    CalDramShadow = CalObj->DramShadow;
    PCalSite = AmpUT_CalibGetSiteStatus(CalId);
    if (PCalSite->Status == CAL_SITE_DONE_VIGNETTE_LOAD_PARTIAL && CalId == CAL_VIGNETTE_ID) {
        CalSubId = SubId + CAL_VIGNETTE_PARTIAL_LOAD_START_ID;
        // The first vig table ID in NAND is CAL_VIGNETTE_PARTIAL_LOAD_START_ID (20)
        CalDramShadow = CalObj->DramShadow + (SubId * sizeof(Vignette_Pack_Storage_s)) + CAL_VIGNETTE_TABLE_BASE;
        //the shift 4 byte is vignette Enable & table count, that are only appear in the start addr
        CalSize = sizeof(Vignette_Pack_Storage_s);//for one vignette table Size
    } else {
        CalSubId = CalId;
    }

    sprintf(tmp,"%d.bin",CalSubId);
    strcat(FileName, tmp);
    FileName[0] = AmpUT_CalibGetDriverLetter();
    Fid = AmbaFS_fopen(FileName,"w");
    if (Fid == NULL) {
        AmbaPrint("fopen %s fail.",FileName);
        return -1;
    }
    AmbaFS_fwrite(CalDramShadow,CalSize, 1, Fid);
    AmbaFS_fclose(Fid);
    AmpUT_CalibRestCheckFlag(CalId);
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibMarkHeadFlag
 *
 *  @Description:: Mark calibration tablehead loaded flag (0:empty 1: already loaded)
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          TableIdx: table ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: return table index
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibMarkHeadFlag(UINT8 CalId, UINT8 Flag)
{
    switch (CalId) {
        case CAL_CA_ID:
            GCATableHeadLoaded = Flag;
            break;
        case CAL_WARP_ID:
            GWarpTableHeadLoaded = Flag;
            break;
        case CAL_VIGNETTE_ID:
            GVignetteTableHeadLoaded = Flag;
            break;
        case CAL_BPC_ID:
            GBPCTableHeadLoaded = Flag;
            break;
        default:
            break;
        }
        return 0;
    }


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibCheckHeadFlag
 *
 *  @Description:: Check calibration table loaded flag (0:empty 1: already loaded)
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          TableIdx: table ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: return table index
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibCheckHeadFlag(UINT8 CalId)
{
    switch (CalId) {
        case CAL_CA_ID:
            return GCATableHeadLoaded;
        case CAL_WARP_ID:
            return GWarpTableHeadLoaded;
        case CAL_VIGNETTE_ID:
            return GVignetteTableHeadLoaded;
        case CAL_BPC_ID:
            return GBPCTableHeadLoaded;
        default:
            return 1;
    }
}



/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibMarkFlag
 *
 *  @Description:: Mark calibration table loaded flag (0:empty 1: already loaded)
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          TableIdx: table ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: return table index
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibMarkFlag(UINT8 CalId, UINT8 TableIdx, UINT8 Flag)
{
    switch (CalId) {
        case CAL_CA_ID:
            GCATableLoaded[TableIdx] = Flag;
            break;
        case CAL_WARP_ID:
            GWarpTableLoaded[TableIdx] = Flag;
            break;
        case CAL_VIGNETTE_ID:
            GVignetteTableLoaded[TableIdx] = Flag;
            break;
        case CAL_BPC_ID:
            GBPCTableLoaded[TableIdx] = Flag;
            break;
        case CAL_AF_ID:
            GAFTableLoaded = Flag;
            break;
        case CAL_GYRO_ID:
            GGyroTableLoaded = Flag;
            break;
        case CAL_MSHUTTER_ID:
            GMShutterTableLoaded = Flag;
            break;
        case CAL_IRIS_ID:
            GIrisTableLoaded = Flag;
            break;
        case CAL_WB_ID:
            GWBTableLoaded = Flag;
            break;
        case CAL_ISO_ID:
            GISOTableLoaded = Flag;
            break;
        case CAL_FLASH_ID:
            GFlashTableLoaded = Flag;
            break;
        case CAL_AUDIO_ID:
            GAudioTableLoaded = Flag;
            break;
        default:
            break;
    }
    return 0;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibCheckFlag
 *
 *  @Description:: Check calibration table loaded flag (0:empty 1: already loaded)
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          TableIdx: table ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: return table index
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibCheckFlag(UINT8 CalId, UINT8 TableIdx)
{
    switch (CalId) {
        case CAL_CA_ID:
            return GCATableLoaded[TableIdx];
        case CAL_WARP_ID:
            return GWarpTableLoaded[TableIdx];
        case CAL_VIGNETTE_ID:
            return GVignetteTableLoaded[TableIdx];
        case CAL_BPC_ID:
            return GBPCTableLoaded[TableIdx];
        case CAL_AF_ID:
            return GAFTableLoaded;
            break;
        case CAL_GYRO_ID:
            return GGyroTableLoaded;
        case CAL_MSHUTTER_ID:
            return GMShutterTableLoaded;
        case CAL_IRIS_ID:
            return GIrisTableLoaded;
        case CAL_WB_ID:
            return GWBTableLoaded;
        case CAL_ISO_ID:
            return GISOTableLoaded;
        case CAL_FLASH_ID:
            return GFlashTableLoaded;
        case CAL_AUDIO_ID:
            return GAudioTableLoaded;
            
        default:
            return 0;
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibRestCheckAllFlag
 *
 *  @Description:: Reset calibration table loaded flag (0:empty 1: already loaded)
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          TableIdx: table ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          NULL
\*-----------------------------------------------------------------------------------------------*/
void AmpUT_CalibRestCheckAllFlag(void)
{
    memset(GCATableLoaded, 0, MAX_CA_TABLE_COUNT);
    memset(GWarpTableLoaded, 0, MAX_WARP_TABLE_COUNT);
    memset(GVignetteTableLoaded, 0, MAX_VIGNETTE_GAIN_TABLE_COUNT);
    memset(GBPCTableLoaded, 0, BPC_MAX_PAGES);
    GAFTableLoaded = 0;
    GGyroTableLoaded = 0;
    GMShutterTableLoaded = 0;
    GIrisTableLoaded = 0;
    GWBTableLoaded = 0;
    GISOTableLoaded = 0;
    GFlashTableLoaded = 0;
    GAudioTableLoaded = 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibRestCheckFlag
 *
 *  @Description:: Reset calibration table loaded flag (0:empty 1: already loaded)
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          TableIdx: table ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          NULL
\*-----------------------------------------------------------------------------------------------*/
void AmpUT_CalibRestCheckFlag(UINT8 CalId)
{
    switch (CalId) {
        case CAL_CA_ID:
            memset(GCATableLoaded, 0, MAX_CA_TABLE_COUNT);
            break;
        case CAL_WARP_ID:
            memset(GWarpTableLoaded, 0, MAX_WARP_TABLE_COUNT);
            break;
        case CAL_VIGNETTE_ID:
            memset(GVignetteTableLoaded, 0, MAX_VIGNETTE_GAIN_TABLE_COUNT);
            break;
        case CAL_BPC_ID:
            memset(GBPCTableLoaded, 0, BPC_MAX_PAGES);
            break;
        case CAL_AF_ID:
            GAFTableLoaded = 0;
            break;
        case CAL_GYRO_ID:
            GGyroTableLoaded = 0;
            break;
        case CAL_MSHUTTER_ID:
            GMShutterTableLoaded = 0;
            break;
        case CAL_IRIS_ID:
            GIrisTableLoaded = 0;
            break;
        case CAL_WB_ID:
            GWBTableLoaded = 0;
            break;
        case CAL_ISO_ID:
            GISOTableLoaded = 0;
            break;
        case CAL_FLASH_ID:
            GFlashTableLoaded = 0;
            break;
        case CAL_AUDIO_ID:
            GAudioTableLoaded = 0;
            break;
        default:
            break;
    }

        }


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibInitLoadSimple
 *
 *  @Description:: simple initial function for calibration
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibInitLoadSimple(UINT32 CalId, UINT8 Format)
{
    Cal_Stie_Status_s  *PCalSite;
    Cal_Obj_s              *CalObj;
    int Rval = 0;

    // Print site Status

    PCalSite = AmpUT_CalibGetSiteStatus(CalId);
    CalObj = AmpUT_CalibGetObj(CalId);
    if (CalObj->Enable == 0) {
        CalMgrPrint("[CAL] site %d didn't Enable", CalId);
        return 0;
    }
    //CalName = CalObj->Name;
    // Print site Status
    //UINT8 format = CALIB_SOURCE_NAND;
    // Format = CalFormat[CalId];

    CalMgrPrint("CalId #%d", CalId);
    // Check whether NVD Size matches
    if (((PCalSite->Status == CAL_SITE_DONE)) || (CalId == CAL_STATUS_ID)) {
        // Load warp/ca calibration table from rom file system
        if (AmpUT_CalibCheckFlag(CalId, CALIB_TABLE_IDX_INIT) == 0) {
            switch (Format) {
                case CALIB_SOURCE_ROMFS:
                    Rval = AmpUT_CalibROMLoad(CalId);
                    break;
                case CALIB_SOURCE_NAND:
                    Rval = AmpUT_CalibNandLoad(CalId);
                    break;
                case CALIB_SOURCE_SDCard:
                    Rval = AmpUT_CalibSDCardLoad(CalId);
                    break;
                default:
                    break;
            }
        } else {
            AmbaPrint("The calibration table %d has been loaded!", CALIB_TABLE_IDX_INIT);
            return -1;
        }
    } else {
        CalMgrPrint("no calibration Status. Need initial first");
    }

    return Rval;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibInitSimple
 *
 *  @Description:: simple initial function for calibration
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibInitSimple(UINT32 CalId)
{

    Cal_Obj_s               *CalObj;

    // Print site Status
    CalObj = AmpUT_CalibGetObj(CalId);

    if (CalObj->Enable == DISABLE) {
        CalMgrPrint("[CAL] site %d didn't Enable", CalId);
        return 0;
    }
    if (CalObj->InitFunc != NULL) {
        if ((CalObj->InitFunc)(CalObj) < 0) {
            CalMgrPrint("[CAL] Site %s %d init fail", CalObj->Name, CalId);
            return -1;
        }
    }
    CalMgrPrint("[CAL] Init site %s %d success", CalObj->Name, CalId);

    return 0;
}



/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibInitFunc
 *
 *  @Description:: initial function for calibration
 *
 *  @Input      ::
 *          CalId: calibration ID
 *          JobId: load or init
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibInitFunc(UINT32 CalId,UINT8 JobId, UINT8 SubId)
{
    Cal_Stie_Status_s   *PCalSite;
    Cal_Obj_s           *CalObj;
    char                *CalName;
    int                 Rval;


    PCalSite = AmpUT_CalibGetSiteStatus(CalId);
    CalObj = AmpUT_CalibGetObj(CalId);

    if (CalObj->Enable == 0) {
        CalMgrPrint("[CAL] site %d didn't Enable", CalId);
        return 0;
    }
    CalName = CalObj->Name;

    if (JobId == CALIB_LOAD) {
        // Print site Status
        UINT8 format = CalFormat[CalId];

        CalMgrPrint("CalId #%d", CalId);
        if ((PCalSite->Status == CAL_SITE_DONE) || (CalId == CAL_STATUS_ID) ||(format == CALIB_SOURCE_ROMFS)) {
            // Load warp/ca calibration table from rom file system
            Rval = AmpUT_CalibCheckSize(CalId);
            if(Rval != OK) {
                return Rval;
            }
            if (format == CALIB_SOURCE_ROMFS) {
                Rval = AmpUT_CalibROMLoadTable(CalId, SubId, 1);
                return Rval;
            } else { // Load calibration data from NAND to DRAM
                #if (CALIB_STORAGE == CALIB_FROM_NAND)
                Rval = AmpUT_CalibNandLoadTable(CalId,SubId,1);// Load calibration data from NAND to DRAM
                #else
                Rval = AmpUT_CalibSDCardLoad(CalId);// Load calibration data from SD card to DRAM
                #endif
                if (Rval < 0) {
                    CalMgrPrint("calib load error");
                    if (CalId == CAL_STATUS_ID) {
                        #if (CALIB_STORAGE == CALIB_FROM_NAND)
                        Rval = AmpUT_CalibNandReset(CalId);
                        #else
                        Rval = AmpUT_CalibSDCardReset(CalId);
                        #endif
                        if (Rval < 0) {
                            return -1;
                        }
                    }
                    if (AmpUT_CalibMemReset(CalId) < 0) {
                        return -1;
                    }
                }
            }
        } else {
            CalMgrPrint("no calibration Status....");
            if (AmpUT_CalibMemReset(CalId) < 0) {
                return -1;
            }
        }
    } else { //initial calibration/ Init calibration (set parameters to other components)
        if (CalObj->InitFunc != NULL) {
            if ((CalObj->InitFunc)(CalObj) < 0) {
                CalMgrPrint("[CAL] Site %s %d init fail", CalName, CalId);
                return -1;
            }
        }
        CalMgrPrint("[CAL] Init site %s %d success", CalName, CalId);
    }
    return 0;
}    

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibVersionCheck
 *
 *  @Description:: check calibration version
 *
 *  @Input      ::
 *          CalId: calibration ID
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibVersionCheck(UINT32 CalId)
{
    Cal_Obj_s *CalObj;
    char *CalName;

    CalObj = AmpUT_CalibGetObj(CalId);

    if (CalObj->Enable == 0) {
        CalMgrPrint("[CAL] site %d didn't Enable", CalId);
        return 0;
    }
    CalName = CalObj->Name;
    // Calibration version check
    if (CalObj->UpgradeFunc != NULL) {
        Cal_Stie_Status_s *cal_site = AmpUT_CalibGetSiteStatus(CalId);

        if ((CalObj->UpgradeFunc)(CalObj, cal_site) < 0) {
            CalMgrPrint("[CAL] Site %s %d upgrade fail", CalName, CalId);
            return -1;
        }
    }
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSetExposureValue
 *
 *  @Description:: set exposure value for AE
 *
 *  @Input      ::
 *          Shutter: shutter time
 *          Agc    : sensor gain
 *          Dgain  : digital gain
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          int: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibSetExposureValue(float Shutter, float Agc, UINT32 Dgain)
{
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0 } };
    int Rval = 0;
    UINT8 ChNo = 0;
    UINT32 gainFactor = 0;
    UINT32 aGainCtrl = 0;
    UINT32 dGainCtrl = 0;
    UINT32 shutterCtrl = 0;
    AMBA_SENSOR_STATUS_INFO_s status;
    UINT8 exposureFrames = 0;

    AmbaSensor_ConvertGainFactor(Chan, Agc, &gainFactor, &aGainCtrl, &dGainCtrl);
    AmbaSensor_SetAnalogGainCtrl(Chan, aGainCtrl);
    AmbaSensor_SetDigitalGainCtrl(Chan, dGainCtrl);
    AmbaSensor_ConvertShutterSpeed(Chan, Shutter, &shutterCtrl);
    AmbaSensor_GetStatus(Chan, &status);
    exposureFrames = (shutterCtrl/status.ModeInfo.NumExposureStepPerFrame);
    exposureFrames = (shutterCtrl%status.ModeInfo.NumExposureStepPerFrame)? exposureFrames+1: exposureFrames;
    AmbaSensor_SetSlowShutterCtrl(Chan, exposureFrames);
    AmbaSensor_SetShutterCtrl(Chan, shutterCtrl);
    AmbaImg_Proc_Cmd(MW_IP_SET_DGAIN, (UINT32)ChNo, (UINT32)Dgain, 0);

    return Rval;
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibTableMapping
 *
 *  @Description:: the mapping for calibration table for multi-channel
 *
 *  @Input      ::
 *          Id: table id
 *          Channel: channel id. CH_ALL for all tables?
 *
 *  @Output     ::
 *          None
 *
 *  @Return     ::
 *          INT: return (Channel+(Id*CALIB_CH_NO))
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibTableMapping(UINT8 Channel, UINT8 Id)
{
    if (Channel == CALIB_CH_ALL) {
        return Id;
    } else {
        return (Channel+(Id*CALIB_CH_NO));
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSDCardFileExists
 *
 *  @Description:: check whether the file exists in SD card
 *
 *  @Input      ::
 *          Filename:   File path in SD card
 *
 *  @Output     ::
 *          None
 *
 *  @Return     ::
 *          INT: Exists:1/Not Exists:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibSDCardFileExists(char *Filename)
{
    AMBA_FS_FILE *Fid = NULL;
    int Rval;

    Fid = AmbaFS_fopen(Filename, "r");
    if (Fid != NULL ) {
        Rval = 1;
    } else {
        Rval = 0;
    }
    AmbaFS_fclose(Fid);
    return Rval;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibRawPP
 *
 *  @Description:: Post info for AmpUT_CalibMgrRawCapture()
 *
 *  @Input      ::
 *          PostpInfo: info for AmpUT_CalibMgrRawCapture
 *  @Output     ::
 *          None
 *
 *  @Return     ::
 *          int: OK:0
\*-----------------------------------------------------------------------------------------------*/
UINT32 AmpUT_CalibRawPP(AMP_STILLENC_POSTP_INFO_s *PostpInfo)
{
    char Fn[64];
    AMBA_FS_FILE *raw = NULL;

    AmbaPrint("[CalibRawPP] Raw 0x%X, p-%d, w-%d, h-%d", \
              PostpInfo->media.RawInfo.RawAddr, \
              PostpInfo->media.RawInfo.RawPitch, \
              PostpInfo->media.RawInfo.RawWidth, \
              PostpInfo->media.RawInfo.RawHeight);



    sprintf(Fn,"C:\\MGR.RAW");
    Fn[0] = AmpUT_CalibGetDriverLetter();

    raw = AmbaFS_fopen(Fn, "w");
    AmbaPrint("[Amp_UT]Dump Raw %s Start!", Fn);
    AmbaFS_fwrite(PostpInfo->media.RawInfo.RawAddr, \
                  PostpInfo->media.RawInfo.RawPitch*PostpInfo->media.RawInfo.RawHeight, 1, raw);
    AmbaFS_fclose(raw);

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibMgrRawCapture
 *
 *  @Description:: Raw capture with buffer allocate by lib
 *
 *  @Input      ::
 *          None
 *  @Output     ::
 *          None
 *
 *  @Return     ::
 *          int: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibMgrRawCapture(UINT16 SensorMode, UINT16 Flip)
{

    //static is necessary for b5
    static AMP_STILLENC_POSTP_s pp_RawReady_cb = {.Process = AmpUT_CalibRawPP};
    UINT8 flag = 0x1;

    AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s ItunerRawCapCtrl = {
        .RawBufSource = 0,
        .ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING,
        .SensorMode = {
            .Bits = {
                .Mode = 0,
                .VerticalFlip = 0,
            }
        },
    };
    ItunerRawCapCtrl.SensorMode.Bits.Mode = SensorMode;
    ItunerRawCapCtrl.SensorMode.Bits.VerticalFlip = Flip;
    ItunerRawCapCtrl.RawBuff.PostProc = &pp_RawReady_cb;

    if (AmpUT_ItunerRawCapture) {
        AmpUT_ItunerRawCapture(flag, ItunerRawCapCtrl);
    } else {
        AmbaPrint("Null AmpUT_ItunerRawCapture");
        K_ASSERT(0);
    }

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibCheckStructure
 *
 *  @Description:: check calib structure size alignment
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibCheckStructure(void)
{
#define Check512(Data)  (sizeof(Data)%512)
    CalMgrPrint("BPC structure check.");
    if (Check512(BPC_Nand_s)) {
        CalMgrPrint("BPC_Nand_s (%d bytes) not mod 512 bytes.", sizeof(BPC_Nand_s));
    }
    CalMgrPrint("VNC structure check.");
    if (Check512(Vignette_Control_s)) {
        CalMgrPrint("Vignette_Control_s size %d bytes not mod 512 bytes.", sizeof(Vignette_Control_s));
    }
    if (Check512(Vignette_Pack_Storage_s)) {
        CalMgrPrint("Vignette_Pack_Storage_s size %d bytes not mod 512 bytes.", sizeof(Vignette_Pack_Storage_s));
    }
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUT_CalibSetDspMode
 *
 *  @Description:: set dsp mode for calibration
 *
 *  @Input      ::
 *          pMode : DSP image mode config
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/

int AmbaUT_CalibSetDspMode(AMBA_DSP_IMG_MODE_CFG_s *pMode)
{
    memcpy(&CalibImgMode, pMode, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUT_CalibGetDspMode
 *
 *  @Description:: get dsp mode for calibration
 *
 *  @Input      ::
 *          pMode : DSP image mode config
  *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmbaUT_CalibGetDspMode(AMBA_DSP_IMG_MODE_CFG_s *pMode)
{
    memcpy(pMode, &CalibImgMode, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUT_CalibRotateMap
 *
 *  @Description:: rotate map
 *
 *  @Input      ::
 *          SourceMap: source map
 *          MapWidth:   source map width
 *          MapHeight:  source map height
 *          Rotate:        rotate degree. AMBA_DSP_ROTATE_0/AMBA_DSP_ROTATE_90/AMBA_DSP_ROTATE_180/AMBA_DSP_ROTATE_270
 *
 *
 *  @Output     ::
 *          TargetMap: Map after rotate
 *
 *  @Return     ::
 *          int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmbaUT_CalibRotateMapTest(UINT16 *TargetMap, UINT16 *SourceMap, UINT16 MapWidth, UINT16 MapHeight, UINT8 Rotate)
{
    int Width, Height;
    for (Height = 0; Height<MapHeight; ++Height) {
        for (Width =0; Width< MapWidth; ++Width) {
            if (Rotate == AMBA_DSP_ROTATE_0) { //clockwise rotate 0 degree
                TargetMap[Width+Height*MapWidth] = SourceMap[Width+Height*MapWidth];
            } else if (Rotate == AMBA_DSP_ROTATE_90) { //clockwise rotate 90 degree
                TargetMap[Width*MapHeight+(MapHeight-Height-1)] = SourceMap[Width+Height*MapWidth];
            } else if (Rotate == AMBA_DSP_ROTATE_180) { //clockwise rotate 180 degree
                TargetMap[(MapWidth-Width-1)+(MapHeight-Height-1)*MapWidth] = SourceMap[Width+Height*MapWidth];
            } else if (Rotate == AMBA_DSP_ROTATE_270) { //clockwise rotate 270 degree
                TargetMap[(MapWidth-Width-1)*MapHeight+Height] = SourceMap[Width+Height*MapWidth];
            }
        }
    }
    return 0;
}

void AmbaUT_CalibSwap(UINT16 * A, UINT16 * B)
{
    UINT16 C;
    C=*A;
    *A=*B;
    *B=C;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUT_CalibRotateMap
 *
 *  @Description:: The calibration ae handler
 *
 *  @Input      ::
 *  MAP:            buffer address
 *  MapWidth:     buffer width
 *  MapHeight:    buffer height
 *  Rotate:         direction
 *  @Output     ::
 *
 *  @Return     ::
 *          Void
\*-----------------------------------------------------------------------------------------------*/
int AmbaUT_CalibRotateMap( UINT16 *Map, UINT16 MapWidth, UINT16 MapHeight, UINT8 Rotate)
{
    int Mapsize=MapWidth*MapHeight;
    int m,n;
    switch (Rotate) {
        case AMBA_DSP_ROTATE_0:
            break;
        case AMBA_DSP_ROTATE_0_HORZ_FLIP:
            for (m=0; m<MapHeight; ++m) {
                for (n=0; n<MapWidth >> 1; ++n) {
                    AmbaUT_CalibSwap(&Map[n+m*MapWidth], &Map[MapWidth-1-n + m*MapWidth]);
                }
            }
            break;
        case  AMBA_DSP_ROTATE_180:
            for (n=0; n < Mapsize >> 1; ++n) {
                AmbaUT_CalibSwap(&Map[n], &Map[Mapsize -1-n]);
            }
            break;
        case AMBA_DSP_ROTATE_180_HORZ_FLIP:
            for (n=0; n<MapWidth; ++n) {
                for (m=0; m<MapHeight >> 1; ++m) {
                    AmbaUT_CalibSwap(&Map[n+m*MapWidth], &Map[n + (MapHeight - 1 - m)*MapWidth]);
                }
            }
            break;
        default:
            AmbaPrintColor(RED, "Not valid rotation mode.");
            break;
    }

    return 0;
}


