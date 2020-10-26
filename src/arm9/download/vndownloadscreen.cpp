#include "vndownloadscreen.h"
#include "vndownload.h"
#include "../tcommon/gui/gui_common.h"
#include "../tcommon/text.h"
#include "../tcommon/png.h"
#include "../prefs.h"
#include <fcntl.h>

VNDownloadScrollPane::VNDownloadScrollPane(VNDownloadScreen* s, Rect upImg, Rect downImg, Rect barImg, FontCache* fc)
: TextScrollPane(s, upImg, downImg, barImg, fc)
{
	vnDownloadScreen = s;
}
VNDownloadScrollPane::~VNDownloadScrollPane() {

}

void VNDownloadScrollPane::DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected) {
	TextScrollPane::DrawListItemBackground(index, buffer, w, h, selected);

	const u16* img = NULL;
    img = vnDownloadScreen->GetIcon(index);
	if (img) {
		blit(img, 32, 32, buffer, w, h, 0, 0, 0, 0, w, h);
	}
}


VNDownloadScreen::VNDownloadScreen(GUI* gui, VNDownload* vns, u16* tex, u16* bg) : Screen(gui)
{
    this->vns = vns;
    this->bg = bg;
    memcpy(topBuffer, bg, 256*192*2);
	topScreenDirty = true;
    updatetop = true;
    
    igetstatus = NOTHING;
    ngetstatus = WAITFORIT;
    nsock = -1;
    iindex = 0;
    itogetindex = 0;
    igetmax = 0;
    memset(ibuf, 0, 3*1024);
    
    SetTexture(tex);
	SetBackgroundImage(bg+256*192);
        
    char path[MAXPATHLEN];
	sprintf(path, "%s/font.ttf", skinPath);
	fontCache = new FontCache(path);
    
    text = new Text(fontCache);
    text->SetBuffer(256, 192);
    
    scrollPane = new VNDownloadScrollPane(this, Rect(0,0,16,16), Rect(0,16,16,16),
			Rect(0,32,16,16), fontCache); //Deleted automatically
	scrollPane->SetItemBg(tex+(224<<8), 256, 32, tex+(192<<8), 256, 32);
	scrollPane->SetTextWrapping(false);
	scrollPane->SetTextBufferSize(1024);
	scrollPane->SetItemHeight(32);
	scrollPane->SetDefaultMargins(40, 8, 0, 0);
	scrollPane->SetFontSize(skin->GetSelectFontSize(), false);
	scrollPane->SetBounds(0, 0, 256, 192);
    
    
    icons = new u16*[vns->novelcount];
    for(int i = 0; i < vns->novelcount; ++i) {
        scrollPane->AppendText(vns->vnlist[i].title);
        icons[i] = NULL;
    }
    itoget = new int[vns->novelcount+1];
    memset(itoget, -1, vns->novelcount+1);
    
    for(int i = 0; i < vns->novelcount; ++i) {
        sprintf(path, "cache/%s.raw", vns->vnlist[i].title);
        if (fexists(path)) {
            icons[i] = new u16[32*32];
            FILE* file = fopen(path, "rb");
            fread(icons[i], 2, 32*32, file);
            fclose(file);
        }
        else {
            itoget[igetmax++] = i;
        }
    }
    
    scrollPane->SetSelectedIndex(0);
    scrollPane->SetScroll(0);
}

VNDownloadScreen::~VNDownloadScreen()
{
    delete text;
    for(int i = 0; i < vns->novelcount; ++i) {
        if (icons[i])
            delete[] icons[i];
    }
    delete[] icons;
    delete[] itoget;
}

u16* VNDownloadScreen::GetIcon(int i)
{
    return icons[i];
}


void VNDownloadScreen::DownloadIcon()
{
    if (igetstatus == NOTHING) {
        igetstatus = DOWNLOADING;
        
        if (itogetindex == igetmax) {
            igetstatus = SITTHEREANDDONOTHING;
            return;
        }

        iindex = itoget[itogetindex++];
        
        isock = http_send(vns->vnlist[iindex].icon);
        fcntl(isock, O_NONBLOCK); // doubt it makes a difference, whatever
        ibufstart = 0;
        iwritecount = 0;
        memset(ibuf, 0, 3*1024);
        ilen = 0;
    }
    else if (igetstatus == DOWNLOADING) {
        int len = recv(isock, ibuf+ilen, 32, 0);
        if (len == -1) {
            return;
        }
        if (len == 0) {
            int i;
            for (i = 0; ibuf+i+4 != NULL; ++i) {
                if (!strncmp(ibuf+i, "\r\n\r\n", 4)) {
                    i += 4;
                    break;
                }
            }
            ibufstart = i + 8; // 4 for \r\n\r\n, another 8 for raw header (w/h)
            
            igetstatus = SAVING;
            
            char path[128];
            sprintf(path, "cache/%s.raw", vns->vnlist[iindex].title);
            ifile = fopen(path, "wb");
            
        }
        ilen += len;
    }
    else if (igetstatus == SAVING) {
        int w = MIN(128, ilen-ibufstart-iwritecount);
        int z = fwrite(ibuf+ibufstart+iwritecount, 1, w, ifile);
        iwritecount += z;
        if (z == 0) {
            igetstatus = IMGING;
        }
    }
    else if (igetstatus == IMGING) {
        fclose(ifile);
        ifile = NULL;
        icons[iindex] = new u16[32*32];
        memcpy(icons[iindex], ibuf+ibufstart, ilen-ibufstart);
        igetstatus = NOTHING;
        
        shutdown(isock, 0);
	    close(isock);
    }
    else if (igetstatus == CLEANUP) {
        //igetstatus = SITTHEREANDDONOTHING;
    }
}

