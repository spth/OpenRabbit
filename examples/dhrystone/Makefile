SDCC=sdcc
SDCCFLAGS=-mr2k --opt-code-speed -I..

all: image.ihx image.bin

image.bin: image.ihx
	objcopy --input-target=ihex --output-target=binary image.ihx image.bin

image.ihx: ../hello/crt0.rel ../hello/target.rel dhry_1.rel dhry_2.rel portme.rel
	$(SDCC) $(SDCCFLAGS) --no-std-crt0 ../hello/crt0.rel ../hello/target.rel *.rel -o image.ihx

%.rel: %.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) $<

clean:
	rm -rf *.rel *.map *.lst *.noi *.lk *.lnk *.sym *.asm *.ihx *.o *.rst *.bin

