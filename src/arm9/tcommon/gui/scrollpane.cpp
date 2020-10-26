#include "scrollpane.h"

#include <algorithm>
#include "gui_common.h"

using namespace std;

ScrollPane::ScrollPane(Screen* s, Rect upImg, Rect downImg, Rect barImg) : Widget(s) {
	this->scrollUpImage = upImg;
	this->scrollDownImage = downImg;
	this->scrollBarImage = barImg;

	scrollBarWidth = MAX(upImg.w, downImg.w);
	scrollBarHeight = MAX(upImg.h, downImg.h);

	scrollUpButton = new Button(s);
	scrollUpButton->SetButtonListener(this);
	scrollUpButton->SetBounds(256, 192, scrollBarWidth, scrollBarHeight);
	scrollUpButton->SetForegroundImage(upImg);

	scrollDownButton = new Button(s);
	scrollDownButton->SetButtonListener(this);
	scrollDownButton->SetBounds(256, 192, scrollBarWidth, scrollBarHeight);
	scrollDownButton->SetForegroundImage(downImg);

	listItemI = NULL;
	totalItemL = 0;
	visibleItemL = 0;
	listItemW = GetWidth() - scrollBarWidth;
	listItemH = 1;

	itemBg = selectedItemBg = NULL;
	itemBgW = itemBgH = selectedItemBgW = selectedItemBgH = 0;

	scrollSpeed = 0;
	scrollPixels = 0;
	scroll = 0;
	selected = 0;
	showSelections = true;
	backgroundColor = BIT(15);
}
ScrollPane::~ScrollPane() {
	screen->RemoveWidget(scrollUpButton);
	screen->RemoveWidget(scrollDownButton);

	delete scrollUpButton;
	delete scrollDownButton;

	DeleteItemImages();
}

//Functions
void ScrollPane::DrawBackground() {
	vector<s16>::iterator i;
	for (i = dirtyItemsBg.begin(); i != dirtyItemsBg.end(); i++) {
		int index = *i;
		if (index >= scroll && index < scroll+visibleItemL) {
			DrawListItemBackground(index, listItemI[index-scroll], listItemW, listItemH, selected==index);
		}
	}

	Rect r = GetBounds();
	if (listItemI) {
		for (i = dirtyItemsBg.begin(); i != dirtyItemsBg.end(); i++) {
			int index = *i;
			if (index >= scroll && index < scroll+visibleItemL) {
				blit(listItemI[index-scroll], listItemW, listItemH, background, r.w, r.h,
						0, 0, 0, (index-scroll)*listItemH, listItemW, listItemH);
			}
		}
	}
	dirtyItemsBg.clear();

	if (background && (totalItemL*listItemH < r.h || r.h % listItemH != 0)) {
		int s = r.w * r.h;
		for (int n = MIN(totalItemL*r.w*listItemH, s - r.w*(r.h % listItemH)); n < s; n++) {
			background[n] = backgroundColor|BIT(15);
		}
	}
}
void ScrollPane::DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected) {
	u16* img = (selected ? selectedItemBg : itemBg);
	u16 iw = (selected ? selectedItemBgW : itemBgW);
	u16 ih = (selected ? selectedItemBgH : itemBgH);

	if (img) {
		blit(img, iw, ih, buffer, w, h, 0, 0, 0, 0, w, h);
	} else {
		int c = (selected ? RGB15(20, 15, 31) : backgroundColor);
		int s = w * h;
		for (int n = 0; n < s; n++) {
			buffer[n] = c|BIT(15);
		}
	}
}
void ScrollPane::DrawForeground() {
	Widget::DrawForeground();

	Rect bounds = GetBounds();

	Rect itemBounds(bounds.x, bounds.y, listItemW, listItemH);
	for (int n = scroll; n < MIN(totalItemL, scroll+visibleItemL); n++) {
		DrawListItemForeground(n, itemBounds, n==selected);
		itemBounds.y += listItemH;
	}

	DrawScrollBar();

	int sel = MAX(0, MIN(totalItemL, selected));
	int dy = 0;
	if (showSelections) {
		dy = sel * (bounds.h-scrollBarHeight*3) / (totalItemL-1);
	} else {
		if (totalItemL > visibleItemL) {
			dy = scroll * (bounds.h-scrollBarHeight*3) / (totalItemL-visibleItemL);
		} else {
			dy = scroll * (bounds.h-scrollBarHeight*3);
		}
	}
	DrawScrollBarThumb(bounds.y + MIN(dy+scrollBarHeight, bounds.h-scrollBarHeight*2));
}
void ScrollPane::DrawScrollBar() {
	Rect bounds = GetBounds();

	int vw = VERTEX_SCALE(scrollBarWidth);
	int vh = VERTEX_SCALE(scrollBarHeight);
	int s = (bounds.h+scrollBarHeight) / scrollBarHeight;

	glTranslate3f32(0, 0, -100);
	for (int n = 0; n < s; n++) {
		drawQuad(texture, inttof32(bounds.x+bounds.w-scrollBarWidth), inttof32(bounds.y+(n<<4)-1),
				vw, vh, scrollBarImage);
	}
	glTranslate3f32(0, 0, 100);
}
void ScrollPane::DrawScrollBarThumb(int dy) {
	Rect bounds = GetBounds();

	int vw = VERTEX_SCALE(scrollBarWidth);
	int vh = VERTEX_SCALE(scrollBarHeight);
	drawQuad(texture, inttof32(bounds.x+bounds.w-scrollBarWidth), inttof32(dy),
			vw, vh,
			Rect(scrollBarImage.x+scrollBarImage.w, scrollBarImage.y, scrollBarImage.w, scrollBarImage.h));
}
void ScrollPane::DrawListItemForeground(s16 index, Rect r, bool selected) {
}

