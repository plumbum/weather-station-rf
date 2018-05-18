; --------------------------------------------------------------------------------
        RamByte BARK_TMP

;       BARK-ENC  ( b addr -- )
;       Encode byte to Barker code sequence
        .dw     LINK

        LINK =  .
        .db     (8)
        .ascii  "BARK-ENC"
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
        
