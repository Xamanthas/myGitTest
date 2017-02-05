cmd_app/fwupdater/AmbaUserSysCtrl.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -Wp,-MD,app/fwupdater/.AmbaUserSysCtrl.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly  -I/home/bingo/RD_A12/release_rtos/app/fwupdater -Iapp/fwupdater -mlittle-endian -DAMBA_KAL_NO_SMP -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-format-security -fno-delete-null-pointer-checks -fshort-enums -fshort-wchar -D"SOC=KBUILD_STR(A12)" -O2 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -ffunction-sections -fdata-sections -Wno-missing-braces -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -gdwarf-3 -Wdeclaration-after-statement -Wpointer-sign -fno-strict-overflow -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -std=c99 -DCC_HAVE_ASM_GOTO -DAMBA_KAL_NO_SMP   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/fs   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaShell   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/FwUpdate   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/misc   -I/home/bingo/RD_A12/release_rtos/peripheral/sensor/inc   -I/home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/non_os    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(AmbaUserSysCtrl)"  -D"KBUILD_MODNAME=KBUILD_STR(libpba)" -c -o app/fwupdater/AmbaUserSysCtrl.o /home/bingo/RD_A12/release_rtos/app/fwupdater/AmbaUserSysCtrl.c

source_app/fwupdater/AmbaUserSysCtrl.o := /home/bingo/RD_A12/release_rtos/app/fwupdater/AmbaUserSysCtrl.c

deps_app/fwupdater/AmbaUserSysCtrl.o := \
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
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaRTSL_GPIO.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaGPIO_Def.h \
    $(wildcard build/include/config/e/.h) \
    $(wildcard build/include/config/e.h) \
    $(wildcard build/include/config/input.h) \
    $(wildcard build/include/config/output.h) \
    $(wildcard build/include/config/alternate.h) \
    $(wildcard build/include/config/s/.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaSysCtrl.h \
  /home/bingo/RD_A12/release_rtos/app/fwupdater/AmbaBuffers.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaKAL.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_api_Uniprocessor.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_port_Uniprocessor.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_user.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/stdlib.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/machine/stdlib.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/threadx/tx_execution_profile.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/AmbaAudio.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys/AmbaPrint.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaShell/AmbaShell.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaI2C.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaI2C_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaGPIO.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaUART.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaUART_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaSD.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaSD_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNAND.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNAND_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaPartition_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNOR.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNOR_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaFIO.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaROM.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaCardManager.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaNFTL.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaPLL_Def.h \
    $(wildcard build/include/config/u/.h) \
    $(wildcard build/include/config/u.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaPLL.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaPLL_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/misc/AmbaVer.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/misc/AmbaMonitor_SD.h \
  /home/bingo/RD_A12/release_rtos/app/fwupdater/AmbaSSPSystemInfo.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_Event.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_EventInfo.h \
    $(wildcard build/include/config/info/s.h) \
    $(wildcard build/include/config/info/s/.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_EventInfo.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP_VideoEnc.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/dsp/AmbaDSP.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/fs/AmbaFS.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/fs/AmbaFS_Def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/pf_w_apicmn.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/pf_apicmn.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/pf_apicmn_def.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/pf_config.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/parts.cfg \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/hardcfg.h \
    $(wildcard build/include/config/enable/exfat/support.h) \
    $(wildcard build/include/config/pf/character/code/set/cp932.h) \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/pf_cfg_p.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/pf_exfat_config.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/pf_types.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/e_types.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/inttypes.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/sys/_intsup.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/lib/gcc/arm-none-eabi/4.9.3/include/stdint.h \
  /usr/local/gcc-arm-none-eabi-4_9-2015q1/arm-none-eabi/include/stdint.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/hardcfg.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/prfile2/pf_exfat_apicmn.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaCache.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/comsvc/AmbaSys/AmbaUtility.h \
  /home/bingo/RD_A12/release_rtos/peripheral/sensor/inc/AmbaSensor.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaVIN.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaVIN_Def.h \
  /home/bingo/RD_A12/release_rtos/bsp/dragonfly/bsp.h \
    $(wildcard build/include/config/build/amba/mw/unittest.h) \
  /home/bingo/RD_A12/release_rtos/peripheral/sensor/inc/AmbaSensor_IMX117.h \
  /home/bingo/RD_A12/release_rtos/vendors/ambarella/inc/ssp/soc/AmbaRTSL_MMU_Def.h \

app/fwupdater/AmbaUserSysCtrl.o: $(deps_app/fwupdater/AmbaUserSysCtrl.o)

$(deps_app/fwupdater/AmbaUserSysCtrl.o):
