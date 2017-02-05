/**
 * @file mw/unittest/extSrc/sample_calib/bpc/AmbaUTCalibBpc.c
 *
 * sample code for bad pixel calibration
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
#include <math.h>
#include "AmbaUTCalibBPC.h"
#include "AmbaUtility.h"



#define BPC_UNPACK_NONE 0
#define BPC_UNPACK_DONE 1
#define BPC_UNPACK_NG      2
#define BPC_UNPACK_RUNNING     3
#define NA 0xff
#define BPC_TYPE_BRIGHT_PIXEL 0
#define BPC_TYPE_DARK_PIXEL 1

#define BPC_RAW_FROM_RAW_CAPTURE 0
#define BPC_RAW_FROM_SD_CARD     1

/**
*  Badpixel related structure
*/
typedef struct Badpixel_Detection_s_ {
    int BadPixelPage;           /**< bad pixel page, the page no. is depending on */
    int BufferIndex;            /**< bpc map index */
    int BPCMode;                /**< BPC mode, define in the sensor driver */
    int Flip;                   /**< 0:disable flip,1:enable flip */
    int Channel;                /**< channel No */
    int Type;                   /**< 0: bright bad pixel,1:dark bad pixel */
    int Num;                    /**< bad pixel number */
    float Threshold;              /**< threshold to detect bad pixel */
    INT16  AeTarget;            /**< -1 for manual AE info */
    UINT8  FlickerMode;         /**< Anti flicker mode. 50 or 60 */
    UINT16 ManualIrisIdx;       /**< Manual iris index, only applied to manual mode */
    float AgcGain;              /**< agcgain value */
    float InverseShutterTime;   /**< shuttertime = 1.0/InverseShutterTime */
    int BlockW;                 /**< detect block width */
    int BlockH;                 /**< detect block height */
    int ClearOld;               /**< 0:keep buffer data,1:clear buffer data */
    int DebugFlag;              /**< 1: save raw image */
    int MaxBadPixel;            /**< bad pixel number threshold */
    char *DebugFilePrefix;      /**< output debug filename prefix */
    UINT32 RawAddress;          /**< image raw data address in memory */
    UINT32 BufferAddress;          /**< bpc buffer address in memory */
    UINT32 LoadRawImage;        /**< 0:from raw capture 1: from sd card */

} BadPixel_Detection_s;

UINT8 SensorFlip = 0;
char BPCMapStr[16] = {"BPC Mode"};
BPC_Buffer_Setup_t BPCBufferSetup[BPC_MAX_MODES*CALIB_CH_NO];
BPC_Obj_t G_BPCObj;
BPC_Mode_Lut_s BPCModeLut[BPC_MAX_MODES*CALIB_CH_NO];

extern AMBA_KAL_BYTE_POOL_t G_MMPL;

int AmpUTCalibBPC_MapDecompress(UINT8 JobID);
BPC_Buffer_s* AmpUTCalibBPC_GetBufferHeader(UINT8 BufferIndex);

