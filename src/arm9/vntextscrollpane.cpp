#include "vntextscrollpane.h"
#include "tcommon/text.h"
#include "tcommon/gui/gui_common.h"

VNTextScrollPane::VNTextScrollPane(Screen* screen, FontCache* fontCache)
: TextScrollPane(screen, Rect(0,0,0,16), Rect(0,0,0,16), Rect(0,0,0,16), fontCache)
{

}

VNTextScrollPane::~VNTextScrollPane() {

}

void VNTextScrollPane::DrawScrollBar() {
	if (scroll >= totalItemL-visibleItemL) {
		return; //Don't show scrollbar if we're scrolled all the way down
	}

	const int vw = VERTEX_SCALE(8);
	const int vh = VERTEX_SCALE(16);

	Rect bounds = GetBounds();
	Rect uv;

	int s = (bounds.h+15) / 16;
	glTranslate3f32(0, 0, -100);

	uv = Rect(240, 32, 8, 16);
	for (int n = 0; n < s; n++) {
		drawQuad(texture, inttof32(bounds.x), inttof32(bounds.y+(n<<4)), vw, vh, uv);
	}
	uv = Rect(248, 32, 8, 16);
	for (int n = 0; n < s; n++) {
		drawQuad(texture, inttof32(bounds.x+bounds.w-uv.w-1), inttof32(bounds.y+(n<<4)), vw, vh, uv);
	}

	glTranslate3f32(0, 0, 100);
}
void VNTextScrollPane::DrawScrollBarThumb(int dy) {
	if (scroll >= totalItemL-visibleItemL) {
		return; //Don't show scrollbar if we're scrolled all the way down
	}

	const int vw = VERTEX_SCALE(8);
	const int vh = VERTEX_SCALE(16);

	Rect bounds = GetBounds();

	if (totalItemL > visibleItemL) {
		dy = scroll * (bounds.h-scrollBarHeight) / (totalItemL-visibleItemL);
	} else {
		dy = scroll * (bounds.h-scrollBarHeight);
	}
	dy += bounds.y;

	drawQuad(texture, inttof32(bounds.x), inttof32(dy),
			vw, vh, Rect(240, 48, 8, 16));
	drawQuad(texture, inttof32(bounds.x+bounds.w-1-8),
			inttof32(dy), vw, vh, Rect(248, 48, 8, 16));
}
