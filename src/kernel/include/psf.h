#ifndef PSF_H_
#define PSF_H_

#include <stdint.h>

#include "stdlib.h"
#include "fslib.h"

// https://wiki.osdev.org/PC_Screen_Font

#define PSF1_FONT_MAGIC 0x0436
#define PSF_FONT_MAGIC  0x864ab572

#define VERSION_PSF1    0x01
#define VERSION_PSF2    0x02


typedef struct {
    uint16_t magic; // Magic bytes for identification.
    uint8_t fontMode; // PSF font mode.
    uint8_t characterSize; // PSF character size.
} PSF1_Header;

typedef struct {
    uint32_t magic;         /* magic bytes to identify PSF */
    uint32_t version;       /* zero */
    uint32_t headersize;    /* offset of bitmaps in file, 32 */
    uint32_t flags;         /* 0 if there's no unicode table */
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
} PSF_font;


uint8_t* PSF_get_glyph(uint8_t symbolnumber, uint8_t version);
uint8_t _psf_get_version(char *_font_structure);
uint32_t _psf_get_width(uint8_t version);
uint32_t _psf_get_height(uint8_t version);

#endif