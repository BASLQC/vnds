#include "script_interpreter.h"
#include "graphics_engine.h"
#include "script_engine.h"
#include "sound_engine.h"
#include "text_engine.h"
#include "vnchoice.h"
#include "vnds.h"
#include "maintextpane.h"

using namespace std;

ScriptInterpreter::ScriptInterpreter(VNDS* vnds) {
	this->vnds = vnds;
}
ScriptInterpreter::~ScriptInterpreter() {
}

void ScriptInterpreter::Execute(Command* command, bool quickread) {
	switch (command->id) {
	case TEXT:      cmd_text(command, quickread, false);return;
	case SETIMG:    cmd_setimg(command, quickread);     return;
	case BGLOAD:    cmd_bgload(command, quickread);     return;
	case SOUND:     cmd_sound(command, quickread);      return;
	case MUSIC:     cmd_music(command, quickread);      return;
	case SKIP:      cmd_skip(command, quickread);       return;

	case CHOICE:    cmd_choice(command, quickread);     return;
	case SETVAR:    cmd_setvar(command, quickread);     return;
	case GSETVAR:   cmd_gsetvar(command, quickread);    return;
	case IF:        cmd_if(command, quickread);         return;
	case FI:        cmd_fi(command, quickread);         return;
	case JUMP:      cmd_jump(command, quickread);       return;
	case DELAY:     cmd_delay(command, quickread);      return;
	case RANDOM:    cmd_random(command, quickread);     return;
    case LABEL:     cmd_label(command, quickread);      return;
    case GOTO:      cmd_goto(command, quickread);       return;
	case ENDSCRIPT: cmd_endscript(command, quickread);  return;

	case END_OF_FILE: cmd_eof(command, quickread);		return;
	}
}

void ScriptInterpreter::cmd_text(Command* cmd, bool quickread, bool skipread) {
	if (!skipread) {
		vnds->graphicsEngine->Flush(quickread);
	}

	char firstChar = cmd->text.text[0];
	if (firstChar == '~') {
		vnds->textEngine->GetTextPane()->AppendText("");
	} else if (firstChar == '!') {
		vnds->SetWaitForInput(true);
	} else {
		char text[CMD_TEXT_LENGTH*2]; //Reserve extra space for inserted variable values
		ReplaceVars(text, cmd->text.text + (firstChar == '@' ? 1 : 0));
		vnds->textEngine->GetTextPane()->AppendText(text);
		vnds->SetWaitForInput(firstChar != '@');
	}
}
void ScriptInterpreter::cmd_setimg(Command* cmd, bool quickread) {
	char path[MAXPATHLEN];
	sprintf(path, "foreground/%s", cmd->setimg.path);
	vnds->graphicsEngine->SetForeground(path, cmd->setimg.x, cmd->setimg.y);
}
void ScriptInterpreter::cmd_bgload(Command* cmd, bool quickread) {
	char path[MAXPATHLEN];
	sprintf(path, "background/%s", cmd->bgload.path);
	vnds->graphicsEngine->SetBackground(path);
}
void ScriptInterpreter::cmd_sound(Command* cmd, bool quickread) {
	if (!quickread) {
		char path[MAXPATHLEN];
		sprintf(path, "sound/%s", cmd->sound.path);
		vnds->soundEngine->PlaySound(path, cmd->sound.repeats);
	}
}
void ScriptInterpreter::cmd_music(Command* cmd, bool quickread) {
	char path[MAXPATHLEN];
	sprintf(path, "sound/%s", cmd->music.path);
	vnds->soundEngine->SetMusic(path);
}
void ScriptInterpreter::cmd_skip(Command* cmd, bool quickread) {
	//Do Nothing
}

