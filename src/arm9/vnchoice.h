#ifndef VNCHOICE_H
#define VNCHOICE_H

#include "common.h"
#include "script_engine.h"
#include "tcommon/gui/textscrollpane.h"

class VNChoice : public TextScrollPane {
	private:
		VNDS* vnds;
		TextEngine* textEngine;
		int choiceLineOffsets[CMD_OPTIONS_MAX_OPTIONS];
		int choiceLineOffsetsL;
		bool active;

		bool IsChoiceStart(int s);
		int GetSelectedChoice(int s);

	public:
		VNChoice(VNDS* vnds, TextEngine* te, FontCache* fontCache);
		virtual ~VNChoice();

		virtual void Activate();
		virtual void Deactivate();
		virtual void RemoveAllItems();
		virtual void RemoveItems(int number);
		virtual void Update(u32& keysDown, u32& keysHeld, touchPosition touch);
		virtual void OnButtonPressed(Button* button);
		virtual bool AppendText(const char* str, u16 color=0, bool stripSpaces=true);
		virtual void DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected);
		virtual void DrawListItemForeground(s16 index, Rect bounds, bool selected);

		bool IsActive();

		virtual void SetSelectedIndex(int s);
};

#endif
