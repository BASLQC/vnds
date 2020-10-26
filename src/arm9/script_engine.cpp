#include "script_engine.h"
#include "graphics_engine.h"
#include "script_interpreter.h"
#include "sound_engine.h"
#include "text_engine.h"
#include "vnds.h"
#include "maintextpane.h"
#include "tcommon/filehandle.h"

using namespace std;

#define READ_BUFFER_SIZE 1024

//------------------------------------------------------------------------------

Variable::Variable(const char* value) {
    type = VT_null;
    intval = 0;
    strval[0] = '\0';

	if (value && value[0] != '\0') {
        if (value[0] == '"' && value[strlen(value)-1] == '"') {
            strncpy(strval, value+1, strlen(value)-2);
			strval[strlen(value)-2] = '\0';
            type = VT_string;
        }
        else if (isdigit(value[0]) || value[0] == '-') {
            intval = atoi(value);
			sprintf(strval, "%d", intval); //Gets rid prefixed zeroes
            type = VT_int;
        }
        else {
            type = VT_int;
            intval = 0;
            strval[0] = '0';
            strval[1] = '\0';
        }
	}
}
Variable::~Variable() {
}

bool Variable::operator== (const Variable& v) const {
	if (type == VT_int && v.type == VT_int) {
		return intval == v.intval;
	}
	return strcmp(strval, v.strval) == 0;
}
bool Variable::operator!= (const Variable& v) const {
	return !(*this == v);
}
bool Variable::operator>  (const Variable& v) const {
	if (type == VT_int && v.type == VT_int) {
		return intval > v.intval;
	}
	return strcmp(strval, v.strval) > 0;
}
bool Variable::operator<  (const Variable& v) const {
	if (type == VT_int && v.type == VT_int) {
		return intval < v.intval;
	}
	return strcmp(strval, v.strval) < 0;
}
bool Variable::operator>= (const Variable& v) const {
	return !(*this < v);
}
bool Variable::operator<= (const Variable& v) const {
	return !(*this > v);
}

//------------------------------------------------------------------------------

ScriptEngine::ScriptEngine(VNDS* vnds, Archive* a) {
	this->vnds = vnds;
	interpreter = new ScriptInterpreter(vnds);

	archive = a;
	file = NULL;

	Reset();
}
ScriptEngine::~ScriptEngine() {
	if (file) {
		fhClose(file);
	}

	delete interpreter;
}

void ScriptEngine::Reset() {
	if (file) fhClose(file);

	file = NULL;
	filePath[0] = '\0';
	fileLine = 0;
	textSkip = 0;

	eof = false;
	readBuffer[0] = '\0';
	readBufferL = 0;
	readBufferOffset = 0;

	eofCommand.id = END_OF_FILE;

	commands.clear();
}
void ScriptEngine::SetScriptFile(const char* filename) {
	if (file) {
		fhClose(file);
	}

	eof = false;
	readBuffer[0] = '\0';
	readBufferL = 0;
	readBufferOffset = 0;

	commands.clear();

	strcpy(filePath, filename);
	fileLine = 0;
	textSkip = 0;

	file = fhOpen(archive, filePath);
	if (!file) {
		vnLog(EL_error, COM_SCRIPT, "Unable to open script file: %s\nPress A to continue.", filename);
		vnds->SetWaitForInput(true);
		return;
	}

	vnLog(EL_verbose, COM_SCRIPT, "Set script: %s", filePath);

	//Read first chunk
	readBufferOffset = 0;
	readBufferL = file->Read(readBuffer, SCRIPT_READ_BUFFER_SIZE);

	if (readBufferL >= 2 && readBuffer[0]==0xFE && readBuffer[1]==0xFF) {
		vnLog(EL_error, COM_SCRIPT, "Script encoding is UTF-16 BE. Only UTF-8 is supported.");
		fhClose(file); file = NULL;
		return;
	} else if (readBufferL >= 2 && readBuffer[0]==0xFF && readBuffer[1]==0xFE) {
		vnLog(EL_error, COM_SCRIPT, "Script encoding is UTF-16 LE. Only UTF-8 is supported.");
		fhClose(file); file = NULL;
		return;
	} else if (readBufferL >= 3 && readBuffer[0]==0xEF && readBuffer[1]==0xBB &&  readBuffer[2]==0xBF) {
		vnLog(EL_verbose, COM_SCRIPT, "Script starts with a UTF-8 BOM.");
		readBufferOffset += 3;
	}
}

