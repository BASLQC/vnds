#include "graphics_engine.h"
#include <fat.h>

#include "sound_engine.h"
#include "res.h"
#include "vnds.h"

//------------------------------------------------------------------------------

VNSprite::VNSprite() {
	path[0] = '\0';
    x = y = 0;
    w = h = 0;
    rgb = NULL;
    alpha = NULL;
}
VNSprite::~VNSprite() {
}

//------------------------------------------------------------------------------

ImageCache::ImageCache() {
    cursor = 0;

    for (u16 n = 0; n < GE_MAX_CACHE_ENTRIES; n++) {
    	rgb[n] = NULL;
    	alpha[n] = NULL;
    }
    Clear();
}
ImageCache::~ImageCache() {
	Clear();
}

void ImageCache::Clear() {
    for (u16 n = 0; n < GE_MAX_CACHE_ENTRIES; n++) {
    	if (rgb[n]) delete[] rgb[n];
    	if (alpha[n]) delete[] alpha[n];

    	keys[n][0] = '\0';
        w[n] = h[n] = 0;
        rgb[n] = NULL;
        alpha[n] = NULL;
        locked[n] = false;
    }
}
void ImageCache::UnlockSlot(s8 index) {
	if (index >= 0 && index < GE_MAX_CACHE_ENTRIES) {
		locked[index] = false;
	}
}
void ImageCache::UnlockAll() {
    for (u16 n = 0; n < GE_MAX_CACHE_ENTRIES; n++) {
        locked[n] = false;
    }
}

bool ImageCache::GetImage(const char* filename, u16** outRGB, u8** outAlpha, u16* outW, u16* outH) {
    for (u16 n = 0; n < GE_MAX_CACHE_ENTRIES; n++) {
        if (strcmp(keys[n], filename) == 0) {
            *outRGB = rgb[n];
            *outAlpha = alpha[n];
            *outW = w[n];
            *outH = h[n];
            return true;
        }
    }

    *outRGB = NULL;
    *outAlpha = NULL;
    *outW = 0;
    *outH = 0;
    return false;
}
bool ImageCache::ContainsImage(const char* filename) {
    for (u16 n = 0; n < GE_MAX_CACHE_ENTRIES; n++) {
        if (strcmp(keys[n], filename) == 0) {
        	return true;
        }
    }
    return false;
}

s8 ImageCache::RequestSlot() {
	if (GE_MAX_CACHE_ENTRIES == 0) {
		return -1;
	}
	if (HEAP_SIZE > GE_MAX_HEAP_SIZE) {
		//Heap size getting dangerously large
		return -1;
	}

    int n = cursor;
    do {
        if (!locked[n]) {
            locked[n] = true;

            //Update cursor
            cursor = n+1;
            if (cursor >= GE_MAX_CACHE_ENTRIES) cursor -= GE_MAX_CACHE_ENTRIES;
            return n;
        }

        n++;
        if (n >= GE_MAX_CACHE_ENTRIES) n -= GE_MAX_CACHE_ENTRIES;
    } while (n != cursor);

	//vnLog(EL_warning, COM_GRAPHICS, "All image slots are locked");
    return -1;
}

//------------------------------------------------------------------------------

GraphicsEngine::GraphicsEngine(VNDS* vnds, u16* s, Archive* bga, Archive* fga) {
	this->vnds = vnds;
    screen = s;
    backgroundArc = bga;
    foregroundArc = fga;

    dirty = true;
    backgroundChanged = true;

    backgroundPath[0] = '\0';
    memset(buffer, 0, 256*192*sizeof(u16)); //Init bg to black
    spritesL = 0;
}
GraphicsEngine::~GraphicsEngine() {
}

const char* GraphicsEngine::GetBackgroundPath() {
	if (backgroundPath[0] == '\0') return NULL;
	return backgroundPath;
}
const VNSprite* GraphicsEngine::GetSprites() {
	return sprites;
}
u8 GraphicsEngine::GetNumberOfSprites() {
	return spritesL;
}
bool GraphicsEngine::IsImageCached(const char* path) {
	return imageCache.ContainsImage(path);
}


