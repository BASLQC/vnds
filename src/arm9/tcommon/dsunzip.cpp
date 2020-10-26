#include "dsunzip.h"

#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include "filehandle.h"

using namespace std;

const int CACHE_VERSION_ID = 1;

bool archiveRecordComp(const ArchiveRecord& r1, const ArchiveRecord& r2) {
	return strcmp(r1.filename, r2.filename) < 0;
}

bool unzip(const char* zipFile, const char* targetFolder) {
	Archive* archive = openZipFile(zipFile, NULL);
	if (!archive) {
		return false; //zipFile doesn't exist or isn't valid
	}

	char targetPath[MAXPATHLEN];
	strcpy(targetPath, targetFolder);
	mkdirs(targetPath);

	char buffer[4096];

	const u32 filecount = archive->filecount;
	for (u32 index = 0; index < filecount; index++) {
		ArchiveRecord r = archive->records[index];

		sprintf(targetPath, "%s/%s%s", targetFolder, archive->prefix, r.filename);
		mkdirs(targetPath);

		if (r.filename[0] == '\0' || r.filename[strlen(r.filename)-1] == '/') {
			continue; //This entry is a folder, we don't have to write those
		}

		FILE* outFile = fopen(targetPath, "wb");
		if (outFile) {
			u32 offset = r.offset;
			u32 fileSize = r.size;
			u32 totalRead = 0;
			while (totalRead < fileSize) {
				int r = archive->Read(buffer, offset+totalRead, MIN(fileSize-totalRead, 4096));
				if (r > 0) {
					totalRead += r;
					fwrite(buffer, 1, r, outFile);
				} else if (r == 0) {
					break;
				}
			}
			fclose(outFile);
		} else {
			delete archive;
			return false;
		}
	}

	delete archive;
	return true;
}

//------------------------------------------------------------------------------

Archive::Archive(FILE* file, const char* pre, u32 archiveFileSize, u32 filecount) {
    this->file = file;
    this->archiveFileSize = archiveFileSize;

    if (pre) {
    	prefix = strdup(pre);
    	prefixL = strlen(pre);
    } else {
    	prefix = (char*)malloc(1);
    	prefix[0] = '\0';
    	prefixL = 0;
    }

    this->records = new ArchiveRecord[filecount];
    this->filecount = filecount;

    filenameBuffersOffsets[0] = 0;
}

Archive::~Archive() {
    if (file) {
        fclose(file);
        file = NULL;
    }

    if (prefix) free(prefix);

    while (filenameBuffers.size() > 0) {
    	char* entry = filenameBuffers.back();
    	filenameBuffers.pop_back();
    	delete[] entry;
    }
    delete[] records;
}

bool Archive::LoadFileEntries(const char* filename) {
	char path[MAXPATHLEN];
	sprintf(path, "cache/%s", filename);
	char* periodChar = strrchr(path, '.');
	if (!periodChar) periodChar = path+strlen(path);
	strcpy(periodChar, ".cache");

	FILE* file = fopen(path, "rb");
	if (!file) {
		return false;
	}

	int cacheVersionId;
	fread(&cacheVersionId, sizeof(int), 1, file);
	if (cacheVersionId != CACHE_VERSION_ID) {
		fclose(file);
		return false;
	}

	int storedArchiveFileSize;
	fread(&storedArchiveFileSize, sizeof(int), 1, file);
	if (archiveFileSize != storedArchiveFileSize) {
		//Zipfile was modified since the cache was created
		fclose(file);
		return false;
	}

	fread(&filecount, sizeof(u32), 1, file);
	for (u32 n = 0; n < filecount; n++) {
		int filenameL;
		fread(&filenameL, sizeof(int), 1, file);
		fread(path, sizeof(char), filenameL, file);
		path[filenameL] = '\0';
		fread(&records[n].offset, sizeof(int), 1, file);
		fread(&records[n].size, sizeof(int), 1, file);

		AddFilename(path, filenameL, n);

		//iprintf("%04d:%04d:%06d:%s\n", n, records[n].size, records[n].offset, records[n].filename);
	}

	fclose(file);
	return true;
}
bool Archive::SaveFileEntries(const char* filename) {
	if (!mkdirs("cache")) {
		return false;
	}

	char path[MAXPATHLEN];
	sprintf(path, "cache/%s", filename);
	char* periodChar = strrchr(path, '.');
	if (!periodChar) periodChar = path+strlen(path);
	strcpy(periodChar, ".cache");

	FILE* file = fopen(path, "wb");
	if (!file) {
		return false;
	}

	fwrite(&CACHE_VERSION_ID, sizeof(int), 1, file);
	fwrite(&archiveFileSize, sizeof(int), 1, file);
	fwrite(&filecount, sizeof(u32), 1, file);
	for (u32 n = 0; n < filecount; n++) {
		int filenameL = strlen(records[n].filename);
		fwrite(&filenameL, sizeof(int), 1, file);
		fwrite(records[n].filename, sizeof(char), filenameL, file);
		fwrite(&records[n].offset, sizeof(u32), 1, file);
		fwrite(&records[n].size, sizeof(u32), 1, file);
	}

	fclose(file);
	return true;
}

