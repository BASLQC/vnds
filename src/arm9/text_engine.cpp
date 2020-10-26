#include "text_engine.h"

#include "../common/fifo.h"
#include "as_lib9.h"
#include "graphics_engine.h"
#include "res.h"
#include "sound_engine.h"
#include "vnchoice.h"
#include "vnds.h"
#include "maintextpane.h"
#include "menu/vnmenu.h"
#include "menu/vnsavemenu.h"
#include "tcommon/text.h"
#include "tcommon/gui/gui_common.h"

namespace text_engine {

class BacklightButton : public Button {
	private:

	public:
		BacklightButton(TextEngine* te) : Button(te) {
		}
		virtual void DrawForeground() {
			textureUV.x += (backlight<<4);
			pressedUV.x += (backlight<<4);

			Button::DrawForeground();

			textureUV.x -= (backlight<<4);
			pressedUV.x -= (backlight<<4);
		}

};

class MuteButton : public Button {
	private:
		VNDS* vnds;

	public:
		MuteButton(VNDS* vnds, TextEngine* te) : Button(te) {
			this->vnds = vnds;
		}
		virtual void DrawForeground() {
			u8 offset = (vnds->soundEngine->IsMuted() ? 0 : 1);

			textureUV.x += (offset<<4);
			pressedUV.x += (offset<<4);

			Button::DrawForeground();

			textureUV.x -= (offset<<4);
			pressedUV.x -= (offset<<4);
		}
};

}

//------------------------------------------------------------------------------

using namespace text_engine;

TextEngine::TextEngine(GUI* gui, VNDS* vnds, u8 fontsize) : Screen(gui) {
	this->vnds = vnds;

	frame = 0;
	backgroundColor = BIT(15);

	char fontPath[MAXPATHLEN];
	sprintf(fontPath, "%s/font.ttf", skinPath);
	if (!fexists(fontPath)) {
		sprintf(fontPath, "%s/font.ttf", baseSkinPath);
	}
	if (!fexists(fontPath)) {
		sprintf(fontPath, "%s", DEFAULT_FONT_PATH);
	}
	fontCache = new FontCache(fontPath, 64*1024);

	textPane = new MainTextPane(this, fontCache);
	textPane->SetFontSize(preferences->GetFontSize() > 0 ? preferences->GetFontSize() : fontsize, true);
	textPane->SetBounds(0, 0, 257, 176);

	choiceView = new VNChoice(vnds, this, fontCache);
	choiceView->SetFontSize(preferences->GetFontSize() > 0 ? preferences->GetFontSize() : fontsize, true);
	choiceView->SetBounds(0, 0, textPane->GetWidth(), textPane->GetHeight());
	choiceView->SetVisible(false);

	backlightButton = new BacklightButton(this);
	backlightButton->SetButtonListener(this);
	backlightButton->SetForegroundImage(Rect(112, 0, 16, 16));
	backlightButton->SetPressedImage(Rect(112, 16, 16, 16));
	backlightButton->SetBounds(64, 176, 16, 16);
	backlightButton->SetActivationKeys(KEY_SELECT);

	muteButton = new MuteButton(vnds, this);
	muteButton->SetButtonListener(this);
	muteButton->SetForegroundImage(Rect(176, 0, 16, 16));
	muteButton->SetPressedImage(Rect(176, 16, 16, 16));
	muteButton->SetBounds(80, 176, 16, 16);

	repeatSoundButton = new Button(this);
	repeatSoundButton->SetButtonListener(this);
	repeatSoundButton->SetForegroundImage(Rect(208, 0, 17, 16));
	repeatSoundButton->SetPressedImage(Rect(208, 16, 17, 16));
	repeatSoundButton->SetBounds(96, 176, 17, 16);

	saveButton = new Button(this);
	saveButton->SetButtonListener(this);
	saveButton->SetForegroundImage(Rect(64, 0, 16, 16));
	saveButton->SetPressedImage(Rect(64, 16, 16, 16));
	saveButton->SetBounds(120, 176, 16, 16);

	loadButton = new Button(this);
	loadButton->SetButtonListener(this);
	loadButton->SetForegroundImage(Rect(80, 0, 16, 16));
	loadButton->SetPressedImage(Rect(80, 16, 16, 16));
	loadButton->SetBounds(136, 176, 16, 16);

	menuButton = new Button(this);
	menuButton->SetButtonListener(this);
	menuButton->SetForegroundImage(Rect(96, 0, 17, 16));
	menuButton->SetPressedImage(Rect(96, 16, 17, 16));
	menuButton->SetBounds(152, 176, 17, 16);

    vnLogSetScrollPane(textPane);
}
TextEngine::~TextEngine() {
    vnLogSetScrollPane(NULL);
    delete fontCache;
}

void TextEngine::Reset() {
	choiceView->Deactivate();
	choiceView->SetVisible(false);
	textPane->RemoveAllItems();
	textPane->SetVisible(true);
}
MainTextPane* TextEngine::GetTextPane() {
	return textPane;
}
VNChoice* TextEngine::GetChoiceView() {
	return choiceView;
}

void TextEngine::SetTexture(u16* textureI) {
	if (textureI) {
		blit(textureI, 256, 256, bottomBarI, 16, 16, 225, 0, 0, 0, 16, 16);
	}

	Screen::SetTexture(textureI);

	RepaintTime();
}

