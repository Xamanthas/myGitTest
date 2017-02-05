/**
 * @file mw/unittest/extSrc/sample_calib/AmbaUTCalibCli.c
 *
 * sample code for calibration command line interface
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

#include "AmbaDataType.h"
#include "AmbaUTCalibMgr.h"
#include "AmbaUTCalibCli.h"
#include "AmbaUtility.h"
#include "vignette/AmbaUTCalibVig.h"

#define MAX_CLI_CMD_TOKEN       (128)
#define MAX_CMD_TOKEN_SIZE  (16)
#define CALIB_MODE0  0
#define CALIB_MODE1  1

extern Cal_Obj_s* AmpUT_CalibGetObj(UINT32 CalId);

#define CALIB_TASK1_PRIORITY          (75)

char CalibOutFn1[256];
int  CalibSel;

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibPrint
 *
 *  @Description:: print the calibration result
 *
 *  @Input      ::
 *          Rval: return value from the calibration result
 *          Fmt: Output string
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          None
\*-----------------------------------------------------------------------------------------------*/
static void AmpUT_CalibPrint(int Rval, const char *Fmt, ...)
{
    va_list Args;
    char Buf[512];

    va_start(Args, Fmt);
    vsprintf(Buf, Fmt, Args);
    va_end(Args);   // avoid dangling pointer

    AmbaPrint("[CALMGR CLI]%s %s",
              (Rval==0)? "[OK]": "[NG]",Buf);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibCli
 *
 *  @Description:: command line interface function for calibraion
 *
 *  @Input      ::
 *          Cmd: input command
 *          MsgAvailable: check the message is available
 *
 *  @Output     ::
 *          OutputStr: Output string for debuging
 *  @Return     ::
 *          None
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibCli(int Argc, char *Argv[], char *OutputStr, int *MsgAvailable)
{
    int Rval=-1;
    static int RvalCalFunc = 0;

    int IsProcessed = 0;
    static char CalibOutputStr[256];
    int i;
    int CalId;
    char Buf[16] = {0};
    Cal_Stie_Status_s *PCalSite, CalSite;
    UINT8 *VigAddr;
    Cal_Obj_s *CalObj;
    int SubNum = 1;


    if (Argc == 0) {
        *MsgAvailable = 0;
        return -1;
    }

    // Processing commands
    IsProcessed = 2;
    if (strcmp(Argv[0], "_calib_reset") == 0) {
        RvalCalFunc = 0;
        CalId = atoi(Argv[1]);
        if (Argc == 3 && CalId == CAL_VIGNETTE_ID) {
            if (strcmp(Argv[2], "partial") == 0) {
                sprintf(OutputStr, "Reset calib %d NAND data (partial)", CalId);
                Rval = AmpUT_CalibResetPartial(CalId);
            } else {
                sprintf(OutputStr, "Reset calib %d NAND data error!", CalId);
#if (CALIB_STORAGE == CALIB_FROM_NAND)
                Rval = AmpUT_CalibNandReset(CalId);
#else //from SD card
                Rval = AmpUT_CalibSDCardReset(CalId);
#endif
            }
        } else {
            sprintf(OutputStr, "Reset calib %d NAND data", CalId);
#if (CALIB_STORAGE == CALIB_FROM_NAND)
            Rval = AmpUT_CalibNandReset(CalId);
#else //from SD card
            Rval = AmpUT_CalibSDCardReset(CalId);
#endif
        }
    } else if (strcmp(Argv[0], "_calib_init") == 0) {
        CalId = atoi(Argv[1]);
        *MsgAvailable = 0;
        Rval = AmpUT_CalibInitFunc(CalId,CALIB_LOAD, CALIB_FULL_LOAD);
        if (Rval != OK) {
            AmbaPrint("Load calib %d fail or no calibration data", CalId);
            return Rval;
        }
         Rval |= AmpUT_CalibInitFunc(CalId,CALIB_INIT, 0);
        if (Rval != OK) {
            AmbaPrint("Init calib %d fail", CalId);
        }
        return Rval;
    } else if (strcmp(Argv[0], "_calib_adjust") == 0) {
        *MsgAvailable = 0;
        AmpUTCalibAdjust_SetControlEnable(atoi(Argv[1]));
        AmbaPrint("_calib_adjust %d",atoi(Argv[1]));
        return Rval;
    } else if (strcmp(Argv[0], "_partial_init") == 0) {
        UINT8 SubId;
        *MsgAvailable = 0;        
        CalId = atoi(Argv[1]);
        SubId = atoi(Argv[2]); 
        AmbaPrint( "Init calib %d SubId = %d", CalId, SubId);
        sprintf(OutputStr, "Re-init calib %d SubId = %d", CalId, SubId);
        Rval = AmpUT_CalibInitFunc(CalId,CALIB_LOAD, SubId);
        if (Rval != OK) {
            AmbaPrint("Load calib %d fail or no calibration data", CalId);
            return Rval;
        }
        Rval |= AmpUT_CalibInitFunc(CalId,CALIB_INIT, 0);
        if (Rval != OK) {
            AmbaPrint("Init calib %d fail", CalId);
        }
        return Rval;
    } else if (strcmp(Argv[0], "_calib_save") == 0) {
        SubNum = 1;
        CalId = atoi(Argv[1]);
        CalObj = AmpUT_CalibGetObj(CalId);
        VigAddr = CalObj->DramShadow;
        PCalSite = AmpUT_CalibGetSiteStatus(CalId);

        sprintf(OutputStr, "Save calib %d to NAND", CalId);
        for (i=0; i<SubNum; i++) {
#if (CALIB_STORAGE == CALIB_FROM_NAND)
            AmpUT_CalibNandSave(CalId,i);
#else //from sd card
            //save to SD card
            AmpUT_CalibSDCardSave(CalId,i);
#endif
        }
        Rval = 0 ;
    } else if (strcmp(Argv[0], "_nand_init") == 0) {
        CalId = atoi(Argv[1]);
#if (CALIB_STORAGE == CALIB_FROM_NAND)
        sprintf(OutputStr, "_nand_save");
        Rval = AmpUT_CalibNandInit();
#endif
    } else if (strcmp(Argv[0], "_nand_to_sd") == 0) {
        CalId = atoi(Argv[1]);
#if (CALIB_STORAGE == CALIB_FROM_NAND)//load from Nand Flash
        sprintf(OutputStr, "Load calib %d from NAND", CalId);
        Rval = AmpUT_CalibNandLoad(CalId);
        if (Rval != OK) {
            return Rval;
        }
        Rval = AmpUT_CalibSDCardSave(CalId,0);
        if (Rval != OK) {
            return Rval;
        }
#endif
    } else if (strcmp(Argv[0], "_sd_to_nand") == 0) {
        CalId = atoi(Argv[1]);
#if (CALIB_STORAGE == CALIB_FROM_NAND)//load from SD card
        Rval = AmpUT_CalibSDCardLoad(CalId);
        if (Rval != OK) {
            return Rval;
        }

        sprintf(OutputStr, "Save calib %d to NAND", CalId);
        Rval = AmpUT_CalibNandSave(CalId,0);
        if (Rval != OK) {
            return Rval;
        }
#endif
    } else if (strcmp(Argv[0], "_calib_load") == 0) {
        CalId = atoi(Argv[1]);
#if (CALIB_STORAGE == CALIB_FROM_NAND)
        sprintf(OutputStr, "Load calib %d from NAND", CalId);
        Rval = AmpUT_CalibNandLoad(CalId);
#else //from sd card
        sprintf(OutputStr, "Load calib %d from SD card", CalId);
        //Load from SD card
        Rval = AmpUT_CalibSDCardLoad(CalId);
#endif
    } else if (strcmp(Argv[0], "_project_name") == 0) {
        AmbaPrint("Calib_Project_Name: %s Script_Project_Name: %s", CALIB_PROJECT_NAME, Argv[1]);
        if ( strcmp(Argv[1] , CALIB_PROJECT_NAME) == 0) {
            sprintf(OutputStr, "Project_Name:%s", CALIB_PROJECT_NAME);
            Rval = 0;
        } else {
            sprintf(OutputStr, "Error, The Project ID is mismatched.");
            Rval = -1;
        }
    } else if (strcmp(Argv[0], "_rom_load")==0) {
        if (Argc ==2) {
            CalId = atoi(Argv[1]);
            Rval = AmpUT_CalibROMLoad(CalId);
        } else {
            CalId = atoi(Argv[1]);
            Rval = AmpUT_CalibROMLoadTable(CalId, atoi(Argv[2]),  atoi(Argv[3]));
        }
    } else if (strcmp(Argv[0], "_rom_load_head")==0) {
        CalId = atoi(Argv[1]);
        AmpUT_CalibROMLoadTableHead(CalId);
    } else if (strcmp(Argv[0], "_nand_load")==0) {
        if (Argc ==2) {
            CalId = atoi(Argv[1]);
            Rval = AmpUT_CalibNandLoad(CalId);
        } else {
            CalId = atoi(Argv[1]);
            Rval = AmpUT_CalibNandLoadTable(CalId, atoi(Argv[2]),  atoi(Argv[3]));
        }
        sprintf(OutputStr, "Load calib %d from NAND", CalId);
        
    } else if (strcmp(Argv[0], "_nand_load_head")==0) {
        CalId = atoi(Argv[1]);
        Rval = AmpUT_CalibNandLoadTableHead(CalId);
        sprintf(OutputStr, "Load calib %d head from NAND", CalId);
    } else if (strcmp(Argv[0], "_sd_save")==0) {
        CalId = atoi(Argv[1]);
        Rval = AmpUT_CalibSDCardSave(CalId, 0 );
    } else if (strcmp(Argv[0], "_simple_init")==0) {
        CalId = atoi(Argv[1]);
        Rval = AmpUT_CalibInitSimple(CalId);
    } else if (strcmp(Argv[0], "_simple_load")==0) {
        UINT8 Format = 0;    
        CalId = atoi(Argv[1]);
        if (strcmp(Argv[2], "nand") == 0) {
            Format = CALIB_FROM_NAND;
        } else if (strcmp(Argv[2], "sd") == 0 ) {
            Format = CALIB_FROM_SD_CARD;
        } else if (strcmp(Argv[2], "rom") == 0) {
            Format = CALIB_FROM_ROM;
        }
        Rval = AmpUT_CalibInitLoadSimple(CalId,  Format);
    } else if (strcmp(Argv[0], "_nand_save")==0) {
        CalId = atoi(Argv[1]);
        Rval = AmpUT_CalibNandSave(CalId, 0);
        sprintf(OutputStr, "Save calib %d to NAND", CalId);
    } else if (strcmp(Argv[0], "_raw_cap")==0) {
        Rval = AmpUT_CalibMgrRawCapture(atoi(Argv[1]), atoi(Argv[2]));
        sprintf(OutputStr, "_raw_cap done");
    } else if (strcmp(Argv[0], "_check_size")==0) {
        Rval = AmpUT_CalibCheckStructure();
        sprintf(OutputStr, "check struct size done");
    } else if (strcmp(Argv[0], "_print_status") == 0) {
        CalId = atoi(Argv[1]);
        PCalSite = AmpUT_CalibGetSiteStatus(CalId);
        // Print site Status

        AmbaPrint("CalId #%d", CalId);
        AmbaPrint("\tstatus = 0x%04X, version=0x%04X", PCalSite->Status, PCalSite->Version);
        AmbaPrint("\tsub site Status:");
        for (i=0; i<8; i++) { // 8 sub-site
            AmbaPrint("\t\toperation #%d = 0x%04X", i, PCalSite->SubSiteStatus[i]);
        }

        AmbaPrint("\treserved: %s", PCalSite->Reserved);

        sprintf(OutputStr, "site%d %s Status:%s ver:0x%08X, sub-stat:%d,%d,%d,%d,%d,%d,%d,%d",
                CalId,
                AmpUT_CalibGetObj(CalId)->Name,
                (PCalSite->Status != 0)? "OK": "NG",
                PCalSite->Version,
                PCalSite->SubSiteStatus[0]>0,
                PCalSite->SubSiteStatus[1]>0,
                PCalSite->SubSiteStatus[2]>0,
                PCalSite->SubSiteStatus[3]>0,
                PCalSite->SubSiteStatus[4]>0,
                PCalSite->SubSiteStatus[5]>0,
                PCalSite->SubSiteStatus[6]>0,
                PCalSite->SubSiteStatus[7]>0);
        if (RvalCalFunc != 0) {
            strcpy(OutputStr,CalibOutputStr);
        }
        Rval = 0;

    } else if (strcmp(Argv[0], "_ctl_init") == 0) {
        AmpUTCalibAdjust_Init();
        AmbaPrint("ctl init");
        Rval = 0;
    } else if (strcmp(Argv[0], "_ctl") == 0) {
        extern void AmpUTCalibAdjust_Func(void);
        
        AmpUTCalibAdjust_Func();
        AmbaPrint("ctl test");
        Rval = 0;
    } else {
        IsProcessed = 0;
        for (CalId=0; CalId<NVD_CALIB_MAX_OBJS; CalId++) {
            Cal_Obj_s *CalObj = AmpUT_CalibGetObj(CalId);

            if (strcmp(Argv[0], CalObj->Name) == 0) {
                if (CalObj->CalibFunc != NULL) {

                    // Get Status and have a local copy
                    PCalSite = AmpUT_CalibGetSiteStatus(CalId);
                    CalSite = *PCalSite;

                    // Call calibration API
                    RvalCalFunc = CalObj->CalibFunc(Argc, Argv, OutputStr, &CalSite, CalObj);
                    if (RvalCalFunc < 0) {
                        sprintf(Buf,". ERR = %d",RvalCalFunc);
                        strcat(OutputStr,Buf);
                    }
                    strcpy(CalibOutputStr,OutputStr);
                    Rval = RvalCalFunc;
                    IsProcessed = 1;
                    // Save Status
                    *PCalSite = CalSite;
                    //save to Nand
#if (CALIB_STORAGE == CALIB_FROM_NAND)
                    AmpUT_CalibNandSave(CAL_STATUS_ID,0);
#else
                    //save to SD card
                    AmpUT_CalibSDCardSave(CAL_STATUS_ID,0);
#endif
                    break;
                }
            }
        }
    }

    if (IsProcessed > 0) {
        if (IsProcessed == 1) {
            Rval = RvalCalFunc;
        }
        AmpUT_CalibPrint(Rval, OutputStr);
        *MsgAvailable = 1;
        return Rval;
    } else {
        *MsgAvailable = 0;
        return 0;
    }


}

/* ---------- */
// Calibration manager SD card IF

#define CALIB_SD_IF_COUNT_DOWN      7

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibGetLineToken
 *
 *  @Description:: divide script line into command tokens
 *
 *  @Input      ::
 *          Buf:     calibration script line
 *
 *  @Output     ::
 *          Argc:   command line token total number
 *          Argv:   command line token contents
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
static int AmpUT_CalibGetLineToken(char Buf[], int *Argc, char *Argv[])
{
    char *Token;
    // Parse the input string to multiple tokens
    Token = strtok(Buf, " ");
    *Argc= 0;
    while (Token != NULL) {

        Argv[*Argc] = Token;
        (*Argc)++;
        Token = strtok(NULL, " ");
    }
    return 0;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibGetLine
 *
 *  @Description:: read calibration information from the script
 *
 *  @Input      ::
 *          Fp: file pointer
 *
 *  @Output     ::
 *          Buf: read the calibration information to the buffer
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
static int AmpUT_CalibGetLine(AMBA_FS_FILE *Fp, char Buf[])
{
    unsigned char Ch;
    // Normal state
    do {
        if (AmbaFS_fread(&Ch, 1, 1, Fp) == 0) { // read 1 byte
            return -1;
        }
        if ( (Ch == '\n') || (Ch == '\r') ) {
            break;  // go to end-of-line Status
        }
        *Buf = Ch;
        Buf++;
    } while (1);

    // End of line state
    do {
        if (AmbaFS_fread(&Ch, 1, 1, Fp) == 0) { // read 1 byte
            break;
        }

        if ( (Ch == '\n') || (Ch == '\r') ) {
            /* do nothing */
        } else {
            // Reach to next line, roll back 1 byte
            AmbaFS_fseek(Fp, -1, PF_SEEK_CUR);
            break;
        }
    } while (1);

    *Buf = '\0';
    return 0;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSdIf
 *
 *  @Description:: calibration by SD card interface
 *
 *  @Input      ::
 *          "cal.txt": script file for calibration
 *
 *  @Output     ::
 *          "cal_out.txt": output for calibration result
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
static int AmpUT_CalibSdif(void)
{
    AMBA_FS_FILE *FpIn = NULL, *FpOut = NULL;
    char CalibFn[] = "C:\\cal.txt";
    char CalibOutFn[] = "C:\\cal_out.txt";
    int Err = 0;
    int Rval, MsgAvailable;
    char Buf[256], OutputStr[256];
    char change_line='\n';
    int NumToken;
    char *Tokens[MAX_CLI_CMD_TOKEN];

    CalibFn[0] = AmpUT_CalibGetDriverLetter();
    if ( (FpIn = AmbaFS_fopen(CalibFn, "r")) == NULL ) {
        Err = 1;
        AmbaPrint("can't open file %s",CalibFn);
        return -1;
    }

    CalibOutFn[0] = AmpUT_CalibGetDriverLetter();
    if ( (FpOut = AmbaFS_fopen(CalibOutFn, "w")) == NULL ) {
        Err = 1;
        AmbaPrint("can't open file %s",CalibOutFn);
        return -1;
    }
    if (Err) {
        AmbaFS_fclose(FpIn);
        AmbaFS_fclose(FpOut);
        return -1;
    }

    while (1) {
        // Read a line from the file
        if (AmpUT_CalibGetLine(FpIn, Buf) == -1) {
            break;
        }

        // Execute the command and show result
        AmpUT_CalibGetLineToken(Buf, &NumToken, Tokens);
        Rval = AmpUT_CalibCli(NumToken, Tokens, OutputStr, &MsgAvailable);
        if (MsgAvailable) {
            AmbaFS_fwrite(OutputStr, strlen(OutputStr), 1, FpOut);
            AmbaFS_fwrite(&change_line, 1, 1, FpOut);
        }


        // If any command failed, then skip all subsequent commands
        if (Rval < 0) {
            break;
        }
    }
    AmbaFS_fclose(FpIn);
    AmbaFS_fclose(FpOut);
    //close calib task,if open calib task
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSdIfTimerHandler
 *
 *  @Description:: Timer handler for calibration by SD card interface
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          void
\*-----------------------------------------------------------------------------------------------*/
void AmpUT_CalibSdIfTimerHandler(void)
{
    static int Time = CALIB_SD_IF_COUNT_DOWN;
    char calib_str[80] = {0};

    Time--;
    if (Time == 0) {
        /** Unregister countdown timer first */
        Time = CALIB_SD_IF_COUNT_DOWN;
        CalibSel = CALIB_MODE0;
        AmbaPrint("Calibration is beginning......");
        AmpUT_CalibSdif ();
        //app_calib_task_init();
    } else {
        // Show remaining Time GUI
        sprintf(calib_str, "calibration go after %d sec", Time);
        AmbaPrint("AmpUT_CalibSdIfTimerHandler: %d", Time);
    }
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibPathIf
 *
 *  @Description:: calibration by SD card interface
 *
 *  @Input      ::
 *          Path: the path for the calibration script
 *
 *  @Output     ::
 *          "cal_out.txt": output for calibration result
 *  @Return     ::
 *          int: OK:0/NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibPathIf (char* Path)
{
    AMBA_FS_FILE *FpIn = NULL, *FpOut = NULL;
    char CalibOutFn[256];
    int Err = 0;
    int Rval, MsgAvailable;
    char Buf[256], OutputStr[256];
    char change_line='\n';
    int NumToken;
    char *Tokens[MAX_CLI_CMD_TOKEN];

    AmbaPrint("AmpUT_CalibPathIf");
    if ((FpIn = AmbaFS_fopen(Path, "r")) == NULL ) {
        AmbaPrint("File open error %s",Path);
        AmbaFS_fclose(FpIn);
        return -1;
    }
    CalibOutFn[0] = '\0';
    strcat(CalibOutFn, Path);
    strcat(CalibOutFn, ".log");


    if ( (FpOut = AmbaFS_fopen(CalibOutFn, "w")) == NULL ) {
        AmbaPrint("File open error %s",CalibOutFn);
        return -1;
    }

    if (Err) {
        AmbaFS_fclose(FpIn);
        AmbaFS_fclose(FpOut);
        return -1;
    }
    while (1) {
        // Read a line from the file
        if (AmpUT_CalibGetLine(FpIn, Buf) == -1) {
            break;
        }
        // Execute the command and show result
        AmpUT_CalibGetLineToken(Buf, &NumToken, Tokens);
        if(NumToken == 0){
            continue;
        }

        Rval = AmpUT_CalibCli(NumToken, Tokens, OutputStr, &MsgAvailable);
        if (MsgAvailable) {
            AmbaFS_fwrite(OutputStr, strlen(OutputStr), 1, FpOut);
            AmbaFS_fwrite(&change_line, 1, 1, FpOut);
        }
        // If any command failed, then skip all subsequent commands
        if (Rval < 0) {
            break;
        }

    }
    AmbaFS_fclose(FpIn);
    AmbaFS_fclose(FpOut);

    return 0;

}

