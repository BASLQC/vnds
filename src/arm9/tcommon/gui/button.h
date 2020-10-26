#ifndef T_BUTTON_H
#define T_BUTTON_H

#include "../common.h"
#include "gui_types.h"
#include "widget.h"

class ButtonListener {
	public:
		virtual void OnButtonPressed(Button* button) = 0;
};

class Button : public Widget {
	private:
		ButtonListener* listener;
		u32 activationKeys;

	protected:
		bool dragover;
		bool pressed;

	public:
		Rect pressedUV;

		Button(Screen* screen);
		virtual ~Button();

		virtual bool OnTouch(u32& down, u32& held, touchPosition touch);
		virtual void OnPressed(u32& down, u32& held, touchPosition touch);
		virtual void Update(u32& down, u32& hHeld, touchPosition touch);
		virtual void DrawForeground();
		void ResetPressedState();

		bool IsPressed();
		bool IsDragOver();

		virtual void SetForegroundImage(Rect r);
		virtual void SetPressedImage(Rect r);
		void SetActivationKeys(u32 k);
		void SetButtonListener(ButtonListener* bl);
};

#endif

