#include "vnsavemenu.h"

#include "../vnds.h"
#include "../res.h"
#include "../saveload.h"
#include "../tcommon/text.h"
#include "../tcommon/xml_parser.h"
#include "../tcommon/gui/gui_common.h"

#define ROWS 2
#define COLS 3
#define BAR_H 10
#define SAVE_IMG_PAD 8

const int NUM_PAGES = (SAVE_SLOTS + (SLOTS_PER_PAGE-1)) / SLOTS_PER_PAGE;

SaveSlotRecord::SaveSlotRecord() {
	valid = false;
	label[0] = '\0';
}
SaveSlotRecord::~SaveSlotRecord() {
}

//------------------------------------------------------------------------------

VNSaveMenu::VNSaveMenu(GUI* gui, u16* textureI, VNDS* vnds, bool save)
: Screen(gui)
{
	this->vnds = vnds;
	this->save = save;
	this->slot = 0;

	//SetTexture(textureI);
	SetBackgroundImage(backgroundI);

	backButton = new Button(this);
	backButton->SetButtonListener(this);
	backButton->SetBounds(53, 160, 64, 32);
	backButton->SetForegroundImage(Rect(192, 192, 64, 32));
	backButton->SetPressedImage(Rect(192, 224, 64, 32));
	backButton->SetActivationKeys(KEY_B|KEY_X|KEY_START);

	actionButton = new Button(this);
	actionButton->SetButtonListener(this);
	actionButton->SetBounds(139, 160, 64, 32);
	actionButton->SetForegroundImage(Rect(save?128:64, 192, 64, 32));
	actionButton->SetPressedImage(Rect(save?128:64, 224, 64, 32));
	actionButton->SetActivationKeys(KEY_A);

	leftButton = new Button(this);
	leftButton->SetButtonListener(this);
	leftButton->SetBounds(0, 160, 32, 32);
	leftButton->SetForegroundImage(Rect(0, 192, 32, 32));
	leftButton->SetPressedImage(Rect(0, 224, 32, 32));

	rightButton = new Button(this);
	rightButton->SetButtonListener(this);
	rightButton->SetBounds(224, 160, 32, 32);
	rightButton->SetForegroundImage(Rect(32, 192, 32, 32));
	rightButton->SetPressedImage(Rect(32, 224, 32, 32));

	text = new Text();
	text->SetBuffer(256, 16);
	text->SetFontSize(skin->GetSavemenuFontSize());

	for (int n = 0; n < SAVE_IMG_W*SAVE_IMG_H*2; n++) {
		savemenuTexI[n] = BIT(15);
	}

	scroll = 0;
	scrollDir = 0;

    SetPage(0);
}
VNSaveMenu::~VNSaveMenu() {
	delete text;
}

void VNSaveMenu::Activate() {
	Screen::Activate();

    skinLoadImage("../../%s/savemenu_tex", savemenuTexI, SAVE_IMG_W, SAVE_IMG_H*2);
    skinLoadImage("../../%s/menu_bg", backgroundI, 256, 192);

	texture2.dataChunk.offset = (u32)VRAM_D;
	texture2.dataChunk.length = 256 * 256 * sizeof(u16);
	texture2.palChunk.offset = texture2.palChunk.length = 0;
	texture2.sizeX = TEXTURE_SIZE_256;
	texture2.sizeY = TEXTURE_SIZE_256;
	texture2.addr = (texture2.dataChunk.offset >> 3) & 0xFFFF;
	texture2.mode = GL_RGBA;
	setTextureParam(&texture2, (TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T));

    UpdateImages();
}

void VNSaveMenu::Scroll(int dir) {
	scroll = 0;
	scrollDir = dir;

	SetBackgroundDirty();
}

void VNSaveMenu::OnButtonPressed(Button* button) {
	if (button == backButton) {
		gui->PopScreen(GT_fadeMain);
	} else if (button == actionButton) {
		if (slot >= 0 && slot < SAVE_SLOTS) {
			if (save || slots[slot].valid) {
				gui->PushScreen(new ConfirmScreen(gui, "Are you sure?",
					Rect(192, 128, 64, 32), Rect(192, 160, 64, 32), this,
					Rect(192, 192, 64, 32), Rect(192, 224, 64, 32), NULL),
					GT_none);
			}
		}
	} else if (button == leftButton) {
		Scroll(1);
	} else if (button == rightButton) {
		Scroll(-1);
	} else {
		//Confirmscreen's ok button was pressed
		if (save) {
			if (saveState(vnds, slot)) {
				vnLog(EL_message, COM_CORE, "Save to slot (%02d) successful", slot);
			}
		} else {
			loadState(vnds, slot);
		}

		gui->PopScreen(GT_fadeMain);
	}
}

