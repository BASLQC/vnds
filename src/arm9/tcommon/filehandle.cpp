#include "filehandle.h"

FileHandle* fhOpen(const char* filename) {
    return fhOpen(NULL, filename);
}
FileHandle* fhOpen(Archive* archive, const char* filename, bool shareFileHandle) {
    //Try regular file first
    FILE* file = fopen(filename, "rb");
    if (file) {
        DefaultFileHandle* fh = new DefaultFileHandle(file, filename);
        return fh;
    }

    //Then try archive
    if (archive) {
        FileHandle* fh = archive->OpenFile(filename, shareFileHandle);
        if (fh) {
            return fh;
        }
    }

    return NULL;
}
void fhClose(FileHandle* fh) {
    if (fh) {
        delete fh;
    }
}
void fhSeek(FileHandle* fh, int offset, int seekType) {
    fh->Seek(offset, seekType);
}
int fhTell(FileHandle* fh) {
    return fh->Tell();
}
int fhRead(void* out, int len, FileHandle* fh) {
    return fh->Read(out, len);
}
int fhRead(void* out, int elemSize, int len, FileHandle* fh) {
	return fhRead(out, elemSize*len, fh);
}
bool fhReadFully(void* out, FileHandle* fh) {
    return fh->ReadFully(out);
}

//=== FileHandle methods =======================================================
FileHandle::FileHandle(const char* filename) {
    strcpy(this->filename, filename);
}
FileHandle::~FileHandle() {
}

void FileHandle::Seek(int offset, int seekType) {
    int base = currentOffset;
    if (seekType == SEEK_SET) {
        base = startOffset;
    } else if (seekType == SEEK_END) {
        base = startOffset + length;
    }
    currentOffset = base + offset;
}
int FileHandle::Tell() {
    return currentOffset - startOffset;
}
bool FileHandle::ReadFully(void* out) {
    Seek(0, SEEK_SET);
    return (u32)Read(out, length) == length;
}

//=== DefaultFileHandle methods ================================================
DefaultFileHandle::DefaultFileHandle(FILE* file, const char* filename) : FileHandle(filename) {
	this->file = file;
	this->startOffset = 0;
	this->currentOffset = 0;

	fseek(file, 0, SEEK_END);
	this->length = ftell(file);
	fseek(file, 0, SEEK_SET);
}
DefaultFileHandle::~DefaultFileHandle() {
	fclose(file);
}

int DefaultFileHandle::Read(void* out, u32 len) {
    fseek(file, currentOffset - startOffset, SEEK_SET);
    int read = fread(out, 1, len, file);
    if (read >= 0) {
        currentOffset += read;
    }
    return read;
}

//=== ArchiveEntryHandle methods ===============================================

ArchiveEntryHandle::ArchiveEntryHandle(Archive* archive, const char* filename,
		u32 entryIndex, bool shareFileHandle)
: FileHandle(filename)
{
	file = NULL;
	if (!shareFileHandle) {
		file = fopen(filename, "rb");
	}

    this->archive = archive;
    this->entryIndex = entryIndex;

    this->startOffset = archive->records[entryIndex].offset;
    this->currentOffset = startOffset;
    this->length = archive->records[entryIndex].size;
}
ArchiveEntryHandle::~ArchiveEntryHandle() {
	if (file) {
		fclose(file);
	}
}

int ArchiveEntryHandle::Read(void* out, u32 len) {
	int s = MIN(len, length - (currentOffset - startOffset));

	int read = 0;
	if (file) {
	    fseek(file, currentOffset, SEEK_SET);
	    read = fread(out, 1, s, file);
	} else {
		read = archive->Read(out, currentOffset, s);
	}
    if (read >= 0) {
        currentOffset += read;
    }
    return read;
}
