SDCC = sdcc
SDCCFLAGS = -mr2k -I.. --data-loc 0xa000

TARGETS = RCM2020 RCM2200 RCM3110 RCM3209 RCM3319 RCM3750 RCM4110

HELLO = $(foreach target, $(TARGETS), hello-$(target).bin)

.PHONY: all clean

all: $(HELLO)

clean:
	rm -f *.asm *.bin *.ihx *.lk *.lst *.map *.noi *.rel *.sym

hello-%.bin: hello-%.ihx
	objcopy --input-target=ihex --output-target=binary $< $@

hello-%.ihx: hello-%.rel
	$(SDCC) $(SDCCFLAGS) $^ -o $@

hello-%.rel: hello.c
	$(SDCC) $(SDCCFLAGS) -D$* -c $< -o $@

