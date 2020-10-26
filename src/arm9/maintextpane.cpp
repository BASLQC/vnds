#include "maintextpane.h"
#include "tcommon/text.h"
#include "tcommon/gui/gui_common.h"

MainTextPane::MainTextPane(Screen* screen, FontCache* fontCache)
: VNTextScrollPane(screen, fontCache)
{
	currentChunkStart = 0;
	SetTextColor(skin->GetTextColor());
	activeTextColor = skin->GetActiveTextColor();
	textSpeed = preferences->GetTextSpeed();
	SetBackgroundColor(skin->GetBackgroundColor());

	SetShowSelections(false);
	SetTextBufferSize(MainTextPane::textBufferL);
	SetDefaultMargins(4, 4, 0, 0);
}

MainTextPane::~MainTextPane() {

}

void MainTextPane::Update(u32& down, u32& held, touchPosition touch) {
	VNTextScrollPane::Update(down, held, touch);

	if (textSpeed > 0) {
		int strL = lineStarts[lineStartsL] - lineStarts[currentChunkStart];
		if (activeVisibleChars < strL) {
			for (int n = currentChunkStart; n < GetNumberOfItems(); n++) {
				int offset = activeVisibleChars+lineStarts[currentChunkStart];
				if (offset-lineStarts[n] >= 0 && offset-lineStarts[n+1] <= 0) {
					SetItemDirty(n);
				}
			}

			activeVisibleChars += textSpeed;
		}
	}
}

void MainTextPane::DisplayFullText() {
	for (int n = currentChunkStart; n < GetNumberOfItems(); n++) {
		SetItemDirty(n);
	}
	activeVisibleChars = 9999;
}

bool MainTextPane::AppendText(const char* text, u16 color, bool stripSpaces) {
	Rect r = GetBounds();
	for (int n = currentChunkStart; n < GetNumberOfItems(); n++) {
		SetItemDirty(n);
	}

	currentChunkStart = GetNumberOfItems();
	while (!TextScrollPane::AppendText(text, color, stripSpaces) && GetNumberOfItems() > 0) {
		int remove = (GetNumberOfItems() >> 2);
		currentChunkStart -= remove;
		RemoveItems(remove);
	}

	activeVisibleChars = (textSpeed > 0 ? 0 : -1);
	return true;
}

void MainTextPane::DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected) {
	u16 oldColor = colors[index];

	if (oldColor == GetTextColor() && index >= currentChunkStart) {
		colors[index] = activeTextColor;
		if (activeVisibleChars >= 0) {
			text->SetVisibleChars(MAX(0, activeVisibleChars
					-lineStarts[index]+lineStarts[currentChunkStart]));
		} else {
			text->SetVisibleChars(-1);
		}
	}

	VNTextScrollPane::DrawListItemBackground(index, buffer, w, h, selected);

	colors[index] = oldColor;
	text->SetVisibleChars(-1);
}

bool MainTextPane::IsFullTextDisplayed() {
	return activeVisibleChars < 0 || activeVisibleChars >= lineStarts[lineStartsL]-lineStarts[currentChunkStart];
}

u16 MainTextPane::GetActiveTextColor() {
	return activeTextColor;
}
s16 MainTextPane::GetTextSpeed() {
	return textSpeed;
}

void MainTextPane::SetActiveTextColor(u16 c) {
	activeTextColor = c|BIT(15);

	SetBackgroundDirty();
	for (int n = scroll; n < MIN(totalItemL, scroll+visibleItemL); n++) {
		SetItemDirty(n);
	}
}
void MainTextPane::SetTextSpeed(s16 ts) {
	textSpeed = (ts > 0 ? ts : -1);
	activeVisibleChars = 9999;
}