unsigned char ReversalTable[256] = {
    0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
    0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
    0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
    0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
    0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
    0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
    0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
    0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
    0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
    0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
    0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
    0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
    0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
    0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
    0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
    0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_GetEffectiveWidthHeight
 *
 *  @Description:: get the effective sensor Width and Height for BPC Mode
 *
 *  @Input      ::
 *          BPCBuffer: the BPC buffer
 *          Flip: sensor Flip(1) or Normal(0)
 *
 *  @Output     ::
 *          Width: effective sensor Width of the BPC MODE
 *          Height: effective sensor Height of the BPC MODE
 *
 *  @Return     ::
 *          int : OK(G_BPCObj.GeometryValid[BPCMode])/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_GetEffectiveWidthHeight(UINT32 BPCBufferIndex, UINT8 Flip, UINT32 *Width, UINT32 *Height) // Get Width/Height from sensor driver
{
    *Width  = G_BPCObj.EffectiveGeometry[Flip][BPCBufferIndex].Width;
    *Height = G_BPCObj.EffectiveGeometry[Flip][BPCBufferIndex].Height;

    return G_BPCObj.GeometryValid[Flip][BPCBufferIndex];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_GetOBWidthHeight
 *
 *  @Description:: get the OB sensor Width and Height for BPC Mode
 *
 *  @Input      ::
 *          BPCBuffer: the BPC buffer
 *          Flip: sensor Flip(1) or Normal(0)
 *  @Output     ::
 *          Width: OB sensor Width of the BPC MODE
 *          Height: OB sensor  Height of the BPC MODE
 *
 *  @Return     ::
 *          int : OK(G_BPCObj.GeometryValid[BPCMode])/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_GetOBWidthHeight(UINT32 BPCBufferIndex, UINT8 Flip, UINT32 *Width, UINT32 *Height) // Get Width/Height from sensor driver
{
    *Width  = G_BPCObj.OBGeometry[Flip][BPCBufferIndex].Width;
    *Height = G_BPCObj.OBGeometry[Flip][BPCBufferIndex].Height;

    return G_BPCObj.GeometryValid[Flip][BPCBufferIndex];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_GetEffectiveGeometry
 *
 *  @Description:: get effective sensor geometry for BPC Mode
 *
 *  @Input      ::
 *          BPCBuffer: the BPC Buffer
 *          Flip: sensor Flip(1) or Normal(0)
 *
 *  @Output     ::
 *          Geometry: effective geometry of bpc mode
 *
 *  @Return     ::
 *          int : OK(G_BPCObj.GeometryValid[BPCMode])/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_GetEffectiveGeometry(UINT32 BPCBuffer, UINT8 Flip, AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s *Geometry) // Get Width/Height from sensor driver
{
    *Geometry = G_BPCObj.EffectiveGeometry[Flip][BPCBuffer];
    return G_BPCObj.GeometryValid[Flip][BPCBuffer];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_GetOBGeometry
 *
 *  @Description:: get OB sensor geometry for BPC Mode
 *
 *  @Input      ::
 *          BPCBuffer: the BPC Buffer
 *          Flip: sensor Flip(1) or Normal(0)
 *
 *  @Output     ::
 *          Geometry: OB geometry of bpc mode
 *
 *  @Return     ::
 *          int : OK(G_BPCObj.GeometryValid[BPCMode])/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_GetOBGeometry(UINT32 BPCBuffer, UINT8 Flip, AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s *Geometry) // Get Width/Height from sensor driver
{
    *Geometry = G_BPCObj.OBGeometry[Flip][BPCBuffer];
    return G_BPCObj.GeometryValid[Flip][BPCBuffer];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_GetOBOffset
 *
 *  @Description:: get the offset of optical black for BPC Mode
 *
 *  @Input      ::
 *          BPCBufferIndex: the BPC buffer index
 *          Flip: sensor Flip(1) or Normal(0)
 *
 *  @Output     ::
 *          OBOffsetX: Offset X of OB sensor mode
 *          OBOffsetY: Offset Y of OB sensor mode
 *
 *  @Return     ::
 *          int : OK(G_BPCObj.GeometryValid[BPCMode])/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_GetOBOffset(UINT32 BPCBufferIndex, UINT8 Flip, UINT32 *OBOffsetX, UINT32 *OBOffsetY)
{
    *OBOffsetX = G_BPCObj.OBOffset[Flip][BPCBufferIndex].X;
    *OBOffsetY = G_BPCObj.OBOffset[Flip][BPCBufferIndex].Y;
    return G_BPCObj.GeometryValid[Flip][BPCBufferIndex];
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BPCTableMapping
 *
 *  @Description:: the mapping table for vignette table for multi-channel
 *
 *  @Input      ::
 *          Id: table id
 *          Channel: channel id. VIG_CH_ALL for all tables?
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *          INT: return (Channel+(Id*CALIB_CH_NO))
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BPCTableMapping(UINT8 Channel, UINT8 ModId)
{
    if (Channel == BPC_CH_ALL) {
        return ModId;
    } else {
        return (Channel+(ModId*CALIB_CH_NO));
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_InitBPCModeLut
 *
 *  @Description:: Initialize the BPC mode lookup table
 *
 *  @Input      ::
 *          BPCModeLut: lookup table for BPC
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int : OK
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_InitBPCModeLut(BPC_Mode_Lut_s *BPCModeLut)
{
    UINT8 i=0,Channel,Index;
    for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
        for (i=0; i<(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); i++) {
            Index = AmpUTCalibBPC_BPCTableMapping(Channel,i);
            BPCModeLut[Index].HSubSample.FactorNum = NA;
            BPCModeLut[Index].HSubSample.FactorDen = NA;
            BPCModeLut[Index].VSubSample.FactorNum = NA;
            BPCModeLut[Index].VSubSample.FactorDen = NA;
            BPCModeLut[Index].BPCMode           = BPC_MODE0+i;
            BPCModeLut[Index].SensorCalMode     = SENSOR_CAL_BPC_MODE0+i;
            BPCBufferSetup[Index].Enable        = 1;
            BPCBufferSetup[Index].BPCMode       = BPC_MODE0+i;
            BPCBufferSetup[Index].BPCPage       = BPC_PAGE0+Index;
            BPCBufferSetup[Index].ActiveBuffer  = BPC_BUFFER0+Index;
            BPCBufferSetup[Index].Option        = BPC_BUFFER_OPTION_NONE;
        }
    }
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_DebugEnable
 *
 *  @Description:: Initialize the BPC mode lookup table
 *
 *  @Input      ::
 *          Enable: enable flag to print debug message
 *
 *  @Output     :: None
 *
 *  @Return     :: None
\*-----------------------------------------------------------------------------------------------*/
void AmpUTCalibBPC_DebugEnable(UINT32 Enable)
{
    G_BPCObj.BPCDebugEn = (Enable>0);
}


/* ---------- */
// BPC NAND Layer
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_NandSetPackAddr
 *
 *  @Description:: set BPC Nand structure address
 *
 *  @Input      ::
 *          BPCNand: BPC Nand address
 *
 *  @Output     :: None
 *
 *  @Return     :: None
\*-----------------------------------------------------------------------------------------------*/
void AmpUTCalibBPC_NandSetPackAddr(void *BPCNand)
{
    G_BPCObj.BPCNand = (BPC_Nand_s*) BPCNand;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_NandGetPage
 *
 *  @Description:: get page information from Nand
 *
 *  @Input      ::
 *          Page: page no.
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *          return page information for specific page
\*-----------------------------------------------------------------------------------------------*/
BPC_Page_s* AmpUTCalibBPC_NandGetPage(UINT32 Page)
{
    return (BPC_Page_s*)(&((G_BPCObj.BPCNand->BPCPage)[Page]));
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_NandErasePage
 *
 *  @Description:: Erase the page data
 *
 *  @Input      ::
 *          Page: page no.
 *          ErasePageHeader: flag to erase page header, erase:1/ don't erase:0
 *          ErasePageContent: flag to erase page content, erase:1/ don't erase:0
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *         OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_NandErasePage(UINT32 Page, UINT32 ErasePageHeader, UINT32 ErasePageContent)
{
    BPC_Page_s *BPCPage = AmpUTCalibBPC_NandGetPage(Page);

    if (ErasePageContent) {
        if (BPCPage->PageValid > 0) {
            UINT32 PageData = (UINT32)(&(G_BPCObj.BPCNand->Data)) + BPCPage->Offset;
            memset((UINT8*)PageData, 0, BPCPage->PageSize); // Erase Page content
        }
    }
    if (ErasePageHeader) {
        memset(BPCPage, 0, sizeof(BPC_Page_s));
    }

    return 0;
}


/* ---------- */
// BPC NAND Layer: Badpixel Page functions
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_NandGetBadpixelPage
 *
 *  @Description:: get badpixel page header from Nand
 *
 *  @Input      ::
 *          Page: page no.
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *         return the bad pixel page header
\*-----------------------------------------------------------------------------------------------*/
BadPixel_Page_s* AmpUTCalibBPC_NandGetBadpixelPage(UINT32 Page)
{
    BPC_Page_s *BPCPage = AmpUTCalibBPC_NandGetPage(Page);
    BPC_Nand_s *BPCNand = G_BPCObj.BPCNand;
    UINT32 BadPixelPage = (UINT32)(&(BPCNand->Data)) + BPCPage->Offset;

    return (BadPixel_Page_s*)BadPixelPage;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_NandGetBadpixelPageData
 *
 *  @Description:: get badpixel page data from Nand
 *
 *  @Input      ::
 *          Page: page no.
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *         return the bad pixel page data
\*-----------------------------------------------------------------------------------------------*/
UINT8* AmpUTCalibBPC_NandGetBadpixelPageData(UINT32 Page)
{
    BadPixel_Page_s *BPCBadPixelPage = AmpUTCalibBPC_NandGetBadpixelPage(Page);
    UINT32 CompressedMap = (UINT32)(&(BPCBadPixelPage->CompressedMap));

    return (UINT8*)CompressedMap;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_NandGetBadpixelCount
 *
 *  @Description:: count the number of bad pixel
 *
 *  @Input      ::
 *          Page: page no.
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *         return number of bad pixel
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_NandGetBadpixelCount(UINT32 Page)
{
    BPC_Page_s *BPCPage = AmpUTCalibBPC_NandGetPage(Page);
    BadPixel_Page_s *BPCBadPixelPage = AmpUTCalibBPC_NandGetBadpixelPage(Page);

    if (BPCPage->PageValid == 0) {
        return 0;
    }

    if (BPCBadPixelPage->RecordValid == 0) {
        return 0;
    }

    return BPCBadPixelPage->NumBadPixels;
}

/* ---------- */
// BPC buffer management
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_GetBufferHeader
 *
 *  @Description:: get the header of BPC buffer
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *         return header of BPC buffer
\*-----------------------------------------------------------------------------------------------*/
BPC_Buffer_s* AmpUTCalibBPC_GetBufferHeader(UINT8 BufferIndex)
{
    return (BPC_Buffer_s*)(&G_BPCObj.BPCBuffer[BufferIndex]);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_GetBufferSize
 *
 *  @Description:: get the buffer size of spcific sensor mode
 *
 *  @Input      ::
 *          BPCMode: BPC mode
 *          Option: single sensor or 2 sensor input
 *
 *  @Output     ::
 *          Size: the buffer size of the mode
 *
 *  @Return     ::
 *         return header of BPC buffer
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_GetBufferSize(UINT8 BufferIndex,UINT32 Option,UINT32 *Size)
{
    UINT32 Width = 0,Height = 0;

    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        AmpUTCalibBPC_GetOBWidthHeight(BufferIndex, 0, &Width, &Height);
    } else {
        AmpUTCalibBPC_GetEffectiveWidthHeight(BufferIndex, 0, &Width, &Height);
    }
    *Size = AmpCalib_BPCWidthToPitchExt(Width)*Height>>3;

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_CreateBuffer
 *
 *  @Description:: create buffer for BPC mode
 *
 *  @Input      ::
 *          Size: single sensor or 2 sensor input
 *          BPCS3dMode: flag for S3D
 *
 *  @Output     ::
 *          RawAddr: address of BPC mode
 *  @Return     ::
 *         INT :OK/0
\*-----------------------------------------------------------------------------------------------*/
static int AmpUTCalibBPC_CreateBuffer(UINT8 **RawAddr, UINT32 Size, UINT8 BPCS3dMode)
{
    MW_BPC_Buffer_Header_s *BPCBuffer;
    AMBA_MEM_CTRL_s Buffer;

    // Allocate additional FPN_BUFFER_HEADER_SIZE bytes for header
    int Rval;

    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &Buffer.pMemAlignedBase, &Buffer.pMemBase, (Size+BPC_BUFFER_HEADER_SIZE), 32);
    if (Rval != OK) {
        CAL_PRINT("[AmpUTCalibBpc]allocate bad pixel buffer fail (%u)!", Size+BPC_BUFFER_HEADER_SIZE);
        return Rval;
    } else {
        BPCBuffer = (MW_BPC_Buffer_Header_s *)(Buffer.pMemAlignedBase);
        *RawAddr = (UINT8 *)(Buffer.pMemAlignedBase) + BPC_BUFFER_HEADER_SIZE;
    }
    BPCBuffer->MagicNumber = BPC_BUFFER_MAGIC_NUMBER;
    BPCBuffer->Size = Size;
    if (G_BPCObj.BPCDebugEn) {
        AmbaPrint("[BPC-IA]BPCBuffer=0x%08X, RawAddr=0x%08X, Size=%d", BPCBuffer, *RawAddr, Size);
    }

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BufferAllocate
 *
 *  @Description:: allocate buffer for all BPC mode
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *         INT :OK/0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BufferAllocate(void)
{
    int ModeIndex = 0, Err=0;
    UINT8 BPCS3dMode = 0;
    Cal_Stie_Status_s *p_cal_site;
    BPC_Page_s *BPCPage;
    UINT8 Channel;
    BPC_Buffer_Setup_t *BufferSetup;
    UINT32 BPCMode; // Calculate buffer Size according to BPCMode
    UINT32 Option = 0;
    UINT32 Size = 0;
    UINT8 BufferIndex = 0;

    for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
        for (ModeIndex=0; ModeIndex<(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); ModeIndex++) {
            BufferIndex = AmpUTCalibBPC_BPCTableMapping(Channel,ModeIndex);
            BufferSetup = &BPCBufferSetup[BufferIndex];
            BPCMode = BufferSetup->BPCMode;


            if (BufferSetup->Enable > 0) {

                
                // get BPC Map Size
                if (AmpUTCalibBPC_GetBufferSize(BufferIndex, Option, &Size) == -1) {
                    CAL_PRINT("[BPC APP] Sensor driver didn't report %s%d Channel = %d BPC Geometry info, unable create buf #%d", BPCMapStr,BPCMode, Channel, ModeIndex);
                    continue;
                }
                {
                    // Call IA to allocate BPC buffer
                    BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(BufferIndex);
                    if (BufferHeader->MemoryAllocated == 0) {

                        if (AmpUTCalibBPC_CreateBuffer(&(BufferHeader->BufferAddr), Size, BPCS3dMode) < 0) {
                            CAL_PRINT("[BPC APP] Allocate buf #%d Channel = %d fail", BufferIndex,Channel);
                        }

                        p_cal_site = AmpUT_CalibGetSiteStatus(CAL_BPC_ID);
                        if (p_cal_site->Status == CAL_SITE_RESET ) {
                            memset(BufferHeader->BufferAddr, 0, Size);
                        }
                        BPCPage = AmpUTCalibBPC_NandGetPage(BufferIndex);
                        if (!(BPCPage->PageValid)) {
                            memset(BufferHeader->BufferAddr, 0, Size);
                        }
                        BufferHeader->Channel = Channel;
                        BufferHeader->BPCMode = BPCMode;
                        BufferHeader->Size = Size;
                        BufferHeader->MemoryAllocated = 1;

                        if (1) {
                            CAL_PRINT("[BPC APP] Allocate buf #%d BPCMode = %d Cahnnel = %d Size=%d addr = 0x%08X",
                                      BufferIndex,BPCMode,Channel,Size,BufferHeader->BufferAddr);
                        }

                    } else {
                        CAL_PRINT("[BPC APP] Mode #%d Channel =%d buf = %d is already allocated",ModeIndex,Channel,BufferIndex);
                    }
                }
            }
        }
    }
    return Err;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BufferClear
 *
 *  @Description:: clear BPC buffer
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *
 *  @Output     :: None
 *  @Return     ::
 *         INT :OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BufferClear(UINT32 BufferIndex)
{
    BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(BufferIndex);

    if (BufferHeader->MemoryAllocated) {
        memset(BufferHeader->BufferAddr, 0, BufferHeader->Size);
        return 0;
    } else {
        CAL_PRINT("[BPC APP] Buffer#%d is not allocated", BufferIndex);
        return -1;
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BufferGet
 *
 *  @Description:: get the buffer address of specific buffer Index
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *
 *  @Output     ::
 *          mem: address of the BPC buffer
 *  @Return     ::
 *         INT :OK:0 / NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BufferGet(UINT32 BufferIndex, UINT8 **Mem)
{
    BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(BufferIndex);

    if (BufferHeader->MemoryAllocated) {
        *Mem = BufferHeader->BufferAddr;
        return 0;
    } else {
        CAL_PRINT("[BPC APP] Buffer #%d is already allocated", BufferIndex);
        return -1;
    }
}


/* --------- */
// BPC buffer management: Lock/unlock/lock-status APIs
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BufferLock
 *
 *  @Description:: lock BPC buffer
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *
 *  @Output     :: None
 *  @Return     ::
 *         INT :OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BufferLock(UINT32 BufferIndex)
{
    G_BPCObj.BufferStatus.Lock[BufferIndex] = 1;
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BufferUnlock
 *
 *  @Description:: unlock BPC buffer
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *
 *  @Output     :: None
 *  @Return     ::
 *         INT :OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BufferUnlock(UINT32 BufferIndex)
{
    G_BPCObj.BufferStatus.Lock[BufferIndex] = 0;
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BufferIsLocked
 *
 *  @Description:: check the lock flag for specific buffer
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *         INT: return the lock flag
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BufferIsLocked(UINT32 BufferIndex)
{
    return G_BPCObj.BufferStatus.Lock[BufferIndex];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BMPRead
 *
 *  @Description:: Read the BMP data to the BPC buffer
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *          BPCBuffer: BPC buffer
 *          Fp: file pointer
 *
 *  @Output     :: None
 *  @Return     ::
 *         INT: OK: 0/NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BMPRead(UINT32 BufferIndex, BPC_Buffer_s* BPCBuffer, AMBA_FS_FILE *Fp)
{
    BMPFile_Magic_t BMPFileMagic = {'B', 'M'};
    BMPFile_Header_t BMPFileHeader;
    BMPInfo_Header_t BMPInfoHeader;
    BMPPalette_Bpp2_t BMPPaletteBpp2;
    UINT32 BMPOffset;
    UINT32 BMPWidth;
    int BMPHeight;
    UINT32 BMPPaddedWidth;
    UINT32 BMPDataSize;
    UINT32 BPCPaddedWidth;   // should align to 32 byte = 256 bit
    int i, j;
    UINT8 RowBuffer[1024];
    UINT8 *RowStartAddr = BPCBuffer->BufferAddr;
    UINT8 Flip;
    AMBA_SENSOR_STATUS_INFO_s SensorStatus;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0, .HdrID = 0 } };

    AmbaSensor_GetStatus(Chan, &SensorStatus);//get flip information from sensor
    Flip = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;
    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        AmpUTCalibBPC_GetOBWidthHeight(BufferIndex, Flip, &BMPWidth, (UINT32 *)&BMPHeight);
    } else {
        AmpUTCalibBPC_GetEffectiveWidthHeight(BufferIndex, Flip, &BMPWidth, (UINT32 *)&BMPHeight);
    }
    BPCPaddedWidth = BPCBuffer->Size / BMPHeight;   // should align to 32 byte = 256 bit

    AmbaFS_fread(&BMPFileMagic, 1, sizeof(BMPFileMagic), Fp);
    if (BMPFileMagic.Magic[0] != 'B' || BMPFileMagic.Magic[1] != 'M') {
        AmbaPrint("Read Error: Input file is not a BMP file\n");
        return -1;
    }

    AmbaFS_fread(&BMPFileHeader, 1, sizeof(BMPFile_Header_t), Fp);
    AmbaFS_fread(&BMPInfoHeader, 1, sizeof(BMPInfo_Header_t), Fp);
    AmbaFS_fread(&BMPPaletteBpp2, 1, sizeof(BMPPalette_Bpp2_t), Fp);

    if (BMPInfoHeader.Bpp != 1) {
        AmbaPrint("Read Error: Bits per Pixel should be 1\n");
        return -1;
    }
    BMPOffset = BMPFileHeader.BMPOffset;
    BMPDataSize = BMPFileHeader.FileSize - BMPOffset;

    if (BMPDataSize != BMPInfoHeader.BMPDataSize) {
        AmbaPrint("Read Error: Data Size Declare in Header Conflict\n");
        return -1;
    }

    BMPWidth = BMPInfoHeader.Width;
    BMPPaddedWidth = (((BMPWidth + 31)>>5)<<5 )>>3;     // should align to 4 byte = 32 bit
    BMPHeight = BMPInfoHeader.Height;

    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        if (BMPWidth != G_BPCObj.OBGeometry[Flip][BufferIndex].Width || BMPPaddedWidth > BPCPaddedWidth) {
        AmbaPrint("Read Error: Data Width Mismatch\n");
        return -1;
    }
        if (abs(BMPHeight) != G_BPCObj.OBGeometry[Flip][BufferIndex].Height) {
            AmbaPrint("Read Error: Data Height Mismatch\n");
            return -1;
        }

    } else {
        if (BMPWidth != G_BPCObj.EffectiveGeometry[Flip][BufferIndex].Width || BMPPaddedWidth > BPCPaddedWidth) {
            AmbaPrint("Read Error: Data Width Mismatch\n");
            return -1;
        }
        if (abs(BMPHeight) != G_BPCObj.EffectiveGeometry[Flip][BufferIndex].Height) {
        AmbaPrint("Read Error: Data Height Mismatch\n");
        return -1;
    }
    }


    if (BMPHeight < 0) {
        BMPHeight *= -1;
        for (i = 0; i < BMPHeight; i++) {
            AmbaFS_fread(&RowBuffer, 1, BMPPaddedWidth, Fp);
            for (j = 0; j < BMPPaddedWidth; j++) {
                RowStartAddr[j] = ReversalTable[RowBuffer[j]];
            }
            RowStartAddr += BPCPaddedWidth;
        }
    } else {
        RowStartAddr += BMPHeight * BPCPaddedWidth;
        for (i = 0; i < BMPHeight; i++) {
            RowStartAddr -= BPCPaddedWidth;
            AmbaFS_fread(&RowBuffer, 1, BMPPaddedWidth, Fp);
            for (j = 0; j < BMPPaddedWidth; j++) {
                RowStartAddr[j] = ReversalTable[RowBuffer[j]];
            }
        }
    }

    AmbaPrint("Read BMP File Done");
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BMPSave
 *
 *  @Description:: Save the BPC data to a BMP image
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *          BPCBuffer: BPC buffer
 *          Fp: file pointer
 *
 *  @Output     :: None
 *  @Return     ::
 *         INT: OK: 0/NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BMPSave(UINT32 BufferIndex, BPC_Buffer_s* BPCBuffer, AMBA_FS_FILE *Fp)
{
    BMPFile_Magic_t BMPFileMagic = {'B', 'M'};
    BMPFile_Header_t BMPFileHeader;
    BMPInfo_Header_t BMPInfoHeader;
    BMPPalette_Bpp2_t BMPPaletteBpp2;
    UINT32 BMPOffset = sizeof(BMPFile_Magic_t) + sizeof(BMPFile_Header_t) + sizeof(BMPInfo_Header_t) + sizeof(BMPPalette_Bpp2_t);
    UINT32 BMPWidth;
    UINT32 BMPHeight;
    UINT32 BMPPaddedWidth;      // should align to 4 byte = 32 bit
    UINT32 BMPDataSize;
    UINT32 BPCPaddedWidth;   // should align to 32 byte = 256 bit
    int i, j;
    UINT8 RowBuffer[1024];
    UINT8 *RowStartAddr = BPCBuffer->BufferAddr;
    UINT8 Flip;
    AMBA_SENSOR_STATUS_INFO_s SensorStatus;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0, .HdrID = 0 } };

    AmbaSensor_GetStatus(Chan, &SensorStatus);//get flip information from sensor
    Flip = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;


    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        AmpUTCalibBPC_GetOBWidthHeight(BufferIndex, Flip, &BMPWidth, &BMPHeight);
    } else {
        AmpUTCalibBPC_GetEffectiveWidthHeight(BufferIndex, Flip, &BMPWidth, &BMPHeight);
    }
    BMPPaddedWidth = (((BMPWidth + 31)>>5)<<5)>>3;      // should align to 4 byte = 32 bit
    BMPDataSize = BMPPaddedWidth * BMPHeight;
    BPCPaddedWidth = BPCBuffer->Size / BMPHeight;   // should align to 32 byte = 256 bit

    BMPFileHeader.FileSize = BMPDataSize + BMPOffset;
    BMPFileHeader.Creator1 = BMPFileHeader.Creator2 = 0;
    BMPFileHeader.BMPOffset = BMPOffset;

    BMPInfoHeader.HeaderSize = sizeof(BMPInfo_Header_t);
    BMPInfoHeader.Width = BMPWidth;
    BMPInfoHeader.Height = -BMPHeight;
    BMPInfoHeader.Planes = 1;
    BMPInfoHeader.Bpp = 1;
    BMPInfoHeader.Compression = 0;
    BMPInfoHeader.BMPDataSize = BMPDataSize;
    BMPInfoHeader.HResolution = 0;
    BMPInfoHeader.VResolution = 0;
    BMPInfoHeader.UsedColors = 0;
    BMPInfoHeader.ImportantColors = 0;

    BMPPaletteBpp2.Color[0] = 0x00000000;
    BMPPaletteBpp2.Color[1] = 0x00FFFFFF;

    AmbaFS_fwrite(&BMPFileMagic, 1, sizeof(BMPFile_Magic_t), Fp);
    AmbaFS_fwrite(&BMPFileHeader, 1, sizeof(BMPFile_Header_t), Fp);
    AmbaFS_fwrite(&BMPInfoHeader, 1, sizeof(BMPInfo_Header_t), Fp);
    AmbaFS_fwrite(&BMPPaletteBpp2, 1, sizeof(BMPPalette_Bpp2_t), Fp);

    for (i = 0; i < BMPHeight; i++) {
        for (j = 0; j < BMPPaddedWidth; j++) {
            RowBuffer[j] = ReversalTable[RowStartAddr[j]];
        }
        AmbaFS_fwrite(&RowBuffer, 1, BMPPaddedWidth, Fp);
        RowStartAddr += BPCPaddedWidth;
    }

    AmbaPrint("BMP file saved\n");
    return 0;
}

/* ---------- */
// BPC info utility
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_SetBuffer
 *
 *  @Description:: set specific value to the BPC buffer
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *          Value: the value should be between 0~255
 *
 *  @Output     :: None
 *  @Return     :: None
\*-----------------------------------------------------------------------------------------------*/
void AmpUTCalibBPC_SetBuffer(UINT8 BufferIndex,UINT8 Value)
{
    BPC_Buffer_s* BPCBuffer = AmpUTCalibBPC_GetBufferHeader(BufferIndex);
    memset(BPCBuffer->BufferAddr, Value, BPCBuffer->Size);
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_SetPixelValue
 *
 *  @Description:: set specific value to the BPC buffer
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *          Value: the value should be between 0~255
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          None
\*-----------------------------------------------------------------------------------------------*/
void AmpUTCalibBPC_SetPixelValue(UINT8 BufferIndex,UINT32 X, UINT32 Y,UINT8 Value)
{
    BPC_Buffer_s* BPCBuffer = AmpUTCalibBPC_GetBufferHeader(BufferIndex);
    UINT32 Width = 0,Height = 0,Offset;
    UINT8 Flip;
    AMBA_SENSOR_STATUS_INFO_s SensorStatus;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0, .HdrID = 0 } };

    AmbaSensor_GetStatus(Chan, &SensorStatus);//get flip information from sensor
    Flip = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;

    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        AmpUTCalibBPC_GetOBWidthHeight(BufferIndex, Flip, &Width, &Height);
    } else {
        AmpUTCalibBPC_GetEffectiveWidthHeight(BufferIndex, Flip, &Width, &Height);
    }
    Offset = ((AmpCalib_BPCWidthToPitchExt(Width)>>3)*Y)+(X>>3);
    AmbaPrintColor(RED,"BufferIndex= %d X = %d Y = %d Value = %d", BufferIndex,X,Y,Value);
    memset(((BPCBuffer->BufferAddr)+Offset), Value, 1);
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_SetRegionValue
 *
 *  @Description:: set specific region to specific value
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *          X: x position
 *          Y: y position
 *          W: region width
 *          H: region height
 *          Value: value
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          None
\*-----------------------------------------------------------------------------------------------*/
void AmpUTCalibBPC_SetRegionValue(UINT8 BufferIndex,UINT32 X, UINT32 Y, UINT32 W, UINT32 H,UINT8 Value)
{
    BPC_Buffer_s* BPCBuffer = AmpUTCalibBPC_GetBufferHeader(BufferIndex);
    UINT32 Width = 0,Height = 0,Offset;
    int i,j;
    UINT8 Flip;
    AMBA_SENSOR_STATUS_INFO_s SensorStatus;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0, .HdrID = 0 } };

    AmbaSensor_GetStatus(Chan, &SensorStatus);//get flip information from sensor
    Flip = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;

    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        AmpUTCalibBPC_GetOBWidthHeight(BufferIndex, Flip, &Width, &Height);
    } else {
        AmpUTCalibBPC_GetEffectiveWidthHeight(BufferIndex, Flip, &Width, &Height);
    }

    AmbaPrintColor(RED,"BPCBuffer->BufferAddr = %x",BPCBuffer->BufferAddr);
    for (i = 0; i < W; i++) {
        for (j = 0; j < H; j++) {
            Offset = ((AmpCalib_BPCWidthToPitchExt(Width)>>3)*(Y+j))+((X+i)>>3);
            AmbaPrintColor(RED,"BufferIndex= %d X = %d Y = %d Value = %d", BufferIndex,(X+i),(Y+j),Value);
            memset(((BPCBuffer->BufferAddr)+Offset), Value, 1);
        }
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_RotateSensor
 *
 *  @Description:: set specific region to specific value
 *
 *  @Input      ::
 *          Channel: sensor channel No
 *          Mode: sensor mode
 *          Flip: flip
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          None
\*-----------------------------------------------------------------------------------------------*/
void AmpUTCalibBPC_RotateSensor(UINT8 Channel,UINT8 Mode, UINT8 Flip)
{
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 1 } };
    AMBA_SENSOR_MODE_ID_u SensorMode = {
        .Bits = {
            .Mode = 0,
            .VerticalFlip = 0,
        }
    };
//    Chan.Bits.VinID= Channel;  //no ideal how to set it?
    SensorMode.Bits.Mode = Mode;
    SensorMode.Bits.VerticalFlip = Flip;
    AmbaSensor_Config(Chan, SensorMode, AMBA_SENSOR_ESHUTTER_TYPE_ROLLING);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_Info
 *
 *  @Description:: calibration information for BPC
 *
 *  @Input      :: None
 *
 *  @Output     :: None
 *
 *  @Return     :: None
\*-----------------------------------------------------------------------------------------------*/
void AmpUTCalibBPC_Info(void)
{
    UINT32 Page, BufferIndex, BPCMode;
    BPC_Buffer_Status_s *Config;
    char *BPCPageTypeStr[] = {"Not allocated", "Badpixel", "Reserved"};
    UINT8 Channel;

    // Print pack BPC pack info
    AmbaPrint("========== BPC Page information ==========");
    for (Page=0; Page<BPC_MAX_PAGES; Page++) {
        BPC_Page_s *BPCPage = AmpUTCalibBPC_NandGetPage(Page);

        if (BPCPage->PageValid) {
            AmbaPrint("Page #%d", Page);
            AmbaPrint("\tpage_size=%d, page_offset=%d, Type=%s",
                      BPCPage->PageSize,
                      BPCPage->Offset,
                      BPCPageTypeStr[BPCPage->Type]);
            if (BPCPage->Type == BPC_RECORD_BADPIXEL) {
                BadPixel_Page_s *BPCBadPixelPage = AmpUTCalibBPC_NandGetBadpixelPage(Page);
                if (BPCBadPixelPage->RecordValid == 0) {
                    AmbaPrint("\tBadpixel record is not Valid");
                } else {
                    AmbaPrint("\trecord_valid=%d, BufferSize=%d, PackedSize=%d, BPCMode=%s%d, Channel=%d, Width=%d, Height=%d",
                              BPCBadPixelPage->RecordValid,
                              BPCBadPixelPage->MaskSize,
                              BPCBadPixelPage->PackedSize,
                              BPCMapStr,BPCBadPixelPage->BPCMode,
                              BPCBadPixelPage->Channel,
                              BPCBadPixelPage->CalibVinSensorGeo.Width,
                              BPCBadPixelPage->CalibVinSensorGeo.Height);
                    AmbaPrint("\tnumber of bad pixels=%d", BPCBadPixelPage->NumBadPixels);
                }
            }
        }
    }

    // Print BPC buffer information
    AmbaPrint("========== BPC buffer information ==========");
    for (BufferIndex=0; BufferIndex<BPC_MAX_BUFFER; BufferIndex++) {
        BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(BufferIndex);
        if (BufferHeader->MemoryAllocated) {
            AmbaPrint("buf#%d, Channel=%d BPCMode=%d Size=%d, address=0x%X",
                      BufferIndex,BufferHeader->Channel,BufferHeader->BPCMode,BufferHeader->Size,BufferHeader->BufferAddr);
        }
    }

    // Print BPC config information
    AmbaPrint("========== BPC config information ==========");
    Config = &G_BPCObj.BufferStatus;
    for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
        for (BPCMode=0; BPCMode<(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); BPCMode++) {
            BufferIndex = AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode);
            if (Config->Valid[BufferIndex] == 1) {
                AmbaPrint("buf#%d: Channel=%d BPCMode=%d Valid=%d, lock=%d, fpn set=%d",
                          BufferIndex,Channel,BPCMode,
                          Config->Valid[BufferIndex],
                          Config->Lock[BufferIndex],
                          Config->BPCSet[BufferIndex]);
            }
        }
    }
    // Print BPC Active buffer information
    AmbaPrint("========== BPC Active Buffer Information ==========");
    for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
        for (BPCMode=0; BPCMode<(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); BPCMode++) {
            AmbaPrint("%s%d Channel = %d BPC Active buffer = %d",
                      BPCMapStr,BPCMode,Channel,
                      G_BPCObj.ActiveBuffer[AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode)]);
        }
    }

    // Print BPC calibration Geometry information
    AmbaPrint("========== BPC Geometry information ==========");
    for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
       for (BPCMode=0; BPCMode<(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); BPCMode++) {
            AMBA_SENSOR_STATUS_INFO_s SensorStatus;
            UINT8 Flip;
            AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0, .HdrID = 0 } };
            
            AmbaSensor_GetStatus(Chan, &SensorStatus);
            Flip = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;
            BufferIndex = AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode);
            AmbaPrint("%s%d Channel=%d Flip = %d BPC BPCMap information : (start_x=%d, start_y=%d, Width=%d, Height=%d \
              HSubSample.FactorNum=%d HSubSample.FactorDen=%d VSubSample.FactorNum=%d VSubSample.FactorDen=%d  )",
                      BPCMapStr,BPCMode,Channel,Flip,
                      G_BPCObj.EffectiveGeometry[Flip][BufferIndex].StartX,
                      G_BPCObj.EffectiveGeometry[Flip][BufferIndex].StartY,
                      G_BPCObj.EffectiveGeometry[Flip][BufferIndex].Width,
                      G_BPCObj.EffectiveGeometry[Flip][BufferIndex].Height,
                      G_BPCObj.EffectiveGeometry[Flip][BufferIndex].HSubSample.FactorNum,
                      G_BPCObj.EffectiveGeometry[Flip][BufferIndex].HSubSample.FactorDen,
                      G_BPCObj.EffectiveGeometry[Flip][BufferIndex].VSubSample.FactorNum,
                      G_BPCObj.EffectiveGeometry[Flip][BufferIndex].VSubSample.FactorDen);
        }
    }
    AmbaPrint("========== BPC Optical Black Geometry information ==========");
    for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
       for (BPCMode=0; BPCMode<(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); BPCMode++) {
            AMBA_SENSOR_STATUS_INFO_s SensorStatus;
            UINT8 Flip;
            AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0, .HdrID = 0 } };
            
            AmbaSensor_GetStatus(Chan, &SensorStatus);
            Flip = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;
            BufferIndex = AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode);
            AmbaPrint("%s%d Channel=%d Flip = %d BPC BPCMap information : (start_x=%d, start_y=%d, Width=%d, Height=%d \
              HSubSample.FactorNum=%d HSubSample.FactorDen=%d VSubSample.FactorNum=%d VSubSample.FactorDen=%d  )",
                      BPCMapStr,BPCMode,Channel,Flip,
                      G_BPCObj.OBGeometry[Flip][BufferIndex].StartX,
                      G_BPCObj.OBGeometry[Flip][BufferIndex].StartY,
                      G_BPCObj.OBGeometry[Flip][BufferIndex].Width,
                      G_BPCObj.OBGeometry[Flip][BufferIndex].Height,
                      G_BPCObj.OBGeometry[Flip][BufferIndex].HSubSample.FactorNum,
                      G_BPCObj.OBGeometry[Flip][BufferIndex].HSubSample.FactorDen,
                      G_BPCObj.OBGeometry[Flip][BufferIndex].VSubSample.FactorNum,
                      G_BPCObj.OBGeometry[Flip][BufferIndex].VSubSample.FactorDen);
        }
    }
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_MapDecompress
 *
 *  @Description:: decompress bad pixel data to the bad pixel buffer
 *
 *  @Input      ::
 *          JobID: initial or update BPC Map
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *          INT: OK: BPC_UNPACK_DONE/ NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_MapDecompress(UINT8 JobID) // Update buffer information according to current sensor mode
{
    int Rval = -1;
    UINT8   BPCPage;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 1 } };
    AMBA_SENSOR_STATUS_INFO_s SensorStatus;
    UINT8   FlipMirror = 0;
    int BPCModeIndex;
    BPC_Buffer_s *BufferHeader;
    UINT32 BPCMode;
    BadPixel_Page_s *BPCBadPixelPage;
    BPC_Page_s *BPCPageInfo;
    UINT8 Channel,BufferIndex;
    AMBA_SENSOR_MODE_ID_u SensorMode = {
        .Bits = {
            .Mode = 0,
            .VerticalFlip = 0
        }
    };
    AMBA_SENSOR_MODE_INFO_s VinInfo;
    memset(&SensorStatus, 0, sizeof(AMBA_SENSOR_STATUS_INFO_s));

    // Update all buffers according to setup information
    for (Channel = 0; Channel < CALIB_CH_NO; Channel++) {
        for (BPCMode = 0; BPCMode < (pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); BPCMode++) {
            AmbaSensor_GetStatus(Chan, &SensorStatus); //need to add channel parameter
            FlipMirror = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;
            BufferIndex = AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode);
            BPCModeIndex = BPCBufferSetup[BufferIndex].BPCMode;
            BPCPage = AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode)+(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode)*CALIB_CH_NO*(FlipMirror);
            BPCPageInfo = AmpUTCalibBPC_NandGetPage(BPCPage);
            BufferHeader = AmpUTCalibBPC_GetBufferHeader(BufferIndex);
            if (G_BPCObj.BPCDebugEn) {
                AmbaPrint("BPCMode = %d Channel = %d BufferIndex = %d",BPCMode,Channel,BufferIndex);
                AmbaPrint("%s , %d BPCPage = %d FlipMirror = %d ",__func__,__LINE__,BPCPage,FlipMirror);
            }
            if (BPCModeIndex < 0) {
                continue;
            }

            // Check whether the buffer is allocated
            if (BufferHeader->MemoryAllocated == 0) {
                AmbaPrint("[BPC APP] [BPC]Channel=%d BPCMode=%d Buffer #%d is not allocated,", Channel,BPCMode,BufferIndex);
                continue;
            }
            // Check whether the BPC buffer is locked
            if (AmpUTCalibBPC_BufferIsLocked(BufferIndex) == 1) {
                AmbaPrint( "[BPC] Buffer #%d is locked, BPCMap update is forbidden", BufferIndex);
                continue;
            }
            // If Page changed, or 1st time decompress, reload it
            if ( (G_BPCObj.BufferStatus.Valid[BufferIndex] == BPC_UNPACK_NONE)) { // 1st time decompress
                G_BPCObj.BufferStatus.Valid[BufferIndex] = BPC_UNPACK_RUNNING;
                if (BPCPageInfo->PageValid) {
                    MW_BPC_Buffer_Header_s *BPCBufferHeader = (MW_BPC_Buffer_Header_s *)(BufferHeader->BufferAddr - BPC_BUFFER_HEADER_SIZE);
                    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s GeometryCal;

                    BPCBadPixelPage = AmpUTCalibBPC_NandGetBadpixelPage(BPCPage);
                    SensorMode.Bits.Mode = pAmbaCalibInfoObj[Channel]->AmbaBadPixelModeTable[BPCBadPixelPage->BPCMode];
                    AmbaSensor_GetModeInfo(Chan, SensorMode, &VinInfo);
                    
                    if (AmpCalib_GetOpticalBlackFlag() == 1) {
                        GeometryCal.Width = VinInfo.OutputInfo.OutputWidth;
                        GeometryCal.Height = VinInfo.OutputInfo.OutputHeight;
                    } else {
                    	GeometryCal.Width = VinInfo.OutputInfo.RecordingPixels.Width;
                    	GeometryCal.Height = VinInfo.OutputInfo.RecordingPixels.Height;
                    }
                    if (BPCBadPixelPage->CalibVinSensorGeo.Width != GeometryCal.Width) {
                        AmbaPrint("[FPN-IA][HW-CONV] Badpixel: page width(%d) != sensor driver reported width(%d)", BPCBadPixelPage->CalibVinSensorGeo.Width, GeometryCal.Width);
                        continue;
                    }
                    if (BPCBadPixelPage->CalibVinSensorGeo.Height != GeometryCal.Height) {
                        AmbaPrint("[FPN-IA][HW-CONV] Badpixel: page height(%d) != sensor driver reported height(%d)", BPCBadPixelPage->CalibVinSensorGeo.Height, GeometryCal.Height);
                        continue;
                    }
                    // Check whether memory size is sufficient for 1-bit static badpixel (roughly check)
                    if (BPCBufferHeader->Size < ((GeometryCal.Width * GeometryCal.Height) >>3) ) {
                        AmbaPrint("[FPN-IA][HW-CONV] Buffer: the assigned buffer size is too small to decompress");
                        continue;
                    }
                    if ((BPCBadPixelPage->BPCMode > BPCModeLut[(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode)-1].BPCMode) || (BPCBadPixelPage->BPCMode < BPCModeLut[0].BPCMode)) {
                        AmbaPrint("[FPN-IA][HW-CONV] Badpixel: page's BPCMode=%d error", BPCBadPixelPage->BPCMode);
                        continue;
                    }
                    Rval = AmpCalib_HWFormatConv(BufferHeader->BufferAddr, BPCBadPixelPage, BPCModeLut);
                    if (Rval >= 0) {
                        BPCBufferHeader->BPCMode = BPCMode;
                        BPCBufferHeader->GeometryCal = GeometryCal;
                        BPCBufferHeader->MagicDataValid = BPC_BUFFER_MAGIC_DATA_VALID;
                    }
                } else {
                    Rval = -1;
                    if (G_BPCObj.BPCDebugEn) {
                        AmbaPrint("Channel = %d BPCMode = %d bad pixel pack data is not exist for page: %d",Channel,BPCMode,BPCPage);
                    }
                }
                if (Rval == -1) {
                    if (G_BPCObj.BPCDebugEn) {
                        AmbaPrint("[BPC APP] BPC HW conv fail, unable to load BPC mode %d to buf #%d", BPCModeIndex, BufferIndex);
                    }
                    G_BPCObj.BufferStatus.Valid[BufferIndex] = BPC_UNPACK_NG;
                } else { // update buf stat
                    G_BPCObj.BufferStatus.BPCSet[BufferIndex] = BPCModeIndex;
                    G_BPCObj.BufferStatus.Valid[BufferIndex] = BPC_UNPACK_DONE;
                }
            }
        }
    }

    return Rval;
}
/* ---------- */

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_MapUpdate
 *
 *  @Description:: update bad pixel BPCMap
 *                 The function is registered to img-algo module as callback fuction
 *
 *  @Input      ::
 *          JobID: initial or update BPC Map
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: BPC_UNPACK_DONE/ NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_MapUpdate(UINT8 JobID) // Update buffer information according to current sensor mode
{
    int Rval = -1;
    UINT8 BPCPage;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 1 } };
    AMBA_SENSOR_STATUS_INFO_s SensorStatus;
    UINT32 BPCMode;
    int BufferIndex;
    AMBA_DSP_IMG_MODE_CFG_s ImgMode;
    AMBA_DSP_IMG_SBP_CORRECTION_s BPCInfo = {0};
    BadPixel_Page_s *BPCBadPixelPage;
    UINT8   FlipMirror = 0;
    UINT8 Channel;
    AMBA_DSP_IMG_WARP_CALC_INFO_s CalcWarp = {0};

    AmbaSensor_GetStatus(Chan, &SensorStatus);
    FlipMirror = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;
    BPCMode = pAmbaCalibInfoObj[0/*AMBA_VIN_CHANNEL0*/]->AmbaBadPixelModeAssignment[SensorStatus.ModeInfo.Mode.Bits.Mode];
    if (G_BPCObj.BPCDebugEn) {
        AmbaPrintColor(RED,"BPCMode = %d",BPCMode);
    }
    // Send BPC BPCMap address to MW
    for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
        BufferIndex = AmpUTCalibBPC_BPCTableMapping(Channel, BPCMode);
        if (G_BPCObj.BufferStatus.Valid[BufferIndex] == BPC_UNPACK_DONE) {
            Rval = BPC_UNPACK_DONE;
            continue;//to make sure all channel have calibration data.
        }
        Rval = BPC_UNPACK_NONE;
        if (G_BPCObj.BPCDebugEn) {
            AmbaPrint("No bad pixel data exist in Buffer %d BPCMode = %d",BufferIndex,BPCMode);
        }
    }
    if (Rval != BPC_UNPACK_DONE) {
        if (G_BPCObj.BPCDebugEn) {
            AmbaPrintColor(BLUE,"bad pixel data is not exist");
        }
        return Rval;
    }

    memset(&ImgMode, 0, sizeof(AMBA_DSP_IMG_MODE_CFG_s));
    AmbaUT_CalibGetDspMode(&ImgMode);
    if (AmbaDSP_ImgGetWarpCompensation(&ImgMode, &CalcWarp) != OK) {
        AmbaPrint("Get Warp Compensation fail!!");
    }

    for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
        BufferIndex = AmpUTCalibBPC_BPCTableMapping(Channel, BPCMode);
        BPCInfo.Enb = 1;
        BPCInfo.CalibSbpInfo.Version = CAL_BPC_VER;
        BPCPage = AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode)+(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode)*CALIB_CH_NO*FlipMirror;
        if (G_BPCObj.BPCDebugEn) {
            AmbaPrintColor(BLUE,"%s , %d BPCPage = %d FlipMirror = %d ",__func__,__LINE__,BPCPage,FlipMirror);
        }
        BPCBadPixelPage = AmpUTCalibBPC_NandGetBadpixelPage(BPCPage);
        BPCInfo.CalibSbpInfo.VinSensorGeo = BPCBadPixelPage->CalibVinSensorGeo;

        BPCInfo.CalibSbpInfo.SbpBuffer = AmpUTCalibBPC_GetBufferHeader(G_BPCObj.ActiveBuffer[BufferIndex])->BufferAddr;

        BPCInfo.CurrentVinSensorGeo = CalcWarp.VinSensorGeo;
        BPCInfo.CurrentVinSensorGeo.Width = ((BPCInfo.CurrentVinSensorGeo.Width + 31) & 0xFFFFFFE0);//to be 32 bytes alignment

        if (G_BPCObj.BPCDebugEn) {
            AmbaPrint("Enable = %d",BPCInfo.Enb);
            AmbaPrint("Version = %X",BPCInfo.CalibSbpInfo.Version);
            AmbaPrint("Buffer Address = %X",BPCInfo.CalibSbpInfo.SbpBuffer);
            AmbaPrint("StartX = %d",BPCInfo.CurrentVinSensorGeo.StartX);
            AmbaPrint("StartY = %d",BPCInfo.CurrentVinSensorGeo.StartY);
            AmbaPrint("Width = %d",BPCInfo.CurrentVinSensorGeo.Width);
            AmbaPrint("Height = %d",BPCInfo.CurrentVinSensorGeo.Height);
            AmbaPrint("HSubSample.FactorNum = %d",BPCInfo.CurrentVinSensorGeo.HSubSample.FactorNum);
            AmbaPrint("HSubSample.FactorDen = %d",BPCInfo.CurrentVinSensorGeo.HSubSample.FactorDen);
            AmbaPrint("VSubSample.FactorNum = %d",BPCInfo.CurrentVinSensorGeo.VSubSample.FactorNum);
            AmbaPrint("VSubSample.FactorDen = %d",BPCInfo.CurrentVinSensorGeo.VSubSample.FactorDen);
            AmbaPrint("BPC Mode = %d",BPCMode);
            AmbaPrint("sensor Mode = %d",SensorStatus.ModeInfo.Mode.Bits.Mode);
        }

        AmbaDSP_ImgSetStaticBadPixelCorrection(&ImgMode, &BPCInfo);
    }

    return Rval;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_Reload
 *
 *  @Description:: reload the bad pixel to the BPC buffer
 *
 *  @Input      :: None
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *          INT: OK
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_Reload(void) // API called by Test command to reload BPC BpcMap
{
    UINT32 BufferIndex;
    UINT8 Channel,Index;
    for (Channel = 0; Channel < CALIB_CH_NO; Channel++) {
        for (BufferIndex = 0; BufferIndex < (pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); BufferIndex++) {
            Index = AmpUTCalibBPC_BPCTableMapping(Channel, BufferIndex);
            G_BPCObj.BufferStatus.Valid[Index] = BPC_UNPACK_NONE;
        }
    }
    AmpUTCalibBPC_MapDecompress(BPC_MAP_UPDATE);
    AmpUTCalibBPC_MapUpdate(0);

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_Init
 *
 *  @Description:: initial function for BPC
 *
 *  @Input      ::
 *          CalObj: calibration object
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_Init(Cal_Obj_s *CalObj)   // This API is the uni-entry point of user-level
{
    static UINT8 BPCInitFlag = 0;
    UINT8 BPCMode,Channel,Index;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 1 } };
    int Rval;
    UINT8 Flip;

    AMBA_SENSOR_MODE_ID_u SensorMode = {
        .Bits = {
            .Mode = 0,
            .VerticalFlip = 0,
            .BypassSyncGenerator = 0,
            .DummyMasterSync = 0
        }
    };
    AMBA_SENSOR_MODE_INFO_s VinInfo;
    
    if (BPCInitFlag == 0) {
        memset(&G_BPCObj, 0, sizeof(BPC_Obj_t));
        AmpUTCalibBPC_InitBPCModeLut(BPCModeLut);//initialize the _img_fpn_mode_lut
        // Reset buffer stat
        memset(&G_BPCObj.BufferStatus, 0, sizeof(BPC_Buffer_Status_s));
        for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
            for (BPCMode=0; BPCMode<(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); BPCMode++) {
                Index = AmpUTCalibBPC_BPCTableMapping(Channel, BPCMode);
                 G_BPCObj.ActiveBuffer[Index] = 0xFF;
            }
        }
        BPCInitFlag = 1;
    }
    for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
        for (BPCMode=0; BPCMode<(pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); BPCMode++) {
            Index = AmpUTCalibBPC_BPCTableMapping(Channel, BPCMode);
            G_BPCObj.BufferStatus.Valid[Index] = BPC_UNPACK_NONE;
        }
    }


    // Setup BPC pack layer
    AmpUTCalibBPC_NandSetPackAddr(((void*)(CalObj->DramShadow)));

    // Initialize BPC buffers
    if (G_BPCObj.BPCMemAllocFlag == 0) {
        G_BPCObj.BPCMemAllocFlag = 1;

        // Set up calibration mode lookup table
        for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
            for (BPCMode=0; BPCMode< (pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); BPCMode++) {
                for (Flip = 0; Flip < 2; Flip++) {
                Index = AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode);
                SensorMode.Bits.Mode = pAmbaCalibInfoObj[Channel]->AmbaBadPixelModeTable[BPCMode];
                    SensorMode.Bits.VerticalFlip = Flip;
                Rval = AmbaSensor_GetModeInfo(Chan, SensorMode, &VinInfo);
                if (Rval == OK) {
                        G_BPCObj.EffectiveGeometry[Flip][Index].Width  = VinInfo.OutputInfo.RecordingPixels.Width;
                        G_BPCObj.EffectiveGeometry[Flip][Index].Height = VinInfo.OutputInfo.RecordingPixels.Height;
                        G_BPCObj.EffectiveGeometry[Flip][Index].StartX = VinInfo.InputInfo.PhotodiodeArray.StartX;
                        G_BPCObj.EffectiveGeometry[Flip][Index].StartY = VinInfo.InputInfo.PhotodiodeArray.StartY;
                        G_BPCObj.EffectiveGeometry[Flip][Index].HSubSample.FactorNum = VinInfo.InputInfo.HSubsample.FactorNum;
                        G_BPCObj.EffectiveGeometry[Flip][Index].HSubSample.FactorDen = VinInfo.InputInfo.HSubsample.FactorDen;
                        G_BPCObj.EffectiveGeometry[Flip][Index].VSubSample.FactorNum = VinInfo.InputInfo.VSubsample.FactorNum;
                        G_BPCObj.EffectiveGeometry[Flip][Index].VSubSample.FactorDen = VinInfo.InputInfo.VSubsample.FactorDen;

                        G_BPCObj.OBGeometry[Flip][Index].Width  = VinInfo.OutputInfo.OutputWidth;
                        G_BPCObj.OBGeometry[Flip][Index].Height = VinInfo.OutputInfo.OutputHeight;
                        G_BPCObj.OBGeometry[Flip][Index].StartX = VinInfo.InputInfo.PhotodiodeArray.StartX -\
                                                                               (VinInfo.OutputInfo.RecordingPixels.StartX * VinInfo.InputInfo.HSubsample.FactorDen \
                                                                                /VinInfo.InputInfo.HSubsample.FactorNum);
                        G_BPCObj.OBGeometry[Flip][Index].StartY = VinInfo.InputInfo.PhotodiodeArray.StartY -\
                                                                               (VinInfo.OutputInfo.RecordingPixels.StartY * VinInfo.InputInfo.VSubsample.FactorDen \
                                                                                /VinInfo.InputInfo.VSubsample.FactorNum);
                        G_BPCObj.OBGeometry[Flip][Index].HSubSample.FactorNum = VinInfo.InputInfo.HSubsample.FactorNum;
                        G_BPCObj.OBGeometry[Flip][Index].HSubSample.FactorDen = VinInfo.InputInfo.HSubsample.FactorDen;
                        G_BPCObj.OBGeometry[Flip][Index].VSubSample.FactorNum = VinInfo.InputInfo.VSubsample.FactorNum;
                        G_BPCObj.OBGeometry[Flip][Index].VSubSample.FactorDen = VinInfo.InputInfo.VSubsample.FactorDen;
                        G_BPCObj.OBOffset[Flip][Index].X = VinInfo.OutputInfo.RecordingPixels.StartX;
                        G_BPCObj.OBOffset[Flip][Index].Y = VinInfo.OutputInfo.RecordingPixels.StartY;
                        
                    BPCModeLut[Index].HSubSample.FactorNum = VinInfo.InputInfo.HSubsample.FactorNum;
                    BPCModeLut[Index].HSubSample.FactorDen = VinInfo.InputInfo.HSubsample.FactorDen;
                    BPCModeLut[Index].VSubSample.FactorNum = VinInfo.InputInfo.VSubsample.FactorNum;
                    BPCModeLut[Index].VSubSample.FactorDen = VinInfo.InputInfo.VSubsample.FactorDen;
                        G_BPCObj.GeometryValid[Flip][Index] = Rval;
                    G_BPCObj.ActiveBuffer[Index] = BPCBufferSetup[Index].ActiveBuffer;//FPNDefaultActiveBuf[BPCMode];
                    }else {
                        G_BPCObj.GeometryValid[Flip][Index] = -1;
                    }
                    
                }
            }
        }
        // Allocate buffer according to buffer table
        AmpUTCalibBPC_BufferAllocate();
    }


    // update BPC buffer
    AmpUTCalibBPC_MapDecompress(BPC_MAP_INIT);
    AmpUTCalibBPC_MapUpdate(0);

    return 0;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_PageCreate
 *
 *  @Description:: page create for bad pixel calibration
 *
 *  @Input      ::
 *          argv: value of input parameter
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_PageCreate(char *argv[],char *OutputStr)
{
    UINT8  BPCMode = atoi(argv[2]);
    UINT32 Size = atoi(argv[3]);
    UINT32 Offset = atoi(argv[4]);//the size is block offset not page offset. for example, the offset for block0 is 0, and for block1, is 4*size0
    UINT32 Type;
    UINT8  Page,Channel;
    BPC_Page_s *BPCPage;


    if (strcmp(argv[5], "BADPIXEL") == 0) {
        Type = BPC_RECORD_BADPIXEL;
    } else {
        sprintf(OutputStr, "Unsupport Page type: %s", argv[5]);
        return -1;
    }

    for (Channel = 0; Channel < CALIB_CH_NO; Channel++) {
        Page = AmpUTCalibBPC_BPCTableMapping(Channel, BPCMode);
        if (Page>0) {
            BPCPage = AmpUTCalibBPC_NandGetPage(Page-1);
            if ((Offset+(Channel*BPCPage->PageSize))<(BPCPage->PageSize+BPCPage->Offset)) {
                sprintf(OutputStr, "Error !!! Page Overlapping, Offset = %d %d",(Offset+(Channel*BPCPage->PageSize)),(BPCPage->PageSize+BPCPage->Offset));
                return BPC_CALIB_PAGE_CREATE_ERROR;
            }
        }

        BPCPage = AmpUTCalibBPC_NandGetPage(Page);
        BPCPage->PageValid = 1;
        BPCPage->PageSize = Size;
        BPCPage->Offset = Offset+(Channel*Size);
        BPCPage->Type = Type;
        if ((BPCPage->Offset+BPCPage->PageSize) > CAL_BPC_SIZE) {
            sprintf(OutputStr, "Error !!! Page Overflow, defined size = %d is smaller than create size = %d",
                    CAL_BPC_SIZE,(BPCPage->Offset+BPCPage->PageSize) );
            return BPC_CALIB_PAGE_CREATE_ERROR;
        }
        AmbaPrint("page_create,BPC Mode=%d Page=%d, Size=%d, Offset=%d, type=%d",BPCMode, Page, Size, Offset, Type);
        sprintf(OutputStr, "page_create,BPC Mode=%d Page=%d, Size=%d, Offset=%d, type=%d",(int)BPCMode, (int)Page, (int)Size, (int)Offset, (int)Type);
    }
    return BPC_CALIB_OK;

}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_CheckParameters
 *
 *  @Description:: check input parameters for bad pixel calibration
 *
 *  @Input      ::
 *          Detection: bad pixel Detection parameter
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_CheckParameters(BadPixel_Detection_s *Detection,char *OutputStr)
{
    BPC_Page_s *BPCPage = AmpUTCalibBPC_NandGetPage(Detection->BadPixelPage);

    if ( (Detection->BadPixelPage < 0)||(Detection->BadPixelPage > BPC_MAX_PAGES)) {
        sprintf(OutputStr, "ERROR !!! Bad pixel Page invalid, it should be between 0 to %d",BPC_MAX_PAGES-1);
        return BPC_CALIB_PAGE_INVALID;
    }
    if ( (Detection->BufferIndex < 0)||(Detection->BufferIndex > BPC_MAX_BUFFER)) {
        sprintf(OutputStr, "ERROR !!! Bad pixel buffer invalid, it should be between 0 to %d",BPC_MAX_BUFFER-1);
        return BPC_CALIB_BUFFER_INVALID;
    }
    if ( (Detection->Num < 0 )||(Detection->Num > 60)) {
        sprintf(OutputStr, "ERROR !!! Bad pixel number invalid, it should 0 to 60");
        return BPC_CALIB_NUM_INVALID;
    }
    if ( (Detection->Threshold < 0 ) ) {
        sprintf(OutputStr, "ERROR !!! Bad pixel threshold invalid, it should 0 to 1000");
        return BPC_CALIB_THRESHOLD_INVALID;
    }
    if ( (Detection->AgcGain < 0 )||(Detection->AgcGain> 1000)) {
        sprintf(OutputStr, "ERROR !!! Bad pixel Agc gain invalid, it should 0 to 1000");
        return BPC_CALIB_AGC_INVALID;
    }
    if ( Detection->InverseShutterTime == 0) {
        sprintf(OutputStr, "ERROR !!! Bad pixel shutter_index invalid, it can't be 0");
        return BPC_CALIB_SHUTTER_INVALID;
    }
    if ( BPCPage->PageValid == 0) {
        sprintf(OutputStr, "ERROR !!! Bad pixel Page invalid");
        return BPC_CALIB_PAGE_INVALID;
    }
    if ( BPCPage->Type != BPC_RECORD_BADPIXEL) {
        sprintf(OutputStr, "ERROR !!! Bad pixel Page type invalid");
        return BPC_CALIB_PAGE_TYPE_INVALID;
    }
    return BPC_CALIB_OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_LoadBPCMap
 *
 *  @Description:: load bad pixel BPCMap from BPC buffer
 *
 *  @Input      ::
 *          Detection: bad pixel Detection parameter
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_LoadBPCMap(BadPixel_Detection_s *Detection, char *OutputStr)
{
    BadPixel_Page_s *BadPixelPage = AmpUTCalibBPC_NandGetBadpixelPage(Detection->BadPixelPage);
    BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(Detection->BufferIndex);
    UINT32 MaskSize = 0, NumBadPixels;

    AmbaPrint( "[BPC APP] load old badpixel BPCMap");
    AmpCalib_BPCUnPackByte(
        (UINT8*)(&(BadPixelPage->CompressedMap)), // packed_mask
        BadPixelPage->PackedSize, // PackedSize
        (UINT8 *)BufferHeader->BufferAddr, // Mask (Target to decompress)
        &MaskSize, // size of unpacked size
        &NumBadPixels//BadCount
    );

    if (MaskSize != BadPixelPage->MaskSize) {
        memset(BufferHeader->BufferAddr, 0, BufferHeader->Size);
        sprintf(OutputStr,"[BPC APP] decompressed BPCMap Size %d != original BPCMap Size %d !!", MaskSize, BadPixelPage->MaskSize);
        return BPC_CALIB_DECOMPRESS_ERROR;
    }

    AmbaPrintColor(RED, "[BPC APP] unpacked %s%d badpixel Mask, buf#%d, bad_pixel_page:%d, packed Size:%d count:%d Size:%d",
                   BPCMapStr,Detection->BPCMode, Detection->BufferIndex, Detection->BadPixelPage, BadPixelPage->PackedSize, NumBadPixels, MaskSize);

    return BPC_CALIB_OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_LoadBPCRawImage
 *
 *  @Description:: load bad pixel BPCMap from BPC buffer
 *
 *  @Input      ::
 *          Index: the prcture Index
 *          Detection: bad pixel Detection parameter
 *          BadPixelDetectAlgo: parameters to detect bad pixel
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_LoadBPCRawImage(UINT8 Index, BadPixel_Detection_s *Detection, BPC_Detect_Algo_Info_s *BadPixelDetectAlgo,char *OutputStr)
{
    char BPCRaw [20] = {'c',':','\\','m','o','d','e','\0'};
    char tmp[10];
    AMBA_FS_FILE *FidRaw = NULL;
    BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(Detection->BufferIndex);
    UINT8 BufferIndex;
    int Rval;
    

    BufferIndex = AmpUTCalibBPC_BPCTableMapping(Detection->Channel, Detection->BPCMode);
    Rval = AmpUTCalibBPC_GetEffectiveWidthHeight(BufferIndex, 0, &(BadPixelDetectAlgo->RawWidth), &(BadPixelDetectAlgo->RawHeight));    
    AmpUTCalibBPC_GetOBWidthHeight(BufferIndex, 0, &(BadPixelDetectAlgo->OBWidth), &(BadPixelDetectAlgo->OBHeight));

    if (Rval == -1) {
        AmbaPrint("Sensor didn't report %s%d geometry information", BPCMapStr,Detection->BPCMode);
        return BPC_CALIB_GET_GEO_ERROR;
    }
    AmbaPrint("Effective Width = %d Height = %d",BadPixelDetectAlgo->RawWidth,BadPixelDetectAlgo->RawHeight);
    AmbaPrint("OB Width = %d Height = %d",BadPixelDetectAlgo->OBWidth,BadPixelDetectAlgo->OBHeight);
    // Filled in badpixel Detection structure
    BadPixelDetectAlgo->AlgoType =  Detection->Type;
    BadPixelDetectAlgo->UpperThreshold =    Detection->Threshold;
    BadPixelDetectAlgo->LowerThreshold =    Detection->Threshold;
    BadPixelDetectAlgo->ThresholdMode = BADPIXEL_THRESHOLD_RELATIVE_TO_AVERAGE; /* If no special reason, we use relative average */
    BadPixelDetectAlgo->BlockWidth =        Detection->BlockW;
    BadPixelDetectAlgo->BlockHeight=        Detection->BlockH;
    BadPixelDetectAlgo->RawPitch = ((BadPixelDetectAlgo->RawWidth+15)/16)*16; // Basic DMA transfer unit is 32-byte, which is 16-pixels, so pad to 16pixels boundary here
    BadPixelDetectAlgo->OBPitch = ((BadPixelDetectAlgo->OBWidth+15)/16)*16; // Basic DMA transfer unit is 32-byte, which is 16-pixels, so pad to 16pixels boundary here
    BadPixelDetectAlgo->DebugFlag      = Detection->DebugFlag;
    BadPixelDetectAlgo->OBEnable        = AmpCalib_GetOpticalBlackFlag();
    //bug, it should be get from sensor driver
    AmpUTCalibBPC_GetOBOffset(BufferIndex, 0, &(BadPixelDetectAlgo->OBOffsetX), &(BadPixelDetectAlgo->OBOffsetY));

    // load raw data from SD card
    sprintf(tmp,"%d_%d.raw",Detection->BPCMode,Index);
    strcat(BPCRaw, tmp);
    BPCRaw[0] = AmpUT_CalibGetDriverLetter();
    AmbaPrint("%s",BPCRaw);
    FidRaw = AmbaFS_fopen(BPCRaw, "r");
    if (FidRaw == NULL) {
        AmbaPrint("Bad pixel calibration NG:open raw image fail");
        return BPC_CALIB_ERROR_OPEN_RAW_ERROR;
    }
    AmbaFS_fread((void*)Detection->RawAddress, sizeof(UINT32), ((BadPixelDetectAlgo->RawWidth* BadPixelDetectAlgo->RawHeight)>>1 ), FidRaw);
    AmbaFS_fclose(FidRaw);
    AmbaPrint("[t_app] Get Size = %d, addr = 0x%X", ((BadPixelDetectAlgo->RawWidth* BadPixelDetectAlgo->RawHeight) * 2), Detection->RawAddress);

    AmbaPrint("RAW data addr:0x%8x Width %d Height(%d)",
              Detection->RawAddress,
              BadPixelDetectAlgo->RawWidth,
              BadPixelDetectAlgo->RawHeight
             );
    Detection->BufferAddress = (UINT32)BufferHeader->BufferAddr;

    return BPC_CALIB_OK;
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BPCount
 *
 *  @Description:: count the number of the bad pixel
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *
 *  @Output     ::
 *  @Return     ::
 *          INT: number of bad pixel
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BPCount(UINT32 BufferIndex)
{
    BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(BufferIndex);
    UINT32 i, BadPixelCount=0;
    UINT8 Mask;

    for (i=0; i<(BufferHeader->Size<<3); i++) {
        Mask = 1<<(i%8);
        if ( (BufferHeader->BufferAddr)[i>>3] & Mask) {
            BadPixelCount++;
        }
    }
    return BadPixelCount;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_PackPredict
 *
 *  @Description:: predict the size after compression
 *
 *  @Input      ::
 *          BufferIndex: buffer Index
 *          View: the channel(sensor) Index
 *
 *  @Output     ::
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_PackPredict(UINT32 BufferIndex, UINT8 View)
{
    // Compress BPC BPCMap to DRAM shadow
    UINT32 Width, Height, BPCMapSize;
    BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(BufferIndex);
    UINT32 NumBadPixels, PackedSize;
    int Rval;
    UINT8 Flip;
    AMBA_SENSOR_STATUS_INFO_s SensorStatus;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0, .HdrID = 0 } };

    AmbaSensor_GetStatus(Chan, &SensorStatus);//get flip information from sensor
    Flip = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;
    // Get calibration BPCMap Width & Height, Width must consider BPC BPCMap padding

    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        Rval = AmpUTCalibBPC_GetOBWidthHeight(BufferIndex, Flip, &Width, &Height);
    } else {
        Rval = AmpUTCalibBPC_GetEffectiveWidthHeight(BufferIndex, Flip, &Width, &Height);
    }    
    if (Rval == -1) {
        AmbaPrint("[BPC APP] Sensor didn't report buffer %d geometry information", BufferIndex);
        return -1;
    }

    BPCMapSize = AmpCalib_BPCWidthToPitchExt(Width)*Height>>3;

    AmbaPrintColor(BLUE,"buf_index = %d (BufferHeader->BufferAddr) = %x",BufferIndex,(BufferHeader->BufferAddr));
    AmpCalib_BPCPackByte((UINT8 *)(BufferHeader->BufferAddr),BPCMapSize, 0, &PackedSize,&NumBadPixels);

    AmbaPrint("[BPC APP] predict packed buf#%d, buffer size:%d, packed size:%d bad pixels num:%d ",
              BufferIndex, BPCMapSize, PackedSize, NumBadPixels );
    return PackedSize;

}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_BadPixelSave
 *
 *  @Description:: predict the size after compression
 *
 *  @Input      ::
 *          BPCMode: the mode id of BPC
 *          BufferIndex: buffer Index
 *          Page: the page Index
 *          View: the channel(sensor) Index
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_BadPixelSave(UINT32 BPCMode, UINT32 BufferIndex, UINT32 Page, UINT8 Channel,UINT8 Flip ,char *OutputStr)
{
    BPC_Page_s *BPCPage = AmpUTCalibBPC_NandGetPage(Page);
    BadPixel_Page_s *BadPixelPage = AmpUTCalibBPC_NandGetBadpixelPage(Page);
    UINT8 *CompressedMapAddr = AmpUTCalibBPC_NandGetBadpixelPageData(Page);
    BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(BufferIndex);

    UINT32 NumBadPixels, PackedSize;
    UINT32 BPCMapSize;
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s Geometry;
    int Rval;

    if (BPCPage->PageValid == 0) { // Check whether BPC Page is correct
        sprintf(OutputStr,"BPC Page isn't valid");
        return BPC_CALIB_SAVE_ERROR;
    }
    if (BufferHeader->MemoryAllocated == 0) {
        sprintf(OutputStr,"AmpUTCalibBPC_BadPixelSave(): BPC BPCMap buffer didn't allocated");
        return BPC_CALIB_SAVE_ERROR;
    }

    // Get calibration BpcMap Width & Height, Width must consider BPC BpcMap padding
    if (AmpCalib_GetOpticalBlackFlag() == 1) {
        Rval = AmpUTCalibBPC_GetOBGeometry(BufferIndex, Flip, &Geometry);
    } else {
        Rval = AmpUTCalibBPC_GetEffectiveGeometry(BufferIndex, Flip, &Geometry);
    }
    if (Rval == -1) {
        sprintf(OutputStr,"Sensor didn't report %s%d geometry information", BPCMapStr,(int)BPCMode);
        return BPC_CALIB_SAVE_ERROR;
    }
    BPCMapSize = (AmpCalib_BPCWidthToPitchExt(Geometry.Width)*Geometry.Height)>>3;

    // Compress BPC BpcMap to DRAM shadow
    if (AmpCalib_BPCPackByte((UINT8 *)(BufferHeader->BufferAddr),BPCMapSize, (UINT8 *)(CompressedMapAddr),&PackedSize,&NumBadPixels)!= -1) {
        BadPixelPage->MagicNumber = MAGIC_BADPIXEL_BYTE;
        BadPixelPage->Version = CAL_BPC_VER;
        BadPixelPage->RecordValid = 1;
        BadPixelPage->MaskSize = BPCMapSize;
        BadPixelPage->BPCMode = BPCMode;
        BadPixelPage->CalibVinSensorGeo = Geometry;
        BadPixelPage->PackedSize = PackedSize;
        BadPixelPage->NumBadPixels = NumBadPixels;
        BadPixelPage->Channel = Channel;
        AmbaPrint("[BPC APP] BadPixelPage->RecordValid :0x%X ",BadPixelPage->RecordValid );

        AmbaPrintColor(GREEN,"[BPC APP] packed %s%d badpixel Mask, buf#%d, Page:%d, packed size:%d count:%d size:%d",
                       BPCMapStr,BPCMode, BufferIndex, Page, PackedSize, NumBadPixels, BufferHeader->Size);

        return 0;
    } else {
        sprintf(OutputStr,"[BPC APP] BPC BPCMap compressed error");
        return BPC_CALIB_SAVE_ERROR;
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_RawCapture
 *
 *  @Description:: Raw capture for bad pixel calibration
 *
 *  @Input      ::
 *          Index: picture index
 *          Detection: bad pixel Detection parameter
 *          View: the channel(sensor) Index
 *          BPCDetectAlgo : badpixel Detection structure
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_RawCapture(UINT8 Index, BadPixel_Detection_s *Detection, UINT8 View,BPC_Detect_Algo_Info_s *BPCDetectAlgo,char *OutputStr)
{
    UINT32 RawWidth, RawHeight, RawPitch;
    AMBA_3A_OP_INFO_s  AaaOpInfo = {DISABLE, DISABLE, DISABLE, DISABLE};
    UINT8 BufferIndex;
    BPC_Buffer_s *BufferHeader = AmpUTCalibBPC_GetBufferHeader(Detection->BufferIndex);
    UINT8 flag = 0x1;
    AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s ItunerRawCapCtrl = {
        .RawBufSource = 1,
        .ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING,
        .SensorMode = {
            .Bits = {
                .Mode = 0,//AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0, //please config your desired Mode
                .VerticalFlip = 0,
            }
        },
    };

//    Flip = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;
    ItunerRawCapCtrl.SensorMode.Bits.Mode = pAmbaCalibInfoObj[Detection->Channel]->AmbaBadPixelModeTable[Detection->BPCMode];
    ItunerRawCapCtrl.SensorMode.Bits.VerticalFlip = Detection->Flip;
    AmbaPrintColor(RED," ItunerRawCapCtrl.SensorMode.Bits.Mode  = %d", ItunerRawCapCtrl.SensorMode.Bits.Mode );


    BufferIndex = AmpUTCalibBPC_BPCTableMapping(Detection->Channel, Detection->BPCMode);
    if (AmpUTCalibBPC_GetEffectiveWidthHeight(BufferIndex, Detection->Flip, &RawWidth, &RawHeight) == -1) {
        AmbaPrint("Sensor didn't report %s%d buf=%d geometry information", BPCMapStr,Detection->BPCMode,BufferIndex);
        return BPC_CALIB_GET_GEO_ERROR;
    }
    RawPitch = ((RawWidth+15)>>4)<<5;
    AmpUTCalibBPC_GetOBWidthHeight(BufferIndex, Detection->Channel, &(BPCDetectAlgo->OBWidth), &(BPCDetectAlgo->OBHeight));
    AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, Detection->Channel, (UINT32)&AaaOpInfo, 0); //

    // Filled in badpixel Detection structure
    BPCDetectAlgo->AlgoType       = Detection->Type&BPC_TYPE_BIT;//check bit 0, 0: hot pixel, 1: dark pixel
    BPCDetectAlgo->ThresholdMode  = Detection->Type&BPC_MODE_BIT;//cheeck bit 7,0 relative, 1:difference
    BPCDetectAlgo->UpperThreshold = Detection->Threshold;
    BPCDetectAlgo->LowerThreshold = Detection->Threshold;
    BPCDetectAlgo->BlockWidth     = Detection->BlockW;
    BPCDetectAlgo->BlockHeight    = Detection->BlockH;
    BPCDetectAlgo->RawPitch       = RawPitch>>1;// Basic DMA transfer unit is 32-byte, which is 16-pixels, so pad to 16pixels boundary here
    BPCDetectAlgo->OBPitch          = ((BPCDetectAlgo->OBWidth+15)>>4)<<4;
    BPCDetectAlgo->RawWidth       = RawWidth;
    BPCDetectAlgo->RawHeight      = RawHeight;
    BPCDetectAlgo->DebugFlag      = Detection->DebugFlag;
    AmpUTCalibBPC_GetOBOffset(BufferIndex, Detection->Flip, &(BPCDetectAlgo->OBOffsetX), &(BPCDetectAlgo->OBOffsetY));
    BPCDetectAlgo->OBEnable        = AmpCalib_GetOpticalBlackFlag();
    Detection->BufferAddress = (UINT32)BufferHeader->BufferAddr;
    AmbaPrintColor(BLUE,"Raw address = %X Detection->BufferAddress = %x", Detection->RawAddress,Detection->BufferAddress);

    AmbaPrint("start to do raw capture");
    ItunerRawCapCtrl.RawBuff.Raw.RawAddr = (UINT8 *)Detection->RawAddress;
    ItunerRawCapCtrl.RawBuff.Raw.RawPitch = RawPitch;
    ItunerRawCapCtrl.RawBuff.Raw.RawWidth = RawWidth;
    ItunerRawCapCtrl.RawBuff.Raw.RawHeight = RawHeight;
    if (AmpUT_ItunerRawCapture) {
        AmpUT_ItunerRawCapture(flag, ItunerRawCapCtrl);
    } else {
        AmbaPrint("Null AmpUT_ItunerRawCapture");
        K_ASSERT(0);
    }
    AmbaPrint("end of raw capture");

    return BPC_CALIB_OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_CalBPC3ASetting
 *
 *  @Description:: set 3A information to BPC calibraton
 *
 *  @Input      ::
 *          Detection: input parameters for BPC calibration
 *          CalAeInfo: AE information for vignette calibration
 *
 *  @Return     ::
 *          INT: OK:0/NG:error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_CalBPC3ASetting(BadPixel_Detection_s *Detection,AMBA_AE_INFO_s *CalAeInfo,char *OutputStr)
{
    UINT16 CurIrisIdx;//max_sht_idx
    AMBA_3A_OP_INFO_s  AaaOpInfo = {DISABLE, DISABLE, DISABLE, DISABLE};
    AE_CONTROL_s AeCtrlInfo;
    UINT8 Sec;

    if (Detection->AeTarget > 0) { //Use AE to compute exposure setting
        /*open calibration AE to tune lighting*/
        AmbaImg_Proc_Cmd(MW_IP_GET_MULTI_AE_CONTROL_CAPABILITY, Detection->Channel, (UINT32)&AeCtrlInfo, 0);
        AeCtrlInfo.DefAeTarget = Detection->AeTarget;
        AmbaImg_Proc_Cmd(MW_IP_SET_MULTI_AE_CONTROL_CAPABILITY, Detection->Channel, (UINT32)&AeCtrlInfo, 0);
        for (Sec =5; Sec>0; Sec--) {
            AmbaPrint("BPC Auto AE delay time : %d second", Sec);
            AmbaKAL_TaskSleep(1000);
        }

        AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, Detection->Channel, IP_MODE_VIDEO, (UINT32)CalAeInfo);
        if ((CalAeInfo->ShutterTime < 0.0083) && (Detection->FlickerMode == 60 )) { // the 0.0083 = 1/120 sec
            AmbaPrint("CalAeInfo. ShutterTime : %f. The shutterTime is too short, please increase the AE target or decrease strength the light source",CalAeInfo->ShutterTime);
            sprintf(OutputStr,"CalAeInfo. ShutterTime : %f. The shutterTime is too short, please increase the AE target or decrease strength the light source",CalAeInfo->ShutterTime);
            return BPC_CALIB_ERROR_SHUTTERTIME_TOO_SHORT;
        } else if ((CalAeInfo->ShutterTime < 0.01) && (Detection->FlickerMode == 50 )) { // the 0.01 = 1/100 sec
            AmbaPrint("CalAeInfo. ShutterTime : %f. The shutterTime is too short, please increase the AE target or decrease strength the light source",CalAeInfo->ShutterTime);
            sprintf(OutputStr,"CalAeInfo. ShutterTime : %f. The shutterTime is too short, please increase the AE target or decrease strength the light source",CalAeInfo->ShutterTime);
            return BPC_CALIB_ERROR_SHUTTERTIME_TOO_SHORT;
        }
    } else {
        float CurShtTime,CurAgcGain;

        //turn off 3A
        AaaOpInfo.AdjOp = 0;
        AaaOpInfo.AeOp = 0;
        AaaOpInfo.AwbOp = 0;
        AaaOpInfo.AfOp = 0;
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, Detection->Channel, (UINT32)&AaaOpInfo, 0); //
        AmbaPrint("disable 3A");
        CalAeInfo->ShutterTime = 1.0/Detection->InverseShutterTime;
        CalAeInfo->AgcGain     = Detection->AgcGain;
        CalAeInfo->IrisIndex    = Detection->ManualIrisIdx;
        CalAeInfo->Dgain         = WB_UNIT_GAIN;
        CalAeInfo->Flash          = 0;
        AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO, Detection->Channel, IP_MODE_STILL, (UINT32)CalAeInfo);

        CurShtTime = CalAeInfo->ShutterTime;
        CurAgcGain = CalAeInfo->AgcGain;
        CurIrisIdx = CalAeInfo->IrisIndex;
        AmbaPrint("BPC calibration AE - shutter index:%f agc gain:%f iris index: %d"
                  ,CurShtTime,CurAgcGain,CurIrisIdx);
    }

    return BPC_CALIB_OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_RawCap
 *
 *  @Description:: Raw capture for bad pixel calibration
 *
 *  @Input      ::
 *          SensorMode: BPC mode
 *          Flip: Flip parameter
 *
 *  @Output     :: None
 *
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_RawCap(UINT8 SensorMode, UINT8 Flip)
{
    UINT32 RawWidth, RawHeight, RawPitch;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 1 } };
    UINT8 flag = 0x1;
    AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s ItunerRawCapCtrl = {
        .RawBufSource = 1,
        .ShType = AMBA_SENSOR_ESHUTTER_TYPE_ROLLING,
        .SensorMode = {
            .Bits = {
                .Mode = 0,//AMBA_SENSOR_IMX117_TYPE_2_3_MODE_0, //please config your desired Mode
                .VerticalFlip = 0,
            }
        },
    };
    void *TempPtr,*TempBufferRel;
    int Rval;
    UINT8 *Buffer;
    char Fn[32];
    AMBA_FS_FILE *Fp = NULL;
    AMBA_SENSOR_MODE_ID_u SensorModeInfo = {
        .Bits = {
            .Mode = 0,
            .VerticalFlip = 0,
        }
    };
    AMBA_SENSOR_MODE_INFO_s VinInfo;

    ItunerRawCapCtrl.SensorMode.Bits.Mode = SensorMode;
    ItunerRawCapCtrl.SensorMode.Bits.VerticalFlip = Flip;
    AmbaPrintColor(RED," ItunerRawCapCtrl.SensorMode.Bits.Mode  = %d", ItunerRawCapCtrl.SensorMode.Bits.Mode );

    SensorModeInfo.Bits.Mode = SensorMode;
    SensorModeInfo.Bits.VerticalFlip = Flip;
    Rval = AmbaSensor_GetModeInfo(Chan, SensorModeInfo, &VinInfo);
    if (Rval == OK) {
        RawWidth = VinInfo.OutputInfo.RecordingPixels.Width;
        RawHeight = VinInfo.OutputInfo.RecordingPixels.Height;
    } else {
        AmbaPrint("Get Sensor Mode Info error");
        return -1;
    }

    RawPitch = ((RawWidth+15)>>4)<<5;
    AmbaPrintColor(RED,"RawWidth = %d RawHeight = %d RawPitch = %d",RawWidth,RawHeight, RawPitch);

    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr, &TempBufferRel, (RawPitch*RawHeight), 32);
    Buffer = (UINT8*)((UINT32)TempPtr) ;
    if (Rval != OK) {
        AmbaPrint("[AmpUTCalibBPC]allocate raw fail (%u)!", RawPitch*RawHeight);
    } else {
        AmbaPrint("[AmpUTCalibBPC]rawBuffAddr (0x%08X) (%u)!", Buffer, RawPitch*RawHeight);
    }



    AmbaPrint("start to do raw capture");

    ItunerRawCapCtrl.RawBuff.Raw.RawAddr = (UINT8 *)Buffer;
    //ItunerRawCapCtrl.RawBuff.Raw.RawPitch =(RawWidth)<<1;
    ItunerRawCapCtrl.RawBuff.Raw.RawPitch =(RawPitch);
    ItunerRawCapCtrl.RawBuff.Raw.RawWidth = RawWidth;
    ItunerRawCapCtrl.RawBuff.Raw.RawHeight = RawHeight;
    if (AmpUT_ItunerRawCapture) {
        AmpUT_ItunerRawCapture(flag, ItunerRawCapCtrl);
    } else {
        AmbaPrint("Null AmpUT_ItunerRawCapture");
        K_ASSERT(0);
    }
    AmbaPrint("end of raw capture");

    AmbaPrint("AmpUTCalibBPC_SaveRawImage");
    sprintf(Fn, "c:\\Mode_%d.raw", SensorMode);
    Fn[0] = AmpUT_CalibGetDriverLetter();
    Fp = AmbaFS_fopen(Fn, "wb");
    AmbaPrint("[AmpBPC_UT]Dump Raw 0x%X Width: %d Height: %d  ",
              Buffer, \
              RawWidth, \
              RawHeight);
    AmbaFS_fwrite((UINT8 *)Buffer,RawPitch*RawHeight, 1, Fp);
    AmbaFS_fclose(Fp);

    return BPC_CALIB_OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_Detect
 *
 *  @Description:: the bad pixel Detection function
 *
 *  @Input      ::
 *          Detection: bad pixel Detection parameter
 *          View: the channel(sensor) Index
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_Detect(BadPixel_Detection_s *Detection, UINT8 View,char *OutputStr)
{
    int BadPixelPage = Detection->BadPixelPage;
    int BufferIndex = Detection->BufferIndex;
    int BPCMode = Detection->BPCMode;
    BPC_Detect_Algo_Info_s BPCDetectAlgo;
    char Fn[64] = {0};
    char Mode[3] = {'w','b','\0'};
    AMBA_FS_FILE *Fp = NULL;
    int NumBadPixels;
    int Rval;
    int i;
    BPC_Page_s *BPCPage = AmpUTCalibBPC_NandGetPage(BadPixelPage);
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s Geometry= {0};
    AMBA_MEM_CTRL_s TempPtr;
    AMBA_MEM_CTRL_s ZeroPtr;
    UINT8 *ZeroPtrAddress;
    UINT8 MappingIndex = 0;
    UINT32 RawPitch;
    AMBA_3A_OP_INFO_s  AaaOpInfoBackup = {DISABLE, DISABLE, DISABLE, DISABLE};
    AMBA_AE_INFO_s CalibAeInfo={0};
    UINT8 Flip;
    AMBA_SENSOR_STATUS_INFO_s SensorStatus;
    AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 0, .HdrID = 0 } };

    AmbaSensor_GetStatus(Chan, &SensorStatus);//get flip information from sensor
    Flip = SensorStatus.ModeInfo.Mode.Bits.VerticalFlip;

    //backup AAA_OP_INFO
    AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, Detection->Channel, (UINT32)&AaaOpInfoBackup, 0);
    AmbaPrint("AmpUTCalibBPC_Detect");
    // Stage1: check input parameters & setup environment
    // Check input parameters
    Rval = AmpUTCalibBPC_CheckParameters(Detection,OutputStr);
    if (Rval < 0) {
        return Rval;
    }

    // Lock BPC buffer, prevent BPC buffer from reloading and other operations
    AmpUTCalibBPC_BufferLock(BufferIndex);
    AmbaPrint("[BPC APP] BPC buffer #%d locked", BufferIndex);

    // Clear BPC buffer before processing
    AmpUTCalibBPC_BufferClear(BufferIndex);
    AmbaPrint("[BPC APP] Clear BPC buf #%d", BufferIndex);

    // If "clear_old" didn't set, then decompress old BPC BPCMap before processing
    if ( Detection->ClearOld == 0) {
        Rval = AmpUTCalibBPC_LoadBPCMap(Detection,OutputStr);
        if (Rval < 0) {
            AmpUTCalibBPC_BufferUnlock(BufferIndex);
            return Rval;
        }
    }

    /* allocate raw buffer address */
    MappingIndex = AmpUTCalibBPC_BPCTableMapping(Detection->Channel, Detection->BPCMode);
    AmpUTCalibBPC_GetEffectiveWidthHeight(MappingIndex, Flip, &Geometry.Width, &Geometry.Height);

    RawPitch = ((Geometry.Width+15)>>4)<<5;
    Rval = AmpUtil_GetAlignedPool(&G_MMPL, &TempPtr.pMemAlignedBase, &TempPtr.pMemBase, (RawPitch*Geometry.Height), 32);
    AmbaPrint("address of raw buffer = %x",(UINT32)(TempPtr.pMemAlignedBase));
    AmbaPrintColor(RED,"buffer size = %d",(RawPitch*Geometry.Height));
    if (Rval != OK) {
        AmbaPrint("[AmpUTCalibBPC]allocate raw fail (%u)!", RawPitch*Geometry.Height);
    } else {
        Detection->RawAddress = (UINT32)(TempPtr.pMemAlignedBase);
        AmbaPrint("[AmpUTCalibBPC]rawBuffAddr (0x%08X) (%u)!", Detection->RawAddress, RawPitch*Geometry.Height);
    }

    // 2nd stage: RAW-capture code, user can hook other BPC Detection within the function below
    Rval = AmpUTCalibBPC_CalBPC3ASetting(Detection,&CalibAeInfo,OutputStr);
    if (Rval < 0) {
        AmbaKAL_BytePoolFree(TempPtr.pMemBase);
        AmpUTCalibBPC_BufferUnlock(BufferIndex);
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, Detection->Channel, (UINT32)&AaaOpInfoBackup, 0);
        return Rval;
    }

    for (i=0; i< Detection->Num; i++) {
        AmbaPrintColor(RED, "picture #%d", i+1);
        if (Detection->LoadRawImage == BPC_RAW_FROM_SD_CARD) {
            Rval = AmpUTCalibBPC_LoadBPCRawImage(i,Detection,&BPCDetectAlgo,OutputStr);
        } else {
            Rval = AmpUTCalibBPC_RawCapture(i, Detection, View, &BPCDetectAlgo,OutputStr);
        }
        if (Rval < 0) {

            AmbaKAL_BytePoolFree(TempPtr.pMemBase);
            AmpUTCalibBPC_BufferUnlock(BufferIndex);
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, Detection->Channel, (UINT32)&AaaOpInfoBackup, 0);
            return Rval;
        }

        // Run badpixel Detection algorithm...
        AmbaPrintColor(BLUE,"NumBadPixels = %d", AmpCalib_BPCDetectFromRaw((UINT8 *)Detection->BufferAddress,(UINT16 *)Detection->RawAddress, &BPCDetectAlgo));
        if (Detection->DebugFlag) {
            AmbaPrint("AmpUTCalibBPC_SaveRawImage");
            sprintf(Fn, "c:\\%s_%d_%d.raw", Detection->DebugFilePrefix,Detection->Channel,i);
            Fn[0] = AmpUT_CalibGetDriverLetter();
            Fp = AmbaFS_fopen(Fn,(char const *) Mode);
            AmbaPrint("[AmpBPC_UT]Dump Raw 0x%X Width: %d Height: %d  ",
                      Detection->RawAddress, \
                      BPCDetectAlgo.RawWidth, \
                      BPCDetectAlgo.RawHeight);
            AmbaFS_fwrite((UINT8 *)Detection->RawAddress, \
                          (BPCDetectAlgo.RawPitch*BPCDetectAlgo.RawHeight)<<1, 1, Fp);
            AmbaFS_fclose(Fp);

                        
            if (AmpCalib_GetOpticalBlackFlag() == 1) {
                int j;
                int DummyX;
                int Pitch;
                
                DummyX = BPCDetectAlgo.OBWidth - BPCDetectAlgo.RawWidth - BPCDetectAlgo.OBOffsetX;
                Pitch = ((BPCDetectAlgo.RawWidth+15)>>4)<<4;
                Rval = AmpUtil_GetAlignedPool(&G_MMPL, &ZeroPtr.pMemAlignedBase, &ZeroPtr.pMemBase, (BPCDetectAlgo.OBWidth<<1), 32);
                AmbaPrintColor(RED,"Zero buffer size = %d",(BPCDetectAlgo.OBWidth<<1));
                if (Rval != OK) {
                    AmbaPrint("[AmpUTCalibBPC]allocate raw fail (%u)!", (BPCDetectAlgo.OBWidth<<1));
                    return -20;
                } else {
                    ZeroPtrAddress = (UINT8 *)(ZeroPtr.pMemAlignedBase);
                    memset(ZeroPtrAddress,0,(BPCDetectAlgo.OBWidth<<1));
                    AmbaPrint("[AmpUTCalibBPC]rawBuffAddr (0x%08X) (%u)!", ZeroPtrAddress, (BPCDetectAlgo.OBWidth<<1));
                }
                
                AmbaPrint("AmpUTCalibBPC_SaveOBImage");
                sprintf(Fn, "c:\\%s_%d_%d_OB.raw", Detection->DebugFilePrefix,Detection->Channel,i);
                Fn[0] = AmpUT_CalibGetDriverLetter();
                Fp = AmbaFS_fopen(Fn,(char const *) Mode);
                AmbaPrint("[AmpBPC_UT]Dump OB 0x%X Width: %d Height: %d  ",
                          Detection->RawAddress, \
                          BPCDetectAlgo.OBWidth, \
                          BPCDetectAlgo.OBHeight);
                for (j = 0; j < BPCDetectAlgo.OBHeight; j++) {
                    if (j < BPCDetectAlgo.OBOffsetY) {
                        AmbaFS_fwrite(ZeroPtrAddress, (BPCDetectAlgo.OBWidth<<1), 1, Fp);
                    } else if (j < (BPCDetectAlgo.OBOffsetY+BPCDetectAlgo.RawHeight)) {
                        AmbaFS_fwrite(ZeroPtrAddress, (BPCDetectAlgo.OBOffsetX<<1), 1, Fp);
                        AmbaFS_fwrite((UINT8*)(Detection->RawAddress+(j-BPCDetectAlgo.OBOffsetY)*(Pitch<<1)), (BPCDetectAlgo.RawWidth<<1), 1, Fp);
                        AmbaFS_fwrite(ZeroPtrAddress, (DummyX<<1), 1, Fp);
                    } else {
                        AmbaFS_fwrite(ZeroPtrAddress, (BPCDetectAlgo.OBWidth<<1), 1, Fp);
                    }
                }
                AmbaFS_fclose(Fp);                
				AmbaKAL_BytePoolFree(ZeroPtr.pMemBase);
            }
        }
    }
    NumBadPixels = AmpUTCalibBPC_BPCount(Detection->BufferIndex);
    AmbaPrintColor(RED,"NumBadPixels = %d",NumBadPixels);
    // 3rd stage: Compress BPC BPCMap & recover environment
    AmbaKAL_BytePoolFree(TempPtr.pMemBase);
    if (AmpUTCalibBPC_PackPredict(BufferIndex,View) < (BPCPage->PageSize- sizeof(BadPixel_Page_s) )) {
        if (AmpUTCalibBPC_BadPixelSave(BPCMode, BufferIndex, BadPixelPage,Detection->Channel,Detection->Flip ,OutputStr) < 0) { // Compressing bpc bitmap to DRAM
            AmpUTCalibBPC_BufferUnlock(BufferIndex);
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, Detection->Channel, (UINT32)&AaaOpInfoBackup, 0);
            return BPC_CALIB_SAVE_ERROR;
        }
    } else {
        sprintf(OutputStr,"ERROR : fpn bad pixel pack predict error, plrase adjust the threshold");
        AmpUTCalibBPC_BufferUnlock(BufferIndex);
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, Detection->Channel, (UINT32)&AaaOpInfoBackup, 0);
        return BPC_CALIB_PREDICT_ERROR;
    }
    AmpUTCalibBPC_BufferUnlock(BufferIndex);

    AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, Detection->Channel, (UINT32)&AaaOpInfoBackup, 0);
    return NumBadPixels;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_Error
 *
 *  @Description:: print the error message for bad pixel calibraion
 *
 *  @Input      ::
 *          CalSite: the calibration calibration site status
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_Error(char *OutputStr, Cal_Stie_Status_s *CalSite)
{
    AmbaPrint("************************************************************");
    AmbaPrint("%s",OutputStr);
    AmbaPrint("************************************************************");
    CalSite->Status = CAL_SITE_RESET;
    return 0;
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_NandGetBadPixelCount
 *
 *  @Description:: get the bad pixel number from Nand
 *
 *  @Input      ::
 *          Page: the page Index
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: number of bad pixel
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_NandGetBadPixelCount(UINT32 Page)
{
    BPC_Page_s *BPCPage = AmpUTCalibBPC_NandGetPage(Page);
    BadPixel_Page_s *BadPixelPage = AmpUTCalibBPC_NandGetBadpixelPage(Page);

    if (BPCPage->PageValid == 0) {
        return 0;
    }
    if (BadPixelPage->RecordValid == 0) {
        return 0;
    }
    return BadPixelPage->NumBadPixels;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_LoadScript
 *
 *  @Description:: load parameters for bad pixel calibration
 *
 *  @Input      ::
 *          argv: value of input parameter
 *          Detection: parameters for bad pixel Detection
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_LoadScript(char *argv[],char *OutputStr,BadPixel_Detection_s *Detection)
{
    AmbaPrint("AmpUTCalibBPC_LoadScript");
    Detection->Channel          = atoi(argv[2]);
    Detection->Flip             = atoi(argv[3]);
    Detection->BPCMode          = atoi(argv[4]);
    Detection->BadPixelPage     = AmpUTCalibBPC_BPCTableMapping(Detection->Channel,Detection->BPCMode)+(pAmbaCalibInfoObj[Detection->Channel]->AmbaNumBadPixelMode)*CALIB_CH_NO*(Detection->Flip);
    Detection->BufferIndex      = AmpUTCalibBPC_BPCTableMapping(Detection->Channel,Detection->BPCMode);
    Detection->Type             = atoi(argv[5]);
    Detection->Num              = atoi(argv[6]);
    Detection->Threshold          = atof(argv[7]);
    Detection->AeTarget         = (INT16) atoi(argv[8]);
    Detection->FlickerMode = (UINT8) atoi(argv[9]);
    Detection->AgcGain          = atof(argv[10]);
    Detection->InverseShutterTime      = atof(argv[11]);
    Detection->ManualIrisIdx  = (UINT16) atoi(argv[12]);
    Detection->BlockW           = atoi(argv[13]);
    Detection->BlockH           = atoi(argv[14]);
    Detection->ClearOld         = atoi(argv[15]);
    Detection->DebugFlag        = atoi(argv[16]);
    Detection->MaxBadPixel      = atoi(argv[17]);
    Detection->DebugFilePrefix  = argv[18];

    AmbaPrint("[BPC_CAL PARAM] Channel: %d", Detection->Channel);
    AmbaPrint("[BPC_CAL PARAM] Flip: %d", Detection->Flip);
    AmbaPrint("[BPC_CAL PARAM] BadPixelPage: %d", Detection->BadPixelPage);
    AmbaPrint("[BPC_CAL PARAM] BufferIndex: %d", Detection->BufferIndex);
    AmbaPrint("[BPC_CAL PARAM] BPCMode: %d", Detection->BPCMode);
    AmbaPrint("[BPC_CAL PARAM] Type: %d", Detection->Type);
    AmbaPrint("[BPC_CAL PARAM] Num: %d", Detection->Num);
    AmbaPrint("[BPC_CAL PARAM] Threshold: %f", Detection->Threshold);
    AmbaPrint("[BPC_CAL PARAM] AgcGain: %f", Detection->AgcGain);
    AmbaPrint("[BPC_CAL PARAM] InverseShutterTime: %f", Detection->InverseShutterTime);
    AmbaPrint("[BPC_CAL PARAM] AeTarget: %d", Detection->AeTarget);
    AmbaPrint("[BPC_CAL PARAM] FlickerMode: %d", Detection->FlickerMode);
    AmbaPrint("[BPC_CAL PARAM] ManualIrisIdx: %d", Detection->ManualIrisIdx);
    AmbaPrint("[BPC_CAL PARAM] BlockW: %d", Detection->BlockW);
    AmbaPrint("[BPC_CAL PARAM] BlockH: %d", Detection->BlockH);
    AmbaPrint("[BPC_CAL PARAM] ClearOld: %d", Detection->ClearOld);
    AmbaPrint("[BPC_CAL PARAM] DebugFlag: %d", Detection->DebugFlag);
    AmbaPrint("[BPC_CAL PARAM] MaxBadPixel: %d", Detection->MaxBadPixel);
    AmbaPrint("[BPC_CAL PARAM] DebugFilePrefix: %s", Detection->DebugFilePrefix);

    // Avoid dark badpixel and threshold>200
    if (Detection->Type==BPC_TYPE_DARK_PIXEL && Detection->Threshold>200) {
        sprintf(OutputStr, "ERROR !!! Dark Badpixel Calibration Threshold Need to <200");
        return BPC_CALIB_LOAD_SCRIPT_ERROR;
    }
    return BPC_CALIB_OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_GetBit
 *
 *  @Description:: get bit value for specific pixel
 *
 *  @Input      ::
 *          BPCMap: address of Bad pixel map
 *          MapW: map width
 *          MapH: map height
 *          X: x value in horizontal direction
 *          Y: y value in vertical direction
 *
 *  @Output     ::
 *
 *  @Return     ::
 *          INT: bit value
\*-----------------------------------------------------------------------------------------------*/
static int AmpUTCalibBPC_GetBit(UINT8* BPCMap, UINT32 MapW, UINT32 MapH, int X, int Y)
{
    UINT8 Mask;
    UINT32 BitOffset, ByteOffset;

    if ( (X<0) || (Y<0) || (X>=MapW) || (Y>=MapH) ) {
        return -1;
    }

    BitOffset = Y*MapW + X;
    ByteOffset = BitOffset / 8;
    Mask = 1 << (BitOffset % 8);

    return (*(BPCMap + ByteOffset)) & Mask;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_Cluster_Detect
 *
 *  @Description:: the entry function for bad pixel calibration
 *
 *  @Input      ::
 *          argc: the number of input parameter
 *          argv: value of input parameter
 *          CalSite: calibration site status
 *          CalObj: calibration site status
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_Cluster_Detect(BPC_Cluster_Detection_s *Detection)
{
    int BadPixelPage = Detection->BadPixelPage;
    int BufferIndex = Detection->BufferIndex;
    int FirstPixelGreen = Detection->FirstPixelGreen;
    int ReloadFromNand = Detection->ReloadFromNand;
    int DebugThd = Detection->DebugThd;
    BadPixel_Page_s *BPCPage;
    BPC_Buffer_s *BufferHeader;
    UINT32 MaskSize = 0, NumBadPixels = 0;
    int NumCluster;
    int Width;
    int Height;
    UINT8 *BPCMap;
    int X, Y;
    int GreenCheckX[] = {-2, -1,  0,  1,  2,  1,  0, -1};
    int GreenCheckY[] = { 0, -1, -2, -1,  0,  1,  2,  1};
    int BrCheckX[] = {-2, -2,  0,  2,  2,  2,  0, -2};
    int BrCheckY[] = { 0, -2, -2, -2,  0,  2,  2,  2};
    int CheckOneDirection[] = { -2, -1, 0, 1,  2};
    int Check5Points[] = { 0, 0, 0, 0, 0};
    int Found;
    int i, Score = 0;
    int *CheckX, *CheckY;
    UINT32 IsPixelGreen;

//      for G channel
//                  G
//                 GXG
//                GX@XG
//                 GXG
//                  G
//      for R channel
//                RGRGR
//                GBGBG  
//                RG@GR
//                GBGBG  
//                RGRGR

    // Stage1: check input parameters & setup environment
    // Check whether the FPN page valid
    BPCPage = AmpUTCalibBPC_NandGetBadpixelPage(BadPixelPage);
    if ( BPCPage->MagicNumber!= MAGIC_BADPIXEL_BYTE) {
        return BPC_CALIB_ERROR_CLUSTER_CHECK;
    }
    if (BPCPage->RecordValid == 0) {
        return BPC_CALIB_ERROR_CLUSTER_CHECK;
    }
    // Lock FPN buffer, prevent FPN buffer from reloading and other operations
    AmpUTCalibBPC_BufferLock(BufferIndex);
    BufferHeader = AmpUTCalibBPC_GetBufferHeader(BufferIndex);

    if (ReloadFromNand) { // Decompress FPN BPCMap
        // Clear FPN buffer before processing
        AmpUTCalibBPC_BufferClear(BufferIndex);
        AmbaPrintColor(GREEN, "[FPN APP] Clear FPN buf #%d", BufferIndex);

        // Decompress FPN BPCMap with original format
        AmbaPrintColor(GREEN, "[FPN APP] load old badpixel BPCMap");
        AmpCalib_BPCUnPackByte(
            (UINT8*)(&(BPCPage->CompressedMap)), // packed_mask
            BPCPage->PackedSize, // PackedSize
            (UINT8 *)BufferHeader->BufferAddr, // Mask (Target to decompress)
            &MaskSize, // size of unpacked size
            &NumBadPixels//BadCount
        );

        if (MaskSize != BPCPage->MaskSize) {
            memset(BufferHeader->BufferAddr, 0, BufferHeader->Size);
            AmbaPrint("[FPN APP] decompressed BPCMap size %d != original BPCMap size %d !!", MaskSize, BPCPage->MaskSize);
            AmpUTCalibBPC_BufferUnlock(BufferIndex);
            return BPC_CALIB_DECOMPRESS_ERROR;
        }

        AmbaPrintColor(GREEN, "[FPN APP] unpacked badpixel Mask, buf#%d, bad_pixel_page:%d, packed size:%d count:%d size:%d",
                       BufferIndex, BadPixelPage, BPCPage->PackedSize, NumBadPixels, MaskSize);
    }

    // 2nd stage: Perform cluster checking

    Width = BPCPage->CalibVinSensorGeo.Width;
    Height = BPCPage->CalibVinSensorGeo.Height;
    BPCMap = BufferHeader->BufferAddr;
    NumCluster = 0;

    for (Y=0; Y<Height; Y++) {
        for (X=0; X<Width; X++) {
            if (AmpUTCalibBPC_GetBit(BPCMap, Width, Height, X, Y)) {

                IsPixelGreen = (FirstPixelGreen + X + Y) % 2;
                Score = 0;
                if (IsPixelGreen) {
                    CheckX = GreenCheckX;
                    CheckY = GreenCheckY;
                } else {
                    CheckX = BrCheckX;
                    CheckY = BrCheckY;
                }
                Found = 0;
                for (i=0; i<sizeof(GreenCheckX)/sizeof(int); i++) {
                    if (AmpUTCalibBPC_GetBit(BPCMap, Width, Height, X+CheckX[i], Y+CheckY[i]) != 0) {
                        Score++;
                    }
                }
                if (Score >= DebugThd) { // All neighbor pixels are bad
                    Found = 1;
                }
                Score = 0;
                if(Found == 0) {//check x direction R g R g R / G r G r G / G b G b G / B g B g B
                    CheckX = CheckOneDirection;
                    CheckY = Check5Points;
                    for(i=0; i<sizeof(CheckOneDirection)/sizeof(int); i++) {
                        if(AmpUTCalibBPC_GetBit(BPCMap, Width, Height, X+CheckX[i], Y+CheckY[i]) != 0) {
                            Score++;
                        }
                }
                    if(Score >= 3) {
                        Found = 1;
                    }
                }
                Score = 0;
                if(Found == 0) {//check x direction R g R g R / G r G r G / G b G b G / B g B g B
                    CheckX = Check5Points;
                    CheckY = CheckOneDirection;
                    for(i=0; i<sizeof(CheckOneDirection)/sizeof(int); i++) {
                        if(AmpUTCalibBPC_GetBit(BPCMap, Width, Height, X+CheckX[i], Y+CheckY[i]) != 0) {
                            Score++;
                        }
                    }
                    if(Score >= 3) {
                        Found = 1;
                    }
                }
                if(Found == 1) {// All neighbor pixels are bad
                    NumCluster++;
                    Found = 0;
                }
            }
        }
    }
    // 3rd stage: Recover environment
    AmpUTCalibBPC_BufferUnlock(BufferIndex);
    return NumCluster;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_Func
 *
 *  @Description:: the entry function for bad pixel calibration
 *
 *  @Input      ::
 *          argc: the number of input parameter
 *          argv: value of input parameter
 *          CalSite: calibration site status
 *          CalObj: calibration site status
 *
 *  @Output     ::
 *          OutputStr: output string to store error message
 *  @Return     ::
 *          INT: OK: 0/NG: error code
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_Func(int argc, char *argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj)
{
    char tmp_str[64] = {0};
    int Rval = -1;
    UINT8 i;
    int Mode = 0;
    // Parse command token
    if (strcmp(argv[1], "page_create") == 0) {
        Rval = AmpUTCalibBPC_PageCreate(argv,OutputStr);
        if (Rval < 0) {
            AmpUTCalibBPC_Error(OutputStr,CalSite);
            AmbaPrint("%s",OutputStr);
            return Rval;
        }
    } else if (strcmp(argv[1], "page_reset") == 0) {
        UINT32 Page = atoi(argv[2]);
        sprintf(OutputStr, "page_reset, Page=%d", (int)Page);
    } else if (strcmp(argv[1], "status_reset") == 0) {
        memset(CalSite, 0, sizeof(Cal_Stie_Status_s));
        sprintf(OutputStr, "Reset BPC calibration site status");

    } else if ((strcmp(argv[1], "badpixel") == 0)||(strcmp(argv[1], "badpixel_flip") == 0)||(strcmp(argv[1], "badpixel_from_raw") == 0)) {
        BadPixel_Detection_s Detection;
        int NumBadPixel = 0;
        AmbaPrint("bad pixel calibration");
        if (argc != 19) {
            sprintf(OutputStr, "ERROR !!! Argument number error");
            return BPC_CALIB_LOAD_SCRIPT_ERROR;
        }

        memset(&Detection, 0, sizeof(BadPixel_Detection_s));

        Rval = AmpUTCalibBPC_LoadScript(argv,OutputStr,&Detection);
        if (Rval < 0) {
            AmpUTCalibBPC_Error(OutputStr,CalSite);
            AmbaPrint("%s",OutputStr);
            return Rval;
        }
        if (strcmp(argv[1], "badpixel_from_raw") == 0) {
            Detection.LoadRawImage = BPC_RAW_FROM_SD_CARD;
        } else {
            Detection.LoadRawImage = BPC_RAW_FROM_RAW_CAPTURE;
        }
            NumBadPixel = AmpUTCalibBPC_Detect(&Detection, Mode,OutputStr);

        if (NumBadPixel < 0) { // something wrong
            AmpUTCalibBPC_Error(OutputStr,CalSite);
            return NumBadPixel;
        } else if (NumBadPixel > Detection.MaxBadPixel) {
            sprintf(OutputStr, "ERROR !!! NumBadPixel=%d exceeds threshold=%d", NumBadPixel, Detection.MaxBadPixel);
            AmpUTCalibBPC_Error(OutputStr,CalSite);
            return BPC_CALIB_EXCEED_THRESHOLD_ERROR;
        } else {
            sprintf(OutputStr, "%s%d badpixel, NumBadPixel=%d", BPCMapStr,Detection.BPCMode, NumBadPixel);
            CalSite->Status = CAL_SITE_DONE;
            CalSite->Version = CAL_BPC_VER;
            CalSite->SubSiteStatus[Detection.Type] = CAL_SITE_DONE;
        }
    } else if (strcmp(argv[1], "badpixel_cluster_chk") == 0) {
        int NumCluster;
        BPC_Cluster_Detection_s Detection;

        Detection.Channel          = atoi(argv[2]);
        Detection.Flip             = atoi(argv[3]);
        Detection.BPCMode          = atoi(argv[4]);
        Detection.BadPixelPage     = (AmpUTCalibBPC_BPCTableMapping(Detection.Channel,Detection.BPCMode)*(Detection.Flip+1));
        Detection.BufferIndex      = AmpUTCalibBPC_BPCTableMapping(Detection.Channel,Detection.BPCMode);
        Detection.FirstPixelGreen   = atoi(argv[5]);
        Detection.ReloadFromNand    = atoi(argv[6]);
        Detection.NgThreshold      = atoi(argv[7]);
        Detection.DebugFlag         = atoi(argv[8]);
        Detection.DebugThd          = atoi(argv[9]);

        NumCluster = AmpUTCalibBPC_Cluster_Detect(&Detection);

        if (NumCluster > Detection.NgThreshold) {
            sprintf(OutputStr, "un-correctable %d badpixel exceed threshold %d", NumCluster, Detection.NgThreshold);
            return -1;
        } else {
            sprintf(OutputStr, "NO un-correctable badpixel cluster found");
        }

    } else if (strcmp(argv[1], "status") == 0) {
        UINT8 Index,Channel;
        sprintf(OutputStr,"BPC correction result:");
        for (Channel=0; Channel<CALIB_CH_NO; Channel++) {
            for (i = 0; i < (pAmbaCalibInfoObj[Channel]->AmbaNumBadPixelMode); i++) {
                Index = AmpUTCalibBPC_BPCTableMapping(Channel,i);
                sprintf(tmp_str, " %s%d:%d  ",BPCMapStr,Index,AmpUTCalibBPC_NandGetBadPixelCount(Index));
                strcat(OutputStr,tmp_str);
            }
        }
    } else if (strcmp(argv[1], "reload") == 0) {
        AmpUTCalibBPC_Reload();
        sprintf(OutputStr, "Reload BPC BPCMap");
    }
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_SetManualExposure
 *
 *  @Description:: the unit test function for bad pixel calibration
 *
 *  @Input      ::
 *          Shutter: shutter time
 *          Agc: analog sensor gain
 *          Dgain: digital gain
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0/NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_SetManualExposure(float Shutter, float Agc,UINT32 Dgain)
{
    AMBA_3A_OP_INFO_s  AaaOpInfo = {DISABLE, DISABLE, DISABLE, DISABLE};
    UINT8 ChNo = 0;

    //turn off 3A
    AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0); //

    AmpUT_CalibSetExposureValue(Shutter,Agc,Dgain);

    return 0;
}


/*-----------------------------------------------------------------------------------------------*\
*  @RoutineName:: AmpUTCalibBPC_Enable
*
*  @Description:: bpc on/off switch
*
*  @Input      ::
*          Enable: 0: disable  1: enable
*          DarkBrightFlag: 0: dark bpc mode  1:bright bpc mode
*
*  @Output     ::
*          None
*  @Return     ::
*          INT: OK: 0/NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_Enable(UINT8 Enable)
{
    AMBA_3A_OP_INFO_s AaaOpInfoTmp = {0};
    AMBA_DSP_IMG_MODE_CFG_s Mode;
    AMBA_DSP_IMG_SBP_CORRECTION_s SbpCorr = {0};
    AMBA_DSP_IMG_DBP_CORRECTION_s DpcInfo = {0};

    if (Enable == 0 || Enable == 1) {
        AmbaImg_Proc_Cmd(MW_IP_GET_AAA_OP_INFO, 0, (UINT32)&AaaOpInfoTmp, 0);
        AaaOpInfoTmp.AeOp = AaaOpInfoTmp.AwbOp = AaaOpInfoTmp.AfOp = AaaOpInfoTmp.AdjOp = 0;
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, 0, (UINT32)&AaaOpInfoTmp, 0);

        AmbaPrint("turn off dpc");
        memset(&Mode, 0, sizeof(Mode));
        Mode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;        
        Mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;        
        Mode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_FV;
        Mode.BatchId = AMBA_DSP_VIDEO_FILTER;
        Mode.ContextId = 0;
        Mode.ConfigId  = 0;

        AmbaDSP_ImgGetDynamicBadPixelCorrection(&Mode, &DpcInfo);
        AmbaDSP_ImgGetStaticBadPixelCorrection(&Mode, &SbpCorr);
        DpcInfo.Enb = 0;
        SbpCorr.Enb = Enable;
        
        AmbaDSP_ImgSetDynamicBadPixelCorrection(&Mode, &DpcInfo);
        AmbaDSP_ImgSetStaticBadPixelCorrection(&Mode, &SbpCorr);
//        Mode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
//        AmbaDSP_ImgSetDynamicBadPixelCorrection(&Mode, &DpcInfo);
//        AmbaDSP_ImgSetStaticBadPixelCorrection(&Mode, &SbpCorr);
        return 0;
    }else {
        AmbaPrint("t cal bpc enable [0/1]");
        return -1;
    }
    
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUTCalibBPC_UTFunc
 *
 *  @Description:: the unit test function for bad pixel calibration
 *
 *  @Input      ::
 *          env: environment
 *          argc: the number of the input parameter
 *          argv: value of input parameter
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0/NG: -1
\*-----------------------------------------------------------------------------------------------*/
int AmpUTCalibBPC_UTFunc(int argc, char *argv[])
{
    Cal_Obj_s           *CalObj;
    int Rval = -1;

    CalObj = AmpUT_CalibGetObj(CAL_BPC_ID);
    if ((strcmp(argv[2], "test") == 0)) {
        //register calibration site
        AmpUT_CalibSiteInit();
        Rval = 0;
        //init BPC calibration
    } else if ((strcmp(argv[2], "init") == 0)) {
        AmpUTCalibBPC_Init(CalObj);
        Rval = 0;
    } else if ((strcmp(argv[2], "reload") == 0)) {
        AmpUTCalibBPC_Reload();
        Rval = 0;
    } else if ((strcmp(argv[2], "map_update") == 0)) {
        AmpUTCalibBPC_MapUpdate(0);
        Rval = 0;
    } else if ((strcmp(argv[2], "info") == 0)) {
        AmpUTCalibBPC_Info();
        Rval = 0;
    } else if ((strcmp(argv[2], "me") == 0)) {
        float Shutter;
        float Agc;
        UINT32 Dgain;
        AMBA_3A_OP_INFO_s  AaaOpInfo = {DISABLE, DISABLE, DISABLE, DISABLE};
        UINT8 ChNo = 0;
        AMBA_AE_INFO_s AeInfo;

        Shutter = (float)atof(argv[3]);
        Agc = (float)atof(argv[4]);
        Dgain = (UINT32)atoi(argv[5]);
        AmbaPrint("shutter = %fs Agc = %fdB Dgain = %d",Shutter,Agc,Dgain);

        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0);
        AeInfo.ShutterTime= Shutter;//in second
        AeInfo.AgcGain = Agc;
        AeInfo.Dgain = 4096;
        AeInfo.Flash = 0;
        AeInfo.IrisIndex = 0;

        AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO,ChNo, IP_MODE_VIDEO, (UINT32)&AeInfo);
        AmbaImg_Proc_Cmd(MW_IP_SET_AE_INFO,ChNo, IP_MODE_STILL, (UINT32)&AeInfo);
        Rval = 0;
    } else if ((strcmp(argv[2], "3a") == 0)) {
        AMBA_3A_OP_INFO_s  AaaOpInfo = {DISABLE, DISABLE, DISABLE, DISABLE};
        UINT8 ChNo = 0;
        UINT8 Enable;

        Enable = atoi(argv[3]);
        if (Enable == 0) {
            AaaOpInfo.AdjOp = 0;
            AaaOpInfo.AeOp = 0;
            AaaOpInfo.AwbOp = 0;
            AaaOpInfo.AfOp = 0;
            AmbaPrint("disable 3A");
        } else if (Enable == 1) {
            AaaOpInfo.AdjOp = 1;
            AaaOpInfo.AeOp = 1;
            AaaOpInfo.AwbOp = 1;
            AaaOpInfo.AfOp = 1;
            AmbaPrint("enable 3A");
        }
        AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, ChNo, (UINT32)&AaaOpInfo, 0); //

        Rval = 0;
    } else if ((strcmp(argv[2], "ae_info") == 0)) {
        AMBA_AE_INFO_s            AeInfotmp;
        UINT8 ChNo = 0;
        AmbaImg_Proc_Cmd(MW_IP_GET_AE_INFO, (UINT32)ChNo, IP_MODE_VIDEO, (UINT32)&AeInfotmp);
        AmbaPrint("shutter = %f",AeInfotmp.ShutterTime);
        AmbaPrint("Agc gain = %f",AeInfotmp.AgcGain);
        AmbaPrint("Dgain = %d",AeInfotmp.Dgain);
        Rval = 0;
    }    else if ((strcmp(argv[2], "set") == 0)) {
        UINT8 BufferIndex;
        UINT8 Value;
        UINT8 Channel;
        UINT8 Index;

        Channel = atoi(argv[3]);
        BufferIndex = atoi(argv[4]);
        Value = atoi(argv[5]);
        Index = AmpUTCalibBPC_BPCTableMapping(Channel,BufferIndex);
        AmbaPrint("%s %d Index  %d",__func__,__LINE__,Index);
        AmpUTCalibBPC_SetBuffer(Index,Value);
        Rval = 0;

    } else if ((strcmp(argv[2], "set_pixel") == 0)) {
        UINT8 BufferIndex;
        UINT8 Value;
        UINT8 Channel;
        UINT8 Index;
        UINT32 X;
        UINT32 Y;

        Channel = atoi(argv[3]);
        BufferIndex = atoi(argv[4]);
        X = atoi(argv[5]);
        Y = atoi(argv[6]);
        Value = atoi(argv[7]);
        Index = AmpUTCalibBPC_BPCTableMapping(Channel,BufferIndex);

        AmpUTCalibBPC_SetPixelValue(Index,X,Y,Value);
        Rval = 0;

    } else if ((strcmp(argv[2], "set_region") == 0)) {
        UINT8 BufferIndex;
        UINT8 Value;
        UINT8 Channel;
        UINT8 Index;
        UINT32 X,Y,W,H;


        Channel = atoi(argv[3]);
        BufferIndex = atoi(argv[4]);
        X = atoi(argv[5]);
        Y = atoi(argv[6]);
        W = atoi(argv[7]);
        H = atoi(argv[8]);
        Value = atoi(argv[9]);

        Index = AmpUTCalibBPC_BPCTableMapping(Channel,BufferIndex);
        AmpUTCalibBPC_SetRegionValue(Index,X, Y, W, H,Value);
        Rval = 0;

    } else if ((strcmp(argv[2], "save") == 0)) {
        UINT32 BufferIndex;
        BPC_Buffer_s* BPCBuffer;
        AMBA_FS_FILE *Fp = NULL;
        UINT8 Channel,Index;

        Channel = atoi(argv[3]);
        BufferIndex = atoi(argv[4]);
        Index = AmpUTCalibBPC_BPCTableMapping(Channel,BufferIndex);
        BPCBuffer =  AmpUTCalibBPC_GetBufferHeader(Index);
        Fp = AmbaFS_fopen(argv[5], "w");

        if (Fp == NULL) {
            AmbaPrint("Can't create the file %s\n", argv[5]);
        } else {
            UINT32 ByteWrite = AmbaFS_fwrite(BPCBuffer->BufferAddr, 1, BPCBuffer->Size, Fp);
            AmbaPrint("Write %d byte Done", ByteWrite);
            AmbaFS_fclose(Fp);
        }
        Rval = 0;
    } else if (strcmp(argv[2], "savebmp") == 0) {
        UINT32 BPCMode;
        BPC_Buffer_s* BPCBuffer;
        AMBA_FS_FILE *Fp = NULL;
        UINT8 Channel,Index;

        Channel = atoi(argv[3]);
        BPCMode = atoi(argv[4]);
        Index = AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode);
        BPCBuffer =  AmpUTCalibBPC_GetBufferHeader(Index);
        AmbaPrintColor(RED,"size of BPCBuffer = %d ",BPCBuffer->Size);
        AmbaPrintColor(RED,"Channel = %d BPCMode = %d Index = %d",Channel,BPCMode,Index);
        Fp = AmbaFS_fopen(argv[5], "w");
        if (Fp == NULL) {
            AmbaPrint("Can't create the file %s\n", argv[5]);
        } else {
            Rval = AmpUTCalibBPC_BMPSave(Index, BPCBuffer, Fp);
            AmbaFS_fclose(Fp);
        }
    } else if (strcmp(argv[2], "rdbmp") == 0) {
        UINT32 BPCMode;
        BPC_Buffer_s* BPCBuffer;
        AMBA_FS_FILE *Fp = NULL;
        UINT8 Channel,Index;

        Channel = atoi(argv[3]);
        BPCMode = atoi(argv[4]);
        Index = AmpUTCalibBPC_BPCTableMapping(Channel,BPCMode);
        BPCBuffer =  AmpUTCalibBPC_GetBufferHeader(Index);

        Fp = AmbaFS_fopen(argv[5], "r");
        if (Fp == NULL) {
            AmbaPrint("Can't read the file %s\n", argv[5]);
        } else {
            Rval = AmpUTCalibBPC_BMPRead(Index, BPCBuffer, Fp);
            AmbaFS_fclose(Fp);
        }
    } else if (strcmp(argv[2], "sensor_cal") == 0) {
        if (argc == 5) {
            UINT8 SensorMode = atoi(argv[3]);
            UINT8 Flip = atoi(argv[4]);
            AmpUTCalibBPC_RawCap(SensorMode, Flip);
            AmbaPrint("capture FPN Mode %d raw image", SensorMode);
        } else {
            AmbaPrint("t cal sensor_cal [SensorMode] [Flip] : capture raw image for sensor mode\n");
            AmbaPrint("SensorMode: sensor mode,0/1/2/3..... ");
        }
        Rval = 0;
    } else if (strcmp(argv[2], "debug") == 0) {
        AmpUTCalibBPC_DebugEnable(atoi(argv[3]));
        Rval = 0;
    } else if (strcmp(argv[2], "cmd") == 0) {
        Rval = 0;
    } else if (strcmp(argv[2], "sensor_status") == 0) {
        AMBA_SENSOR_STATUS_INFO_s SensorStatus;
        AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 1 } };

        memset(&SensorStatus, 0, sizeof(AMBA_SENSOR_STATUS_INFO_s));
        AmbaSensor_GetStatus(Chan, &SensorStatus);
        AmbaPrint("output info startx = %d",SensorStatus.ModeInfo.OutputInfo.RecordingPixels.StartX);
        AmbaPrint("output info starty = %d",SensorStatus.ModeInfo.OutputInfo.RecordingPixels.StartY);
        AmbaPrint("output info Width = %d",SensorStatus.ModeInfo.OutputInfo.RecordingPixels.Width);
        AmbaPrint("output info Height = %d",SensorStatus.ModeInfo.OutputInfo.RecordingPixels.Height);

        AmbaPrint("input info StartX = %d",SensorStatus.ModeInfo.InputInfo.PhotodiodeArray.StartX);
        AmbaPrint("input info StartY = %d",SensorStatus.ModeInfo.InputInfo.PhotodiodeArray.StartY);
        AmbaPrint("input info Width = %d",SensorStatus.ModeInfo.InputInfo.PhotodiodeArray.Width);
        AmbaPrint("input info Height = %d",SensorStatus.ModeInfo.InputInfo.PhotodiodeArray.Height);

        AmbaPrint("Mode = %d",SensorStatus.ModeInfo.Mode.Bits.Mode);
        AmbaPrint("Data = %d",SensorStatus.ModeInfo.Mode.Data);
        AmbaPrint("Flip = %d",SensorStatus.ModeInfo.Mode.Bits.VerticalFlip);

        AmbaPrint("H FactorNum = %d",SensorStatus.ModeInfo.InputInfo.HSubsample.FactorNum);
        AmbaPrint("H FactorDen = %d",SensorStatus.ModeInfo.InputInfo.HSubsample.FactorDen);
        AmbaPrint("V FactorNum = %d",SensorStatus.ModeInfo.InputInfo.VSubsample.FactorNum);
        AmbaPrint("V FactorDen = %d",SensorStatus.ModeInfo.InputInfo.VSubsample.FactorDen);

        Rval = 0;
    } else if (strcmp(argv[2], "nand_status") == 0) {
        UINT32 CalId;
        UINT32 Offset;
        UINT32 SectorNum;


        AmbaPrint("test function for getting calibration Nand offset and sectors");
        CalId = atoi(argv[3]);
        AmpUT_CalibGetNandStatus(CalId,&Offset, &SectorNum);
        AmbaPrintColor(BLUE,"Offset = %d SectorNum = %d",Offset,SectorNum);
        Rval = 0;
    } else if (strcmp(argv[2], "sensor_flip") == 0) {
        UINT8 Channel,Mode,Flip;
        AMBA_SENSOR_STATUS_INFO_s SensorStatus;
        AMBA_DSP_CHANNEL_ID_u Chan = {.Data = 0, .Bits = { .VinID = 0, .SensorID = 1 } };

        memset(&SensorStatus, 0, sizeof(AMBA_SENSOR_STATUS_INFO_s));
        if (argc == 5) {
            Channel = atoi(argv[3]);
            Flip = atoi(argv[4]);

            AmbaSensor_GetStatus(Chan, &SensorStatus);
            Mode = SensorStatus.ModeInfo.Mode.Bits.Mode;
            AmbaPrint("Channel = %d Mode = %d Flip = %d",Channel,Mode,Flip);
            SensorFlip = Flip;
            AmpUTCalibBPC_RotateSensor(Channel, Mode, Flip);
            AmpUTCalibBPC_Reload();
            Rval = 0;
        } else {
            AmbaPrint("t cal bpc sensor_flip channel flip : BPC BPCMap rotation");
        }
    } else if (strcmp(argv[2], "ae_target") == 0) {
        AE_CONTROL_s AeCtrlInfo;
        UINT32 chNo = 0;
        UINT16 AeTarget;

        AeTarget = atoi(argv[3]);
        AmbaImg_Proc_Cmd(MW_IP_GET_MULTI_AE_CONTROL_CAPABILITY, chNo, (UINT32)&AeCtrlInfo, 0);
        AeCtrlInfo.DefAeTarget = AeTarget;
        AmbaImg_Proc_Cmd(MW_IP_SET_MULTI_AE_CONTROL_CAPABILITY, chNo, (UINT32)&AeCtrlInfo, 0);
		Rval = 0;
    } else if (strcmp(argv[2], "enable") == 0) {
        AmpUTCalibBPC_Enable(atoi(argv[3]));
        Rval = 0;
    } else if (strcmp(argv[2], "dpc") == 0) {
        AMBA_DSP_IMG_MODE_CFG_s Mode;
        AMBA_DSP_IMG_DBP_CORRECTION_s Dpc;
        AmbaPrint("turn off dpc");
        memset(&Mode, 0, sizeof(Mode));
        Dpc.Enb = 0;
        Mode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;        
        Mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;        
        Mode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_FV;
        Mode.BatchId = AMBA_DSP_VIDEO_FILTER;
        Mode.ContextId = 0;
        Mode.ConfigId  = 0;

        AmbaDSP_ImgGetDynamicBadPixelCorrection(&Mode, &Dpc);
        Dpc.Enb = atoi(argv[3]);
        
        AmbaDSP_ImgSetDynamicBadPixelCorrection(&Mode, &Dpc);
//        Mode.Pipe = AMBA_DSP_IMG_PIPE_STILL;
//        AmbaDSP_ImgSetDynamicBadPixelCorrection(&Mode, &Dpc);
        Rval = 0;

    } else if (strcmp(argv[2], "set_ob") == 0) {
        UINT8 OBFlag;
        OBFlag = atoi(argv[3]);        
        AmpCalib_SetOpticalBlackFlag(&OBFlag);
        AmbaPrint("OBFlag = %d",OBFlag);
        Rval = 0;
    } else if (strcmp(argv[2], "get_ob") == 0) {
        AmbaPrint("OBFlag = %d",AmpCalib_GetOpticalBlackFlag());
        Rval = 0;
    }


    

    if (Rval == -1) {
        AmbaPrint("t cal bpc init : re-init BPC to normal condition");
        AmbaPrint("t cal bpc reload : (the same as t cal bpc init)");
        AmbaPrint("t cal bpc info : print BPC information");
        AmbaPrint("t cal bpc debug [0/1] : toggle APP-level BPC debug information");
        AmbaPrint("t cal bpc set [channel] [BPCMode_idx] [byte_value] : Set BPC BPCMap to value");
        AmbaPrint("t cal bpc set_pixel[channel][BPCMode_idx][X][Y][byte_value] : Set BPC specific pixel with a specific value");
        AmbaPrint("t cal bpc set_region[channel][BPCMode_idx][X][Y][W][H][byte_value] : Set BPC BPCMap to value");
        AmbaPrint("t cal bpc rd [channel][BPCMode_idx][file_name] : Read BPC BPCMap from file");
        AmbaPrint("t cal bpc rdbmp[channel][BPCMode_idx][file_name] : Read BPC BPCMap from bmp file");
        AmbaPrint("t cal bpc save [channel][BPCMode_idx][file_name] : Save BPC BPCMap to file");
        AmbaPrint("t cal bpc me shutter_time AgcGain Dgain : set manual exposure");
        AmbaPrint("t cal bpc savebmp [channel][BPCMode_idx] [file_name] : Save BPC BPCMap to bmp file");
        AmbaPrint("t cal bpc predict [buf_idx]: Predict packed size of BPC BPCMap to NAND");
        AmbaPrint("t cal bpc sensor_cal [SensorMode] [Flip] : capture raw image for key in sensor mode");
        AmbaPrint("t cal bpc dump [buf_idx] [method] [filename]: Dump badpixel location(0:terminal 1:coordination 2:bit file 3:bmp file)");
        AmbaPrint("t cal bpc sensor_flip channel flip : sensor flip");
        AmbaPrint("t cal bpc enable [0/1]");
        AmbaPrint("t cal bpc dpc en HotStr DarkStr Method [0/1]");
        AmbaPrint("t cal bpc set_ob en[0/1]");
        AmbaPrint("t cal bpc get_ob");
    }

    return Rval;
}
