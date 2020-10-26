#include "common.h"

#include <fat.h>
#include <ctype.h>
#include <sys/dir.h>
#include <errno.h>
#include "filehandle.h"
#include "png.h"
#include "text.h"
#include "../../common/fifo.h"

#define BLT_ALPHA_BITS  8
#define BLT_MAX_ALPHA   ((1 << BLT_ALPHA_BITS) - 1)
#define BLT_ROUND       (1 << (BLT_ALPHA_BITS - 1))
#define BLT_THRESHOLD   8
#define BLT_THRESHOLD2  128

#define LOWC(x)         ((x)&31)

u16 fifoCommandMode = 0;
u8  backlight = 1;

void tcommonFIFOCallback(u32 value, void* userdata) {
	switch (fifoCommandMode) {
	case MSG_TOGGLE_BACKLIGHT:
		backlight = value;
		fifoCommandMode = 0;
		break;
	default:

		switch (value) {
		case MSG_TOGGLE_BACKLIGHT:
			fifoCommandMode = value;
			break;
		default:
			consoleDemoInit();
			iprintf("Unknown FIFO Command: 0x%x (mode=%d)\n", value, fifoCommandMode);
			waitForAnyKey();
			fifoCommandMode = 0;
		}
	}

}

void toggleBacklight() {
	//fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM7, MSG_TOGGLE_BACKLIGHT); //--in libnds 1.3.1 you HAVE to check the return value or the command won't be sent (GCC is probably to blame)
	if (!fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM7, MSG_TOGGLE_BACKLIGHT)) {
		iprintf("Error sending backlight message to ARM7\n");
	}
}

u8 chartohex(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + c - 'a';
	if (c >= 'A' && c <= 'F') return 10 + c - 'A';
	return 0;
}

inline s16 blitMinX(s16 sx, s16 dx) {
    return sx + MAX(0, -dx);
}
inline s16 blitMinY(s16 sy, s16 dy) {
    return sy + MAX(0, -dy);
}
inline s16 blitMaxX(s16 minX, u16 sw, u16 dw, s16 sx, s16 dx, u16 cw) {
    return sx + MIN(MIN(dw-dx, sw-sx), cw);
}
inline s16 blitMaxY(s16 minY, u16 sh, u16 dh, s16 sy, s16 dy, u16 ch) {
    return sy + MIN(MIN(dh-dy, sh-sy), ch);
}

void blit2(const u16* src, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const int sInc = sw - maxX + minX;
    const int dInc = dw - maxX + minX;
    const int srcOffset = (minY * sw) + minX;

    const u16* sc = src + srcOffset;
    u16* dc = dst + (MAX(0, dy) * dw) + MAX(0, dx);

    for (int y = minY; y < maxY; y++) {
    	for (int x = minX; x < maxX; x++) {
    		if (*sc & BIT(15)) {
    			*dc = *sc;
    		}

    		sc++;
    		dc++;
    	}
        sc += sInc;
        dc += dInc;
    }
}

void blit2(const u16* src, const u8* alpha, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const int sInc = sw - maxX + minX;
    const int dInc = dw - maxX + minX;
    const int srcOffset = (minY * sw) + minX;

    const u8*  sa = alpha + srcOffset;
    const u16* sc = src   + srcOffset;
    u16* dc = dst   + (MAX(0, dy) * dw) + MAX(0, dx);

    for (int y = minY; y < maxY; y++) {
    	for (int x = minX; x < maxX; x++) {
    		*dc = ((*sa & 0x80) ? *sc : 0);

    		sa++;
    		sc++;
    		dc++;
    	}
    	alpha += sInc;
        sc += sInc;
        dc += dInc;
    }
}

void blit(const u16* src, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const u16* sc = src + (minY * sw) + minX;
    u16* dc = dst + (MAX(0, dy) * dw) + MAX(0, dx);

    for (int y = minY; y < maxY; y++) {
        memcpy(dc, sc, (maxX - minX) << 1);
        sc += sw;
        dc += dw;
    }
}

