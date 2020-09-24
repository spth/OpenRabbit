SDCC=sdcc
SDCCFLAGS=-mr2k --opt-code-speed -I..

all: image.ihx image.bin

image.bin: image.ihx
	objcopy --input-target=ihex --output-target=binary image.ihx image.bin

image.ihx: crt0-rcm2200.rel whetstone.rel portme.rel
	$(SDCC) $(SDCCFLAGS) --no-std-crt0 *.rel -o image.ihx

%.rel: %.c
	$(SDCC) --fverbose-asm -c $(SDCCFLAGS) $<

crt0-%.rel: ../crt0-noint.s ../targetconfigurations/%/targetstartup.s
	sdasrab -plosgff -I../targetconfigurations/$* $<
	mv ../crt0-noint.rel $@

clean:
	rm -rf *.rel *.map *.lst *.noi *.lk *.lnk *.sym *.asm *.ihx *.o *.rst *.bin

