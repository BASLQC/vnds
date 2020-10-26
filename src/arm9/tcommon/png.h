#ifndef T_PNG_H
#define T_PNG_H

#include <png.h>
#include "common.h"

extern void (*pngErrorCallback) (png_structp png_ptr, png_const_charp error_msg);
extern void (*pngWarningCallback) (png_structp png_ptr, png_const_charp warning_msg);

bool pngGetBounds(char* data, int dataL, u16* w, u16* h);
bool pngLoadImage(char* data, int dataL, u16* out, u8* alphaOut, int w, int h);
bool pngSaveImage(const char* path, u16* data, int width, int height);

#endif
