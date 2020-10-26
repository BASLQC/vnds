#ifndef VNMENU_H
#define VNMENU_H

#include "../common.h"
#include "../tcommon/gui/screen.h"
#include "../tcommon/gui/button.h"

class VNMenu : public Screen, public ButtonListener {
	private:
		VNDS* vnds;
		Button* backButton;
		Button* exitGameButton;
		Spinner* fontSizeSpinner;
		Spinner* textSpeedSpinner;
		Spinner* soundVolumeSpinner;
		Spinner* musicVolumeSpinner;

		u16 backgroundI[256*192];

	public:
		VNMenu(GUI* gui, u16* textureI, VNDS* vnds);
		virtual ~VNMenu();

		virtual void OnButtonPressed(Button* button);
		virtual void Activate();

};

#endif