void ScrollPane::OnButtonPressed(Button* button) {
	if (button == scrollUpButton) {
		if (selected > 0) {
			SetSelectedIndex(selected-1);
			if (selected < scroll) {
				SetScroll(selected);
			}
		}
	} else if (button == scrollDownButton) {
		if (selected < totalItemL-1) {
			SetSelectedIndex(selected+1);
			if (selected >= scroll+visibleItemL) {
				SetScroll(selected-visibleItemL+1);
			}
		}
	}
}
bool ScrollPane::OnTouch(u32& keysDown, u32& keysHeld, touchPosition touch) {
	if (keysDown & KEY_TOUCH) {
		scrollPixels = 0;
	}

	Rect r = GetBounds();
    if (touch.px-r.x < listItemW) {
    	if (showSelections) {
    		SetSelectedIndex(((touch.py-r.y) / listItemH) + scroll);
    	} else {
    		if (!(keysDown&KEY_TOUCH) && (keysHeld&KEY_TOUCH)) {
    			int inc = oldTouchPosition.py - touch.py;

				// when scrolling up, old.py ends up being large and messing up
    			// the scrolling, this fixes
				if (abs(inc) < 50) {
					scrollPixels += MAX(-listItemH, MIN(listItemH, inc));

					if (inc < 0) scrollSpeed = MIN(scrollSpeed, inc);
					if (inc > 0) scrollSpeed = MAX(scrollSpeed, inc);

		    		return inc != 0;
				}
    		}
    	}
    } else {
    	if (touch.py-r.y >= scrollBarHeight && touch.py-r.y < r.h-scrollBarHeight) {
    		if (showSelections) {
				SetSelectedIndex((touch.py-r.y-scrollBarHeight) * totalItemL / (r.h-scrollBarHeight*2));

				if (selected < scroll) {
					SetScroll(selected);
				} else if (selected >= scroll+visibleItemL) {
					SetScroll(selected-visibleItemL+1);
				}
    		} else {
				if (totalItemL > visibleItemL) {
					SetScroll((touch.py-r.y-scrollBarHeight) * (totalItemL-visibleItemL+1) / (r.h-scrollBarHeight*2));
				}
    		}
    		return true;
    	}
    }

	return false;
}
void ScrollPane::Update(u32& down, u32& held, touchPosition touch) {
	Widget::Update(down, held, touch);

	u32 repeat = keysDownRepeat();
	if ((down|repeat) & KEY_UP) {
		OnButtonPressed(scrollUpButton);
	} else if ((down|repeat) & KEY_DOWN) {
		OnButtonPressed(scrollDownButton);
	} else if ((down|repeat) & KEY_LEFT) {
		SetScroll(MAX(0, scroll - visibleItemL));
		SetSelectedIndex(MAX(0, selected - visibleItemL));
	} else if ((down|repeat) & KEY_RIGHT) {
		SetScroll(MIN(totalItemL-visibleItemL, scroll + visibleItemL));
		SetSelectedIndex(MIN(totalItemL-1, selected + visibleItemL));
	}

    if (abs(scrollPixels) >= listItemH) {
		int s = scrollPixels / listItemH;
		//scrollPixels -= listItemH * s;
		scrollPixels = 0;
		SetScroll(scroll + s);
    }

    scrollSpeed = (7*scrollSpeed) >> 3;
    if (scrollSpeed != 0) {
    	scrollPixels += scrollSpeed;
    }
    if (scrollSpeed < 0) {
    	scrollSpeed++;
    	if (scroll <= 0) scrollSpeed = 0;
    }
    if (scrollSpeed > 0) {
    	scrollSpeed--;
    	if (scroll >= totalItemL-visibleItemL) scrollSpeed = 0;
    }

    oldTouchPosition = touch;
}

