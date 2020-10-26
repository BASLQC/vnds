#include "label.h"

#include "../text.h"
#include "gui_common.h"

Label::Label(Screen* screen, const char* label, u8 fontsize) : Widget(screen) {
	this->label = strdup(label);
	this->fontsize = fontsize;

	text = new Text();
}
Label::~Label() {
	delete text;
	free(label);
}

void Label::DrawBackground() {
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
	text->PrintLine(label);

	text->BlitToScreen(background, r.w, r.h, 0, 0, 0, 0, r.w, r.h);
	text->SetBuffer(0, 0);
	text->PopState();
}
