#include <stdint.h>
#include <stdio.h>

#include "r2k.h"

// Todo: Omit clock doubling to simplify example?
#if defined(RCM2200)
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // clock doubler for 11.0592 MHz base
#elif defined(RCM3209)
#define SERIAL_DIVIDER_38400 36
#define CLOCK_DOUBLER 0x03
#endif

// _sdcc_external_startup, if present, will be called very early, before initalization
// of global objects. This makes it e.g. useful for dealing with watchdogs that might
// otherwise bite if there are many or large global objects that take a long time to initialize.
void _sdcc_external_startup(void)
{
	// Disable watchdog
	WDTTR = 0x51;
	WDTTR = 0x54;

	// normal oscillator, processor and peripheral from main clock, no periodic interrupt
	GCSR = 0x08;

	GCDR = CLOCK_DOUBLER;
}

int putchar(int c)
{
	// Convert newline to CRLF
	if (c == '\n') {
		putchar('\r');
	}

	while (SASR & 0x04);	// Wait for empty transmitter data register
	SADR = c;
	return c;
}

void main(void)
{
	PCFR = 0x40;	// Use pin PC6 as TXA

	TAT4R = SERIAL_DIVIDER_38400 - 1;	// Value in register is one less than the divider used (e.g. a value of 0 will result in clock division by 1).
	TACSR = 0x01;	// Enable timer A

	SACR = 0x00;	// No interrupts, 8-bit async mode

	for (;;) {
		printf("Hello, world!\n");
	}
}