void ScriptEngine::ExecuteNextCommand(bool quickread) {
	if (commands.size() == 0) {
		ReadNextCommand();
	}

	Command c = commands.front();
	commands.pop_front();

	fileLine++;
	if (c.id == TEXT) {
		textSkip++;
	}
	interpreter->Execute(&c, quickread);
}

void ScriptEngine::QuickRead() {
	bool quit = false;

	//Deactivate text
	bool wasTransparent = vnds->textEngine->GetTextPane()->IsTransparent();
	vnds->textEngine->GetTextPane()->SetTransparent(true);

	//Deactivate sound
	bool wasMuted = vnds->soundEngine->IsMuted();
	vnds->soundEngine->StopSound(); //Stop currently playing sound effects
	vnds->soundEngine->SetMuted(true);

	//Graphics stuff
	char* bgpath = new char[MAXPATHLEN];
	char* fgpath[GE_MAX_SPRITES];
	int  fgx[GE_MAX_SPRITES];
	int  fgy[GE_MAX_SPRITES];
	int  fgL;

	bgpath[0] = '\0';
	for (int n = 0; n < GE_MAX_SPRITES; n++) {
		fgpath[n] = new char[MAXPATHLEN];
	}
	fgL = 0;

	//Handle commands
	do {
		if (commands.size() == 0) {
			ReadNextCommand();
		}
		Command command = commands.front();
		commands.pop_front();
		fileLine++;

		if ((fileLine & 31) == 0) {
			vnds->soundEngine->Update();
		}

		switch (command.id) {
		case BGLOAD:
			sprintf(bgpath, "background/%s", command.bgload.path);
			fgL = 0;
			break;
		case SETIMG:
			if (fgL >= GE_MAX_SPRITES) {
		    	vnLog(EL_warning, COM_GRAPHICS, "Maximum number of sprites reached");
			} else {
				sprintf(fgpath[fgL], "foreground/%s", command.setimg.path);
				fgx[fgL] = command.setimg.x;
				fgy[fgL] = command.setimg.y;
				fgL++;
			}
			break;
		case TEXT:
			textSkip++;
			interpreter->cmd_text(&command, true, true);
			break;

		case SOUND:
		case MUSIC:
		case SKIP:
		case SETVAR:
		case GSETVAR:
		case IF:
		case FI:
		case DELAY:
		case RANDOM:
		case GOTO:
		case CLEARTEXT:
		case LABEL:
			interpreter->Execute(&command, true);
			break;

		case CHOICE:
		case JUMP:
		case ENDSCRIPT:
		case END_OF_FILE:
			interpreter->Execute(&command, true);
			quit = true;
			break;
		}
	} while (!quit);

	//Flush graphics
	if (bgpath[0] != '\0') {
		vnds->graphicsEngine->SetBackground(bgpath);
	}
	for (int n = 0; n < fgL; n++) {
		vnds->graphicsEngine->SetForeground(fgpath[n], fgx[n], fgy[n]);
	}
	vnds->graphicsEngine->Flush(true);

	delete bgpath;
	for (int n = 0; n < GE_MAX_SPRITES; n++) {
		delete[] fgpath[n];
	}

	//Reactivate sound
	vnds->soundEngine->SetMuted(wasMuted);

	//Reactivate text
	vnds->textEngine->GetTextPane()->SetTransparent(wasTransparent);

	//Always autocontinue
	vnds->SetWaitForInput(false);
}

Command ScriptEngine::GetCommand(u32 offset) {
	while (commands.size() <= offset) {
		ReadNextCommand();
	}
	return commands[offset];
}

