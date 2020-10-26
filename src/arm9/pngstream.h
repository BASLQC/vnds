#ifndef PNGSTREAM_H
#define PNGSTREAM_H

#include "common.h"
#include "tcommon/png.h"

class PNGStream {
	private:
		SoundEngine* soundEngine;
		FileHandle* file;
		char filename[MAXPATHLEN];
		bool ready;
		bool error;

		u16* dst;
		u8* dstA;
		u16 rgb[256*192];
		u8  alpha[256*192];
		png_structp png_ptr;
		png_infop info_ptr;
		png_infop end_ptr;

	public:
		int line;
		u16 width, height;

		PNGStream(SoundEngine* soundEngine);
		virtual ~PNGStream();

		bool Start(Archive* archive, const char* filename);
		bool IsStreaming();
		bool IsImageReady();
		bool ReadLine();
		bool Stop();

		bool GetBounds(Archive* archive, const char* filename, u16* w, u16 *h);
		bool Read(Archive* archive, const char* filename, u16* out, u8* alphaOut);

};

#endif