void ScrollPane::DeleteItemImages() {
	if (listItemI) {
		for (int n = 0; n < visibleItemL; n++) {
			delete[] listItemI[n];
		}
		delete[] listItemI;
	}
	listItemI = NULL;
}
void ScrollPane::CreateItemImages() {
	DeleteItemImages();

	Rect r = GetBounds();
	listItemW = r.w - scrollBarWidth;
	//visibleItemL = (r.h+listItemH-1) / listItemH;
	visibleItemL = r.h / listItemH;
	listItemI = new u16*[visibleItemL];
	for (int n = 0; n < visibleItemL; n++) {
		listItemI[n] = new u16[r.w*listItemH];
	}

	for (int n = scroll; n < scroll+visibleItemL; n++) {
		SetItemDirty(n);
	}
}

//Getters
bool ScrollPane::IsBackgroundDirty() {
	return Widget::IsBackgroundDirty() || dirtyItemsBg.size() > 0;
}
int ScrollPane::GetSelectedIndex() {
	return selected;
}
int ScrollPane::GetNumberOfItems() {
	return totalItemL;
}
u16 ScrollPane::GetBackgroundColor() {
	return backgroundColor;
}
int ScrollPane::GetScroll() {
	return scroll;
}
int ScrollPane::GetVisibleItems() {
	return visibleItemL;
}

//Setters
void ScrollPane::SetBackgroundColor(u16 c) {
	backgroundColor = c;

	SetBackgroundDirty();
}
void ScrollPane::SetBounds(s16 x, s16 y, u16 w, u16 h) {
	SetBounds(Rect(x, y, w, h));
}
void ScrollPane::SetBounds(Rect r) {
	Widget::SetBounds(r);

	if (scrollUpButton && scrollDownButton) {
		scrollUpButton->SetPos(r.x+r.w-scrollUpButton->GetWidth(), r.y);
		scrollDownButton->SetPos(r.x+r.w-scrollDownButton->GetWidth(),
				r.y+r.h-scrollDownButton->GetHeight());

		DeleteItemImages();
		CreateItemImages();
	}
}
void ScrollPane::SetShowSelections(bool s) {
	showSelections = s;
}
void ScrollPane::SetItemHeight(u16 ih) {
	if (ih > 0) {
		DeleteItemImages();
		listItemH = ih;
		CreateItemImages();
	}
}
void ScrollPane::SetItemDirty(s16 index) {
	if (index >= 0) {
		if (find(dirtyItemsBg.begin(), dirtyItemsBg.end(), index) == dirtyItemsBg.end()) {
			dirtyItemsBg.push_back(index);
		}
	}
}
void ScrollPane::SetNumberOfItems(s16 numberOfItems) {
	totalItemL = numberOfItems;

	for (int n = scroll; n < scroll+visibleItemL; n++) {
		SetItemDirty(n);
	}
}

