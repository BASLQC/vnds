#ifndef T_UTIL_3D_H
#define T_UTIL_3D_H

#include <nds.h>

const int INV_VERTEX_SCALE = inttof32(64);
const int VERTEX_SCALE = divf32(inttof32(1), inttof32(64)); //= (1 / 64) in x.12 format

struct v2{
    int x;
    int y;
};

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
