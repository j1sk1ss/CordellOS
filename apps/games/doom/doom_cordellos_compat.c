#include "doom_cordellos_compat.h"

#undef fopen
#undef fclose
#undef fread
#undef fwrite
#undef fseek
#undef ftell
#undef feof
#undef ferror
#undef fflush
#undef fprintf
#undef vfprintf
#undef fscanf
#undef fgets
#undef remove
#undef rename
#undef mkdir
#undef getenv
#undef exit
#undef atexit
#undef sscanf

#include <fslib.h>
#include <graphics.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct FILE {
    int ci;
    int pos;
    int size;
    int eof;
    int writable;
};

static FILE _stdin = { -1, 0, 0, 0, 0 };
static FILE _stdout = { -2, 0, 0, 0, 1 };
static FILE _stderr = { -3, 0, 0, 0, 1 };

FILE* stdin = &_stdin;
FILE* stdout = &_stdout;
FILE* stderr = &_stderr;

static void normalize_path(const char* path, char* output, size_t output_size)
{
    size_t i = 0;

    if (output_size == 0) {
        return;
    }

    while (path != NULL && path[i] != '\0' && i < output_size - 1) {
        output[i] = path[i] == '/' ? '\\' : path[i];
        i++;
    }

    output[i] = '\0';
}

static int has_directory(const char* path)
{
    while (path != NULL && *path != '\0') {
        if (*path == '\\' || *path == '/') {
            return 1;
        }
        path++;
    }

    return 0;
}

FILE* dg_fopen(const char* path, const char* mode)
{
    char normalized_path[256];
    char doom_path[256];
    int writing = mode != NULL && mode[0] == 'w';

    normalize_path(path, normalized_path, sizeof(normalized_path));

    if (!cexists(normalized_path) && !has_directory(normalized_path)) {
        snprintf(doom_path, sizeof(doom_path), "HOME\\APPS\\GAMES\\DOOM\\%s", normalized_path);
        normalize_path(doom_path, normalized_path, sizeof(normalized_path));
    }

    if (writing && !cexists(normalized_path)) {
        return NULL;
    }

    int ci = copen(normalized_path);
    if (ci < 0) {
        return NULL;
    }

    CInfo_t info;
    cstat(ci, &info);

    FILE* stream = malloc(sizeof(FILE));
    if (stream == NULL) {
        cclose(ci);
        return NULL;
    }

    stream->ci = ci;
    stream->pos = 0;
    stream->size = info.size;
    stream->eof = 0;
    stream->writable = writing;
    return stream;
}

int dg_fclose(FILE* stream)
{
    if (stream == NULL || stream == stdin || stream == stdout || stream == stderr) {
        return 0;
    }

    cclose(stream->ci);
    free(stream);
    return 0;
}

size_t dg_fread(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    if (stream == NULL || ptr == NULL || size == 0 || nmemb == 0) {
        return 0;
    }

    int bytes = (int)(size * nmemb);
    if (stream->pos + bytes > stream->size) {
        bytes = stream->size - stream->pos;
        stream->eof = 1;
    }

    if (bytes <= 0) {
        stream->eof = 1;
        return 0;
    }

    fread(stream->ci, stream->pos, ptr, bytes);
    stream->pos += bytes;
    return bytes / size;
}

size_t dg_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    if (stream == stdout || stream == stderr) {
        const char* text = ptr;
        for (size_t i = 0; i < size * nmemb; i++) {
            cputc(text[i], WHITE, BLACK);
        }
        return nmemb;
    }

    if (stream == NULL || ptr == NULL || size == 0 || nmemb == 0) {
        return 0;
    }

    int bytes = (int)(size * nmemb);
    fwrite(stream->ci, stream->pos, (uint8_t*)ptr, bytes);
    stream->pos += bytes;
    if (stream->pos > stream->size) {
        stream->size = stream->pos;
    }

    return nmemb;
}

