#include "textfield.h"
#include "gui_common.h"
#include "../text.h"

#define DVK_BACKSPACE 8
#define DVK_ENTER 10

TextField::TextField(Screen* screen, u8 size, const char* str) : Widget(screen) {
	string = (str ? strdup(str) : NULL);
	fontsize = size;

	text = new Text();
	text->SetFontSize(fontsize);
}
TextField::~TextField() {
	if (string) free(string);

	delete text;
}

void TextField::DrawBackground() {
	Rect r = GetBounds();
	if (!image && background) {
		int s = r.w * r.h;
		for (int n = 0; n < s; n++) {
			background[n] = BIT(15);
		}
	}

	Widget::DrawBackground();

	if (string) {
		text->PushState();
		text->SetBuffer(r.w, r.h);
		text->SetFontSize(fontsize);

		text->SetMargins(8, 8, -(text->GetLineHeight()>>3), 2);
		text->PrintLine(string);

		text->BlitToScreen(background, r.w, r.h, 0, 0, 0, 0, r.w, r.h);
		text->SetBuffer(0, 0);
		text->PopState();
	}
}

bool TextField::OnTouch(u32& down, u32& held, touchPosition touch) {
	if (!(down & KEY_TOUCH)) {
		return true;
	}

	u32 oldVideoMode = REG_DISPCNT;
	videoSetMode(MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG2_ACTIVE);

	bgSetPriority(screen->gui->bg2, 3);
    u16* bg = (u16*)BG_BMP_RAM(8);
    vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    dmaFillHalfWords(BIT(15), bg, 256*256*sizeof(u16));

    u16* buffer = new u16[256*128];
    dmaFillHalfWords(BIT(15), buffer, 256*128*sizeof(u16));
    dmaCopy(buffer, bg, 256*128*sizeof(u16));

	text->PushState();
    text->SetBuffer(256, 128);
    text->SetFont("mono.ttf");

    char str[1024];
    int strL = 0;
    if (string) {
    	strL = strlen(string);
    	memcpy(str, string, strL);
    }
    str[strL] = '\0';

    Keyboard* defaultKdb = keyboardGetDefault();
    Keyboard* kbd = keyboardInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x512,
    		defaultKdb->mapBase, defaultKdb->tileBase, true, true);
	kbd->scrollSpeed = 0;
	keyboardShow();

	dmaCopy(kbd->palette, BG_PALETTE, kbd->paletteLen);
	bool redraw = true;
	while (1) {
		keyboardUpdate();
		if (keysDown() & KEY_TOUCH) {
			touchRead(&touch);

			int key = keyboardGetKey(touch.px, touch.py);
			if (key == DVK_ENTER) {
				break;
			} else {
				if (key > 31 && key < 127) { //printable character
					str[strL++] = (char)key;
					str[strL] = '\0';

				} else if (key == DVK_BACKSPACE) {
					str[--strL] = '\0';
				}
				redraw = true;
			}
		}
		if (redraw) {
			text->ClearBuffer();
			text->SetFontSize(16);
			text->SetMargins(8, 8, 8, 8);
			text->PrintString(str);

			dmaFillHalfWords(BIT(15), buffer, 256*128*sizeof(u16));
			text->BlitToScreen(buffer, 256, 128, 0, 0, 0, 0, 256, 128);
			dmaCopy(buffer, bg, 256*128*sizeof(u16));

			redraw = false;
		}
		swiWaitForVBlank();
	}

	scanKeys();
	down = 0;

	text->SetBuffer(0, 0);
	text->PopState();

	delete[] buffer;
    vramSetBankD(VRAM_D_LCD);
	dmaFillHalfWords(0, screen->gui->main_bg, 256*192*sizeof(u16));

	SetText(str);

	screen->SetBackgroundDirty();
    REG_DISPCNT = oldVideoMode;
    REG_BG0CNT = BG_PRIORITY(1);
	return true;
}

const char* TextField::GetText() {
	return string;
}

void TextField::SetText(const char* text) {
	if (string) free(string);

	string = (text ? strdup(text) : NULL);
}
