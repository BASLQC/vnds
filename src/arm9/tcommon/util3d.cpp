#include "common.h"

#define GL_FOG_SHIFT(n) ((n) << 8)

void glReset() {
    while (GFX_BUSY);

    //Clear FIFO
    GFX_STATUS |= (1 << 29);

    //Clear overflows for list memory
    GFX_CONTROL = GL_COLOR_UNDERFLOW | GL_POLY_OVERFLOW | GL_TEXTURE_2D | GL_BLEND
    		//| GL_OUTLINE
    		/*| GL_FOG | GL_FOG_SHIFT(0)*/
    		;
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

	glMaterialf(GL_AMBIENT,  RGB15(16, 16, 16));
	glMaterialf(GL_DIFFUSE,  RGB15(16, 16, 16));
	glMaterialf(GL_SPECULAR, RGB15( 8,  8,  8) | BIT(15));
	glMaterialf(GL_EMISSION, RGB15(16, 16, 16));
    glMaterialShinyness();

    glLight(0, RGB15(31, 31, 31), 0, 0, inttov10(-1));
	glColor(RGB15(16, 16, 16));

	/*
	int fogAlpha = 31;
	GFX_FOG_COLOR = RGB15(31, 0, 0) | ((fogAlpha&31) << 16);
	GFX_FOG_OFFSET = 0;
	for (int n = 0; n <= 31; n++) {
		GFX_FOG_TABLE[n] = 127 * n / 31;
	}
	*/
}
