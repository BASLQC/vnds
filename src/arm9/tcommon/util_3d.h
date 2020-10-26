#ifndef T_UTIL_3D_H
#define T_UTIL_3D_H

#include <nds.h>

#define VERTEX_SCALE_FACTOR 	0x40		//divf32(inttof32(1), inttof32(64))
#define INV_VERTEX_SCALE_FACTOR 0x40000		//inttof32(64)
#define VERTEX_SCALE(x)			((x) << 6)
#define INV_VERTEX_SCALE(x)		((x) << 18)

struct TextureData {
    int id;
    int format;
    int pal_addr;
    u32 size;
};

class Rect;

void init3D();
void glReset();
void drawQuad(TextureData* texture, s32 x, s32 y, v16 vw, v16 vh, Rect uv);

void loadTexture(TextureData* target, GL_TEXTURE_TYPE_ENUM format, const char* dtaFile,
    const char* palFile, GL_TEXTURE_PARAM_ENUM param=TEXGEN_TEXCOORD);

void loadTexture(TextureData* target, const char* format, const char* dtaFile,
    const char* palFile, GL_TEXTURE_PARAM_ENUM param=TEXGEN_TEXCOORD);

#endif