void blit(const u8* src, u16 sw, u16 sh,
          u8* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const u8* sc = src + (minY * sw) + minX;
    u8* dc = dst + (MAX(0, dy) * dw) + MAX(0, dx);

    for (int y = minY; y < maxY; y++) {
        memcpy(dc, sc, maxX - minX);
        sc += sw;
        dc += dw;
    }
}

void blitAlpha(const u16* src, const u8* srcA, u16 sw, u16 sh,
               u16* dst,           u16 dw, u16 dh,
               s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    //optimized asm for blending 2 16-bit palettes: http://forum.gbadev.org/viewtopic.php?p=53322

    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const int sInc = sw - maxX + minX;
    const int dInc = dw - maxX + minX;
    const int srcOffset = (minY * sw) + minX;

    const u8*  sa = srcA + srcOffset;
    const u16* sc = src  + srcOffset;
    u16* dc = dst  + (MAX(0, dy) * dw) + MAX(0, dx);

    int s;
    int a;
    int d;
    int sbr;
    int dbr;
    int sg;
    int dg;

    for (int y = minY; y < maxY; y++) {
        for (int x = minX; x < maxX; x++) {
            a = *sa;
			if (a >= BLT_THRESHOLD) {
	            s = *sc;
	            d = *dc;
				if (d & BIT(15) && a <= BLT_MAX_ALPHA - BLT_THRESHOLD) {
					sbr = (s & 0x1F) | (s<<6 & 0x1F0000);
					dbr = (d & 0x1F) | (d<<6 & 0x1F0000);
					sg = s & 0x3E0;
					dg = d & 0x3E0;

					dbr = (dbr + (((sbr-dbr)*a + BLT_ROUND) >> BLT_ALPHA_BITS));
					dg  = (dg  + (((sg -dg )*a + BLT_ROUND) >> BLT_ALPHA_BITS));
					*dc = (dbr&0x1F) | (dg&0x3E0) | (dbr>>6 & 0x7C00) | BIT(15);
				} else {
					if (a >= BLT_THRESHOLD2) {
						*dc = s | BIT(15);
					}
				}
			}
            sa++;
            sc++;
            dc++;
        }
        sc += sInc;
        sa += sInc;
        dc += dInc;
    }
}

u32 oldMode;
u32 oldModeSub;

void fadeBlack(u16 frames) {
    oldMode = (REG_DISPCNT);
    oldModeSub = (REG_DISPCNT_SUB);
    videoSetMode(oldMode & (~DISPLAY_SPR_ACTIVE));
    videoSetModeSub(oldModeSub & (~DISPLAY_SPR_ACTIVE));

    REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
    REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BG2 | BLEND_SRC_BG3;

    for (u16 n = 0; n <= frames; n++) {
    	REG_BLDY = 0x1F * n / frames;
    	REG_BLDY_SUB = 0x1F * n / frames;
        swiWaitForVBlank();
    }
}

void unfadeBlack2(u16 frames) {
    for (u16 n = 0; n <= frames; n++) {
    	REG_BLDY = 0x1F - 0x1F * n / frames;
        REG_BLDY_SUB = 0x1F - 0x1F * n / frames;
        swiWaitForVBlank();
    }

    REG_BLDCNT = BLEND_NONE;
    REG_BLDCNT_SUB = BLEND_NONE;
}
void unfadeBlack(u16 frames) {
	unfadeBlack2(frames);

    videoSetMode(oldMode);
    videoSetModeSub(oldModeSub);
}

void darken(u16* screen, u8 factor, s16 x, s16 y, s16 w, s16 h) {
    int t = y * 256;
    for (int v = y; v < y+h; v++) {
        t += x;
        for (int u = x; u < x+w; u++) {
            screen[t] = RGB15(((screen[t]&31)>>factor), (((screen[t]>>5)&31)>>factor),
                (((screen[t]>>10)&31)>>factor)) | BIT(15);
            t++;
        }
        t += 256 - (x+w);
    }
}

