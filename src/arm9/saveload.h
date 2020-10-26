#ifndef SAVELOAD_H
#define SAVELOAD_H

#include "common.h"

#define SAVE_SLOTS 18
#define SAVE_IMG_W 64
#define SAVE_IMG_H 48

void saveDate(char* out); //Gets a string representation of the current date/time
bool loadState(VNDS* vnds, u16 slot);
bool saveState(VNDS* vnds, u16 slot);
bool loadGlobal(VNDS* vnds);
bool saveGlobal(VNDS* vnds);

#endif
