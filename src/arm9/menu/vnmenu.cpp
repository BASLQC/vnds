#include "vnmenu.h"

#include "../vnds.h"
#include "../text_engine.h"
#include "../vnchoice.h"
#include "../maintextpane.h"
#include "../tcommon/gui/gui_common.h"

VNMenu::VNMenu(GUI* gui, u16* textureI, VNDS* vnds) : Screen(gui) {
	this->vnds = vnds;

	//SetTexture(textureI);
	SetBackgroundImage(backgroundI);

	backButton = new Button(this);
	backButton->SetButtonListener(this);
	backButton->SetBounds(16, 160, 96, 32);
	backButton->SetForegroundImage(Rect(0, 128, 96, 32));
	backButton->SetPressedImage(Rect(0, 160, 96, 32));
	backButton->SetActivationKeys(KEY_B|KEY_X|KEY_START);

	exitGameButton = new Button(this);
	exitGameButton->SetButtonListener(this);
	exitGameButton->SetBounds(144, 160, 96, 32);
	exitGameButton->SetForegroundImage(Rect(96, 128, 96, 32));
	exitGameButton->SetPressedImage(Rect(96, 160, 96, 32));

	fontSizeSpinner = new Spinner(this, Rect(32, 0, 16, 16), Rect(32, 16, 16, 16),
			Rect(48, 0, 16, 16), Rect(48, 16, 16, 16), skin->GetLabelFontSize());
	fontSizeSpinner->SetNumeric(6, 24);
	fontSizeSpinner->SetSelected(vnds->textEngine->GetTextPane()->GetFontSize());

	createLabeledWidget(this, "Font Size", fontSizeSpinner, Rect(32, 32, 184, 16),
			skin->GetLabelFontSize());

	textSpeedSpinner = new Spinner(this, Rect(32, 0, 16, 16), Rect(32, 16, 16, 16),
			Rect(48, 0, 16, 16), Rect(48, 16, 16, 16), skin->GetLabelFontSize());
	textSpeedSpinner->AddChoice("max", -1);
	textSpeedSpinner->AddChoice("slow", 1);
	textSpeedSpinner->AddChoice("med",  2);
	textSpeedSpinner->AddChoice("fast", 3);

	switch (vnds->textEngine->GetTextPane()->GetTextSpeed()) {
	case 1:
	case 2:
	case 3:
		textSpeedSpinner->SetSelected(vnds->textEngine->GetTextPane()->GetTextSpeed());
		break;
	default:
		textSpeedSpinner->SetSelected(0);
	}

	createLabeledWidget(this, "Text Speed", textSpeedSpinner, Rect(32, 56, 184, 16),
			skin->GetLabelFontSize());

	soundVolumeSpinner = new Spinner(this, Rect(32, 0, 16, 16), Rect(32, 16, 16, 16),
			Rect(48, 0, 16, 16), Rect(48, 16, 16, 16), skin->GetLabelFontSize());
	soundVolumeSpinner->SetNumeric(0, 16);
	soundVolumeSpinner->SetSelected((preferences->GetSoundVolume()+7) / 8);
	soundVolumeSpinner->SetSpinnerListener(this);

	createLabeledWidget(this, "Sound Vol.", soundVolumeSpinner, Rect(32, 80, 184, 16),
			skin->GetLabelFontSize());

	musicVolumeSpinner = new Spinner(this, Rect(32, 0, 16, 16), Rect(32, 16, 16, 16),
			Rect(48, 0, 16, 16), Rect(48, 16, 16, 16), skin->GetLabelFontSize());
	musicVolumeSpinner->SetNumeric(0, 16);
	musicVolumeSpinner->SetSelected((preferences->GetMusicVolume()+7) / 8);
	musicVolumeSpinner->SetSpinnerListener(this);

	createLabeledWidget(this, "Music Vol.", musicVolumeSpinner, Rect(32, 104, 184, 16),
			skin->GetLabelFontSize());
}
VNMenu::~VNMenu() {
}

void VNMenu::Activate() {
	Screen::Activate();

	skinLoadImage("../../%s/menu_bg", backgroundI, 256, 192);
}

void VNMenu::OnButtonPressed(Button* button) {
	if (button == backButton) {
		int ts = textSpeedSpinner->GetNumericValue();
		if (ts <= 0) ts = -1;

		preferences->SetFontSize(fontSizeSpinner->GetNumericValue(), vnds);
		preferences->SetTextSpeed(ts, vnds);
		if (!preferences->Save("../../config.ini")) {
			vnLog(EL_error, COM_CORE, "Error saving preferences");
		}

		gui->PopScreen(GT_fadeMain);
	} else if (button == exitGameButton) {
		gui->PushScreen(new ConfirmScreen(gui, "Return to main menu?",
			Rect(192, 128, 64, 32), Rect(192, 160, 64, 32), this,
			Rect(192, 192, 64, 32), Rect(192, 224, 64, 32), NULL),
			GT_none);
	} else {
		//OK button of confirmscreen has been pressed

		if (!preferences->Save("../../config.ini")) {
			vnLog(EL_error, COM_CORE, "Error saving preferences");
		}

		vnds->Quit();
	}
}

void VNMenu::OnSpinnerChanged(Spinner* spinner) {
	if (spinner == soundVolumeSpinner) {
		preferences->SetSoundVolume(MIN(127, 8*soundVolumeSpinner->GetNumericValue()), vnds);
	} else if (spinner == musicVolumeSpinner) {
		preferences->SetMusicVolume(MIN(127, 8*musicVolumeSpinner->GetNumericValue()), vnds);
	}
}
