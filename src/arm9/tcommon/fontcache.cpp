#include "fontcache.h"

FontCache* defaultFontCache = NULL;

void createDefaultFontCache(const char* defaultFontPath) {
	destroyDefaultFontCache();
	defaultFontCache = new FontCache(defaultFontPath);
}
void destroyDefaultFontCache() {
	if (defaultFontCache) {
		destroyDefaultFontCache();
		defaultFontCache = NULL;
	}
}

//Custom face requester for freetype, loads the font from a file
static FT_Error simpleFaceRequester(FTC_FaceID face_id, FT_Library library,
    FT_Pointer request_data, FT_Face* aface)
{
    FT_Error error;

    FontIdentifier* face = (FontIdentifier*)face_id;
	error = FT_New_Face(library, face->filePath, face->faceIndex, aface);
	if (!error) {
		error = FT_Select_Charmap(*aface, FT_ENCODING_UNICODE);
		if (!error && *aface) {
			face->cmapIndex = ((*aface)->charmap ? FT_Get_Charmap_Index((*aface)->charmap) : 0);
		}
	}

	if (error) {
		iprintf("Font face req. error: %08x\n", error);
		waitForAnyKey();
	}
    return error;
}

//------------------------------------------------------------------------------

FontCache::FontCache(const char* defaultFontPath, u32 cacheSize) {
    fontsL = 0;
    font = NULL;
    fontsize = 0;

    FT_Error error;

    error = FT_Init_FreeType(&library);
    if (error) goto err;
    error = FTC_Manager_New(library, 0, 0, cacheSize, &simpleFaceRequester, NULL, &manager);
    if (error) goto err;
    error = FTC_SBitCache_New(manager, &sbitCache);
    if (error) goto err;
    error = FTC_CMapCache_New(manager, &cmapCache);
    if (error) goto err;

    err:
    if (error) {
    	scanKeys();
    	consoleDemoInit();
    	iprintf("Font init failed: %08x\n", error);
    	waitForAnyKey();
    }

    AddFont(defaultFontPath);
    SetFont(defaultFontPath);
    SetFontSize(1);
    SetFontSize(0);
}
FontCache::~FontCache() {
    scaler.face_id = NULL;

    FTC_Manager_Done(manager);
    FT_Done_FreeType(library);
}

//Functions
void FontCache::ClearCache() {
    FTC_Manager_Reset(manager);
}
void FontCache::AddFont(const char* filename) {
    strncpy(fonts[fontsL].filePath, filename, MAX_FONT_PATH_LEN-1);
    fonts[fontsL].filePath[MAX_FONT_PATH_LEN-1] = '\0';
    fonts[fontsL].faceIndex = 0;

    fontsL++;
}

//Getters
const char* FontCache::GetDefaultFont() {
	return (fontsL > 0 ? fonts[0].filePath : NULL);
}

FTC_SBit FontCache::GetGlyph(u32 c) {
    u32 flags = FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL;
    u32 index = FTC_CMapCache_Lookup(cmapCache, scaler.face_id, font->cmapIndex, c);

    FTC_SBitCache_LookupScaler(sbitCache, &scaler, flags, index, &sbit, NULL);
    return sbit;
}

s8 FontCache::GetAdvance(u32 c, FTC_SBit sbit) {
    if (c == '\t') return (GetAdvance(' ') << 2);
    if (!sbit) sbit = GetGlyph(c);
    return sbit->xadvance;
}

/* Returns the numbers of bytes in the codepoint that has been read. Returns 0 on an error. */
u8 FontCache::GetCodePoint(const char* str, u32* c) {
    char str0 = *str;

    u8 result = 0;

    if (str0 <= 0x7F) {
        *c = str0;
        result = 1;
    } else if (str0 >= 0xC2 && str0 <= 0xDF) {
        if (str0 == '\0' || str[1] == '\0') {
            return 0;
        }
        *c = ((str0-192)<<6) | (str[1]-128);
        result = 2;
    } else if (str0 >= 0xE0 && str0 <= 0xEF) {
        if (str0 == '\0' || str[1] == '\0' || str[2] == '\0') {
            return 0;
        }
        *c = ((str0-224)<<12) | ((str[1]-128)<<6) | (str[2]-128);
        result = 3;
    } else if (str0 >= 0xF0 && str0 <= 0xF4) {
        if (str0 == '\0' || str[1] == '\0' || str[2] == '\0' || str[3] == '\0') {
            return 0;
        }
        *c = ((str0-240)<<18) | ((str[1]-128)<<12) | ((str[2]-128)<<6) | (str[3]-128);
        result = 4;
    }

    return result;
}
u8 FontCache::GetLineHeight() {
    FT_Face face;
    FTC_Manager_LookupFace(manager, scaler.face_id, &face);
    return (face->size->metrics.height >> 6);
}

//Setters
FontIdentifier* FontCache::SetFont(const char* filename) {
	if (!filename) return NULL;

    for (u16 n = 0; n < fontsL; n++) {
        if (strcmp(fonts[n].filePath, filename) == 0) {
            //ClearCache();
            font = &fonts[n];
            scaler.face_id = (FTC_FaceID)font;
            GetGlyph(' '); //Update current font, or GetLineHeight() gives a wrong value
            return font;
        }
    }

    AddFont(filename);
    font = &fonts[fontsL-1];
    scaler.face_id = (FTC_FaceID)font;
    GetGlyph(' '); //Update current font, or GetLineHeight() gives a wrong value
    return NULL;
}
u8 FontCache::SetFontSize(u8 s) {
    if (s == 0) {
        s = DEFAULT_FONT_SIZE;
    }
    if (s != fontsize) {
    	//ClearCache();
        fontsize = s;
        scaler.width  = (FT_UInt)fontsize;
        scaler.height = (FT_UInt)fontsize;
        scaler.pixel  = 72;
        scaler.x_res  = 0;
        scaler.y_res  = 0;

        GetGlyph(' '); //Update current font, or GetLineHeight() gives a wrong value
    }
    return s;
}
