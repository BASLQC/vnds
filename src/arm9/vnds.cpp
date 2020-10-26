#include "vnds.h"

#include "graphics_engine.h"
#include "res.h"
#include "script_engine.h"
#include "sound_engine.h"
#include "text_engine.h"
#include "vnchoice.h"
#include "maintextpane.h"
#include "tcommon/dsunzip.h"
#include "tcommon/gui/gui_common.h"

using namespace std;

VNDS::VNDS(NovelInfo* novelInfo) {
	quit = false;
	delay = 0;
	waitForInput = false;

	srand(time(NULL));

    gui = new GUI();
	gui->VideoInit();

	//Show loading image
	u16* temp = new u16[256*384];
	skinLoadImage("%s/loading_bg", temp, 256, 384);
	DC_FlushRange(temp, 256*384*2);
	dmaCopyHalfWordsAsynch(1, temp, gui->sub_bg, 256*192*2);
    dmaCopyHalfWordsAsynch(2, temp + 256*192, gui->main_bg, 256*192*2);
    while (dmaBusy(1) | dmaBusy(2));
    delete[] temp;

    //Init GUI and subsystems
    textEngine = new TextEngine(gui, this, novelInfo->GetFontSize());
    textEngine->SetBackgroundColor(skin->GetBackgroundColor());
    gui->PushScreen(textEngine);

    //Copy texture into VRAM, but don't save a copy in TextEngine
    u16* textureI = new u16[256*256];
    skinLoadImage("%s/ingame_tex", textureI, 256, 256);
    textEngine->SetTexture(textureI);
    textEngine->SetTexture(NULL);
    delete[] textureI;

    //Start
    chdir(novelInfo->GetPath());

    vnLog(EL_verbose, COM_CORE, "VNDS init ok");
}
VNDS::~VNDS() {
	if (gui) delete gui;

	chdir("../..");
}

void VNDS::Reset() {
	textEngine->Reset();
	graphicsEngine->Reset();
	scriptEngine->Reset();
	soundEngine->Reset();

	globals.clear();
	variables.clear();

	loadGlobal(this);
	waitForInput = false;
	delay = 0;
}

void VNDS::Idle() {

	//Preload the next png image
	#ifdef IMG_LOAD_PROFILE
		iprintf("\x1b[s\x1b[0;28H%s\x1b[u",
				(pngStream->IsImageReady() ? "100%" : "    "));
	#endif

	if (pngStream->IsStreaming()) {
		#ifdef IMG_LOAD_PROFILE
			if (pngStream->height > 0) {
				iprintf("\x1b[s\x1b[0;28H%3d%%\x1b[u", 100 * pngStream->line / pngStream->height);
			}
		#endif

		int t = 0;
		while (t < PNG_STREAM_LINES && pngStream->ReadLine()) {
			t++;
		}
	} else if (!pngStream->IsImageReady()) {
		for (int n = 0; n < PNG_STREAM_LINES; n++) {
			Command cmd = scriptEngine->GetCommand(n);
			if (cmd.id == SETIMG) {
			    char path[MAXPATHLEN];
			    sprintf(path, "foreground/%s", cmd.setimg.path);
			    if (!graphicsEngine->IsImageCached(path)) {
					pngStream->Start(foregroundArchive, path);
					break;
			    }
			}
		}
	}
}

bool VNDS::IsWaitingForInput() {
	return delay <= 0 && gui->GetActiveScreen() == textEngine
		&& !textEngine->GetChoiceView()->IsActive() && waitForInput
		&& textEngine->GetTextPane()->IsFullTextDisplayed();
}
int VNDS::GetDelay() {
	return delay;
}