ArchiveEntryHandle* Archive::OpenFile(const char* filename, bool shareFileHandle) {
    int index = GetFileIndex(filename);
    if (index >= 0) {
        ArchiveEntryHandle* fh = new ArchiveEntryHandle(this, filename, index, shareFileHandle);
        return fh;
    }
    return NULL;
}

int Archive::GetFileIndex(const char* name) {
	if (strncmp(name, prefix, prefixL) != 0) {
		return -1;
	}
	name += prefixL;

	//This works on the unsorted version
    /*for (u32 n = 0; n < filecount; n++) {
    	if (strcmp(records[n].filename, name) == 0) {
    		return n;
    	}
    }*/

	//Binary search
	int low = 0;
	int high = filecount;
	int mid;
	int cmp;
	while (low < high) {
        mid = (low + high)>>1;
        cmp = strcmp(records[mid].filename, name);
        if (cmp > 0) {
        	high = mid;
        } else if (cmp < 0) {
        	low = mid+1;
        } else {
            return mid;
        }
	}
    return -1;
}

int Archive::Read(void* out, u32 offset, u32 len) {
    fseek(file, offset, SEEK_SET);
    return fread(out, 1, MIN(archiveFileSize - offset, len), file);
}

void Archive::AddFilename(const char* filename, int filenameL, u32 index) {
	char* buffer;;
	int s = filenameBuffers.size()-1;

	if (s < 0 || FILENAME_BUFFER_SIZE - filenameBuffersOffsets[s] < filenameL+1) {
		buffer = new char[FILENAME_BUFFER_SIZE];
		s++;

		filenameBuffers.push_back(buffer);
		filenameBuffersOffsets[s] = 0;
	} else {
		buffer = filenameBuffers.back();
	}

	records[index].filename = buffer+filenameBuffersOffsets[s];
	memcpy(records[index].filename, filename, filenameL+1);
	filenameBuffersOffsets[s] += filenameL+1;
}

int findCentralDirectory(FILE* file, int pos) {
    #define bufferL (1<<10)
    #define readMax (1<<14)

    unsigned char* buffer = new unsigned char[bufferL + 3];
    memset(buffer + bufferL, 0, 3); //Init last bytes to zero

    int read = 0;
    int bufferOffset = pos;
    do {
        read += bufferL;
        bufferOffset -= bufferL;
        if (bufferOffset < 0) {
            //There are a few bytes left, but I can't be bothered to search them...
            delete[] buffer;
            return -1;
        }
        fseek(file, -read, SEEK_CUR);

        if (fread(buffer, 1, bufferL, file) != bufferL) {
            //Read error
            delete[] buffer;
            return -1;
        }

        //"central directory" starts with 0x06054b50
        for (int n = (bufferL+3)-1; n >= 3; n--) {
            if (buffer[n-3]==0x50 && buffer[n-2]==0x4B && buffer[n-1]==0x05 && buffer[n]==0x06) {
                //Header found
                delete[] buffer;
                return bufferOffset + (n-3);
            }
        }

        //Move the first three bytes to the end of the buffer, they could match
        //during the next iteration
        buffer[bufferL]   = buffer[0];
        buffer[bufferL+1] = buffer[1];
        buffer[bufferL+2] = buffer[2];
    } while (bufferOffset > 0 && read < readMax);

    delete[] buffer;
    return -1;
}

