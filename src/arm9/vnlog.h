#ifndef VN_LOG_H
#define VN_LOG_H

enum ErrorLevel {
	EL_verbose = 16,
	EL_missing = 8,  //Used for missing files
	EL_warning = 4,
	EL_error   = 2,
	EL_message = 0
};

#ifdef __cplusplus

#include "common.h"
#include <cstdarg>
#include <vector>

class LogEntry {
	private:
		ErrorLevel elevel;
		const char* name;
		const char* message;

	public:
		LogEntry(ErrorLevel elevel, const char* name, const char* message);
		virtual ~LogEntry();

};

class LogBuffer {
	private:
		char* buffer;
		int bufferL;
		int writeOffset;

		std::vector<LogEntry> entries;
		TextScrollPane* scrollPane;

	public:
		LogBuffer(int size);
		virtual ~LogBuffer();

		bool Append(ErrorLevel elevel, const char* componentName, const char* format, va_list args);
		void FlushToDisk();

		const std::vector<LogEntry> GetEntries();

		void SetScrollPane(TextScrollPane* scrollPane);
};

extern "C" {

#endif

void vnInitLog(int size);
void vnDestroyLog();
void vnLog(enum ErrorLevel elevel, const char* componentName, const char* format, ...);

#ifdef __cplusplus

} //end extern C

void vnLogSetScrollPane(TextScrollPane* scrollPane);

#endif

#endif
