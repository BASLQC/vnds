#include "saveload.h"

#include <time.h>
#include "vnds.h"
#include "graphics_engine.h"
#include "script_engine.h"
#include "sound_engine.h"

using namespace std;

void loadVars(map<string, Variable>& varMap, XMLNode& node) {
    int L = node.nChildNode("var");
    for (int n = 0; n < L; n++) {
        char type[5];

        XMLNode varE = node.getChildNode("var", n);
        if (varE.getAttribute("type")) {
			strncpy(type, varE.getAttribute("type"), 4);
			type[4] = '\0';
        } else {
        	type[0] = '\0';
        }

        Variable var;

        if (varE.getAttribute("value")) {
			strncpy(var.strval, varE.getAttribute("value"), VAR_STRING_LENGTH-1);
			var.strval[VAR_STRING_LENGTH-1] = '\0';
			var.intval = atoi(var.strval);
        } else {
        	var.strval[0] = '\0';
        	var.intval = 0;
        }

        if (strcmp(type, "int") == 0) {
            var.type = VT_int;
        } else if (strcmp(type, "str") == 0) {
            var.type = VT_string;
        } else {
        	vnLog(EL_warning, COM_CORE, "Unable to load unknown type of variable: \"%s\", type");
        	continue;
        }

        varMap[varE.getAttribute("name")] = var;
    }
}
void saveVars(FILE* file, map<string, Variable>& varMap, u8 indent) {
	char s[indent+1];
	memset(s, ' ', indent);
	s[indent] = '\0';

    map<string, Variable>::iterator iter;
    for (iter = varMap.begin(); iter != varMap.end(); iter++) {
    	const char* name = iter->first.c_str();
    	Variable var = iter->second;

        if (var.type == VT_string) {
            fprintf(file, "%s<var name=\"%s\" type=\"str\" value=\"%s\" />\n", s, name, var.strval);
        } else if (var.type == VT_int) {
            fprintf(file, "%s<var name=\"%s\" type=\"int\" value=\"%d\" />\n", s, name, var.intval);
        } else {
        	vnLog(EL_warning, COM_CORE, "Can't save this unknown type of variable: %d", var.type);
        	continue;
        }
    }
}

bool loadState(VNDS* vnds, u16 slot) {
    char path[MAXPATHLEN];
    sprintf(path, "save/save%.2d.sav", slot);
    if (!fexists(path)) {
    	vnLog(EL_error, COM_CORE, "Error loading savefile \"%s\". File not found.", path);
        return false;
    }

    vnds->Reset();

    //Load XML
    XMLNode rootE = XMLNode::openFileHelper(path, "save");

    //Variables
    XMLNode variablesE = rootE.getChildNode("variables");
    loadVars(vnds->variables, variablesE);

    //Script Engine
    XMLNode scriptE = rootE.getChildNode("script");
    const char* scriptPath = scriptE.getChildNode("file").getText();
    u32 textSkip = 0;
    if (scriptE.getChildNode("position").getText()) {
    	textSkip = atoi(scriptE.getChildNode("position").getText());
    }

    vnds->scriptEngine->SetScriptFile(scriptPath ? scriptPath : "script/main.scr");
    vnds->scriptEngine->SkipTextCommands(textSkip);

    if (rootE.nChildNode("state") == 0) {
    	//Save file doesn't contain state
        return true;
    }

    XMLNode stateE = rootE.getChildNode("state");

    if (stateE.getChildNode("background").getText()) {
    	vnds->graphicsEngine->SetBackground(stateE.getChildNode("background").getText());
    }

    XMLNode spritesE = stateE.getChildNode("sprites");
    int spritesEChildren = spritesE.nChildNode("sprite");
    for (int n = 0; n < spritesEChildren; n++) {
    	int x = 0;
    	int y = 0;

        XMLNode spriteE = spritesE.getChildNode("sprite", n);
        if (spriteE.getAttribute("x")) x = atoi(spriteE.getAttribute("x"));
        if (spriteE.getAttribute("y")) y = atoi(spriteE.getAttribute("y"));
        const char* path = spriteE.getAttribute("path");

        if (path) {
        	vnds->graphicsEngine->SetForeground(path, x, y);
        }
    }

    vnds->graphicsEngine->Flush(true);
    if (stateE.getChildNode("music").getText()) {
    	vnds->soundEngine->SetMusic(stateE.getChildNode("music").getText());
    }

    return true;
}

