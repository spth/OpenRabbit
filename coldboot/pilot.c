/*
   Copyright (c) 2020 Digi International Inc.

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
/*************************************************************
Pilot BIOS source code

This is what Dynamic C uses to load the regular BIOS to RAM

The new TC protocol is used...

*************************************************************/

// Note:  The "PILOT_BIOS" macro should be placed in the
//  Project Options' Defines box.  This allows the standard
//  RabbitBios.c file, with appropriate conditional compilation
//  directives, to be used to compile the Pilot BIOS.
//#define PILOT_BIOS

#pragma DATAWAITSUSED off
#memmap root

#ifndef _RAM_
	#error "Must compile pilot BIOS to RAM."
#endif

#ifdef __SEPARATE_INST_DATA__
#if __SEPARATE_INST_DATA__
#error "Turn off separate I&D space."
#endif
#endif

// uncomment the following macro to create a RAM-only pilot BIOS
//#define RAMONLYPILOT

// uncomment the following macro to create a pilot BIOS
//  without ID/User Blocks sector erase protection
//#define PB_UNRESTRICTED_ERASES

// uncomment the following macro to create a pilot BIOS
//  without ID/User Blocks sector write protection
//#define PB_UNRESTRICTED_WRITES

// Directives to locate the code and data correctly for the pilot BIOS.
#rcodorg rootcode2 0x00 0x6000 0x1800 apply
#rvarorg rootdata2 0x00 0x7FFF 0x0800 apply

// turn this on to debug the ISR via PD0
//#define PB_TRACE_ISR

// uncomment this to debug the pilot via serial port B
//#define PB_DEBUG_SERB

// where the PilotBIOS resides in LOGICAL space
#define PILOT_LOCATION	0x6000
// this size MUST be EVEN! (or the copy loop will fail)
#define PILOT_SIZE		0x2000

#ifdef RAMONLYPILOT
#define RAM_CS_TO_USE 0x40	// 2 ws, /OE0/WE0/CS0
#else
// the RAM control lines to use (uncomment exactly one of the following list):
#define RAM_CS_TO_USE 0x45	// 2 ws, /OE1/WE1/CS1
//#define RAM_CS_TO_USE 0x46	// 2 ws, /OE1/WE1/CS2
//#define RAM_CS_TO_USE 0x44	// 2 ws, /OE1/WE1/CS0
//#define RAM_CS_TO_USE 0x41	// 2 ws, /OE0/WE0/CS1
//#define RAM_CS_TO_USE 0x42	// 2 ws, /OE0/WE0/CS2
#endif	// RAMONLYPILOT

// the serial port to use
#define _PB_SxCR			SACR
#define _PB_SxSR			SASR
#define _PB_SxDR			SADR
#define _PB_INT_VECTOR	SERA_OFS
#define _PB_PCFRVAL		11000000b
#define _PB_BAUDTIMER	TAT4R

struct _dkcHeader {		//	structure for a communication header
	char  count;				//	number of bytes to transmit/receive
	int  address;			//	address
	char XPCval;				//	XPC
	char chksum;			//	checksum
	char *cPointer;		//	content pointer
	int  length;
};

// pull in the new-style target communications definitions
#use "tc_defs.lib"

// pull in debug sub-type definitions
#define TC_TYPE_DEBUG						0x01
#define TC_DEBUG_SETDEBUGTAG				0x1a
#define TC_DEBUG_GETDEBUGTAG           0x1b

// Pilot BIOS' own flash info structure
_FlashInfoType _FlashInfo;

// Pilot BIOS' own shadow registers
char MB0CRShadow,MB1CRShadow,MB2CRShadow,MB3CRShadow;

// identification of the hardware
char	PB_RAM_SIZE;
int	PB_FLASH_ID;
char	PB_DIV19200;
char	PB_FREQDIVIDER;
char 	PB_USEDOUBLER;
unsigned long	_PB_CPUID;
unsigned long  PB_IDBLOCK_PADDR; // physical address of the system id block

//root int ._PB_Entry();			// entry point for the Pilot BIOS finite state machine
root int _PB_Init() ;			// initilaization for the Pilot BIOS finite state machine
root _PB_WriteMem();
root _PB_ReadProgPort();
root _PB_WriteProgPort();
//root ._PB_InitTC();
root _PB_SetIntVecTab();
root int _PB_readIDBlock(int flashBitMap);
root int _PB_getcrc(char *Data, char count, int accum);

#define HIXPC 				0x71
#define LOXPCBASE 		0xF9
#define DIVADRS 			0x3F00
#define REGBIOSFLAG		0x3F01
#define FREQADRS			0x3F02
#define _PBINTVEC_BASE  0x5800		// this is a LOGICAL address that should fall
												// 	between the code and variables
#define BEGREGBIOS 		0x0000

/******* variable definitions ***********/
char sysStack[0x0200];
SysIDBlockType	_PB_SysIDBlock;

/* flash driver storage */
struct _dkcHeader curHeader;		//	current header information
char commBuffer[256];

/* the current packet */
_TC_PacketHeader	_PB_Header;
_TC_PacketFooter	_PB_Footer;
char 					_PB_Buffer[256];	//	packet body

/* vars for the ISR - it receives into this temp buffer */
/* this _must_ be 256 bytes long - it depends on the wrapping at 255->0 */
char	_PB_RXBuffer[256];
char	_PB_RXWritePointer;		// offset into _PB_Buffer we are receiving into
char	_PB_RXReadPointer;		// offset into _PB_Buffer we are reading from

/* FSM variables */
void*	_PB_Mode;				// mode (rx/tx) that the FSM is currently in
void*	_PB_State;				//	this is the address (state) of the comm FSM
char	_PB_esc;					// escaped character marker
int	_PB_length;				// length remaining to read/write
char*	_PB_ptr;					// pointer into the currently receiving/transmitting buffer
int	_PB_checksum;			// running checksum, computed on a byte-by-byte basis
int	_PB_save_checksum;	// temporary storage of the checksum

/* flags to queue the changing of the BAUD rate after a SETBAUDRATE packet */
char	_PB_setbaudrate;		// true if a baud rate change has been queued
char	_PB_newdivider;		// the new divider to set the baud rate too
char	_PB_waittx;				// true if we are waiting for a TX to finish before changing the baud rate

int update_delay;
char update_val;
unsigned int _PB_debugtag;

#asm root nodebug
premain::
main::
	db		0CCh		; loader looks for this byte; then replaces with 00h

#ifdef PB_TRACE_ISR
	; enable PD0 to trace the ISR
	ld		a,0x01
	ioi ld (PDDDR),a
	ld		a,0x00
	ioi ld (PDDCR),a
	ioi ld (PDDR),a
#endif

;**** Grab the 19200 baud frequency divider
	ld		a,(FREQADRS)
	ld		(PB_DIV19200),a

;**** TEST RAM SIZE ***********
;	Dynamic C needs to know how much RAM is available
	ld 	a,HIXPC			; upper Phys. Addr. byte
	ld    xpc,a
	ld 	hl,0xefff		; save byte at test location
	ld    b,(hl)
	exx
	ld 	hl,0xefff		; initialize logical xmem address

	ld 	c,1				; c counts number of 32k blocks of RAM
	ld 	e,LOXPCBASE		; start at Phys Adr RAMBASE+0x7fff
.loop:
	ld 	a,HIXPC			; save upper Phys Adr byte in d
	ld    xpc,a
	ld 	a,(hl)			;
	ld 	d,a				;

	ld    a,e				; read PhysAdr 0x80000+(0x8000*c - 1)
	ld    xpc,a				;	into a
	ld 	a,(hl)			;

	cp 	d
	jr    nz, .not_it		; can't be this size if not equal,
								;   but need to confirm with another
								;   value if equal

	ld 	a,HIXPC			; Invert the bits in the high PA byte
	ld    xpc,a				;   and see if it changes @
	ld    a,d				;   PhysAdr RAMBASE+(0x8000*c - 1)
	xor   0xff				;
	ld 	(hl),a			;
	ld 	d,a      	   ;
	ld    a,e				;
	ld    xpc,a
	ld 	a,(hl)			;
	cp    d					;

	jr    z, .done

.not_it:
	inc   c
	jr    z, .errdone		; if c rolls over something is very wrong
	ld    a, e
	add   a, 8				; e has xpc for next 32k block
	ld    e, a
	jr    .loop

.errdone:
	ld    c, 0
.done:
	ld    h, 0				; restore  upper PhysAddr byte
	ld    a, c
	ex    af, af'			; save count
	exx
	ld 	a, HIXPC
	ld    xpc, a
	ld 	hl, 0xefff
	ld    (hl),b
	ex    af,af'			; a now has count

	ld		(PB_RAM_SIZE),a
