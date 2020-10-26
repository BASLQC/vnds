#include "labeledwidget.h"
#include "gui_common.h"

LabeledWidget* createLabeledWidget(Screen* screen, const char* label, Widget* widget,
		Rect bounds, u8 fontsize)
{
	LabeledWidget* lw = new LabeledWidget(screen, label, widget, fontsize);
	lw->SetBounds(bounds);
	return lw;
}

LabeledWidget::LabeledWidget(Screen* screen, const char* lbl, Widget* w, u8 fontsize)
: Widget(screen)
{
	widget = w;
	label = new Label(screen, lbl, fontsize);

	SetTransparent(true);
	SetVisible(false);
}
LabeledWidget::~LabeledWidget() {
	screen->RemoveWidget(label);
	delete label;
}

void LabeledWidget::SetBounds(s16 x, s16 y, u16 w, u16 h) {
	SetBounds(Rect(x, y, w, h));
}
void LabeledWidget::SetBounds(Rect r) {
	Widget::SetBounds(r);

	if (label && widget) {
		label->SetBounds(r.x, r.y, r.w/2, r.h);
		widget->SetBounds(r.x+r.w/2, r.y, r.w-r.w/2, r.h);
	}
}
