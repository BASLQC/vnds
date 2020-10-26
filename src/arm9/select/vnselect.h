#ifndef VNSELECT_H
#define VNSELECT_H

#include "../common.h"
#include "vnselectscreen.h"

class VNSelect {
	private:
		GUI* gui;
		u16 textureI[256*256];
		u16 backgroundI[256*384];

		VNSelectScreen* mainScreen;

	public:
		NovelInfo* selectedNovelInfo;
		int selectedNovel;

		VNSelect();
		~VNSelect();

		void Run();
		void Play();
};

#endif
