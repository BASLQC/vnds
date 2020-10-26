#ifndef T_CONFIRMSCREEN_H
#define T_CONFIRMSCREEN_H

#include "../common.h"
#include "gui_types.h"
#include "screen.h"
#include "button.h"

class ConfirmScreen : public Screen, public ButtonListener {
	private:
		Button* okButton;
		Button* cancelButton;
		ButtonListener* okListener;
		ButtonListener* cancelListener;

		Text* text;
		char* message;

	public:
		ConfirmScreen(GUI* gui, const char* message,
				Rect ok, Rect okp, ButtonListener* okl,
				Rect cancel, Rect cancelp, ButtonListener* cancell);

		virtual ~ConfirmScreen();

		virtual void OnButtonPressed(Button* button);
		virtual void DrawBackground();

};

#endif
