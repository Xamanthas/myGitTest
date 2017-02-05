/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmpUT_Csc.c
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Csc Unit test
 *
 *  @History        ::
 *      Date        Name        Comments
 *      11/20/'14   PeterWeng
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#include <display/Display.h>
#include "AmpUnitTest.h"
#include "AmbaPrint.h"
#include "stdlib.h"
#include "AmpUT_Display.h"

static UINT8 DispInit = 0;

void AmpUT_CscSet(char **argv)
{
	AMP_DISP_CSC_CONFIG_s CscCfg;
	AMP_DISP_HDLR_s *DispHdlr;

	AmbaPrint("AmpUT_CscSet");

	if (DispInit == 0) {
	    AmpUT_Display_Init();
	    DispInit = 1;
	}

	if (atoi(argv[2])==0){
	    AmpUT_Display_DispHdlrGet(0, &DispHdlr);
	} else {
	    AmpUT_Display_DispHdlrGet(1, &DispHdlr);
	}

	CscCfg.CscMatrix[0] = atof(argv[3]);
	CscCfg.CscMatrix[1] = atof(argv[4]);
	CscCfg.CscMatrix[2] = atof(argv[5]);
	CscCfg.CscMatrix[3] = atof(argv[6]);
	CscCfg.CscMatrix[4] = atof(argv[7]);
	CscCfg.CscMatrix[5] = atof(argv[8]);
	CscCfg.CscMatrix[6] = atof(argv[9]);
	CscCfg.CscMatrix[7] = atof(argv[10]);
	CscCfg.CscMatrix[8] = atof(argv[11]);

	CscCfg.Constant[0] = atoi(argv[12]);
	CscCfg.Constant[1] = atoi(argv[13]);
	CscCfg.Constant[2] = atoi(argv[14]);

	CscCfg.Clamp[0] = atoi(argv[15]);
	CscCfg.Clamp[1] = atoi(argv[16]);
	CscCfg.Clamp[2] = atoi(argv[17]);
	CscCfg.Clamp[3] = atoi(argv[18]);
	CscCfg.Clamp[4] = atoi(argv[19]);
	CscCfg.Clamp[5] = atoi(argv[20]);

	AmpDisplay_SetVoutCsc(DispHdlr, &CscCfg);
}

void AmpUT_CscGet(char **argv)
{
	AMP_DISP_CSC_CONFIG_s CscCfg;
	AMP_DISP_HDLR_s *DispHdlr;

	AmbaPrint("AmpUT_CscGet");

    if (DispInit == 0) {
        AmpUT_Display_Init();
        DispInit = 1;
    }

    if (atoi(argv[2])==0){
        AmpUT_Display_DispHdlrGet(0, &DispHdlr);
    } else {
        AmpUT_Display_DispHdlrGet(1, &DispHdlr);
    }

	AmpDisplay_GetVoutCsc(DispHdlr, &CscCfg);

	AmbaPrint(" %1.5f %1.5f %1.5f   %3d   %3d~%3d", CscCfg.CscMatrix[0],
			CscCfg.CscMatrix[1],
			CscCfg.CscMatrix[2],
			CscCfg.Constant[0],
			CscCfg.Clamp[0],
			CscCfg.Clamp[1]);
	AmbaPrint(" %1.5f %1.5f %1.5f   %3d   %3d~%3d", CscCfg.CscMatrix[3],
			CscCfg.CscMatrix[4],
			CscCfg.CscMatrix[5],
			CscCfg.Constant[1],
			CscCfg.Clamp[2],
			CscCfg.Clamp[3]);
	AmbaPrint(" %1.5f %1.5f %1.5f   %3d   %3d~%3d", CscCfg.CscMatrix[6],
			CscCfg.CscMatrix[7],
			CscCfg.CscMatrix[8],
			CscCfg.Constant[2],
			CscCfg.Clamp[4],
			CscCfg.Clamp[5]);
}

int AmpUT_CscTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_DisplayTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"set") == 0) {
        AmpUT_CscSet(argv);
    } else if ((strcmp(argv[1],"get") == 0) && (argc > 2)) {
    	AmpUT_CscGet(argv);
    } else {
    	AmbaPrint("t csc set [ch] [param...]");
    	AmbaPrint("t csc get [ch] [param...]");
    }
    return 0;
}

int AmpUT_CscInit(void)
{
    // hook command
    AmbaTest_RegisterCommand("csc", AmpUT_CscTest);

    return AMP_OK;
}