void TextEngine::Update(u32& down, u32& held, touchPosition touch) {
	Screen::Update(down, held, touch);

	if (!choiceView->IsActive() || (held & KEY_L)) {
		choiceView->SetVisible(false);
		textPane->SetVisible(true);
	} else {
		choiceView->SetVisible(true);
		textPane->SetVisible(false);
	}

	///if (textPane->GetScroll() >= textPane->GetNumberOfItems() - textPane->GetVisibleItems()) {
	//	if ((down & KEY_TOUCH) && vnds->IsWaitingForInput()) {
	//		vnds->Continue();
	//	}
	//}

	if ((frame & 511) == 0) {
		RepaintTime();
	}

	frame++;
}

void TextEngine::DrawBackground() {
	dmaFillHalfWords(backgroundColor, background, 256*192*sizeof(u16));
}

void TextEngine::RepaintTime() {
    //Redraw time texture
	int w = 88;
	int h = 16;
	u16 temp[w * h];
	for (int n = 0; n < w; n += h) {
		blit(bottomBarI, h, h, temp, w, h, 0, 0, n, 0, h, h);
	}

	char timeStr[32];
	printTime(timeStr);

	Text* text = new Text();
	text->SetBuffer(w, h);
	text->SetFontSize(10);
	text->SetMargins(0, 0, 0, 0);
	text->PrintLine(timeStr);
	int dx = w - text->GetPenX() - 4;
	int dy = -1;
	text->BlitToScreen(temp, w, h, 0, 0, dx, dy, w, h);
	delete text;

	swiWaitForVBlank();
	vramSetBankA(VRAM_A_LCD);
	blit(temp, w, h, VRAM_A, 256, 256, 0, 0, 64, 32, w, h);
    vramSetBankA(VRAM_A_TEXTURE);
}

void TextEngine::DrawWaitingIcon(u32 frame) {
	int a = (frame>>3) & 7;
	int s = (a < 4 ? 16 : -16);
	v16 vw = VERTEX_SCALE(16);
	v16 vh = VERTEX_SCALE(16);

	glPolyFmt(BASE_POLY_FMT | POLY_ID(32) | POLY_ALPHA(31));
	glTranslate3f32(0, 0, 5);
	drawQuad(texture, inttof32(0), inttof32(176), vw, vh,
			Rect(32 + 16 * (a&1) + (s > 0 ? 0 : -s-1),
					32 + 16 * ((a&3)>>1) + (s > 0 ? 0 : -s), s, s));
	glTranslate3f32(0, 0, -5);
	glPolyFmt(DEFAULT_POLY_FMT);
}

void TextEngine::DrawForeground() {
	static u32 drawFrame = 0;

	if (vnds->IsWaitingForInput()) {
		v16 vw = VERTEX_SCALE(16);
		v16 vh = VERTEX_SCALE(16);

		int alpha = 30;
		if (preferences->GetWaitInputAnim() == 1) {
			//int pulse = 15 * SIN[(drawFrame<<3) & 511];
			//libnds degrees in circle changed from 512 to 32768, this is a quick fix
			int oldAngle = (drawFrame * 10 / 4) & 511;
			s32 pulse = 15 * sinLerp((oldAngle << 6) & (DEGREES_IN_CIRCLE-1));

			alpha = 15 + MIN(15, MAX(-15, pulse>>11));
		}

		if (alpha > 0) {
			glPolyFmt(BASE_POLY_FMT | POLY_ID(32) | POLY_ALPHA(1 + alpha));
			glTranslate3f32(0, 0, 5);
			drawQuad(texture, inttof32(0), inttof32(176), vw, vh, Rect(16, 48, 16, 16));
			glTranslate3f32(0, 0, -5);
			glPolyFmt(DEFAULT_POLY_FMT);
		}

		drawFrame++;
	} else if (vnds->GetDelay() > 0) {
		DrawWaitingIcon(drawFrame);
		drawFrame++;
	} else {
		drawFrame = 0;
	}

	int vw, vh;

	vw = VERTEX_SCALE(88);
	vh = VERTEX_SCALE(16);
	drawQuad(texture, inttof32(256-88), inttof32(192-16), vw, vh, Rect(64, 32, 88, 16));

	vw = VERTEX_SCALE(256);
	vh = VERTEX_SCALE(16);
	drawQuad(texture, 0, inttof32(176), vw, vh, Rect(225, 0, 16, 16));

	Screen::DrawForeground();
}

void TextEngine::ShowMenu() {
	vnds->graphicsEngine->ClearCache();
	vnds->soundEngine->StopSound();
	gui->PushScreen(new VNMenu(gui, textureImage, vnds), GT_fadeMain);
}
void TextEngine::ShowLoadMenu() {
	vnds->graphicsEngine->ClearCache();
	vnds->soundEngine->StopSound();
	gui->PushScreen(new VNSaveMenu(gui, textureImage, vnds, false), GT_fadeMain);
}
void TextEngine::ShowSaveMenu() {
	vnds->graphicsEngine->ClearCache();
	vnds->soundEngine->StopSound();
	gui->PushScreen(new VNSaveMenu(gui, textureImage, vnds, true), GT_fadeMain);
}

void TextEngine::OnButtonPressed(Button* button) {
	if (button == backlightButton) {
		toggleBacklight();
	} else if (button == saveButton) {
		ShowSaveMenu();
	} else if (button == loadButton) {
		ShowLoadMenu();
	} else if (button == menuButton) {
		ShowMenu();
	} else if (button == muteButton) {
		vnds->soundEngine->SetMuted(!vnds->soundEngine->IsMuted());
	} else if (button == repeatSoundButton) {
		vnds->soundEngine->ReplaySound();
	}
}

void TextEngine::SetBackgroundColor(u16 bgc) {
	backgroundColor = BIT(15)|bgc;

	textPane->SetBackgroundColor(backgroundColor);
	choiceView->SetBackgroundColor(backgroundColor);

	SetBackgroundDirty();
}
