#ifndef NOVELINFO_H
#define NOVELINFO_H

#include "common.h"

enum NovelType {
    FOLDER,
    NOVELZIP,
    DOWNLOAD
};

class NovelInfo {
	private:
		char path[MAXPATHLEN];
		char title[128];
		u8 fontsize;
        
        NovelType noveltype;
        Archive* nvl;

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
        NovelType GetNovelType();

		void SetPath(const char* p);
		void SetTitle(const char* t);
		void SetFontSize(u8 s);
        void SetNovelType(NovelType n);
        void SetArchive(Archive* a);
};

#endif
