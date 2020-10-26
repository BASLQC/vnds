#include "textfield.h"
#include "gui_common.h"
#include "../text.h"

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

	text->PushState();
	text->SetBuffer(r.w, r.h);
	text->SetFontSize(fontsize);

	text->SetMargins(8, 8, -(text->GetLineHeight()>>3), 2);
	text->PrintLine(string);

	text->BlitToScreen(background, r.w, r.h, 0, 0, 0, 0, r.w, r.h);
	text->SetBuffer(0, 0);
	text->PopState();
}
bool TextField::OnTouch(u32& keysDown, u32& keysHeld, touchPosition touch) {

	//TODO: Show on-screen keyboard
	iprintf("Should be showing a keyboard now, but it's not finished yet");

	return true;
}

const char* TextField::GetText() {
	return string;
}

void TextField::SetText(const char* text) {
	if (string) free(string);

	string = (text ? strdup(text) : NULL);
}
