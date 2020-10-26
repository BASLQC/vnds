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
    case CLEARTEXT: cmd_cleartext(command, quickread);  return;
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
		vnds->SetWaitForInput(false);
	} else if (firstChar == '!') {
		vnds->SetWaitForInput(true);
	} else {
		//char text[CMD_TEXT_LENGTH*2]; //Reserve extra space for inserted variable values
		ReplaceVars(rtext, cmd->text.text + (firstChar == '@' ? 1 : 0));
		vnds->textEngine->GetTextPane()->AppendText(rtext);
		vnds->SetWaitForInput(firstChar != '@');
	}
}
void ScriptInterpreter::cmd_setimg(Command* cmd, bool quickread) {
	char path[MAXPATHLEN];
    ReplaceVars(rtext, cmd->setimg.path);
	sprintf(path, "foreground/%s", rtext);
	vnds->graphicsEngine->SetForeground(path, cmd->setimg.x, cmd->setimg.y);
}
void ScriptInterpreter::cmd_bgload(Command* cmd, bool quickread) {
	if (vnds->graphicsEngine->IsBackgroundChanged()) {
		vnds->graphicsEngine->Flush(quickread);
	}

	char path[MAXPATHLEN];
    ReplaceVars(rtext, cmd->bgload.path);
	sprintf(path, "background/%s", rtext);

	vnds->graphicsEngine->SetBackground(path, cmd->bgload.fadeTime);
}
void ScriptInterpreter::cmd_sound(Command* cmd, bool quickread) {
	if (!quickread) {
		char path[MAXPATHLEN];
        ReplaceVars(rtext, cmd->sound.path);
		sprintf(path, "sound/%s", rtext);
		vnds->soundEngine->PlaySound(path, cmd->sound.repeats);
	}
}
void ScriptInterpreter::cmd_music(Command* cmd, bool quickread) {
	char path[MAXPATHLEN];
    ReplaceVars(rtext, cmd->music.path);
	sprintf(path, "sound/%s", rtext);
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
        ReplaceVars(rtext, cmd->choice.optionsBuffer+cmd->choice.optionsOffsets[n]);
		cv->AppendText(rtext);
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
    ReplaceVars(rtext, cmd->jump.path);
	sprintf(path, "script/%s", rtext);
	vnds->scriptEngine->SetScriptFile(path);
    if (cmd->jump.label[0] != '\0') {
        Command g;
        strncpy(g.lgoto.label, cmd->jump.label, VAR_STRING_LENGTH);
        cmd_goto(&g, quickread);
    }
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
    ReplaceVars(rtext, cmd->lgoto.label);

    vnds->scriptEngine->JumpToLabel(rtext);
}

void ScriptInterpreter::cmd_cleartext(Command* cmd, bool quickread) {
	if (strncmp(cmd->cleartext.cleartype, "!", 1) == 0)	{
		//"!" full case, Full wipe all text, including history
		vnds->textEngine->Reset();
	}
	else {
		//default case
		//"~" soft case, output number of text ~ equal to the maximum lines that can be displayed on the screen at once
		int n = 0;
		int displayLines = vnds->textEngine->GetTextPane()->GetVisibleItems();
		while (n <= displayLines)	{
			vnds->textEngine->GetTextPane()->AppendText("");
			n++;
		}
	}
}

void ScriptInterpreter::ReplaceVars(char* out, const char* text) {
	int n = 0;
	int t = 0;
	while (text[n] != '\0') {
		//Allow for "blah $var blah" and "blah{$var}blah"
		if (text[n] != '$' && (text[n] != '{' || text[n+1] != '$')) {
			out[t] = text[n];

			n++;
			t++;
		} else {
			//Let $$ be the escape code for a single dollar sign character
            if (text[n] == '$' && text[n+1] == '$') {
                out[t] = '$';
                t++;
                n += 2;
                continue;
            }

			char endChar = ' ';
			if (text[n] == '{') {
				endChar = '}';
				n++; //Skip '{' char
			}
            n++; //Skip '$' char

			char varname[VAR_NAME_LENGTH];
			int x = 0;
			while (x < VAR_NAME_LENGTH-1 && text[n] != '\0'
				&& text[n] != '\n' && text[n] != endChar)
			{
				varname[x] = text[n];
				x++;
				n++;
			}
			varname[x] = '\0';

			if (!isspace(endChar)) {
				n++; //Throw away '}' char
			}

			Variable var;
			if (vnds->globals.count(varname) > 0) {
				var = vnds->globals[varname];
			} else if (vnds->variables.count(varname) > 0)  {
				var = vnds->variables[varname];
			}

			strcpy(out+t, var.strval);
			t += strlen(var.strval);
		}
	}
	out[t] = '\0';
}

bool ScriptInterpreter::EvaluateIf(const char* expr1, const char* op, const char* expr2) {
	Variable v1(expr1);
	if (vnds->variables.count(expr1) > 0)  {
		v1 = vnds->variables[expr1];
	} else if (vnds->globals.count(expr1) > 0) {
		v1 = vnds->globals[expr1];
	}

	Variable v2(expr2);
	if (vnds->variables.count(expr2) > 0)  {
		v2 = vnds->variables[expr2];
	} else if (vnds->globals.count(expr2) > 0) {
		v2 = vnds->globals[expr2];
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
