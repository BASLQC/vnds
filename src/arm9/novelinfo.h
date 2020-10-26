#ifndef NOVELINFO_H
#define NOVELINFO_H

#include "common.h"

class NovelInfo {
	private:
		char path[MAXPATHLEN];
		char title[128];
		u8 fontsize;

		u16 thumbnail[100*75];
		u16 icon[32*32];
		bool thumbnailLoaded;
		bool iconLoaded;

	public:

		NovelInfo();
		virtual ~NovelInfo();

		const char* GetPath();
		const char* GetTitle();
		u8 GetFontSize();
		const u16* GetIcon();
		const u16* GetThumbnail();

		void SetPath(const char* p);
		void SetTitle(const char* t);
		void SetFontSize(u8 s);
};

#endif
