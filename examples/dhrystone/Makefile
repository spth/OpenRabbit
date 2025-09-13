SDCC=sdcc
SDCCFLAGS=-mr2k --data-loc 0xa000 --opt-code-speed -I..

TARGETS = RCM2020 RCM2200 RCM3209 RCM3319 RCM3750 RCM4110

DHRYSTONES = $(foreach target, $(TARGETS), dhrystone-$(target).bin) $(foreach target, $(TARGETS), dhrystone-$(target).ihx)

# Default: Build Dhrystone as binary and Intel hex for all target Rabbits.
all: $(DHRYSTONES)

dhrystone-%.bin: dhrystone-%.ihx
	objcopy --input-target=ihex --output-target=binary $< $@

dhrystone-%.ihx: dhry_1.rel dhry_2.rel portme-%.rel
	$(SDCC) $(SDCCFLAGS) $^ -o $@

portme-%.rel: portme.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) -D$* $< -o $@

%.rel: %.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) $<

clean:
	rm -rf *.rel *.map *.lst *.noi *.lk *.lnk *.sym *.asm *.ihx *.o *.rst *.bin

