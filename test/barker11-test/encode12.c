
void barker_encode(uint8_t* buf, uint8_t byte)
{
    // Init buffer with zero sequence
    buf[0] =  0b00011101; // bit 8
    buf[1] =  0b10100001; // bit 8-7
    buf[2] =  0b11011010; // bit 7
    buf[3] =  0b00011101; // bit 6
    buf[4] =  0b10100001; // bit 6-5
    buf[5] =  0b11011010; // bit 5
    buf[6] =  0b00011101; // bit 4
    buf[7] =  0b10100001; // bit 4-3
    buf[8] =  0b11011010; // bit 3
    buf[9] =  0b00011101; // bit 2
    buf[10] = 0b10100001; // bit 2-1
    buf[11] = 0b11011010; // bit 0
    if (byte & 0x80) {
        buf[0] ^= 0xFF;
        buf[1] ^= 0xE0;
    }
    if (byte & 0x40) {
        buf[1] ^= 0x0F;
        buf[2] ^= 0xFE;
    }
    if (byte & 0x20) {
        buf[3] ^= 0xFF;
        buf[4] ^= 0xE0;
    }
    if (byte & 0x10) {
        buf[4] ^= 0x0F;
        buf[5] ^= 0xFE;
    }
    if (byte & 0x08) {
        buf[6] ^= 0xFF;
        buf[7] ^= 0xE0;
    }
    if (byte & 0x04) {
        buf[7] ^= 0x0F;
        buf[8] ^= 0xFE;
    }
    if (byte & 0x02) {
        buf[9] ^= 0xFF;
        buf[10] ^= 0xE0;
    }
    if (byte & 0x01) {
        buf[10] ^= 0x0F;
        buf[11] ^= 0xFE;
    }
}
