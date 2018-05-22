#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Barker codes

Len 7:  +1 +1 +1 −1 −1 +1 −1	−16.9 dB
Len 11: +1 +1 +1 −1 −1 −1 +1 −1 −1 +1 −1	−20.8 dB
*/

//                 A9876543210
#define BARKER_1 0b11100010010
#define BARKER_0 0b00011101101

// Translate byte to 11 bytes in buf.
void barker_encode(uint8_t* buf, uint8_t byte)
{
    // Init buffer with zero sequence
    buf[0] =  0b00011101; // 77777777
    buf[1] =  0b10100011; // 77766666
    buf[2] =  0b10110100; // 66666655
    buf[3] =  0b01110110; // 55555555
    buf[4] =  0b10001110; // 54444444
    buf[5] =  0b11010001; // 44443333
    buf[6] =  0b11011010; // 33333332
    buf[7] =  0b00111011; // 22222222
    buf[8] =  0b01000111; // 22111111
    buf[9] =  0b01101000; // 11111000
    buf[10] = 0b11101101; // 00000000
    // Make one bits
    if (byte & 0x80) {
        buf[0] ^= 0xFF;
        buf[1] ^= 0xE0;
    }
    if (byte & 0x40) {
        buf[1] ^= 0x1F;
        buf[2] ^= 0xFC;
    }
    if (byte & 0x20) {
        buf[2] ^= 0x03;
        buf[3] ^= 0xFF;
        buf[4] ^= 0x80;
    }
    if (byte & 0x10) {
        buf[4] ^= 0x7F;
        buf[5] ^= 0xF0;
    }
    if (byte & 0x08) {
        buf[5] ^= 0x0F;
        buf[6] ^= 0xFE;
    }
    if (byte & 0x04) {
        buf[6] ^= 0x01;
        buf[7] ^= 0xFF;
        buf[8] ^= 0xC0;
    }
    if (byte & 0x02) {
        buf[8] ^= 0x3F;
        buf[9] ^= 0xF8;
    }
    if (byte & 0x01) {
        buf[9] ^= 0x07;
        buf[10] ^= 0xFF;
    }
}

#define ENC_MASK 0b1001001001001001001001001001001
//             AAA9998887776665554443332221110
#define ENC_BIT1 0b1001001000000000001000000001000
#define ENC_BIT0 (ENC_BIT1 ^ ENC_MASK)

#define ENC_TIMEOUT 48

#define ENC_CHECK(var)  ((var) == 0 /* || ((var) & ((var)-1)) == 0 */)

int barker_decode(uint8_t inbit)
{
    static uint32_t shift_reg = 0;
    static uint8_t chip_cnt = 0;
    static uint8_t out_byte = 0;
    static uint8_t bit_cnt = 0;

    shift_reg = (shift_reg<<1) | (inbit?1:0);
    // uint32_t masked = (shift_reg | (shift_reg<<1)) & ENC_MASK;
    uint32_t masked = (shift_reg) & ENC_MASK;
    uint32_t t1 = masked ^ ENC_BIT1;
    uint32_t t0 = masked ^ ENC_BIT0;
    if (chip_cnt < ENC_TIMEOUT) {
        chip_cnt++;
    } else {
        bit_cnt = 0;
    }
    // printf("[%d] %08X 1:%08X 0:%08X\r\n", chip_cnt, shift_reg, t1, t0);
    if (ENC_CHECK(t1)) {
        if (chip_cnt > 4) {
            chip_cnt = 0;
            bit_cnt++;
            out_byte = (out_byte<<1) | 1;
        }
    } else if (ENC_CHECK(t0)) {
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

#define NOISE_PROMILLE 10

uint8_t noise(uint8_t b)
{
    b = b?1:0;
    if ((rand() % 1000) < NOISE_PROMILLE) {
        b ^= 1;
    }
    return b;
}

int bd3(uint8_t bit) {
    int bt1 = barker_decode(noise(bit));
    int bt2 = barker_decode(noise(bit));
    int bt3 = barker_decode(noise(bit));
    if (bt1 >=0 ) return bt1;
    if (bt2 >=0 ) return bt2;
    if (bt3 >=0 ) return bt3;
    return -1;
}

void send_empty_noise(void)
{
    for(int i=0; i<1024; i++) {
        barker_decode(rand() & 1);
    }
}

#define BUF_SIZE (11*4+1)

uint8_t send_buf[BUF_SIZE];

uint32_t send_recv(uint32_t b)
{
    send_empty_noise();
    // Send preamble
    for(int i=0; i<16; i++) {
        bd3(1); bd3(1); bd3(1); bd3(1);
        bd3(0); bd3(0); bd3(0); bd3(0);
    }

    // Create data to send
    barker_encode(send_buf,      (b>>24)&0xFF);
    barker_encode(send_buf+11,   (b>>16)&0xFF);
    barker_encode(send_buf+11*2, (b>>8) &0xFF);
    barker_encode(send_buf+11*3, (b>>0) &0xFF);
    send_buf[BUF_SIZE-1] = 0; // Transmitter off

    uint32_t r = 0; // Result
    for(int i=0; i<BUF_SIZE; i++) {
        uint8_t b = send_buf[i];
        for(int j=0; j<8; j++) {
            uint8_t bit = (b & 0x80)?1:0;
            int bt = bd3(bit);
            b <<= 1;
            if (bt >= 0) {
                r = (r<<8) | (bt & 0xFF);
            }
        }

    }
    send_empty_noise();
    return r;
}

#define BYTES 100000

int main(int argc, char** argv)
{
    int lost = 0;
    // printf("Mask 1:%08x; 0:%08x\r\n", mask1, mask0);
    for(uint32_t d=0; d<BYTES; d++) {
        uint32_t data = rand();
        uint32_t resp = send_recv(data);
        if (data != resp) {
            printf("%08X: %08X\r\n", data, resp);
            lost++;
        }
    }
    printf("Lost: %f\r\n", 100.0*(double)lost/BYTES);

    return 0;
}
