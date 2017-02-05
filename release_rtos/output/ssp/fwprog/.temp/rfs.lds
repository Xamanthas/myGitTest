





OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(__vector)
SECTIONS
{
 . = 0x100000;
 .text : {
  __BEGIN_FIRMWARE_IMAGE__ = ABSOLUTE(.);

  *AmbaBootlooder.o (.text)
  *.o (.text*)
  * (.rodata*)
 }

 .data : {
  . = ALIGN(4);
  * (.data*)
 }

 .fwdata : {
  . = ALIGN(2048);
  __BEGIN_RFS_IMAGE__ = .;
  KEEP (ssp/fwprog/.temp/rfs.fw (*))
  . = ALIGN(2048);
                . += 0x100;
  __END_RFS_IMAGE__ = .;


  . = ALIGN(2048);
  __END_FIRMWARE_IMAGE__ = ABSOLUTE(.);
 }

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
  * (COMMON)
  __bss_end = .;



  . = ALIGN(32);
  __svc_stack_start = .;
  . = __svc_stack_start + 0x8000;
  __svc_stack_end = .;



  . = ALIGN(32);
  __irq_stack_start = .;
  . = __irq_stack_start + 0x100;
  __irq_stack_end = .;



  . = ALIGN(32);
  __fiq_stack_start = .;
  . = __fiq_stack_start + 0x100;
  __fiq_stack_end = .;



  . = ALIGN(32);
  __und_stack_start = .;
  . = __und_stack_start + 0x100;
  __und_stack_end = .;



  . = ALIGN(32);
  __abt_stack_start = .;
  . = __abt_stack_start + 0x100;
  __abt_stack_end = .;



  . = ALIGN(32);
  __sys_stack_start = .;
  . = __sys_stack_start + 0x8000;
  __sys_stack_end = .;
 }

  PROVIDE (end = .);

 .memfwprog . (NOLOAD) : {
  . = ALIGN(0x10000);
  __memfwprog_result = .;
  . = __memfwprog_result + (0x10000);
  . = ALIGN(0x10000);
  __memfwprog_command = .;
  . = __memfwprog_command + (0x10000);
 }

 /DISCARD/ : {

 * (.ARM.exidx)


  ssp/fwprog/.temp/bst.fw (*)


  ssp/fwprog/.temp/bld.fw (*)


  ssp/fwprog/.temp/pba.fw (*)


  ssp/fwprog/.temp/sys.fw (*)


  ssp/fwprog/.temp/dsp.fw (*)


  ssp/fwprog/.temp/rom.fw (*)


  ssp/fwprog/.temp/lnx.fw (*)


  ssp/fwprog/.temp/dtb.fw (*)




 }
}
