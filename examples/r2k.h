#ifndef __R2K_H__
#define __R2K_H__ 1

#define _REG(addr, reg)	__sfr __at(addr) reg

_REG(0x00, GCSR);  // global control / status register
_REG(0x01, RTCCR); // real time clock control register
_REG(0x02, RTC0R); // real time clock register 0
_REG(0x03, RTC1R); // real time clock register 1
_REG(0x04, RTC2R); // real time clock register 2
_REG(0x05, RTC3R); // real time clock register 3
_REG(0x06, RTC4R); // real time clock register 4
_REG(0x07, RTC5R); // real time clock register 5
_REG(0x08, WDTCR); // watch-dog timer control register
_REG(0x09, WDTTR); // watch-dog timer test register
_REG(0x0A, GCM0R); // global clock modulator register 0
_REG(0x0B, GCM1R); // global clock modulator register 1
_REG(0x0D, GPSCR); // global power save control register
_REG(0x0E, GOCR);  // global output control register
_REG(0x0F, GCDR);  // global clock double register
_REG(0x10, MMIDR);
_REG(0x11, STACKSEG);
_REG(0x12, DATASEG);
_REG(0x13, SEGSIZE);
_REG(0x14, MB0CR); // Memory Bank 0 Control Register
_REG(0x15, MB1CR); // Memory Bank 1 Control Register
_REG(0x16, MB2CR); // Memory Bank 2 Control Register
_REG(0x17, MB3CR); // Memory Bank 3 Control Register

_REG(0x48, PGDR);
_REG(0x4C, PGCR);
_REG(0x4D, PGFR);
_REG(0x4E, PGDCR);
_REG(0x4F, PGDDR);

_REG(0x50, PCDR);  // Port E data register
_REG(0x55, PCFR);  // Port C function register

_REG(0x70, PEDR);  // Port E data register
_REG(0x74, PECR);  // Port E control register
_REG(0x77, PEDDR); // Port E data direction register

_REG(0xA0, TACSR); // Timer A Control/Status Register
_REG(0xA9, TAT4R); // Timer A Time Constant 4 Register

_REG(0xC0, SADR);  // Serial Port A Data Register
_REG(0xC3, SASR);  // Serial Port A Status Register
_REG(0xC4, SACR);  // Serial Port A Control Register

#undef _REG

#endif