void GraphicsEngine::Reset() {
	dirty = true;
	backgroundChanged = true;
	backgroundPath[0] = '\0';

    memset(buffer, 0, 256*192*sizeof(u16)); //Init bg to black
	imageCache.Clear();
	spritesL = 0;

	Flush(true);
}
void GraphicsEngine::Flush(bool quickread) {
    if (!dirty) {
        backgroundChanged = false;
    	return;
    }
    dirty = false;

    if (!quickread) {
    	if (backgroundChanged) {
            REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BG3;
    	    for (u16 n = 0; n <= GE_BG_FADE_FRAMES; n++) {
    	        REG_BLDY_SUB = 0x10 * n / GE_BG_FADE_FRAMES;
    			vnds->soundEngine->Update();
    	        swiWaitForVBlank();
    	        //waitForAnyKey();
    	    }

            //Copy to VRAM
            DC_FlushRange(buffer, 256*192*sizeof(u16));
            dmaCopy(buffer, screen, 256*192*sizeof(u16));

    	    for (u16 n = 0; n <= GE_BG_FADE_FRAMES; n++) {
    	        REG_BLDY_SUB = 0x10 - 0x10 * n / GE_BG_FADE_FRAMES;
    			vnds->soundEngine->Update();
    	        swiWaitForVBlank();
    	        //waitForAnyKey();
    	    }
    	    REG_BLDCNT_SUB = BLEND_NONE;
    	}

    	//Copy background to SUB_SPRITES
		vramSetBankC(VRAM_C_LCD);
		vramSetBankD(VRAM_D_LCD);
		dmaCopy(VRAM_C+256*64, VRAM_D, 256*192*sizeof(u16));
		vramSetBankD(VRAM_D_SUB_SPRITE);
		vramSetBankC(VRAM_C_SUB_BG_0x06200000);
    }

    backgroundChanged = false;

    //Draw sprites
    u16* tempRGB   = NULL;
    u8*  tempAlpha = NULL;

    for (u16 n = 0; n < spritesL; n++) {
        VNSprite* s = &sprites[n];
        if (!s->rgb) {
        	//Load image on the fly if needed
        	if (!tempRGB)   tempRGB   = new u16[256*192];
        	if (!tempAlpha) tempAlpha = new u8[256*192];

        	if (loadImage(foregroundArc, s->path, &tempRGB, &tempAlpha, &s->w, &s->h, false)) {
    			blitAlpha(tempRGB, tempAlpha, s->w, s->h,
    					  buffer,             256,  192,
    					  0, 0, s->x, s->y, s->w, s->h);
        	}
        } else {
        	//Render image
			blitAlpha(s->rgb, s->alpha, s->w, s->h,
					  buffer,           256,  192,
					  0, 0, s->x, s->y, s->w, s->h);
        }
    }

    if (tempRGB)   delete[] tempRGB;
    if (tempAlpha) delete[] tempAlpha;

    SpriteEntry subSprites[12];
    u32 oldModeSub = (REG_DISPCNT_SUB);

    if (!quickread && spritesL > 0) {
    	//Setup sub-sprites
    	int i = 0;
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 4; x++) {
                subSprites[i].attribute[0] = ATTR0_BMP | ATTR0_SQUARE | (y<<6);
                subSprites[i].attribute[1] = ATTR1_SIZE_64 | (x<<6);
                subSprites[i].attribute[2] = ATTR2_ALPHA(15) | ATTR2_PRIORITY(0) | (y<<8) | (x<<3);
                i++;
            }
        }
        memcpy(OAM_SUB, subSprites, i*sizeof(SpriteEntry));

        //Display SUB_SPRITES on top of SUB_BG3
        videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_2D_BMP_256);
    }

	//Copy new image to VRAM
    DC_FlushRange(buffer, 256*192*sizeof(u16));
    dmaCopy(buffer, screen, 256*192*sizeof(u16));

    if (!quickread && spritesL > 0) {
    	//Blend
    	REG_BLDALPHA_SUB = 0x10 | (0 << 8);
		REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_SPRITE | BLEND_DST_BG3;
		for (int n = 0; n <= GE_FADE_FRAMES; n++) {
			int a = 0x10 - 0x10 * n / GE_FADE_FRAMES;
			int b = 0x10 - a;
			REG_BLDALPHA_SUB = a | (b << 8);
			vnds->soundEngine->Update();

			for (int n = 0; n < 12; n++) {
				subSprites[n].palette = MIN(15, a);
			}
			memcpy(OAM_SUB, subSprites, 12*sizeof(SpriteEntry));
			swiWaitForVBlank();
		}
		REG_BLDCNT_SUB = BLEND_NONE;

	    //Hide SUB_SPRITES again
	    videoSetModeSub(oldModeSub);
	    for (int n = 0; n < 12; n++) {
	    	subSprites[n].isHidden = true;
	    }
	    memcpy(OAM_SUB, subSprites, 12*sizeof(SpriteEntry));
    }
}
void GraphicsEngine::ClearCache() {
	imageCache.Clear();
}

void GraphicsEngine::SetBackground(const char* filename) {
    backgroundChanged = (strcmp(backgroundPath, filename) != 0);
    strcpy(backgroundPath, filename);

    if (loadImage(backgroundArc, backgroundPath, buffer, NULL)) {
        ClearForeground();
        dirty = true;
    } else {
    	vnLog(EL_missing, COM_GRAPHICS, "Can't open background image: %s", filename);
    }
}
void GraphicsEngine::ClearForeground() {
    spritesL = 0;
    imageCache.UnlockAll();
}
void GraphicsEngine::SetForeground(const char* filename, s16 x, s16 y) {
    if (spritesL >= GE_MAX_SPRITES) {
    	vnLog(EL_warning, COM_GRAPHICS, "Maximum number of sprites reached");
        return;
    }

    u16* rgb = NULL;
    u8* alpha = NULL;
    u16 w = 0;
    u16 h = 0;
    if (!imageCache.GetImage(filename, &rgb, &alpha, &w, &h)) {
    	bool success = false;

		s8 slot = imageCache.RequestSlot();
		if (slot >= 0) {
			if (loadImage(foregroundArc, filename, &rgb, &alpha, &w, &h, true)) {
				if (imageCache.rgb[slot]) delete[] imageCache.rgb[slot];
				if (imageCache.alpha[slot]) delete[] imageCache.alpha[slot];

				strcpy(imageCache.keys[slot], filename);
				imageCache.w[slot] = w;
				imageCache.h[slot] = h;
				imageCache.rgb[slot] = rgb;
				imageCache.alpha[slot] = alpha;

				success = true;
    		} else {
    			imageCache.UnlockSlot(slot);
    			if (rgb)  delete[] rgb;
    			if (alpha)delete[] alpha;
    			rgb = NULL;
    			alpha = NULL;

    	    	vnLog(EL_missing, COM_GRAPHICS, "Can't open foreground image: %s", filename);
    			return;
    		}
    	}
    }

    //Put information in VNSprite
    VNSprite* s = &sprites[spritesL];
    strcpy(s->path, filename);
    s->x = x;
    s->y = y;
    s->w = w;
    s->h = h;
    s->rgb = rgb;
    s->alpha = alpha;

    //Update
    spritesL++;
    dirty = true;
}

//------------------------------------------------------------------------------