void VNDownloadScreen::DownloadNovel()
{
    if (ngetstatus == WAITFORIT) {
        return;
    }
    if (ngetstatus == LOOKATTHESIZEOFTHAT) {
        // stop getting icons
        
        int rlen = 0;
        char buf[512];
        
        while(true) {
            if (ntotallen != 0)
                break;
            rlen = recv(nsock, buf, 511, 0);

            for(int c = 0; c < rlen; ++c) {
                if (buf[c] == '\n') {
                    if (!strncmp("Content-Length", buf+c+1, LLEN)) {
                        char tmp[16];
                        sscanf(buf+c,"%s %d\r\n", tmp, &ntotallen);
                        break;
                    }
                }
            }
            if (rlen == 0)
                break;
        }
        updatetop = true;
        char path[128];
        sprintf(path, "novels/%s", basename(vns->vnlist[scrollPane->GetSelectedIndex()].path));
        nfile = fopen(path, "wb");
        ngetstatus = LOOKATMEGO;
    }
    if (ngetstatus == LOOKATMEGO) {
        int alen;
        alen = recv(nsock, nbuf, 1024, 0);
        fwrite(nbuf, 1, alen, nfile);
        nlen += alen;
        
        if (alen == 0) {
            ngetstatus = ITHASBEENDONE;
        }
        
        updatetop = true;
    }
    if (ngetstatus == ITHASBEENDONE) {
        fclose(nfile);
        shutdown(nsock,0);
        close(nsock);
        nsock = -1;
        ngetstatus = WAITFORIT;
    }
}

void VNDownloadScreen::Update(u32& down, u32& held, touchPosition touch)
{
    if (ngetstatus != LOOKATMEGO) {
	    Screen::Update(down, held, touch);
        DownloadIcon();
    }
    
    DownloadNovel();
    
    if (down == KEY_START) {
        vns->Stop();
    }
    
    if (down == KEY_A) {
        if (nsock == -1) {
            nsock = http_send(vns->vnlist[scrollPane->GetSelectedIndex()].path);
            ngetstatus = LOOKATTHESIZEOFTHAT;
            ntotallen = 0;
            nlen = 0;
        }
    }
}

void VNDownloadScreen::DrawTopScreen()
{
	int s = scrollPane->GetSelectedIndex();
    
	if (lastSelected != s) {
        updatetop = true;
    }
    if (updatetop) {
		if (s >= 0 && s < vns->novelcount) {
            dmaCopy(this->bg, topBuffer, 256*192*2);
            char buf[256];
            text->ClearBuffer();

            text->SetPen(12, 48);
	        text->SetColor(RGB15(31, 31, 31));
            text->SetFontSize(16);
            text->PrintLine(vns->vnlist[s].title);
            text->PrintNewline();
            
            text->SetFontSize(12);
            text->SetPen(12, text->GetPenY());
            sprintf(buf, "version: %s", vns->vnlist[s].version);
            text->PrintLine(buf);
            text->PrintNewline();
            
            text->SetPen(12, text->GetPenY());
            sprintf(buf, "size: %s", vns->vnlist[s].size);
            text->PrintLine(buf);
            text->PrintNewline();
            
            if (ngetstatus == LOOKATMEGO) {
                text->PrintNewline();
                text->PrintNewline();
                sprintf(buf, "%db / %db", nlen, ntotallen);
                text->PrintLine(buf);
                text->PrintNewline();
            }
    
            text->BlitToScreen(topBuffer);
			topScreenDirty = true;
		}
		lastSelected = s;
	}

	if (topScreenDirty) {
		DC_FlushRange(topBuffer, 256*192*2);
		dmaCopy(topBuffer, gui->sub_bg, 256*192*2);
		topScreenDirty = false;
	}
}

void VNDownloadScreen::DrawForeground()
{
	Screen::DrawForeground();
}


void VNDownloadScreen::OnButtonPressed(Button* button)
{
}
