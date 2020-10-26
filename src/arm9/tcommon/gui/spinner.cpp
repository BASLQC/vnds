#include "spinner.h"

#include "../text.h"
#include "gui_common.h"

using namespace std;

Spinner::Spinner(Screen* screen, Rect l, Rect lp, Rect r, Rect rp, u8 fontsize) : Widget(screen) {
	mode = SM_choice;
	minval = maxval = 0;
	selected = 0;
	listener = NULL;

	spinLeftButton = new Button(screen);
	spinLeftButton->SetButtonListener(this);
	spinLeftButton->SetBounds(256, 192, l.w, l.h);
	spinLeftButton->SetForegroundImage(l);
	spinLeftButton->SetPressedImage(lp);

	spinRightButton = new Button(screen);
	spinRightButton->SetButtonListener(this);
	spinRightButton->SetBounds(256, 192, r.w, r.h);
	spinRightButton->SetForegroundImage(r);
	spinRightButton->SetPressedImage(rp);

	text = new Text();
	text->SetFontSize(fontsize);
}
Spinner::~Spinner() {
	screen->RemoveWidget(spinLeftButton);
	screen->RemoveWidget(spinRightButton);

	delete spinLeftButton;
	delete spinRightButton;

	delete text;
}

void Spinner::RemoveAll() {
	choices.clear();
	colors.clear();
}

void Spinner::AddChoice(const char* label, int value) {
	mode = SM_choice;

	SpinnerChoice choice;
	strncpy(choice.label, label, 63);
	choice.label[63] = '\0';
	choice.value = value;
	choices.push_back(choice);
}
void Spinner::AddColor(u16 c) {
	c |= BIT(15);
	mode = SM_color;

	for (u32 n = 0; n < colors.size(); n++) {
		if (colors[n] == c) {
			return;
		}
	}
	colors.push_back(c);
}

void Spinner::DrawBackground() {
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

	int ml = spinLeftButton->GetWidth()+8;
	int mr = spinRightButton->GetWidth()+8;
	int mt = -(text->GetLineHeight()>>3);
	int mb = 2;

	char valstr[64] = "";
	if (mode == SM_numeric) {
		text->SetMargins(ml, mr, mt, mb);

		sprintf(valstr, "%02d", selected);
		int spaceLeft = r.w-text->GetPenX()-text->GetMarginRight();
		text->SetPen(text->GetPenX() + (spaceLeft-text->GetLineHeight())/2, text->GetPenY());
		text->PrintLine(valstr);
	} else if (mode == SM_choice) {
		if (selected >= 0 && selected < (int)choices.size()) {
			strncpy(valstr, choices[selected].label, 63);
			valstr[63] = '\0';
		}
		text->SetMargins(0, 0, 0, 0);
		text->PrintLine(valstr);
		text->ClearBuffer();

		text->SetMargins(ml+(r.w-ml-mr-text->GetPenX())/2, mr, mt, mb);
		text->PrintLine(valstr);
	} else if (mode == SM_color) {
		if (selected >= 0 && selected < (int)colors.size()) {
			u16 temp[16*16];
			for (int n = 0; n < 16*16; n++) temp[n] = colors[selected];

			blit(temp, 16, 16, background, r.w, r.h, 0, 0, (r.w-16)/2, (r.h-16)/2, 16, 16);
		}
	}

	text->BlitToScreen(background, r.w, r.h, 0, 0, 0, 0, r.w, r.h);
	text->SetBuffer(0, 0);
	text->PopState();
}

void Spinner::OnButtonPressed(Button* button) {
	if (button == spinLeftButton) {
		SetSelected(selected-1);
	} else if (button == spinRightButton) {
		SetSelected(selected+1);
	}
}

void Spinner::SetNumeric(int x0, int x1) {
	mode = SM_numeric;
	minval = x0;
	maxval = x1;
	selected = minval;
}

SpinnerChoice* Spinner::GetChoiceValue() {
	if (selected >= 0 && selected < (int)choices.size()) {
		return &choices[selected];
	}
	return NULL;
}
int Spinner::GetNumericValue() {
	return selected;
}
u16 Spinner::GetColorValue() {
	if (selected >= 0 && selected < (int)colors.size()) {
		return colors[selected];
	}
	return RGB15(31, 0, 31)|BIT(15);
}

void Spinner::SetSpinnerListener(SpinnerListener* l) {
	listener = l;
}

void Spinner::SetSelectedChoice(int v) {
	for (u32 n = 0; n < choices.size(); n++) {
		if (choices[n].value == v) {
			SetSelected(n);
			return;
		}
	}
}
void Spinner::SetSelected(int s) {
	if (mode == SM_numeric) {
		if (s < minval) s = minval;
		if (s > maxval) s = maxval;
	} else if (mode == SM_choice) {
		if (s < 0) s = choices.size()-1;
		if (s > (int)choices.size()-1) s = 0;
	} else if (mode == SM_color) {
		if (s < 0) s = colors.size()-1;
		if (s > (int)colors.size()-1) s = 0;
	}

	if (selected != s) {
		selected = s;
		SetBackgroundDirty();

		if (listener) listener->OnSpinnerChanged(this);
	}
}

void Spinner::SetBounds(s16 x, s16 y, u16 w, u16 h) {
	SetBounds(Rect(x, y, w, h));
}
void Spinner::SetBounds(Rect r) {
	Widget::SetBounds(r);

	if (spinLeftButton && spinRightButton) {
		spinLeftButton->SetPos(r.x, r.y+(r.h-spinLeftButton->GetHeight())/2);
		spinRightButton->SetPos(r.x+r.w-spinRightButton->GetWidth(), r.y+(r.h-spinRightButton->GetHeight())/2);
	}
}

