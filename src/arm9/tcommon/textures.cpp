#include "common.h"
#include "filehandle.h"
#include "png.h"

using namespace std;

void drawQuad(const Texture* texture, s32 x, s32 y, v16 vw, v16 vh, Rect uv) {
	glPushMatrix();

	glTranslate3f32(x, y+2048, 0);

	MATRIX_SCALE = INV_VERTEX_SCALE_FACTOR;
	MATRIX_SCALE = INV_VERTEX_SCALE_FACTOR;
	MATRIX_SCALE = inttof32(1);

	//glRotateZi(drawAngle);
	setActiveTexture(texture);

    int u0 = (uv.x) << 4;
    int u1 = (uv.x+uv.w) << 4;
    int v0 = (uv.y << 20);
    int v1 = (uv.y+uv.h) << 20;

    int a = 0 & 0xFFFF;
    int c = vw & 0xFFFF;
    int b = 0 << 16;
    int d = vh << 16;

	glBegin(GL_QUAD);
	glColor(RGB15(31, 31, 31)); //glNormal(NORMAL_PACK(0, 0, inttov10(1)));
	GFX_TEX_COORD = u0 | v1;
	GFX_VERTEX_XY = a  | d;
	GFX_TEX_COORD = u1 | v1;
	GFX_VERTEX_XY = c  | d;
	GFX_TEX_COORD = u1 | v0;
	GFX_VERTEX_XY = c  | b;
	GFX_TEX_COORD = u0 | v0;
	GFX_VERTEX_XY = a  | b;

	glPopMatrix(1);
}

void setTextureParam(Texture* tex, u32 param) {
	tex->format &= ~0xE00F0000;
	tex->format |= param;
}

MemoryChunk allocMemoryChunk(list<MemoryChunk>& freelist, u32 size) {
	list<MemoryChunk>::iterator itr = freelist.begin();
	while (itr != freelist.end()) {
		MemoryChunk chunk = *itr;

		if (chunk.length >= size) {
			freelist.erase(itr);

			if (chunk.length > size) {
				MemoryChunk newChunk = {chunk.offset + size, chunk.length - size};
				freelist.push_front(newChunk);
			}

			chunk.length = size;
			return chunk;
		}
		++itr;
	}

	MemoryChunk nullChunk = {0, 0};
	return nullChunk;
}

void freeMemoryChunk(list<MemoryChunk>& freelist, MemoryChunk m) {
	if (m.length == 0) return;

	list<MemoryChunk>::iterator itr = freelist.begin();
	while (itr != freelist.end()) {
		MemoryChunk chunk = *itr;
		if (m.offset+m.length == chunk.offset) {
			chunk.offset -= m.length; //Free chunk to the left
			return;
		} else if (m.offset == chunk.offset+chunk.length) {
			chunk.length += m.length; //Free chunk to the right
			return;
		}
		++itr;
	}

	freelist.push_back(m);
}

inline u32 alignVal(u32 val, u32 to) {
	return (val & (to-1))? (val & ~(to-1)) + to : val;
}

Texture* loadTexture(TextureManager* texmgr, const char* format,
		const char* dtaFile, const char* palFile, GL_TEXTURE_PARAM_ENUM param)
{
    GL_TEXTURE_TYPE_ENUM f = GL_RGB32_A3;
    if (strcmp(format, "A3I5") == 0) {
        f = GL_RGB32_A3;
    } else if (strcmp(format, "A5I3") == 0) {
        f = GL_RGB8_A5;
    } else if (strcmp(format, "RGB256") == 0) {
        f = GL_RGB256;
    } else if (strcmp(format, "RGB") == 0) {
        f = GL_RGB;
    } else if (strcmp(format, "RGBA") == 0) {
        f = GL_RGBA;
    }

    return loadTexture(texmgr, f, dtaFile, palFile, param);
}

