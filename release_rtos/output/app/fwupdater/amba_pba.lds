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
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(__vector)
SECTIONS
{
 . = 0xa0000000;
 __ddr_start = .;
 __ddr_resv_start = .;
 . = __ddr_resv_start + 0x00001000;
 __ddr_resv_end = .;
 .text : {
  __ddr_region0_ro_start = .;
  *AmbaBoot.o (.text*)
  *AmbaHwStack.o (.text*)
  *AmbaBoot.o (.rodata*)
  *AmbaHwStack.o (.rodata*)
  * (.text*)
  * (.rodata*)
  * (.constdata*)
  /* For libaudio.a */
  * (i.__ARM_common*)
  * (FFTCODE*)
  __ddr_region0_ro_end = .;
  . = ALIGN(32);
  __ddr_region1_ro_start = .;
  __ddr_region1_ro_end = .;
  . = ALIGN(32);
  __ddr_region2_ro_start = .;
  __ddr_region2_ro_end = .;
  . = ALIGN(32);
  __ddr_region3_ro_start = .;
  __ddr_region3_ro_end = .;
  . = ALIGN(32);
  __ddr_region4_ro_start = .;
  __ddr_region4_ro_end = .;
  . = ALIGN(32);
  __ddr_region5_ro_start = .;
  __ddr_region5_ro_end = .;
 }
 .dsp_buf : {
  . = ALIGN(0x1000);
  __dsp_buf_start = .;
  * (.dsp_buf*)
  __dsp_buf_end = .;
 }
 .data : {
  . = ALIGN(0x1000);
  __ddr_region0_rw_start = .;
  *AmbaBoot.o (.data*)
  *AmbaHwStack.o (.data*)
  * (.data*)
  __ddr_region0_rw_end = .;
  . = ALIGN(32);
  __ddr_region1_rw_start = .;
  __ddr_region1_rw_end = .;
  . = ALIGN(32);
  __ddr_region2_rw_start = .;
  __ddr_region2_rw_end = .;
  . = ALIGN(32);
  __ddr_region3_rw_start = .;
  __ddr_region3_rw_end = .;
  . = ALIGN(32);
  __ddr_region4_rw_start = .;
  __ddr_region4_rw_end = .;
  . = ALIGN(32);
  __ddr_region5_rw_start = .;
  __ddr_region5_rw_end = .;
 }
 /* Provided malloc heap for libc. */
 PROVIDE (end = .);
 __sbrk_start = .;
 . = __sbrk_start + 0x2000;
 __sbrk_end = .;
 .bss.noinit : {
  . = ALIGN(32);
  __noinit_start = .;
  * (.bss.noinit*)
  __noinit_end = .;
 }
 .bss : {
  . = ALIGN(32);
  __bss_start = .;
  * (.bss*)
  __bss_end = .;
 }
 /* FIXME: There are some unused memory here ! */
 /***********************/
 /* ucode orcode region */
 /***********************/
 . = ALIGN(0x100000);
 __ucode_start = .;
 __ucode_orcode_start = .;
 . = __ucode_orcode_start + 0x480000;
 __ucode_orcode_end = .;
 . = ALIGN(32);
 __ucode_orcme_start = .;
 . = __ucode_orcme_start + 0x80000;
 __ucode_orcme_end = .;
 . = ALIGN(32);
 __ucode_orcmd_start = .;
 . = __ucode_orcmd_start + 0x80000;
 __ucode_orcmd_end = .;
 . = ALIGN(32);
 __ucode_default_data_start = .;
 . = __ucode_default_data_start + 0x80000;
 __ucode_default_data_end = .;
 __ucode_end = .;
 /***************************/
 /*  non-cached heap region */
 /***************************/
 . = ALIGN(0x1000);
 __non_cache_heap_start = .;
 . = __non_cache_heap_start + 0x1000000;
 __non_cache_heap_end = .;
 /***************************/
 /*  cached heap region     */
 /***************************/
 . = ALIGN(0x1000);
 __cache_heap_start = .;
 . = __ddr_start + 0x40000000 - 0x04000000;
 __cache_heap_end = .;
 /*************************/
 /* linux region          */
 /*************************/
 . = ALIGN(0x100000);
 __linux_start = .;
 . = __linux_start + 0x04000000;
 __linux_end = .;
 /*************************/
 /* DDR end               */
 /*************************/
 /*
	 * Provide a end symbol for newlib malloc.
	 * We don't use malloc for newlib, so it should be ok.
	 */
 . = __ddr_start + 0x40000000;
 __ddr_end = .;
}
