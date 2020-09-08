CXX = g++
SDCC = sdcc
ASRAB = sdasrab

.PHONY: all clean

all: coldload.bin

clean:
	rm -f *.rel *.ihx *.noi *.map *.lk coldload.no3.bin

makecold: makecold.cpp
	$(CXX) makecold.cpp -o $@

coldload.bin: coldload.no3.bin makecold
	rm -f coldload.bin
	./makecold coldload.no3.bin $@
	
coldload.no3.bin: coldload.ihx
	objcopy --input-target=ihex --output-target=binary coldload.ihx $@
	
coldload.ihx: coldload.rel
	$(SDCC) -mr2k --no-std-crt0 --code-loc 0x0 coldload.rel || true
	
coldload.rel: coldload.s
	$(ASRAB) -o coldload.s

# TODO: Modify pilot.c to make it actually compileable with SDCC
pilot.bin: pilot.c
	$(SDCC) -mr2k --no-std-crt0 pilot.c

