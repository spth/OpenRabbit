SDCC=sdcc
SDCCFLAGS=-mr2k -I..

.PHONY: clean

all: hello.bin

clean:
	rm -f *.asm *.bin *.ihx *.lk *.lst *.map *.noi *.rel *.sym

hello.bin: hello.ihx
	objcopy --input-target=ihex --output-target=binary hello.ihx hello.bin

hello.ihx: crt0.rel hello.rel target.rel
	$(SDCC) $(SDCCFLAGS) --no-std-crt0 $^ -o $@

hello.rel: hello.c
	$(SDCC) $(SDCCFLAGS) -c hello.c -o hello.rel

crt0.rel: crt0.s
	sdasrab -plosgff crt0.s

target/%: target/%.c
	$(SDCC) $(SDCCFLAGS) -c $< -o target.rel