void VNDS::Update() {
	if (gui->GetActiveScreen() != textEngine) {
		return;
	}

	u32 down = keysDown();
	u32 held = keysHeld();

    u32 anyKey = KEY_A|KEY_B|KEY_X|KEY_Y|KEY_START|KEY_SELECT|KEY_L|KEY_R|KEY_TOUCH;

	if (delay > 0) {
		delay--;
        if (down & anyKey) {
            delay = 0;
        }
		Idle();
	} else if (held & KEY_R) {
		//R held

		if (!textEngine->GetChoiceView()->IsActive()) {
			if (down & KEY_Y) {
			    int oldMode = (REG_DISPCNT);
			    const int fadeFrames = 8;

			    u16* loadingI = new u16[256*192];
			    bool imageOK = skinLoadImage("../../%s/skipload_bg", loadingI, 256, 192);
				if (imageOK) {
					memset(loadingI+(176<<8), 0, (16<<8)*2);
				    vramSetBankD(VRAM_D_LCD);
				    dmaCopy(loadingI, VRAM_D, 256*192*sizeof(u16));
					vramSetBankD(VRAM_D_MAIN_BG_0x06020000);

					REG_BG2CNT = BG_BMP16_256x256 | BG_BMP_BASE(8) | BG_PRIORITY(1);
					videoSetMode(DEFAULT_VIDEO_MODE | DISPLAY_BG2_ACTIVE);

					REG_BLDALPHA = 0 | (0x10 << 8);
					REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG0 | BLEND_DST_BG3;
				    for (int n = 0; n <= fadeFrames; n++) {
				        int a = 0x0B * n / fadeFrames;
				        int b = 0x10 - a;
				        REG_BLDALPHA = a | (b << 8);
				        swiWaitForVBlank();
				    }
				}
				delete[] loadingI;

				scriptEngine->QuickRead();

				if (imageOK) {
				    for (int n = 0; n <= fadeFrames; n++) {
				        int a = 0x10 - 0x0B * n / fadeFrames;
				        int b = 0x10 - a;
				        REG_BLDALPHA = a | (b << 8);
				        swiWaitForVBlank();
				    }
				    REG_BLDCNT = BLEND_NONE;

				    videoSetMode(oldMode);
				}
			} else if (down & KEY_A) {
				vnLog(EL_message, COM_CORE, "Heap Size: %d.%03d.%03d", HEAP_SIZE/1000000,
						(HEAP_SIZE/1000)%1000, HEAP_SIZE%1000);
			} else if (down & KEY_B) {
			    SaveScreenShot();
			}
		}

		if (down & KEY_X) {
			textEngine->ShowSaveMenu();
		}
	} else if (held & KEY_L) {
		//L held

		if (down & KEY_X) {
			textEngine->ShowLoadMenu();
		}
	} else {
		//No buttons held
		if (down & (KEY_X|KEY_START)) {
			textEngine->ShowMenu();
		} else if (gui->GetActiveScreen() == textEngine && !textEngine->GetChoiceView()->IsActive()) {
			if (!waitForInput) {
				Continue(held & KEY_Y);
			} else {
				if (!textEngine->GetTextPane()->IsFullTextDisplayed()) {
					if ((down & anyKey) || (held & KEY_Y)) {
						textEngine->GetTextPane()->DisplayFullText();
					}
				} else if ((down & (KEY_A|KEY_B)) || (held & KEY_Y)) {
					Continue(held & KEY_Y);
				}
			}
		} else {
			Idle();
		}
	}
}

void VNDS::Continue(bool quickread) {
	delay = 0;
	waitForInput = false;
	scriptEngine->ExecuteNextCommand(quickread);
}

void VNDS::Quit() {
	quit = true;
}

bool zipError;

void onZipProgress(int val, int max, const char* message) {
	if (max >= 0) {
		if (message) {
			int messageL = strlen(message);
			iprintf("\x1b[16;0H%*s%s%*s", (32-messageL+1)/2, " ", message, (32-messageL)/2, " ");
		} else {
			iprintf("\x1b[16;0H                               ");
		}
		if (max > 0) {
			iprintf("\x1b[17;14H%3d%%", 100*val/max);
		} else {
			iprintf("\x1b[17;0H                               ");
		}
	} else {
		zipError = true;

		consoleDemoInit();
		BG_PALETTE_SUB[0]   = BIT(15)|RGB15(0, 0, 31);
		BG_PALETTE_SUB[255] = BIT(15)|RGB15(31, 31, 31);

	    const char* warning =
	    "--------------------------------"
	    "           Fatal Error          "
	    "--------------------------------"
	    "                                "
		" The zip file contains a        "
		" compressed entry, but only     "
	    " uncompressed zip files are     "
	    " supported.                     "
	    "                                "
	    "--------------------------------";
	    iprintf(warning);

	    waitForAnyKey();
	}
}

