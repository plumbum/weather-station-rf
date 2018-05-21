#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Barker codes

Len 7:  +1 +1 +1 −1 −1 +1 −1	−16.9 dB
Len 11: +1 +1 +1 −1 −1 −1 +1 −1 −1 +1 −1	−20.8 dB
*/

//                BBBBBBBz
#define BARKER7 0b11100100

#define COUNTS 50000000

uint32_t expand(uint8_t b, uint8_t mask)
{
    uint32_t r = 0;
    for(int i=0; i<=7; i++) {
        r = (r<<4) | ((b & 0x80)?mask:0);
        b <<= 1;
    }
    return r;
}

// uint32_t MASK1;
// uint32_t MASK0;
// uint32_t MASK = 0x88888880;

#define MASK  0x88888880
#define MASK1 0x88800800 
#define MASK0 0x00088080 

/*
uint8_t barker_bit(uint8_t bit)
{
    static uint32_t shift_reg = 0;
    static uint8_t skip_bits = 0;

    shift_reg = (shift_reg<<1) | (bit?1:0);
    uint32_t masked = shift_reg & MASK;
    uint32_t t1 = masked ^ MASK1;
    uint32_t t0 = masked ^ MASK0;
    if (skip_bits < TIMEOUT) skip_bits++;
    if (t1 == 0 || (t1 & (t1-1)) == 0) {
        if (skip_bits > 4) {
            skip_bits = 0;
            return 1;
        }
    } else if (t0 == 0 || (t0 & (t0-1)) == 0) {
        if (skip_bits > 4) {
            skip_bits = 0;
            return 0;
        }
    }
    return 0xFF;
}
*/

#define TIMEOUT 48

int barker_byte(uint8_t inbit)
{
    static uint32_t shift_reg = 0;
    static uint8_t chip_cnt = 0;
    static uint8_t out_byte = 0;
    static uint8_t bit_cnt = 0;

    shift_reg = (shift_reg<<1) | (inbit?1:0);
    // uint32_t masked = (shift_reg) & MASK;
    // uint32_t masked = (shift_reg & (shift_reg<<1)) & MASK;
    uint32_t masked = (shift_reg | (shift_reg<<1)) & MASK;
    uint32_t t1 = masked ^ MASK1;
    uint32_t t0 = masked ^ MASK0;
    if (chip_cnt < TIMEOUT) {
        chip_cnt++;
    } else {
        bit_cnt = 0;
    }
    // printf("[%d] %08X 1:%08X 0:%08X\r\n", chip_cnt, shift_reg, t1, t0);
    if (t1 == 0 /* || (t1 & (t1-1)) == 0 */) {
        if (chip_cnt > 4) {
            chip_cnt = 0;
            bit_cnt++;
            out_byte = (out_byte<<1) | 1;
        }
    } else if (t0 == 0 /* || (t0 & (t0-1)) == 0 */) {
        if (chip_cnt > 4) {
            chip_cnt = 0;
            bit_cnt++;
            out_byte = (out_byte<<1) | 0;
        }
    }
    if (bit_cnt >= 8) {
        bit_cnt = 0;
        return out_byte;
    }
    return -1;

}

uint32_t barker_buf[8];


void make_byte(uint8_t b) {
    for(int i=0; i<8; i++) {
        barker_buf[i] = expand((b & 0x80)?(BARKER7):(BARKER7 ^ 0xFE), 15);
    }
}

uint8_t send_byte(uint8_t b) {
    uint8_t b1 = b;
    uint8_t lost = 1;
    int bt;
    for(int i=0; i<8; i++) {
        uint32_t bark = expand((b1 & 0x80)?(BARKER7):(BARKER7 ^ 0xFE), 15);
        b1 <<= 1;
        // printf("%08X:", bark);
        for(int chip=0; chip<32; chip++) {
            uint8_t bit;
            bit = (bark & 0x80000000UL)?1:0;
            if ((rand() % 1000) < 10) {
                // printf("^");
                bit ^= 1;
            }
            bark <<= 1;
            bt = barker_byte(bit);
            if (bt >= 0) {
                printf(" %02X", bt);
                if (b != bt) {
                    printf(" !");
                } else {
                    lost = 0;
                }
            }
        }
    }
    for(int chip=0; chip<256; chip++) {
        bt = barker_byte(rand() & 1);
        if (bt >= 0) {
            printf(" %02X", bt);
            if (b != bt) {
                printf(" !!");
            }
        }
    }

    printf("\r\n");
    return lost;
}

#define BYTES 100000

int main(int argc, char** argv)
{
    int lost = 0;
    // printf("Mask 1:%08x; 0:%08x\r\n", mask1, mask0);
    for(int d=0; d<BYTES; d++) {
        printf("%02X:", d & 0xFF);
        if (send_byte(d & 0xFF)) {
            lost++;
        }
    }
    printf("Lost: %f\r\n", 100.0*(double)lost/BYTES);

    return 0;
}