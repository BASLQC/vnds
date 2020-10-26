#ifndef T_TEXT_H
#define T_TEXT_H

#include "common.h"
#include "fontcache.h"

/*
 * Draw methods accept ANSI color escape codes "\x1b[$1;$2m"
 * ----------------------------------------------------------------------------
 * Reset       0        Reset         39;0
 * Black       30;0     Gray          30;1
 * Maroon      31;0     Red           31;1
 * Green       32;0     Lime          32;1
 * Olive       33;0     Yellow        33;1
 * Navy        34;0     Blue          34;1
 * Purple      35;0     Fuchsia       35;1
 * Teal        36;0     Aqua          36;1
 * Silver      37;0     White         37;1
 * ----------------------------------------------------------------------------
 */

#define ALPHA_THRESHOLD     16
#define MAX_STATES          32

struct TextState {
    FontIdentifier* font;
    s16  penX, penY;
    u16  color;
    u16  visibleChars;
    u16  marginLeft, marginRight, marginTop, marginBottom;
    u8   fontsize;
};

class Text {
    private:
    	FontCache* cache;
        TextState states[MAX_STATES];
        u8 statesL;

        FontIdentifier* font;
        u8   fontsize;
        s16  penX, penY;
        u16  color;
        s16  visibleChars;
        s16  marginLeft, marginRight, marginTop, marginBottom;

        u16* buffer;
        u8*  bufferAlpha;
        u16  bufferW, bufferH;

        u8   ProcessEscapeCode(const char* string, u16 defaultColor);
        void PrintChar(u32 c);
        int  WrapString(const char* string, bool draw, void(*drawFunc)(const char*)=NULL);

    public:
    	u16 lineBreaks[64]; //Value set in WrapString
    	u16 lineBreaksL;    //Value set in WrapString

        Text(FontCache* fc=defaultFontCache);
        virtual ~Text();

        void PushState();
        void PopState(u8 amount=1);

        void ClearBuffer();
        void CopyToScreen(u16* screen, u16 screenW=SCREEN_WIDTH, u16 screenH=SCREEN_HEIGHT,
            s16 sx=0, s16 sy=0, s16 dx=0, s16 dy=0, u16 cw=SCREEN_WIDTH, u16 ch=SCREEN_HEIGHT);
        void BlitToScreen(u16* screen, u16 screenW=SCREEN_WIDTH, u16 screenH=SCREEN_HEIGHT,
            s16 sx=0, s16 sy=0, s16 dx=0, s16 dy=0, u16 cw=SCREEN_WIDTH, u16 ch=SCREEN_HEIGHT);
        u8   PrintString(const char* str, void(*drawFunc)(const char*)=NULL);
        void PrintLine(const char* str);
        void PrintNewline();

        u16  GetBufferWidth();
        u16  GetBufferHeight();

        u64  GetMargins();
        u16  GetMarginLeft();
        u16  GetMarginRight();
        u16  GetMarginTop();
        u16  GetMarginBottom();
        s16  GetPenX();
        s16  GetPenY();
        u16  GetColor();
        u8   GetFontSize();

        u8   GetLineHeight();
        u8   GetStringLines(const char* str);

        void SetBuffer(s16 w, s16 h);

        void SetVisibleChars(s16 vc);
        void SetMargins(u64 m);
        void SetMargins(s16 left, s16 right, s16 top, s16 bottom);
        void SetPen(s16 x, s16 y);
        void SetColor(u16 color);
        FontIdentifier* SetFont(const char* filename);
        u8 SetFontSize(u8 size);
};

#endif

