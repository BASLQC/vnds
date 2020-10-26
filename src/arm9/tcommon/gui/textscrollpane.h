#ifndef T_TEXTSCROLLPANE_H
#define T_TEXTSCROLLPANE_H

#include "../fontcache.h"
#include "scrollpane.h"

class TextScrollPane : public ScrollPane {
	private:
		static const int maxLineStartsL = 1024;

		bool textWrapping;
		u16 fontSize;
		u16 textColor;

	protected:
		Text* text;
		u16 colors[maxLineStartsL];
		int lineStarts[maxLineStartsL];
		u16 lineStartsL;
		char* textBuffer;
		int textBufferL;

		friend void printCallback(const char* text);

	public:
		TextScrollPane(Screen* screen, Rect upImg, Rect downImg, Rect barImg, FontCache* fontCache=NULL);
		virtual ~TextScrollPane();

		void RemoveAllItems();
		void RemoveItems(int number);
		virtual bool AppendText(const char* str, u16 color=0, bool doLayout=true);
		virtual void DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected);
		virtual void DrawListItemForeground(s16 index, Rect bounds, bool selected);
		virtual void OnButtonPressed(Button* button);

		virtual int GetFontSize();
		virtual u16 GetTextColor();

		void SetTextBufferSize(int size);
		void SetTextWrapping(bool w); //Changing this will only effect future calls to AppendText()
		virtual void SetFont(const char* filename);
		virtual void SetFontSize(u16 s, bool sizeToFit);
		virtual void SetTextColor(u16 c);
		virtual void SetDefaultMargins(u16 left, u16 right, u16 top, u16 bottom);
		virtual void SetItemHeight(u16 ih);
		virtual void SetBounds(s16 x, s16 y, u16 w, u16 h);
		virtual void SetBounds(Rect r);
		virtual void SetVisible(bool v);

};

#endif
