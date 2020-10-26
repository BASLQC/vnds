#include "png.h"

#include <stdlib.h>

struct PNGReadStruct {
	char* data;
	png_size_t dataL;
	png_size_t offset;
};

static void pngReadFunction(png_structp png_ptr, png_bytep data, png_size_t length) {
	PNGReadStruct* readInfo = (PNGReadStruct*)png_get_io_ptr(png_ptr);

	int copyL = MIN(length, readInfo->dataL-readInfo->offset);
	memcpy(data, readInfo->data + readInfo->offset, copyL);
	readInfo->offset += copyL;
}

void errorCallback(png_structp png_ptr, png_const_charp error_msg) {
	fprintf(stderr, "%s\n", error_msg);
	longjmp(png_ptr->jmpbuf, 1);
}
void warningCallback(png_structp png_ptr, png_const_charp warning_msg) {
	//fprintf(stderr, "%s\n", warning_msg);
}

void (*pngErrorCallback) (png_structp png_ptr, png_const_charp error_msg) = &errorCallback;
void (*pngWarningCallback) (png_structp png_ptr, png_const_charp warning_msg) = &warningCallback;

bool pngGetBounds(char* data, int dataL, u16* w, u16* h) {
	png_structp png_ptr;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr) {
        return false;
    }

    PNGReadStruct readStruct = {data, dataL, 0};
	png_set_read_fn(png_ptr, (void *)&readStruct, pngReadFunction);

	png_voidp error_ptr = png_get_error_ptr(png_ptr);
    png_set_error_fn(png_ptr, error_ptr, pngErrorCallback, pngWarningCallback);

	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return false;
	}

	png_set_sig_bytes(png_ptr, sig_read);
    png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);

    *w = width;
    *h = height;

    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    return true;
}

bool pngLoadImage(char* data, int dataL, u16* out, u8* alphaOut, int w, int h) {
    png_structp png_ptr;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr) {
        return false;
    }

    PNGReadStruct readStruct = {data, dataL, 0};
	png_set_read_fn(png_ptr, (void *)&readStruct, pngReadFunction);

	png_voidp error_ptr = png_get_error_ptr(png_ptr);
    png_set_error_fn(png_ptr, error_ptr, pngErrorCallback, pngWarningCallback);

	u32 sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return false;
	}
    if (setjmp(png_jmpbuf(png_ptr))) { // IO init error
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return false;
    }

	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);

	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

	u32 row[1024];

	u16* dst = out;
	u32* src;
	u32 s;
	if (!alphaOut) {
		for (int y = 0; y < h; y++) {
			png_read_row(png_ptr, (png_byte*)row, png_bytep_NULL);
			src = (u32*)row;
			for (int x = 0; x < w; x++) {
				s = *src;
				*dst = ((s>>24)?BIT(15):0)|((s>>3)&0x001F)|((s>>6)&0x03E0)|((s>>9)&0x7C00);
				src++;
				dst++;
			}
		}
	} else {
		u8* dstA = alphaOut;
		for (int y = 0; y < h; y++) {
			png_read_row(png_ptr, (png_byte*)row, png_bytep_NULL);
			src = (u32*)row;
	        for (int x = 0; x < w; x++) {
	        	s = *src;
				*dst = BIT(15)|((s>>3)&0x001F)|((s>>6)&0x03E0)|((s>>9)&0x7C00);
	        	*dstA = (s>>24);
	        	src++;
	        	dst++;
	        	dstA++;
	        }
	    }
	}

	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    return true;
}

bool pngSaveImage(const char* path, u16* data, int width, int height) {
    FILE *file = fopen(path, "wb");
    if (!file) {
       return false;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
        (png_voidp)NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(file);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(file);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(file);
        return false;
    }

    png_init_io(png_ptr, file);

    png_set_IHDR(png_ptr, info_ptr, width, height,
       8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
       PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	int t = 0;
	png_byte* line = new png_byte[width * 3];
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width * 3; ) {
			line[x++] = ((data[t]    ) & 31) << 3;
			line[x++] = ((data[t]>> 5) & 31) << 3;
			line[x++] = ((data[t]>>10) & 31) << 3;
			t++;
		}
		png_write_row(png_ptr, line);
	}
	delete[] line;

	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(file);
	return true;
}
