cmd_peripheral/audio/AmbaAudio_TLV320AIC3256.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -Wp,-MD,peripheral/audio/.AmbaAudio_TLV320AIC3256.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly  -I/home/bingo/RD_A12/release_rtos/peripheral -Iperipheral -mlittle-endian -DAMBA_KAL_NO_SMP -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-format-security -fno-delete-null-pointer-checks -fshort-enums -fshort-wchar -D"SOC=KBUILD_STR(A12)" -O2 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -ffunction-sections -fdata-sections -Wno-missing-braces -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -gdwarf-3 -Wdeclaration-after-statement -Wpointer-sign -fno-strict-overflow -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -std=c99 -DCC_HAVE_ASM_GOTO -DAMBA_KAL_NO_SMP  -I/home/bingo/RD_A12/release_rtos/peripheral/sbrg/inc -Iperipheral/sbrg/inc  -I/home/bingo/RD_A12/release_rtos/peripheral/sensor/inc -Iperipheral/sensor/inc  -I/home/bingo/RD_A12/release_rtos/peripheral/sbrg/b5/inc -Iperipheral/sbrg/b5/inc  -I/home/bingo/RD_A12/release_rtos/peripheral/yuvinput/inc -Iperipheral/yuvinput/inc  -I/home/bingo/RD_A12/release_rtos/peripheral/audio/inc -Iperipheral/audio/inc  -I/home/bingo/RD_A12/release_rtos/peripheral/lcd/inc -Iperipheral/lcd/inc  -I/home/bingo/RD_A12/release_rtos/peripheral/imu/inc -Iperipheral/imu/inc  -I/home/bingo/RD_A12/release_rtos/peripheral/misc/inc -Iperipheral/misc/inc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/misc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaTest   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaShell    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(AmbaAudio_TLV320AIC3256)"  -D"KBUILD_MODNAME=KBUILD_STR(libperipheral)" -c -o peripheral/audio/AmbaAudio_TLV320AIC3256.o /home/bingo/RD_A12/release_rtos/peripheral/audio/AmbaAudio_TLV320AIC3256.c

source_peripheral/audio/AmbaAudio_TLV320AIC3256.o := /home/bingo/RD_A12/release_rtos/peripheral/audio/AmbaAudio_TLV320AIC3256.c

deps_peripheral/audio/AmbaAudio_TLV320AIC3256.o := \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaDataType.h \
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
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaI2C.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaI2C_Def.h \
  /home/bingo/RD_A12/release_rtos/peripheral/audio/inc/AmbaAudio_TLV320AIC3256.h \
  /home/bingo/RD_A12/release_rtos/peripheral/audio/inc/AmbaAudio_CODEC.h \

peripheral/audio/AmbaAudio_TLV320AIC3256.o: $(deps_peripheral/audio/AmbaAudio_TLV320AIC3256.o)

$(deps_peripheral/audio/AmbaAudio_TLV320AIC3256.o):
