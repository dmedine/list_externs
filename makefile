LINUXINCLUDE?=-I../../src
EXT?=pd_linux

.PHONY: listtool listorder finish

all: listtool listorder finish

listtool:
	$(MAKE) -C listtool
	
listorder:
	$(MAKE) -C listorder
	
finish:
	mv listorder/listorder.$(EXT) ./
	mv listtool/listtool.$(EXT) ./	
