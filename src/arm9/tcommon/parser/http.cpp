#include "http.h"
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

//Value can be a relative OR an absolute URL
void toAbsoluteUrl(char* out, const char* baseUrl, const char* value) {
    if (strncmp(value, "http://", 7) == 0) {
        strcpy(out, value+7);
    } else {
        sprintf(out, "%s/%s", baseUrl, value);
    }
}

//==============================================================================

HttpHeader::HttpHeader() {
}
HttpHeader::~HttpHeader() {
    Clear();
}

void HttpHeader::Clear() {
    u32 esL = entries.size();
    HttpHeaderEntry* es[esL];
    for (u32 n = 0; n < esL; n++) {
        es[n] = entries[n];
    }
    entries.clear();

    for (u32 n = 0; n < esL; n++) {
        delete es[n];
    }
}

//The buffer should be large enough to contain the entire tcp header
int HttpHeader::ReadFromTCP(char* buffer, int bufferL, TCPConnection* con) {
    int lastRead;
    int readOffset = 0;
    int writeOffset = 0;
    do {
        lastRead = con->Receive(buffer + writeOffset, bufferL - writeOffset);
        if (lastRead < 0) {
            if (errno != EWOULDBLOCK) {
                printf("TCP Error: %d\n", errno);
                break;
            }
        }
        if (lastRead > 0) {
            writeOffset += lastRead;
        }

        int lineL = -1;
        do {
            lineL = ProcessLine(buffer + readOffset, writeOffset - readOffset);
            if (lineL > 0) {
                readOffset += lineL;
                if (lineL == 2) { //line equals \r\n
                    break;
                }
            }
        } while (lineL > 0);
        if (lineL == 2) { //line equals \r\n
            break;
        }
    } while (lastRead != 0 && writeOffset < bufferL);

    memmove(buffer, buffer + readOffset, writeOffset - readOffset);
    return (writeOffset - readOffset);
}

//Returns -1 if no lines could be read, otherwise it returns the length of the line INCLUDING the \r\n part
int HttpHeader::ProcessLine(char* buffer, int bufferL) {
    char* lineEnd = (char*)memchr(buffer, '\n', bufferL);
    if (lineEnd && lineEnd != buffer && lineEnd[-1] == '\r') {
        //char found, not the first char in the array, previous char = '\r'

        lineEnd++; //Move to the end of the line (directly after \r\n)
        int lineL = (lineEnd - buffer);

        char* value = (char*)memchr(buffer, ':', lineL);
        if (value) {
            buffer[lineL-2] = '\0';
            *value = '\0';
            value++;

            SetEntry(buffer, value);
        }
        return lineL;
    }
    return -1;
}

HttpHeaderEntry* HttpHeader::GetEntry(const char* name) {
    for (u32 n = 0; n < entries.size(); n++) {
        if (strcmp(entries[n]->GetName(), name) == 0) {
            return entries[n];
        }
    }
    return NULL;
}
void HttpHeader::SetEntry(const char* name, const char* value) {
    //Remove any other entries with the same name first
    for (u32 n = 0; n < entries.size(); n++) {
        if (strcmp(entries[n]->GetName(), name) == 0) {
            HttpHeaderEntry* entry = entries[n];
            entries.erase(entries.begin() + n);
            delete entry;
            n--;
        }
    }

    //Add the new entry
    entries.push_back(new HttpHeaderEntry(name, value));
}

//==============================================================================

HttpHeaderEntry::HttpHeaderEntry(const char* n, const char* v) {
    name = strdup(n);
    trimString(name);

    value = strdup(v);
    trimString(value);
    unescapeString(value);
}
HttpHeaderEntry::~HttpHeaderEntry() {
    free(name);
    free(value);
}

char* HttpHeaderEntry::GetName() {
    return name;
}
char* HttpHeaderEntry::GetValue() {
    return value;
}
char* HttpHeaderEntry::AsString() {
    return value;
}
int HttpHeaderEntry::AsInt() {
    return atoi(value);
}
