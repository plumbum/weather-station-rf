
\res MCU: STM8L051

\res export CLK_PCKENR1
\res export CLK_PCKENR2 

\res export DMA1_GCSR
\res export DMA1_GIR1
\res export DMA1_C2CR
\res export DMA1_C2SPR
\res export DMA1_C2NDTR
\res export DMA1_C2PARH
\ \res export DMA1_C2PARL
\res export DMA1_C2M0ARH
\ \res export DMA1_C2M0ARL


\res export SPI1_CR1
\res export SPI1_CR2
\res export SPI1_ICR
\res export SPI1_SR
\res export SPI1_DR
\ \res export SPI1_CRCPR
\ \res export SPI1_RXCRCR
\ \res export SPI1_TXCRCR

\res export PB_ODR
\res export PB_DDR
\res export PB_CR1

\res export USART1_BRR1
\res export CLK_CKDIVR

#require ]C!
#require ]B!

: clk2mhz ( -- )
    $0D00 USART1_BRR1 !
    %011  CLK_CKDIVR C!
;

\ ------------------------------------------------------------------------------
\ Send byte as 11-chip [Barker code](https://en.wikipedia.org/wiki/Barker_code) via SPI.
\ 11-chip Barker sequence: +1 +1 +1 −1 −1 −1 +1 −1 −1 +1 −1 (Sidelobe level ratio -20.8dB).
\ This code use SPI in transmit only master mode and DMA channel 2 to evenly data flow.

\ Init SPI and DMA.
\ br - SPI baudrate.
: bark-init ( br -- )
    \ Init GPIO
    [ 1 PB_CR1 6 ]B! \ SPI1_MOSI Push-pull/Pull-up
    \ The SPI CLK is not used, but can not be disabled by software.
    [ 1 PB_CR1 5 ]B! \ SPI1_CLK Push-pull/Pull-up

    \ Init SPI
    [ 1 CLK_PCKENR1 4 ]B! \ Enable SPI clock
    $07 AND
    2* 2* 2*    \ 3 left shifts
    SPI1_CR1 C! \ Set baudrate
    [ %11000011 SPI1_CR2 ]C! \ BDM, BDOE, SSM, SSI. Transmit-only mode, Internal slave master
    [ 1 SPI1_CR1 2 ]B! \ Master mode
    [ 1 SPI1_CR1 6 ]B! \ SPI enable

    \ Init DMA chan 2
    [ 1 CLK_PCKENR2 4 ]B! \ DMA clock enable
    [ 1 DMA1_GCSR ]C! \ Global DMA enable
    \ DMA channel 2. SPI1_TX 
    [ %00101000 DMA1_C2CR  ]C! \ Addr inc, mem->periph
    [ %00100000 DMA1_C2SPR ]C! \ Priority high
    SPI1_DR DMA1_C2PARH ! \ Target periph address

    [ 1 SPI1_ICR  1 ]B! \ TXDMAEN - TX DMA Enable
;

\ Wait while DMA busy.
: bark-wait ( -- )
    [ $720E , DMA1_C2SPR , $FB C, ]
;

: bark-busy ( -- f )
    [
      $905F ,                       \ [ 1]         CLRW    Y
      $720F , DMA1_C2SPR , $00 C,   \ [ 2]         BTJF    DMA1_C2SPR, #7, 1$
      $9059 ,                       \ [ 2] 1$:     RLCW    Y
      $5A C,                        \ [ 2]         DECW    X
      $5A C,                        \ [ 2]         DECW    X
      $FF C,                        \ [ 2]         LDW     (X),Y
    ]
;

\ Send buffer to SPI.
: bark-send ( n addr -- )
    [ 0 DMA1_C2CR 0 ]B! \ DMA channel 2 disable
    DMA1_C2M0ARH  ! \ Source memory
    DMA1_C2NDTR  C! \ Length in bytes
    [ 1 DMA1_C2CR 0 ]B! \ DMA channel 2 enable, start transmission
;

\ Buffer to create send byte sequence. variable 2 byte + allot 9+1 bytes. Last byte must be 0, for SPI MOSI off.
variable bark-buf $A allot
variable bark-tmp

\ Store to buffer sequense of 11 bytes to send 8 zero bits (one byte).
\ %11100010010 \ Bit 1
\ %00011101101 \ Bit 0
: bark-buf0 ( -- )
    [ %00011101 bark-buf $0 + ]C! \  0 [77777777] bit
    [ %10100011 bark-buf $1 + ]C! \  1 [77766666] bit
    [ %10110100 bark-buf $2 + ]C! \  2 [66666655] bit
    [ %01110110 bark-buf $3 + ]C! \  3 [55555555] bit
    [ %10001110 bark-buf $4 + ]C! \  4 [54444444] bit
    [ %11010001 bark-buf $5 + ]C! \  5 [44443333] bit
    [ %11011010 bark-buf $6 + ]C! \  6 [33333332] bit
    [ %00111011 bark-buf $7 + ]C! \  7 [22222222] bit
    [ %01000111 bark-buf $8 + ]C! \  8 [22111111] bit
    [ %01101000 bark-buf $9 + ]C! \  9 [11111000] bit
    [ %11101101 bark-buf $A + ]C! \ 10 [00000000] bit
    [ 0         bark-buf $B + ]C! \ Final zero byte for SPI MOSI off.
;

\ Encode byte to Barker code sequence.
: bark-enc ( b addr -- )
    [
      $89  C,                       \ [ 2]          PUSHW   X
      $E603 ,                       \ [ 1]          LD      A, (#3,X)
      $FE  C,                       \ [ 2]          LDW     X, (X)
      $C7 C, bark-tmp ,             \ [ 1]          LD      TMP_BYTE, A
       \ ; bit 7
      $720F , bark-tmp , $07 C,     \ [ 2]          BTJF    TMP_BYTE, #7, 7$
      $73  C,                       \ [ 1]          CPL     (X)
      $A6E0 ,                       \ [ 1]          LD      A, #0xE0
      $E801 ,                       \ [ 1]          XOR     A, (#1,X)
      $E701 ,                       \ [ 1]          LD      (#1,X), A
       \       7$:
       \ ; bit 6
      $720D , bark-tmp , $0C C,     \ [ 2]          BTJF    TMP_BYTE, #6, 6$
      $A61F ,                       \ [ 1]          LD      A, #0x1F
      $E801 ,                       \ [ 1]          XOR     A, (#1,X)
      $E701 ,                       \ [ 1]          LD      (#1,X), A
      $A6FC ,                       \ [ 1]          LD      A, #0xFC
      $E802 ,                       \ [ 1]          XOR     A, (#2,X)
      $E702 ,                       \ [ 1]          LD      (#2,X), A
       \       6$:
       \ ; bit 5
      $720B , bark-tmp , $0E C,     \ [ 2]          BTJF    TMP_BYTE, #5, 5$
      $A603 ,                       \ [ 1]          LD      A, #0x03
      $E802 ,                       \ [ 1]          XOR     A, (#2,X)
      $E702 ,                       \ [ 1]          LD      (#2,X), A
      $6303 ,                       \ [ 1]          CPL     (#3,X)
      $A680 ,                       \ [ 1]          LD      A, #0x80
      $E804 ,                       \ [ 1]          XOR     A, (#4,X)
      $E704 ,                       \ [ 1]          LD      (#4,X), A
       \       5$:
       \ ; bit 4
      $7209 , bark-tmp , $0C C,     \ [ 2]          BTJF    TMP_BYTE, #4, 4$
      $A67F ,                       \ [ 1]          LD      A, #0x7F
      $E804 ,                       \ [ 1]          XOR     A, (#4,X)
      $E704 ,                       \ [ 1]          LD      (#4,X), A
      $A6F0 ,                       \ [ 1]          LD      A, #0xF0
      $E805 ,                       \ [ 1]          XOR     A, (#5,X)
      $E705 ,                       \ [ 1]          LD      (#5,X), A
       \       4$:
       \ ; bit 3
      $7207 , bark-tmp , $0C C,     \ [ 2]          BTJF    TMP_BYTE, #3, 3$
      $A60F ,                       \ [ 1]          LD      A, #0x0F
      $E805 ,                       \ [ 1]          XOR     A, (#5,X)
      $E705 ,                       \ [ 1]          LD      (#5,X), A
      $A6FE ,                       \ [ 1]          LD      A, #0xFE
      $E806 ,                       \ [ 1]          XOR     A, (#6,X)
      $E706 ,                       \ [ 1]          LD      (#6,X), A
       \       3$:
       \ ; bit 2
      $7205 , bark-tmp , $0E C,     \ [ 2]          BTJF    TMP_BYTE, #2, 2$
      $A601 ,                       \ [ 1]          LD      A, #0x01
      $E806 ,                       \ [ 1]          XOR     A, (#6,X)
      $E706 ,                       \ [ 1]          LD      (#6,X), A
      $6307 ,                       \ [ 1]          CPL     (#7,X)
      $A6C0 ,                       \ [ 1]          LD      A, #0xC0
      $E808 ,                       \ [ 1]          XOR     A, (#8,X)
      $E708 ,                       \ [ 1]          LD      (#8,X), A
       \       2$:
       \ ; bit 1
      $7203 , bark-tmp , $0C C,     \ [ 2]          BTJF    TMP_BYTE, #1, 1$
      $A63F ,                       \ [ 1]          LD      A, #0x3F
      $E808 ,                       \ [ 1]          XOR     A, (#8,X)
      $E708 ,                       \ [ 1]          LD      (#8,X), A
      $A6F8 ,                       \ [ 1]          LD      A, #0xF8
      $E809 ,                       \ [ 1]          XOR     A, (#9,X)
      $E709 ,                       \ [ 1]          LD      (#9,X), A
       \       1$:
       \ ; bit 0
      $7201 , bark-tmp , $08 C,     \ [ 2]          BTJF    TMP_BYTE, #0, 0$
      $A607 ,                       \ [ 1]          LD      A, #0x07
      $E809 ,                       \ [ 1]          XOR     A, (#9,X)
      $E709 ,                       \ [ 1]          LD      (#9,X), A
      $630A ,                       \ [ 1]          CPL     (#10,X)
       \       0$:
      $85 C,                        \ [ 2]          POPW    X
      $1C C, $0004 ,                \ [ 2]          ADDW    X, #4
    ]
;

clk2mhz

$5200 $0F dump
7 bark-init
$5200 $0F dump

bark-buf0
$A5 bark-buf bark-enc
\ $0C bark-buf bark-send

$5200 $0F dump

bark-buf $0c dump

hex
: t $80 bark-buf bark-send bark-busy . ;
: t1 $508A $80 bark-buf bark-send c@ . ;

