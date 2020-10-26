#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include "common.h"
#include <deque>

#define SCRIPT_READ_BUFFER_SIZE   8192

#define VAR_NAME_LENGTH    		  64
#define VAR_STRING_LENGTH         64
#define VAR_OP_LENGTH             4

#define CMD_TEXT_LENGTH 		  500
#define CMD_OPTIONS_MAX_OPTIONS   32
#define CMD_OPTIONS_BUFFER_LENGTH 500

//------------------------------------------------------------------------------

enum VarType {
	VT_null   = 0,
    VT_int    = 1,
    VT_string = 2
};

class Variable {
	private:

    public:
        VarType type;

        //Remember to always keep strval and intval in sync!
        int     intval;
        char    strval[VAR_STRING_LENGTH];

        Variable(const char* value=NULL);
        virtual ~Variable();

        bool operator== (const Variable& v) const;
        bool operator!= (const Variable& v) const;
        bool operator>  (const Variable& v) const;
        bool operator<  (const Variable& v) const;
        bool operator>= (const Variable& v) const;
        bool operator<= (const Variable& v) const;

};

//------------------------------------------------------------------------------

enum CommandType {
    BGLOAD,
    SETIMG,
    SOUND,
    MUSIC,
    TEXT,
    CHOICE,
    SETVAR,
    GSETVAR,
    IF,
    FI,
    JUMP,
    DELAY,
    RANDOM,
    SKIP,
    ENDSCRIPT,
    LABEL,
    GOTO,
    CLEARTEXT,
    END_OF_FILE //Special command that's used when you try to read more commands than are in the script
};

struct Command {
    CommandType id;

    union {
		struct {
			char path[MAXPATHLEN];
			s16  fadeTime;
		} bgload;
		struct {
			int  x;
			int  y;
			char path[MAXPATHLEN];
		} setimg;
		struct {
			int  repeats;
			char path[MAXPATHLEN];
		} sound;
		struct {
			char path[MAXPATHLEN];
		} music;
		struct {
			char text[CMD_TEXT_LENGTH];
		} text;
		struct {
			u8   optionsL;
			u8   optionsOffsets[CMD_OPTIONS_MAX_OPTIONS];
			char optionsBuffer[CMD_OPTIONS_BUFFER_LENGTH];
		} choice;
		struct {
			char name[VAR_NAME_LENGTH];
			char op[VAR_OP_LENGTH];
			char value[VAR_STRING_LENGTH];
		} setvar;
		struct {
			char expr1[VAR_NAME_LENGTH];
			char op[VAR_OP_LENGTH];
			char expr2[VAR_NAME_LENGTH];
		} vif;
		struct {
			char path[MAXPATHLEN];
            char label[VAR_NAME_LENGTH];
		} jump;
		struct {
			int  time;
		} delay;
		struct {
			char label[VAR_NAME_LENGTH];
		} label;
		struct {
			char label[VAR_NAME_LENGTH];
		} lgoto;
		struct {
			char cleartype[VAR_NAME_LENGTH];
		} cleartext;
		struct {
			char name[VAR_NAME_LENGTH];
			int  low;
			int  high;
		} random;
    };
};

//------------------------------------------------------------------------------

class ScriptEngine {
	private:
		VNDS* vnds;
		ScriptInterpreter* interpreter;

		Archive* archive;
		FileHandle* file;
		char filePath[MAXPATHLEN];
		u32  fileLine;
		u32  textSkip;

		bool eof;
		char readBuffer[SCRIPT_READ_BUFFER_SIZE];
		int  readBufferL;
		int  readBufferOffset;

		std::deque<Command> commands;
		Command eofCommand;

		void ReadNextCommand(); //Enqueues end_script commands if no more commands are available
		void ParseCommand(Command* cmd, char* data);

	public:
		ScriptEngine(VNDS* vnds, Archive* archive);
		virtual ~ScriptEngine();

		void Reset();
		void ExecuteNextCommand(bool quickread);
		void QuickRead();
		void SkipCommands(u32 number);
		void SkipTextCommands(u32 number);
		bool JumpToLabel(const char* lbl);

		Command GetCommand(u32 offset);
		const char* GetOpenFile();
		u32 GetCurrentLine();
		u32 GetTextSkip();

		void SetScriptFile(const char* filename);

};

#endif
