/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaCortexNAND_S34MLxG1.asm
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: NAND Parameters for ARM11 under core frequency 312Mhz
 *-------------------------------------------------------------------------------------------------------------------*/

        @MODULE  ?AmbaARM11NAND_Param

        @SECTION BstNandParam:CODE:ROOT(2)

        .global  AmbaWarmBootMagicCode
        .global  AmbaNandTiming

        @.data
	.section BstNandParam

#if !defined(CONFIG_ENABLE_SPINOR_BOOT)
AmbaWarmBootMagicCode:
        @DC8     'AmbarellaCamera', 0x00
        .string  "AmbarellaCamera"
#endif

AmbaNandTiming:
        .word    0x02020402
        .word    0x01010101
        .word    0x02021004
        .word    0x0202100b
        .word    0x05020c00
        .word    0x00101002
