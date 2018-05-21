#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Barker codes

Len 7:  +1 +1 +1 −1 −1 +1 −1	−16.9 dB
Len 11: +1 +1 +1 −1 −1 −1 +1 −1 −1 +1 −1	−20.8 dB
*/

//                 A9876543210
#define BARKER11 0b111000100100

#define MASK 0b1001001001001001001001001001001
//             AAA9998887776665554443332221110
#define BIT1 0b1001001000000000001000000001000
#define BIT0 (BIT1 ^ MASK)

#define TIMEOUT 48

int barker_byte(uint8_t inbit)
{
    static uint32_t shift_reg = 0;
    static uint8_t chip_cnt = 0;
    static uint8_t out_byte = 0;
    static uint8_t bit_cnt = 0;

    shift_reg = (shift_reg<<1) | (inbit?1:0);
    // uint32_t masked = (shift_reg | (shift_reg<<1)) & MASK;
    uint32_t masked = (shift_reg) & MASK;
    uint32_t t1 = masked ^ BIT1;
    uint32_t t0 = masked ^ BIT0;
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

uint8_t send_byte(uint8_t b) {
    uint8_t b1 = b;
    uint8_t lost = 1;
    int bt;
    for(int i=0; i<8; i++) {
        uint32_t bark = (b1 & 0x80)?(BARKER11):(BARKER11 ^ 0xFFE);
        b1 <<= 1;
        // printf("%08X:", bark);
        for(int chip=0; chip<12; chip++) {
            uint8_t bit;
            bit = (bark & 0x800UL)?1:0;
            bark <<= 1;
            for(int dup=0; dup<3; dup++) {
                uint8_t bit1 = bit;
                if ((rand() % 1000) < 10) {
                    // printf("^");
                    bit ^= 1;
                }
                bt = barker_byte(bit1);
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

#define BYTES 1000000

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