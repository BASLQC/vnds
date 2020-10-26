#ifndef VNSELECTSCREEN_H
#define VNSELECTSCREEN_H

#include "../novelinfo.h"
#include "../tcommon/gui/screen.h"
#include "../tcommon/gui/button.h"
#include "../tcommon/gui/textscrollpane.h"

class VNSelect;
class VNSelectScreen;

class VNSelectScrollPane : public TextScrollPane {
	private:
		VNSelectScreen* vnSelectScreen;

	public:
		VNSelectScrollPane(VNSelectScreen* screen, Rect upImg, Rect downImg, Rect barImg, FontCache* fc);
		virtual ~VNSelectScrollPane();

		virtual void DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected);

};

class VNSelectScreen : public Screen, public ButtonListener {
	private:
		VNSelect* vnselect;
		TextScrollPane* scrollPane;
		Button* aboutButton;
		Button* playButton;

		u8 loadState;

		FontCache* fontCache;
		u16 topBuffer[256*192];
		u16 thumbnailI[100*75];
		u16 iconI[32*32];

		bool topScreenDirty;
		int lastSelected;

		void LoadNovels();

	public:
		NovelInfo** novelInfo;
		u16 novelInfoL;

		VNSelectScreen(GUI* gui, VNSelect* vnselect, u16* tex, u16* bg);
		virtual ~VNSelectScreen();

		virtual void OnButtonPressed(Button* button);
		virtual void Update(u32& down, u32& held, touchPosition touch);
		virtual void DrawTopScreen();
		virtual void DrawForeground();

		int GetSelectedIndex();
};

#endif