void saveDate(char* out) {
	time_t ti = time(NULL);
	tm* t = localtime(&ti);

	int year = t->tm_year + 1900;
	int month = t->tm_mon + 1;
	int day = t->tm_mday;
	int hour = t->tm_hour;
	int min = t->tm_min;

	sprintf(out, "%.2d:%.2d %d/%.2d/%.2d", hour, min, year, month, day);
}
bool saveXml(VNDS* vnds, u16 slot) {
    char buffer[256];

    const char* scriptFile = vnds->scriptEngine->GetOpenFile();
    if (!scriptFile) {
    	vnLog(EL_error, COM_CORE, "Unable to save the game, current script's filename is NULL");
    	return false;
    }

    //Open file
    sprintf(buffer, "save/save%.2d.sav", slot);
    FILE* file = fopen(buffer, "wb");
    if (!file) {
		vnLog(EL_error, COM_CORE, "Error opening file \"%s\" for saving", buffer);
    	return false;
    }
    fprintf(file, "<save>\n");

    //ScripEngine state
    u32 textSkip = vnds->scriptEngine->GetTextSkip() - 1;
    fprintf(file, "  <script><file>%s</file><position>%d</position></script>\n", scriptFile, textSkip);

	//Date
	saveDate(buffer);
	fprintf(file, "  <date>%s</date>\n", buffer);

	//Variables
    fprintf(file, "  <variables>\n");
	saveVars(file, vnds->variables, 4);
    fprintf(file, "  </variables>\n");

    //Begin Other State
    fprintf(file, "  <state>\n");
    {
		//Music
		const char* musicPath = vnds->soundEngine->GetMusicPath();
		fprintf(file, "    <music>%s</music>\n", musicPath ? musicPath : "");

		//Background
		const char* backgroundPath = vnds->graphicsEngine->GetBackgroundPath();
		fprintf(file, "    <background>%s</background>\n", backgroundPath ? backgroundPath : "");

		//Sprites
		fprintf(file, "    <sprites>\n");
		const VNSprite* sprites = vnds->graphicsEngine->GetSprites();
		int spritesL = vnds->graphicsEngine->GetNumberOfSprites();
		for (int n = 0; n < spritesL; n++) {
			fprintf(file, "      <sprite path=\"%s\" x=\"%d\" y=\"%d\"/>\n",
					sprites[n].path, sprites[n].x, sprites[n].y);
		}
		fprintf(file, "    </sprites>\n");
    }
    //End Other State
    fprintf(file, "  </state>\n");

    fprintf(file, "</save>\n");
    fclose(file);
    return true;
}

void saveImage(VNDS* vnds, u16 slot) {
	char* path = new char[MAXPATHLEN];
	sprintf(path, "save/save%.2d.img", slot);
	FILE* file = fopen(path, "wb");
	delete[] path;

    if (!file) {
    	vnLog(EL_warning, COM_CORE, "Error while saving save file thumbnail");
    	return;
    }

	u16* rgb = new u16[SAVE_IMG_W * SAVE_IMG_H];
	u16* screen = vnds->graphicsEngine->buffer;

	int t = 0;
	for (int y = 0; y < SAVE_IMG_H; y++) {
		int y0 = (y  ) * 192 / SAVE_IMG_H;
		int y1 = (y+1) * 192 / SAVE_IMG_H;

		for (int x = 0; x < SAVE_IMG_W; x++) {
			int x0 = ((x  )<<8) / SAVE_IMG_W;
			int x1 = ((x+1)<<8) / SAVE_IMG_W;

			int div = (y1-y0) * (x1-x0);

			int sumR = 0, sumG = 0, sumB = 0;
			for (int r = y0; r < y1; r++) {
				int s = (r << 8) + x0;
				for (int c = x0; c < x1; c++) {
					sumR += (screen[s]    ) & 31;
					sumG += (screen[s]>> 5) & 31;
					sumB += (screen[s]>>10) & 31;
					s++;
				}
			}
			rgb[t] = RGB15(sumR/div, sumG/div, sumB/div) | BIT(15);
			t++;
		}
	}

	fwrite(rgb, sizeof(u16), SAVE_IMG_W * SAVE_IMG_H, file);
	fclose(file);

	delete[] rgb;
}

bool saveState(VNDS* vnds, u16 slot) {
	if (!mkdirs("save")) {
		vnLog(EL_error, COM_CORE, "Error creating save folder");
		return false;
	}

	if (!saveXml(vnds, slot)) {
		vnLog(EL_error, COM_CORE, "Save failed");
		return false;
	}

	saveImage(vnds, slot);
	return true;
}

bool loadGlobal(VNDS* vnds) {
    const char* path = "save/global.sav";
    if (!fexists(path)) {
    	vnLog(EL_verbose, COM_CORE, "global.sav doesn't exist");
    	return true; //File not found is OK for global.sav
    }

    XMLNode rootE = XMLNode::openFileHelper(path, "global");
    loadVars(vnds->globals, rootE);
    return true;
}
bool saveGlobal(VNDS* vnds) {
    if (!mkdirs("save")) {
		vnLog(EL_error, COM_CORE, "Error creating save folder");
		return false;
	}

    const char* path = "save/global.sav";
    FILE* file = fopen(path, "wb");
    if (!file) {
		vnLog(EL_error, COM_CORE, "Error opening file \"%s\" for saving", path);
    	return false;
    }

    fprintf(file, "<global>\n");
    saveVars(file, vnds->globals, 2);
    fprintf(file, "</global>\n");

    fclose(file);
    return true;
}
