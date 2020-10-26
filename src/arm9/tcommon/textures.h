#ifndef T_TEXTURES_H
#define T_TEXTURES_H

#include <nds.h>
#include <list>
#include "common.h"

struct MemoryChunk {
	u32 offset;
	u32 length;
};

struct Texture {
	MemoryChunk dataChunk;
	MemoryChunk palChunk;

	union {
		struct {
			u16 addr;
			u16 : 4;
			u16 sizeX : 3;
			u16 sizeY : 3;
			u16 mode  : 3;
			u16 : 3;
		};
		struct {
			u32 format;
		};
	};
};

class TextureManager {
	protected:
		static const int texturesL = 32;

		std::list<MemoryChunk> dataFreeList;
		std::list<MemoryChunk> palFreeList;

		Texture textures[texturesL];

		int GetFreeTextureSlot();

	public:
		TextureManager();
		virtual ~TextureManager();

		virtual void Reset();
		virtual Texture* AddTexture(u8 sizeX, u8 sizeY,
				GL_TEXTURE_TYPE_ENUM mode, GL_TEXTURE_PARAM_ENUM param,
				const u16* data=NULL, u32 dataL=0, const u16* pal=NULL, u32 palL=0);
		virtual void RemoveTexture(Texture* tex);

		virtual void TogglePixelVRAM(bool on);
		virtual void TogglePaletteVRAM(bool on);
};

bool loadImage(const char* file, u16* out, u8* outA, u16 w, u16 h, int format);
Texture* loadTexture(TextureManager* texmgr, const char* format,
		const char* dtaFile, const char* palFile, GL_TEXTURE_PARAM_ENUM param);
Texture* loadTexture(TextureManager* texmgr, GL_TEXTURE_TYPE_ENUM format,
		const char* dtaFile, const char* palFile, GL_TEXTURE_PARAM_ENUM param);
void setTextureParam(Texture* tex, u32 param);
void drawQuad(const Texture* texture, s32 x, s32 y, v16 vw, v16 vh, Rect uv);

inline
void setActiveTexture(const Texture* tex) {
	if (!tex) {
		GFX_TEX_FORMAT = 0;
	} else {
		GFX_TEX_FORMAT = tex->format;
		if (tex->palChunk.length) {
			GFX_PAL_FORMAT = tex->palChunk.offset >> (4-(tex->format==GL_RGB4));
		} else {
			GFX_PAL_FORMAT = 0;
		}
	}
}

#endif
