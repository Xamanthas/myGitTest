cmd_comsvc/AmbaSys/AmbaMemcpy_32to32.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -Wp,-MD,comsvc/AmbaSys/.AmbaMemcpy_32to32.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly -mlittle-endian -DAMBA_KAL_NO_SMP -D__ASSEMBLY__ -D__ASM__ -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -Wa,--gdwarf-2 -DAMBA_KAL_NO_SMP    -x assembler-with-cpp -c -o comsvc/AmbaSys/AmbaMemcpy_32to32.o /home/bingo/RD_A12/release_rtos/comsvc/AmbaSys/AmbaMemcpy_32to32.asm

source_comsvc/AmbaSys/AmbaMemcpy_32to32.o := /home/bingo/RD_A12/release_rtos/comsvc/AmbaSys/AmbaMemcpy_32to32.asm

deps_comsvc/AmbaSys/AmbaMemcpy_32to32.o := \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \

comsvc/AmbaSys/AmbaMemcpy_32to32.o: $(deps_comsvc/AmbaSys/AmbaMemcpy_32to32.o)

$(deps_comsvc/AmbaSys/AmbaMemcpy_32to32.o):
