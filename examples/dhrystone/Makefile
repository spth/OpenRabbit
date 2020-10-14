SDCC=sdcc
SDCCFLAGS=-mr2k --data-loc 0xa000 --opt-code-speed -I..

TARGETS = RCM2200 RCM2020 RCM3319 RCM3750 RCM4110 #RCM3209 - todo: add MB2CR_VALUE in targetconfigurations.h, then enable here.

DHRYSTONES = $(foreach target, $(TARGETS), dhrystone-$(target).bin)

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