void VNSaveMenu::Update(u32& down, u32& held, touchPosition touch) {
	Screen::Update(down, held, touch);

    if (scroll < -256) {
    	scroll = scrollDir = 0;
    	SetPage(page < NUM_PAGES-1 ? page+1 : 0);
    	slot = pageMin;
    } else if (scroll >= 256) {
    	scroll = scrollDir = 0;
    	SetPage(page > 0 ? page-1 : NUM_PAGES-1);
    	slot = pageMax;
    }

    if (scroll == 0) {
        if (held & KEY_TOUCH) {
            if (touch.py >= BAR_H && touch.py < 150) {
    			int col = (touch.px * COLS / 256);
    			int row = ((touch.py-BAR_H) * ROWS / (160 - BAR_H));
    			int newSelected = pageMin + row * COLS + col;

    			if (newSelected >= pageMin && newSelected <= pageMax) {
    				slot = newSelected;
    				animationFrame = 0;
    			}
            }
        }

        //Keyboard slot selection
        int oldSlot = slot;

		int row = (slot % SLOTS_PER_PAGE) / COLS;
		int col = slot % COLS;

		if (down & KEY_LEFT)  col--;
		if (down & KEY_RIGHT) col++;
		if (down & KEY_UP)    row--;
		if (down & KEY_DOWN)  row++;

		if (row < 0) row = ROWS-1;
		if (row >= ROWS) row = 0;
		if (col < 0) {
			slot = (page-1) * SLOTS_PER_PAGE + row*COLS + COLS-1;
		} else if (col >= COLS) {
			slot = (page+1) * SLOTS_PER_PAGE + row*COLS;
		} else {
			slot = page*SLOTS_PER_PAGE+row*COLS+col;
		}

		if (slot != oldSlot) {
			animationFrame = 0;
		}

		if (slot < pageMin) {
			Scroll(1);
		} else if (slot > pageMax) {
			Scroll(-1);
		}
    }
}

void VNSaveMenu::UpdateImages() {
	char path[MAXPATHLEN];
	u16 imgBuffer[SAVE_IMG_W * SAVE_IMG_H];
    vramSetBankD(VRAM_D_LCD);

    XmlFile xmlFile;

    //Store in the bottomright rect in VRAM
    blit(savemenuTexI, SAVE_IMG_W, SAVE_IMG_H*2, VRAM_D, 256, 256,
    		0, SAVE_IMG_H, 256-SAVE_IMG_W, 256-SAVE_IMG_H, SAVE_IMG_W, SAVE_IMG_H);

    int cols = 256 / SAVE_IMG_W;
	for (int n = 0; n < SAVE_SLOTS; n++) {
        slots[n].valid = true;

        //Read date from save file
        sprintf(path, "save/save%.2d.sav", n);
        if (!fexists(path)) {
        	slots[n].valid = false;
        	continue;
        }

        XmlNode* rootE = xmlFile.Open(path);
        XmlNode* dateE = rootE->GetChild("date");

        if (dateE) {
			int labelL = SaveSlotRecord::labelLength;
			strncpy(slots[n].label, dateE->GetTextContent(), labelL-1);
			slots[n].label[labelL-1] = '\0';
        } else {
        	slots[n].label[0] = '\0';
        }

        //Read image
        int dx = SAVE_IMG_W * (n % cols);
        int dy = SAVE_IMG_H * (n / cols);

        sprintf(path, "save/save%.2d.img", n);
        FILE* file = fopen(path, "rb");
        if (file) {
            fread(imgBuffer, sizeof(u16), SAVE_IMG_W * SAVE_IMG_H, file);
            fclose(file);

            blit(imgBuffer, SAVE_IMG_W, SAVE_IMG_H, VRAM_D, 256, 256,
            		0, 0, dx, dy, SAVE_IMG_W, SAVE_IMG_H);
        } else {
            blit(savemenuTexI, SAVE_IMG_W, SAVE_IMG_H*2, VRAM_D, 256, 256,
            		0, 0, dx, dy, SAVE_IMG_W, SAVE_IMG_H);
        }
    }

	vramSetBankD(VRAM_D_TEXTURE);
}

void VNSaveMenu::SetPage(s16 p) {
    page = p;

    if (page < 0) page += NUM_PAGES;
    if (page >= NUM_PAGES) page -= NUM_PAGES;

    pageMin = page * SLOTS_PER_PAGE;
    pageMax = pageMin + SLOTS_PER_PAGE - 1;

    if (slot < pageMin) slot = pageMin;
    if (slot > pageMax) slot = pageMax;

    SetBackgroundDirty();

    animationFrame = 0;
}

