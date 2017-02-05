 /**
  * @file \src\unittest\MWUnitTest\AmpUT_YUVReadSD.c
  *
  * YUV Reader from SD card
  *
  * History:
  *    2015/01/21 - [Bill Chou] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include "AmpUT_YUVReadSD.h"
#include "ambava_traffic_sign_detector.h"

#define SD_FRAME_TEST_FN    "C:\\26_out.amyuv"
#define SD_FRAME_LLWS_FN    "C:\\LDWS_out.amyuv"
#define SD_FRAME_FCMD_FN    "C:\\FCMD_out.amyuv"

#define YUVREADSD_TASK_NAME "YUVReadSD_Task"
#ifndef AMP_ENC_EVENT_HACK_SDREAD
#define AMP_ENC_EVENT_HACK_SDREAD (AMP_ENC_EVENT_START_NUM+99)
#endif
extern AMBA_KAL_BYTE_POOL_t G_MMPL;
static int Is_Finished = 0;
static int Is_InputSequen = 1;
static AMBA_MEM_CTRL_s Sd_Frame_Buff = {0};
#define YUVReadSD_TaskStackSize (1<<16)
static UINT8 YUVReadSD_TaskStack[YUVReadSD_TaskStackSize];

static G_ReadSD_t G_ReadSD = { 0 };

static int VASD_FileIndex = 0;
static char Frame_FileName_List[SD_FRAME_MAX_FILENUM][64] = {0};
static char Even_FileName_List[SD_FRAME_MAX_FILENUM][64] = {0};

#define SDY_LOC AmbaKAL_MutexTake(&G_ReadSD.Mutex, AMBA_KAL_WAIT_FOREVER);
#define SDY_UNL AmbaKAL_MutexGive(&G_ReadSD.Mutex);

int Option_File_Reader(char* File_Name );

int AmbaVa_GetTheCurren_SDreadName(char** Des)
{
    int Rval = 0;
    if (Frame_FileName_List[VASD_FileIndex][0] != 0) {
        *Des = Frame_FileName_List[VASD_FileIndex];
    }
    else {
        Rval = NG;
    }
    return Rval;
}

char* AmbaVa_fget(char* s, int n, AMBA_FS_FILE *stream)
{
    char c, tmp;
    char* p = s;
    tmp = '\n';
    while (n > 1) {
        if (1 == AmbaFS_fread( &c, sizeof(char), 1, stream )) {
            *p++ = c;
            n--;

            if (c == tmp)
                break;
            continue;
        }

        /* error handling */
        if (AmbaFS_feof(stream) && (p != s))
            break;

        return NULL;
    }

    if (n > 0)
        *p = (char)0;

    return s;
}

int Str_Cmp(char *src0, char *src1)
{
    UINT32 i;
    char bSrc0[128], bSrc1[128];

    for(i=0; i<strlen(src0); i++)
    {
        bSrc0[i] = (char)toupper((int)src0[i]);
    }
    bSrc0[i] = '\0';
    for(i=0; i<strlen(src1); i++)
    {
        bSrc1[i] = (char)toupper((int)src1[i]);
    }
    bSrc1[i] = '\0';

    return strstr(bSrc0, bSrc1) != 0;
}

void Input_File_Handler(int argc, char **argv)
{
    char File_Path[64];
    int i;
    sprintf(File_Path, "c:\\%s", argv[2]);
    if(Str_Cmp(argv[2], ".amyuv")){
        SDY_LOC
        ;
        sprintf(Frame_FileName_List[0], "c:\\%s", argv[2]);
        SDY_UNL
        ;
    } else if (Str_Cmp(argv[2], ".txt")) {
               Option_File_Reader(File_Path);
    } else {
         AmbaPrint(" Please input .amyuv or .txt");
    }
}

