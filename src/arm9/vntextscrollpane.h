#ifndef VNTEXTSCROLLPANE_H
#define VNTEXTSCROLLPANE_H

#include "common.h"
#include "tcommon/gui/textscrollpane.h"

class VNTextScrollPane : public TextScrollPane {
	private:

	public:
		VNTextScrollPane(Screen* screen, FontCache* fontCache);
		virtual ~VNTextScrollPane();

		virtual void DrawScrollBar();
		virtual void DrawScrollBarThumb(int dy);
};

#endif