void resetVideo() {
	REG_BG0CNT = REG_BG1CNT = REG_BG2CNT = REG_BG3CNT = 0;
	REG_BG0CNT_SUB = REG_BG1CNT_SUB = REG_BG2CNT_SUB = REG_BG3CNT_SUB = 0;

    vramSetMainBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
	vramSetBankE(VRAM_E_LCD);
    vramSetBankF(VRAM_F_LCD);
    vramSetBankG(VRAM_G_LCD);
    vramSetBankH(VRAM_H_LCD);
    vramSetBankI(VRAM_I_LCD);

    memset(VRAM_A, 0, 4*128*1024 + 64*1024);

    memset(OAM,     0, SPRITE_COUNT * sizeof(SpriteEntry));
    memset(OAM_SUB, 0, SPRITE_COUNT * sizeof(SpriteEntry));

    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
}

void waitForAnyKey() {
    while (true) {
        scanKeys();
        if (keysDown() & (KEY_TOUCH|KEY_A|KEY_B|KEY_X|KEY_Y|KEY_START)) {
            break;
        }
        swiWaitForVBlank();
    }
}

void trimString(char* string) {
    int a = 0;
    int b = strlen(string);

    while (isspace(string[a]) && a < b) a++;
    while (isspace(string[b-1]) && b > a) b--;

    if (b-a <= 0) {
        //string length is zero
        string[0] = '\0';
        return;
    }
    memmove(string, string+a, b-a);
	string[b-a] = '\0';
}
void unescapeString(char* str) {
	char* s = str;
    while (*s != '\0') {
        if (*s == '\\') {
            s++;
            switch (*s) {
                case '\\': *str = '\\'; break;
                case '\'': *str = '\''; break;
                case '\"': *str = '\"'; break;
                case 'n':  *str = '\n'; break;
                case 'r':  *str = '\r'; break;
                case 't':  *str = '\t'; break;
                case 'f':  *str = '\f'; break;
                default: /*Error*/ break;
            }
        } else {
        	*str = *s;
        }
        str++;
        s++;
    }
    *str = '\0';
}

//Accepts version strings in a(.b(.c)?)? format
u32 versionStringToInt(char* string) {
	u32 version = 0;

    char* firstDot = strchr(string, '.');
    char* secondDot = NULL;
    if (firstDot) {
        *firstDot = '\0';
        secondDot = strchr(firstDot+1, '.');
        if (secondDot) {
            *secondDot = '\0';
        }
    }

    //Example result: 1.12.37 = 11237
    version = atoi(string)*10000;
    if (firstDot) {
        version += atoi(firstDot+1)*100;
        *firstDot = '.';
    }
    if (secondDot) {
        version += atoi(secondDot+1);
        *secondDot = '.';
    }
    return version;
}
void versionIntToString(char* out, u32 version) {
    sprintf(out, "%d.%d.%d", (int)(version/10000), (int)((version/100)%100), (int)(version%100));
}

bool mkdirs(const char* constPath) {
	char path[MAXPATHLEN];
	strcpy(path, constPath);

	char* slash = path;
	while ((slash = strchr(slash, '/')) != NULL) {
		*slash = '\0';
		mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);
		*slash = '/';
		slash++;
	}

	if (mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO) == 0) {
		return true;
	}
	if (errno == EEXIST) {
		return true; //Folder already exists. That's not a problem for us.
	}
	return false;
}
bool fexists(const char* filename) {
	FILE* file = fopen(filename, "rb");
	if (file) {
		fclose(file);
		return true;
	}
	return false;
}

void setupCapture(int bank) {
    REG_DISPCAPCNT = DCAP_ENABLE | DCAP_MODE(0) | DCAP_DST(0) | DCAP_SRC(0) | DCAP_SIZE(3) |
        DCAP_OFFSET(0) | DCAP_BANK(bank) | DCAP_B(15) | DCAP_A(0);
}
void waitForCapture() {
	while (REG_DISPCAPCNT & DCAP_ENABLE) {
		swiWaitForVBlank();
	}
}
