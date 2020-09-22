__sfr __at(0x50)	PCDR;
__sfr __at(0x55)	PCFR;  // Port C function register
__sfr __at(0x70)	PEDR;
__sfr __at(0x74)	PECR;
__sfr __at(0x77)	PEDDR;
__sfr __at(0xa0)	TACSR; // Timer A Control/Status Register
__sfr __at(0xa9)	TAT4R; // Timer A Time Constant 4 Register
__sfr __at(0xc0)	SADR;  // Serial Port A Data Register
__sfr __at(0xc3)	SASR;  // Serial Port A Status Register
__sfr __at(0xc4)	SACR;  // Serial Port A Control Register

#include <stdio.h>

int putchar(int c)
{
	while(SASR & 0x40);	// Wait for empty transmitter data register
	SADR = c;
	return(c);
}

void main(void)
{
	PCDR = 0x01;
	PECR = 0x82;
	PEDDR = 0x82;
	PEDR = 0x80;
	
	PCFR = 0x40;	// Use pin PC6 as TXA

	TAT4R = 143;	// Aim for 4800 baud (assuming 22.1 MHz system clock)
	TACSR = 0x01;	// Enable timer A

	SACR = 0x00;	// No interrupts, 8-bit async mode

	for(;;)
		printf("Hello, world!\n");
}

