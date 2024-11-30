#include "../include/psf.h"


uint8_t _psf_get_version(uint8_t* _font_structure) {
   PSF1_Header*_v1_font = (PSF1_Header*)_font_structure;
   if( _v1_font->magic == PSF1_FONT_MAGIC){
       return VERSION_PSF1;
   }

   PSF_font *_v2_font = (PSF_font *)_font_structure;
   if(_v2_font->magic == PSF_FONT_MAGIC){
       return VERSION_PSF2;
   }

   return 0;
}

uint8_t* PSF_get_glyph(uint8_t* _font_structure, uint8_t symbolnumber) {
    uint8_t version = _psf_get_version(_font_structure);
    if (version == VERSION_PSF1){
        PSF1_Header* loaded_font = (PSF1_Header*)_font_structure;
        return (uint8_t *) loaded_font + sizeof(PSF1_Header) + (symbolnumber * loaded_font->characterSize);
    } 
    else if (version == VERSION_PSF2) {
        PSF_font* loaded_font = (PSF_font*)_font_structure;
        return  (uint8_t*)loaded_font + loaded_font->headersize + (symbolnumber * loaded_font->bytesperglyph);
    }

    return 0;
}

uint32_t _psf_get_width(uint8_t* _font_structure) {
    uint8_t version = _psf_get_version(_font_structure);
    if ( version == VERSION_PSF1) {
        return 8;
    }
    return ((PSF_font*)_font_structure)->width;
}

uint32_t _psf_get_height(uint8_t* _font_structure) {
    uint8_t version = _psf_get_version(_font_structure);
    if ( version == VERSION_PSF1) {
        return ((PSF1_Header*)_font_structure)->characterSize;
    }

    return ((PSF_font*)_font_structure)->height;
}