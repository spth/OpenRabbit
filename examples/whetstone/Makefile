SDCC=sdcc
SDCCFLAGS=-mr2k --data-loc 0xa000 --opt-code-speed -DRCM2200 -I..

all: image.ihx image.bin

image.bin: image.ihx
	objcopy --input-target=ihex --output-target=binary image.ihx image.bin

image.ihx: crt0-generic.rel whetstone.rel portme.rel
	$(SDCC) $(SDCCFLAGS) --no-std-crt0 *.rel -o image.ihx

%.rel: %.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) $<

crt0-generic.rel: ../crt0-generic.s
	sdasrab -plosgff $<
	mv ../crt0-generic.rel $@

clean:
	rm -rf *.rel *.map *.lst *.noi *.lk *.lnk *.sym *.asm *.ihx *.o *.rst *.bin

