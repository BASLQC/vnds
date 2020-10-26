#include "vnselect.h"

#include "../as_lib9.h"
#include "../tcommon/text.h"
#include "../tcommon/gui/gui_common.h"

VNSelect::VNSelect() {
	selectedNovel = -1;

	gui = new GUI();

	skinLoadImage("%s/select_tex", textureI, 256, 256);
}
VNSelect::~VNSelect() {
	delete gui;
}

void VNSelect::Run() {
	skinLoadImage("%s/select_bg", backgroundI, 256, 384);
	DC_FlushRange(backgroundI, 256*384*2);

	gui->VideoInit();

	mainScreen = new VNSelectScreen(gui, this, textureI, backgroundI);
	gui->PushScreen(mainScreen);

	for (int n = 0; n < 3; n++) gui->Update(); //Force loading of novels
	gui->Draw();
	unfadeBlack2(16);

	while (selectedNovel < 0) {
		gui->Update();
        gui->Draw();

        AS_SoundVBL();
        swiWaitForVBlank();
	}
}

void VNSelect::Play() {
	selectedNovel = mainScreen->GetSelectedIndex();
	if (selectedNovel >= 0 && selectedNovel < mainScreen->novelInfoL) {
		selectedNovelInfo = mainScreen->novelInfo[selectedNovel];
	} else {
		selectedNovel = -1;
	}
}