Texture* loadTexture(TextureManager* texmgr, GL_TEXTURE_TYPE_ENUM format,
		const char* dtaFile, const char* palFile, GL_TEXTURE_PARAM_ENUM param)
{
    u16* dta = NULL;
    u16* pal = NULL;
    int dtaSize = 0;
    int palSize = 0;

    if (dtaFile) {
        FILE* f = fopen(dtaFile, "rb");
        if (f) {
            setvbuf(f, NULL, _IONBF, 0);
            fseek(f, 0, SEEK_END);
            dtaSize = ftell(f);
            fseek(f, 0, SEEK_SET);
            dta = new u16[dtaSize>>1];
            fread(dta, 1, dtaSize, f);
            fclose(f);
        }
    }
    if (palFile) {
        FILE* f = fopen(palFile, "rb");
        if (f) {
            setvbuf(f, NULL, _IONBF, 0);
            fseek(f, 0, SEEK_END);
            palSize = ftell(f);
            fseek(f, 0, SEEK_SET);
            pal = new u16[palSize>>1];
            fread(pal, 1, palSize, f);
            fclose(f);
        }
    }

    int bpp = 1;
    if (format == GL_RGB || format == GL_RGBA) {
        bpp = 2;
    }

    int pixels = dtaSize / bpp;
    GL_TEXTURE_SIZE_ENUM size = TEXTURE_SIZE_64;
    if (pixels >= 1024*1024) size = TEXTURE_SIZE_1024;
    else if (pixels >= 512*512) size = TEXTURE_SIZE_512;
    else if (pixels >= 256*256) size = TEXTURE_SIZE_256;
    else if (pixels >= 128*128) size = TEXTURE_SIZE_128;
    else if (pixels >= 64*64) size = TEXTURE_SIZE_64;
    else if (pixels >= 32*32) size = TEXTURE_SIZE_32;
    else if (pixels >= 16*16) size = TEXTURE_SIZE_16;
    else size = TEXTURE_SIZE_8;

    Texture* tex = texmgr->AddTexture(size, size, format, param, dta, dtaSize, pal, palSize);
	if (dta) delete[] dta;
	if (pal) delete[] pal;
	return tex;
}

bool loadImage(const char* file, u16* out, u8* outA, u16 w, u16 h, int format) {
    if (format == 0) {
        char path[MAXPATHLEN];
    	sprintf(path, "%s.png", file);

        FILE* file = fopen(path, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            int dataL = ftell(file);
            fseek(file, 0, SEEK_SET);
			char* data = new char[dataL];

			fread(data, sizeof(char), dataL, file);
			bool result = pngLoadImage(data, dataL, out, outA, w, h);

			delete[] data;
			fclose(file);
			return result;
        }
        return false;
    }

    char dtaPath[MAXPATHLEN];
    sprintf(dtaPath, "%s.dta", file);
    char palPath[MAXPATHLEN];
    sprintf(palPath, "%s.pal", file);

    FileHandle* fhDTA = fhOpen(dtaPath);
    if (!fhDTA) return false;

    FileHandle* fhPAL = NULL;
    u16* pal = NULL;
    if (format != GL_RGBA) {
        fhPAL = fhOpen(palPath);
        if (!fhPAL) {
            fhClose(fhDTA);
            return false;
        }
        pal = new u16[fhPAL->length/sizeof(u16)];
        fhReadFully(pal, fhPAL);
    }

    u8* dta = new u8[fhDTA->length];
    fhReadFully(dta, fhDTA);

    if (format == GL_RGB32_A3) {
        for (u32 n = 0; n < fhDTA->length; n++) {
            out[n] = pal[dta[n] & 31];
            if (outA) {
                outA[n] = (dta[n]&0xE0) + 16;
                if (outA[n] >= 255-16) outA[n] = 255;
                if (outA[n] <= 16) outA[n] = 0;
            }
        }
    } else if (format == GL_RGB8_A5) {
        for (u32 n = 0; n < fhDTA->length; n++) {
            out[n] = pal[dta[n] & 7];
            if (outA) {
                outA[n] = (dta[n]&0xF8) + 4;
                if (outA[n] >= 255-4) outA[n] = 255;
                if (outA[n] <= 4) outA[n] = 0;
            }
        }
    } else if (format == GL_RGB256) {
        for (u32 n = 0; n < fhDTA->length; n++) {
            out[n] = pal[dta[n]]|BIT(15);
            if (outA) outA[n] = (out[n]|BIT(15) ? 255 : 0);
        }
    } else if (format == GL_RGBA) {
        for (u32 n = 0; n < fhDTA->length; n++) {
            out[n>>1] = (dta[n+1]<<8)|(dta[n]);
            n++;
            if (outA) outA[n] = (out[n]|BIT(15) ? 255 : 0);
        }
    }

    if (fhDTA) fhClose(fhDTA);
    if (dta) delete[] dta;
    if (fhPAL) fhClose(fhPAL);
    if (pal) delete[] pal;

    return true;
}

