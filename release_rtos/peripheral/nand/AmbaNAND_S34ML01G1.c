/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaNAND_S34ML01G1.c
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: NAND device Info
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaNAND_Def.h"

const AMBA_NAND_DEV_INFO_s AmbaNAND_DevInfo = {
    "SPANSION S34ML02G1_256MB_PG2K",    /* NAND_NAME */

    AMBA_NAND_SIZE_1G_BITS,

    /*--- DevInfo ---*/
    2112,   /* NAND_PAGE_SIZE */
    64,     /* NAND_SPARE_SIZE */
    64,     /* NAND_PAGES_PER_BLOCK */
    1024,   /* NAND_BLOCKS_PER_PLANE */
    1024,   /* NAND_BLOCKS_PER_ZONE */
    1,      /* NAND_TOTAL_ZONES (NAND_TOTAL_BLOCKS / NAND_BLOCKS_PER_ZONE) */
    1,      /* NAND_TOTAL_PLANES (NAND_TOTAL_BLOCKS / NAND_BLOCKS_PER_PLANE) */

    2,      /* NAND_COLUMN_CYCLES */
    2,      /* NAND_PAGE_CYCLES */
    4,      /* NAND_ID_CYCLES */
    4,      /* NAND_ECC_BIT */

    /* Copyback must be in the same plane, so we have to know the plane address */
    0x1,  /* NAND_PLANE_ADDR_MASK */

    /*--- Timing ---*/
    12,     /* NAND_TCLS */
    12,     /* NAND_TALS */
    20,     /* NAND_TCS */
    12,     /* NAND_TDS */
    5,      /* NAND_TCLH */
    5,      /* NAND_TALH */
    5,      /* NAND_TCH */
    5,      /* NAND_TDH */
    12,     /* NAND_TWP */
    10,     /* NAND_TWH */
    100,    /* NAND_TWB */
    20,     /* NAND_TRR */
    12,     /* NAND_TRP */
    10,     /* NAND_TREH */
    100,    /* NAND_TRB */
    70,     /* NAND_TCEH */
    20,     /* NAND_TREA */
    10,     /* NAND_TCLR */
    60,     /* NAND_TWHR */
    0,      /* NAND_TIR */
    100,    /* NAND_TWW */
    100,    /* NAND_TRHZ */
    10      /* NAND_TAR */
};
