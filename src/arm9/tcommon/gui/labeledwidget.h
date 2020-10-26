#ifndef T_LABELEDWIDGET_H
#define T_LABELEDWIDGET_H

#include "gui_common.h"

class LabeledWidget : public Widget {
	private:
		Label* label;
		Widget* widget;

	public:
		LabeledWidget(Screen* screen, const char* label, Widget* widget, u8 fontsize=DEFAULT_FONT_SIZE);
		virtual ~LabeledWidget();

		virtual void SetBounds(s16 x, s16 y, u16 w, u16 h);
		virtual void SetBounds(Rect r);

};

LabeledWidget* createLabeledWidget(Screen* screen, const char* label, Widget* widget,
		Rect bounds, u8 fontsize=DEFAULT_FONT_SIZE);

#endif
