#ifndef PREFS_H
#define PREFS_H

#include "common.h"
#include "vnlog.h"

#define DEFAULT_FONT_PATH "_default.ttf"

class Skin {
	private:
		u8  selectFontSize;
		u8  aboutFontSize;
		u8  savemenuFontSize;
		u8  labelFontSize;

		u16 backgroundColor;
		u16 textColor;
		u16 activeTextColor;

	public:
		Skin();
		virtual ~Skin();

		bool Load(const char* name);

		u8 GetSelectFontSize();
		u8 GetAboutFontSize();
		u8 GetSavemenuFontSize();
		u8 GetLabelFontSize();

		u16 GetBackgroundColor();
		u16 GetTextColor();
		u16 GetActiveTextColor();

};

class Preferences {
	private:
		s8 textSpeed;
		u8 fontSize;
		u8 soundVolume;
		u8 musicVolume;
		ErrorLevel debugLevel;
		u8 waitInputAnim;

		ErrorLevel StringToErrorLevel(const char* str);
		void ErrorLevelToString(char* out, ErrorLevel el);

	public:
		Preferences();
		virtual ~Preferences();

		bool Load(const char* path);
		bool Save(const char* path);

		s8   GetTextSpeed();
		u8   GetFontSize();
		u8   GetSoundVolume();
		u8   GetMusicVolume();
		ErrorLevel GetDebugLevel();
		u8   GetWaitInputAnim();

		void SetTextSpeed(s8 s, VNDS* vnds=NULL);
		void SetFontSize(u8 s, VNDS* vnds=NULL);
		void SetSoundVolume(u8 v, VNDS* vnds=NULL);
		void SetMusicVolume(u8 v, VNDS* vnds=NULL);
		void SetDebugLevel(ErrorLevel el);

};

extern char skinPath[128];
extern char baseSkinPath[128];
extern Skin* skin;
extern Preferences* preferences;

bool skinLoadImage(const char* format, u16* out, int w, int h);

#endif
