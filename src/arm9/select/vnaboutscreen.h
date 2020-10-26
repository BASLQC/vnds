#ifndef VNABOUTSCREEN_H
#define VNABOUTSCREEN_H

#include "../common.h"
#include "../tcommon/gui/screen.h"
#include "../tcommon/gui/button.h"

class VNAboutScreen : public Screen, public ButtonListener {
	private:
		Button* backButton;
		Text* text;
		u16 bg[256*192];
		bool imageLoaded;

	public:
		VNAboutScreen(GUI* gui, u16* textureI);
		virtual ~VNAboutScreen();

		virtual void Activate();
		virtual void DrawBackground();
		virtual void OnButtonPressed(Button* button);
};

#endif
