#ifndef T_HTTP_H
#define T_HTTP_H

#include <vector>
#include <fat.h>
#include <unistd.h>
#include <sys/dir.h>

#include "../wifi.h"

using namespace std;

class HttpHeaderEntry {
    private:
        char* name;
        char* value;

    public:
        HttpHeaderEntry(const char* name, const char* value);
        virtual ~HttpHeaderEntry();

        char* GetName();
        char* GetValue();
        char* AsString();
        int   AsInt();
};

class HttpHeader {
    private:
        vector<HttpHeaderEntry*> entries;

        int ProcessLine(char* buffer, int bufferL);

    public:
        HttpHeader();
        virtual ~HttpHeader();

        void Clear();
        int ReadFromTCP(char* buffer, int bufferL, TCPConnection* con);

        HttpHeaderEntry* GetEntry(const char* name);

        void SetEntry(const char* name, const char* value);
};

void toAbsoluteUrl(char* out, const char* baseUrl, const char* value);

#endif
