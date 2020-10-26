#include "screen.h"

using namespace std;

#include "gui_common.h"

Screen::Screen(GUI* g) {
	gui = g;
	backgroundDirty = true;
	active = false;
	textureImage = NULL;
	background = new u16[256*192];
	backgroundImage = NULL;
}

Screen::~Screen() {
	while (!widgets.empty()) {
		Widget* w = widgets.front();
		widgets.pop_front();
		delete w;
	}
	delete[] background;
}

void Screen::AddWidget(Widget* w) {
	if (w) {
		widgets.push_back(w);
	}
}
void Screen::RemoveWidget(Widget* w) {
	if (w) {
		list<Widget*>::iterator i;
		for (i = widgets.begin(); i != widgets.end(); i++) {
			if (*i == w) {
				widgets.erase(i);
				break;
			}
		}
	}
}

void Screen::Activate() {
	active = true;

	SetTexture(textureImage);

	list<Widget*>::reverse_iterator i;
	for (i = widgets.rbegin(); i != widgets.rend(); i++) {
		(*i)->Activate();
	}

	SetBackgroundDirty();
}
void Screen::Deactivate() {
	list<Widget*>::reverse_iterator i;
	for (i = widgets.rbegin(); i != widgets.rend(); i++) {
		(*i)->FlushChanges();
	}

	//texture.id = -1;
	active = false;
}
void Screen::Cancel() {
	list<Widget*>::reverse_iterator i;
	for (i = widgets.rbegin(); i != widgets.rend(); i++) {
		(*i)->Cancel();
	}
}
void Screen::Update(u32& keysDown, u32& keysHeld, touchPosition touch) {
	list<Widget*>::reverse_iterator i;
	for (i = widgets.rbegin(); i != widgets.rend(); i++) {
		Widget* w = *i;
		if (w->IsVisible()) {
			w->Update(keysDown, keysHeld, touch);
		}
	}
}
void Screen::DrawBackground() {
	if (backgroundImage) {
		memcpy(background, backgroundImage, 256*192*2);
	} else {
		for (int n = 0; n < 256*192; n++) {
			background[n] = BIT(15);
		}
	}
}
void Screen::DrawForeground() {
}
bool Screen::Draw() {
	list<Widget*>::iterator i;

	bool bgChanged = backgroundDirty;
	for (i = widgets.begin(); i != widgets.end(); i++) {
		Widget* w = *i;
		if (!w->IsTransparent() && w->IsVisible() && w->IsBackgroundDirty()) {
			bgChanged = true;
		}
	}
	if (bgChanged) {
		DrawBackground();
	}
	if (bgChanged) {
		for (i = widgets.begin(); i != widgets.end(); i++) {
			Widget* w = *i;
			if (!w->IsTransparent() && w->IsVisible()) {
				if (backgroundDirty || w->IsBackgroundDirty()) {
					w->SetBackgroundDirty(false);
					w->DrawBackground();
				}
				blit2(w->background, w->GetWidth(), w->GetHeight(), background, 256, 192, 0, 0,
						w->GetX(), w->GetY(), w->GetWidth(), w->GetHeight());
			}
		}

		bgChanged = true;
	}

	int t = 0;
	for (i = widgets.begin(); i != widgets.end(); i++) {
		Widget* w = *i;
		if (w->IsVisible()) {
			glPolyFmt(DEFAULT_POLY_FMT | POLY_ID(t&31));
			w->DrawForeground();
		}
		t++;
	}
	DrawForeground();

	bool result = bgChanged || backgroundDirty;
	backgroundDirty = false;
	return result;
}
void Screen::DrawTopScreen() {

}
void Screen::SetBackgroundDirty(bool v) {
	backgroundDirty = v;
}
void Screen::SetBackgroundImage(u16* bgi) {
	backgroundImage = bgi;
	backgroundDirty = true;
}
void Screen::SetTexture(u16* texImg) {
	textureImage = texImg;

	if (active && texImg) {
		//Generate Texture
		glResetTextures();

		int ids[1];
		glGenTextures(1, ids);
		memset(&texture, 0, sizeof(TextureData));
		texture.id = ids[0];

		//Load texture
		glBindTexture(0, texture.id);
		texture.format = GL_RGBA;
		texture.size = 256*256*2;

	    vramSetBankA(VRAM_A_LCD);
		dmaCopy(textureImage, VRAM_A, texture.size);
		glTexParameter(TEXTURE_SIZE_256, TEXTURE_SIZE_256, (u32*)VRAM_A, GL_RGBA,
				(GL_TEXTURE_PARAM_ENUM)(TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T));

	    vramSetBankA(VRAM_A_TEXTURE);
	}
}
