data_DATA = coldload.bin
dist_data_DATA = pilot.bin
noinst_PROGRAMS = makecold

makecold_SOURCES = makecold.cpp

EXTRA_DIST = coldload.s

MOSTLYCLEANFILES = *.rel *.lk *.noi *.map *.ihx coldload.no3.bin coldload.bin

coldload.bin: coldload.no3.bin makecold
	rm -f coldload.bin
	./makecold coldload.no3.bin $@

coldload.no3.bin: coldload.ihx
	objcopy --input-target=ihex --output-target=binary $< $@

coldload.ihx: coldload.rel
	$(SDCC) -mr2k --no-std-crt0 --code-loc 0x0 $< || true
	
coldload.rel: coldload.s
	$(ASRAB) -o $@ $<

# TODO: Modify pilot.c to make it actually compileable with SDCC
#pilot.bin: pilot.c
#	$(SDCC) -mr2k --no-std-crt0 --code-loc 0x6000 --data-loc 0x7fff pilot.c

