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

bool ImageCache::GetImage(const char* filename, u16** outRGB, u8** outAlpha,
		u16* outW, u16* outH, bool lockOnSuccess)
{
    for (u16 n = 0; n < GE_MAX_CACHE_ENTRIES; n++) {
        if (strcmp(keys[n], filename) == 0) {
        	if (lockOnSuccess) locked[n] = true;
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
    backgroundFadeTime = 0;
    memset(backgroundBuffer, 0, 256*192*sizeof(u16)); //Init bg to black
    spritesL = 0;
    drawnSpritesL = 0;
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
    backgroundFadeTime = 0;

    memset(backgroundBuffer, 0, 256*192*sizeof(u16)); //Init bg to black
	imageCache.Clear();
	spritesL = 0;
	drawnSpritesL = 0;

	Flush(true);
}

static void blend(VNDS* vnds, SpriteEntry* subSprites, int from, int to,
		int frames=GE_FADE_FRAMES)
{
	REG_BLDALPHA_SUB = (from) | ((0x10 - from) << 8);
	REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_SPRITE | BLEND_DST_BG3;
	for (int n = 0; n <= frames; n++) {
		int a = from + (to - from) * n / frames;
		int b = 0x10 - a;

		REG_BLDALPHA_SUB = a | (b << 8);
		for (int i = 0; i < 12; i++) {
			subSprites[i].palette = MIN(15, a);
		}
		memcpy(OAM_SUB, subSprites, 12*sizeof(SpriteEntry));

		if ((n & 15) == 0) vnds->soundEngine->Update();
		swiWaitForVBlank();
	}
	REG_BLDCNT_SUB = BLEND_NONE;

	for (int i = 0; i < 12; i++) {
		subSprites[i].palette = MIN(15, to);
	}
	memcpy(OAM_SUB, subSprites, 12*sizeof(SpriteEntry));
}
static void showSubSprites(SpriteEntry* subSprites) {
    for (int n = 0; n < 12; n++) {
    	subSprites[n].isHidden = false;
    }
    memcpy(OAM_SUB, subSprites, 12*sizeof(SpriteEntry));
}
static void hideSubSprites(SpriteEntry* subSprites) {
    for (int n = 0; n < 12; n++) {
    	subSprites[n].isHidden = true;
    }
    memcpy(OAM_SUB, subSprites, 12*sizeof(SpriteEntry));
}

void GraphicsEngine::Flush(bool quickread) {
    if (!dirty) {
        backgroundChanged = false;
    	return;
    }
    dirty = false;

    //Setup second screen layer
    SpriteEntry subSprites[12];
    u32 oldModeSub = (REG_DISPCNT_SUB);
    if (!quickread) {
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
    	hideSubSprites(subSprites);
        videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_2D_BMP_256);
    }

    //Draw Background
    u16* buffer = new u16[256*192];
	if (!backgroundChanged) {
		//Copy BG into buffer
	    memcpy(buffer, backgroundBuffer, 256*192*sizeof(u16));
	} else {
		if (!quickread) {
			//Copy old BG to VRAM_D
			vramSetBankD(VRAM_D_LCD);
			DC_FlushRange(backgroundBuffer, 256*192*sizeof(u16));
			dmaCopy(backgroundBuffer, VRAM_D, 256*192*sizeof(u16));
			vramSetBankD(VRAM_D_SUB_SPRITE);
		}

		//Load new BG
		if (backgroundPath[0] != '\0') {
			if (!loadImage(backgroundArc, backgroundPath, backgroundBuffer, NULL)) {
				vnLog(EL_missing, COM_GRAPHICS, "Can't open background image: %s", backgroundPath);
			}
		}
	    memcpy(buffer, backgroundBuffer, 256*192*sizeof(u16));

		if (!quickread) {
			int bgFadeTime = backgroundFadeTime;
			if (bgFadeTime < 0) {
				bgFadeTime = GE_BG_FADE_FRAMES;
			}

			//Fade out sprites
			swiWaitForVBlank();
			showSubSprites(subSprites);
			if (drawnSpritesL > 0) {
				blend(vnds, subSprites, 0, 0x10, bgFadeTime);
			}

	    	//Cross-fade to new BG
	    	DC_FlushRange(backgroundBuffer, 256*192*sizeof(u16));
			dmaCopy(backgroundBuffer, screen, 256*192*sizeof(u16));
	    	blend(vnds, subSprites, 0x10, 0, bgFadeTime);
	    	hideSubSprites(subSprites);
		}
	}

	if (!quickread && (drawnSpritesL > 0 || spritesL > 0)) {
    	//Copy current image to SUB_SPRITES
		swiWaitForVBlank();
		vramSetBankD(VRAM_D_LCD);
		vramSetBankC(VRAM_C_LCD);
		dmaCopy(VRAM_C+64*256, VRAM_D, 256*192*sizeof(u16));
		vramSetBankC(VRAM_C_SUB_BG_0x06200000);
		vramSetBankD(VRAM_D_SUB_SPRITE);

    	blend(vnds, subSprites, 0, 0x10, 0);
		showSubSprites(subSprites);
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

        	if (!tempRGB || !tempAlpha) {
        		//Oh no!
        	} else if (loadImage(foregroundArc, s->path, &tempRGB, &tempAlpha, &s->w, &s->h, false)) {
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

	//Copy new image to VRAM
    DC_FlushRange(buffer, 256*192*sizeof(u16));
    dmaCopy(buffer, screen, 256*192*sizeof(u16));

    if (!quickread && (drawnSpritesL != 0 || spritesL > 0)) {
		//Blend in new image
    	blend(vnds, subSprites, 0x10, 0);
    	hideSubSprites(subSprites);
    }
    videoSetModeSub(oldModeSub);

    delete[] buffer;
    drawnSpritesL = spritesL;
}
void GraphicsEngine::ClearCache() {
	imageCache.Clear();
}

bool GraphicsEngine::IsBackgroundChanged() {
	return backgroundChanged;
}
void GraphicsEngine::SetBackground(const char* filename, s16 fadeTime) {
    backgroundChanged |= (strcmp(backgroundPath, filename) != 0);
    strcpy(backgroundPath, filename);
    backgroundFadeTime = fadeTime;

    ClearForeground();
    dirty = true;
}
void GraphicsEngine::ClearForeground() {
    spritesL = 0;
    imageCache.UnlockAll();
    //imageCache.Clear();
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
    if (!imageCache.GetImage(filename, &rgb, &alpha, &w, &h, true)) {
    	bool success = false;

		s8 slot = imageCache.RequestSlot();
		if (slot >= 0) {
			if (loadImage(foregroundArc, filename, &rgb, &alpha, &w, &h, true)) {
				for (int n = 0; n < spritesL; n++) {
					if (sprites[n].rgb && sprites[n].rgb == imageCache.rgb[slot]) {
						vnLog(EL_error, COM_GRAPHICS, "It's the apocalypse! %s %s",
								filename, imageCache.keys[slot]);
					}
				}

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
