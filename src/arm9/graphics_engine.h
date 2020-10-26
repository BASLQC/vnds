#ifndef GRAPHICS_ENGINE_H
#define GRAPHICS_ENGINE_H

#include "common.h"
#include "tcommon/dsunzip.h"

#define GE_MAX_SPRITES 9
#define GE_MAX_CACHE_ENTRIES GE_MAX_SPRITES
#define GE_MAX_HEAP_SIZE ((3072<<10) - (512<<10)) //Stop adding images if the heap size gets larger
#define GE_FADE_FRAMES 16
#define GE_BG_FADE_FRAMES 16

class VNSprite {
    private:

    public:
    	char path[MAXPATHLEN];
        s16  x;
        s16  y;
        u16  w;
        u16  h;
        u16* rgb;
        u8*  alpha;

        VNSprite();
        virtual ~VNSprite();

};

class ImageCache {
    private:
        u16 cursor;
        bool locked[GE_MAX_CACHE_ENTRIES];

    public:
        char keys[GE_MAX_CACHE_ENTRIES][MAXPATHLEN];
        u16  w[GE_MAX_CACHE_ENTRIES];
        u16  h[GE_MAX_CACHE_ENTRIES];
        u16* rgb[GE_MAX_CACHE_ENTRIES];
        u8*  alpha[GE_MAX_CACHE_ENTRIES];

        ImageCache();
        virtual ~ImageCache();

        void Clear();
        void UnlockAll();
        void UnlockSlot(s8 index);
        bool GetImage(const char* filename, u16** outRGB, u8** outAlpha, u16* outW, u16* outH, bool lockOnSuccess);
        bool ContainsImage(const char* filename);
        s8 RequestSlot();
};

class GraphicsEngine {
    private:
    	VNDS* vnds;
        Archive* backgroundArc;
        Archive* foregroundArc;

        bool dirty;
        bool backgroundChanged;
        char backgroundPath[MAXPATHLEN];
        s16  backgroundFadeTime;
        u16  backgroundBuffer[256*192];

        ImageCache imageCache;
        VNSprite sprites[GE_MAX_SPRITES];
        u8 spritesL;
        u8 drawnSpritesL;

        void ClearForeground();

    public:
    	u16* screen;

    	GraphicsEngine(VNDS* vnds, u16* screen, Archive* backgroundArc, Archive* foregroundArc);
        virtual ~GraphicsEngine();

        bool IsImageCached(const char* path);
        const char* GetBackgroundPath();
        const VNSprite* GetSprites();
        u8 GetNumberOfSprites();

        void Reset();
        void Flush(bool quickread);
        void ClearCache();
        bool IsBackgroundChanged();
        void SetBackground(const char* filename, s16 fadeTime=-1);
        void SetForeground(const char* filename, s16 x, s16 y);
};

#endif
