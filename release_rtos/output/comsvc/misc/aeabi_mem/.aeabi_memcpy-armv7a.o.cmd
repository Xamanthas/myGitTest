cmd_comsvc/misc/aeabi_mem/aeabi_memcpy-armv7a.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -Wp,-MD,comsvc/misc/aeabi_mem/.aeabi_memcpy-armv7a.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly -mlittle-endian -DAMBA_KAL_NO_SMP -D__ASSEMBLY__ -D__ASM__ -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -Wa,--gdwarf-2 -DAMBA_KAL_NO_SMP   -c -o comsvc/misc/aeabi_mem/aeabi_memcpy-armv7a.o /home/bingo/RD_A12/release_rtos/comsvc/misc/aeabi_mem/aeabi_memcpy-armv7a.S

source_comsvc/misc/aeabi_mem/aeabi_memcpy-armv7a.o := /home/bingo/RD_A12/release_rtos/comsvc/misc/aeabi_mem/aeabi_memcpy-armv7a.S

deps_comsvc/misc/aeabi_mem/aeabi_memcpy-armv7a.o := \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \
  /home/bingo/RD_A12/release_rtos/comsvc/misc/aeabi_mem/arm_asm.h \
  /home/bingo/RD_A12/release_rtos/comsvc/misc/aeabi_mem/acle-compat.h \

comsvc/misc/aeabi_mem/aeabi_memcpy-armv7a.o: $(deps_comsvc/misc/aeabi_mem/aeabi_memcpy-armv7a.o)

$(deps_comsvc/misc/aeabi_mem/aeabi_memcpy-armv7a.o):
