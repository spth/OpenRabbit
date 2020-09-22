.PHONY: clean

all: hello.bin

clean:
	rm -f *.rel *.ihx *.bin

hello.bin: hello.ihx
	objcopy --input-target=ihex --output-target=binary hello.ihx hello.bin

hello.ihx: crt0.rel hello.rel
	sdcc -mr2k --no-std-crt0 crt0.rel hello.rel -o hello.ihx

hello.rel: hello.c
	sdcc -mr2k -c hello.c -o hello.rel

crt0.rel: crt0.s
	sdasrab -plosgff crt0.s

