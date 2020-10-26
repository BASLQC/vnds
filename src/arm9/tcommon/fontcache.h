#ifndef T_FONTCACHE_H
#define T_FONTCACHE_H

#include "common.h"

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_BITMAP_H
#include FT_GLYPH_H

#define GLYPH_CACHE_SIZE  	(32 * 1024)
#define DEFAULT_FONT_SIZE 	10
#define MAX_FONTS         	8
#define MAX_FONT_PATH_LEN	128

/* Define custom face identification structure */
struct FontIdentifier {
    char filePath[MAX_FONT_PATH_LEN];
    u32  faceIndex;
    u32  cmapIndex;
};

class FontCache {
	private:
        FT_Library library;
        FTC_Manager manager;
        FTC_ScalerRec scaler;
        FTC_CMapCache cmapCache;
        FTC_SBitCache sbitCache;
        FTC_SBit sbit;

        FontIdentifier fonts[MAX_FONTS];
        u8 fontsL;

        FontIdentifier* font;
        u8 fontsize;

	public:
		FontCache(const char* defaultFontPath, u32 cacheSize=GLYPH_CACHE_SIZE);
		~FontCache();

        void ClearCache();
        void AddFont(const char* filename);

        static u8 GetCodePoint(const char* str, u32* c);
        const char* GetDefaultFont();
        FTC_SBit GetGlyph(u32 c);
        s8   GetAdvance(u32 c, FTC_SBit sbit=NULL);
        u8   GetLineHeight();

        FontIdentifier* SetFont(const char* filename);
        u8 SetFontSize(u8 s);

};

extern FontCache* defaultFontCache;
void createDefaultFontCache(const char* defaultFontPath);
void destroyDefaultFontCache();

#endif
