#include "button.h"
#include "gui_common.h"

//------------------------------------------------------------------------------

Button::Button(Screen* s) : Widget(s) {
	listener = NULL;
	activationKeys = 0;
	pressed = false;
	dragover = false;

	SetTransparent(true);
}
Button::~Button() {
}

//Functions
bool Button::OnTouch(u32& keysDown, u32& keysHeld, touchPosition touch) {
	if (keysDown & KEY_TOUCH) {
		pressed = true;
	}
	dragover = keysHeld & KEY_TOUCH;
	return true;
}
void Button::Update(u32& keysDown, u32& keysHeld, touchPosition touch) {
	if (pressed && !(keysHeld & KEY_TOUCH) && !(keysHeld & activationKeys)) {
		if (dragover) {
			OnPressed(keysDown, keysHeld, touch);
			ResetPressedState();
			return;
		}
		pressed = false;
	}

	dragover = false;

	Widget::Update(keysDown, keysHeld, touch);
	if (keysDown & activationKeys) {
		pressed = true;
	}
	if (keysHeld & activationKeys) {
		dragover = true;
	}
}
void Button::ResetPressedState() {
	pressed = dragover = false;
}
void Button::OnPressed(u32& keysDown, u32& keysHeld, touchPosition touch) {
	if (listener) {
		listener->OnButtonPressed(this);
	}
}
void Button::DrawForeground() {
	Rect bounds = GetBounds();
	if (IsPressed() && IsDragOver()) {
		if (pressedUV.w > 0 && pressedUV.h > 0) {
			drawQuad(texture, inttof32(bounds.x), inttof32(bounds.y), vw, vh, pressedUV);
		}
	} else {
		if (textureUV.w > 0 && textureUV.h > 0) {
			drawQuad(texture, inttof32(bounds.x), inttof32(bounds.y), vw, vh, textureUV);
		}
	}
}

//Getters
bool Button::IsPressed() {
	return pressed;
}
bool Button::IsDragOver() {
	return dragover;
}

//Setters
void Button::SetForegroundImage(Rect r) {
	Widget::SetForegroundImage(r);

	if (pressedUV.w == 0 && pressedUV.h == 0) {
		SetPressedImage(Rect(r.x+r.w, r.y, r.w, r.h));
	}
}
void Button::SetPressedImage(Rect r) {
	pressedUV = r;
}
void Button::SetActivationKeys(u32 k) {
	activationKeys = k;
}
void Button::SetButtonListener(ButtonListener* bl) {
	listener = bl;
}
