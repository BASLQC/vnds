#include "vnchoice.h"

#include "text_engine.h"
#include "vnds.h"
#include "maintextpane.h"
#include "tcommon/text.h"
#include "tcommon/gui/gui_common.h"

VNChoice::VNChoice(VNDS* vnds, TextEngine* te, FontCache* fontCache)
: TextScrollPane(te, Rect(0,0,16,16), Rect(0,16,16,16), Rect(0,32,16,16), fontCache)
{
	this->vnds = vnds;
	this->textEngine = te;

	choiceLineOffsetsL = 0;
	active = false;

	SetShowSelections(true);
	SetTextColor(skin->GetTextColor());
	SetTextBufferSize(CMD_TEXT_LENGTH*2); //Reserve double space for additional formatting
	SetDefaultMargins(20, 20, 0, 0);
	SetBackgroundColor(skin->GetBackgroundColor());
}
VNChoice::~VNChoice() {

}

void VNChoice::RemoveAllItems() {
	choiceLineOffsetsL = 0;

	TextScrollPane::RemoveAllItems();
}
void VNChoice::RemoveItems(int number) {
	memmove(choiceLineOffsets, choiceLineOffsets+number, (choiceLineOffsetsL-number)*sizeof(int));
	choiceLineOffsetsL -= number;

	TextScrollPane::RemoveItems(number);
}

void VNChoice::Update(u32& keysDown, u32& keysHeld, touchPosition touch) {
	TextScrollPane::Update(keysDown, keysHeld, touch);

	if (!(keysHeld&KEY_L) && (keysDown&KEY_A)) {
		Deactivate();
	}
}

void VNChoice::Activate() {
	active = true;

	textEngine->GetTextPane()->SetVisible(false);
	SetVisible(true);
}
void VNChoice::Deactivate() {
	active = false;

	char valueStr[16];
	sprintf(valueStr, "%d", GetSelectedChoice(GetSelectedIndex())+1);
	vnds->SetVariable("selected", '=', valueStr);

	SetVisible(false);
}

void VNChoice::OnButtonPressed(Button* button) {
	if (button == scrollUpButton) {
		int s = GetSelectedIndex();
		if (s > 0) {
			do {
				s--;
			} while (s >= 0 && !IsChoiceStart(s));

			if (s >= 0) {
				SetSelectedIndex(s);
				if (s < scroll) {
					SetScroll(s);
				}
			}
		}
	} else if (button == scrollDownButton) {
		int s = GetSelectedIndex();
		if (s < totalItemL-1) {
			do {
				s++;
			} while (s < GetNumberOfItems() && !IsChoiceStart(s));

			if (s < GetNumberOfItems()) {
				SetSelectedIndex(s);
				if (s >= scroll+visibleItemL) {
					SetScroll(s-visibleItemL+1);
				}
			}
		}
	}
}

bool VNChoice::AppendText(const char* str, u16 color, bool stripSpaces) {
	int oldLines = GetNumberOfItems();
	TextScrollPane::AppendText(str, color, stripSpaces);
	//TextScrollPane::AppendText("\n", color, stripSpaces);

	if (choiceLineOffsetsL < CMD_OPTIONS_MAX_OPTIONS) {
		choiceLineOffsets[choiceLineOffsetsL] = oldLines;
		choiceLineOffsetsL++;
	} else {
		vnLog(EL_error, COM_TEXT, "VNChoice.AppendText() :: Can't add that many choices!");
	}
	return true;
}

void VNChoice::DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected) {
	u64 oldMargins = text->GetMargins();
	u16 oldColor = colors[index];
	if (selected || GetSelectedChoice(index) == GetSelectedChoice(GetSelectedIndex())) {
		colors[index] = vnds->textEngine->GetTextPane()->GetActiveTextColor();
	}

	if (!IsChoiceStart(index)) {
		text->SetMargins(36, 4, 0, 0);
	}

	TextScrollPane::DrawListItemBackground(index, buffer, w, h, false);

	colors[index] = oldColor;
	text->SetMargins(oldMargins);
}
void VNChoice::DrawListItemForeground(s16 index, Rect r, bool selected) {
	TextScrollPane::DrawListItemForeground(index, r, selected);

	if (GetSelectedIndex() == index) {
		s32 vw = VERTEX_SCALE(16);
		s32 vh = VERTEX_SCALE(16);
		drawQuad(texture, inttof32(r.x), inttof32(r.y-2), vw, vh, Rect(0, 48, 16, 16));
	}
}

bool VNChoice::IsChoiceStart(int s) {
	for (int n = 0; n < choiceLineOffsetsL; n++) {
		if (choiceLineOffsets[n] == s) {
			return true;
		}
	}
	return false;
}
int VNChoice::GetSelectedChoice(int s) {
	int index = 0;
	for (int n = 0; n < choiceLineOffsetsL; n++) {
		if (s >= choiceLineOffsets[n] && (n+1 >= choiceLineOffsetsL || s < choiceLineOffsets[n+1])) {
			index = n;
		}
	}
	return index;
}
bool VNChoice::IsActive() {
	return active;
}

void VNChoice::SetSelectedIndex(int s) {
	int index = GetSelectedChoice(s);
	int t = choiceLineOffsets[index];

	for (int n = 0; n < GetNumberOfItems(); n++) {
		SetItemDirty(n);
	}
	TextScrollPane::SetSelectedIndex(t);

	int prevOption = (index == 0 ? 0 : choiceLineOffsets[index-1]);
	int nextOption = (index == choiceLineOffsetsL-1 ? totalItemL : choiceLineOffsets[index+1]);
	SetScroll(MAX(prevOption, nextOption-visibleItemL));
}
