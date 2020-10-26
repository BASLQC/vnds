#ifndef TEXT_ENGINE_H
#define TEXT_ENGINE_H

#include "common.h"
#include "tcommon/gui/screen.h"
#include "tcommon/gui/button.h"

class TextEngine : public Screen, public ButtonListener {
	private:
		VNDS* vnds;
		MainTextPane* textPane;
		VNChoice* choiceView;
		Button* backlightButton;
		Button* saveButton;
		Button* loadButton;
		Button* menuButton;
		Button* muteButton;
		Button* repeatSoundButton;
		Button* textAdvButton;

		u16 bottomBarI[16 * 16];
		FontCache* fontCache;
		u32 frame;
		u16 backgroundColor;

		void DrawWaitingIcon(u32 frame);
		void RepaintTime();

	public:
		TextEngine(GUI* gui, VNDS* vnds, u8 fontsize);
		virtual ~TextEngine();

		void Reset();
		void ShowMenu();
		void ShowLoadMenu();
		void ShowSaveMenu();
		virtual void SetTexture(u16* textureI);
		virtual void Update(u32& down, u32& held, touchPosition touch);
		virtual void OnButtonPressed(Button* button);
		virtual void DrawBackground();
		virtual void DrawForeground();

		MainTextPane* GetTextPane();
		VNChoice* GetChoiceView();

		void SetBackgroundColor(u16 c);
};

#endif
