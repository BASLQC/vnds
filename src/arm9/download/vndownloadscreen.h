#ifndef _VNDOWNLOADSCREEN_H_
#define _VNDOWNLOADSCREEN_H_

#include "../tcommon/gui/screen.h"
#include "../tcommon/gui/button.h"
#include "../tcommon/gui/textscrollpane.h"

class VNDownload;
class VNDownloadScreen;

enum IconGetStatus {
    NOTHING,
    DOWNLOADING,
    SAVING,
    IMGING,
    CLEANUP,
    SITTHEREANDDONOTHING
};

enum NovelGetStatus {
    WAITFORIT,
    LOOKATTHESIZEOFTHAT,
    LOOKATMEGO,
    ITHASBEENDONE
};

class VNDownloadScrollPane : public TextScrollPane {
	private:
		VNDownloadScreen* vnDownloadScreen;

	public:
		VNDownloadScrollPane(VNDownloadScreen* screen, Rect upImg, Rect downImg, Rect barImg, FontCache* fc);
		virtual ~VNDownloadScrollPane();

		virtual void DrawListItemBackground(s16 index, u16* buffer, u16 w, u16 h, bool selected);
};

class VNDownloadScreen: public Screen, public ButtonListener {
    private:
        bool topScreenDirty;
        bool updatetop;
        int lastSelected;
        
        VNDownload* vns;
        Text* text;
    
        u16* bg;
        u16 topBuffer[256*192];
        
        u16** icons;
        int* itoget;
        int itogetindex;
        int igetmax;
        IconGetStatus igetstatus;
        int iindex;
        
        int isock;
        int ibufstart;
        
        FILE* ifile;
        int iwritecount;
        
        int ilen;
        char ibuf[3*1024];
        
        NovelGetStatus ngetstatus;
        FILE* nfile;
        int nsock;
        int nlen;
        int ntotallen;
        char nbuf[1024];
        
        VNDownloadScrollPane* scrollPane;
		FontCache* fontCache;
        
        void DownloadIcon();
        void DownloadNovel();
    
    public:
        VNDownloadScreen(GUI* gui, VNDownload* vndownload, u16* tex, u16* bg);
        virtual ~VNDownloadScreen();
        
        u16* GetIcon(int);
        
        virtual void OnButtonPressed(Button* button);
		virtual void Update(u32& down, u32& held, touchPosition touch);
		virtual void DrawTopScreen();
        //virtual void DrawBottomScreen();
		virtual void DrawForeground();
        
};











#endif /* _VNDOWNLOADSCREEN_H_ */
