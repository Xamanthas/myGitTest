cmd_ssp/fwprog/AmbaVer.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -Wp,-MD,ssp/fwprog/.AmbaVer.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly  -I/home/bingo/RD_A12/release_rtos/ssp/fwprog -Issp/fwprog -mlittle-endian -DAMBA_KAL_NO_SMP -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-format-security -fno-delete-null-pointer-checks -fshort-enums -fshort-wchar -D"SOC=KBUILD_STR(A12)" -O2 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -ffunction-sections -fdata-sections -Wno-missing-braces -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -gdwarf-3 -Wdeclaration-after-statement -Wpointer-sign -fno-strict-overflow -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -std=c99 -DCC_HAVE_ASM_GOTO -DAMBA_KAL_NO_SMP  -I/home/bingo/RD_A12/release_rtos/ssp/fwprog -Issp/fwprog   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/misc -DAMBA_BUILD_DATE=\"""\" -DAMBA_BUILD_DATE_NUM=0 -DAMBA_BUILD_SEC_NUM=0 -DAMBA_BUILD_MACHINE=\""bingo@bingo-All-Series:[gcc 4.9.3]"\" -DAMBA_CI_COUNT=0 -DAMBA_CI_ID=\""local"\"    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(AmbaVer)"  -D"KBUILD_MODNAME=KBUILD_STR(libfwprog)" -c -o ssp/fwprog/AmbaVer.o /home/bingo/RD_A12/release_rtos/ssp/fwprog/AmbaVer.c

source_ssp/fwprog/AmbaVer.o := /home/bingo/RD_A12/release_rtos/ssp/fwprog/AmbaVer.c

deps_ssp/fwprog/AmbaVer.o := \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaDataType.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/misc/AmbaVer.h \

ssp/fwprog/AmbaVer.o: $(deps_ssp/fwprog/AmbaVer.o)

$(deps_ssp/fwprog/AmbaVer.o):
