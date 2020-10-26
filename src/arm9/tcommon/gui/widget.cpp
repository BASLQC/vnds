#include "widget.h"
#include "gui_common.h"

Widget::Widget(Screen* s) : screen(s) {
	bounds = Rect(0, 0, 0, 0);
	visible = true;
	backgroundDirty = true;
	transparent = false;

	background = NULL;
	image = NULL;

	screen->AddWidget(this);
}
Widget::~Widget() {
	if (background) {
		delete[] background;
		background = NULL;
	}
}

//Functions
void Widget::Activate() {
	texture = screen->texture;

	OnActivated();
}
void Widget::OnActivated() {
}
void Widget::FlushChanges() {
	OnFlushChanges();
}
void Widget::OnFlushChanges() {
}
void Widget::Cancel() {
	OnCancel();
}
void Widget::OnCancel() {
}

bool Widget::OnTouch(u32& down, u32& held, touchPosition touch) {
	return true;
}
void Widget::Update(u32& down, u32& held, touchPosition touch) {
	if (held & KEY_TOUCH) {
	    //If touchXY=(0,0) then an error has probably occurred.
		if (IsVisible() && (touch.px|touch.py) && Contains(touch.px, touch.py)) {
			if (OnTouch(down, held, touch)) {
				down &= ~KEY_TOUCH;
			}
		}
	}
}
void Widget::DrawBackground() {
	if (image && background) {
		blit(image, imageW, imageH, background, bounds.w, bounds.h,
				imageU, imageV, 0, 0, bounds.w, bounds.h);
	}
}
void Widget::DrawForeground() {
	if (textureUV.w > 0 && textureUV.h > 0) {
		drawQuad(texture, inttof32(bounds.x), inttof32(bounds.y), vw, vh, textureUV);
	}
}

//Getters
bool Widget::IsVisible() {
	return visible;
}
bool Widget::IsTransparent() {
	return transparent;
}
bool Widget::Contains(int px, int py) {
	return px >= bounds.x && px < bounds.x+bounds.w && py >= bounds.y && py < bounds.y+bounds.h;
}
s16 Widget::GetX() {
	return bounds.x;
}
s16 Widget::GetY() {
	return bounds.y;
}
u16 Widget::GetWidth() {
	return bounds.w;
}
u16 Widget::GetHeight() {
	return bounds.h;
}
Rect Widget::GetBounds() {
	return bounds;
}
bool Widget::IsBackgroundDirty() {
	return backgroundDirty;
}

//Setters
void Widget::SetPos(s16 nx, s16 ny) {
	SetBounds(nx, ny, bounds.w, bounds.h);
}
void Widget::SetSize(u16 nw, u16 nh) {
	SetBounds(bounds.x, bounds.y, nw, nh);
}
void Widget::SetBounds(s16 nx, s16 ny, u16 nw, u16 nh) {
	SetBounds(Rect(nx, ny, nw, nh));
}
void Widget::SetBounds(Rect r) {
	bool sizeChanged = bounds.w != r.w || bounds.h != r.h;

	bounds.x = r.x;
	bounds.y = r.y;
	bounds.w = r.w;
	bounds.h = r.h;

	if (sizeChanged) {
		if (background) {
			delete[] background;
			background = NULL;
		}
		if (visible) {
			if (bounds.w > 0 && bounds.h > 0) {
				background = new u16[bounds.w * bounds.h];
			}
		}

		vw = VERTEX_SCALE(bounds.w);
		vh = VERTEX_SCALE(bounds.h);

		backgroundDirty = true;
	}
}
void Widget::SetVisible(bool v) {
	if (visible != v) {
		visible = v;

		if (!visible) {
			if (background) {
				delete[] background;
				background = NULL;
			}
		} else {
			if (!background && bounds.w > 0 && bounds.h > 0) {
				background = new u16[bounds.w * bounds.h];
			}
		}
	}
}
void Widget::SetTransparent(bool t) {
	if (transparent != t) {
		transparent = t;
		if (!transparent) {
			SetBackgroundDirty();
		}
	}
}
void Widget::SetBackgroundDirty(bool v) {
	backgroundDirty = v;
}
void Widget::SetForegroundImage(Rect uv) {
	this->texture = screen->texture;
	this->textureUV = uv;
}
void Widget::SetBackgroundImage(u16* img, u16 iw, u16 ih, s16 iu, s16 iv) {
	image = img;
	imageW = iw;
	imageH = ih;
	imageU = iu;
	imageV = iv;

	backgroundDirty = true;
}
