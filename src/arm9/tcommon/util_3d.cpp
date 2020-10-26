#include "util_3d.h"
#include <stdio.h>
#include "common.h"

void glReset() {
    while (GFX_STATUS & (1 << 27)); //Wait

    // Clear the FIFO
    GFX_STATUS |= (1 << 29);

    // Clear overflows for list memory
    GFX_CONTROL = ((1 << 12)|(1 << 13)) | GL_TEXTURE_2D | GL_ANTIALIAS | GL_BLEND | GL_FOG;
}


void drawQuad(TextureData* texture, s32 x, s32 y, v16 vw, v16 vh, Rect uv) {
    glPushMatrix();

    glTranslate3f32(x, y, 0);

    MATRIX_SCALE = INV_VERTEX_SCALE_FACTOR;
	MATRIX_SCALE = INV_VERTEX_SCALE_FACTOR;
	MATRIX_SCALE = inttof32(1);

    //glRotateZi(drawAngle);

	if (texture) {
		glBindTexture(GL_TEXTURE_2D, texture->id);
		if (texture->format != GL_RGB && texture->format != GL_RGBA) {
			glColorTable(texture->format, texture->pal_addr);
		}
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	int s = 2;
    int a, b, c, d;
    c = vw + s;
    d = vh + s;
    a = s;
    b = s;

	glBegin(GL_QUAD);
		glNormal(NORMAL_PACK(0, 0, inttov10(1)));
		glTexCoord2t16(uv.x << 4, uv.y << 4);
        glVertex3v16(a, b, 0);
		glTexCoord2t16((uv.x + uv.w) << 4, uv.y << 4);
        glVertex3v16(c, b, 0);
		glTexCoord2t16((uv.x + uv.w) << 4, (uv.y + uv.h) << 4);
        glVertex3v16(c, d, 0);
		glTexCoord2t16(uv.x << 4, (uv.y + uv.h) << 4);
        glVertex3v16(a, d, 0);
	glEnd();

	glPopMatrix(1);
}

void init3D() {
	glInit();

    glClearColor(0, 0, 0, 0);
	glClearPolyID(63); // BG must have a unique polygon ID for AA to work
	glClearDepth(0x7FFF);

	//Set our viewport to be the same size as the screen
	glViewport(0, 0, 255, 191);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glOrthof32(0, VERTEX_SCALE(256), VERTEX_SCALE(192), 0, floattof32(0.1), inttof32(100));

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

	glMaterialf(GL_AMBIENT, RGB15(16, 16, 16));
	glMaterialf(GL_DIFFUSE, RGB15(16, 16, 16));
	glMaterialf(GL_SPECULAR, BIT(15) | RGB15(8, 8, 8));
	glMaterialf(GL_EMISSION, RGB15(16, 16, 16));
    glMaterialShinyness();

	glLight(0, RGB15(31, 31, 31), 0, floattov10(-1.0), 0);
}

void loadTexture(TextureData* target, const char* format, const char* dtaFile,
    const char* palFile, GL_TEXTURE_PARAM_ENUM param)
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

    loadTexture(target, f, dtaFile, palFile, param);
}










uint16* vramGetBanka(uint16 *addr) {
//---------------------------------------------------------------------------------
	if(addr >= VRAM_A && addr < VRAM_B)
		return VRAM_A;
	else if(addr >= VRAM_B && addr < VRAM_C)
		return VRAM_B;
	else if(addr >= VRAM_C && addr < VRAM_D)
		return VRAM_C;
	else if(addr >= VRAM_D && addr < VRAM_E)
		return VRAM_D;
	else if(addr >= VRAM_E && addr < VRAM_F)
		return VRAM_E;
	else if(addr >= VRAM_F && addr < VRAM_G)
		return VRAM_F;
	else if(addr >= VRAM_G && addr < VRAM_H)
		return VRAM_G;
	else if(addr >= VRAM_H && addr < VRAM_I)
		return VRAM_H;
	else return VRAM_I;
}


//---------------------------------------------------------------------------------
int vramIsTextureBanka(uint16 *addr) {
//---------------------------------------------------------------------------------
	uint16* vram = vramGetBanka(addr);

	if(vram == VRAM_A)
	{
		if((VRAM_A_CR & 3) == ((VRAM_A_TEXTURE) & 3))
			return 1;
		else return 0;
	}
	else if(vram == VRAM_B)
	{
		if((VRAM_B_CR & 3) == ((VRAM_B_TEXTURE) & 3))
			return 1;
		else return 0;
	}
	else if(vram == VRAM_C)
	{
		if((VRAM_C_CR & 3) == ((VRAM_C_TEXTURE) & 3))
			return 1;
		else return 0;
	}
	else if(vram == VRAM_D)
	{
		if((VRAM_D_CR & 3) == ((VRAM_D_TEXTURE) & 3))
			return 1;
		else return 0;
	}
	else
		return 0;
}

