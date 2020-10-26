#ifndef T_COMMON_H
#define T_COMMON_H

/*
 * A collection of random utility functions and unspecific macro's
 */

#include <nds.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#define HEAP_SIZE (mallinfo().uordblks)

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

class Rect {
    public:
        int x, y, w, h;

        Rect()
			: x(0), y(0), w(0), h(0)
        {
        }
        Rect(int rx, int ry, int rw, int rh)
			: x(rx), y(ry), w(rw), h(rh)
        {

        }
};

extern u8 backlight;

void tcommonFIFOCallback(u32 value, void* userdata);
void toggleBacklight();

u8 chartohex(char c);

//Copy a block of pixels, ignore transparency
void blit(const u16* src, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

//Copy a block of pixels, ignore transparency
void blit(const u8* src, u16 sw, u16 sh,
          u8* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

//Copy a block of pixels, use bit 15 for binary transparency
void blit2(const u16* src, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

//Copy a block of pixels, use the alpha channel for binary transparency
void blit2(const u16* src, const u8* alpha, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

//Copy a block of pixels, use the alpha channel for 8-bit transparency
void blitAlpha(const u16* src, const u8* srcA, u16 sw, u16 sh,
               u16* dst,           u16 dw, u16 dh,
               s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

void fadeBlack(u16 frames);
void unfadeBlack(u16 frames);
void unfadeBlack2(u16 frames);
bool loadImage(const char* file, u16* out, u8* outA, u16 w, u16 h, int format=0);
void darken(u16* screen, u8 factor, s16 x, s16 y, s16 w, s16 h);
void resetVideo();

void waitForAnyKey();
void trimString(char* string);
void unescapeString(char* string);
u32 versionStringToInt(char* string);
void versionIntToString(char* out, u32 version);
bool mkdirs(const char* filename);
bool fexists(const char* filename);
void setupCapture(int bank);
void waitForCapture();

#include "gui/gui_types.h"
#include "util3d.h"
#include "textures.h"

//Forward decls
class Archive;
class FileHandle;
class FontCache;
class DefaultFileHandle;
class ArchiveFileHandle;
class Text;

#endif