Archive* openZipFile(const char* filename, const char* prefix, void (*callback)(int, int, const char*)) {
    char nameBuffer[MAXPATHLEN];
    sprintf(nameBuffer, "Opening %s...", filename);

    if (callback) callback(0, 0, nameBuffer);

    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    int prefixL = (prefix ? strlen(prefix) : 0);
    setvbuf(file, NULL, _IONBF, 0);

    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);

    int headerPos = findCentralDirectory(file, fileSize);
    if (headerPos < 0) {
        fclose(file);
        return NULL;
    }

    //Skip to header and ignore a few bytes I'm not interested in.
    fseek(file, headerPos + 10, SEEK_SET);

    u16 filecount;
    fread(&filecount, 2, 1, file);

    fseek(file, 4, SEEK_CUR); //boring

    u32 offset;
    fread(&offset, 4, 1, file);

    Archive* archive = new Archive(file, prefix, fileSize, filecount);

    if (archive->LoadFileEntries(filename)) {
    	return archive;
    }

    //Read file headers
    u16 filenameLength, extraFieldLength, commentLength, compressionMethod;
    fseek(file, offset, SEEK_SET);

    ArchiveRecord* r = archive->records;

    //Skip boring stuff
    #define INITIAL_SKIP 10
    fseek(file, INITIAL_SKIP, SEEK_CUR);

    int t = 0;
    for (int n = 0; n < filecount; n++) {
    	if (callback && ((filecount <= 800 && n&7) || n&31)) {
    		callback(n+1, filecount, NULL);
    	}

        fread(&compressionMethod, 2, 1, file);
        if (compressionMethod != 0) {
        	if (callback) callback(n+1, -1, NULL);
        	delete archive;
        	return NULL;
        }

        fseek(file, 12, SEEK_CUR); //Boring...

        fread(&r[t].size, 4, 1, file);
        fread(&filenameLength, 2, 1, file);
        fread(&extraFieldLength, 2, 1, file);
        fread(&commentLength, 2, 1, file);

        fseek(file, 8, SEEK_CUR); //Boring...

        //Offset
        fread(&r[t].offset, 4, 1, file);

        //Filename
        fread(nameBuffer, 1, filenameLength, file);
        nameBuffer[filenameLength] = '\0';

        //The current value of offset is the offset of the local file header, not
        //the file itself. Change the offset to point to the file.
        r[n].offset += 30 + filenameLength + extraFieldLength;

        //Skip to next relevant data
        fseek(file, extraFieldLength + commentLength + INITIAL_SKIP, SEEK_CUR);

        if (strncmp(prefix, nameBuffer, prefixL) == 0) {
        	archive->AddFilename(nameBuffer+prefixL, filenameLength-prefixL, t);
        	t++;
        }
    }
    archive->filecount = t;

    sort(archive->records, archive->records+archive->filecount, &archiveRecordComp);

    if (callback) callback(1, 1, "Writing cache...");

    archive->SaveFileEntries(filename);
    return archive;
}

Archive* openArchive(const char* filename, const char* prefix, void (*callback)(int, int, const char*)) {
    char path[260];
    Archive* archive = NULL;

    sprintf(path, "%s.zip", filename);
    archive = openZipFile(path, prefix, callback);
    if (archive) {
        return archive;
    }

    return NULL;
}
void closeArchive(Archive* archive) {
    if (archive) {
        delete archive;
    }
}
