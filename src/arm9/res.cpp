#include "res.h"

#include "gba-jpeg-decode.h"
#include "tcommon/filehandle.h"
#include "tcommon/png.h"

PNGStream* pngStream = NULL;

void printTime(char* out) {
	time_t ti = time(NULL);
	tm* t = localtime(&ti);

	int year = t->tm_year + 1900;
	int month = t->tm_mon + 1;
	int day = t->tm_mday;
	int hour = t->tm_hour;
	int min = t->tm_min;

	sprintf(out, "%.4d-%.2d-%.2d %.2d:%.2d", year, month, day, hour, min);
}

bool loadImage(Archive* archive, const char* filename, u16* rgbOut, u8* alphaOut,
		u16* outW, u16* outH)
{
	return loadImage(archive, filename, &rgbOut, &alphaOut, outW, outH, false);
}

bool loadImage(Archive* archive, const char* filename, u16** rgbOut, u8** alphaOut,
		u16* outW, u16* outH, bool generate)
{

#ifdef IMG_LOAD_PROFILE
	TIMER3_DATA = -0x8000;
	TIMER3_CR = TIMER_ENABLE | TIMER_DIV_64;
	int start = TIMER3_DATA;
	bool png = false;
#endif

	bool success = false;
	char* periodChar = strrchr(filename, '.');
	if (periodChar) {
		if (periodChar[1]=='j' || periodChar[1]=='J') {
			FileHandle* fh = fhOpen(archive, filename);
			if (fh) {
				char* read = new char[fh->length];
				fh->ReadFully(read);

				if (outW) *outW = 256;
				if (outH) *outH = 192;

				u16* rgb = (rgbOut ? *rgbOut : NULL);
				if (generate && rgbOut && !rgb) {
					*rgbOut = rgb = new u16[256*192];
				}

				success = JPEG_DecompressImage((const unsigned char*)read, rgb, 256, 192);

				delete[] read;
				fhClose(fh);
			}
		} else {
			#ifdef IMG_LOAD_PROFILE
				png = true;
			#endif

			if (!pngStream) {
				vnLog(EL_error, COM_GRAPHICS, "pngStream is NULL");
			} else {
				u16 w, h;
				if (pngStream->GetBounds(archive, filename, &w, &h)) {
					if (outW) *outW = w;
					if (outH) *outH = h;

					u16* rgb = (rgbOut ? *rgbOut : NULL);
					u8* alpha = (alphaOut ? *alphaOut : NULL);
					if (generate) {
						if (rgbOut && !rgb) {
							*rgbOut = rgb = new u16[w*h];
						}
						if (alphaOut && !alpha) {
							*alphaOut = alpha = new u8[w*h];
						}
					}
					success = pngStream->Read(archive, filename, rgb, alpha);
				}
			}
		}
	} else {
		vnLog(EL_warning, COM_GRAPHICS, "Can't determine image type: no file-ext found.");
	}

#ifdef IMG_LOAD_PROFILE
	int end = TIMER3_DATA;

	start = (1<<16) - start;
	end = (1<<16) - end;
	if (start > end) {
		vnLog(EL_verbose, COM_GRAPHICS, "%s: %d", (png ? "png" : "jpg"), start-end);
	}
	TIMER3_CR = 0;
#endif

	return success;
}

