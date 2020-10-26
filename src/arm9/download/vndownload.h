#ifndef _VNDOWNLOAD_H_
#define _VNDOWNLOAD_H_

#include "http.h"
#include "../common.h"
#include "vndownloadscreen.h"

struct VNItem {
    char title[128];
    char path[128];
    char icon[128];
    char version[16];
    char size[32];
};


class VNDownload {
    private:
        GUI* gui;
        u16 textureI[256*256];
        u16 backgroundI[256*384];
        
        bool running;
        
        VNDownloadScreen* mainScreen;
        
        void GetList();
        void ParseList(char*, int);
        
    public:
        VNDownload();
        ~VNDownload();

        int novelcount;
        VNItem* vnlist;

        void Run();
        void Stop();
};

#endif /* _VNDOWNLOAD_H_ */
