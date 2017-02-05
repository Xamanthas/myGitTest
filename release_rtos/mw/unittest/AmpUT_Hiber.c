/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaHiber_Test.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Test/Reference code for AmbaHiber.
 *
 *  @History        ::
 *      Date        Name        Comments
 *      10/15/2013  Kerson     Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>

#include "AmpUnitTest.h"
#include "AmbaLink.h"


/*============================================================================*\
 *                                                                            *
 *                           Test module Management                           *
 *                                                                            *
\*============================================================================*/
static void HiberShowBootType(AMBA_SHELL_ENV_s *pEnv , int Argc, char **Argv)
{
    switch (AmbaLink_BootType(5000)) {
    case AMBALINK_WARM_BOOT:
        AmbaPrint("Linux is warm boot.");
        break;
    case AMBALINK_HIBER_BOOT:
        AmbaPrint("Linux is hibernation boot.");
        break;
    case AMBALINK_COLD_BOOT:
    default:
        AmbaPrint("Linux is cold boot.");
    }
}

static void HiberTest_Suspend(AMBA_SHELL_ENV_s *pEnv, int Mode)
{
    AmbaPrint("AmbaIPC_LinkCtrlSuspendLinux(%d)");
    AmbaIPC_LinkCtrlSuspendLinux(Mode);

    if (AmbaIPC_LinkCtrlWaitSuspendLinux(5000) == OK)
        AmbaPrint("AmbaIPC_LinkCtrlWaitSuspendLinux(%d) done.", Mode);
}

static void HiberTest_Resume(AMBA_SHELL_ENV_s *pEnv, int Mode)
{
    AmbaPrint("AmbaIPC_LinkCtrlResumeLinux(%d)", Mode);
    AmbaIPC_LinkCtrlResumeLinux(Mode);

    if (AmbaIPC_LinkCtrlWaitResumeLinux(5000) == OK)
    	AmbaPrint("AmbaIPC_LinkCtrlWaitResumeLinux(%d) done.", Mode);
}

static inline void HiberTestUsage(AMBA_SHELL_ENV_s *pEnv , int Argc, char **Argv)
{
    AmbaPrint("Usage: t %s <command>, where command is:", Argv[0]);
    AmbaPrint("\tsuspend <suspend_mode>, 0: hiber to NAND, 1: hiber to RAM, 2: standby to RAM, 3: Sleep to RAM");
    AmbaPrint("\tresume  <suspend_mode>, 0: hiber to NAND, 1: hiber to RAM, 2: standby to RAM, 3: Sleep to RAM");
    AmbaPrint("\tstress  <suspend_mode> <count>, 0: hiber to NAND, 1: hiber to RAM, 2: standby to RAM, 3: Sleep to RAM");
    AmbaPrint("\twipeout");
    AmbaPrint("\tboottype");
}

static int AmpUT_HiberTestEntry(AMBA_SHELL_ENV_s *pEnv, int Argc, char **Argv)
{
	extern int AmbaIPC_HiberWipeout(UINT32 Flag);

	int Rval = 0;
    UINT32 Count, i;

	if (Argc == 3 && strcmp(Argv[1], "suspend") == 0) {
		sscanf(Argv[2], "%d", &Rval);
        HiberTest_Suspend(pEnv, Rval);

	} else if (Argc == 3 && strcmp(Argv[1], "resume") == 0) {
		sscanf(Argv[2], "%d", &Rval);
        HiberTest_Resume(pEnv, Rval);

	} else if (Argc == 4 && strcmp(Argv[1], "stress") == 0) {
		sscanf(Argv[2], "%d", &Rval);
   		sscanf(Argv[3], "%d", &Count);

        for (i = 0; i < Count; i++) {
            AmbaPrint("===== stress loop %d =====", i);

            HiberTest_Suspend(pEnv, Rval);
            HiberTest_Resume(pEnv, Rval);
        }

	} else if (Argc == 2 && strcmp(Argv[1], "wipeout") == 0) {
		Rval = AmbaIPC_HiberWipeout(0);

	} else if (Argc == 2 && strcmp(Argv[1], "boottype") == 0) {
		HiberShowBootType(pEnv, Argc, Argv);

	} else {
		HiberTestUsage(pEnv, Argc, Argv);
		return -1;
	}

    return Rval;
}

/*----------------------------------------------------------------------------*\
 *  @RoutineName::  AmbaHiber_TestInit
 *
 *  @Description::  Init test module
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
 *
\*----------------------------------------------------------------------------*/
int AmpUT_HiberTestAdd(void)
{
    AmbaPrint("Adding Test command hiber: %s %d",__FUNCTION__, __LINE__);
    AmbaTest_RegisterCommand("hiber", AmpUT_HiberTestEntry);

    return 0;
}