int Option_File_Reader(char* File_Name )
{
    int Rval = 0;
    AMBA_FS_FILE* fpi;
    char buff[128];
    char buff_match[128];
    char buff_FileName[64];
    char buff_EventName[64];
    int k = 0;
    fpi =  AmbaFS_fopen(File_Name,"r");
    if (fpi == NULL) {
        AmbaPrint("%s Open fail", File_Name);
        return -1;
    }
    AmbaVa_fget(buff,127,fpi);
    strcpy(buff_match,"#va_op_file");
    if(!strncmp(buff,buff_match,strlen("#va_op_file"))){
        while((!AmbaFS_feof(fpi))){
            AmbaVa_fget(buff,127,fpi);
            if (k < (SD_FRAME_MAX_FILENUM) ) {
                sscanf(buff, "%s %s", buff_FileName, buff_EventName);
                SDY_LOC
                ;
                sprintf(Frame_FileName_List[k], "c:\\%s", buff_FileName);
                sprintf(Even_FileName_List[k], "c:\\%s", buff_EventName);
                SDY_UNL
                ;
            }
            else{
                AmbaPrint("too many files,");
            }
            k++;
        }
            AmbaFS_fclose(fpi);
            AmbaPrint("OP file read success");
        }
        else
        {
            AmbaFS_fclose(fpi);
            AmbaPrint("OP file Format Error");
        }
    return Rval;
}

int AmbaSD_FrmReader_Register(UINT32 event, SD_FRMRdr_CB func)
{
    int T;
    for (T = 0; T < SDFRM_READER_MAX_CB; T++) {
        if (G_ReadSD.CbFunc[T].Func == NULL) {
            G_ReadSD.CbFunc[T].Func = func;
            G_ReadSD.CbFunc[T].Event = event;
            break;
        }
    }
    if (T == SDFRM_READER_MAX_CB) {
        return NG;
    }
    return OK;
}

int AmbaSD_FrmReader_UnRegister(UINT32 event, SD_FRMRdr_CB func)
{
    int T;
    if (G_ReadSD.Init == 0) {
        AmbaPrint("sd frm reader is not inited !!");
        return NG;
    }
    SDY_LOC
    ;
    for (T = 0; T < SDFRM_READER_MAX_CB; T++) {
        if (G_ReadSD.CbFunc[T].Func == func && G_ReadSD.CbFunc[T].Event == event) {
            G_ReadSD.CbFunc[T].Func = NULL;
            G_ReadSD.CbFunc[T].Event = 0;
        }
    }
    SDY_UNL
    ;
    return OK;
}

