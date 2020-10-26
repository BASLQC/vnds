#include "ini_parser.h"
#include <ctype.h>

#define SCRIPT_SIZE 8192
#define BUF_L       4096

//==============================================================================

FileList::FileList(const char* folder, const char* ext, bool filesOnly) {
    u32 extL = (ext ? strlen(ext) : 0);

    files = NULL;
    filesL = 0;

    DIR_ITER* dir = diropen(folder);
    if (dir) {
        struct stat st;
        char filename[MAXPATHLEN]; //Always guaranteed to be enough to hold a filename
    	while (dirnext(dir, filename, &st) == 0) {
            if (!filesOnly || (st.st_mode & S_IFDIR) == 0) {
                for (int n = 0; filename[n] != '\0'; n++) filename[n] = tolower(filename[n]);

				char* temp = (ext ? strstr(filename, ext) : NULL);
				if (!ext || (temp && strlen(temp) == extL)) {
					filesL++;
				}
            }
        }

        files = new char*[filesL];
        for (int n = 0; n < filesL; n++) {
            files[n] = NULL;
        }

        dirreset(dir);
        int t = 0;
        while (dirnext(dir, filename, &st) == 0) {
            if (!filesOnly || (st.st_mode & S_IFDIR) == 0) {
                for (int n = 0; filename[n] != '\0'; n++) filename[n] = tolower(filename[n]);

                char* temp = (ext ? strstr(filename, ext) : NULL);
                if (!ext || (temp && strlen(temp) == extL)) {
                    int filenameL = strlen(filename);
                    files[t] = new char[filenameL+1];
                    strncpy(files[t], filename, filenameL);
                    files[t][filenameL] = '\0';
                    t++;
                }
            }
    	}
    	filesL = t;

    	dirclose(dir);
    }
    
    Reset();
}
FileList::~FileList() {
    if (files) {
        for (int n = 0; n < filesL; n++) {
            if (files[n]) delete[] files[n];
        }
        delete[] files;
    }
}

void FileList::Reset() {
    index = -1;
}
char* FileList::NextFile() {
    index++;
    if (files && index < filesL) {
        return files[index];
    }
    return NULL;
}

s16 FileList::GetFilesL() {
    return filesL;
}

//==============================================================================

IniFile::IniFile() {    
}
IniFile::~IniFile() {
    Clear();
}

void IniFile::Clear() {
    u32 recL = records.size();
    IniRecord* rec[recL];
    for (u32 n = 0; n < recL; n++) {
        rec[n] = records[n];
    }    
    records.clear();
    
    for (u32 n = 0; n < recL; n++) {
        delete rec[n];
    }
}

bool IniFile::Load(const char* file) {
    FileHandle* fh = fhOpen(file);
    if (!fh) {
        return false;
    }    

    bool result = Load(fh);

    fhClose(fh);      
    return result;  
}

bool IniFile::Load(FileHandle* fh) {
    Clear();
    
    char* script = new char[SCRIPT_SIZE];    
    char* buf = new char[BUF_L];
    int readSize = 0;
    int readIndex = 0;
    
    do {
        ProcessLine(fh, script, readIndex, readSize, buf);
    } while (readSize > 0);
    
    delete[] buf;
    delete[] script;
    
    return true;
}

bool IniFile::Save(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return false;

    for (u32 n = 0; n < records.size(); n++) {
        fprintf(f, "%s=%s\n", records[n]->name, records[n]->value);
    }
    
    fclose(f);
    return true;
}

void IniFile::ProcessLine(FileHandle* fh, char* script, int& readIndex, int& readSize,
    char* lineBuffer)
{
    bool nonWhitespaceRead = false;
    int t = 0;
    while (t < BUF_L - 1) {
        if (readIndex >= readSize) {
            readSize = fh->Read(script, SCRIPT_SIZE);
            readIndex = 0;
            if (readSize <= 0) {
                break;
            }
        }
        if (script[readIndex] == '\r' || script[readIndex] == '\n') {
            readIndex++;
            break;
        }
        
        if (script[readIndex] != ' ' && script[readIndex] != '\t') {
            nonWhitespaceRead = true;
        }
        if (nonWhitespaceRead) {
            lineBuffer[t] = script[readIndex];
            t++;
        }
        
        readIndex++;
    }    
    lineBuffer[t++] = '\0';

    if (lineBuffer[0] == '#') {
        //Commented line
        return;
    }
    
    char* name = lineBuffer;
    char* value = strstr(lineBuffer, "=");    
    if (value) {
        //Replace the equals sign with a null character to split the string in two parts
        value[0] = '\0';
        value++;

        SetRecord(name, value);
    }    
}

IniRecord* IniFile::GetRecord(const char* name) {
    for (u32 n = 0; n < records.size(); n++) {
        if (strcmp(records[n]->name, name) == 0) {
            return records[n];
        }
    }
    return NULL;
}

void IniFile::SetRecord(const char* name, const char* value) {
    //Remove any other entries with the same name first
    for (u32 n = 0; n < records.size(); n++) {
        if (strcmp(records[n]->name, name) == 0) {
            IniRecord* record = records[n];
            records.erase(records.begin() + n);
            delete record;
            n--;
        }
    }
    
    //Add the new entry
    records.push_back(new IniRecord(name, value));
}

//==============================================================================

IniRecord::IniRecord(const char* n, const char* v) {
    name = strdup(n);
    trimString(name);
    
    value = strdup(v);
    trimString(value);
    unescapeString(value);
}
IniRecord::~IniRecord() {
    free(name);
    free(value);
}

char* IniRecord::GetName() {
    return name;
}
char* IniRecord::GetValue() {
    return value;
}
char* IniRecord::AsString() {
    return value;
}
s32 IniRecord::AsFixed() {
    return floattof32(atof(value));
}
int IniRecord::AsInt() {
    return atoi(value);
}
bool IniRecord::AsBool() {
    return (strcmp(value, "true") == 0);
}
