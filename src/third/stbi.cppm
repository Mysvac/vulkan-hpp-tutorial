module;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

export module stbi;


export namespace stbi {
    constexpr int VERSION = STBI_VERSION;
    constexpr int DEFAULT = STBI_default;
    constexpr int GREY = STBI_grey;
    constexpr int GREY_ALPHA = STBI_grey_alpha;
    constexpr int RGB = STBI_rgb;
    constexpr int RGB_ALPHA = STBI_rgb_alpha;

    using uc = stbi_uc;
    using us = stbi_us;

    using io_callbacks = stbi_io_callbacks;

    inline uc* load_from_memory(const uc* buffer,const int len, int* x, int* y, int* channels_in_file,const int desired_channels) {
        return stbi_load_from_memory(buffer, len, x, y, channels_in_file, desired_channels);
    }
    inline uc* load_from_callbacks(const io_callbacks* clbk, void* user, int* x, int* y, int* channels_in_file, const int desired_channels) {
        return stbi_load_from_callbacks(clbk, user, x, y, channels_in_file, desired_channels);
    }
    inline us* load_16_from_memory(const uc *buffer, const int len, int *x, int *y, int *channels_in_file, const int desired_channels) {
        return stbi_load_16_from_memory(buffer, len, x, y, channels_in_file, desired_channels);
    }
    inline us* load_16_from_callbacks(const io_callbacks *clbk, void *user, int *x, int *y, int *channels_in_file, const int desired_channels) {
        return stbi_load_16_from_callbacks(clbk, user, x, y, channels_in_file, desired_channels);
    }

#ifndef STBI_NO_STDIO
    inline uc* load(const char* filename, int* x, int* y, int* channels_in_file, const int desired_channels) {
        return stbi_load(filename, x, y, channels_in_file, desired_channels);
    }
    inline uc* load_from_file(FILE* f, int* x, int* y, int* channels_in_file, const int desired_channels) {
        return stbi_load_from_file(f, x, y, channels_in_file, desired_channels);
    }
    inline us* load_16(const char* filename, int* x, int* y, int* channels_in_file, const int desired_channels) {
        return stbi_load_16(filename, x, y, channels_in_file, desired_channels);
    }
    inline us* load_from_file_16(FILE* f, int* x, int* y, int* channels_in_file, const int desired_channels) {
        return stbi_load_from_file_16(f, x, y, channels_in_file, desired_channels);
    }
#endif // STBI_NO_STDIO

#ifndef STBI_NO_GIF
    inline uc* load_gif_from_memory(const uc* buffer,const int len, int **delays, int *x, int *y, int *z, int *comp,const int req_comp) {
        return stbi_load_gif_from_memory(buffer, len, delays, x, y, z, comp, req_comp);
    }
#endif // STBI_NO_GIF

#ifdef STBI_WINDOWS_UTF8
    inline int convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t* input) {
        return stbi_convert_wchar_to_utf8(buffer, bufferlen, input);
    }
#endif // STBI_WINDOWS_UTF8

#ifndef STBI_NO_LINEAR
    inline float* loadf_from_memory(const uc *buffer, const int len, int *x, int *y, int *channels_in_file, const int desired_channels) {
        return stbi_loadf_from_memory(buffer, len, x, y, channels_in_file, desired_channels);
    }
    inline float* loadf_from_callbacks(const stbi_io_callbacks *clbk, void *user, int *x, int *y, int *channels_in_file, const int desired_channels) {
        return stbi_loadf_from_callbacks(clbk, user, x, y, channels_in_file, desired_channels);
    }
    #ifndef STBI_NO_STDIO
        inline float* loadf(const char *filename, int *x, int *y, int *channels_in_file,const  int desired_channels) {
            return stbi_loadf(filename, x, y, channels_in_file, desired_channels);
        }
        inline float* loadf_from_file(FILE *f, int *x, int *y, int *channels_in_file,const  int desired_channels) {
            return stbi_loadf_from_file(f, x, y, channels_in_file, desired_channels);
        }
    #endif // STBI_NO_STDIO
#endif // STBI_NO_LINEAR



#ifndef STBI_NO_HDR
    inline void hdr_to_ldr_gamma(const float gamma) {
        stbi_hdr_to_ldr_gamma(gamma);
    }
    inline void hdr_to_ldr_scale(const float scale) {
        stbi_hdr_to_ldr_scale(scale);
    }
#endif // STBI_NO_HDR

#ifndef STBI_NO_LINEAR
    inline void ldr_to_hdr_gamma(const float gamma) {
        stbi_ldr_to_hdr_gamma(gamma);
    }
    inline void ldr_to_hdr_scale(const float scale) {
        stbi_ldr_to_hdr_scale(scale);
    }
