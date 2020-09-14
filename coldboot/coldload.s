; Copyright (c) 2020 Digi International Inc.
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.

; This file is the source for coldload.bin. It has been translated from
; Dynamic C syntax to sdasrab syntax by Philipp Klaus Krause in 2020.
; It can be assembled as follows:
; ~/sdcc-trunk/sdcc/bin/sdasrab -o coldload.s
; sdcc -mr2k --code-loc 0 --no-std-crt0 coldload.rel
; objcopy --input-target=ihex --output-target=binary coldload.ihx coldload_not3.bin
; makecold coldload_not3.bin coldload.bin

DIVADDR       .equ 0x3f00    ; time constant address
REGBIOSFLAG   .equ 0x3f01    ; start bare BIOS flag address
FREQADRS      .equ 0x3f02    ; frequency divisor address
COLDLOADDEBUG .equ 0

RTC0R         .equ 0x02      ; Real Time Clock Byte 0 Register
WDTCR         .equ 0x08      ; Watchdog Timer Control Register
WDTTR         .equ 0x09      ; Watchdog Timer Test Register
DATASEG       .equ 0x12      ; MMU Bank Base Register
SEGSIZE       .equ 0x13      ; MMU Common Bank Area Register
PCFR          .equ 0x55      ; Port C Function Register
TACSR         .equ 0xa0      ; Timer A Control/Status Register
TAT4R         .equ 0xa9      ; Timer A Time Constant 4 Register
SADR          .equ 0xc0      ; Serial Port A Data Register
SASR          .equ 0xc3      ; Serial Port A Status Register
SACR          .equ 0xc4      ; Serial Port A Control Register

;#rcodorg rootcod2 0x0 0x0 0x6000 apply

.area _CODE

premain::
main::
	ld	sp, #(coldloadend+0x200) ; set up stack in low root segment

;  Start of crystal frequency detection
	ld	bc, #0x0000        		; our counter
	ld	de, #0x07ff        		; mask for RTC bits

; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
; !!!!!WARNING: Time critical code for crystal frequency       !
; !!!!!detection begins here. Adding or removing code from the !
; !!!!!following loops will affect the frequency computation.  !
; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

wait_for_zero:
	ioi
	ld	(RTC0R), a                   ; fill RTC registers
	ioi
	ld	hl, (RTC0R)                  ; get lowest two RTC regs
	and	hl, de                       ; mask off bits
	jr	nz, wait_for_zero            ; wait until bits 0-9 are zero

timing_loop:
	inc	bc                           ; increment counter
	push	bc                           ; save counter
	ld	b, #0x98                     ; empirical loop value
		                             ; (timed for 2 wait states)
	ld	hl, #WDTCR
delay_loop:
	ioi
	ld	(hl), #0x5a                  ; hit watchdog
	djnz	delay_loop
	pop	bc                           ; restore counter
	ioi
	ld	(RTC0R), a                   ; fill RTC registers
	ioi
	ld	hl, (RTC0R)                  ; get lowest two RTC regs
	bit	2, h                         ; test bit 10
	jr	z, timing_loop               ; repeat until bit set

; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
; !!!!!Time critical code for crystal frequency detection ends	!
; !!!!!here!                                                   !
; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	ld	h, b
	ld	l, c
	ld	de, #0x0008
	add	hl, de			     ; add 8 (equiv. to rounding up later)

	rr	hl
	rr	hl
	rr	hl
	rr	hl                           ; divide by 16
	ld	a, l                         ; this is our divider!

	dec	a
	ioi
	ld	(TAT4R), a                   ; set timer A4 running at 57600 baud
	inc	a

	ld	b, a
	sla	a
	add	a, b                         ; multiply by 3 to get 19200 baud

	ld	(FREQADRS), a                ; save divisor for later
	dec	a

	ld	(DIVADDR), a                 ; save 19200 baud scaling
	ld	a, #0x01
	ioi
	ld	(TACSR), a                   ; enable timer A with cpuclk/2
	xor	a, a
	ioi
	ld	(SACR), a                    ; set serial port A async, 8 bit, pport C input
	ld	a, #0x51
	ioi
	ld	(WDTTR), a                   ; disable the watchdog timer
	ld	a, #0x54
	ioi
	ld	(WDTTR), a                   ; disable the watchdog timer
	ld	a, #0x40
	ioi
	ld	(PCFR), a

	call	_get_byte
	ld	e, a                         ; pilot BIOS's begin physical address LSB

	call	_get_byte
	ld	d, a                         ; pilot BIOS's begin physical address LSmidB

	call	_get_byte
	ld	c, a                         ; pilot BIOS's begin physical address MSmidB

	call	_get_byte
	ld	b, a                         ; pilot BIOS's begin physical address MSB

	call _get_byte
	ld	l, a                         ; pilot BIOS's size LSB

	call _get_byte
	ld	h, a                         ; pilot BIOS's size MSB

	call _get_byte
	altd
	ld	a, a                         ; store received checksum in a'

	ld	a, e                         ; initialize and calculate local checksum . . .
	add	a, d
	add	a, c
	add	a, b
	add	a, l
	add	a, h
	call	_send_byte                   ; send ack echoing the locally calculated checksum
	nop

	exx
	ld	b, a
	ex	af, af'                      ; get received checksum
	cp	a, b                         ; compare checksums
	jp	nz, timeout                  ; if checksums do not match error out
	exx
	push	hl                           ; save pilot BIOS's size
	ld	h, c                         ; copy pilot BIOS's begin physical address middle
	ld	l, d                         ;  bytes into HL
	rr	hl                           ; shift physical address bits 19:12 into L
	rr	hl
	rr	hl
	rr	hl
	ld	a, l                         ; copy pilot BIOS's physical address bits 19:12 into A
	sub	a, #0x06                     ; calculate dataseg value for pilot at 0x6000 logical
	ioi
	ld	(DATASEG), a
	ld	a, #0xe6                     ; no stack seg (0xE000), put data seg boundary at 0x6000
	ioi
	ld	(SEGSIZE), a

	ld	a, d                         ; copy pilot BIOS's physical address LSmidB into A
	and	a, #0x0f                     ; change upper nibble of
	or	a, #0x60                     ;  LSmidB to 0x6x
	ld	h, a                         ; copy the 0x6xxx logical address into HL
	ld	l, e

	ld	a, l

	pop	de                           ; recover the pilot BIOS's size into DE
	ld	iy, hl                       ; save pilot's logical begin in IY for copy-to-RAM index
	ld	ix, hl                       ;  and in IX for the jump to the pilot BIOS

_wait_for_CC:
	call	_get_byte
	nop
	cp	a, #0xcc                     ; initial pilot BIOS code (flag) byte?
	jr      nz, _wait_for_CC
	xor	a, a
	ld	(iy), a                      ; replace the 0xCC marker with 0x00 (nop)
	inc	iy                           ; increment the copy-to-RAM index
	dec	de                           ; one less byte to copy
	ld	bc, #0xcccc                  ; update the (initially 0x0000) 8-bit Fletcher
		                             ;  checksum value with the 0xCC just received

_load_pilot_loop:
	call	_get_byte
	nop
	ld	(iy), a

	; Use 8-bit Fletcher checksum algorithm.  See RFC1145 for more info.
	add	a, b
	adc	a, #0x00
	ld	b,a                          ; A = A + D[i]
	add	a, c
	adc	a, #0x00
	ld	c,a                          ; B = B + A

	inc	iy                           ; increment the copy-to-RAM index
	dec	de                           ; one less byte to copy
	bool	hl
	ld	l, h                         ; zero hl
	or	hl, de                       ; check remaining size of pilot
	jr	nz, _load_pilot_loop         ; repeat until size bytes are received

	ld	a, c                         ; send LSB of pilot BIOS's Fletcher checksum
	call	_send_byte
	nop
	ld	a, b                         ; send MSB of pilot BIOS's Fletcher checksum
	call	_send_byte
	nop

;ioi	ld	(WDTTR), a                   ; reenable the watchdog timer

	jp	(ix)                         ; start running pilot bios

_get_byte::
pollrxbuf:
	ioi
	ld	a, (SASR)                    ; check byte receive status
	bit	7, a
	jr	z, pollrxbuf                 ; wait until byte received
	ioi
	ld	a, (SADR)                    ; get byte
	ret

; Must not destroy register A!!
; destroys hl'
_send_byte::
	exx
polltxbuf:
	ld	hl, #SASR
	ioi
	bit	3, (hl)
	jr	nz, polltxbuf                ; wait for serial port A not bus
	ioi
	ld	(SADR), a                    ; send byte
	exx
	ret

timeout::
	ld	e, #0x55
	jr	timeout
coldloadend::

;#if COLDLOADDEBUG
;ledchg::
;	push	hl
;	push	af
;	ld	a, #0x84
;	ioi
;	ld	(SPCR), a
;	ld	hl, WDTCR
;	ioi
;	ld	(hl), #0x5a			; hit watchdog
;	pop	af
;	ioi
;	ld	(PADR), a
;	pop	hl
;	ret
;justloop::
;	ld	a, #0x5a
;	ioi
;	ld	(WDTCR), a			; hit watchdog
;	jr	justloop
;#endif

