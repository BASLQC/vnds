#include "novelinfo.h"

NovelInfo::NovelInfo() {
	iconLoaded = thumbnailLoaded = false;

	strcpy(path, "_error");
	strcpy(title, "nameless");
	fontsize = 0;

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
		char path[MAXPATHLEN];
		sprintf(path, "%s/icon", GetPath());
		loadImage(path, icon, NULL, 32, 32);
		iconLoaded = true;
	}
	return icon;
}
const u16* NovelInfo::GetThumbnail() {
	if (!thumbnailLoaded) {
		char path[MAXPATHLEN];
		sprintf(path, "%s/thumbnail", GetPath());
		loadImage(path, thumbnail, NULL, 100, 75);
		thumbnailLoaded = true;
	}
	return thumbnail;
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
