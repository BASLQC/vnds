#include "vnselectscreen.h"

#include "vnselect.h"
#include "vnaboutscreen.h"
#include "../tcommon/parser/ini_parser.h"
#include "../tcommon/gui/gui_common.h"

VNSelectScrollPane::VNSelectScrollPane(VNSelectScreen* s, Rect upImg, Rect downImg, Rect barImg, FontCache* fc)
: TextScrollPane(s, upImg, downImg, barImg, fc)
{
	vnSelectScreen = s;
}
VNSelectScrollPane::~VNSelectScrollPane() {

}

void VNSelectScrollPane::DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected) {
	TextScrollPane::DrawListItemBackground(index, buffer, w, h, selected);

	const u16* img = NULL;
	if (index >= 0 && index < vnSelectScreen->novelInfoL) {
		img = vnSelectScreen->novelInfo[index]->GetIcon();
	}
	if (img) {
		blit(img, 32, 32, buffer, w, h, 0, 0, 0, 0, w, h);
	}
}

//------------------------------------------------------------------------------

VNSelectScreen::VNSelectScreen(GUI* gui, VNSelect* vns, u16* tex, u16* bg) : Screen(gui) {
	vnselect = vns;

	memcpy(topBuffer, bg, 256*192*2);
	topScreenDirty = true;
	lastSelected = -1;

	loadState = 0;
	novelInfo = NULL;
	novelInfoL = 0;

	SetTexture(tex);
	SetBackgroundImage(bg+256*192);

	char path[MAXPATHLEN];
	sprintf(path, "%s/font.ttf", skinPath);
	fontCache = new FontCache(path);

	scrollPane = new VNSelectScrollPane(this, Rect(0,0,16,16), Rect(0,16,16,16),
			Rect(0,32,16,16), fontCache); //Deleted automatically
	scrollPane->SetItemBg(tex+(224<<8), 256, 32, tex+(192<<8), 256, 32);
	scrollPane->SetTextWrapping(false);
	scrollPane->SetTextBufferSize(1024);
	scrollPane->SetItemHeight(32);
	scrollPane->SetDefaultMargins(40, 8, 0, 0);
	scrollPane->SetFontSize(skin->GetSelectFontSize(), false);
	scrollPane->SetBounds(0, 0, 256, 160);

	aboutButton = new Button(this);
	aboutButton->SetButtonListener(this);
	aboutButton->SetBounds(16, 160, 96, 32);
	aboutButton->SetForegroundImage(Rect(16, 64, 96, 32));
	aboutButton->SetPressedImage(Rect(16, 96, 96, 32));

	playButton = new Button(this);
	playButton->SetButtonListener(this);
	playButton->SetBounds(144, 160, 96, 32);
	playButton->SetForegroundImage(Rect(144, 64, 96, 32));
	playButton->SetPressedImage(Rect(144, 96, 96, 32));
	playButton->SetActivationKeys(KEY_A|KEY_START);
}
VNSelectScreen::~VNSelectScreen() {
    if (novelInfo) {
    	for (int n = 0; n < novelInfoL; n++) {
    		delete novelInfo[n];
    	}
    	delete[] novelInfo;
    }
    delete fontCache;
}

void VNSelectScreen::LoadNovels() {
    IniFile iniFile;
    FileList folderList("novels", NULL, false);

    if (novelInfo) {
    	for (int n = 0; n < novelInfoL; n++) {
    		delete novelInfo[n];
    	}
    	delete[] novelInfo;
    }
    novelInfoL = folderList.GetFilesL();
    novelInfo = new NovelInfo*[novelInfoL];

    int t = 0;
    char path[MAXPATHLEN];
    char* folderName;
    while ((folderName = folderList.NextFile()) != NULL) {
    	novelInfo[t] = new NovelInfo();

    	sprintf(path, "novels/%s", folderName);
        novelInfo[t]->SetPath(path);

        IniRecord* r;
        sprintf(path, "novels/%s/info.txt", folderName);
        if (iniFile.Load(path)) {
            r = iniFile.GetRecord("title");
            if (r) novelInfo[t]->SetTitle(r->AsString());
            r = iniFile.GetRecord("fontsize");
            if (r) novelInfo[t]->SetFontSize(r->AsInt());

            t++;
        }
    }
    novelInfoL = t;

    //Sort by title asc.
    //Fuck Yeah Bubble Sort!
    for (int a = 0; a < t; a++) {
        for (int b = a+1; b < t; b++) {
            if (strcmp(novelInfo[a]->GetTitle(), novelInfo[b]->GetTitle()) > 0) {
                NovelInfo* temp = novelInfo[a];
                novelInfo[a] = novelInfo[b];
                novelInfo[b] = temp;
            }
        }
    }
}

void VNSelectScreen::Update(u32& down, u32& held, touchPosition touch) {
	if (loadState < 2) {
		if (loadState == 0) {
			scrollPane->RemoveAllItems();
		} else if (loadState == 1) {
			lastSelected = -1;

			LoadNovels();

			if (novelInfoL > 0) {
				for (int n = 0; n < novelInfoL; n++) {
					scrollPane->AppendText(novelInfo[n]->GetTitle());
				}
				scrollPane->SetSelectedIndex(0);
			} else {
				scrollPane->AppendText("no novels found in /vnds/novels", RGB15(31, 8, 8));
				scrollPane->SetSelectedIndex(-1);
			}
			scrollPane->SetScroll(0);
		}
		loadState++;
	}

	Screen::Update(down, held, touch);
}
void VNSelectScreen::DrawTopScreen() {
	int s = scrollPane->GetSelectedIndex();
	if (lastSelected != s) {
		if (s >= 0 && s < novelInfoL) {
			blit(novelInfo[s]->GetThumbnail(), 100, 75, topBuffer, 256, 192, 0, 0, 25, 57, 100, 75);

			topScreenDirty = true;
		}
		lastSelected = s;
	}

	if (topScreenDirty) {
		DC_FlushRange(topBuffer, 256*192*2);
		dmaCopy(topBuffer, gui->sub_bg, 256*192*2);
		topScreenDirty = false;
	}
}

void VNSelectScreen::DrawForeground() {
	Screen::DrawForeground();

	drawQuad(texture, inttof32(0), inttof32(160), VERTEX_SCALE(256), VERTEX_SCALE(32),
			Rect(0, 64, 8, 32));
}


void VNSelectScreen::OnButtonPressed(Button* button) {
	if (button == aboutButton) {
		gui->PushScreen(new VNAboutScreen(gui, textureImage));
	} else if (button == playButton) {
		vnselect->Play();
	}
}

int VNSelectScreen::GetSelectedIndex() {
	return scrollPane->GetSelectedIndex();
}