//-----------------------------------------------------------------------------

TextureManager::TextureManager() {
	Reset();
}
TextureManager::~TextureManager() {

}

void TextureManager::Reset() {
	dataFreeList.clear();
	palFreeList.clear();
	memset(textures, 0, sizeof(Texture) * texturesL);

	MemoryChunk data1 = {(u32)VRAM_A, 128<<10};
	MemoryChunk data2 = {(u32)(VRAM_B+256*32), 96<<10};
	dataFreeList.push_back(data1);
	dataFreeList.push_back(data2);

	MemoryChunk pal1 = {(u32)VRAM_E, 64<<10};
	palFreeList.push_back(pal1);
}

Texture* TextureManager::AddTexture(u8 sizeX, u8 sizeY,
		GL_TEXTURE_TYPE_ENUM mode, GL_TEXTURE_PARAM_ENUM param,
		const u16* data, u32 dataL, const u16* pal, u32 palL)
{
	if (dataL == 0) {
		dataL = 1 << (sizeX + sizeY + 6);
		switch (mode) {
			case GL_RGB:
			case GL_RGBA:
				dataL = dataL << 1;
				break;
			case GL_RGB4:
				dataL = dataL >> 2;
				break;
			case GL_RGB16:
				dataL = dataL >> 1;
				break;
			default:
				break;
		}
	}

	MemoryChunk dataChunk = allocMemoryChunk(dataFreeList, dataL);
	if (dataChunk.length == 0) return NULL;

	MemoryChunk palChunk = {0, 0};
	if (palL > 0) {
		palChunk = allocMemoryChunk(palFreeList, alignVal(palL, 16));
		if (palChunk.length == 0) {
			freeMemoryChunk(dataFreeList, dataChunk);
			return NULL;
		}
	}

	//iprintf("%x %d %x %d\n", dataChunk.offset, dataChunk.length, palChunk.offset, palChunk.length);

	int slot = GetFreeTextureSlot();
	if (slot < 0) {
		freeMemoryChunk(dataFreeList, dataChunk);
		freeMemoryChunk(palFreeList, palChunk);
		return NULL;
	}

	Texture* tex = &textures[slot];
	tex->dataChunk = dataChunk;
	tex->palChunk = palChunk;
	tex->sizeX = sizeX;
	tex->sizeY = sizeY;
	tex->addr = (dataChunk.offset >> 3) & 0xFFFF;
	tex->mode = mode;
	setTextureParam(tex, param);

	swiWaitForVBlank();
	while(dmaBusy(1) || dmaBusy(2)); //Wait for DMA to finish

	if (data && dataL > 0) {
		TogglePixelVRAM(false);
		DC_FlushRange(data, dataL);
		dmaCopyHalfWordsAsynch(1, data, (u16*)dataChunk.offset, dataL);
	}
	if (pal && palL > 0) {
		TogglePaletteVRAM(false);
		DC_FlushRange(pal, palL);
		dmaCopyHalfWordsAsynch(2, pal, (u16*)palChunk.offset, palL);
		//swiCopy(pal, (u16*)palChunk.offset, (palL>>2) | COPY_MODE_WORD);
	}

	while(dmaBusy(1) || dmaBusy(2)); //Wait for DMA to finish

	if (data && dataL > 0) TogglePixelVRAM(true);
	if (pal && palL > 0) TogglePaletteVRAM(true);

	return tex;
}

void TextureManager::TogglePixelVRAM(bool on) {
	static u32 oldMode;
	if (!on) {
		oldMode = vramSetMainBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
	} else {
		vramRestoreMainBanks(oldMode);
	}
}
void TextureManager::TogglePaletteVRAM(bool on) {
	static VRAM_E_TYPE oldMode;
	if (!on) {
		oldMode = (VRAM_E_TYPE)VRAM_E_CR;
		vramSetBankE(VRAM_E_LCD);
	} else {
		vramSetBankE(oldMode);
	}
}

void TextureManager::RemoveTexture(Texture* tex) {
	freeMemoryChunk(dataFreeList, tex->dataChunk);
	freeMemoryChunk(palFreeList, tex->palChunk);
	memset(tex, 0, sizeof(Texture));
}

int TextureManager::GetFreeTextureSlot() {
	for (int n = 0; n < texturesL; n++) {
		if (textures[n].format == 0) return n;
	}
	return -1;
}
