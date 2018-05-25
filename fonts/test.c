#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "mono16x28.h"

// http://www.win.tue.nl/~aeb/linux/kbd/font-formats-1.html
// xxd -i stuff.bin stuff.h

#define PSF2_MAGIC0     0x72
#define PSF2_MAGIC1     0xb5
#define PSF2_MAGIC2     0x4a
#define PSF2_MAGIC3     0x86

/* bits used in flags */
#define PSF2_HAS_UNICODE_TABLE 0x01

/* max version recognized so far */
#define PSF2_MAXVERSION 0

/* UTF8 separators */
#define PSF2_SEPARATOR  0xFF
#define PSF2_STARTSEQ   0xFE

typedef struct {
        unsigned char magic[4];
        unsigned int version;
        unsigned int headersize;    /* offset of bitmaps in file */
        unsigned int flags;
        unsigned int length;        /* number of glyphs */
        unsigned int charsize;      /* number of bytes for each character */
        unsigned int height, width; /* max dimensions of glyphs */
        /* charsize = height * ((width + 7) / 8) */
} psf2_header_t;

typedef struct {
    psf2_header_t header;
    uint8_t data[];
} psf2_t; 

void printGlyph(psf2_t* font, int gl)
{
    uint8_t* g = font->data + gl * font->header.charsize;
    int bcnt = 0;
    uint8_t b;
    for(int r=0; r < font->header.height; r++) {
        printf("    ");
        for(int c=0; c < font->header.width; c++) {
            if (bcnt == 0) {
                b = *g++;
            }
            putchar((b & 0x80)?'*':'.');
            b <<= 1;
            bcnt = (bcnt + 1) & 7;
        }
        printf("\r\n");
    }
    
}

int main(int argc, char** argv)
{
    psf2_t* font = (psf2_t*)mono16x28_psf;

    printf("Magic: %02X %02X %02X %02X\r\n", font->header.magic[0], font->header.magic[1], font->header.magic[2], font->header.magic[3]);
    printf("Version: %d\r\n", font->header.version);
    printf("Flags: 0x%X\r\n", font->header.flags);
    printf("Glyphs: %d\r\n", font->header.length);
    printf("Charsize: %d\r\n", font->header.charsize);
    printf("Height: %d; Width: %d\r\n", font->header.height, font->header.width);

    for(int gl=0; gl <= font->header.length; gl++) {
        printf("Char No: %d\r\n", gl);
        printGlyph(font, gl);
    }

    return 0;
}