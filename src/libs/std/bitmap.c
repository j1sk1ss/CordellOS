#include "../include/bitmap.h"


bitmap_t* BMP_create(char* file_path, int screen_x, int screen_y) {
    bitmap_t* ret = (bitmap_t*)clralloc(sizeof(bitmap_t));
    int content = fopen(file_path);
    if (content == -1) {
        printf("File not found\n");
        BMP_unload(ret);
        return NULL;
    }

    uint8_t* header = clralloc(sizeof(bmp_fileheader_t));
    fread(content, 0, header, sizeof(bmp_fileheader_t));

    bmp_fileheader_t* h = (bmp_fileheader_t*)header;
    uint32_t offset = h->bfOffBits;

    free(header);

    uint8_t* info = clralloc(sizeof(bmp_infoheader_t));
    fread(content, sizeof(bmp_fileheader_t), info, sizeof(bmp_infoheader_t));

    bmp_infoheader_t* inf = (bmp_infoheader_t*)info;
    ret->width         = inf->biWidth;
    ret->height        = inf->biHeight;
    ret->header_offset = offset;
    ret->file          = content;
    ret->bpp           = inf->biBitCount;
    ret->x             = screen_x;
    ret->y             = screen_y;

    free(info);
    return ret;
}

void BMP_display(bitmap_t* bmp) {
    if (bmp == NULL) {
        printf("\nInvalid bitmap\n");
        return;
    }

    if (bmp->width > get_resolution_x() || bmp->height > get_resolution_y()) {
        printf("\nInvalid resolution of bitmap %ix%i > %ix%i\n", bmp->width, bmp->height, get_resolution_x(), get_resolution_y());
        return;
    }
    
    uint32_t bytes_per_pixel = bmp->bpp / 8;
    uint32_t line_size       = bmp->width * bytes_per_pixel;
    uint32_t load_size       = LOAD_PART * bytes_per_pixel;

    for (int y = bmp->height - 1; y >= 0; y--) {
        uint32_t offset     = bmp->header_offset + y * line_size;
        uint32_t image_part = 0;
        uint32_t line_part  = 0;

        while (image_part < line_size) {
            uint8_t* bytes = clralloc(load_size);
            fread(bmp->file, offset + image_part, bytes, load_size);

            for (int x = 0; x < LOAD_PART; x++) {
                uint32_t color_index = x * bytes_per_pixel;

                uint32_t blue  = bytes[color_index] & 0xff;
                uint32_t green = bytes[color_index + 1] & 0xff;
                uint32_t red   = bytes[color_index + 2] & 0xff;
                uint32_t alpha = bytes[color_index + 3] & 0xff;

                if (x + line_part < bmp->width) 
                    pput_pixel(
                        x + bmp->x + line_part, 
                        (bmp->height - 1 - y) + bmp->y, 
                        ((alpha << 24) | (red << 16) | (green << 8) | (blue))
                    );

                else break;
            }

            free(bytes);

            image_part += load_size;
            line_part  += LOAD_PART;
        }
    }

    swipe_buffers();
}

void BMP_unload(bitmap_t* bitmap) {
    if (bitmap->file != NULL) FSLIB_unload_content_system(bitmap->file);
    if (bitmap != NULL) free(bitmap);
}