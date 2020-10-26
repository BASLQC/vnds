#include "gui.h"
#include "gui_common.h"

GUI::GUI() {
	historyL = 0;
	removeListL = 0;
	fade = 0;

	main_bg = sub_bg = NULL;
}
GUI::~GUI() {
	for (int n = 0; n < removeListL; n++) {
		delete removeList[n];
	}
	for (int n = historyL-1; n >= 0; n--) {
		delete history[n];
	}

	resetVideo();
}

void GUI::VideoInit() {
	resetVideo();
	lcdMainOnBottom();

    main_bg = (u16*)BG_BMP_RAM(2);
    sub_bg = (u16*)BG_BMP_RAM_SUB(2);

    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_MAIN_BG_0x06000000);
    vramSetBankC(VRAM_C_SUB_BG_0x06200000);
    vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
	vramSetBankE(VRAM_E_TEX_PALETTE);
    vramSetBankF(VRAM_F_MAIN_SPRITE);

    REG_BG0CNT = BG_PRIORITY(1);

	bg2 = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 8, 0);
	bgSetPriority(bg2, 0);
	int bg3 = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 2, 0);
	bgSetPriority(bg3, 2);

	int bg1 = bgInitSub(1, BgType_Text4bpp, BgSize_T_256x256, 7, 0);
	bgSetPriority(bg1, 1);
	BG_PALETTE_SUB[0] = RGB15(0, 0, 0);
	BG_PALETTE_SUB[255] = RGB15(31, 31, 31);

	int sbg3 = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 2, 0);
	bgSetPriority(sbg3, 2);

    memset(mainSprites, 0, SPRITE_COUNT * sizeof(SpriteEntry));
    memset(subSprites, 0, SPRITE_COUNT * sizeof(SpriteEntry));
    for (int n = 0; n < SPRITE_COUNT; n++) {
    	mainSprites[n].isHidden = true;
    	subSprites[n].isHidden = true;
    }
    UpdateOAM();

    //if (GetActiveScreen()) {
		consoleInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x256, 7, 0, false, true);
		videoSetMode(DEFAULT_VIDEO_MODE);
		videoSetModeSub(MODE_5_2D | DISPLAY_BG1_ACTIVE | DISPLAY_BG3_ACTIVE);
	    init3D();
	    glFlush(0);
	    swiWaitForVBlank();
	//}
}
void GUI::UpdateOAM() {
    memcpy(OAM, mainSprites, SPRITE_COUNT * sizeof(SpriteEntry));
    memcpy(OAM_SUB, subSprites, SPRITE_COUNT * sizeof(SpriteEntry));
}

