#ifndef T_SCROLLPANE_H
#define T_SCROLLPANE_H

#include <vector>
#include "../common.h"
#include "gui_types.h"
#include "widget.h"
#include "button.h"

class ScrollPane : public Widget, public ButtonListener {
	private:
		Rect scrollUpImage;
		Rect scrollDownImage;
		Rect scrollBarImage;

		touchPosition oldTouchPosition;
        u16** listItemI;
        std::vector<s16> dirtyItemsBg;
        s16 selected;
		u16 backgroundColor;
		s16 scrollPixels;
		s16 scrollSpeed;

        void DeleteItemImages();
        void CreateItemImages();

	protected:
		Button* scrollUpButton;
		Button* scrollDownButton;

		s16 scrollBarWidth;
		s16 scrollBarHeight;

        u16 listItemW;
        u16 listItemH;

        u16 totalItemL;
        u16 visibleItemL;

        s16 scroll;
		bool showSelections;

        u16* itemBg;
        u16  itemBgW, itemBgH;
        u16* selectedItemBg;
        u16  selectedItemBgW, selectedItemBgH;

	public:
		ScrollPane(Screen* screen, Rect upImg, Rect downImg, Rect barImg);
		virtual ~ScrollPane();

		virtual void DrawBackground();
		virtual void DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected);
		virtual void DrawForeground();
		virtual void DrawListItemForeground(s16 index, Rect bounds, bool selected);
		virtual void DrawScrollBar();
		virtual void DrawScrollBarThumb(int dy);
		virtual void OnButtonPressed(Button* button);
		virtual bool OnTouch(u32& down, u32& held, touchPosition touch);
		virtual void Update(u32& down, u32& held, touchPosition touch);

		virtual bool IsBackgroundDirty();
		virtual u16  GetBackgroundColor();
		virtual int  GetSelectedIndex();
				int  GetVisibleItems();
		        int  GetNumberOfItems();
		        int  GetScroll();

		virtual void SetBounds(s16 x, s16 y, u16 w, u16 h);
		virtual void SetBounds(Rect r);
		virtual void SetBackgroundColor(u16 c);
		virtual void SetItemHeight(u16 ih);
		virtual void SetItemDirty(s16 index);
		virtual void SetNumberOfItems(s16 numberOfItems);
		virtual void SetItemBg(u16* bg, u16 bgw, u16 bgh, u16* selectedBg, u16 sbgw, u16 sbgh);
		virtual void SetSelectedIndex(int s);
		virtual void SetShowSelections(bool s);
		virtual void SetScroll(int s);
		virtual void SetVisible(bool v);

};

#endif

