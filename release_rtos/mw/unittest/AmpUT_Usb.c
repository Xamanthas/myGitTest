/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmpUT_Usb.c
 *
 *  @Copyright      :: Copyright (C) 2014 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Ambarella Shell APIs
 *
 *  @History        ::
 *      Date        Who         Comments
 *      03/12/2014  pwchen      Created
 *
 \*-------------------------------------------------------------------------------------------------------------------*/

#include "AmbaDataType.h"
#include "AmbaKAL.h"
#include "AmbaFS.h"
#include "AmbaPrintk.h"
#include "AmbaShell.h"
#include "AmbaTest.h"
#include "AmbaUtility.h"
#include "extSrc/sample_usb/AmpUsb.h"

static int USB_TestUsage(void)
{
	int RetVal = OK;

	AmbaPrint("MSPUSB test command:");
	AmbaPrint("  init_device [class]: Init USB Device");
	AmbaPrint("       [class] : msc, mtp.");
	return RetVal;
}

static int MSPUSB_TestCmd(AMBA_SHELL_ENV_s *Env, int Argc, char **Argv)
{
	int RetVal = OK;

	if (Argc >= 2) {
        if (strcmp(Argv[1], "init_device") == 0) {
			if (Argc == 2) {
                AmpUSB_Init();
                AmpUSB_InitJack();
                AmpUsbMsc_DoMountInit();
                AmpUsbMsc_DoMount(2);
                AmpUsbMsc_Start();
			} else {
				if (strcmp(Argv[2], "msc") == 0) {
                    AmpUSB_Init();
                    AmpUSB_InitJack();
                    AmpUsbMsc_DoMountInit();
                    AmpUsbMsc_DoMount(2);
                    AmpUsbMsc_Start();
				} else if (strcmp(Argv[2], "mtp") == 0) {
                    AmpUSB_Init();
                    AmpUSB_InitJack();
                    AmpUsbAmage_Start();
				} else {
					AmbaPrint("Don't know how to init %s", Argv[2]);
				}
			}
        } else {
			RetVal = NG;
		}
	} else {
		RetVal = NG;
	}

	if (RetVal != OK) {
		USB_TestUsage();
	}

	return RetVal;
}

int  AmbaTest_MspUsbAddCommands(void)
{
	AmbaPrint("Adding AmpUT_Usb cmd");
	AmbaTest_RegisterCommand("mspusb", MSPUSB_TestCmd);
	return OK;
}

