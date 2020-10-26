#ifndef SCRIPT_INTERPRETER_H
#define SCRIPT_INTERPRETER_H

#include "common.h"
#include <map>
#include <string>

#define CMD_TEXT_LENGTH 		  500

class ScriptInterpreter {
	private:
		VNDS* vnds;
        char rtext[CMD_TEXT_LENGTH*2]; // since all functions use it now

		void cmd_setimg(Command* cmd, bool quickread=false);
		void cmd_bgload(Command* cmd, bool quickread=false);
		void cmd_sound(Command* cmd, bool quickread=false);
		void cmd_music(Command* cmd, bool quickread=false);
		void cmd_skip(Command* cmd, bool quickread=false);

		void cmd_choice(Command* cmd, bool quickread=false);
		void cmd_setvar(Command* cmd, bool quickread=false);
		void cmd_gsetvar(Command* cmd, bool quickread=false);
		void cmd_if(Command* cmd, bool quickread=false);
		void cmd_fi(Command* cmd, bool quickread=false);
		void cmd_jump(Command* cmd, bool quickread=false);
		void cmd_delay(Command* cmd, bool quickread=false);
		void cmd_random(Command* cmd, bool quickread=false);
		void cmd_endscript(Command* cmd, bool quickread=false);
        void cmd_label(Command* cmd, bool quickread=false);
        void cmd_goto(Command* cmd, bool quickread=false);
        void cmd_cleartext(Command* cmd, bool quickread=false);

		void cmd_eof(Command* cmd, bool quickread=false);

		bool EvaluateIf(const char* expr1, const char* op, const char* expr2);
		void ReplaceVars(char* out, const char* text);

	public:
		ScriptInterpreter(VNDS* vnds);
		virtual ~ScriptInterpreter();

		void cmd_text(Command* cmd, bool quickread=false, bool skipread=false);
		void Execute(Command* command, bool quickread=false);

};

#endif
