#ifndef T_GUI_H
#define T_GUI_H

#include "../common.h"
#include "gui_types.h"

#define DEFAULT_VIDEO_MODE (MODE_5_3D|DISPLAY_BG3_ACTIVE|DISPLAY_SPR_ACTIVE|DISPLAY_SPR_2D_BMP_256)
#define BASE_POLY_FMT (POLY_CULL_NONE|POLY_FORMAT_LIGHT0)
#define DEFAULT_POLY_FMT (BASE_POLY_FMT|POLY_ALPHA(31))
#define SCREEN_HISTORY_MAXSIZE 8

enum GUITransition {
	GT_none,
	GT_slide,
	GT_fade,
	GT_fadeMain,
	GT_fadeSub
};

class GUI {
	private:
        int fade;

        Screen* history[SCREEN_HISTORY_MAXSIZE];
        Screen* removeList[2];
        int removeListL;

	public:
		int historyL;

		u16* main_bg;
        u16* sub_bg;
        int bg2, bg3;
        TextureManager texmgr;
        SpriteEntry mainSprites[SPRITE_COUNT];
        SpriteEntry subSprites[SPRITE_COUNT];

        GUI();
		virtual ~GUI();

		void PushScreen(Screen* s, GUITransition transition=GT_slide);
		void PopScreen(GUITransition transition=GT_slide);
		void VideoInit();
		void UpdateOAM();
		virtual void Update();
		virtual void Draw();

		Screen* GetActiveScreen();
};

#endif
