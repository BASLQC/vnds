#ifndef T_TEXTFIELD_H
#define T_TEXTFIELD_H

#include "gui_types.h"
#include "widget.h"
#include "../fontcache.h"

class TextField : public Widget {
	private:
		char* string;
		int limit;
		u8 fontsize;
		Text* text;

	public:
		TextField(Screen* screen, u8 fontsize=DEFAULT_FONT_SIZE, const char* str=NULL);
		virtual ~TextField();

		virtual void DrawBackground();
		virtual bool OnTouch(u32& keysDown, u32& keysHeld, touchPosition touch);

		const char* GetText();

		virtual void SetText(const char* text);
		virtual void SetLimit(int l);
};

#endif