int AmpUT_YUVReadSD_Play(char* File_Name)
{
    int Rval = 0;
    int i, YFrameSize, UVFrame_Size, is_fileend, frame_count, pre_frame_count;
    int Total_Frame_Cont = 0;
    AMBA_FS_FILE *fheader, *fp;
    char YuvType[12];
    UINT8* YFrame_Buffer_Head [SD_FRAME_BUFFER_NUM];
    UINT8* UVFrame_Buffer_Head [SD_FRAME_BUFFER_NUM];
    AMBAYUV_HEADER_t header[1];
    AMP_ENC_YUV_INFO_s info[1];
    int Send_CountCycle = 0;
    UINT32 CurTickCount = 0;
    UINT32 PreTickCount = 0;
    int adas_yuvsend_cycle = 0;
    /// read the header
    fheader = AmbaFS_fopen( File_Name, "rb");
    if (fheader == NULL) {
        AmbaPrint("  AmpUT_YUVReadSD_Play Open FILE %s fail", File_Name);
        return NG;
    } else {
        AmbaFS_fread(header, sizeof(AMBAYUV_HEADER_t), 1, fheader);
        AmbaFS_fclose(fheader);
        AmbaPrint("%s Open Sucess", File_Name);
        AmbaKAL_TaskSleep(500);
    }
    YFrameSize = header->Height * header->Width;
    UVFrame_Size = header->Height * header->Width / 2;
    adas_yuvsend_cycle = (int)(1000 / header->FPS);
    /// frame info
    info->height = header->Height;
    info->width = header->Width;
    info->pitch = header->Width;
    info->ySize = YFrameSize;
    if( strcpy(YuvType,"YUV420") && (!strncmp(header->YuvType,YuvType,6))){
        info->colorFmt = AMP_YUV_420;
    }
    else if (strcpy(YuvType,"YUV422") && (!strncmp(header->YuvType,YuvType,6))) {
        info->colorFmt = AMP_YUV_422;
    }

    {
        AmbaPrint("Frame  heigh     is %d ", info->height );
        AmbaPrint("Frame  width     is %d ", info->width );
        AmbaPrint("Frame  pitch     is %d ", info->pitch );
        AmbaPrint("Frame  ySize     is %d ", info->ySize );
        AmbaPrint("Frame  colorFmt  is %s ", YuvType );
    }
    /// memory for 6 frame.
    if (Sd_Frame_Buff.pMemAlignedBase != 0) {
        for (i = 0; i < SD_FRAME_BUFFER_NUM; i++ ) {
            YFrame_Buffer_Head[i] = (UINT8*)Sd_Frame_Buff.pMemAlignedBase + i * (YFrameSize + UVFrame_Size) ;
            UVFrame_Buffer_Head[i] = YFrame_Buffer_Head[i] + YFrameSize ;
        }
    }
    else {
        AmbaPrint("YUVReadSD pMemAlignedBase is not inited");
    }
    fp = AmbaFS_fopen( File_Name, "rb");
    AmbaFS_fseek(fp, sizeof (AMBAYUV_HEADER_t), SEEK_SET);
    is_fileend = 0;
    frame_count = 0;
    pre_frame_count = -1;
    i = 0;
    while (!AmbaFS_feof(fp)) {
        /// process cycle
        CurTickCount = AmbaKAL_GetTickCount();
        if ( (CurTickCount - PreTickCount) < adas_yuvsend_cycle) {
            AmbaKAL_TaskSleep(1);
            continue;
        } else {
            PreTickCount = CurTickCount;
        }
        AmbaFS_fread( YFrame_Buffer_Head[i], (YFrameSize + UVFrame_Size), 1, fp);
        info->yAddr = YFrame_Buffer_Head[i];
        info->uvAddr = UVFrame_Buffer_Head[i];
        if ((frame_count - pre_frame_count) > Send_CountCycle ) {
            extern int AmbaVA_FrmHdlr_NewFrame(UINT32 event, AMP_ENC_YUV_INFO_s *YUVInfo);
            AmbaVA_FrmHdlr_NewFrame(AMP_ENC_EVENT_HACK_SDREAD, info);
            pre_frame_count = frame_count;
        }
        i++;
        frame_count++;
        Total_Frame_Cont++;
        if (i > (SD_FRAME_BUFFER_NUM - 2)) {
            i =  0;
        }
    }
    AmbaFS_fclose(fp);
    AmbaPrintColor(RED, "%s Close Sucess", File_Name);
    AmbaKAL_TaskSleep(100);
    return Rval;
}