;**** END TEST RAM SIZE ********

	; the pilot BIOS's stack is not set up yet, use inline code
	;  to hit the watchdog instead of " call _PB_hitwd"
	ld		a, 0x5A
	ioi	ld (WDTCR), a

#if 0
	; prepare the blinkenlights for a jackrabbit board (8 PADR lights)
	ld		a,0x84
	ioi ld (SPCR),a
	; turn the lights off first...
	ld		a,0x55
	ioi ld (PADR),a
#endif

#if 0
		; prepare the blinkenlights for an RCM2000 proto-board (8 PADR LEDs)
		ld		a, 0x84
ioi	ld		(SPCR), a
		; initially, turn the (active low) LEDs off
		ld		a, 0xFF
ioi	ld		(PADR), a
#endif

#if 0
	; prepare the blinkenlights for an RCM2300 (PE1 & PE7)
	ld		a,0x82
	ioi ld (PEDDR),a
	ld		a,0x00
	ioi ld (PEFR),a
	ld		a,0x82
	ioi ld (PEDR),a
#endif

	ipset 1

._PB_SetProgPort:
	;	comm port A
	;	parallel port c, bit 6,7 use alternate pin assignment
	ld 	a,_PB_PCFRVAL
	ioi   ld (PCFR),a

	ld		a,0xc3			;	jump instruction
	ld		(_PBINTVEC_BASE+_PB_INT_VECTOR),a
	ld		hl,._PB_SerialISR
	ld		(_PBINTVEC_BASE+_PB_INT_VECTOR+1),hl

	;	at port c, async, 8-bit, interrupt priority 1
	ld 	a,00000001b
	ioi   ld (_PB_SxCR),a

._PB_GetCPUID:
	ioi	ld a, (GROM)			; read CPU identification parameters
	and	0x1F						; (formatted for unsigned long read by compiler)
	ld		(_PB_CPUID+2), a
	ioi	ld a, (GRAM)
	and	0x1F
	ld		(_PB_CPUID+3), a
	ioi	ld a, (GREV)
	and	0x1F
	ld		(_PB_CPUID+0), a
	ioi	ld a, (GCPU)
	and	0x1F
	ld		(_PB_CPUID+1), a

._PB_SetSP:
		; initialize stack pointer
		ld		hl, sysStack+@LENGTH
		ld		sp, hl
#if 0		// start Port A debug LEDs output conditional code block
		push	af
		; inspect SP register content
		bool	hl
      ld		l, h
		adc	hl, sp
;      ld		a, h
      ld		a, l
		; inspect IP register content
;		push	ip
;      push	ip
;      pop	af
      cpl
ioi	ld		(PADR), a
		pop	af
