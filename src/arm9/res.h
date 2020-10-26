#ifndef VNDS_RES_H
#define VNDS_RES_H

#include "pngstream.h"

extern PNGStream* pngStream;

bool loadImage(Archive* archive, const char* filename, u16* rgbOut, u8* alphaOut,
		u16* outW=NULL, u16* outH=NULL);

bool loadImage(Archive* archive, const char* filename, u16** rgbOut, u8** alphaOut,
		u16* outW, u16* outH, bool generate);

void printTime(char* out);

#endif
