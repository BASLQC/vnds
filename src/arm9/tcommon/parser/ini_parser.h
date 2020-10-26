#ifndef T_INI_PARSER_H
#define T_INI_PARSER_H

#include <vector>
#include <fat.h>
#include <unistd.h>
#include <sys/dir.h>
#include "../filehandle.h"

class FileList {
    private:
        char** files;
        s16 filesL;
        s16 index;

    public:
        FileList(const char* folder, const char* ext=NULL, bool filesOnly=true);
        virtual ~FileList();

        void Reset();
        char* NextFile();

        s16 GetFilesL();
};

class IniRecord {
    private:

    public:
        char* name;
        char* value;

        IniRecord(const char* name, const char* value);
        virtual ~IniRecord();

        char* GetName();
        char* GetValue();
        char* AsString();
        s32   AsFixed();
        int   AsInt();
        bool  AsBool();
};

class IniFile {
    private:
        std::vector<IniRecord*> records;

        void Clear();
        void ProcessLine(FileHandle* fh, char* script, int& readIndex, int& readSize,
                         char* lineBuffer);

    public:
        IniFile();
        virtual ~IniFile();

        bool Load(const char* file);
        bool Load(FileHandle* fh);

        bool Save(const char* file);

        IniRecord* GetRecord(const char* name);
        void SetRecord(const char* name, const char* value);
};

#endif