void GUI::PushScreen(Screen* s, GUITransition transition) {
	bool previousScreenNull = (historyL == 0);

	if (historyL == SCREEN_HISTORY_MAXSIZE) {
		//Push the leftmost element out of the array
		removeList[removeListL++] = history[0];
		for (int n = 1; n < historyL; n++) {
			history[n-1] = history[n];
		}
		historyL--;
	}

	const u32 frames    = 8;
	u32 oldVideoMode    = REG_DISPCNT;
	u32 oldVideoModeSub = REG_DISPCNT_SUB;
	bool fm = (transition == GT_fade || transition == GT_fadeMain);
	bool fs = (transition == GT_fade || transition == GT_fadeSub);

	//Append to history array
	if (!previousScreenNull) {
		if (transition == GT_slide) {
			vramSetBankD(VRAM_D_LCD);
			setupCapture(3);
			Draw();
			waitForCapture();
			vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
		} else if (fm || fs) {
		    if (fm) {
		    	videoSetMode(oldVideoMode & (~DISPLAY_SPR_ACTIVE));
		    	REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG3;
		    }
		    if (fs) {
				videoSetModeSub(oldVideoModeSub & ~DISPLAY_SPR_ACTIVE);
				REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BG3;
		    }
		    for (u16 n = 0; n <= frames; n++) {
		        if (fm) REG_BLDY = 0x1F * n / frames;
		        if (fs) REG_BLDY_SUB = 0x1F * n / frames;
		        swiWaitForVBlank();
		    }
		}
	}

	Screen* oldActive = GetActiveScreen();
	if (oldActive) {
		s->texture = oldActive->texture;
		oldActive->Deactivate();
	}

	history[historyL] = s;
	s->Activate();
	historyL++;

	if (!previousScreenNull) {
		if (transition == GT_slide) {
	        swiWaitForVBlank();
			bgSetPriority(bg2, 3);
			videoSetMode(oldVideoMode | DISPLAY_BG2_ACTIVE);
			for (int n = 0; n <= 16; n++) {
				REG_BG0HOFS = -256+(16*n);
				REG_BG3X = (-256+16*n) << 8;

				Draw();
				swiWaitForVBlank();
			}
			REG_BG0HOFS = 0;
			REG_BG3X = 0;
		} else if (fm || fs) {
			Draw();
			for (u16 n = 0; n <= frames; n++) {
		        if (fm) REG_BLDY = 0x1F - 0x1F * n / frames;
		        if (fs) REG_BLDY_SUB = 0x1F - 0x1F * n / frames;
		        swiWaitForVBlank();
		    }

		    if (fm) REG_BLDCNT = BLEND_NONE;
		    if (fs) REG_BLDCNT_SUB = BLEND_NONE;
		}
	}

	videoSetMode(oldVideoMode);
	videoSetModeSub(oldVideoModeSub);
}
void GUI::PopScreen(GUITransition transition) {
	Screen* remove = GetActiveScreen();
	if (remove && historyL > 1) {
		const u32 frames    = 8;
		u32 oldVideoMode    = REG_DISPCNT;
		u32 oldVideoModeSub = REG_DISPCNT_SUB;
		bool fm = (transition == GT_fade || transition == GT_fadeMain);
		bool fs = (transition == GT_fade || transition == GT_fadeSub);

		if (transition == GT_slide) {
			vramSetBankD(VRAM_D_LCD);
			setupCapture(3);
			Draw();
			waitForCapture();
			vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
		} else if (fm || fs) {
		    if (fm) {
		    	videoSetMode(oldVideoMode & (~DISPLAY_SPR_ACTIVE));
		    	REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG3;
		    }
		    if (fs) {
				videoSetModeSub(oldVideoModeSub & ~DISPLAY_SPR_ACTIVE);
				REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BG3;
		    }
		    for (u16 n = 0; n <= frames; n++) {
		        if (fm) REG_BLDY = 0x1F * n / frames;
		        if (fs) REG_BLDY_SUB = 0x1F * n / frames;
		        swiWaitForVBlank();
		    }
		}

	    //Remove later, because we want to be able to call PopScreen from within a screen.
		remove->Cancel();
    	removeList[removeListL++] = remove;

		historyL--;
		if (historyL > 0) {
			history[historyL-1]->Activate();
		}

		if (transition == GT_slide) {
			swiWaitForVBlank();

			bgSetPriority(bg2, 0);
			videoSetMode(DEFAULT_VIDEO_MODE | DISPLAY_BG2_ACTIVE);
			for (int n = 0; n <= 16; n++) {
				REG_BG2X = (-16*n) << 8;
				Draw();
				swiWaitForVBlank();
			}
			videoSetMode(DEFAULT_VIDEO_MODE);

			REG_BG2X = 0;
		} else if (fm || fs) {
			Draw();
			for (u16 n = 0; n <= frames; n++) {
		        if (fm) REG_BLDY = 0x1F - 0x1F * n / frames;
		        if (fs) REG_BLDY_SUB = 0x1F - 0x1F * n / frames;
		        swiWaitForVBlank();
		    }

		    if (fm) REG_BLDCNT = BLEND_NONE;
		    if (fs) REG_BLDCNT_SUB = BLEND_NONE;
		}

		videoSetMode(oldVideoMode);
		videoSetModeSub(oldVideoModeSub);
	}
}

void GUI::Update() {
	scanKeys();
	u32 down = keysDown();
	u32 held = keysHeld();

	touchPosition touch;
	touchRead(&touch);

	for (int n = 0; n < removeListL; n++) {
		delete removeList[n];
	}
	removeListL = 0;

	Screen* screen = GetActiveScreen();
	if (screen) {
		screen->Update(down, held, touch);
	}
}
void GUI::Draw() {
    Screen* screen = GetActiveScreen();
	if (screen) {
        glReset();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
	    glOrthof32(0, VERTEX_SCALE(256), VERTEX_SCALE(192), 0, floattof32(0.1), inttof32(100));

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glColor3f(1, 1, 1);

	  	glPolyFmt(DEFAULT_POLY_FMT | POLY_ID(0));

	    MATRIX_SCALE = VERTEX_SCALE_FACTOR;
		MATRIX_SCALE = VERTEX_SCALE_FACTOR;
		MATRIX_SCALE = inttof32(1);

	    glViewport(0, 0, 255, 191);
	    glTranslate3f32(0, 0, inttof32(-5));

		if (screen->Draw()) {
			DC_FlushRange(screen->background, 256*192*2);
			dmaCopy(screen->background, main_bg, 256*192*2);
		}
		screen->DrawTopScreen();

		glPopMatrix(1);
		glFlush(0);
	}
	UpdateOAM();
}

Screen* GUI::GetActiveScreen() {
	return (historyL > 0 ? history[historyL-1] : NULL);
}
