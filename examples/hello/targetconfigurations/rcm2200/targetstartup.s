GCDR_VALUE 	.equ	0x07	; clock doubler for 11.0592 base
MB0CR_VALUE	.equ	0xC0
MB1CR_VALUE	.equ	0xC2
MB2CR_VALUE	.equ	0xC5
MB3CR_VALUE	.equ	0xC5

ld	a, #MB0CR_VALUE
ioi
ld	(MB0CR), a

ld	a, #MB1CR_VALUE
ioi
ld	(MB1CR), a
ld	a, #MB2CR_VALUE
ioi
ld	(MB2CR), a

ld	a, #MB3CR_VALUE
ioi
ld	(MB3CR), a

ld	a, #GCDR_VALUE
ioi
ld	(GCDR), a