void VNDS::Run() {
	quit = false;

	//Open archives
    consoleClear();
    zipError = false;
    foregroundArchive = openArchive("foreground", "foreground/", &onZipProgress);
    if (!zipError) {
    backgroundArchive = openArchive("background", "background/", &onZipProgress);
    if (!zipError) {
    scriptArchive = openArchive("script", "script/", &onZipProgress);
    if (!zipError) {
    soundArchive = openArchive("sound", "sound/", &onZipProgress);
    if (!zipError) {
    consoleClear();

    //Load font
    if (fexists("default.ttf")) {
    	textEngine->GetTextPane()->SetFont("default.ttf");
    	textEngine->GetChoiceView()->SetFont("default.ttf");
    	vnLog(EL_verbose, COM_TEXT, "Custom font (default.ttf) found.");
    }

    //Init state
    loadGlobal(this);

    scriptEngine = new ScriptEngine(this, scriptArchive);
    scriptEngine->SetScriptFile("script/main.scr");

    graphicsEngine = new GraphicsEngine(this, (u16*)BG_BMP_RAM_SUB(2), backgroundArchive, foregroundArchive);
    graphicsEngine->Flush(true);

    soundEngine = new SoundEngine(soundArchive);
	pngStream = new PNGStream(soundEngine);

	textEngine->Activate();

    //Main loop
	while (!quit) {
		gui->Update();
		Update();

		if (!quit) {
			gui->Draw();
			soundEngine->Update();
			swiWaitForVBlank();
		}
	}

	fadeBlack(16);

	//Cleanup
	delete scriptEngine;
	scriptEngine = NULL;

	delete graphicsEngine;
	graphicsEngine = NULL;

	delete soundEngine;
	soundEngine = NULL;

	delete pngStream;
	pngStream = NULL;

	//Close ZIP archives
	closeArchive(soundArchive);
	soundArchive = NULL;
	}
	closeArchive(scriptArchive);
	scriptArchive = NULL;
	}
	closeArchive(backgroundArchive);
	backgroundArchive = NULL;
	}
	closeArchive(foregroundArchive);
	foregroundArchive = NULL;
	}

	resetVideo();
}

void VNDS::SaveScreenShot() {
	vnLog(EL_message, COM_CORE, "Saving screenshot...");

	bool wasMuted = soundEngine->IsMuted();
	soundEngine->SetMuted(true);

	vramSetBankD(VRAM_D_LCD);
	setupCapture(3);
	gui->Draw();
	waitForCapture();

	u16* temp = new u16[256*384];
	memcpy(temp, gui->sub_bg, 256*192*2);
	memcpy(temp+256*192, VRAM_D, 256*192*2);

    char filename[32];
    time_t ti = time(NULL);
    tm* tm = localtime(&ti);
    unsigned int timestamp = tm->tm_sec + 60*(tm->tm_min + 60*(tm->tm_hour + 24*(tm->tm_yday + 366 * tm->tm_year)));
    sprintf(filename, "%u.png", timestamp);

    if (pngSaveImage(filename, temp, 256, 384)) {
		vnLog(EL_message, COM_CORE, "Screenshot saved successfully.");
	} else {
		vnLog(EL_message, COM_CORE, "Error saving screenshot.");
	}
	delete[] temp;

	soundEngine->SetMuted(wasMuted);
}

void VNDS::SetDelay(int d) {
	delay = d;
}
void VNDS::SetWaitForInput(bool b) {
	waitForInput = b;
}

void VNDS::SetVariable(const char* name, char op, const char* value) {
	SetVar(variables, name, op, value);
}
void VNDS::SetGlobal(const char* name, char op, const char* value) {
	SetVar(globals, name, op, value);
	saveGlobal(this);
}
void VNDS::SetVar(map<std::string, Variable>& map,
		const char* name, char op, const char* value)
{
	if (op == '~') {
		map.clear();
		return;
	}

    Variable var(value);
    if (variables.count(value) != 0) {
        var = variables[value];
    } else if (globals.count(value) != 0) {
        var = globals[value];
    }
	Variable target = map[name];

	VarType inferredType = VT_int;
	if (target.type == VT_string || var.type == VT_string) {
		inferredType = VT_string;
	}
	target.type = inferredType;

	switch (inferredType) {
	case VT_int:
		switch (op) {
		case '+':
			target.intval += var.intval;
			break;
		case '-':
			target.intval -= var.intval;
			break;
		case '=':
			target.intval = var.intval;
			break;
		default:
			vnLog(EL_warning, COM_CORE, "setvar :: Unsupported operator '%c' for target type int", op);
			return;
		}

		sprintf(target.strval, "%d", target.intval);
		break;
	case VT_string:
		int offset;
		switch (op) {
		case '+':
			offset = strlen(target.strval);
			strncpy(target.strval+offset, var.strval, VAR_STRING_LENGTH-offset-1);
			target.strval[VAR_STRING_LENGTH-1] = '\0';
			break;
		case '=':
			strncpy(target.strval, var.strval, VAR_STRING_LENGTH-1);
			target.strval[VAR_STRING_LENGTH-1] = '\0';
			break;
		default:
			vnLog(EL_warning, COM_CORE, "setvar :: Unsupported operator '%c' for target type string", op);
			return;
		}

		target.intval = atoi(target.strval);
		break;
	case VT_null:
		//Do nothing
		break;
	}

	vnLog(EL_verbose, COM_SCRIPT, "(g)setvar %s %c %s", name, op, target.strval);
	map[name] = target;
}
