obj-y += libpba.a

# Link script
obj-y += amba_pba.lds

ccflags-y := -I$(srctree)/vendors/ambarella/inc/ssp
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/soc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/threadx
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/dsp
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/fs
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/prfile2
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaSys
#ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaTest
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaShell
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/FwUpdate
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/misc
ccflags-y += -I$(srctree)/peripheral/sensor/inc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/soc/non_os

libpba-objs += AmbaSSPSystemInfo.o
libpba-objs += AmbaBuffers.o
libpba-objs += AmbaUserSysCtrl.o


ifeq ($(CONFIG_ENABLE_EMMC_BOOT), y) 
libpba-objs += AmbaFirmwareUpdater_EMMC.o
else ifeq ($(CONFIG_ENABLE_SPINOR_BOOT), y) 
libpba-objs += AmbaFirmwareUpdaterNOR.o
else
libpba-objs += AmbaFirmwareUpdater.o
endif


$(obj)/AmbaVerSetSspUnitTest.o	:	$(call amba_ver_dep,AmbaVerSetSspUnitTest.o,$(libpba-objs))
CFLAGS_AmbaVerSetSspUnitTest.o =      -DAMBA_BUILD_DATE=$(AMBA_BUILD_DATE)            \
                                      -DAMBA_BUILD_DATE_NUM=$(AMBA_BUILD_DATE_NUM)    \
                                      -DAMBA_BUILD_SEC_NUM=$(AMBA_BUILD_SEC_NUM)      \
                                      -DAMBA_BUILD_MACHINE=$(AMBA_BUILD_MACHINE)      \
                                      -DAMBA_CI_COUNT=$(AMBA_CI_COUNT)                \
                                      -DAMBA_CI_ID=$(AMBA_CI_ID)


install-files := libpba.a
