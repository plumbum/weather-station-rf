#include "psf.h"

uint8_t* psf_get_glyph(const psf2_t* font, unsigned int glyph)
{
    return (uint8_t*)(font + font->header.headersize + glyph * font->header.charsize);
}

unsigned int psf_glyph_num(const psf2_t* font, uint32_t encoding)
{
    if (font->header.flags & PSF2_HAS_UNICODE_TABLE) {

    } else {
        
    }

}

uint32_t psf_glyphs_num(const psf2_t* font)
{
    return font->header.length;
}

uint32_t psf_glyph_width(const psf2_t* font)
{
    return font->header.width;
}

uint32_t psf_glyph_height(const psf2_t* font)
{
    return font->header.height;
}