#endif // STBI_NO_LINEAR


    inline int is_hdr_from_callbacks(const io_callbacks *clbk, void *user) {
        return stbi_is_hdr_from_callbacks(clbk, user);
    }
    inline int is_hdr_from_memory(const uc *buffer, const int len) {
        return stbi_is_hdr_from_memory(buffer, len);
    }

#ifndef STBI_NO_STDIO
    inline int is_hdr(const char *filename) {
        return stbi_is_hdr(filename);
    }
    inline int is_hdr_from_file(FILE *f) {
        return stbi_is_hdr_from_file(f);
    }
#endif // STBI_NO_STDIO


    inline const char* failure_reason() {
        return stbi_failure_reason();
    }
    inline void image_free(void *retval_from_stbi_load) {
        stbi_image_free(retval_from_stbi_load);
    }
    inline int info_from_memory(const uc *buffer, const int len, int *x, int *y, int *comp) {
        return stbi_info_from_memory(buffer, len, x, y, comp);
    }
    inline int info_from_callbacks(const io_callbacks *clbk, void *user, int *x, int *y, int *comp) {
        return stbi_info_from_callbacks(clbk, user, x, y, comp);
    }
    inline int is_16_bit_from_memory(const uc *buffer, const int len) {
        return stbi_is_16_bit_from_memory(buffer, len);
    }
    inline int is_16_bit_from_callbacks(const io_callbacks *clbk, void *user) {
        return stbi_is_16_bit_from_callbacks(clbk, user);
    }

#ifndef STBI_NO_STDIO
    inline int info(const char *filename, int *x, int *y, int *comp) {
        return stbi_info(filename, x, y, comp);
    }
    inline int info_from_file(FILE *f, int *x, int *y, int *comp) {
        return stbi_info_from_file(f, x, y, comp);
    }
    inline int is_16_bit(const char *filename) {
        return stbi_is_16_bit(filename);
    }
    inline int is_16_bit_from_file(FILE *f) {
        return stbi_is_16_bit_from_file(f);
    }
#endif // STBI_NO_STDIO

    inline void set_unpremultiply_on_load(const int flag_true_if_should_unpremultiply) {
        stbi_set_unpremultiply_on_load(flag_true_if_should_unpremultiply);
    }
    inline void convert_iphone_png_to_rgb(const int flag_true_if_should_convert) {
        stbi_convert_iphone_png_to_rgb(flag_true_if_should_convert);
    }
    inline void set_flip_vertically_on_load(const int flag_true_if_should_flip) {
        stbi_set_flip_vertically_on_load(flag_true_if_should_flip);
    }
    inline void set_unpremultiply_on_load_thread(const int flag_true_if_should_unpremultiply) {
        stbi_set_unpremultiply_on_load_thread(flag_true_if_should_unpremultiply);
    }
    inline void convert_iphone_png_to_rgb_thread(const int flag_true_if_should_convert) {
        stbi_convert_iphone_png_to_rgb_thread(flag_true_if_should_convert);
    }
    inline void set_flip_vertically_on_load_thread(const int flag_true_if_should_flip) {
        stbi_set_flip_vertically_on_load_thread(flag_true_if_should_flip);
    }

    inline char* zlib_decode_malloc_guesssize(const char *buffer,const int len,const int initial_size, int *outlen) {
        return stbi_zlib_decode_malloc_guesssize(buffer, len, initial_size, outlen);
    }
    inline char* zlib_decode_malloc_guesssize_headerflag(const char *buffer,const int len,const int initial_size, int *outlen, int parse_header) {
        return stbi_zlib_decode_malloc_guesssize_headerflag(buffer, len, initial_size, outlen, parse_header);
    }
    inline char* zlib_decode_malloc(const char *buffer, const int len, int *outlen) {
        return stbi_zlib_decode_malloc(buffer, len, outlen);
    }
    inline int zlib_decode_buffer(char *obuffer, const int olen, const char *ibuffer, const int ilen) {
        return stbi_zlib_decode_buffer(obuffer, olen, ibuffer, ilen);
    }
    inline char* zlib_decode_noheader_malloc(const char *buffer, const int len, int *outlen) {
        return stbi_zlib_decode_noheader_malloc(buffer, len, outlen);
    }
    inline int zlib_decode_noheader_buffer(char *obuffer, const int olen, const char *ibuffer, const int ilen) {
        return stbi_zlib_decode_noheader_buffer(obuffer, olen, ibuffer, ilen);
    }

}


