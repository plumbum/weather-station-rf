#include "psf.h"

uint8_t* psf_get_glyph(const psf2_t* font, int glyph)
{
    return font->data + glyph * font->header.charsize;
}

uint32_t psf_glyph_width(const psf2_t* font)
{
    return font->header.width;
}

uint32_t psf_glyph_height(const psf2_t* font)
{
    return font->header.height;
}

uint32_t psf_glyphs(const psf2_t* font)
{
    return font->header.length;
}
