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

int Screen::ListWidgets(Widget** out) {
	int t = 0;
	list<Widget*>::iterator i = widgets.begin();
	while (i != widgets.end()) {
		out[t++] = *i;
		++i;
	}
	return t;
}

int Screen::ListWidgetsReverse(Widget** out) {
	int t = 0;
	list<Widget*>::reverse_iterator i = widgets.rbegin();
	while (i != widgets.rend()) {
		out[t++] = *i;
		++i;
	}
	return t;
}

void Screen::Activate() {
	active = true;

	SetTexture(textureImage);

	Widget* ws[widgets.size()];
	int t = ListWidgetsReverse(ws);
	for (int n = 0; n < t; n++) {
		Widget* w = ws[n];
		w->Activate();
	}

	SetBackgroundDirty();
}
void Screen::Deactivate() {
	Widget* ws[widgets.size()];
	int t = ListWidgetsReverse(ws);
	for (int n = 0; n < t; n++) {
		Widget* w = ws[n];
		w->FlushChanges();
	}

	//texture.id = -1;
	active = false;
}
void Screen::Cancel() {
	Widget* ws[widgets.size()];
	int t = ListWidgetsReverse(ws);
	for (int n = 0; n < t; n++) {
		Widget* w = ws[n];
		w->Cancel();
	}
}
void Screen::Update(u32& keysDown, u32& keysHeld, touchPosition touch) {
	Widget* ws[widgets.size()];
	int t = ListWidgetsReverse(ws);

	for (int n = 0; n < t; n++) {
		Widget* w = ws[n];
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
	Widget* widgets[widgets.size()];
	int t = ListWidgets(widgets);

	bool bgChanged = backgroundDirty;
	for (int n = 0; n < t; n++) {
		Widget* w = widgets[n];
		if (!w->IsTransparent() && w->IsVisible() && w->IsBackgroundDirty()) {
			bgChanged = true;
		}
	}
	if (bgChanged) {
		DrawBackground();
	}
	if (bgChanged) {
		for (int n = 0; n < t; n++) {
			Widget* w = widgets[n];
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

	for (int n = 0; n < t; n++) {
		Widget* w = widgets[n];
		if (w->IsVisible()) {
			glPolyFmt(DEFAULT_POLY_FMT | POLY_ID(t&31));
			w->DrawForeground();
		}
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
		TextureManager* texmgr = &gui->texmgr;
		texmgr->Reset();
		texture = texmgr->AddTexture(TEXTURE_SIZE_256, TEXTURE_SIZE_256, GL_RGBA,
			(GL_TEXTURE_PARAM_ENUM)(TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T),
			textureImage, 256*256*sizeof(u16), NULL, 0);

		//iprintf("%08x\n", texture);

	}
}
