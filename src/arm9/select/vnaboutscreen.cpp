#include "vnaboutscreen.h"

#include "../tcommon/text.h"
#include "../tcommon/gui/gui_common.h"

VNAboutScreen::VNAboutScreen(GUI* gui, u16* textureI) : Screen(gui) {
	imageLoaded = false;

	SetTexture(textureI);

	text = new Text();
	text->SetBuffer(256, 192);

	backButton = new Button(this);
	backButton->SetButtonListener(this);
	backButton->SetBounds(80, 160, 96, 32);
	backButton->SetForegroundImage(Rect(160, 0, 96, 32));
	backButton->SetPressedImage(Rect(160, 32, 96, 32));
	backButton->SetActivationKeys(KEY_A|KEY_B|KEY_START);
}
VNAboutScreen::~VNAboutScreen() {
	delete text;
}

void VNAboutScreen::Activate() {
	Screen::Activate();

	if (!imageLoaded) {
		if (skinLoadImage("%s/about_bg", bg, 256, 192)) {
			SetBackgroundImage(bg);
		}
		imageLoaded = true;
	}
}
void VNAboutScreen::DrawBackground() {
	Screen::DrawBackground();

	text->ClearBuffer();
	text->SetMargins(16, 16, 48, 48);

	text->SetColor(RGB15(8, 16, 31));
	text->SetFontSize(skin->GetAboutFontSize()+2);
	text->PrintLine(VERSION_STRING);
	text->PrintNewline();

	text->SetPen(text->GetPenX(), text->GetPenY()+8);

	text->SetColor(RGB15(31, 31, 31));
	text->SetFontSize(skin->GetAboutFontSize()+2);
	text->PrintLine("Original program\n");
	text->SetFontSize(skin->GetAboutFontSize());
	text->PrintLine("\tJake\n");

	text->SetPen(text->GetPenX(), text->GetPenY()+8);

	text->SetFontSize(skin->GetAboutFontSize()+2);
	text->PrintLine("Additional programming\n");
	text->SetFontSize(skin->GetAboutFontSize());
	text->PrintLine("\tanoNL\n");

	text->BlitToScreen(background);
}

void VNAboutScreen::OnButtonPressed(Button* button) {
	if (button == backButton) {
		gui->PopScreen();
	}
}