void ScrollPane::SetItemBg(u16* bg, u16 bgw, u16 bgh, u16* selectedBg, u16 sbgw, u16 sbgh) {
	itemBg = bg;
	itemBgW = bgw;
	itemBgH = bgh;
	selectedItemBg = selectedBg;
	selectedItemBgW = sbgw;
	selectedItemBgH = sbgh;

	for (int n = scroll; n < scroll+visibleItemL; n++) {
		SetItemDirty(n);
	}
}

void ScrollPane::SetSelectedIndex(int s) {
	s = MAX(0, MIN(totalItemL-1, s));

	if (selected != s) {
		SetItemDirty(selected);
		SetItemDirty(s);

		selected = s;
	}
}

void ScrollPane::SetScroll(int s) {
	s = MAX(0, MIN(totalItemL-visibleItemL, s));
	int oldScroll = scroll;

	scroll = s;

	if (scroll != oldScroll) {
		Rect r = GetBounds();
		int delta = scroll-oldScroll;

		if (delta > 0 && delta < visibleItemL) {
			//Scroll item graphics
			if (listItemI) {
				for (int n = 0; n < delta; n++) {
					u16* temp = listItemI[0];
					for (u16 n = 1; n < visibleItemL; n++) listItemI[n-1] = listItemI[n];
					listItemI[visibleItemL-1] = temp;
				}
			}

			//Scroll background
			if (background && IsVisible()) {
				memmove(background, background+(r.w*delta*listItemH), r.w*(r.h-delta*listItemH)*2);
			}

			//Set new items to dirty
			for (int n = oldScroll+visibleItemL; n < scroll+visibleItemL; n++) {
				SetItemDirty(n);
			}
		} else if (delta < 0 && delta > -visibleItemL) {
			//Scroll item graphics
			if (listItemI) {
				for (int n = 0; n < -delta; n++) {
					u16* temp = listItemI[visibleItemL-1];
					for (u16 n = visibleItemL-1; n > 0; n--) listItemI[n] = listItemI[n-1];
					listItemI[0] = temp;
				}
			}

			//Scroll background
			if (background && IsVisible()) {
				memmove(background+(r.w*-delta*listItemH), background, r.w*(r.h-(-delta*listItemH))*2);
			}

			//Set new items to dirty
			for (int n = scroll; n < oldScroll; n++) {
				SetItemDirty(n);
			}
		} else {
			for (int n = scroll; n < scroll+visibleItemL; n++) {
				SetItemDirty(n);
			}
		}

		//Repaint the part of the background that's not covered by items
		if (background && (totalItemL*listItemH < r.h || r.h % listItemH != 0)) {
			int s = r.w * r.h;
			for (int n = MIN(totalItemL*r.w*listItemH, s - r.w*(r.h % listItemH)); n < s; n++) {
				background[n] = backgroundColor|BIT(15);
			}
		}
	}
}

void ScrollPane::SetVisible(bool v) {
	if (v != IsVisible()) {
		Widget::SetVisible(v);
		scrollUpButton->SetVisible(v);
		scrollDownButton->SetVisible(v);

		if (!v) {
			DeleteItemImages();
		} else {
			CreateItemImages();
		}
	}
}
