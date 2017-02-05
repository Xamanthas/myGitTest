obj-y += libopencv_hal.a

ccflags-y := -I$(srctree)/vendors/ambarella/inc/ssp
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/link
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/soc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/threadx
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/rpcprog
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/dsp
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/fs
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/prfile
ccflags-y += -I$(srctree)/vendors/ambarella/inc/ssp/prfile2
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaSys
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaShell
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/AmbaTest
ccflags-y += -I$(srctree)/vendors/ambarella/inc/comsvc/misc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/mw
ccflags-y += -I$(srctree)/vendors/ambarella/inc/mw/net/rpcprog
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image/ituner
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image/imgschdlr
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image/imgproc
ccflags-y += -I$(srctree)/vendors/ambarella/inc/image/imgcalib
ccflags-y += -I$(srctree)/vendors/foss/opencv/inc
ccflags-y += -I$(srctree)/vendors/foss/opencv/inc/opencv
ccflags-y += -I$(srctree)/vendors/foss/opencv/inc/opencv2
ccflags-y += -I$(srctree)/vendors/ambarella/inc/va
ccflags-y += -I$(srctree)/vendors/ambarella/inc
ccflags-y += -I$(srctree)/peripheral/sensor/inc
ccflags-y += -I$(srctree)/peripheral/lcd/inc
ccflags-y += -I$(srctree)/peripheral/audio/inc
ccflags-y += -I$(srctree)/peripheral/inc/misc
ccflags-y += -I$(src)/extSrc/tune
ccflags-y += -I$(src)/extSrc/sample_calib
ccflags-y += -I$(src)/extSrc/sample_iqparam_handler
ccflags-y += -std=gnu99
ccflags-y += -fshort-wchar
ccflags-y += -frtti
ccflags-y += -fexceptions


ccflags-y += -I$(src)/../include
ccflags-y += -I$(src)/..
#cvconfig.h
ccflags-y += -I$(src)/../../..
#opencv2/core/opencl/ocl_defs
ccflags-y += -I$(src)/../../core/include

ccflags-y += -DOPENCV_NOSTL
#ccflags-y += -mfloat-abi=softfp
#ccflags-y += -mfpu=neon
#ccflags-y += -DENABLE_NEON=ON
#pass build check
ccflags-y += -D__OPENCV_BUILD
ccflags-y += -include opencv2/hal.hpp


libopencv_hal-objs += 	arithm.o \
			color.o \
			filter.o \
			mathfuncs.o \
			matrix.o \
			resize.o \
			stat.o \
			warp.o

#command_line_parser.o

install-files := libopencv_hal.a