int AmpUT_YUVReadSD_I420InputImgTest(char* File_Name, int IsYUV)
{
    int Rval = 0;
    UINT8 *BuffHead, *UvSrcHead, *UDesHead, *VDesHead, *pUvSrcHead, *pUDesHead, *pVDesHead;
    int i, j, T;
    AMBA_FS_FILE *fp, *fheader;
    AMBA_MEM_CTRL_s Sd_Img_Buff = {0};
    AMBAVA_YUV_INFO_s yuvinfo[1];
    AMBAYUV_HEADER_t header[1];

    if (Sd_Frame_Buff.pMemAlignedBase != 0) {
            BuffHead = (UINT8*)Sd_Frame_Buff.pMemAlignedBase;
    }
    else {
        AmbaPrint(" AmpUT_YUVReadSD_ImgTest buffer not init");
        return -1;
    }

    if (!IsYUV) {
        /*
        fp = AmbaFS_fopen(File_Name, "rb");
        AmbaFS_fread(BuffHead, SD_IMG_TEST_FN_W * SD_IMG_TEST_FN_H *  sizeof(char), 1, fp);
        AmbaFS_fclose(fp);

        yuvinfo->colorFmt = AMP_YUV_420;
        yuvinfo->height = SD_IMG_TEST_FN_H;
        yuvinfo->pitch = SD_IMG_TEST_FN_W;
        yuvinfo->uvAddr = NULL;
        yuvinfo->width = SD_IMG_TEST_FN_W;
        yuvinfo->yAddr = BuffHead;
        yuvinfo->ySize = SD_IMG_TEST_FN_W * SD_IMG_TEST_FN_H;
        */
    } else {
       /// read the header
        fheader = AmbaFS_fopen( File_Name, "rb");
        if (fheader == NULL) {
            AmbaPrint("%s Open fail", File_Name);
            return NG;
        } else {
            AmbaFS_fread(header, sizeof(AMBAYUV_HEADER_t), 1, fheader);
            AmbaFS_fclose(fheader);
            AmbaPrint("%s Open Sucess", File_Name);
            AmbaKAL_TaskSleep(500);
        }

        /// buffer align
        UDesHead = BuffHead + header->Width * header->Height;
        VDesHead = UDesHead + (header->Width / 2) * (header->Height / 2);
        UvSrcHead = VDesHead + (header->Width / 2) * (header->Height / 2);

        /// read data
        fp = AmbaFS_fopen(File_Name, "rb");
        AmbaFS_fseek(fp, sizeof (AMBAYUV_HEADER_t), SEEK_SET);
        AmbaFS_fread(BuffHead, header->Width * header->Height  *  sizeof(char), 1, fp);
        AmbaFS_fread(UvSrcHead, header->Width * (header->Height / 2)  *  sizeof(char), 1, fp);
        AmbaFS_fclose(fp);

        yuvinfo->colorFmt = AMBAVA_YUV_420;
        yuvinfo->height =   header->Height;
        yuvinfo->width =    header->Width;
        yuvinfo->pitch =    header->Width;
        yuvinfo->uvAddr =   UvSrcHead;
        yuvinfo->yAddr =    BuffHead;
        yuvinfo->ySize =    header->Width * header->Height;
    }

    /// TransForm AmvaYvu to I420 for test
    /// /* non-interlace
    /*
    pUDesHead = UDesHead;
    pVDesHead = VDesHead;
    pUvSrcHead = UvSrcHead;
    for (j = 0; j < (header->Height/2); j ++) {
        for (i = 0; i < (header->Width/2); i++) {
            *pVDesHead = *pUvSrcHead;
            pVDesHead++;
            pUvSrcHead++;
            *pUDesHead = *pUvSrcHead;
            pUDesHead++;
            pUvSrcHead++;
        }
    }
    */
    SDY_LOC
    ;
    for (T = 0; T < SDFRM_READER_MAX_CB; T++) {
        if (G_ReadSD.CbFunc[T].Func != NULL ) {
            G_ReadSD.CbFunc[T].Func( AMP_ENC_EVENT_START_NUM+99 , yuvinfo ); /// I420 input
        }
    }
    SDY_UNL
    ;

    return Rval;
}

void YUVReadSD_Maintask(UINT32 info)
{
    int T;
    while (1) {
        if (Frame_FileName_List[VASD_FileIndex] != 0 && Is_Finished == 0){
            SDY_LOC
            ;
            for (VASD_FileIndex = 0; VASD_FileIndex < SD_FRAME_MAX_FILENUM; VASD_FileIndex++) {
                if (Frame_FileName_List[VASD_FileIndex][0] != 0 ) {
                    if ( Is_InputSequen) {
                        AmpUT_YUVReadSD_Play(Frame_FileName_List[VASD_FileIndex]);
                    } else {
                        AmpUT_YUVReadSD_I420InputImgTest(Frame_FileName_List[VASD_FileIndex], 1);
                    }
                }
            }
            SDY_UNL
            ;
            Is_Finished = 1;
        }
        else if (Is_Finished == 1) {
            SDY_LOC
            ;
            for (T = 0; T < SD_FRAME_MAX_FILENUM; T++) {
                memset(Frame_FileName_List[T], 0, sizeof(char) * 64);
            }
            SDY_UNL
            ;
            AmbaKAL_TaskSleep(1);
        }
        else {
            AmbaKAL_TaskSleep(1);
        }
    }
}

