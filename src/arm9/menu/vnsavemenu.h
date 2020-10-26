#ifndef VNSAVEMENU_H
#define VNSAVEMENU_H

#include "../common.h"
#include "../tcommon/gui/screen.h"
#include "../tcommon/gui/button.h"

#define SLOTS_PER_PAGE 6

class SaveSlotRecord {
	public:
		static const int labelLength = 32;

		bool valid;
		char label[labelLength];

		SaveSlotRecord();
		~SaveSlotRecord();
};

class VNSaveMenu : public Screen, public ButtonListener {
	private:
		VNDS* vnds;
		Button* backButton;
		Button* actionButton;
		Button* leftButton;
		Button* rightButton;
		Text* text;
		Texture texture2;
		bool save;

		s16 slot;
		s16 page;
		u16 pageMin, pageMax;
		SaveSlotRecord slots[SAVE_SLOTS];
		u16 backgroundI[256*192];
		u16 savemenuTexI[SAVE_IMG_W * SAVE_IMG_H * 2];
		u32 animationFrame;
		int scroll;
		int scrollDir;

		void UpdateImages();
		void Scroll(int dir);
		void DrawSlot(int n);

	public:
		VNSaveMenu(GUI* gui, u16* textureI, VNDS* vnds, bool save);
		virtual ~VNSaveMenu();

		void SetPage(s16 p);

		virtual void OnButtonPressed(Button* button);
		virtual void Activate();
		virtual void Update(u32& down, u32& held, touchPosition touch);
		virtual void DrawBackground();
		virtual void DrawForeground();

};

#endif