void ScriptEngine::ReadNextCommand() {
	if (eof || !file) {
		commands.push_back(eofCommand);
		return;
	}

	char buffer[READ_BUFFER_SIZE];
	const int maxRead = READ_BUFFER_SIZE-1;

	int t = 0;
	bool overflow = false;
	while (t < maxRead) {
		if (readBufferOffset >= readBufferL) {
			readBufferOffset = 0;
			readBufferL = file->Read(readBuffer, SCRIPT_READ_BUFFER_SIZE);
            if (readBufferL <= 0) {
            	eof = true;
                break;
            }
		}

		char* newlineIndex = (char*)memchr(readBuffer+readBufferOffset, '\n', readBufferL-readBufferOffset);
		int wantToCopy;
		if (!newlineIndex) {
			wantToCopy = readBufferL-readBufferOffset;
		} else {
			wantToCopy = (newlineIndex-readBuffer)-readBufferOffset;
		}

		int copyL = MIN(maxRead-t, wantToCopy);
		memcpy(buffer+t, readBuffer+readBufferOffset, copyL);
		t += copyL;
		readBufferOffset += copyL;

		if (wantToCopy > copyL) {
			t = 0;
			overflow = true;
		} else {
			if (newlineIndex) {
				readBufferOffset++; //move past newline
				break;
			}
		}
	}
	if (t < maxRead) {
		buffer[t] = '\0';
	} else {
		buffer[maxRead] = '\0';
	}

	if (overflow) {
		vnLog(EL_error, COM_SCRIPT, "Command line is too long (> %d characters)", READ_BUFFER_SIZE);

		Command command;
		command.id = SKIP;
		commands.push_back(command);
		return;
	}

	trimString(buffer);

	Command command;
	ParseCommand(&command, buffer);
	commands.push_back(command);

	//vnLog(EL_verbose, cname, "command parsed. id=%d", command.id);
}

bool readStr(char* out, int outL) {
	char* path = strtok(NULL, " \t\r");
	if (!path) {
		out[0] = '\0';
		return false;
	}
	strncpy(out, path, outL-1);
	out[outL-1] = '\0';
	return true;
}
bool readInt(int* out) {
	char* str = strtok(NULL, " \t\r");
	if (!str) {
		*out = 0;
		return false;
	}
	*out = atoi(str);
	return true;
}
void ScriptEngine::ParseCommand(Command* cmd, char* data) {
	char* name = strtok(data, " \t\r");

	if (!name) {
		//Empty line
		cmd->id = SKIP;
		return;
	}

    if (name[0] == '#') {
        cmd->id = SKIP;
    } else if (strcmp(name, "bgload") == 0) {
    	cmd->id = BGLOAD;
    	readStr(cmd->bgload.path, MAXPATHLEN);
    	int fadeTime;
    	if (readInt(&fadeTime)) {
    		cmd->bgload.fadeTime = fadeTime;
    	} else {
    		cmd->bgload.fadeTime = -1;
    	}
    } else if (strcmp(name, "setimg") == 0) {
        cmd->id = SETIMG;
    	readStr(cmd->setimg.path, MAXPATHLEN);
    	readInt(&cmd->setimg.x);
    	readInt(&cmd->setimg.y);
    } else if (strcmp(name, "sound") == 0) {
    	cmd->id = SOUND;
    	readStr(cmd->sound.path, MAXPATHLEN);
    	if (!readInt(&cmd->sound.repeats)) {
    		cmd->sound.repeats = 1;
    	}
    } else if (strcmp(name, "music") == 0) {
    	cmd->id = MUSIC;
    	readStr(cmd->music.path, MAXPATHLEN);
    } else if (strcmp(name, "text") == 0) {
    	cmd->id = TEXT;
        char* text = strtok(NULL, "\n");
        if (text) {
        	while (isspace(*text)) text++;

			strncpy(cmd->text.text, text, CMD_TEXT_LENGTH-1);
			cmd->text.text[CMD_TEXT_LENGTH-1] = '\0';
        } else {
        	cmd->text.text[0] = '\0';
        }
    } else if (strcmp(name, "choice") == 0) {
        cmd->id = CHOICE;

        char* optionsStr;
        int t = 0;
        char* buffer = cmd->choice.optionsBuffer;
        int bufferL = CMD_OPTIONS_BUFFER_LENGTH;
        int offset = 0;
        while ((optionsStr = strtok(NULL, "|")) != NULL) {
        	if (t >= CMD_OPTIONS_MAX_OPTIONS) {
        		vnLog(EL_error, COM_SCRIPT, "Too many options in choice (> %d options)", CMD_OPTIONS_MAX_OPTIONS);
        		cmd->id = SKIP;
        		return;
        	}

        	int optionsStrL = strlen(optionsStr);
        	if (optionsStrL+1 > bufferL) {
        		//Buffer overflow
        		vnLog(EL_error, COM_SCRIPT, "Too many characters in choice options (> %d characters)", bufferL);
        		cmd->id = SKIP;
        		return;
        	}

        	cmd->choice.optionsOffsets[t] = offset;
        	memcpy(buffer+offset, optionsStr, optionsStrL+1);
        	offset += optionsStrL+1;

        	t++;
        }
        cmd->choice.optionsL = t;
    } else if (strcmp(name, "setvar") == 0 || strcmp(name, "gsetvar") == 0) {
        cmd->id = (name[0] == 'g' ? GSETVAR : SETVAR);
        readStr(cmd->setvar.name, VAR_NAME_LENGTH);
        readStr(cmd->setvar.op,   VAR_OP_LENGTH);

        char* value = strtok(NULL, "\n");
        if (value) {
			strncpy(cmd->setvar.value, value, VAR_STRING_LENGTH-1);
			cmd->setvar.value[VAR_STRING_LENGTH-1] = '\0';
        } else {
        	cmd->setvar.value[0] = '\0';
        }
    } else if (strcmp(name, "if") == 0) {
        cmd->id = IF;
        readStr(cmd->vif.expr1, VAR_NAME_LENGTH);
        readStr(cmd->vif.op,    VAR_OP_LENGTH);

        char* value = strtok(NULL, "\n");
        if (value) {
			strncpy(cmd->vif.expr2, value, VAR_NAME_LENGTH-1);
			cmd->vif.expr2[VAR_NAME_LENGTH-1] = '\0';
        } else {
        	cmd->vif.expr2[0] = '\0';
        }
    } else if (strcmp(name, "fi") == 0) {
        cmd->id = FI;
    } else if (strcmp(name, "jump") == 0) {
        cmd->id = JUMP;
    	readStr(cmd->jump.path, MAXPATHLEN);
        readStr(cmd->jump.label, VAR_NAME_LENGTH);
    } else if (strcmp(name, "delay") == 0) {
        cmd->id = DELAY;
        readInt(&cmd->delay.time);
    } else if (strcmp(name, "random") == 0) {
        cmd->id = RANDOM;
        readStr(cmd->random.name, VAR_NAME_LENGTH);
        readInt(&cmd->random.low);
        readInt(&cmd->random.high);
    } else if (strcmp(name, "label") == 0) {
        cmd->id = LABEL; // maybe index these later if it proves to be slow
        readStr(cmd->label.label, VAR_NAME_LENGTH);
    } else if (strcmp(name, "goto") == 0) {
        cmd->id = GOTO;
        readStr(cmd->lgoto.label, VAR_NAME_LENGTH);
    } else if (strcmp(name, "cleartext") == 0) {
        cmd->id = CLEARTEXT;
        readStr(cmd->cleartext.cleartype, VAR_NAME_LENGTH);
    } else if (strcmp(name, "endscript") == 0) {
        cmd->id = ENDSCRIPT;
    } else if (eof) {
    	cmd->id = ENDSCRIPT;
    } else {
    	cmd->id = SKIP;

		vnLog(EL_error, COM_SCRIPT, "Unknown command: %s", name);
    }
}