int AmpUT_YUVReadSD_Init( int IsInputSequen)
{
    int Rval = 0;
    Is_InputSequen = IsInputSequen;
    Rval = AmbaKAL_MemAllocate(&G_MMPL, &(Sd_Frame_Buff), SD_FRAME_BUFFER_TOTALSIZE, 32);
    if (Rval != OK) {
        AmbaPrint(" AmpUT_YUVReadSD_Init Can't allocate memory.");
        return -1;
    }else {
        AmbaPrint("AmbaKAL_MemAllocate Alignedbuf = 0x%X / size = %d  !",Sd_Frame_Buff.pMemAlignedBase, SD_FRAME_BUFFER_TOTALSIZE);
    }

    // create mutex
    Rval = AmbaKAL_MutexCreate(&G_ReadSD.Mutex);
    if (Rval == OK) {
        AmbaPrintColor(GREEN, "[frmhdlr INIT] Create Mutex success");
    } else {
        AmbaPrintColor(RED, "[frmhdlr INIT] Create Mutex fail = %d", Rval);
        return NG;
    }
    // create task
    Rval = AmbaKAL_TaskCreate(&G_ReadSD.Task,
                              YUVREADSD_TASK_NAME,
                              90,
                              YUVReadSD_Maintask,
                              0x0,
                              YUVReadSD_TaskStack,
                              YUVReadSD_TaskStackSize,
                              AMBA_KAL_AUTO_START);
    if (Rval == OK) {
        AmbaPrintColor(GREEN, "[SD Reader INIT] Create Task success");
    } else {
        AmbaPrintColor(RED, "[SD Reader INIT] Create Task fail = %d", Rval);
        return NG;
    }
    G_ReadSD.Init = 1;
    return Rval;
}

int AmpUT_YUVReadSD_DeInit(void)
{
    int Rval = 0;
    if (G_ReadSD.Init == 0) {
        AmbaPrint("AmpUT_YUVReadSD is not init \n");
        return -1;
    }
    if (Sd_Frame_Buff.pMemBase != 0) {
        AmbaKAL_BytePoolFree(Sd_Frame_Buff.pMemBase);
    }
    G_ReadSD.Init = 0;
    return Rval;
}


/////////////////////////////////////////
/// external part
////////////////////////////////////////


int AmpUT_YUVReadSD(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_VATest cmd: %s", argv[1]);
    if ( strcmp(argv[1],"ls") == 0 ) {
        AmbaPrint("ls.... not yet");
    }  else if ( strcmp(argv[1],"-i") == 0 ) {
        if ( argv[2] == NULL){
            AmbaPrint("give a file name");
        }
        else {
            Input_File_Handler(argc, argv);
            AmpUT_YUVReadSD_Init(1);
        }
    } else if ( strcmp(argv[1],"-s") == 0 ) {
        if ( argv[2] == NULL){
            AmbaPrint("give a file name");
        }
        else {
            Input_File_Handler(argc, argv);
            AmpUT_YUVReadSD_Init(0);
        }
    } else if ( strcmp(argv[1], "rall") == 0 ) {
        AmbaPrint("rall.... not yet");
    } else if ( strcmp(argv[1],"deinit") == 0 ) {
        AmpUT_YUVReadSD_DeInit();
    } else if ( strcmp(argv[1], "test") == 0 ) {
        SDY_LOC
        ;
        sprintf(Frame_FileName_List[0], "%s", SD_FRAME_TEST_FN);
        SDY_UNL
        ;
        AmpUT_YUVReadSD_Init(1);
        AmpUT_YUVReadSD_DeInit();
    } else {
        AmbaPrint("Usage: t ysd cmd ...");
        AmbaPrint("    cmd:");
        AmbaPrint("       -i [FileName]: input file name and play");
        AmbaPrint("       deinit       : deinit buffer");
    }
    return 0;
}

int AmpUT_YUVReadSDAdd(void)
{
    AmbaPrint("Adding AmpUT_YUVReadSDAdd");
    // hook command
    AmbaTest_RegisterCommand("yuvreadsd", AmpUT_YUVReadSD);
    AmbaTest_RegisterCommand("ysd", AmpUT_YUVReadSD);
    return AMP_OK;
}

