#ifndef VNDS_H
#define VNDS_H

#include "common.h"
#include "novelinfo.h"
#include <map>
#include <string>

class VNDS {
	private:
		GUI* gui;

		Archive* foregroundArchive;
		Archive* backgroundArchive;
		Archive* soundArchive;
		Archive* scriptArchive;

		bool quit;
		bool waitForInput;
		int  delay;

		void Idle();
		void Update();
		void SetVar(std::map<std::string, Variable>& map, const char* name, char op, const char* value);
		void SaveScreenShot();

	public:
        std::map<std::string, Variable> globals;
        std::map<std::string, Variable> variables;

		TextEngine* textEngine;
		GraphicsEngine* graphicsEngine;
		ScriptEngine* scriptEngine;
		SoundEngine* soundEngine;

		VNDS(NovelInfo* novelInfo);
		virtual ~VNDS();

		void Continue(bool quickread);
		void Reset();
		void Quit();
		void Run();

		bool IsWaitingForInput();
		int  GetDelay();

		void SetDelay(int d);
		void SetWaitForInput(bool b);
		void SetVariable(const char* name, char op, const char* value);
		void SetGlobal(const char* name, char op, const char* value);

};

#endif
