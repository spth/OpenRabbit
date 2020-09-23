;--------------------------------------------------------------------------
;  crt0.s - Generic crt0.s for a rabbit 2000
;	derived from "Generic crt0.s for a Z80"
;
;  Copyright (C) 2000, Michael Hope
;  Modified for rabbit by Leland Morrison 2011
;
;  This library is free software; you can redistribute it and/or modify it
;  under the terms of the GNU General Public License as published by the
;  Free Software Foundation; either version 2, or (at your option) any
;  later version.
;
;  This library is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License 
;  along with this library; see the file COPYING. If not, write to the
;  Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
;   MA 02110-1301, USA.
;
;  As a special exception, if you link this library with other files,
;  some of which are compiled with SDCC, to produce an executable,
;  this library does not by itself cause the resulting executable to
;  be covered by the GNU General Public License. This exception does
;  not however invalidate any other reasons why the executable file
;   might be covered by the GNU General Public License.
;--------------------------------------------------------------------------

        .module crt0
       	.globl	_main

GCSR		.equ	0x00 ; Global control / status register
WDTTR		.equ	0x09 ; Watchdog timer test register
MMIDR		.equ	0x10
STACKSEG	.equ	0x11
DATASEG		.equ	0x12
SEGSIZE		.equ	0x13
MB0CR		.equ	0x14 ; Memory Bank 0 Control Register
MB1CR		.equ	0x15 ; Memory Bank 1 Control Register
MB2CR		.equ	0x16 ; Memory Bank 2 Control Register
MB3CR		.equ	0x17 ; Memory Bank 3 Control Register

	.area	_HEADER (ABS)

	; Reset vector - assuming smode0 and smode1 input pins are grounded
	.org 	0

	; setup internal interrupts
	ld	a, #1
	ld	iir, a

	; disable watchdog
	ld	a, #0x51;
	ioi
	ld	(WDTTR), a
	ld	a, #0x54;
	ioi
	ld	(WDTTR), a

	; normal oscillator, processor and peripheral from main clock
	; no periodic interrupt
	ld	a, #0x08
	ioi
	ld	(GCSR), a


	; Initialize target hardware (MBxCR, GCDR).  We haven't mapped RAM
	; or the stack yet, so we cannot `call` we need to jump and have
	; that code jump back.
	jp	target_init
init_complete::

	ld	a, #0xD6	; stack at 0xD000, data at 0x6000
	ioi
	ld	(SEGSIZE), a

	ld	a, #0x7B
	ioi
	ld	(DATASEG), a	; data segment at 7B:6000 = 0x81000

	ld	a, #0x73
	ioi
	ld	(STACKSEG), a	; stack base at 73:D000 = 0x80000

	; Set stack pointer directly above top of RAM.
	ld	sp, #0xe000

	; Initialise global variables
	call	gsinit

	call	_main
	jp	_exit

	; Periodic interrupt
	.org	0x100
	push	af
	ioi
	ld	a, (GCSR) ; clear interrupt
	pop	af
	reti

	; rst 0x10
	.org	0x120
	ret

	; rst 0x18
	.org	0x130
	ret

	; rst 0x20
	.org	0x140
	ret

	; rst 0x28
	.org	0x150
	ret

	; rst 0x38
	.org	0x170
	ret

	; slave port
	.org	0x180
	reti

	; timer a
	.org	0x1a0
	reti

	; timer b
	.org	0x1b0
	reti

	; serial port a
	.org	0x1c0
	reti

	; serial port b
	.org	0x1d0
	reti

	; serial port c
	.org	0x1e0
	reti

	; serial port d
	.org	0x1f0
	reti

	.org	0x200

	;; Ordering of segments for the linker.
	.area	_HOME
	.area	_CODE
	.area	_INITIALIZER
	.area   _GSINIT
	.area   _GSFINAL

	.area	_DATA
	.area	_INITIALIZED
	.area	_BSEG
	.area   _BSS
	.area   _HEAP

	.area   _CODE
_exit::
	;; Exit - special code to the emulator
	ld	a,#0
	rst     #0x28
1$:
	;halt		; opcode for halt used for 'altd' on rabbit processors
	jr	1$

	.area   _GSINIT
gsinit::
	ld	bc, #l__INITIALIZER
	ld	a, b
	or	a, c
	jr	Z, gsinit_next
	ld	de, #s__INITIALIZED
	ld	hl, #s__INITIALIZER
	ldir
gsinit_next:

	.area   _GSFINAL
	ret

