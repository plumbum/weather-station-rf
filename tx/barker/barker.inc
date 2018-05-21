
; --------------------------------------------------------------------------------
; BBUSY:
;         CLR     A
;         BTJF    DMA1_C2SPR, #7, 1$
; 1$:
;         RLC     A
;         DECW    X
;         LD      (X),A
;         CLR     A
;         DECW    X
;         LD      (X),A
;         RET

; ------------------------------------------------------------------------------
; Send byte as 11-chip [Barker code](https://en.wikipedia.org/wiki/Barker_code) via SPI.
; 11-chip Barker sequence: +1 +1 +1 −1 −1 −1 +1 −1 −1 +1 −1 (Sidelobe level ratio -20.8dB).
; This code use SPI in transmit only master mode and DMA channel 2 to evenly data flow.

; --------------------------------------------------------------------------------
;       B-INIT  ( br -- )
;       Init SPI and DMA.
;       br - SPI baudrate.
        .dw     LINK

        LINK =  .
        .db     (6)
        .ascii  "B-INIT"
BARK_INIT:
        ; Init GPIO
        ; SPI1_MOSI Push-pull/Pull-up
        BSET    PB_CR1,#6
        ; The SPI CLK is not used, but can not be disabled by software.
        ; SPI1_CLK Push-pull/Pull-up
        BSET    PB_CR1,#5

        ; Init SPI
        BSET    CLK_PCKENR1, #4 ; Enable SPI clock

        ; Pop baudrate value from stack
        INCW    X
        LD      A, (X)
        INCW    X
        AND     A, #0x07
        SLL     A
        SLL     A
        SLL     A
        LD      SPI1_CR1, A ; Set baudrate
        MOV     SPI1_CR2, #0b11000011 ; BDM, BDOE, SSM, SSI. Transmit-only mode, Internal slave master
        BSET    SPI1_CR1, #2 ; Set master mode
        BSET    SPI1_CR1, #6 ; SPI enable

        ; Init DMA channel 2
		BSET 	CLK_PCKENR2, #4 ; DMA clock enable
		MOV 	DMA1_GCSR, #1 ; DMA global enable
		; Configure DMA channel 2. SPI1_TX
		MOV 	DMA1_C2CR,  #0b00101000 ; Address increment, mem to perif
		MOV 	DMA1_C2SPR, #0b00100000 ; Prioriti high
		; Target periph address
		LDW 	Y, #SPI1_DR
		LDW 	DMA1_C2PARH, Y

		BSET 	SPI1_ICR, #1 ; TXDMAEN - TX DMA Enable

		; Set system clk to 2MHz
		MOV 	CLK_CKDIVR, #3
        LDW     Y, #0x0D00       ; 9600 baud
        LDW     USART1_BRR1, Y

        RET

; --------------------------------------------------------------------------------
;       BARK!  ( n addr -- )
;       Send n bytes from RAM address to SPI via DMA.
        .dw     LINK

        LINK =  .
        .db     (5)
        .ascii  "BARK!"
BARK_SEND:
		BRES 	DMA1_C2SPR, #1 ; Clear transaction complete flag
		BRES 	DMA1_C2CR, #0 ; DMA channel 2 disable
		PUSHW 	X
		LD		A, (#3, X)
		LD 		DMA1_C2NDTR, A
		LDW		X, (X)
		LDW		DMA1_C2M0ARH, X
		BSET 	DMA1_C2CR, #0 ; DMA channel 2 enable, start transmission
		POPW 	X
		ADDW 	X, #4
		RET

; --------------------------------------------------------------------------------
;       B-WAIT  ( -- )
;       Wait until transmission done
        .dw     LINK

        LINK =  .
        .db     (6)
        .ascii  "B-WAIT"
BARK_WAIT:
1$:		BTJT	DMA1_C2SPR, #1, 2$ ; Done if transmit complete
		WFI		; Sleep
		JRA 	1$
2$: 	BRES 	DMA1_C2SPR, #1 ; Clear transaction complete flag
		RET

; --------------------------------------------------------------------------------
;       BARK0  ( addr -- )
;       Fill 12 bytes buffer with sequence for 8 zero bits.
;		11 bytes has data, and 12-th byte always 0.
;		Barker code: %11100010010 \ Bit 1
; 					 %00011101101 \ Bit 0
        .dw     LINK

        LINK =  .
        .db     (3)
        .ascii  "B-0"
BARK_ZERO:
        PUSHW   X
        LDW     X, (X)
        LD      A,#0b00011101 ;  0 [77777777] bit
        LD      (X), A
        LD      A,#0b10100011 ;  1 [77766666] bit
        LD      (#1, X), A
        LD      A,#0b10110100 ;  2 [66666655] bit
        LD      (#2, X), A
        LD      A,#0b01110110 ;  3 [55555555] bit
        LD      (#3, X), A
        LD      A,#0b10001110 ;  4 [54444444] bit
        LD      (#4, X), A
        LD      A,#0b11010001 ;  5 [44443333] bit
        LD      (#5, X), A
        LD      A,#0b11011010 ;  6 [33333332] bit
        LD      (#6, X), A
        LD      A,#0b00111011 ;  7 [22222222] bit
        LD      (#7, X), A
        LD      A,#0b01000111 ;  8 [22111111] bit
        LD      (#8, X), A
        LD      A,#0b01101000 ;  9 [11111000] bit
        LD      (#9, X), A
        LD      A,#0b11101101 ; 10 [00000000] bit
        LD      (#10, X), A
        POPW    X
        INCW    X
        INCW    X
        RET

; --------------------------------------------------------------------------------
;       B-ENC  ( b addr -- )
;       Encode byte to Barker code sequence.
;       Buffer must be prefilled with B-ZERO.
        RamByte BARK_TMP
        .dw     LINK

        LINK =  .
        .db     (5)
        .ascii  "B-ENC"
BARK_ENC:
        PUSHW   X
        LD      A, (#3,X)
        LDW     X, (X)
        LD      BARK_TMP, A

        ; bit 7
        BTJF    BARK_TMP, #7, 7$
        CPL     (X)
        LD      A, #0xE0
        XOR     A, (#1,X)
        LD      (#1,X), A
7$:
        ; bit 6
        BTJF    BARK_TMP, #6, 6$
        LD      A, #0x1F
        XOR     A, (#1,X)
        LD      (#1,X), A
        LD      A, #0xFC
        XOR     A, (#2,X)
        LD      (#2,X), A
6$:
        ; bit 5
        BTJF    BARK_TMP, #5, 5$
        LD      A, #0x03
        XOR     A, (#2,X)
        LD      (#2,X), A
        CPL     (#3,X)
        LD      A, #0x80
        XOR     A, (#4,X)
        LD      (#4,X), A
5$:
        ; bit 4
        BTJF    BARK_TMP, #4, 4$
        LD      A, #0x7F
        XOR     A, (#4,X)
        LD      (#4,X), A
        LD      A, #0xF0
        XOR     A, (#5,X)
        LD      (#5,X), A
4$:
        ; bit 3
        BTJF    BARK_TMP, #3, 3$
        LD      A, #0x0F
        XOR     A, (#5,X)
        LD      (#5,X), A
        LD      A, #0xFE
        XOR     A, (#6,X)
        LD      (#6,X), A
3$:
        ; bit 2
        BTJF    BARK_TMP, #2, 2$
        LD      A, #0x01
        XOR     A, (#6,X)
        LD      (#6,X), A
        CPL     (#7,X)
        LD      A, #0xC0
        XOR     A, (#8,X)
        LD      (#8,X), A
2$:
        ; bit 1
        BTJF    BARK_TMP, #1, 1$
        LD      A, #0x3F
        XOR     A, (#8,X)
        LD      (#8,X), A
        LD      A, #0xF8
        XOR     A, (#9,X)
        LD      (#9,X), A
1$:
        ; bit 0
        BTJF    BARK_TMP, #0, 0$
        LD      A, #0x07
        XOR     A, (#9,X)
        LD      (#9,X), A
        CPL     (#10,X)
0$:
        POPW    X
        ADDW    X, #4
        RET
        
