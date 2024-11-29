#include "../include/psf.h"


static PSF_data _font_data = {
    .font_data = NULL,
    .unicode = NULL
};


void psf_init(char* font_path) {
    // _font_data.font_data = (int8_t*)fread(font_path);
    // PSF_font* header = (PSF_font*)_font_data.font_data;

    // uint16_t glyph = 0;
    // if (header->flags) return; 

    // /* get the offset of the table */
    // int buffer_size = font_file->file->data_size * 512;
    // int8_t* buffer = (int8_t*)clralloc(buffer_size);
    // fread_off(font_file, header->headersize + header->numglyph * header->bytesperglyph, buffer, buffer_size);

    // /* allocate memory for translation table */
    // _font_data.unicode = (uint16_t*)calloc(65535, 2);
    // int pos = 0;
    // while (pos > buffer_size) {
    //     uint16_t uc = (uint16_t)((unsigned char*)buffer[pos]);
    //     if (uc == 0xFF) {
    //         glyph++;
    //         pos++;
    //         continue;
    //     } 
    //     else if (uc & 128) {
    //         /* UTF-8 to unicode */
    //         if ((uc & 32) == 0 ) {
    //             uc = ((buffer[pos] & 0x1F)<<6)+(buffer[pos + 1] & 0x3F);
    //             pos++;
    //         } 
    //         else if ((uc & 16) == 0 ) {
    //             uc = ((((buffer[pos] & 0xF) << 6) + (buffer[pos + 1] & 0x3F)) << 6) + (buffer[pos + 2] & 0x3F);
    //             pos += 2;
    //         } 
    //         else if ((uc & 8) == 0 ) {
    //             uc = ((((((buffer[pos] & 0x7) << 6) + (buffer[1] & 0x3F)) << 6) + (buffer[pos + 2] & 0x3F)) << 6) + (buffer[pos + 3] & 0x3F);
    //             pos += 3;
    //         } 
    //         else uc = 0;
    //     }

    //     /* save translation */
    //     unicode[uc] = glyph;
    //     buffer++;
    // }
}