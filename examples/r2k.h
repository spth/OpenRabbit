#ifndef __R2K_H__
#define __R2K_H__ 1

#define _REG(addr, reg)	__sfr __at(addr) reg

_REG(0x00, GCSR);  // global control / status register

_REG(0x01, RTCCR); // Real Time Clock Control Register
_REG(0x02, RTC0R); // Real Time Clock Data Register 0
_REG(0x03, RTC1R); // Real Time Clock Data Register 1
_REG(0x04, RTC2R); // Real Time Clock Data Register 2
_REG(0x05, RTC3R); // Real Time Clock Data Register 3
_REG(0x06, RTC4R); // Real Time Clock Data Register 4
_REG(0x07, RTC5R); // Real Time Clock Data Register 5

_REG(0x08, WDTCR); // watch-dog timer control register
_REG(0x09, WDTTR); // watch-dog timer test register

_REG(0x0D, GPSCR); // global power save control register
_REG(0x0E, GOCR);  // global output control register
_REG(0x0F, GCDR);  // global clock double register

_REG(0x10, MMIDR); // MMI Instruction / Data Register
_REG(0x11, STACKSEG);
_REG(0x12, DATASEG);
_REG(0x13, SEGSIZE);

_REG(0x14, MB0CR); // Memory Bank 0 Control Register
_REG(0x15, MB1CR); // Memory Bank 1 Control Register
_REG(0x16, MB2CR); // Memory Bank 2 Control Register
_REG(0x17, MB3CR); // Memory Bank 3 Control Register

_REG(0x20, SPD0R); // Slave Port Data 0 Register
_REG(0x21, SPD0R); // Slave Port Data 1 Register
_REG(0x22, SPD0R); // Slave Port Data 2 Register
_REG(0x23, SPSR);  // Slave Port Status Register
_REG(0x24, SPCR);  // Slave Port Control Register

_REG(0x30, PADR);  // Port A Data Register
_REG(0x40, PBDR);  // Port B Data Register

_REG(0x48, PGDR);
_REG(0x4C, PGCR);
_REG(0x4D, PGFR);
_REG(0x4E, PGDCR);
_REG(0x4F, PGDDR);

_REG(0x50, PCDR);  // Port C Data Register
_REG(0x55, PCFR);  // Port C Function Register

_REG(0x60, PDDR);  // Port D Data Register
_REG(0x64, PDCR);  // Port D Control Register
_REG(0x64, PDDCR); // Port D Drive Control Register
_REG(0x67, PDDDR); // Port D Data Direction Register
_REG(0x68, PDB0R); // Port D Bit 0 Register
_REG(0x69, PDB1R); // Port D Bit 1 Register
_REG(0x6A, PDB2R); // Port D Bit 2 Register
_REG(0x6B, PDB3R); // Port D Bit 3 Register
_REG(0x6C, PDB4R); // Port D Bit 4 Register
_REG(0x6D, PDB5R); // Port D Bit 5 Register
_REG(0x6E, PDB6R); // Port D Bit 6 Register
_REG(0x6F, PDB7R); // Port D Bit 7 Register

_REG(0x70, PEDR);  // Port E data register
_REG(0x74, PECR);  // Port E control register
_REG(0x77, PEDDR); // Port E data direction register
_REG(0x78, PDB0R); // Port E Bit 0 Register
_REG(0x79, PDB1R); // Port E Bit 1 Register
_REG(0x7A, PDB2R); // Port E Bit 2 Register
_REG(0x7B, PDB3R); // Port E Bit 3 Register
_REG(0x7C, PDB4R); // Port E Bit 4 Register
_REG(0x7D, PDB5R); // Port E Bit 5 Register
_REG(0x7E, PDB6R); // Port E Bit 6 Register
_REG(0x7F, PDB7R); // Port E Bit 7 Register

_REG(0x80, IB0CR); // I/O Bank 0 Control Register
_REG(0x81, IB1CR); // I/O Bank 1 Control Register
_REG(0x82, IB2CR); // I/O Bank 2 Control Register
_REG(0x83, IB3CR); // I/O Bank 3 Control Register
_REG(0x84, IB4CR); // I/O Bank 4 Control Register
_REG(0x85, IB5CR); // I/O Bank 5 Control Register
_REG(0x86, IB6CR); // I/O Bank 6 Control Register
_REG(0x87, IB7CR); // I/O Bank 7 Control Register

_REG(0x98, I0CR);  // Interrupt 0 Control Register
_REG(0x99, I1CR);  // Interrupt 1 Control Register

_REG(0xA0, TACSR); // Timer A Control/Status Register
_REG(0xA9, TAT4R); // Timer A Time Constant 4 Register

_REG(0xC0, SADR);  // Serial Port A Data Register
_REG(0xC1, SAAR);  // Serial Port A Adress Register
_REG(0xC3, SASR);  // Serial Port A Status Register
_REG(0xC4, SACR);  // Serial Port A Control Register

#undef _REG

#endif

