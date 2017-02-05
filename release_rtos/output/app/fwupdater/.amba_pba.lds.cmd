cmd_app/fwupdater/amba_pba.lds := /usr/local/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-gcc -E -Wp,-MD,app/fwupdater/.amba_pba.lds.d   -I/home/bingo/RD_A12/release_rtos/build/include -Ibuild/include -include /home/bingo/RD_A12/release_rtos/build/include/kconfig.h -I/home/bingo/RD_A12/release_rtos/bsp/dragonfly -mlittle-endian -DAMBA_KAL_NO_SMP -P -C -Uarm -D__ASSEMBLY__ -DLINKER_SCRIPT -o app/fwupdater/amba_pba.lds /home/bingo/RD_A12/release_rtos/app/fwupdater/amba_pba.lds.S

source_app/fwupdater/amba_pba.lds := /home/bingo/RD_A12/release_rtos/app/fwupdater/amba_pba.lds.S

deps_app/fwupdater/amba_pba.lds := \
    $(wildcard build/include/config/ambalink/mem/size.h) \
    $(wildcard build/include/config/ddr/size.h) \
  /home/bingo/RD_A12/release_rtos/build/include/kconfig.h \
    $(wildcard build/include/config/h.h) \
    $(wildcard build/include/config/.h) \
    $(wildcard build/include/config/booger.h) \
    $(wildcard build/include/config/foo.h) \

app/fwupdater/amba_pba.lds: $(deps_app/fwupdater/amba_pba.lds)

$(deps_app/fwupdater/amba_pba.lds):
