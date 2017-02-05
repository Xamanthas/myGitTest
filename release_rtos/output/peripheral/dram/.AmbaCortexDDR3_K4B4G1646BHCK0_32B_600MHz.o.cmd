cmd_peripheral/dram/AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.o := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -Wp,-MD,peripheral/dram/.AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.o.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly -mlittle-endian -DAMBA_KAL_NO_SMP -D__ASSEMBLY__ -D__ASM__ -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfpv3 -marm -march=armv7-a -mcpu=cortex-a9 -mhard-float -Wa,--gdwarf-2 -DAMBA_KAL_NO_SMP    -x assembler-with-cpp -c -o peripheral/dram/AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.o /home/bingo/RD_A12/release_rtos/peripheral/dram/AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.asm

source_peripheral/dram/AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.o := /home/bingo/RD_A12/release_rtos/peripheral/dram/AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.asm

deps_peripheral/dram/AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.o := \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \

peripheral/dram/AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.o: $(deps_peripheral/dram/AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.o)

$(deps_peripheral/dram/AmbaCortexDDR3_K4B4G1646BHCK0_32B_600MHz.o):