uint32* getNextTextureSlota(int size) {
//---------------------------------------------------------------------------------
	uint32* result = glGlob->nextBlock;
	glGlob->nextBlock += size >> 2;

	//uh-oh...out of texture memory in this bank...find next one assigned to textures
	while(!vramIsTextureBanka((uint16*)glGlob->nextBlock - 1) && glGlob->nextBlock <= (uint32*)VRAM_E)
	{
		glGlob->nextBlock = (uint32*)vramGetBanka((uint16*)result) + (0x20000 >> 2); //next bank
		result = glGlob->nextBlock;
		glGlob->nextBlock += size >> 2;
	}

	if(glGlob->nextBlock > (uint32*)VRAM_E) {
		result = 0;
	}
	return result;
}

int glTexImage2Da(int target, int empty1, GL_TEXTURE_TYPE_ENUM type, int sizeX, int sizeY, int empty2, GL_TEXTURE_PARAM_ENUM  param, const uint8* texture) {
//---------------------------------------------------------------------------------
	uint32 size = 0;
	uint32* addr;

	size = 1 << (sizeX + sizeY + 6);


	switch (type) {
		case GL_RGB:
		case GL_RGBA:
			size = size << 1;
			break;
		case GL_RGB4:
			size = size >> 2;
			break;
		case GL_RGB16:
			size = size >> 1;
			break;
		default:
			break;
	}

   addr = 0;//(u32*)DynamicArrayGet(&glGlob->texturePtrs, glGlob->activeTexture);
   //iprintf("a2 %p %p\n", addr, VRAM_A);


   if(!addr)
   {
	   addr = getNextTextureSlota(size);

      if(!addr)
         return 0;

      DynamicArraySet(&glGlob->texturePtrs, glGlob->activeTexture, addr);
   }

	// unlock texture memory
	//vramTemp = vramSetMainBanks(VRAM_A_LCD,VRAM_B_LCD,VRAM_C_LCD,VRAM_D_LCD);
	vramSetBankA(VRAM_A_LCD);
	vramSetBankE(VRAM_E_LCD);

	if (type == GL_RGB) {
		// We do GL_RGB as GL_RGBA, but we set each alpha bit to 1 during the copy
		u16 * src = (u16*)texture;
		u16 * dest = (u16*)addr;

		glTexParameter(sizeX, sizeY, addr, GL_RGBA, param);

		while (size--) {
			*dest++ = *src | (1 << 15);
			src++;
		}
	} else {
		   //iprintf("a1\n");

		   // For everything else, we do a straight copy
		glTexParameter(sizeX, sizeY, addr, type, param);
		   //iprintf("a2 %p %p\n", addr, VRAM_A);
		dmaCopy((uint32*)texture, addr, size);
		   //iprintf("a3\n");
		   //waitForAnyKey();
	}

	//vramRestoreMainBanks(vramTemp);
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankE(VRAM_E_TEX_PALETTE);

	return 1;
}










void loadTexture(TextureData* target, GL_TEXTURE_TYPE_ENUM format, const char* dtaFile,
    const char* palFile, GL_TEXTURE_PARAM_ENUM param)
{
    u8* dta = NULL;
    u8* pal = NULL;
    int dtaL = 0;
    int palL = 0;

    if (dtaFile) {
        FILE* f = fopen(dtaFile, "rb");
        if (f) {
            setvbuf(f, NULL, _IONBF, 0);
            fseek(f, 0, SEEK_END);
            dtaL = ftell(f);
            fseek(f, 0, SEEK_SET);
            dta = new u8[dtaL];
            fread(dta, 1, dtaL, f);
            fclose(f);
        }
    }
    if (palFile) {
        FILE* f = fopen(palFile, "rb");
        if (f) {
            setvbuf(f, NULL, _IONBF, 0);
            fseek(f, 0, SEEK_END);
            palL = ftell(f);
            fseek(f, 0, SEEK_SET);
            pal = new u8[palL];
            fread(pal, 1, palL, f);
            fclose(f);
        }
    }

    int bpp = 1;
    if (format == GL_RGB || format == GL_RGBA) {
        bpp = 2;
    }

    int pixels = dtaL / bpp;
    GL_TEXTURE_SIZE_ENUM size = TEXTURE_SIZE_64;
    if (pixels >= 1024*1024) size = TEXTURE_SIZE_1024;
    else if (pixels >= 512*512) size = TEXTURE_SIZE_512;
    else if (pixels >= 256*256) size = TEXTURE_SIZE_256;
    else if (pixels >= 128*128) size = TEXTURE_SIZE_128;
    else if (pixels >= 64*64) size = TEXTURE_SIZE_64;
    else if (pixels >= 32*32) size = TEXTURE_SIZE_32;
    else if (pixels >= 16*16) size = TEXTURE_SIZE_16;
    else size = TEXTURE_SIZE_8;

	glBindTexture(0, target->id);
	target->format = format;
	target->size = dtaL + palL;
	if (dta) {
		DC_FlushRange(dta, dtaL);
    	glTexImage2Da(0, 0, format, size, size, 0, param, dta);
    	delete[] dta;
    }
	if (pal) {
		DC_FlushRange(pal, palL);
    	target->pal_addr = gluTexLoadPal((u16*)pal, palL, format);
    	delete[] pal;
    }
}
