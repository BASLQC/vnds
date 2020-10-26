#ifndef T_LABEL_H
#define T_LABEL_H

#include "../common.h"
#include "../fontcache.h"
#include "gui_types.h"
#include "widget.h"

class Label : public Widget {
	private:
		char* label;
		Text* text;
		u8 fontsize;

	public:
		Label(Screen* screen, const char* label, u8 fontsize=DEFAULT_FONT_SIZE);
		virtual ~Label();

		virtual void DrawBackground();

};

#endif
