APP = kfc

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += *.png $(ASSETS).lua

include $(SDK_DIR)/Makefile.rules

run : $(APP).elf
	siftulator $(APP).elf
