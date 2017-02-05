/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaShell_AmbaLink.c
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions of Run Time Support Library for NAND flash
 *
 *  @History        ::
 *      Date        Name        Comments
 *      07/30/2013  Evan Chen    Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#include    <stdio.h>
#include    <string.h>

#include    "AmbaDataType.h"
#include    "AmbaKAL.h"
#include    "AmbaLink.h"
#include    "AmbaShell.h"

#ifdef AMBA_LINK
static inline void usage(AMBA_SHELL_ENV_s *pEnv , int Argc, char **Argv)
{
    AmbaShell_Print(pEnv,
                    "Usage: %s <command>, where command is:\n"
                    "\tboot\n"
                    "\thiber_return\n"
                    "\thiber_resume\n"
                    "\thiber_wipeout\n"
#if 0
                    "\tboot-halt\n"
                    "\tcmdline [new value]\n"
                    "\tqboot\n"
                    "\treboot\n"
                    "\tsuspend\n"
                    "\tresume\n"
                    "\twipeout\n"
                    "\tprintk\n"
                    "\tabwipeout\n"
                    "\tabresume\n"
                    "\tabreturn\n"
                    "\tabdump d\n"
                    "\twowlan [cut power]\n"
                    "\tturbo [0 or 1]\n"
                    "\tpriority 115\n",
#endif
                    , Argv[0]);
}

int ambsh_ambalink(AMBA_SHELL_ENV_s *env, int Argc, char **Argv)
{
    int Rval;

    if (Argc == 2 && strcmp(Argv[1], "boot") == 0) {
        extern int AmbaLink_Boot(void);
        Rval = AmbaLink_Boot();
    } else if (Argc == 2 && strcmp(Argv[1], "hiber_return") == 0) {
        int AmbaIPC_HiberReturn(void);
        AmbaIPC_HiberReturn();
    } else if (Argc == 2 && strcmp(Argv[1], "hiber_resume") == 0) {
        int AmbaIPC_HiberResume(UINT32 Flag);
        AmbaIPC_HiberResume(0);
        __SEV();      /* Will make following WFE clear event flag but not sleep */
    } else if (Argc == 2 && strcmp(Argv[1], "hiber_wipeout") == 0) {
        int AmbaIPC_HiberWipeout(UINT32 Flag);
        AmbaIPC_HiberWipeout(0);
    } else {
        usage(env, Argc, Argv);
        return -1;
    }
    return Rval;
}
#else
int ambsh_ambalink(AMBA_SHELL_ENV_s *env, int Argc, char **Argv) { return 0; }
#endif

