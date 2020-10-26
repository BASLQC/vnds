#include "pngstream.h"
#include "sound_engine.h"
#include "tcommon/filehandle.h"

//------------------------------------------------------------------------------

static void pngStreamReadFunction(png_structp png_ptr, png_bytep data, png_size_t length) {
	FileHandle* file = (FileHandle*)png_get_io_ptr(png_ptr);
	if (file) {
		file->Read(data, length);
	}
}

//------------------------------------------------------------------------------

PNGStream::PNGStream(SoundEngine* soundEngine) {
	this->soundEngine = soundEngine;

	filename[0] = '\0';
	ready = false;
	error = false;
	line = 0;

	file = NULL;
	png_ptr = NULL;
}
PNGStream::~PNGStream() {
	Stop();
}

bool PNGStream::Stop() {
	if (file) {
		if (png_ptr) {
			if (setjmp(png_jmpbuf(png_ptr))) { //Exception handler, see libpng manual
				png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
				png_ptr = NULL;

				fhClose(file); file = NULL; error = true;
				return false;
			}

			png_read_end(png_ptr, end_ptr);
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);

			png_ptr = NULL;
			fhClose(file);
			file = NULL;
		}
	}
	return true;
}

bool PNGStream::Start(Archive* archive, const char* name) {
	Stop();

	ready = false;
	error = true;
	line = 0;

	if (strcmp(filename, name) == 0 && error) {
		return false; //Already tried loading this image, but apparently it failed
	}

	png_ptr = NULL;
	strcpy(filename, name);
	dst = rgb;
	dstA = alpha;
	file = fhOpen(archive, name, false);
	if (!file) {
		error = true;
		return false;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fhClose(file); file = NULL; error = true;
		return false;
	}

	png_set_read_fn(png_ptr, (void*)file, pngStreamReadFunction);
	png_voidp error_ptr = png_get_error_ptr(png_ptr);
	png_set_error_fn(png_ptr, error_ptr, pngErrorCallback, pngWarningCallback);

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		png_ptr = NULL;
		fhClose(file); file = NULL; error = true;
		return false;
	}

	end_ptr = png_create_info_struct(png_ptr);
	if (!end_ptr) {
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		png_ptr = NULL;
		fhClose(file); file = NULL; error = true;
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr))) { //Exception handler, see libpng manual
		//png_read_end(png_ptr, end_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
		png_ptr = NULL;

		fhClose(file); file = NULL; error = true;
		return false;
	}

	u32 sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;

	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);

	this->width = MIN(width, 256);
	this->height = MIN(height, 192);

	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

	error = false;
	return true;
}

bool PNGStream::IsStreaming() {
	return file && line < height && !error;
}
bool PNGStream::IsImageReady() {
	return ready && !error;
}

bool PNGStream::ReadLine() {
	if (!IsStreaming()) {
		return true;
	}

	u32* src;
	u32 s;

	if (setjmp(png_jmpbuf(png_ptr))) { //Exception handler, see libpng manual
		png_read_end(png_ptr, end_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
		png_ptr = NULL;

		fhClose(file); file = NULL; error = true;
		return false;
	}

	u32 stream_row[512];
	png_read_row(png_ptr, (png_byte*)stream_row, png_bytep_NULL);
	src = (u32*)stream_row;

	for (int x = 0; x < width; x++) {
		s = *src;
		*dst = BIT(15)|((s>>3)&0x001F)|((s>>6)&0x03E0)|((s>>9)&0x7C00);
		*dstA = (s>>24);
		src++;
		dst++;
		dstA++;
	}

	line++;
	if (line >= height) {
		ready = true;
		Stop();
	}
	return true;
}

bool PNGStream::GetBounds(Archive* archive, const char* name, u16* w, u16 *h) {
	if ((strcmp(filename, name) == 0 && !error) || Start(archive, name)) {
		*w = width;
		*h = height;
		return true;
	}
	return false;
}

bool PNGStream::Read(Archive* archive, const char* name, u16* out, u8* alphaOut) {
	//iprintf("%d %d %d %d\n", line, height, error?1:0, ready?1:0);
	if ((strcmp(filename, name) == 0 && !error) || Start(archive, name)) {
		while (IsStreaming()) {
			int s = MIN(PNG_STREAM_LINES, height - line);
			for (int n = 0; n < s; n++) {
				if (!ReadLine()) {
					return false;
				}
			}
			soundEngine->Update();
		}

		if (out) {
			memcpy(out, rgb, width*height*sizeof(u16));
		}
		if (alphaOut) {
			memcpy(alphaOut, alpha, width*height*sizeof(u8));
		}

		//Throw image away once it's been used
		ready = false;
		filename[0] = '\0';
		return true;
	}

	Stop();
	ready = false;
	filename[0] = '\0';
	return false;
}