int dg_fseek(FILE* stream, long offset, int whence)
{
    if (stream == NULL) {
        return -1;
    }

    int base = 0;
    if (whence == SEEK_CUR) {
        base = stream->pos;
    } else if (whence == SEEK_END) {
        base = stream->size;
    }

    int next = base + (int)offset;
    if (next < 0) {
        next = 0;
    }

    stream->pos = next;
    stream->eof = stream->pos >= stream->size;
    return 0;
}

long dg_ftell(FILE* stream)
{
    return stream == NULL ? -1 : stream->pos;
}

int dg_feof(FILE* stream)
{
    return stream == NULL || stream->eof;
}

int dg_ferror(FILE* stream)
{
    (void)stream;
    return 0;
}

int dg_fflush(FILE* stream)
{
    (void)stream;
    return 0;
}

int dg_fprintf(FILE* stream, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int result = dg_vfprintf(stream, fmt, args);
    va_end(args);
    return result;
}

int dg_vfprintf(FILE* stream, const char* fmt, va_list args)
{
    char buffer[512];
    int result = vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (result < 0) {
        return result;
    }

    dg_fwrite(buffer, 1, strlen(buffer), stream);
    return result;
}

int dg_fscanf(FILE* stream, const char* fmt, ...)
{
    va_list args;
    int assigned = 0;

    if (stream == NULL || fmt == NULL) {
        return 0;
    }

    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == ' ' || *fmt == '\t' || *fmt == '\n') {
            char c;
            while (dg_fread(&c, 1, 1, stream) == 1) {
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    dg_fseek(stream, -1, SEEK_CUR);
                    break;
                }
            }
            fmt++;
            continue;
        }

        if (*fmt != '%') {
            char c;
            if (dg_fread(&c, 1, 1, stream) != 1 || c != *fmt) {
                break;
            }
            fmt++;
            continue;
        }

        fmt++;

        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        char* output = va_arg(args, char*);
        if (output == NULL) {
            break;
        }

        if (*fmt == 's') {
            char c;
            int count = 0;

            while (dg_fread(&c, 1, 1, stream) == 1) {
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    dg_fseek(stream, -1, SEEK_CUR);
                    break;
                }
            }

            while ((width == 0 || count < width) && dg_fread(&c, 1, 1, stream) == 1) {
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    dg_fseek(stream, -1, SEEK_CUR);
                    break;
                }
                output[count++] = c;
            }

            output[count] = '\0';
            if (count == 0) {
                break;
            }
            assigned++;
            fmt++;
            continue;
        }

        if (*fmt == '[') {
            int negate = 0;
            int stop_at_newline = 0;
            char c;
            int count = 0;

            fmt++;
            if (*fmt == '^') {
                negate = 1;
                fmt++;
            }
            if (*fmt == '\n') {
                stop_at_newline = 1;
                fmt++;
            }
            if (*fmt == ']') {
                fmt++;
            }

            while ((width == 0 || count < width) && dg_fread(&c, 1, 1, stream) == 1) {
                int match = stop_at_newline && c == '\n';
                if ((negate && match) || (!negate && !match)) {
                    dg_fseek(stream, -1, SEEK_CUR);
                    break;
                }
                output[count++] = c;
            }

            output[count] = '\0';
            if (count == 0) {
                break;
            }
            assigned++;
            continue;
        }

        break;
    }

    va_end(args);
    return assigned;
}

char* dg_fgets(char* str, int n, FILE* stream)
{
    if (str == NULL || n <= 0 || stream == NULL) {
        return NULL;
    }

    int i = 0;
    while (i < n - 1) {
        char c = 0;
        if (dg_fread(&c, 1, 1, stream) != 1) {
            break;
        }
        str[i++] = c;
        if (c == '\n') {
            break;
        }
    }

    str[i] = '\0';
    return i == 0 ? NULL : str;
}

int dg_remove(const char* path)
{
    if (path != NULL && cexists(path)) {
        rmcontent(path);
    }
    return 0;
}

int dg_rename(const char* oldpath, const char* newpath)
{
    (void)oldpath;
    (void)newpath;
    return -1;
}

int dg_mkdir(const char* path, int mode)
{
    (void)path;
    (void)mode;
    return 0;
}
