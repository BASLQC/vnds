#include <nds.h>
#include <fat.h>
#include <unistd.h>
#include <sys/dir.h>

#include "common.h"
#include "as_lib9.h"
#include "vnds_types.h"
#include "vnds.h"
#include "select/vnselect.h"
#include "download/vndownload.h"
#include "tcommon/text.h"
#include "tcommon/xml_parser.h"

#ifdef USE_EFS
#include "efs_lib.h"
#endif

void epicFail() {
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);

    consoleDemoInit();

    printf("\x1b[0;0H");
    BG_PALETTE_SUB[0]   = RGB15( 0,  0, 31) | BIT(15);
    BG_PALETTE_SUB[255] = RGB15(31, 31, 31) | BIT(15);
}

void libfatFail() {
    epicFail();

    const char* warning =
    "--------------------------------"
    "         libfat failure         "
    "--------------------------------"
    "                                "
    " An error is preventing the     "
    " game from accessing external   "
    " files.                         "
    "                                "
    " If you're using an emulator,   "
    " make sure it supports DLDI     "
    " and that it's activated.       "
    "                                "
    " In case you're seeing this     "
    " screen on a real DS, make sure "
    " you've applied the correct     "
    " DLDI patch (most modern        "
    " flashcards do this             "
    " automatically)                 "
    "                                "
    " Note: Some cards only          "
    " autopatch .nds files stored in "
    " the root folder of the card.   "
    "                                "
    "--------------------------------";
    iprintf(warning);

    while (true) {
        swiWaitForVBlank();
    }
}

void installFail() {
    epicFail();

    const char* warning =
    "--------------------------------"
    "      Invalid Installation      "
    "--------------------------------"
    "                                "
    " Please make sure the game data "
    " is stored in /vnds/            "
    "                                "
    "--------------------------------";
    iprintf(warning);

    while (true) {
        swiWaitForVBlank();
    }
}
void checkInstall() {
	if (!fexists(DEFAULT_FONT_PATH)) {
        installFail();
	}
}

inline u32 readFIFO() {
    while (REG_IPC_FIFO_CR & IPC_FIFO_RECV_EMPTY);
    return REG_IPC_FIFO_RX;
}

int main(int argc, char** argv) {
	powerOn(POWER_ALL);
    defaultExceptionHandler();

    fifoSetValue32Handler(FIFO_BACKLIGHT, &FIFOBacklight, NULL);

    //Init filesystem
	//NOTE: EFS started misbehaving when the .nds grew to over 32MB
    #ifdef USE_EFS
    if (!EFS_Init(EFS_AND_FAT|EFS_DEFAULT_DEVICE, NULL)) {
    #endif
        //If that fails, init regular FAT
        if (!fatInitDefault()) {
            libfatFail();
            return 1;
        }
        chdir("/vnds");
    #ifdef USE_EFS
    }
    #endif

    //Check installation
    checkInstall();

    //Init default font
    createDefaultFontCache(DEFAULT_FONT_PATH);

    //Timer0 + Timer1 are used by as_lib
    //Timer2 is for AAC
    //Timer3 is for Wifi
    InitAACPlayer();

    //Init ASLib
	AS_Init(AS_MODE_MP3, 24*1024);
    AS_SetDefaultSettings(AS_ADPCM, 22050, 0);

    Wifi_InitDefault(INIT_ONLY);

    //Create log
    vnInitLog(0);

    NovelInfo* selectedNovelInfo = new NovelInfo();
    fadeBlack(1);
	while (true) {
		if (!preferences->Load("config.ini")) {
			skin->Load("skins/default");
		}
		strcpy(baseSkinPath, skinPath);

		VNSelect* vnselect = new VNSelect();
		vnselect->Run();
        if (vnselect->selectedNovelInfo == NULL) {
            delete vnselect;
            
            VNDownload* vndownload = new VNDownload();
            vndownload->Run();
            delete vndownload;
            
            continue;
        }
        
		memcpy(selectedNovelInfo, vnselect->selectedNovelInfo, sizeof(NovelInfo));
		delete vnselect;

		{
			char path[MAXPATHLEN];
			sprintf(path, "%s/skin", selectedNovelInfo->GetPath());
			skin->Load(path);
		}

        VNDS* vnds = new VNDS(selectedNovelInfo);
        vnds->Run();
        delete vnds;
	}
	delete selectedNovelInfo;

    vnDestroyLog();
    destroyDefaultFontCache();
    return 0;
}