void VNSaveMenu::DrawBackground() {
	Screen::DrawBackground();

	text->SetColor(RGB15(31, 31, 31));
    for (int n = pageMin; n <= pageMax; n++) {
    	int l = n % SLOTS_PER_PAGE;

    	int col = l % COLS;
    	int row = l / COLS;
    	int x = 16 + 80*col;
        int y = BAR_H + 8 + 67*row;

        if (col == 0 && row <= 1) {
        	darken(background, 3, 0, y+SAVE_IMG_H+3, 256, BAR_H);
        }

        if (scrollDir == 0) {
			text->ClearBuffer();
			text->SetMargins(0, 0, (BAR_H - text->GetLineHeight())/2 - 3, 0);
			text->PrintLine(slots[n].label);
			text->BlitToScreen(background, 256, 192, 0, 0,
					x-2, y+SAVE_IMG_H+3, SAVE_IMG_W+2, BAR_H);
        }
    }

    for (int n = 0; n < 256 * BAR_H; n++) {
        int a = 31 - (n/256);
        background[n] = RGB15(a, a, a) | BIT(15);
    }

    char string[128];
    text->SetColor(RGB15(0, 0, 0));
    text->ClearBuffer();

    printTime(string);
    text->SetMargins(8, 8, (BAR_H - text->GetLineHeight())/2 - 2, 0);
    text->PrintLine(string);

    sprintf(string, "Page %d/%d", page+1, NUM_PAGES);
    text->SetMargins(207, 8, (BAR_H - text->GetLineHeight())/2 - 2, 0);
    text->PrintLine(string);

    text->BlitToScreen(background, 256, 192, 0, 0, 0, 0, 256, BAR_H);
}

void VNSaveMenu::DrawSlot(int n) {
    int texcols = 256 / SAVE_IMG_W;
    int vw = VERTEX_SCALE(SAVE_IMG_W);
    int vh = VERTEX_SCALE(SAVE_IMG_H);

    int p = n / SLOTS_PER_PAGE;
	int l = n % SLOTS_PER_PAGE;

	int col = p * COLS + l % COLS;
	int row = l / COLS;
	int x = 16*p  + 16 + 80*col;
    int y = BAR_H + 8 + 67*row;

    Rect r(SAVE_IMG_W * (n % texcols), SAVE_IMG_H * (n / texcols), SAVE_IMG_W, SAVE_IMG_H);
    if (!slots[n].valid) {
    	r = Rect(256-SAVE_IMG_W, 256-SAVE_IMG_H, SAVE_IMG_W, SAVE_IMG_H);
    }

    if (slot == n) {
		// there is a slight but very noticable delay when it is most zoomed out, this fixes that
        if (animationFrame % 111 == 0) {
            animationFrame = 115;
        }

     	//s16 delta = -COS[((animationFrame+33)<<2) & 511];
        //libnds degrees in circle changed from 512 to 32768, this is a quick fix
        int oldAngle = ((animationFrame+33) << 2) & 511;
        s32 delta = -cosLerp((oldAngle << 6) & (DEGREES_IN_CIRCLE-1));

    	if (scroll != 0) {
    		delta /= abs(scroll)>>3;
    	}

    	drawQuad(&texture2, inttof32(x - (delta>>10)), inttof32(y - (delta>>10)),
    			VERTEX_SCALE(SAVE_IMG_W + (delta>>9)),
    			VERTEX_SCALE(SAVE_IMG_H + (delta>>9)),
    			r);
    } else {
    	drawQuad(&texture2, inttof32(x), inttof32(y), vw, vh, r);
    }

}

void VNSaveMenu::DrawForeground() {
	Screen::DrawForeground();

    glPushMatrix();
    glTranslate3f32(inttof32(scroll-256*page), 0, 0);
    for (int n = pageMin; n <= pageMax; n++) {
    	DrawSlot(n);
    }
    glPopMatrix(1);

	if (scroll != 0) {
	    glPushMatrix();

	    int start;
    	if (scroll < 0) {
    		start = (pageMax + 1 < SAVE_SLOTS ? pageMax+1 : 0);
    		int p = start / SLOTS_PER_PAGE;
    		glTranslate3f32(inttof32(scroll+256-256*p), 0, 0);
    	} else {
    		start = (pageMin-SLOTS_PER_PAGE >= 0 ? pageMin-SLOTS_PER_PAGE : SAVE_SLOTS-SLOTS_PER_PAGE);
    		int p = start / SLOTS_PER_PAGE;
    		glTranslate3f32(inttof32(scroll-256-256*p), 0, 0);
    	}

		for (int n = start; n < start+SLOTS_PER_PAGE; n++) {
			DrawSlot(n);
		}
	    glPopMatrix(1);
    }

	if (scrollDir != 0) {
		scroll += (scrollDir<<3);
	}
	animationFrame++;
}
