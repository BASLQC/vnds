#include "novelinfo.h"
#include "tcommon/filehandle.h"
#include "tcommon/png.h"

NovelInfo::NovelInfo() {
	iconLoaded = thumbnailLoaded = false;

	strcpy(path, "_error");
	strcpy(title, "nameless");
	fontsize = 0;
    iconLoaded = false;
    thumbnailLoaded = false;
    nvl = NULL;

	for (int n = 0; n < 32*32; n++) {
		icon[n] = BIT(15);
	}
	for (int n = 0; n < 100*75; n++) {
		thumbnail[n] = BIT(15);
	}
}
NovelInfo::~NovelInfo() {
}

const char* NovelInfo::GetPath() {
	return path;
}
const char* NovelInfo::GetTitle() {
	return title;
}
u8 NovelInfo::GetFontSize() {
	return fontsize;
}
const u16* NovelInfo::GetIcon() {
	if (!iconLoaded) {
        if (noveltype == NOVELZIP) {
            FileHandle* fh = fhOpen(nvl, "icon.png");
            char data[fh->length+1];
            fh->ReadFully(data);
		    pngLoadImage(data, fh->length, icon, NULL, 32, 32);
            fhClose(fh);
        }
        else {
            char path[MAXPATHLEN];
		    sprintf(path, "%s/icon", GetPath());
		    loadImage(path, icon, NULL, 32, 32);
        }
		iconLoaded = true;
	}
	return icon;
}
const u16* NovelInfo::GetThumbnail() {
	if (!thumbnailLoaded) {
        if (noveltype == NOVELZIP) {
            FileHandle* fh = fhOpen(nvl, "thumbnail.png");
            char* data = new char[fh->length+1];
            fh->ReadFully(data);
		    pngLoadImage(data, fh->length, thumbnail, NULL, 100, 75);
            delete[] data;
            fhClose(fh);
        }
        else {
            char path[MAXPATHLEN];
            sprintf(path, "%s/thumbnail", GetPath());
            loadImage(path, thumbnail, NULL, 100, 75);
        }
        thumbnailLoaded = true;
	}
	return thumbnail;
}
NovelType NovelInfo::GetNovelType() {
    return noveltype;
}

void NovelInfo::SetPath(const char* p) {
	strncpy(path, p, MAXPATHLEN-1);
	path[MAXPATHLEN-1] = '\0';
}
void NovelInfo::SetTitle(const char* t) {
	strncpy(title, t, 127);
	title[127] = '\0';
}
void NovelInfo::SetFontSize(u8 s) {
	fontsize = s;
}
void NovelInfo::SetNovelType(NovelType n) {
    noveltype = n;
}
void NovelInfo::SetArchive(Archive* a) {
    nvl = a;
}
