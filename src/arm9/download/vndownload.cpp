#include "vndownload.h"

#include "../as_lib9.h"
#include "../tcommon/text.h"
#include "../tcommon/gui/gui_common.h"
#include "../tcommon/xml_parser.h"

// make this pretty later somehow eventually.

//static bool wifi_enable = true;

VNDownload::VNDownload()
{
    gui = new GUI();
    gui->VideoInit();

    skinLoadImage("%s/select_tex", textureI, 256, 256);
    skinLoadImage("%s/download_bg", backgroundI, 256, 384);
    DC_FlushRange(textureI, 256*256*2);
	DC_FlushRange(backgroundI, 256*384*2);
    
    printf("wifi init!\n");
    
    running = true;
    int wifiStatus = ASSOCSTATUS_DISCONNECTED;
    Wifi_EnableWifi(); 
    Wifi_AutoConnect();
    
    while(wifiStatus != ASSOCSTATUS_ASSOCIATED) {
        wifiStatus = Wifi_AssocStatus();
    }
    if (wifiStatus != ASSOCSTATUS_CANNOTCONNECT)
        printf("connected!\n\n");
    else {
        printf("failed!\n\n");
        running = false;
        waitForAnyKey();
    }
    
    printf("getting file list...\n");
    GetList();
    //ParseList(NULL, 0);
}

VNDownload::~VNDownload()
{
    delete gui;
    delete[] vnlist;
    
    Wifi_DisconnectAP();
    Wifi_DisableWifi();
    //wifi_enable = false;
}

void VNDownload::GetList()
{
    printf("sending request\n");
    int sockfd = http_send("/files/vnds/dlc.xml");

    printf("recieving\n");
    char* data = http_recv(sockfd);
    int tlen = strlen(data);
    printf("%d bytes recieved\n", tlen);
    
    printf("reading list...\n");
    ParseList(data, tlen);
    
    delete[] data;
	shutdown(sockfd,0);
	close(sockfd);
}

void VNDownload::ParseList(char* data, int len)
{
    mkdirs("cache/");
    FILE* tfile = fopen("cache/dlc.xml", "w");
    fwrite(data, 1, len, tfile);
    fclose(tfile);
    
    XmlFile file;
    XmlNode* root = file.Open("cache/dlc.xml");
    
    novelcount = root->children.size();
    vnlist = new VNItem[novelcount];
    
    int ncount = 0;
    for (u32 i = 0; i < root->children.size(); i++) {
    	if (strcmp(root->children[i]->text, "novel")) {
    		continue;
        }

        XmlNode* novel = root->children[i];
        strncpy(vnlist[ncount].title, novel->GetChild("title")->GetTextContent(), 128);
        strncpy(vnlist[ncount].path, novel->GetChild("path")->GetTextContent(), 128);
        strncpy(vnlist[ncount].icon, novel->GetChild("icon")->GetTextContent(), 128);
        strncpy(vnlist[ncount].size, novel->GetChild("size")->GetTextContent(), 32);
        strncpy(vnlist[ncount].version, novel->GetChild("version")->GetTextContent(), 16);
        ++ncount;
    }
}

void VNDownload::Run()
{
	mainScreen = new VNDownloadScreen(gui, this, textureI, backgroundI);
	gui->PushScreen(mainScreen);
    consoleClear();
    
    while (running) {
		gui->Update();
        gui->Draw();

        AS_SoundVBL();
        swiWaitForVBlank();
	}
    
}

void VNDownload::Stop()
{
    running = false;
}
