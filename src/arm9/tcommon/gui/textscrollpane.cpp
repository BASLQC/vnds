#include "textscrollpane.h"

using namespace std;

#include "gui_common.h"
#include "../text.h"

TextScrollPane::TextScrollPane(Screen* s, Rect upImg, Rect downImg, Rect barImg, FontCache* fc)
: ScrollPane(s, upImg, downImg, barImg)
{
	if (fc) {
		text = new Text(fc);
	} else {
		text = new Text();
	}
	text->SetMargins(8, 8, 0, 0);

	textWrapping = true;
	fontSize = text->GetFontSize();
	textColor = RGB15(31, 31, 31)|BIT(15);
	textBuffer = NULL;
	SetTextBufferSize(256);
}

TextScrollPane::~TextScrollPane() {
	if (textBuffer) delete[] textBuffer;
	if (text) delete text;
}

void TextScrollPane::OnButtonPressed(Button* button) {
	if (button == scrollUpButton) {
		if (!showSelections) {
			SetSelectedIndex(scroll);
		}
	} else if (button == scrollDownButton) {
		if (!showSelections) {
			SetSelectedIndex(scroll + visibleItemL - 1);
		}
	}

	ScrollPane::OnButtonPressed(button);
}

void TextScrollPane::DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected) {
	ScrollPane::DrawListItemBackground(index, buffer, w, h, (showSelections ? selected : false));

	if (index >= 0 && index < lineStartsL) {
		text->ClearBuffer();
		text->SetBuffer(w, h);
		text->PushState();
		text->SetFontSize(fontSize);
		text->SetMargins(text->GetMarginLeft(), text->GetMarginRight(),
				((h-text->GetLineHeight())>>1) - (text->GetLineHeight()>>3), 0);
		text->SetColor(colors[index]);
		text->PrintLine(textBuffer+lineStarts[index]);
		text->BlitToScreen(buffer, w, h, 0, 0, 0, -2, w, h);
		text->PopState();
	}
}
void TextScrollPane::DrawListItemForeground(s16 index, Rect r, bool selected) {
	if (showSelections) {
		ScrollPane::DrawListItemForeground(index, r, (showSelections ? selected : false));
	}
}

bool TextScrollPane::AppendText(const char* str, u16 color, bool stripSpaces) {
	if (color == 0) {
		color = textColor;
	}

	text->GetStringLines(str);

	u16* lineBreaks = text->lineBreaks;
	int lineBreaksL = (textWrapping ? text->lineBreaksL : 1);

	int oldLineStartsL = lineStartsL;

	int lastStrOffset = 0;
	for (int n = 0; n < lineBreaksL; n++) {
		if (lineStartsL >= maxLineStartsL - 1) {
			//No more room in lineStarts array, time to bail
			lineStartsL = oldLineStartsL;
			return false;
		}

		int offset = lineStarts[lineStartsL];
		int strL = lineBreaks[n] - lastStrOffset;
		int copyL = MIN(strL, (textBufferL-offset) - strL);
		if (copyL <= 0 || copyL < strL) {
			lineStartsL = oldLineStartsL;
			return false;
		}

		const char* src = str+lastStrOffset;
		char* dst = textBuffer+offset;

		if (stripSpaces) {
			//merge multiple spaces in a row into a single space
			for (int c = 0; c < copyL; c++) {
				if (*src != ' ' || c == 0 || dst[-1] != ' ') {
					*dst = *src;
					dst++;
				}
				src++;
			}
		} else {
			strncpy(dst, src, copyL);
			dst += copyL;
		}
		*dst = '\0';

		colors[lineStartsL] = color|BIT(15);
		lineStartsL++;
		lineStarts[lineStartsL] = offset + (dst - textBuffer - offset) + 1;
		lastStrOffset = lineBreaks[n];
	}

	for (int n = 0; n < lineBreaksL; n++) {
		SetItemDirty(oldLineStartsL+n);
	}
	SetNumberOfItems(lineStartsL);

	SetSelectedIndex(totalItemL-1);
	SetScroll(MAX(0, totalItemL - visibleItemL));

	return true;
}
void TextScrollPane::RemoveAllItems() {
	lineStartsL = 0;

	ScrollPane::SetNumberOfItems(0);
}

//Remove the first <number> items from the list
void TextScrollPane::RemoveItems(int number) {
	if (number < lineStartsL) {
		int bufferStart = lineStarts[number];
		int bufferEnd   = lineStarts[lineStartsL];

		memmove(textBuffer, textBuffer+bufferStart, bufferEnd - bufferStart);
		memmove(colors,     colors+number,     (lineStartsL-number)*sizeof(u16));
		for (int n = 0; n <= lineStartsL-number; n++) {
			lineStarts[n] = lineStarts[n+number]-bufferStart;
		}

		lineStartsL -= number;
		totalItemL  -= number;

		SetBackgroundDirty();
		for (int n = scroll; n < scroll+visibleItemL; n++) {
			SetItemDirty(n);
		}
	} else {
		RemoveAllItems();
	}
}

int TextScrollPane::GetFontSize() {
	return fontSize;
}

u16 TextScrollPane::GetTextColor() {
	return textColor;
}

void TextScrollPane::SetTextColor(u16 c) {
	textColor = c|BIT(15);

	SetBackgroundDirty();
	for (int n = scroll; n < scroll+visibleItemL; n++) {
		SetItemDirty(n);
	}
}

void TextScrollPane::SetTextBufferSize(int size) {
	if (textBuffer) {
		delete[] textBuffer;
	}

	textBufferL = size;
	textBuffer = new char[size];
	textBuffer[0] = '\0';
	lineStarts[0] = 0;
	lineStartsL = 0;
}
void TextScrollPane::SetTextWrapping(bool w) {
	textWrapping = w;
}
void TextScrollPane::SetFont(const char* filename) {
	text->SetFont(filename);
}
void TextScrollPane::SetFontSize(u16 s, bool sizeToFit) {
	if (s != 0 && (sizeToFit || fontSize != s)) {
		text->SetFontSize(fontSize = s);

		if (sizeToFit || listItemH < text->GetLineHeight()+1) {
			SetItemHeight(text->GetLineHeight()+1);
		}
	}
}
void TextScrollPane::SetItemHeight(u16 ih) {
	int oldScroll = scroll;
	scroll = 0;

	ScrollPane::SetItemHeight(ih);
	text->SetBuffer(listItemW, listItemH);

	SetScroll(oldScroll);
}
void TextScrollPane::SetDefaultMargins(u16 left, u16 right, u16 top, u16 bottom) {
	text->SetMargins(left, right, top, bottom);
}
void TextScrollPane::SetBounds(s16 x, s16 y, u16 w, u16 h) {
	SetBounds(Rect(x, y, w, h));
}
void TextScrollPane::SetBounds(Rect r) {
	ScrollPane::SetBounds(r);

	if (IsVisible()) {
		text->SetBuffer(listItemW, listItemH);
	}
}
void TextScrollPane::SetVisible(bool v) {
	if (v != IsVisible()) {
		ScrollPane::SetVisible(v);

		if (!v) {
			text->SetBuffer(0, 0);
		} else {
			text->SetBuffer(listItemW, listItemH);
			SetBackgroundDirty();
			for (int n = scroll; n < scroll+visibleItemL; n++) {
				SetItemDirty(n);
			}
		}
	}
}
