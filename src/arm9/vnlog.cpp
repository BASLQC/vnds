#include "vnlog.h"
#include "prefs.h"
#include "tcommon/png.h"
#include "tcommon/gui/gui_common.h"

using namespace std;

LogBuffer* logBuffer = NULL;

//------------------------------------------------------------------------------
LogEntry::LogEntry(ErrorLevel el, const char* n, const char* m)
: elevel(el), name(n), message(m)
{

}
LogEntry::~LogEntry() {

}

//------------------------------------------------------------------------------

LogBuffer::LogBuffer(int size) {
	bufferL = size;
	buffer = new char[bufferL];
	writeOffset = 0;

	scrollPane = NULL;
}
LogBuffer::~LogBuffer() {
	if (buffer) delete[] buffer;
}

bool LogBuffer::Append(ErrorLevel elevel, const char* component, const char* format, va_list args) {
	char* message = buffer+writeOffset;
	int written = 0;
	written += snprintf(message, bufferL-1-writeOffset-written, "[%s] ", component);
	written += vsnprintf(message+written, bufferL-1-writeOffset-written, format, args);
    if (written >= bufferL-writeOffset) {
    	return false;
    }

    writeOffset += written;
    buffer[writeOffset] = '\n';
    writeOffset++;

    entries.push_back(LogEntry(elevel, component, message));
    if (scrollPane) {
    	u16 c = RGB15(31, 8, 8);
    	if (elevel == EL_warning || elevel == EL_missing) {
    		c = RGB15(31, 16, 8);
    	} else if (elevel == EL_verbose) {
    		c = RGB15(8, 16, 31);
    	}

#ifndef DEBUG
    	if (preferences->GetDebugLevel() < elevel) {
    		return true;
    	}
#endif

    	scrollPane->AppendText(message, c);
    }
    return true;
}

void LogBuffer::FlushToDisk() {
	FILE* file = fopen("vnds_log.log", "a");
	if (file) {
		fwrite(buffer, sizeof(char), writeOffset, file);
		fclose(file);
	}

	writeOffset = 0;
	entries.clear();
}

const vector<LogEntry> LogBuffer::GetEntries() {
	return entries;
}

void LogBuffer::SetScrollPane(TextScrollPane* scrollPane) {
	this->scrollPane = scrollPane;
}

//------------------------------------------------------------------------------

void pngErr(png_structp png_ptr, png_const_charp error_msg) {
	vnLog(EL_error, COM_GRAPHICS, "libpng_err: %s", error_msg);
	longjmp(png_ptr->jmpbuf, 1);
}
void pngWarn(png_structp png_ptr, png_const_charp warning_msg) {
	//vnLog(EL_warning, COM_GRAPHICS, "libpng_warn: %s", warning_msg);
}

void vnInitLog(int logSize) {
	vnDestroyLog();
	logBuffer = new LogBuffer(logSize > 0 ? logSize : 65535);

	pngErrorCallback = &pngErr;
	pngWarningCallback = &pngWarn;
}
void vnDestroyLog() {
	if (logBuffer) {
		delete logBuffer;
	}
}

void vnLogSetScrollPane(TextScrollPane* scrollPane) {
	logBuffer->SetScrollPane(scrollPane);
}

void vnLog(ErrorLevel elevel, const char* componentName, const char* format, ...) {
    va_list args;
    va_start(args, format);
	if (!logBuffer->Append(elevel, componentName, format, args)) {
		logBuffer->FlushToDisk();
		logBuffer->Append(elevel, componentName, format, args);
	}
    va_end(args);
}
