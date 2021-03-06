APP = kfc

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += *.png $(ASSETS).lua

include $(SDK_DIR)/Makefile.rules

build : $(APP).elf

sim : $(APP).elf
	siftulator -n 6 $(APP).elf

push :
	git push origin master
