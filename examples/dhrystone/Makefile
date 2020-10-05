SDCC=sdcc
SDCCFLAGS=-mr2k --data-loc 0xa000 --opt-code-speed -I..

TARGETS = RCM2200 RCM3209

DHRYSTONES = $(foreach target, $(TARGETS), dhrystone-$(target).bin)

all: $(DHRYSTONES)

dhrystone-%.bin: dhrystone-%.ihx
	objcopy --input-target=ihex --output-target=binary $< $@

dhrystone-%.ihx: crt0-generic.rel dhry_1.rel dhry_2.rel portme-%.rel
	$(SDCC) $(SDCCFLAGS) --no-std-crt0 $^ -o $@

portme-%.rel: portme.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) -D$* $< -o $@

%.rel: %.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) $<

crt0-generic.rel: ../crt0-generic.s
	sdasrab -plosgff $<
	mv ../crt0-generic.rel $@

clean:
	rm -rf *.rel *.map *.lst *.noi *.lk *.lnk *.sym *.asm *.ihx *.o *.rst *.bin

