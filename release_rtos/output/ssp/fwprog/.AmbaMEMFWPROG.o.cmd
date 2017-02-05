cmd_ssp/fwprog/AmbaMEMFWPROG.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -Wp,-MD,ssp/fwprog/.AmbaMEMFWPROG.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly  -I/home/bingo/RD_A12/release_rtos/ssp/fwprog -Issp/fwprog -mlittle-endian -DAMBA_KAL_NO_SMP -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-format-security -fno-delete-null-pointer-checks -fshort-enums -fshort-wchar -D"SOC=KBUILD_STR(A12)" -O2 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -ffunction-sections -fdata-sections -Wno-missing-braces -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -gdwarf-3 -Wdeclaration-after-statement -Wpointer-sign -fno-strict-overflow -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -std=c99 -DCC_HAVE_ASM_GOTO -DAMBA_KAL_NO_SMP  -I/home/bingo/RD_A12/release_rtos/ssp/fwprog -Issp/fwprog   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/misc    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(AmbaMEMFWPROG)"  -D"KBUILD_MODNAME=KBUILD_STR(libfwprog)" -c -o ssp/fwprog/AmbaMEMFWPROG.o /home/bingo/RD_A12/release_rtos/ssp/fwprog/AmbaMEMFWPROG.c

source_ssp/fwprog/AmbaMEMFWPROG.o := /home/bingo/RD_A12/release_rtos/ssp/fwprog/AmbaMEMFWPROG.c

deps_ssp/fwprog/AmbaMEMFWPROG.o := \
    $(wildcard build/include/config/emmc/accpart/user.h) \
    $(wildcard build/include/config/emmc/accpart/bp1.h) \
    $(wildcard build/include/config/emmc/bootpart/user.h) \
    $(wildcard build/include/config/emmc/bootpart/bp1.h) \
    $(wildcard build/include/config/emmc/boot/1bit.h) \
    $(wildcard build/include/config/emmc/boot/4bit.h) \
    $(wildcard build/include/config/s.h) \
    $(wildcard build/include/config/enable/emmc/boot.h) \
    $(wildcard build/include/config/enable/spinor/boot.h) \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/stdio.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/_ansi.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/newlib.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/config.h \
    $(wildcard build/include/config/h//.h) \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/ieeefp.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/features.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/cdefs.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/_default_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/lib/gcc/arm-none-eabi/4.9.3/include/stddef.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/lib/gcc/arm-none-eabi/4.9.3/include/stdarg.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/reent.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/_ansi.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/lock.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/stdio.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/string.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/string.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaDataType.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaHwIO.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaSD_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNAND_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaPartition_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNOR_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaRTSL_NAND.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNAND_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNAND_PartitionTable.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNOR_PartitionTable.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNOR_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os/AmbaNonOS_NAND.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os/AmbaNonOS_EMMC.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os/AmbaNonOS_NOR.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os/AmbaNonOS_NandBBT.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os/AmbaNonOS_UART.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os/AmbaFWPROG.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os/AmbaNORFWPROG.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaCortexA9.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaGPIO.h \
    $(wildcard build/include/config/e.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaGPIO_Def.h \
    $(wildcard build/include/config/e/.h) \
    $(wildcard build/include/config/input.h) \
    $(wildcard build/include/config/output.h) \
    $(wildcard build/include/config/alternate.h) \
    $(wildcard build/include/config/s/.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaRTSL_PLL.h \
    $(wildcard build/include/config/u.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaPLL_Def.h \
    $(wildcard build/include/config/u/.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaRTSL_FIO.h \
    $(wildcard build/include/config/nand/none.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaRTSL_I2S.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaI2S_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaRTSL_MMU.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaRTSL_MMU_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaRTSL_GPIO.h \

ssp/fwprog/AmbaMEMFWPROG.o: $(deps_ssp/fwprog/AmbaMEMFWPROG.o)

$(deps_ssp/fwprog/AmbaMEMFWPROG.o):
