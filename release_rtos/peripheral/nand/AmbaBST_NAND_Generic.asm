/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaNAND_Generic.asm
 *
 *  @Copyright      :: Copyright (C) 2015 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Generic NAND Parameters for Bootstrap
\*-------------------------------------------------------------------------------------------------------------------*/

        .global AmbaWarmBootMagicCode
        .global AmbaNandTiming

        @.data
        .section BstNandParam

AmbaWarmBootMagicCode:
        .string "AmbarellaCamera"

AmbaNandTiming:
        .word   0x20202020
        .word   0x20202020
        .word   0x20204020
        .word   0x20202020
        .word   0x20202020
        .word   0x20202020
