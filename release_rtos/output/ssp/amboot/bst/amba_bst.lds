/*
 *
 * Automatically generated file; DO NOT EDIT.
 * Ambarella SDK Configuration
 *
 */
/*
 * Helper macros to use CONFIG_ options in C/CPP expressions. Note that
 * these only work with boolean and tristate options.
 */
/*
 * Getting something that works in C and CPP for an arg that may or may
 * not be defined is tricky.  Here, if we have "#define CONFIG_BOOGER 1"
 * we match on the placeholder define, insert the "0," for arg1 and generate
 * the triplet (0, 1, 0).  Then the last step cherry picks the 2nd arg (a one).
 * When CONFIG_BOOGER is not defined, we generate a (... 1, 0) pair, and when
 * the last step cherry picks the 2nd arg, we get a zero.
 */
/*
 * IS_ENABLED(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'y' or 'm',
 * 0 otherwise.
 *
 */
/*
 * IS_BUILTIN(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'y', 0
 * otherwise. For boolean options, this is equivalent to
 * IS_ENABLED(CONFIG_FOO).
 */
/*
 * IS_MODULE(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'm', 0
 * otherwise.
 */
/**
 * ssp/amboot/bst/amba_bst.lds
 *
 * History:
 *    2014/11/04 - [Kerson Chen] modified from amboot bst.
 *
 * Copyright (C) 2004-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
DRAM_START_ADDR = 0x00000000;
MIN_DRAM_SIZE = 0x01000000;
AMBOOT_BST_RAM_START = DRAM_START_ADDR;
NAND_BST_START = 0xffff0000;
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(__vector)
SECTIONS
{
 . = NAND_BST_START;
 .bst_stage1 . : {
        *AmbaCortexBootstrapPreboot.o (.text)
  *AmbaCortexBootstrap.o (.text)
  *AmbaCortexInit*.o (.text)
  *AmbaCortexBootstrap.o (.rodata)
  *AmbaCortexInit*.o (.rodata)
 }
 .bst_stage2 AMBOOT_BST_RAM_START : AT (ADDR(.bst_stage1) + SIZEOF(.bst_stage1)) {
  __postreloc_ram_start = .;
  *AmbaCortexBootstrapNAND* (.text)
  __postreloc_ram_end = .;
 }
 .data (NAND_BST_START + SIZEOF(.bst_stage1) + SIZEOF(.bst_stage2)) : AT (ADDR(.bst_stage1) + SIZEOF(.bst_stage1) + SIZEOF(.bst_stage2)) {
  __table_start = .;
  *(BstNandParam)
  *(BstDramParam)
  __table_end = .;
  . += 0x800 - (SIZEOF(.bst_stage1) + SIZEOF(.bst_stage2) + __table_end - __table_start) ;
 }
 __postreloc_start = LOADADDR(.bst_stage2);
 __postreloc_end = __postreloc_start + __postreloc_ram_end - __postreloc_ram_start;
 /DISCARD/ : {
  *(.ARM.exidx.exit.text)
  *(.ARM.extab.exit.text)
  *(.ARM.exidx.cpuexit.text)
  *(.ARM.extab.cpuexit.text)
  *(.ARM.exidx.devexit.text)
  *(.ARM.extab.devexit.text)
  *(.ARM*)
  *(.debug*)
  *(.discard*)
 }
}
