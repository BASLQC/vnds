#ifndef T_SPINNER_H
#define T_SPINNER_H

#include <vector>
#include "../common.h"
#include "../fontcache.h"
#include "gui_types.h"
#include "widget.h"
#include "button.h"

struct SpinnerChoice {
	char label[64];
	int value;
};

enum SpinnerMode {
	SM_choice,
	SM_numeric,
	SM_color
};

class SpinnerListener {
	public:
		virtual void OnSpinnerChanged(Spinner* spinner) = 0;
};

class Spinner : public Widget, public ButtonListener {
	private:
		SpinnerListener* listener;
		Button* spinLeftButton;
		Button* spinRightButton;

		int minval; //SM_numeric
		int maxval; //SM_numeric
		std::vector<SpinnerChoice> choices; //SM_choice
		std::vector<u16> colors; //SM_color

	protected:
		Text* text;
		SpinnerMode mode;
		int selected;

	public:
		Spinner(Screen* screen, Rect l, Rect lp, Rect r, Rect rp, u8 fontsize=DEFAULT_FONT_SIZE);
		virtual ~Spinner();

		virtual void RemoveAll();
		virtual void AddChoice(const char* label, int value);
		virtual void AddColor(u16 c);
		virtual void DrawBackground();
		virtual void OnButtonPressed(Button* button);

		SpinnerChoice* GetChoiceValue();
		int GetNumericValue();
		u16 GetColorValue();

		void SetNumeric(int minval, int maxval);
		void SetSelected(int s);
		void SetSelectedChoice(int v);
		void SetSpinnerListener(SpinnerListener* l);
		virtual void SetBounds(s16 x, s16 y, u16 w, u16 h);
		virtual void SetBounds(Rect r);

};

#endif
