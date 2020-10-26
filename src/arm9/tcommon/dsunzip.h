#ifndef T_DSUNZIP_H
#define T_DSUNZIP_H

#include "common.h"
#include <list>

class ArchiveEntryHandle;

struct ArchiveRecord {
	char* filename;
	u32 offset;
	u32 size;
};

class Archive {

    private:
        FILE* file;
        int prefixL;
        int archiveFileSize;

        static const int MAX_FILENAME_BUFFERS = 256;
        static const int FILENAME_BUFFER_SIZE = 8192;

        std::list<char*> filenameBuffers;
        int filenameBuffersOffsets[MAX_FILENAME_BUFFERS];

    public:
    	ArchiveRecord* records;
        char* prefix;
        u32 filecount;

        Archive(FILE* file, const char* prefix, u32 archiveFileSize, u32 filecount);
        ~Archive();

        ArchiveEntryHandle* OpenFile(const char* filename, bool shareFileHandle);
        int GetFileIndex(const char* filename);
        int Read(void* out, u32 offset, u32 len);
        void AddFilename(const char* filename, int filenameL, u32 index);

        bool LoadFileEntries(const char* filename);
        bool SaveFileEntries(const char* filename);
};

Archive* openZipFile(const char* filename, const char* prefix, void (*callback)(int, int, const char*) = NULL);
Archive* openArchive(const char* filename, const char* prefix, void (*callback)(int, int, const char*) = NULL);
void closeArchive(Archive* archive);
bool unzip(const char* zipFile, const char* targetFolder);

#endif
