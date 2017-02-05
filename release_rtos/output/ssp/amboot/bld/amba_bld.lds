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
 . = 0x10000000;
 .text : {
  *AmbaBootlooder.o (.text)
  * (.text*)
  * (.rodata*)
  *(.fixup)
 }
 . = ALIGN(4);
 .data : {
  * (.data*)
 }
 . = ALIGN(4);
 .bss.noinit : {
                __noinit_start = .;
                * (.bss.noinit*)
                __noinit_end = .;
        }
 . = ALIGN(4);
 .bss : {
                . = ALIGN(4);
  __bss_start = .;
  * (.bss*)
  * (COMMON)
  __bss_end = .;
  /* Stack for SVC mode */
  . = ALIGN(32);
  __svc_stack_start = .;
  . = __svc_stack_start + 0x8000;
  __svc_stack_end = .;
  /* Stack for IRQ mode */
  . = ALIGN(32);
  __irq_stack_start = .;
  . = __irq_stack_start + 0x100;
  __irq_stack_end = .;
  /* Stack for FIQ mode */
  . = ALIGN(32);
  __fiq_stack_start = .;
  . = __fiq_stack_start + 0x100;
  __fiq_stack_end = .;
  /* Stack for UND mode */
  . = ALIGN(32);
  __und_stack_start = .;
  . = __und_stack_start + 0x100;
  __und_stack_end = .;
  /* Stack for ABT mode */
  . = ALIGN(32);
  __abt_stack_start = .;
  . = __abt_stack_start + 0x100;
  __abt_stack_end = .;
  /* Stack for SYS mode */
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
  *(.ARM.exidx.exit.text)
  *(.ARM.extab.exit.text)
  *(.ARM.exidx.cpuexit.text)
  *(.ARM.extab.cpuexit.text)
  *(.ARM.exidx.devexit.text)
  *(.ARM.extab.devexit.text)
  *(.discard*)
 }
}
