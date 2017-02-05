obj-y += libapp.a
# Link script

ccflags-y := -I$(srctree)/vendors/ambarella/inc/ssp
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/link
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/link/rpcprog
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/soc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/threadx
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/rpcprog
ccflags-y += -I$(srctree)/vendors/foss/gps
ccflags-y += -I$(srctree)/peripheral/misc/inc
ccflags-y += -I$(srctree)/peripheral/sensor/inc
ccflags-y += -I$(srctree)/peripheral/lcd/inc
ccflags-y += -I$(srctree)/peripheral/imu/inc
ccflags-y += -I$(srctree)/peripheral/audio/inc
ccflags-y += -I$(srctree)/peripheral/yuvinput/inc
ccflags-y += -I$(srctree)/peripheral/gps/inc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaSys
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaShell
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaTest
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/misc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/soc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/dsp
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/fs
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/prfile2
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/threadx
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/rpcprog
ccflags-y += -I$(srctree)/vendors/ambarella/inc/mw
ccflags-y += -I$(srctree)/vendors/ambarella/inc/mw/net/rpcprog
ccflags-y += -I$(srctree)/vendors/ambarella/inc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image
ccflags-y += -I$(srctree)/app/connected/applib/inc
ccflags-y += -I$(srctree)/app/connected/app


libapp-objs := AmbaVer_SetApp.o

CFLAGS_AmbaVer_SetApp.o = -DAMBA_BUILD_DATE=$(AMBA_BUILD_DATE)            \
                  -DAMBA_BUILD_DATE_NUM=$(AMBA_BUILD_DATE_NUM)    \
                  -DAMBA_BUILD_SEC_NUM=$(AMBA_BUILD_SEC_NUM)      \
                  -DAMBA_BUILD_MACHINE=$(AMBA_BUILD_MACHINE)      \
                  -DAMBA_CI_COUNT=$(AMBA_CI_COUNT_N1)                \
                  -DAMBA_CI_ID=$(AMBA_CI_ID_N1)

include $(srctree)/app/connected/app/apps/Makefile
include $(srctree)/app/connected/app/peripheral_mod/Makefile
include $(srctree)/app/connected/app/framework/Makefile
include $(srctree)/app/connected/app/system/Makefile
include $(srctree)/app/connected/app/thirdparty/Makefile

install-files := libapp.a

