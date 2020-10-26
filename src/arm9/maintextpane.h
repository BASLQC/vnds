#ifndef MAINTEXTPANE_H
#define MAINTEXTPANE_H

#include "common.h"
#include "vntextscrollpane.h"

class MainTextPane : public VNTextScrollPane {
	private:
		static const int textBufferL = 8192;

		int currentChunkStart;
		u16 activeTextColor;
		s16 activeVisibleChars;
		s16 textSpeed;

	public:
		MainTextPane(Screen* screen, FontCache* fontCache);
		virtual ~MainTextPane();

		virtual bool AppendText(const char* text, u16 color=0, bool stripSpaces=true);
		virtual void DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected);
		virtual void Update(u32& down, u32& held, touchPosition touch);
		void DisplayFullText();

		bool IsFullTextDisplayed();
		u16 GetActiveTextColor();
		s16 GetTextSpeed();

		void SetActiveTextColor(u16 c);
		void SetTextSpeed(s16 ts);

};

#endif
