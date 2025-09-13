SDCC=sdcc
SDCCFLAGS=-mr2k --data-loc 0xa000 --opt-code-speed -I..

TARGETS = RCM2020 RCM2200 RCM3209 RCM3319 RCM3750 RCM4110

WHETSTONES = $(foreach target, $(TARGETS), whetstone-$(target).bin)

all: $(WHETSTONES)

whetstone-%.bin: whetstone-%.ihx
	objcopy --input-target=ihex --output-target=binary $< $@

whetstone-%.ihx: whetstone.rel portme-%.rel
	$(SDCC) $(SDCCFLAGS) $^ -o $@

portme-%.rel: portme.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) -D$* $< -o $@

%.rel: %.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) $<

clean:
	rm -rf *.rel *.map *.lst *.noi *.lk *.lnk *.sym *.asm *.ihx *.o *.rst *.bin

