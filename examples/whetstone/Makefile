SDCC=sdcc
SDCCFLAGS=-mr2k --data-loc 0xa000 --opt-code-speed -DRCM2200 -I..

TARGETS = RCM2200 RCM2020 RCM3319 RCM3750 RCM4110 #RCM3209 - todo: add MB2CR_VALUE in targetconfigurations.h, then enable here.

WHETSTONES = $(foreach target, $(TARGETS), whetstone-$(target).bin)

all: $(WHETSTONES)

whetstone-%.bin: whetstone-%.ihx
	objcopy --input-target=ihex --output-target=binary $< $@

whetstone-%.ihx: ../crt0-generic.rel whetstone.rel portme-%.rel
	$(SDCC) $(SDCCFLAGS) --no-std-crt0 $^ -o $@

portme-%.rel: portme.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) -D$* $< -o $@

%.rel: %.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) $<

../crt0-generic.rel: ../crt0-generic.s
	sdasrab -plosgff $<

clean:
	rm -rf *.rel *.map *.lst *.noi *.lk *.lnk *.sym *.asm *.ihx *.o *.rst *.bin

