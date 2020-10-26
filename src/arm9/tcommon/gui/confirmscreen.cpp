#include "confirmscreen.h"

#include "gui_common.h"
#include "../text.h"

ConfirmScreen::ConfirmScreen(GUI* gui, const char* message,
		Rect ok, Rect okp, ButtonListener* okl,
		Rect cancel, Rect cancelp, ButtonListener* cancell)
: Screen(gui)
{
	this->message = strdup(message);
	this->okListener = okl;
	this->cancelListener = cancell;

	okButton = new Button(this);
	okButton->SetButtonListener(this);
	okButton->SetBounds(112 - ok.w, 112, ok.w, ok.h);
	okButton->SetForegroundImage(ok);
	okButton->SetPressedImage(okp);
	okButton->SetActivationKeys(KEY_A);

	cancelButton = new Button(this);
	cancelButton->SetButtonListener(this);
	cancelButton->SetBounds(144, 112, cancel.w, cancel.h);
	cancelButton->SetForegroundImage(cancel);
	cancelButton->SetPressedImage(cancelp);
	cancelButton->SetActivationKeys(KEY_B);

	text = new Text();
	text->SetBuffer(224, 64);
}

ConfirmScreen::~ConfirmScreen() {
	free(message);

	delete text;
}

void ConfirmScreen::DrawBackground() {
	Screen::DrawBackground();

	text->PushState();
	text->SetFontSize(14);

	text->ClearBuffer();
	text->SetMargins(0, 0, 0, 0);
	int lines = text->PrintString(message);

	text->ClearBuffer();
	text->SetMargins((224-text->GetPenX())/2, 0,
			64-text->GetLineHeight()*lines-(text->GetLineHeight()>>2), 0);
	text->PrintString(message);
	text->BlitToScreen(background, 256, 192, 0, 0, 16, 16, 224, 64);

	text->PopState();
}

void ConfirmScreen::OnButtonPressed(Button* button) {
	gui->PopScreen(GT_none);
	if (button == okButton) {
		if (okListener) okListener->OnButtonPressed(button);
	} else if (button == cancelButton) {
		if (cancelListener) cancelListener->OnButtonPressed(button);
	}
}
