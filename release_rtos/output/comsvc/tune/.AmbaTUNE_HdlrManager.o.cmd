cmd_comsvc/tune/AmbaTUNE_HdlrManager.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -Wp,-MD,comsvc/tune/.AmbaTUNE_HdlrManager.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly  -I/home/bingo/RD_A12/release_rtos/comsvc/tune -Icomsvc/tune -mlittle-endian -DAMBA_KAL_NO_SMP -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-format-security -fno-delete-null-pointer-checks -fshort-enums -fshort-wchar -D"SOC=KBUILD_STR(A12)" -O2 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -ffunction-sections -fdata-sections -Wno-missing-braces -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -gdwarf-3 -Wdeclaration-after-statement -Wpointer-sign -fno-strict-overflow -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -std=c99 -DCC_HAVE_ASM_GOTO -DAMBA_KAL_NO_SMP   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/link   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/misc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/image/ituner   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/fs   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/usb   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/tune   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/image/imgproc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/image/imgschdlr   -I/home/bingo/RD_A12/release_rtos/peripheral/sensor/inc    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(AmbaTUNE_HdlrManager)"  -D"KBUILD_MODNAME=KBUILD_STR(libcomsvc_tune)" -c -o comsvc/tune/AmbaTUNE_HdlrManager.o /home/bingo/RD_A12/release_rtos/comsvc/tune/AmbaTUNE_HdlrManager.c

source_comsvc/tune/AmbaTUNE_HdlrManager.o := /home/bingo/RD_A12/release_rtos/comsvc/tune/AmbaTUNE_HdlrManager.c

deps_comsvc/tune/AmbaTUNE_HdlrManager.o := \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \
  /home/bingo/RD_A12/release_rtos/comsvc/tune/AmbaTUNE_TextHdlr.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaDataType.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_ImgDef.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/image/ituner/AmbaImgCalibItuner.h \
    $(wildcard build/include/config/s.h) \
    $(wildcard build/include/config/video/channel/num.h) \
    $(wildcard build/include/config/video/slice/num.h) \
    $(wildcard build/include/config/still/slice/num.h) \
    $(wildcard build/include/config/info/s/.h) \
    $(wildcard build/include/config/info/s.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP.h \
    $(wildcard build/include/config/s/.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_Img3aStatistics.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_ImgDef.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_EventInfo.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_ImgFilter.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_ImgUtility.h \
    $(wildcard build/include/config/execute/mode/e/.h) \
    $(wildcard build/include/config/execute/mode/e.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaKAL.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_api_Uniprocessor.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_port_Uniprocessor.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_user.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/stdlib.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/ieeefp.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/_ansi.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/newlib.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/config.h \
    $(wildcard build/include/config/h//.h) \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/features.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/lib/gcc/arm-none-eabi/4.9.3/include/stddef.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/reent.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/_ansi.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/_default_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/lock.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/cdefs.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/stdlib.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/string.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/string.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_execution_profile.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys/AmbaPrintk.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/lib/gcc/arm-none-eabi/4.9.3/include/stdarg.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/intrinsics.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/lib/gcc/arm-none-eabi/4.9.3/include/stdint.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/stdint.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/_intsup.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaSysTimer.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys/AmbaPrint.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/tune/AmbaTUNE_HdlrManager.h \
  /home/bingo/RD_A12/release_rtos/comsvc/tune/AmbaTUNE_USBHdlr.h \

comsvc/tune/AmbaTUNE_HdlrManager.o: $(deps_comsvc/tune/AmbaTUNE_HdlrManager.o)

$(deps_comsvc/tune/AmbaTUNE_HdlrManager.o):
