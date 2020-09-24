SDCC = sdcc
SDCCFLAGS = -mr2k -I..

TARGETS = rcm2200 rcm3209

HELLO = $(foreach target, $(TARGETS), hello-$(target).bin)

.PHONY: all clean

all: $(HELLO)

clean:
	rm -f *.asm *.bin *.ihx *.lk *.lst *.map *.noi *.rel *.sym

hello-%.bin: hello-%.ihx
	objcopy --input-target=ihex --output-target=binary $< $@

hello-%.ihx: crt0-%.rel hello-%.rel
	$(SDCC) $(SDCCFLAGS) --no-std-crt0 $^ -o $@

hello-%.rel: hello.c ../targetconfigurations/%/targetconfig.h
	$(SDCC) $(SDCCFLAGS) -I../targetconfigurations/$* -c $< -o $@

crt0-%.rel: ../crt0-noint.s ../targetconfigurations/%/targetstartup.s
	sdasrab -plosgff -I../targetconfigurations/$* $<
	mv ../crt0-noint.rel $@

