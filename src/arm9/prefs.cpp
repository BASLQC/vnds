#include "prefs.h"
#include "vnds.h"
#include "text_engine.h"
#include "sound_engine.h"
#include "vnchoice.h"
#include "maintextpane.h"
#include "tcommon/parser/ini_parser.h"

char skinPath[128] = "skins/default";
char baseSkinPath[128] = "skins/default";

Skin* skin = new Skin();
Preferences* preferences = new Preferences();

bool skinLoadImage(const char* format, u16* out, int w, int h) {
	char path[MAXPATHLEN];

	sprintf(path, format, skinPath);
	if (loadImage(path, out, NULL, w, h, 0) || loadImage(path, out, NULL, w, h, GL_RGBA)) {
		return true;
	}

	sprintf(path, format, baseSkinPath);
	if (loadImage(path, out, NULL, w, h, 0) || loadImage(path, out, NULL, w, h, GL_RGBA)) {
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------

Skin::Skin() {
	selectFontSize   = 12;
	aboutFontSize    = 12;
	savemenuFontSize = 8;
	labelFontSize    = 12;

	backgroundColor  = BIT(15)|RGB15(00,00,00);
	textColor        = BIT(15)|RGB15(31,31,31);
	activeTextColor  = BIT(15)|RGB15(17,22,31);
}
Skin::~Skin() {

}

u16 string2color(const char* str) {
	int r, g, b;
	sscanf(str, "%d,%d,%d", &r, &g, &b);
	return RGB15(r&31, g&31, b&31)|BIT(15);
}

bool Skin::Load(const char* file) {
	char path[MAXPATHLEN];
	sprintf(path, "%s/skin.ini", file);

	IniFile iniFile;
	if (iniFile.Load(path)) {
        IniRecord* r;

        r = iniFile.GetRecord("version");
        if (r) {
        	if (strcmp(r->AsString(), "1.0") > 0) {
        		//skin version too new
        		vnLog(EL_error, COM_CORE, "Skin version is too new: %s", r->AsString());
        		return false;
        	}
        }

        r = iniFile.GetRecord("selectFontSize");
        if (r) selectFontSize = r->AsInt();
        r = iniFile.GetRecord("aboutFontSize");
        if (r) aboutFontSize = r->AsInt();
        r = iniFile.GetRecord("savemenuFontSize");
        if (r) savemenuFontSize = r->AsInt();
        r = iniFile.GetRecord("labelFontSize");
        if (r) labelFontSize = r->AsInt();

        r = iniFile.GetRecord("backgroundColor");
        if (r) backgroundColor = string2color(r->AsString());
        r = iniFile.GetRecord("textColor");
        if (r) textColor = string2color(r->AsString());
        r = iniFile.GetRecord("activeTextColor");
        if (r) activeTextColor = string2color(r->AsString());
	} else {
		return false;
	}

	strcpy(skinPath, file);
	return true;
}

u8 Skin::GetSelectFontSize() {
	return selectFontSize;
}
u8 Skin::GetAboutFontSize() {
	return aboutFontSize;
}
u8 Skin::GetSavemenuFontSize() {
	return savemenuFontSize;
}
u8 Skin::GetLabelFontSize() {
	return labelFontSize;
}

u16 Skin::GetBackgroundColor() {
	return backgroundColor;
}
u16 Skin::GetTextColor() {
	return textColor;
}
u16 Skin::GetActiveTextColor() {
	return activeTextColor;
}

//------------------------------------------------------------------------------

Preferences::Preferences() {
	fontSize = 12;
	textSpeed = 2;
	soundVolume = 127;
	musicVolume = 64;
	debugLevel = EL_warning;
	waitInputAnim = 1;
}
Preferences::~Preferences() {
}

bool Preferences::Load(const char* path) {
	IniFile iniFile;
	if (iniFile.Load(path)) {
        IniRecord* r;

        r = iniFile.GetRecord("skin");
        if (r) {
        	char path[MAXPATHLEN];
        	sprintf(path, "skins/%s", r->AsString());
        	skin->Load(path);
        }

        r = iniFile.GetRecord("fontsize");
        if (r) fontSize = MIN(24, MAX(6, r->AsInt()));
        r = iniFile.GetRecord("textspeed");
        if (r) {
        	textSpeed = r->AsInt();
        	if (textSpeed <= 0) textSpeed = -1;
        }
        r = iniFile.GetRecord("soundvolume");
        if (r) soundVolume = MIN(127, MAX(0, r->AsInt()));
        r = iniFile.GetRecord("musicvolume");
        if (r) musicVolume = MIN(127, MAX(0, r->AsInt()));

        r = iniFile.GetRecord("debuglevel");
        if (r) debugLevel = StringToErrorLevel(r->AsString());

        r = iniFile.GetRecord("waitinputanim");
        if (r) waitInputAnim = r->AsInt();
	} else {
		return false;
	}

	return true;
}

ErrorLevel Preferences::StringToErrorLevel(const char* str) {
	ErrorLevel result = EL_warning;

	if (strcmp(str, "error") == 0) {
		result = EL_error;
	} else if (strcmp(str, "warning") == 0) {
		result = EL_warning;
	} else if (strcmp(str, "missing") == 0) {
		result = EL_missing;
	} else if (strcmp(str, "verbose") == 0) {
		result = EL_verbose;
	}

	return result;
}
void Preferences::ErrorLevelToString(char* out, ErrorLevel el) {
	switch (el) {
	case EL_verbose: strcpy(out, "verbose"); return;
	case EL_missing: strcpy(out, "missing"); return;
	case EL_warning: strcpy(out, "warning"); return;
	case EL_error:   strcpy(out, "error");   return;
	case EL_message: strcpy(out, "message"); return;
	}

	strcpy(out, "");
}


bool Preferences::Save(const char* path) {
	char temp[256];

	IniFile iniFile;
	iniFile.Load(path); //If the file exists, add old values first, then overwrite/add new values

	sprintf(temp, "%d", fontSize);
	iniFile.SetRecord("fontsize", temp);
	sprintf(temp, "%d", (textSpeed <= 0 ? 0 : textSpeed));
	iniFile.SetRecord("textspeed", temp);
	sprintf(temp, "%d", soundVolume);
	iniFile.SetRecord("soundvolume", temp);
	sprintf(temp, "%d", musicVolume);
	iniFile.SetRecord("musicvolume", temp);
	ErrorLevelToString(temp, debugLevel);
	iniFile.SetRecord("debuglevel", temp);
	sprintf(temp, "%d", waitInputAnim);
	iniFile.SetRecord("waitinputanim", temp);

	return iniFile.Save(path);
}

s8 Preferences::GetTextSpeed() {
	return textSpeed;
}
u8 Preferences::GetFontSize() {
	return fontSize;
}
u8 Preferences::GetSoundVolume() {
	return soundVolume;
}
u8 Preferences::GetMusicVolume() {
	return musicVolume;
}
ErrorLevel Preferences::GetDebugLevel() {
	return debugLevel;
}
u8 Preferences::GetWaitInputAnim() {
	return waitInputAnim;
}

void Preferences::SetTextSpeed(s8 s, VNDS* vnds) {
	textSpeed = s;
	if (vnds) {
		vnds->textEngine->GetTextPane()->SetTextSpeed(s);
	}
}
void Preferences::SetFontSize(u8 s, VNDS* vnds) {
	fontSize = s;
	if (vnds) {
		vnds->textEngine->GetChoiceView()->SetFontSize(s, true);
		vnds->textEngine->GetTextPane()->SetFontSize(s, true);
	}
}
void Preferences::SetSoundVolume(u8 v, VNDS* vnds) {
	soundVolume = v;
	if (vnds) {
		//Don't change the volume for currently playing sounds, just future ones.
		//vnds->soundEngine->SetSoundVolume(v);
	}
}
void Preferences::SetMusicVolume(u8 v, VNDS* vnds) {
	musicVolume = v;
	if (vnds) {
		vnds->soundEngine->SetMusicVolume(v);
	}
}
void Preferences::SetDebugLevel(ErrorLevel el) {
	debugLevel = el;
}

//------------------------------------------------------------------------------
