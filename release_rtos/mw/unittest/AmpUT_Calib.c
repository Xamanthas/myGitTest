/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmpUT_Calib.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Unit test function for calibration
 *
 *  @History        ::
 *      Date        Name        Comments
 *      07/10/2013  Allen Chiu Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/
#include "AmpUnitTest.h"
#include <string.h>
#include <math.h>
#include "AmbaPrintk.h"
#include "AmbaSensor.h"
#include "extSrc/sample_calib/AmbaUTCalibMgr.h"
#include "extSrc/sample_calib/AmbaUTCalibCli.h"

extern int AmpUT_CalibSiteInit(void);
extern int AmpUTVignette_UTFunc(int Argc, char *Argv[]);
extern int AmpUTCalibBPC_UTFunc(int Argc, char *Argv[]);
extern int AmpUTBLC_UTFunc(int Argc, char *Argv[]);
extern int AmpUTWB_UTFunc(int Argc, char *Argv[]);
extern int AmpUTWarp_UTFunc(int Argc, char *Argv[]);
extern int AmpUTCA_UTFunc(int Argc, char *Argv[]);
//extern int AmpUTAudio_UTFunc(int Argc, char *Argv[]);

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibMgr
 *
 *  @Description:: test cmd to do calibration
 *
 *  @Input      ::
 *          env: environment
 *          argc: the number of the input parameter
 *          argv: value of input parameter
 *
 *  @Output     ::
 *          None
 *
 *  @Return     ::
 *          INT: OK:0/ NG:-1
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibMgr(AMBA_SHELL_ENV_s  *env, int argc, char *argv[])
{
    int rval = -1;
    int msg_available;
    char output_str[256];

    if ((strcmp(argv[2], "cmd") == 0)) {
        // Compose command string
        rval = AmpUT_CalibCli(argc-3, argv+3, output_str, &msg_available);
        if (msg_available) {
            AmbaPrint("Output string: %s", output_str);
        }
    } else if ((strcmp(argv[2], "path") == 0)) {
        rval = AmpUT_CalibPathIf(argv[3]);
    }
    if (rval == -1) {
        AmbaPrint("t cal -mgr cmd [cali_cmd]: calibration cmd i/f (arg num<15)\n"
                  "t cal -mgr path : specify calibation script path\n");
    }
    return rval;
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibUseage
 *
 *  @Description:: print the useage for calibration unit test
 *
 *  @Input      ::
 *          None
 *  @Output     ::
 *          None
 *
 *  @Return     ::
 *          None
\*-----------------------------------------------------------------------------------------------*/
void AmpUT_CalibUseage(void)
{

    AmbaPrint(" calib test command:");
    AmbaPrint(" site_init : calibration site init");
    AmbaPrint(" mgr : for doing calibration ");
    AmbaPrint(" bpc : Amba bad pixel calibration");
    AmbaPrint(" vnc : Amba vignette calibration");
    AmbaPrint(" blc : Amba black level calibration");
    AmbaPrint(" wb : Amba black level calibration");
    AmbaPrint(" warp : warp calibration");
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibTest
 *
 *  @Description:: entry function for calibration unit test
 *
 *  @Input      ::
 *          env: environment
 *          argc: the number of the input parameter
 *          argv: value of input parameter
 *
 *  @Output     ::
 *          None
 *
 *  @Return     ::
 *          INT: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibTest(AMBA_SHELL_ENV_s  *env, int argc, char *argv[])
{
    AmbaPrint("AmpUT_CalibTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"mgr") == 0) {
        AmpUT_CalibMgr(env, argc, argv);
    } else if (strcmp(argv[1],"site_init") == 0) {
        AmpUT_CalibSiteInit();
    } else if (strcmp(argv[1],"bpc") == 0) {
        AmpUTCalibBPC_UTFunc(argc, argv);
    } else if (strcmp(argv[1],"vnc") == 0) {
        AmpUTVignette_UTFunc(argc, argv);
    } else if (strcmp(argv[1],"blc") == 0) {
        AmpUTBLC_UTFunc(argc, argv);
    } else if (strcmp(argv[1],"wb") == 0) {
        AmpUTWB_UTFunc(argc, argv);
    } else if (strcmp(argv[1],"warp") == 0) {
        AmpUTWarp_UTFunc(argc, argv);
    } else if (strcmp(argv[1],"ca") == 0) {
        AmpUTCA_UTFunc(argc, argv);
    } else if (strcmp(argv[1],"audio") == 0) {
//        AmpUTAudio_UTFunc(argc, argv);
    }else {
        AmpUT_CalibUseage();
    }
    return 0;
}



/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibInit
 *
 *  @Description:: register the unit test function for calibration
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *
 *  @Return     ::
 *          INT: OK:0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibModuleInit(void)
{
    // hook command
    AmbaTest_RegisterCommand("cal", AmpUT_CalibTest);

    return AMP_OK;
}