void ScriptInterpreter::cmd_choice(Command* cmd, bool quickread) {
	vnds->graphicsEngine->Flush(quickread);

	VNChoice* cv = vnds->textEngine->GetChoiceView();
	cv->Activate();
	cv->RemoveAllItems();
	for (int n = 0; n < cmd->choice.optionsL; n++) {
		cv->AppendText(cmd->choice.optionsBuffer+cmd->choice.optionsOffsets[n]);
	}
	cv->SetSelectedIndex(0);
	cv->SetScroll(0);
}
void ScriptInterpreter::cmd_setvar(Command* cmd, bool quickread) {
	vnds->SetVariable(cmd->setvar.name, cmd->setvar.op[0], cmd->setvar.value);
}
void ScriptInterpreter::cmd_gsetvar(Command* cmd, bool quickread) {
	vnds->SetGlobal(cmd->setvar.name, cmd->setvar.op[0], cmd->setvar.value);
}
void ScriptInterpreter::cmd_if(Command* cmd, bool quickread) {
	if (!EvaluateIf(cmd->vif.expr1, cmd->vif.op, cmd->vif.expr2)) {
		//If condition evaluates to false, skip to matching fi
		int nesting = 1;

		Command cmd;
		do {
			cmd = vnds->scriptEngine->GetCommand(0);
			if (cmd.id == IF) {
				nesting++;
			} else if (cmd.id == FI) {
				nesting--;
			}
			vnds->scriptEngine->SkipCommands(1);
		} while (nesting > 0 && cmd.id != END_OF_FILE);

		if (nesting > 0) {
			vnLog(EL_warning, COM_SCRIPT, "Invalid nesting of if's. Reached the end of the file before encountering the required number of fi's");
		}
	}
}
void ScriptInterpreter::cmd_fi(Command* cmd, bool quickread) {
	//Do Nothing
}
void ScriptInterpreter::cmd_jump(Command* cmd, bool quickread) {
	char path[MAXPATHLEN];
	sprintf(path, "script/%s", cmd->jump.path);
	vnds->scriptEngine->SetScriptFile(path);
}
void ScriptInterpreter::cmd_delay(Command* cmd, bool quickread) {
	if (!quickread) {
		vnds->graphicsEngine->Flush(quickread);
		vnds->SetDelay(cmd->delay.time);
	}
}
void ScriptInterpreter::cmd_random(Command* cmd, bool quickread) {
    int high  = cmd->random.high + 1;
    int low   = cmd->random.low;
    int value = (rand() % (high-low)) + low;

    char valueString[VAR_STRING_LENGTH];
    sprintf(valueString, "%d", value);
    vnds->SetVariable(cmd->random.name, '=', valueString);
}
void ScriptInterpreter::cmd_endscript(Command* cmd, bool quickread) {
	vnds->scriptEngine->SetScriptFile("script/main.scr");
}
void ScriptInterpreter::cmd_eof(Command* cmd, bool quickread) {
	vnds->scriptEngine->SetScriptFile("script/main.scr");
}

void ScriptInterpreter::cmd_label(Command* cmd, bool quickread) {
	//Do Nothing
}
void ScriptInterpreter::cmd_goto(Command* cmd, bool quickread) {
    // jump back to the begining
	vnds->scriptEngine->SetScriptFile(vnds->scriptEngine->GetOpenFile());

    Command lbl;
    while (true) {
        lbl = vnds->scriptEngine->GetCommand(0);
        if (lbl.id == LABEL) {
            if (strcmp(lbl.label.label, cmd->lgoto.label) == 0) {
                break;
            }
        }
        if (lbl.id == END_OF_FILE) {
            vnLog(EL_error, COM_SCRIPT, "goto cannot find label: '%s'", cmd->lgoto.label);
            break;
        }
        vnds->scriptEngine->SkipCommands(1);
    }
}

void ScriptInterpreter::ReplaceVars(char* out, const char* text) {
	int n = 0;
	int t = 0;
	while (text[n] != '\0') {
		if (text[n] != '$') {
			out[t] = text[n];

			n++;
			t++;
		} else {
			n++; //Skip '$' char

			char varname[VAR_NAME_LENGTH];
			int x = 0;
			while (x < VAR_NAME_LENGTH-1 && text[n] != '\0' && !isspace(text[n])) {
				varname[x] = text[n];

				x++;
				n++;
			}
			varname[x] = '\0';

			Variable var(varname);
			if (vnds->globals.count(varname) > 0) {
				var = vnds->globals[varname];
			} else if (vnds->variables.count(varname) > 0)  {
				var = vnds->variables[varname];
			}

			strcpy(out+t, var.strval);
			t += strlen(var.strval);
		}
	}
	out[n] = '\0';
}
bool ScriptInterpreter::EvaluateIf(const char* expr1, const char* op, const char* expr2) {
	Variable v1(expr1);
	if (vnds->globals.count(expr1) > 0) {
		v1 = vnds->globals[expr1];
	} else if (vnds->variables.count(expr1) > 0)  {
		v1 = vnds->variables[expr1];
	}

	Variable v2(expr2);
	if (vnds->globals.count(expr2) > 0) {
		v2 = vnds->globals[expr2];
	} else if (vnds->variables.count(expr2) > 0)  {
		v2 = vnds->variables[expr2];
	}

	vnLog(EL_verbose, COM_SCRIPT, "eval_if: %s %s %s", expr1, op, expr2);
    if (op[0] == '=' && op[1] == '=') {
    	return (v1 == v2);
    } else if (op[0] == '!' && op[1] == '=') {
    	return (v1 != v2);
    } else if (op[0] == '>' && op[1] == '=') {
    	return (v1 >= v2);
    } else if (op[0] == '<' && op[1] == '=') {
    	return (v1 <= v2);
    } else if (op[0] == '>') {
    	return (v1 > v2);
    } else if (op[0] == '<') {
    	return (v1 < v2);
    } else {
    	vnLog(EL_error, COM_SCRIPT, "Unknown operator in if expression: '%s'", op);
    	return false;
    }
}
