cmd_peripheral/nand/AmbaCortexNAND_S34MLxG1.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -Wp,-MD,peripheral/nand/.AmbaCortexNAND_S34MLxG1.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly -mlittle-endian -DAMBA_KAL_NO_SMP -D__ASSEMBLY__ -D__ASM__ -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -Wa,--gdwarf-2 -DAMBA_KAL_NO_SMP    -x assembler-with-cpp -c -o peripheral/nand/AmbaCortexNAND_S34MLxG1.o /home/bingo/RD_A12/release_rtos/peripheral/nand/AmbaCortexNAND_S34MLxG1.asm

source_peripheral/nand/AmbaCortexNAND_S34MLxG1.o := /home/bingo/RD_A12/release_rtos/peripheral/nand/AmbaCortexNAND_S34MLxG1.asm

deps_peripheral/nand/AmbaCortexNAND_S34MLxG1.o := \
    $(wildcard build/include/config/enable/spinor/boot.h) \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \

peripheral/nand/AmbaCortexNAND_S34MLxG1.o: $(deps_peripheral/nand/AmbaCortexNAND_S34MLxG1.o)

$(deps_peripheral/nand/AmbaCortexNAND_S34MLxG1.o):
