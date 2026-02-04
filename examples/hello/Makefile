SDCC = sdcc
SDCCFLAGS = -mr2k -I.. --data-loc 0xa000

TARGETS = RCM2000 RCM2020 RCM2200 RCM2210 RCM3010 RCM3110 RCM3209 RCM3310 RCM3319 RCM3710 RCM3720 RCM3750 RCM4110 RCM4210 RCM5700

HELLO = $(foreach target, $(TARGETS), hello-$(target).bin) $(foreach target, $(TARGETS), hello-$(target).ihx)

.PHONY: all clean

all: $(HELLO)

clean:
	rm -f *.asm *.bin *.ihx *.lk *.lst *.map *.noi *.rel *.sym

hello-%.bin: hello-%.ihx
	objcopy --input-target=ihex --output-target=binary $< $@

hello-%.ihx: hello-%.rel
	$(SDCC) $(SDCCFLAGS) $^ -o $@

hello-%.rel: hello.c ../targetconfigurations.h
	$(SDCC) $(SDCCFLAGS) -D$* -c $< -o $@

