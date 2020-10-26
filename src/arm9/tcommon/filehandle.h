#ifndef T_FILEHANDLE_H
#define T_FILEHANDLE_H

#include <nds.h>
#include <string.h>
#include "dsunzip.h"

class FileHandle {
	private:

    public:
        char filename[MAXPATHLEN];
        u32 startOffset;
        u32 currentOffset;
        u32 length;

        FileHandle(const char* filename);
        virtual ~FileHandle();

        virtual void Seek(int offset, int seekType);
        virtual int Tell();
        virtual int Read(void* out, u32 len) = 0; //Pure virtual
        virtual bool ReadFully(void* out);
};

class DefaultFileHandle : public FileHandle {
    private:
        FILE* file;

    public:
        DefaultFileHandle(FILE* file, const char* filename);
        virtual ~DefaultFileHandle();

        virtual int Read(void* out, u32 len);
};

class ArchiveEntryHandle : public FileHandle {
    private:
    	FILE* file;
        Archive* archive;
        u32 entryIndex;

    public:
        ArchiveEntryHandle(Archive* archive, const char* filename, u32 entryIndex, bool shareFileHandle);
        virtual ~ArchiveEntryHandle();

        virtual int Read(void* out, u32 len);
};

FileHandle* fhOpen(const char* filename);
FileHandle* fhOpen(Archive* archive, const char* filename, bool shareFileHandle=true);
void fhClose(FileHandle* fh);
void fhSeek(FileHandle* fh, int offset, int seekType);
int fhTell(FileHandle* fh);
int fhRead(void* out, int len, FileHandle* fh);
int fhRead(void* out, int elemSize, int len, FileHandle* fh);
bool fhReadFully(void* out, FileHandle* fh);

#endif