bool ScriptEngine::JumpToLabel(const char* lbl) {
	SetScriptFile(vnds->scriptEngine->GetOpenFile());

    Command c;
    while (true) {
        c = vnds->scriptEngine->GetCommand(0);
        if (c.id == LABEL) {
            if (strcmp(c.label.label, lbl) == 0) {
                return true;
            }
		} else if (c.id == TEXT) {
			textSkip++;
        } else if (c.id == END_OF_FILE) {
            vnLog(EL_error, COM_SCRIPT, "goto cannot find label: '%s'", lbl);
            return false;
        }
        SkipCommands(1);
    }
}

void ScriptEngine::SkipTextCommands(u32 number) {
	while (textSkip < number) {
		if (commands.size() <= 0) {
			ReadNextCommand();
		}

		Command c = commands.front();
		commands.pop_front();
		fileLine++;
		if (c.id == END_OF_FILE) {
			return; //panic
		} else if (c.id == TEXT) {
			textSkip++;
			interpreter->Execute(&c, true);
		}
	}
}
void ScriptEngine::SkipCommands(u32 number) {
	while (commands.size() <= number) {
		ReadNextCommand();
	}

	fileLine += number;
	commands.erase(commands.begin(), commands.begin()+number);
}

const char* ScriptEngine::GetOpenFile() {
	if (filePath[0] == '\0') return NULL;
	return filePath;
}
u32 ScriptEngine::GetCurrentLine() {
	return fileLine;
}
u32 ScriptEngine::GetTextSkip() {
	return textSkip;
}