#endif	// end Port A debug LEDs output conditional code block
#ifdef PB_DEBUG_SERB
	; initialize serial B debug info output tools (mustn't call this
	;  before the pilot BIOS's stack is set up, as done just above!)
	call	_initserial_dbg
#endif
	ld 	hl, 0x0C
	call	_PB_readIDBlock
	bool	hl
	jr		z, ._PB_idBlockOk

._PB_idBlockBad:				; clear SysIDBlock struct if not on flash or in RAM
	ld		hl, _PB_SysIDBlock
	ld		b, _PB_SysIDBlock+marker+6-_PB_SysIDBlock
	xor	a
.blockEraseLoop:
	ld		(hl), a
	inc	hl
	djnz	.blockEraseLoop

._PB_idBlockOk:
#if 0		// start serial B debug conditional code block
		push	af				; preserve A, F registers' content
		call	_sendcrnl_dbg
		ld		a, (_PB_SysIDBlock+flashID+3)
		call	_sendbyte_dbg
		ld		a, (_PB_SysIDBlock+flashID+2)
		call	_sendbyte_dbg
		ld		a, (_PB_SysIDBlock+flashID+1)
		call	_sendbyte_dbg
		ld		a, (_PB_SysIDBlock+flashID+0)
		call	_sendbyte_dbg
		call	_sendcrnl_dbg
		ld		a, (_PB_SysIDBlock+flashType+1)
		call	_sendbyte_dbg
		ld		a, (_PB_SysIDBlock+flashType+0)
		call	_sendbyte_dbg
		call	_sendcrnl_dbg
		ld		a, (_PB_SysIDBlock+flashSize+1)
		call	_sendbyte_dbg
		ld		a, (_PB_SysIDBlock+flashSize+0)
		call	_sendbyte_dbg
		call	_sendcrnl_dbg
		ld		a, (_PB_SysIDBlock+sectorSize+1)
		call	_sendbyte_dbg
		ld		a, (_PB_SysIDBlock+sectorSize+0)
		call	_sendbyte_dbg
		call	_sendcrnl_dbg
		ld		a, (_PB_SysIDBlock+numSectors+1)
		call	_sendbyte_dbg
		ld		a, (_PB_SysIDBlock+numSectors+0)
		call	_sendbyte_dbg
		call	_sendcrnl_dbg
		ld		a, (_PB_SysIDBlock+flashSpeed+1)
		call	_sendbyte_dbg
		ld		a, (_PB_SysIDBlock+flashSpeed+0)
		call	_sendbyte_dbg
		call	_sendcrnl_dbg
;		call	_sendchar_dbg
		pop	af				; restore A, F registers' content
#endif	// end serial B debug conditional code block

_PB_CheckClockDoubler::
	call	_PB_getDoublerSetting
	ld		a, l
	ld		(PB_USEDOUBLER), a	; nonzero = enable doubler (later)

	ld		a, (PB_DIV19200)
	bool	hl							; still contains result of _PB_getDoublerSetting
	jr		z, .dont_double
.do_double:
	sla	a							; multiply by two
.dont_double:
	ld		(PB_FREQDIVIDER), a	; use this value for baud rate calcs

#ifndef RAMONLYPILOT
	;; initialize flash driver later, but get the flash ID now (for the info probe)
	ld		a, 0x72
	ld		(_FlashInfo+flashXPC), a

	call	_GetFlashID
#else
	bool	hl
	ld		l, h
#endif

	ld		(PB_FLASH_ID), hl

._PB_SetComm:
	call	_PB_InitBaudRateChange
	call	._PB_InitTC
	call	_PB_SetIntVecTab
	ipset 0

_PB_Loop::
	call	_PB_hitwd
	call	_PB_SerialPoll						; drive the FSM and TX
	call	_PB_RunQueuedBaudRateChange	; run any queued baud rate changes
	jp		_PB_Loop

._PB_InitTC:
	call	_PB_Init						; initialize comm module
	call	_PB_InitRXRing				; init the RX ring pointers
	ret

_PB_hitwd::
	push	af
	ld		a, 0x5A						; hit watchdog timer
	ioi	ld (WDTCR), a
	pop	af
	ret

; This will kill the PilotBIOS - Last-resort error handling
_PB_LockupPilotBios::
	ipset	3
._PB_LockupPilotLoop:
	call	_PB_hitwd
	jp		._PB_LockupPilotLoop

;//****** Init the flash driver... ****************************

_PB_InitFlashDriver::
#ifndef RAMONLYPILOT
		ld		a, RAM_CS_TO_USE
		ld		(MB0CRShadow), a
		ld		(MB1CRShadow), a
		ld		a, 0x40
		ld		(MB2CRShadow), a
		ld		(MB3CRShadow), a		; setup the shadow registers for the flash driver to use

		ld		hl, 0x04 | 0x08
		call	_InitFlashDriver

		bool	hl
		jp		nz, _PB_LockupPilotBios	; _InitFlashDriver should have returned success (0)

; determine if we have (possibly) 2x256kb flashes vs. 1x512kb flash
; (rely only on flashSize member, because it works with all small or large or
;  nonuniform sector flash types)
		ld		a, (_FlashInfo+flashSize)
		cp		0x41					; is primary flash > 256KB?
		jr		nc, .done_PB_IFD	; if yes, skip mapping 2nd flash . . .
		ld		a, 0x42				; 2nd flash uses /CS2, /OE0, /WE0, 1 wait
		ld		(MB3CRShadow), a
ioi	ld		(MB3CR), a			; (speculatively) map 2nd flash into MB3CR quadrant
.done_PB_IFD:
#if 0		// start Port A debug LEDs output conditional code block
		push	af
		; inspect SP register content
;		bool	hl
;		ld		l, h
;		adc	hl, sp
;		ld		a, h
;		ld		a, l
		; inspect IP register content
;		push	ip
;		push	ip
;		pop	af
		; inspect UserBlockSector
;		ld		a, (UserBlockSector+1)
		ld		a, (UserBlockSector)
		; inspect UserBlockSector
;		ld		a, (IDBlockSector+1)
;		ld		a, (IDBlockSector)
		cpl
ioi	ld		(PADR), a
		pop	af
#endif	// end Port A debug LEDs output conditional code block
#endif
	ret

;//****** Read the ID block... ****************************

_PB_readIDBlock::
	ld		a, xpc
	push	af						; save current xpc

	;; determine top memory quadrant mapped to flash
.flashInQ3:
	bit	3, l
	jr		z, .flashInQ2
	ld		a, 0xF0				; XPC to access top of flash
	jr		.copystart
.flashInQ2:
	bit	2, l
	jr		z, .flashInQ1
	ld		a, 0xB0				; XPC to access top of flash
	jr		.copystart
.flashInQ1:
	bit	1, l
	jr		z, .flashInQ0
	ld		a, 0x70				; XPC to access top of flash
	jr		.copystart
.flashInQ0:
	ld		a, 0x30				; XPC to access top of flash
	bit	0, l
	jr		nz, .copystart
	ld		hl, 0x0000			; if no bits set, error
	ld		(_PB_SysIDBlock+tableVersion), hl
	ld		hl, -1
	jp		.iddone

	;; copy top 16 bytes of block to struct in RAM
.copystart:
	ld		b, 17					; check 17 locations for the ID block
									;  (top-0K, top-4K, top-8K, ..., top-64K)
.copyheader:
	push	bc
	ld		xpc, a
	ld		hl, 0xFFF0			; a:hl points to tail part of ID block in flash (0x07FFF0)
	ld		de, _PB_SysIDBlock+idBlockSize
	ld		bc, 16
	ldir							; copy top 16 bytes of block to RAM

	;; check for 55 AA 55 AA 55 AA marker at end of ID block

	ld		hl, _PB_SysIDBlock+marker
	ld		b, 6					; Initialize loop counter
	push	af						; save XPC value for later
	ld		a, 0x55           ; Initial 55 value
.loop1:
	cp		(hl)              ; Check to see that marker matches value in A
	jr		nz, .badmarker

	inc	hl                ; Move to next marker byte
	cpl                     ; Complement A to get AA or 55 to check next byte.
	djnz	.loop1

	jr		.copyblock
.badmarker:
	pop	af						; fix stack
	dec	a						; try location 0x1000 bytes lower
	pop	bc
	djnz	.copyheader

	ld		hl, 0x0000			; return an error value
	ld		(_PB_SysIDBlock+tableVersion), hl
	ld		hl, -2
	jp		.iddone

	;; copy entire ID block to RAM

.copyblock:
	pop	af						; get XPC value
	pop	bc						; (clean up stack now -- old BC loop value)

	ld		hl, (_PB_SysIDBlock+idBlockSize)
   ex		de, hl
   bool	hl    				; clears HL and Carry flag for following subtraction
   ld		L, h
   sbc	hl, de				; hl now contains address to read the id block
	; bc now contains ID block size
	ld		bc, _PB_SysIDBlock+idBlockSize-_PB_SysIDBlock

	ld		xpc, a				; a:hl now points to ID block in flash

#if 1
	; Small diversion to convert address of idblock to a physical address
   ; and store in PB_IDBLOCK_PADDR.
  	ex		de,hl								; a=segment, hl=logical address
   ld		de', de

	bool	hl
	ld		l, h
   or 	a 						; clear carry
   ld 	h, a
   add	hl, hl
	add	hl, hl
   add	hl, hl
   add	hl, hl

	add	hl, de
   ld    (PB_IDBLOCK_PADDR), hl	; LSBs

   ld 	de, 0
   rl		de

   ld		h, d
   ld		l, a
   rr		hl
   rr		hl
	rr		hl
	rr		hl

	adc	hl, de
	ld		de, 0x0f
   and	hl, de						; MSBs

   ld		(PB_IDBLOCK_PADDR+2), hl
   ; Finished calculation of PB_IDBLOCK_PADDR. Continue with IDblock read.
   ex 	de', hl				; restore logical address
#endif
	ld		de, _PB_SysIDBlock		; de now points to struct in RAM
	ldir									; copy entire block to RAM

	;; now perform CRC check on block
.checkCRC:
	ld		hl, _PB_SysIDBlock+idBlockCRC
	ld		hl, (hl)				; get CRC value
	ex		de, hl
	ex		de, hl'				; save it in hl'

	ld		hl, _PB_SysIDBlock+idBlockCRC
	xor	a
	ld		(hl), a
	inc	hl
	ld		(hl), a				; clear out CRC value

	; first, do CRC on first 'n' bytes (avoid 'reserved' field)
	ld		hl, _PB_SysIDBlock+idBlock2-_PB_SysIDBlock
	ex		de, hl				; save total block size in de
	ld		hl, 0x0000
	ex		de', hl				; save running CRC value in de'

	; the _PB_getcrc() function can only process 255 bytes at a time,
	; so we may need to break the ID block into 255-byte chunks...
.crc_loop:
	push	de						; save byte count
	ex		de', hl
	push	hl						; push initial CRC value
	ex		de', hl
	ld		hl, 255
	sbc	hl, de
	jr		c, .biggerThan255
	ld		hl, _PB_SysIDBlock+idBlock2-_PB_SysIDBlock
	jr		.crc_cont
.biggerThan255:
	ld		hl, 255
.crc_cont:
	push	hl						; push size of data
	ld		hl, _PB_SysIDBlock
	push	hl						; push pointer to data
	call	_PB_getcrc
	add	sp, 6

	ld		b, h
	ld		c, l					; put this chunk's CRC in bc
	ex		de', hl
	add	hl, bc				; add this CRC value to running value
	ex		de', hl

	pop	de

	ld		hl, 255
	ex		de, hl
	xor	a						; clear carry
	sbc	hl, de
	ex		de, hl
	jr		nc, .crc_loop
	; now do last 16 bytes
	ex		de', hl				; move CRC value back to hl
	push	hl						; push last CRC output as next CRC input
	ld		hl, 16
	push	hl						; push size of data
	ld		hl, _PB_SysIDBlock+idBlockSize
	push	hl						; push pointer to data
	call	_PB_getcrc
	add	sp, 6
	;; hl now contains CRC value for ID block

	ex		de, hl'				; get original ID block value
	xor	a						; clear carry
	push	de						; save CRC value (temp)
	sbc	hl, de
	pop	de
	jr		nz, .badCRC

	;; CRC block matches, so restore CRC value to _PB_SysIDBlock
	ld		hl, _PB_SysIDBlock+idBlockCRC
	ld		(hl), e
	inc	hl
	ld		(hl), d				; restore CRC value in _PB_SysIDBlock table

	bool	hl
	ld		l, h					; return value = 0
	jp		.iddone
.badCRC:
	ld		hl, 0x0000
	ld		(_PB_SysIDBlock+tableVersion), hl
	ld		hl, -3				; return an error value
.iddone:
	pop	af
	ld		xpc, a				; restore XPC
	ret

_PB_getcrc::
	ld 	hl,0002   			; locate first 2 byte argument
	add 	hl,sp
	ld 	e,(hl)
	inc 	hl
	ld 	d,(hl)
	push	de    				; de has the address of the data
	inc 	hl     				; two more increment get the second 1 byte argument
	ld 	c,(hl)  				;  c has the number of counts
	inc	 hl     				; now pointing to the low byte of accum
	inc 	hl
	ld 	a,(hl)  				; a has the low byte of the accum
	inc 	hl
	ld 	h,(hl)  				; h has the high byte of the accum
	ld 	l,a     				; l has the low byte of the accum
.do_data:               		; compute crc for (de+1) bytes of data
	pop 	de     				;
	ld 	a,(de)  				; load first data
	inc 	de     				; update to next memory location
	push 	de    				; keep last memory location
	ld 	d,a     				; use d for current data
	ld 	b,8     				; rotate and xor data 8 times
.eight_times:
	ld 	a,h     				; xor data to accum
	xor 	d
	rlca
	jr 	nc,.no_poly  		; no carry, no need to xor polynomial
	add 	hl,hl
	ld 	a,0x10      		; xor 0x1021 to accum
	xor 	h
	ld 	h,a
	ld 	a,0x21
	xor 	l
	ld 	l,a
	jr 	.rotate_data
.no_poly:                	; only need to shift accum
	add 	hl,hl
.rotate_data:
	xor 	a
	rl 	d
	djnz 	.eight_times
	dec 	c
	jr 	nz,.do_data
	pop 	de    				; pop one last time to balance loop
	ret

;//*********** Serial interrupt handler ************
._PB_SerialISR:
	exx								; ISR has the ALT register set!
	ex		af,af'					; it may use: a, hl, bc, de only!

#ifdef PB_TRACE_ISR
	ld		a,0x01
	ioi ld (PDDR),a				; set PD0 high
#endif

	ioi ld a,(_PB_SxSR)			; what type of int is this?
	bit	7,a
	jr		z,._PBNotRXInt

	; this is a receive interrupt
	ioi ld a,(_PB_SxDR)			; get the byte (and clear the interrupt)
	ld		b,a						; store it in 'b'

	ld		a,(_PB_RXReadPointer)
	ld		c,a
	ld		a,(_PB_RXWritePointer)
	inc	a							; move the write pointer to the next cell
	cp		c							; does it collide w/ the READ pointer?
	jr		z,._PBReadyToExit		; if so, drop this byte

	ld		(_PB_RXWritePointer),a	; update the write pointer to the new location
	dec	a							; move back to the previous cell

	ld		e,a
	xor	a
	ld		d,a						; de has the offset
	ld		hl,_PB_RXBuffer		; get the buffer's start address
	add	hl,de						; hl points at the write-to cell
	ld		a,b						; move the data back to a
	ld		(hl),a					; store the byte

	jr		._PBReadyToExit			; all done!

._PBNotRXInt:
	; TX is handled by POLLING - just drop these interrupts
	ioi ld (_PB_SxSR),a

._PBReadyToExit:
#ifdef PB_TRACE_ISR
	ld		a,0x00
	ioi ld (PDDR),a				; set PD0 low
#endif

	ex		af,af'
	exx
	ipres
	ret

_PB_SerialPoll::	; POLL the serial port instead of using interrupts, to drive the FSM and TX
	call	._PB_Entry	 			; enter the FSM
	ret

_PB_ReadProgPort::
	;	destroys A
	;	returns byte read (if any) in A
	;	returns with Z set if nothing is read

	;	check if there is anything available
	ioi   ld a,(_PB_SxSR)
	bit	SS_RRDY_BIT,a			; if a received byte ready?
	ret	z							; nope, return with z set
	;	otherwise, a byte *is* ready, read from data port
	ioi   ld a,(_PB_SxDR)
	ret								; return with z *not* set

_PB_WriteProgPort::
	;	assumes byte to transmit is in C
	;	destroys A
	;	returns with Z reset if not transmitted

	;	check if the port is ready
	ioi   ld a,(_PB_SxSR)

	bit	SS_TFULL_BIT,a			; can I transmit now?
	ret	nz							; nope, return with nz set
	; otherwise, the transmit buffer is ready, write to it!
	ld		a,c						; move byte to transmit to a
	ioi   ld (_PB_SxDR),a
	ret								; return with z *not* set

_PB_CanProgPortTransmit::
	;  destroys A
	;  returns with Z reset if the transmitter is busy,
	;  and Z set if it is avaliable to transmit
	ioi	ld a,(_PB_SxSR)
	bit   SS_TFULL_BIT,a 		; can I transmit now?
	ret

_PB_IsProgPortTxBusy::
	; destroys A
	; returns with Z reset if the transmiter is doing anything
	; and Z set if the transmitter is completely idle
	ioi	ld	a,(_PB_SxSR)
	bit	2,a
	ret	nz
	bit	3,a
	ret

_PB_SetIntVecTab::
	;	set up interrupt vector table
	ld		a,0xff & (_PBINTVEC_BASE >> 8)	;	R register has 0x20, so interrupt table starts at 2000
	ld		iir,a
	ld		eir,a
	ret

_PB_InitRXRing::
	xor	a
	ld		(_PB_RXWritePointer),a
	ld		(_PB_RXReadPointer),a
	ret

_PB_Init::	;	initialize the communication module
	ld		hl,._PB_ModeRX
	ld		(_PB_Mode),hl			; start receiving initially
	ld		hl,._PB_RXNothing
	ld		(_PB_State),hl			; default to the Nothing state, that will drop bytes
	xor	a
	ld		(_PB_esc),a				; do not unescape the next byte
	bool	hl
	ld		l,h
	ld		(_PB_length),hl		; nothing to receive
	ld		(_PB_ptr),hl			; no buffer to receive into
	ret

._PB_Read:		; Read out of the RX ring instead of calling the ReadPort function directly
	push	ip
	ipset	1							; this must be done with ints OFF!

	ld		a,(_PB_RXWritePointer)
	ld		b,a
	ld		a,(_PB_RXReadPointer)
	cp		b							; do the pointers match?
	jr		z,._PB_ReadNoData		; if so, there is no data to read

	; data is good - update the read pointer
	inc	a
	ld		(_PB_RXReadPointer),a
	dec	a
	ld		e,a
	xor	a
	ld		d,a						; de has the offset
	ld		hl,_PB_RXBuffer		; get the buffer's START address
	add	hl,de
	ld		a,(hl)					; get the byte
	bool	hl							; set NZ
	jr		._PB_ReadDone

._PB_ReadNoData:
	xor	a
	or		a							; set Z
	;jr	._PB_ReadDone

._PB_ReadDone:
	pop	ip
	ret

._PB_Write:  	; this is the abstraction of the actual write mechanism
	jp		_PB_WriteProgPort

._PB_CanTransmit:	; abstraction of the above
	jp		_PB_CanProgPortTransmit

._PB_IsTransmiterIdle:	; abstraction of the above
	jp		_PB_IsProgPortTxBusy

; Computes a checksum
; Uses the 8-bit Fletcher checksum algorithim. See RFC1145 for more info
;
; assumes the following:
;   hl == the checksum variable
;   a  == the value to add to the checksum
_PB_Checksum::
	ld		c,a								; save the value in a
	add	a,h
	adc	a,0x00
	ld		h,a								; A = A + D[i]
	add	a,l
	adc	a,0x00
	ld		l,a								; B = B + A
	ld		a,c								; restore a
	ret

; Converts a physical address stored in bc,de to a logical address.
; The resulting xpc will be in a, and the offset will be in hl.
_PB_PhysicalToLogical::
	push	bc			; save the address for later
	push	de

	ld		a,0x0f
	and	c
	ld		c,a		; c = c & 0x0f
	ld		a,0xf0
	and	d			; a = d & 0xf0
	or		c			; a = c | d
	rlca				; transpose the two nibbles
	rlca
	rlca
	rlca				; a = xpc + 0xe
	sub	0xE		; a = xpc

	ld		hl,0x0fff
	and	hl,de
	ex		de,hl
	ld		hl,0xe000
	add	hl,de

	pop	de
	pop	bc			; restore the address
	ret

;//****** START IN RAM - just jump to the beginning again
_PB_StartRegBiosRAM::
	xor	a
  	ioi	ld (GCDR), a
	jp		BEGREGBIOS

;//****** START IN FLASH - Code to fix the MBxCR mapings, and jump to 0x0000

_PB_StartRegBiosFLASH::
	ld		a,RAM_CS_TO_USE
	ioi ld (MB2CR),a								; set the 3rd quadrant to be a mirror of RAM

	ipset	3											; turn off interrupts
	call	_PB_hitwd

	ld		de,_PB_StartSecondQuadrant			; get our destination address
	ioi ld a,(DATASEG)
	rrca
	rrca
	rrca
	rrca												; swap the nibbles
	ld		c,a										; save it in b
	and	0xF0										; get the high nibble
	ld		h,a
	xor	a
	ld		b,a										; set b to 0
	ld		l,a
	add	hl,de
	ex		de,hl										; de has the low word of the physical address
	ld		a,c										; get the original data
	adc	a,0x00
	and	0x0F										; mask out the low nibble
	or		0x08										; set this bit to move to the 3rd quadrant
	ld		c,a										; save it in c; bc has the high word of the physical address

	call	_PB_PhysicalToLogical
	ld		xpc,a										; set the window to the 3nd quadrant
	xor	a
  	ioi	ld (GCDR), a
	jp		(hl)										; ...and jump there

_PB_StartSecondQuadrant::
	call	_PB_hitwd
	ld		a,0x40
	ioi ld (MB0CR),a								; set the 1st quadrant to FLASH

	jp		BEGREGBIOS								; ...and start the real BIOS

;//****** Lookup the Baud Rate divider, based on div19200
; expects:
;		ix: points to the (long) baud rate
; returns (if nz is set):
;		a: the new divider
; returns (if z is set):
;		<nothing - baud rate is unacceptable>

_PB_LookupBaudRateDivider::
	ld		hl,_PB_BaudTable
._PB_LookupLoop:
	ex		de,hl
	ld		hl,_PB_EndBaudTable
	or		a
	sbc	hl,de						; does (hl == _PB_EndBaudTable)?
	ret	z							; return w/ z set if we hit the end of the table
	ex		de,hl

	ld		iy,hl						; iy has the table entry
	call	._PB_CompareTableEntry
	jr		z,._PB_FoundTableEntry

	ld		de,5						; de == size of one table entry
	add	hl,de
	jr		._PB_LookupLoop		; try the next entry

._PB_FoundTableEntry:				; iy has the table entry!
	ld		a,(iy+4)					; get the divider value
	ld		e,a						; put it in 'e'
	ld		a,(PB_FREQDIVIDER)	; a has the original divider
	ld		d,a						; put it in 'd'
	xor	a
	ld		b,a						; the result will be in 'b'

._PB_DivLoop:	; compute (d/e)
	; are we done? (is the old divider 0?)
	xor	a
	or		d
	jr		z,._PB_DivFinished

	; do one subtraction
	ld		a,d
	sub	e
	jr		c,._PB_BadDiv
	ld		d,a

	; increment the result
	inc	b
	jr		._PB_DivLoop

._PB_BadDiv:	; couldn't divide it evenly
	xor	a
	or		a							; set z
	ret

._PB_DivFinished:
	ld		a,b
	dec	a							; subtract 1 from the divider to get the actuall value
	ld		b,a						; keep in in b for a sec...

	ld		d,0xff
	or		d							; set nz

	ld		a,b						; the return value
	ret

_PB_BaudTable::
;        baud rate, in hex     divider relative to 19200
;        (little endien!)
;        --------------------  -------------------------
	db		0x00,0xE1,0x00,0x00,	 3
	db		0x00,0xC2,0x01,0x00,  6
	db		0x00,0x84,0x03,0x00,  12
	db		0x00,0x08,0x07,0x00,  24
_PB_EndBaudTable::
	db		0x00,0x00,0x00,0x00,  0x00

._PB_CompareTableEntry:	; compare a single entry - ix & iy should point to the longs
								; _MUST_ not clobber hl!
	ld		a,(ix+0)
	cp		(iy+0)
	ret	nz
	ld		a,(ix+1)
	cp		(iy+1)
	ret	nz
	ld		a,(ix+2)
	cp		(iy+2)
	ret	nz
	ld		a,(ix+3)
	cp		(iy+3)
	ret

_PB_InitBaudRateChange::
	; init the baud rate change values
	xor	a
	ld		(_PB_setbaudrate),a
	ld		(_PB_newdivider),a
	ld		(_PB_waittx),a
	ret

_PB_RunQueuedBaudRateChange::		; poll to see if we need to change the baud rate
	ld		a,(_PB_setbaudrate)
	or		a
	ret	z								; return if no change has been queued

	ld		a,(_PB_waittx)
	or		a
	ret	nz								; return if the transmitter is still active

	; test the serial port
	call	._PB_IsTransmiterIdle
	ret	nz								; the transmitter is busy - return

	; the transmitter is completely idle! set the baud rate!
	push	ip
	ipset 3								; ints off for safety!

	; enable doubler if possible
	ld		a, (PB_USEDOUBLER)
	ioi	ld (GCDR), a

	ld		a,(_PB_newdivider)
	ioi ld (_PB_BAUDTIMER),a		; set the baud rate!

	call	_PB_InitBaudRateChange	; reset everything back to normal
	call	._PB_InitTC					; make sure the FSM is reset as well

	call	._PB_Read						; flush any byte currently there...

	pop	ip
	ret

;//****** Relocation code, to move the pilot BIOS at run time ********

; return with nz set to NAK, z set to ACK
_PB_Relocate::
	ld		hl,(ix)
	ex		de,hl
	ld		hl,(ix+2)
	ld		b,h
	ld		c,l

	ld 	h,c								; compute the new DATASEG value
	ld		l,d
	rr		hl
	rr		hl
	rr		hl
	rr		hl
	ld		a,l
	sub	0x6 								; a is now our dataseg value
	ex		af,af'							; save it in af' for later

	call	_PB_PhysicalToLogical		; get the SEGMENT:OFFSET version of the address
	push	de									; protect the address for later
	ld		de,0xe000
	or		a
	sbc	hl,de
	pop	de
	ret	nz									; return if this address is not on a page boundary

	push	ip
	ipset	3									; nothing else should be running while this is going on
	ex		af,af'							; bring back the DATASEG value
	push	af									; ...and move it to the stack

	; copy the PilotBIOS to the new location (bc:de has the physical address of the destination)
	ex		de,hl
	ld		ix,hl								; ix has the low word of the dst address
	ld		a,c								; a has the high byte of the dst address
	ld		iy,PILOT_LOCATION				; iy has the (logical) src address
	ld		bc,PILOT_SIZE					; bc has the size of the copy (MUST BE EVEN!)
	ld		de,0x02							; increment size after each copy

._PB_RelocateCOPYTOP:
	; are we done?
	ex		af,af'							; save the address value that is in 'a'
	ld		a,b
	or		c
	jr		z,._PB_RelocateCOPYDONE		; leave if the count is 0
	ex		af,af'

	; copy one word
	ld		hl,(iy)							; get the word from the source
	ldp	(ix),hl							; ...and store it to the dst

	; move to the next word
	add	iy,de								; move the src (logical) pointer
	add	ix,de								; move the dst (physical) pointer
	adc	a,0x00							; force the dst's higher byte to update properly
	dec	bc									; decrement our count by 2
	dec	bc
	jr		._PB_RelocateCOPYTOP			; and loop

._PB_RelocateCOPYDONE:
	; the new segment is in 'a' - just load it to the DATASEG (where we are
	; running from), and things should continue to work.
	pop	af
	ioi ld (DATASEG),a
	; ok, we should be in the new location now
	pop	ip									; let other thing run again
	xor	a
	or		a									; set Z
	ret

;//****** Begin Pilot BIOS FSM ****************************

._PB_Entry: 	;this is the entry point of the communication FSM
	ld		hl,(_PB_Mode)			; load address of current mode
	jp		(hl)					  	; and jump indirect to it

._PB_ModeRX:	; receiving - get the byte that was received
	call	._PB_Read
	ret	z							; nothing was available - return

	cp		TC_FRAMING_START
	jr		z,._PB_RXHaveStart		; was the character the beginning of a new packet?

	ld		b,a						; save the read character
	ld		a,(_PB_esc)
	or		a							; should the next character be escaped
	jr		z,._PB_RXNoEsc

	ld		a,b
	xor	0x20						; unescape the character
	ld		b,a
	xor	a
	ld		(_PB_esc),a				; mark the next character as not-escaped
	jr		._PB_RXHaveCharacter	; continue with the un-escaped character

._PB_RXNoEsc: ; the character is in b, and does not need to be escaped
	ld		a,b						; character is in BOTH a AND b
	cp		TC_FRAMING_ESC
	jr		nz,._PB_RXHaveCharacter	; not the ESC character

	ld		a,0x01
	ld		(_PB_esc),a				; mark the next character as one that needs to be escaped
	ret								; this character is done - return

._PB_RXHaveCharacter:	; the good character is in b
	ld		a,b						; the character is in BOTH a AND b
	ld		hl,(_PB_checksum)		; get the running checksum
	call	_PB_Checksum
	ld		(_PB_checksum),hl		; save the running checksum

	ld		hl,(_PB_ptr)			; is there any place to receive into?
	ld		a,l
	or		h
	jr		z,._PB_RXSkipStore

	; ptr is non-NULL -- store the character
	ld		a,b
	ld		(hl),a					; store the character
	inc	hl
	ld		(_PB_ptr),hl			; increment the ptr and store it back
	ld		hl,(_PB_length)		; get the length
	dec	hl
	ld		(_PB_length),hl		; decrement by 1 and store it again
	ld		a,l
	or		h
	jr		z,._PB_RXSkipStore		; if length is 0 (post decrement), enter the FSM
	ret								; otherwise, return, so more characters can be read

._PB_RXSkipStore:
	ld		a,b
	ld		hl,(_PB_State)
	jp		(hl)						; normal character - enter the RX state machine
										; the received byte is in 'a' if it matters

._PB_RXNothing:	; a START character has not been received - do nothing
	ret

._PB_RXHaveStart: ; received a START character! reset everything for the new packet!
	bool	hl
	ld		l,h
	ld		a,h
	ld		(_PB_esc),a				; do not escape anything, initially
	ld		(_PB_checksum),hl		; init the checksum to 0
	ld		hl,TC_HEADER_SIZE-2
	ld		(_PB_length),hl		; store the length of the header (minus the header_checksum)
	ld		hl,_PB_Header
	ld		(_PB_ptr),hl			; mark where to store the header
	ld		hl,._PB_RXHaveHeader
	ld		(_PB_State),hl			; start receiving the header
	ret

._PB_RXHaveHeader: ; have the header of the packet - store the checksum and get the header_checksum to compare
	ld		hl,(_PB_checksum)
	ld		(_PB_save_checksum),hl	; save the header_checksum for later
	ld		hl,2
	ld		(_PB_length),hl		; save the length (only 2, as only the header_checksum needs to be received)
	ld		hl,._PB_RXHaveHeaderChecksum
	ld		(_PB_State),hl
	ret

._PB_RXHaveHeaderChecksum:
	ld		hl,(_PB_save_checksum)
	ex		de,hl
	ld		hl,(_PB_Header+header_checksum)
	or		a
	sbc	hl,de
	jr		nz,._PB_RXBadHeaderChecksum	; do the checksums match?

	; they did - start receiving the body of the packet
	ld		hl,(_PB_Header+length)
	ld		(_PB_length),hl		; length of the body
	ld		a,l
	or		h
	jr		z,._PB_RXHaveBody		; is the length 0? if so, go straight to getting the footer
	ld		hl,_PB_Buffer
	ld		(_PB_ptr),hl			; point at the buffer...
	ld		hl,._PB_RXHaveBody
	ld		(_PB_State),hl
	ret

._PB_RXBadHeaderChecksum: ; the header checksum failed - flush the rest of the packet
._PB_RXBadChecksum: ; the main checksum failed - flush the rest of the packet
	call	_PB_Init	; reset everything and wait for a START command again
	ret

._PB_RXHaveBody: ; the body of the packet has been received - get the footer
	ld		hl,(_PB_checksum)
	ld		(_PB_save_checksum),hl	; save a copy of the checksum for later
	ld		hl,TC_FOOTER_SIZE
	ld		(_PB_length),hl		; store the footer's length
	ld		hl,_PB_Footer
	ld		(_PB_ptr),hl			; point at the storage for the footer
	ld		hl,._PB_RXHaveFooter
	ld		(_PB_State),hl
	ret

._PB_RXHaveFooter: ; the footer has been received - verify the checksum
	ld		hl,(_PB_save_checksum)
	ex		de,hl
	ld		hl,(_PB_Footer+checksum)
	or		a
	sbc	hl,de
	jr		nz,._PB_RXBadChecksum	; the checksums didn't match

	; the checksums matched - the entire packet has been received
	ld		ix,_PB_Buffer			; point at the packet buffer, for the handlers to use
	ld		a,(_PB_Header+type)
	cp		TC_TYPE_SYSTEM
	jp		z,._PB_SystemSubtype
   cp		TC_TYPE_DEBUG
   jp		z,._PB_DebugSubtype
	jp		._PB_NakPacket			; is the type SYSTEM or DEBUG? if not, NAK the packet


._PB_SystemSubtype:
	ld		a,(_PB_Header+subtype)	; dispatch by subtype... (should this be a jump table?)
	cp		TC_SYSTEM_NAK
	jr		z,._PB_HandleNAK
	cp		TC_SYSTEM_NOOP
	jr		z,._PB_HandleNOOP
	cp		TC_SYSTEM_READ
	jr		z,._PB_HandleREAD
	cp		TC_SYSTEM_WRITE
	jr		z,._PB_HandleWRITE
	cp		TC_SYSTEM_INFOPROBE
	jp		z,._PB_HandleINFOPROBE
	cp		TC_SYSTEM_STARTBIOS
	jp		z,._PB_HandleSTARTBIOS
	cp		TC_SYSTEM_SETBAUDRATE
	jp		z,._PB_HandleSETBAUDRATE
	cp		TC_SYSTEM_RELOCATE
	jp		z,._PB_HandleRELOCATE
	cp		TC_SYSTEM_ERASEFLASH
	jp		z,._PB_HandleERASEFLASH
	cp		TC_SYSTEM_FLASHDATA
	jp		z,._PB_HandleFLASHDATA
	; unknown subtype - NAK it!
	jp		._PB_NakPacket

._PB_DebugSubtype:
	ld		a,(_PB_Header+subtype)
   cp		TC_DEBUG_GETDEBUGTAG
   jp		z,._PB_HandleGetDebugTag
   cp		TC_DEBUG_SETDEBUGTAG
   jp		z,._PB_HandleSetDebugTag
   ; unknown subtype - NAK it
   jp		._PB_NakPacket

._PB_HandleNAK: ; Handling of NAKs is not supported here - just drop the packet
	call	_PB_Init
	ret

._PB_HandleNOOP: ; Reflect the packet back at them...
	jp		._PB_AckPacket

._PB_HandleREAD: ; read a block of data, and reply with it
	ld		a,(ix)				; get the TYPE of the READ
	cp		TC_SYSREAD_PHYSICAL
	jp		nz,._PB_NakPacket	; Not a physical-address READ! This is not supported!

	ld		a,xpc
	push	af						; save the XPC window

	ld		hl,(ix+3)			; get the physical address
	ex		de,hl
	ld		hl,(ix+5)			; ...and the 2nd word of the address
	ld		b,h
	ld		c,l
	call	_PB_PhysicalToLogical
	ld		xpc,a					; set the new window
	ld		iy,hl					; iy is the source!

	ld		hl,(ix+1)			; get the length of the read
	ld		b,h
	ld		c,l					; save it in bc
	ld		(ix),hl				; build the ACK header
	push	bc
	ld		bc,6
	add	hl,bc					; get the total length of the packet
	ld		(_PB_Header+length),hl	; ...and store it in the packet
	pop	bc

	ld		hl,(ix+3)
	ld		(ix+2),hl			; move the physical address down one byte
	ld		hl,(ix+5)
	ld		(ix+4),hl			; ...and the 2nd word of the address

	ld		de,6
	add	ix,de					; move ix such that it points at the destination
	ld		hl,ix
	ex		de,hl					; de has the destination
	ld		hl,iy					; hl has the source -- bc already has the length
	ldir							; copy the data into the packet

	pop	af						; restore the XPC window
	jp		._PB_AckPacket		; ACK the READ

._PB_HandleWRITE: ; write the given data out to memory
	ld		a,xpc
	push	af						; save the xpc

	ld		a,(ix)				; get the WRITE type
	cp		TC_SYSWRITE_PHYSICAL
	jp		nz,._PB_NakPacket	; only PHYSICAL address are supported!

	ld		hl,(ix+3)
	ex		de,hl
	ld		hl,(ix+5)			; get the physical address of the buffer
	ld		b,h
	ld		c,l					; bc:de has the physical address

	ld		a,c					; get A[19:16]
	and	0x0F					; mask out the unused bits
	cp		0x08
	jr		nc,._PB_WRITEFlash

._PB_WRITERam:					; write it to RAM...
	call	_PB_PhysicalToLogical
	ld		xpc,a
	ex		de,hl					; xpc:de now points at the destination
	ld		hl,(ix+1)			; get the length of the write
	ld		b,h
	ld		c,l					; ...and put it in bc
	push	de
	ld		de,7
	ld		hl,ix
	add	hl,de					; hl points at the source data
	pop	de
	ldir							; copy the data
	jr		._PB_WRITEAck		; ack the packet

._PB_WRITEFlash:				; write it to flash...
	ld		hl,(ix+1)			; find the length from the packet
	ld		b,h
	ld		c,l					; ...and put it in bc
	ld		hl,_PB_Buffer
	ld		de,7
	add	hl,de					; find the beginning of the data
	ld		de,commBuffer

._PB_WRITEcopyloop:
	ld		a, (hl)
	ld		(de), a
	inc	hl
	inc	de
	dec	bc
	xor	a
	cp		c
	jr		nz, ._PB_WRITEcopyloop
	cp		b
	jr		nz, ._PB_WRITEcopyloop

	ld		hl,(ix+3)			; get the physical address of the destination
	ex		de,hl
	ld		hl,(ix+5)
	ld		b,h
	ld		c,l
	call	_PB_PhysicalToLogical
	ld		(curHeader+address),hl
	ld		(curHeader+XPCval),a
	ld		hl,(ix+1)
	ld		(curHeader+length),hl

	ld		a, (MB3CRShadow)				; check if we have a 2nd flash
	cp		0x42
	jr		nz, .noChangeXPC

	ld    a, (curHeader+XPCval)
	cp		0xB2
	jr   	c, .noChangeXPC					; writing to second flash?
	ld    a, 0xB2
	ld 	(_FlashInfo+flashXPC), a	; fool flash driver into pointing to 2nd flash
#ifdef PB_UNRESTRICTED_WRITES
; always enable ID/User Blocks overwrite!
.noChangeXPC:
#endif
	bool	hl
	inc	hl										; ensure HL is nonzero, disable
	ld		(_overwrite_block_flag), hl	;  ID/User Blocks protection!
#ifndef PB_UNRESTRICTED_WRITES
; don't enable ID/User Blocks overwrite
.noChangeXPC:
#endif
	push	ip
	ipset	3						; turn off interupts while in the flash-writer!
	call	FSM_XFlash			; write it all!
	pop	ip						; restore interrupts

	ld		a, 0x72				; restore XPC value for 1st flash
	ld		(_FlashInfo+flashXPC), a

	ex		de, hl
	bool	hl
	ld		l, h									; ensure HL is zero, enable
	ld		(_overwrite_block_flag), hl	;  ID/User Blocks protection
	ex		de, hl

	bool	hl
	jr		nz,._PB_WRITENak
	;jr		._PB_WRITEAck

._PB_WRITEAck:
	bool	hl
	ld		l,h
	ld		(_PB_Header+length),hl	; no body to the ACK packet

	pop	af
	ld		xpc,a					; restore the xpc
	jp		._PB_AckPacket		; reply as an ACK

._PB_WRITENak:
	pop	af
	ld		xpc,a
	jp		._PB_NakPacket

._PB_HandleINFOPROBE: ; return a block of configuration data
	ld		hl, (PB_IDBLOCK_PADDR)
   ld		(ix), hl
	ld		hl, (PB_IDBLOCK_PADDR+2)
   ld		(ix+2), hl
   ld		bc, 4
   add	ix, bc
	ld		hl,(PB_FLASH_ID)	; get the flash id
	ld		(ix),hl				; ...and store it in the packet
	inc	ix
	inc	ix						; move to the next field in the packet
	ld		a,(PB_RAM_SIZE)	; get the RAM size
	ld		(ix),a				; ...and store it in the packet
	inc	ix						; move to the div19200 field
	ld		a,(PB_DIV19200)	; get the 19200 baud divider
	ld		(ix),a				; ...and store it in the packet
	inc	ix						; move to the IDBlock field
	ld		a, (_PB_CPUID)		; the current CPUID value
	ld		(ix),a				; store the 4-byte CPUID value
	inc	ix
	ld		a, (_PB_CPUID+1)
	ld		(ix),a
	inc	ix
	ld		a, (_PB_CPUID+2)
	ld		(ix),a
	inc	ix
	ld		a, (_PB_CPUID+3)
	ld		(ix),a
	inc	ix						; move past the CPUID value
	ld		hl,ix
	ex		de,hl								; destination is in DE
	ld		hl,_PB_SysIDBlock				; hl is the source
	ld		bc,sizeof(SysIDBlockType)	; bc is the length
	ldir										; copy the IDBlock

	ld		hl,sizeof(SysIDBlockType)
	ex		de,hl
	ld		hl, 12; sizeof(PB_FLASH_ID)+sizeof(PB_RAM_SIZE)+sizeof(PB_DIV19200)+sizeof(_PB_CPUID)
	add	hl,de								; hl == length of the packet - IDBlock+3
	ld		(_PB_Header+length),hl		; store the length in the outgoing packet
	jp		._PB_AckPacket					; send the ACK back

._PB_HandleSTARTBIOS: ; start executing the main BIOS
	ld		a,(ix)							; get the start_mode
	cp		TC_STARTBIOS_RAM
	jp		z,_PB_StartRegBiosRAM		; should we run in RAM?
	cp		TC_STARTBIOS_FLASH
	jp		z,_PB_StartRegBiosFLASH		; should we run in FLASH?
	jp		._PB_NakPacket					; unknown START type - nak it

._PB_HandleSETBAUDRATE: ; test the incoming baud rate and maybe set ours
	; all replys have length 0
	bool	hl
	ld		l,h
	ld		(_PB_Header+length),hl		; the reply is of 0 length

	call	_PB_LookupBaudRateDivider	; lookup the divider
	jp		z,._PB_NakPacket				; was the divider acceptable?

	; queue the divider to be set after the packet is finished
	ld		(_PB_newdivider),a			; store the divider for later
	ld		a,0xff
	ld		(_PB_setbaudrate),a			; queue the baud rate change
	ld		(_PB_waittx),a					; mark that we are waiting for the TX to finish
	jp		._PB_AckPacket					; ACK the packet

._PB_HandleRELOCATE: ; relocate ourselves to the specified page
	; ix points to the physical-address of our destination
	call	_PB_Relocate
	jp		nz,._PB_NakPacket				; error relocating to that address

	bool	hl
	ld		l,h
	ld		(_PB_Header+length),hl		; our ACK has no data
	jp		._PB_AckPacket					; ...and send the ACK

._PB_HandleERASEFLASH: ; erase the entire FLASH
	ld		hl, (ix+2)						; get physical address passed from compiler
	ld		b, h
	ld		c, l
	ld		hl, (ix)
	ex		de, hl

	ld		a, (_FlashInfo+flashSize)
	cp		0x41							     	; is the 1st flash > 256kb?
	jr		nc, ._PB_eraseSomeFirstFlash ; if so, don't erase it all if code > 256kb

	ld		a, (ix+2)
	cp		0x0C								; are we in the 2nd flash address range?
	jr		nc, ._PB_eraseAllFirstFlash
._PB_eraseSomeFirstFlash:
	call	longToSector					; convert to sector number
	ld		b, h
	ld		c, l								; highest sector used now in BC
	inc	bc									; make it number of sectors to erase
	jr		._PB_eraseFirst

._PB_eraseAllFirstFlash:
	ld		bc, 0x0000						; shorthand for erase all sectors

._PB_eraseFirst:
#ifdef PB_UNRESTRICTED_ERASES
	bool	hl
	inc	hl										; ensure HL is nonzero, disable
	ld		(_overwrite_block_flag), hl	;  ID/User Blocks protection!
#endif
	call	_EraseFlashChip
	bool	hl
	jp		nz,._PB_NakPacket				; error in erase flash (shouldn't happen)

	ld		a, (MB3CRShadow)
	cp		0x42								; do we have two flash?
	jr		nz, ._PB_HEFAck

	ld		a, (ix+2)
	cp		0x0C								; are we in the 2nd flash address range?
	jr		c, ._PB_HEFAck

	// erase entire 2nd flash
	ld		bc, 0x0000						; shorthand for erase all sectors
	call	_EraseFlashChip2
	bool	hl
	jp		nz,._PB_NakPacket				; error in erase flash (shouldn't happen)

._PB_HEFAck:
	; hl is already zero
	ld		(_PB_Header+length),hl		; our ACK has no data
	jp		._PB_AckPacket					; ...and send the ACK

._PB_HandleFLASHDATA:
	call	_PB_InitFlashDriver			; packet info read in _InitFlashDriver

	bool	hl
	ld		l,h
	ld		(_PB_Header+length),hl		; our ACK has no data
	jp		._PB_AckPacket					; ...and send the ACK

._PB_HandleGetDebugTag:
	ld		hl,(_PB_debugtag)
   ld		(ix),hl
   ld		hl,sizeof(_PB_debugtag)
   ld		(_PB_Header+length),hl	; ...and store it in the packet
   jp		._PB_AckPacket

._PB_HandleSetDebugTag:
	ld		hl,(ix)
   ld		(_PB_debugtag),hl
	jp		._PB_AckPacket

._PB_NakPacket: ; OR the subtype w/ TC_NAK and send them an empty packet back
	ld		a,(_PB_Header+subtype)
	or		TC_NAK
	ld		(_PB_Header+subtype),a
	bool	hl
	ld		l,h
	ld		(_PB_Header+length),hl
	jr		._PB_Reply

._PB_AckPacket: ; OR the subtype w/ TC_ACK and send the packet back
	ld		a,(_PB_Header+subtype)
	or		TC_ACK
	ld		(_PB_Header+subtype),a
	;jr		._PB_Reply

._PB_Reply: ; the reply is in the header/buffer/footer buffers - send it!
	ld		hl,._PB_ModeTX
	ld		(_PB_Mode),hl			; move to the TX mode
	ld		hl,._PB_SendHeaderChecksum
	ld		(_PB_State),hl			; after sending the header, build and send the header_checksum

	ld		hl,TC_HEADER_SIZE-2	; 2 for the header_checksum
	ld		(_PB_length),hl
	ld		hl,_PB_Header
	ld		(_PB_ptr),hl			; point at the buffer

	ld		c,TC_FRAMING_START	; start with a START character
._PB_TXStart:
	call	._PB_Write
	jr		nz,._PB_TXStart			; loop to start the transmission

	bool	hl
	ld		l,h
	ld		(_PB_checksum),hl		; start the checksum at zero

	ld		a,TC_FRAMING_ESC
	ld		(_PB_esc),a				; init the ESC marker to non-escape-mode
	ret

._PB_ModeTX: ; the transmit mode - send the current buffer
	call	._PB_Read					; this is only HALF-DUPLEX, so flush any received character
	call	._PB_CanTransmit
	ret	nz							; return if the transmitter is still busy - another INT will happen later

	ld		a,(_PB_esc)				; is an escaped character pending?
	cp		TC_FRAMING_ESC
	jr		z,._PB_TXNoEsc

	ld		c,a
._PB_TXSendEscapedChar:
	call	._PB_Write
	jr		nz,._PB_TXSendEscapedChar	; loop while the escaped character is sent
	ld		a,TC_FRAMING_ESC
	ld		(_PB_esc),a				; mark the next character as non-escaped
	jr		._PB_TXFinishCharacter	; finishs the character that was started last int

._PB_TXNoEsc: ; no escaped character was pending - get a character to transmit
	ld		hl,(_PB_ptr)
	ld		a,(hl)					; get the next character to send...
	inc	hl
	ld		(_PB_ptr),hl				; increment the pointer and store it again

	ld		hl,(_PB_checksum)		; add the new character to the running checksum
	call	_PB_Checksum
	ld		(_PB_checksum),hl		; save the new checksum

	cp		TC_FRAMING_ESC
	jr		z,._PB_TXEsc				; is was a ESC character - escape it
	cp		TC_FRAMING_START
	jr		z,._PB_TXEsc				; is was a START character - escape it

	ld		c,a
._PB_TXSendChar:
	call	._PB_Write
	jr		nz,._PB_TXSendChar		; loop while sending the data character
	jr		._PB_TXFinishCharacter

._PB_TXEsc: ; escape the character
	xor	0x20
	ld		(_PB_esc),a				; save the escaped character for next time
	ld		c,TC_FRAMING_ESC
._PB_TXSendEscChar:
	call	._PB_Write
	jr		nz,._PB_TXSendEscChar	; loop while sending the ESC char
	; fall through to the FinishCharacter section
	ret	; all done for now...

._PB_TXFinishCharacter:
	ld		hl,(_PB_length)		; get the length remaining
	dec	hl
	ld		(_PB_length),hl		; store length-1
	ld		a,h
	or		l
	ret	nz							; return if the length is still >0

	ld		hl,(_PB_State)
	jp		(hl)						; a section is done - jump to the proper handler

._PB_SendHeaderChecksum: ; send the header_checksum, from the current running-checksum
	ld		hl,(_PB_checksum)
	ld		(_PB_Header+header_checksum),hl	; store the header_checksum
	ld		hl,2
	ld		(_PB_length),hl		; store the length of the header_checksum
	ld		hl,._PB_SendBody
	ld		(_PB_State),hl
	ret

._PB_SendBody: ; send the body of the packet, if any
	ld		hl,(_PB_Header+length)
	ld		a,h
	or		l
	jr		z,._PB_SendFooter		; is the length 0? if so, skip to the footer
	ld		(_PB_length),hl		; store the length of the body to send
	ld		hl,_PB_Buffer
	ld		(_PB_ptr),hl			; point at the data-buffer, for the body-portion
	ld		hl,._PB_SendFooter
	ld		(_PB_State),hl
	ret

._PB_SendFooter: ; generate the checksum out of the running-checksum, and send the footer
	ld		hl,(_PB_checksum)		; get the running-checksum
	ld		(_PB_Footer+checksum),hl	; ...and store it in the footer
	ld		hl,_PB_Footer
	ld		(_PB_ptr),hl			; send out of the footer
	ld		hl,TC_FOOTER_SIZE
	ld		(_PB_length),hl		; set the length of the footer
	ld		hl,._PB_SendDone
	ld		(_PB_State),hl			; when finished, move to the DONE state
	ret

._PB_SendDone: ; all done sending! reset everything and move back to the beginning!
	xor	a
	ld		(_PB_waittx),a			; mark that the TX has finished
	call	_PB_Init
	ret

_PB_getDoublerSetting::
	; this function is a modified duplicate of the functions _getDoublerSetting
	; found in CPUPARAM.LIB starting with DynC 7.25.  It was copied here since
	; the BIOS normally receives a macro called _CPU_ID_ containing the CPU type
	; and revision number, but the pilot BIOS reads that information itself.
	xor	a							; clear carry flag
	ld		a, (PB_DIV19200)

	ld		de, 0x0100				; Rabbit 3000 CPU ID
	ld		hl, (_PB_CPUID)		; target's CPU ID
	sbc	hl, de					; is target's CPU ID >= Rabbit 3000 CPU ID?
	jr		nc, ._PB_gds_R3000	; yes, go get Rabbit 3000 doubler setting

	; Rabbit 2000-specific section
._PB_gds_R2000:
	; Rabbit 2000 products automatically have the clock doubler
	;  disabled if the base oscillator is more than 12.9024 MHz.
	ld		hl, 7					; 20 nS low time setting
	cp		13						; is base oscillator 7.3728 MHz or lower?
	jr		c, ._PB_gds_done	; yes, go return 20 nS low time setting
	ld		L, 4					; 14 nS low time setting
	cp		19						; is base oscillator 11.0592 MHz or lower?
	jr		c, ._PB_gds_done	; yes, go return 14 nS low time setting
	ld		L, 2					; 10 nS low time setting
	cp		22						; is base oscillator 12.9024 MHz or lower?
	jr		c, ._PB_gds_done	; yes, go return 10 nS low time setting
	ld		L, 0					; disabled clock doubler setting
	jr		._PB_gds_done		; > 12.9024 MHz, go return disabled doubler setting

	; Rabbit 3000-specific section
._PB_gds_R3000:
	; Rabbit 3000 products automatically have the clock doubler
	;  disabled if the base oscillator is more than 25.8048 MHz.
	ld		hl, 15				; 20 nS low time setting
	cp		7						; is base oscillator 3.6864 MHz or lower?
	jr		c, ._PB_gds_done	; yes, go return 20 nS low time setting
	ld		L, 10					; 15 nS low time setting
	cp		25						; is base oscillator 14.7456 MHz or lower?
	jr		c, ._PB_gds_done	; yes, go return 15 nS low time setting
	ld		L, 4					; 9 nS low time setting
	cp		31						; is base oscillator 18.432 MHz or lower?
	jr		c, ._PB_gds_done	; yes, go return 9 nS low time setting
	ld		L, 2					; 7 nS low time setting
	cp		37						; is base oscillator 22.1184 MHz or lower?
	jr		c, ._PB_gds_done	; yes, go return 7 nS low time setting
	ld		L, 1					; 6 nS low time setting
	cp		43						; is base oscillator 25.8048 MHz or lower?
	jr		c, ._PB_gds_done	; yes, go return 6 nS low time setting
	ld		L, 0					; disabled clock doubler setting
									; > 25.8048 MHz, return disabled doubler setting
._PB_gds_done:
	ret


#ifdef PB_DEBUG_SERB
; serial B debug info output tools
_initserial_dbg::
;	ld		a, 3						;; 56kb @ 7 MHz
;	ld		a, 4						;; 56kb @ 9.2 MHz
;	ld		a, 16						;; 56kb @ 11 MHz
;	ld		a, 7						;; 56kb @ 14 MHz
;	ld		a, 11						;; 56kb @ 22 MHz
;	ld		a, 15						;; 56kb @ 29 MHz
;
	ld		a, (PB_DIV19200)		;; 19k2 @ any MHz
	ioi	ld (TAT5R), a
	ld		a, 0xF0
	ioi	ld (PCFR), a
	xor	a
	ioi	ld (SBCR), a
	ret

_sendchar_dbg::
	push	af
.wait:
	ioi	ld a, (SBSR)
	bit	2, a
	jr		nz, .wait
	pop	af
	ioi	ld	(SBDR), a
	ret

_sendcrnl_dbg::
	ld		a, 10
	call	_sendchar_dbg
	ld		a, 13
	call	_sendchar_dbg
	ret

_sendbyte_dbg::
	push	af
	and	0xF0
	srl	a
	srl	a
	srl	a
	srl	a
	add	a, '0'
	cp		'9'+1
	jr		c, .nohex1
	add	a, 7
.nohex1:
	call	_sendchar_dbg

	pop	af
	and	0x0F
	add	a, '0'
	cp		'9'+1
	jr		c, .nohex2
	add	a, 7
.nohex2:
	call	_sendchar_dbg
	ret
#endif

_PB_EndOfpilot_c::
#endasm