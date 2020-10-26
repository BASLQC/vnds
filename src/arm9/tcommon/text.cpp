#include "text.h"

#include <nds.h>
#include <fat.h>

#define INVISIBLE_CHAR(c) ((c)==' ' || (c)=='\t' || (c)=='\r')

u16 ansiiPalette[] = {
	//Regular colors
	RGB15( 0, 0, 0), RGB15(15, 0, 0), RGB15( 0,15, 0), RGB15(15,15, 0),
	RGB15( 0, 0,15), RGB15(15, 0,15), RGB15( 0,15,15), RGB15(24,24,24),
	//Bright colors
	RGB15(15,15,15), RGB15(31, 0, 0), RGB15( 0,31, 0), RGB15(31,31, 0),
	RGB15( 0, 0,31), RGB15(31, 0,31), RGB15( 0,31,31), RGB15(31,31,31)
};

Text::Text(FontCache* fc) {
	cache = fc;

    buffer = NULL;
    bufferAlpha = NULL;
    bufferW = bufferH = 0;

    statesL = 0;
    PushState();

   	SetFont(fc->GetDefaultFont());
    SetFontSize(DEFAULT_FONT_SIZE);

    penX = marginLeft;
    penY = marginTop + GetLineHeight();
}

Text::~Text() {
    if (buffer)      delete[] buffer;
    if (bufferAlpha) delete[] bufferAlpha;
}

void Text::PrintChar(u32 chr) {
    if (!buffer) {
        //Can't draw if there's no buffer
        return;
    }

    FTC_SBit sbit = cache->GetGlyph(chr);
    int tx = penX + cache->GetAdvance(chr, sbit);
    if (INVISIBLE_CHAR(chr)) {
        penX = tx;
        return;
    }

    if (!sbit->buffer) {
        penX = tx;
        return;
    }

    u16 col = color | BIT(15);

    int dx = penX + sbit->left;
    int dy = penY - sbit->top;
    int xmin = MAX(0, marginLeft-dx);
    int ymin = MAX(0, marginTop-dy);
    int xmax = MIN(bufferW - marginRight - dx, sbit->width);
    int ymax = MIN(bufferH - marginBottom - dy, sbit->height);

    if (xmax >= xmin && ymax >= ymin) {
		int sjump = sbit->width - xmax + xmin;
		int djump = bufferW - xmax + xmin;
		int dstOffset = (MAX(marginTop, dy) * bufferW) + MAX(marginLeft, dx);

		u8*  sa = sbit->buffer + (ymin * sbit->width) + xmin;
		u16* dc = buffer + dstOffset;
		u8*  da = bufferAlpha + dstOffset;
		for (int y = ymin; y < ymax; y++) {
			for (int x = xmin; x < xmax; x++) {
				if (*sa >= ALPHA_THRESHOLD) {
					*da = *sa;
					*dc = col;
				}
				sa++;
				dc++;
				da++;
			}
			sa += sjump;
			dc += djump;
			da += djump;
		}
    }

    //Move pen
    penX = tx;
}

void Text::PrintLine(const char* str) {
	u16 lastColor = color;

    int t = 0;
    while (*str) {
    	if (visibleChars >= 0 && t >= visibleChars) {
    		break;
    	}

		//Parse ANSI escape codes
		while (*str == '\\' && (strncmp(str, "\\x1b[", 5) == 0 || strncmp(str, "\\x1B[", 5) == 0)) {
			str += 5;
    		str += ProcessEscapeCode(str, lastColor);
		}

        u32 c = '?';
        int bytes = FontCache::GetCodePoint(str, &c);
        if (bytes == 0) {
        	bytes = 1;
        } else {
			if (c == '\n') {
				PrintNewline();
			} else {
				PrintChar(c);
			}
        }
		str += bytes;
		t++;
    }

    color = lastColor;
}

void Text::PushState() {
    if (statesL == 0) {
        font = NULL;
        fontsize = 0;
        penX = penY = 0;
        visibleChars = -1;
        color = RGB15(31, 31, 31) | BIT(15);
        marginLeft = marginRight = marginTop = marginBottom = 5;
    }

    //Store the previous state
    TextState* s = &states[statesL];
    s->font = font;
    s->penX = penX; s->penY = penY;
    s->visibleChars = visibleChars;
    s->color = color;
    s->marginLeft = marginLeft; s->marginRight  = marginRight;
    s->marginTop  = marginTop;  s->marginBottom = marginBottom;
    s->fontsize = fontsize;

    statesL++;
}

void Text::PopState(u8 amount) {
    if (amount >= statesL) {
        statesL = 0;
    } else {
        statesL -= amount;
    }

    //Restore state from slot
    TextState* s = &states[statesL];
    penX = s->penX; penY = s->penY;
    color = s->color;
    marginLeft = s->marginLeft; marginRight  = s->marginRight;
    marginTop  = s->marginTop;  marginBottom = s->marginBottom;

    if (s->font && font != s->font) {
        SetFont(s->font->filePath);
    }
    if (fontsize != s->fontsize) {
        SetFontSize(s->fontsize);
    }
}

void Text::BlitToScreen(u16* screen, u16 screenW, u16 screenH,
    s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    if (screen && buffer) {
        blitAlpha(buffer, bufferAlpha, bufferW, bufferH,
                  screen, screenW, screenH,
                  sx, sy, dx, dy, cw, ch);
    }
}
void Text::CopyToScreen(u16* screen, u16 screenW, u16 screenH,
    s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    if (screen && buffer) {
        blit2(buffer, bufferAlpha, bufferW, bufferH,
             screen, screenW, screenH,
             sx, sy, dx, dy, cw, ch);
    }
}
void Text::ClearBuffer() {
    if (bufferAlpha) {
        memset(bufferAlpha, 0, bufferW * bufferH);
    }
}

u8 Text::ProcessEscapeCode(const char* string, u16 defaultColor) {
	int c = 39;
	int brightness = 0;

	char* end = strchr(string, 'm');
	if (!end) {
		return 0;
	}

	u32 len = (end - string);
	if (len == 1 && string[0] == '0') {
		//OK
	} else {
		if (EOF == siscanf(string, "%d;%d", &c, &brightness)) {
			return 0;
		}
	}

	//Only foreground color is supported
	c -= 30;
	if (c >= 0 && c <= 7 && brightness >= 0 && brightness <= 1) {
		SetColor(ansiiPalette[8*brightness+c]);
	} else if (brightness == 0 && c == 9) {
		SetColor(defaultColor);
	}

	return (u8)(len + 1);
}

int Text::WrapString(const char* string, bool draw, void(*drawFunc)(const char*)) {
	lineBreaksL = 0;

	int from = 0;
	int to = strlen(string);
	int lineOffset = 0;
	int visibleLines = -1;

	const int maxWidth = bufferW - marginLeft - marginRight - 1;
    const int spaceW = cache->GetAdvance(' ');
    const int lineHeight = GetLineHeight();

    if (maxWidth < spaceW) {
    	return lineBreaksL = 1;
    } else if (from == 0 && to == 0) {
        if (drawFunc) {
        	drawFunc(string);
        }
    }

	bool panic = false;
	int line = 0;
    int lineWidth = penX - marginLeft;
    int wordCount = 0;
    int charCount = 0;
    int lineDrawEnd = 0;

    u16 lastColor = color;
	int lastPenX = penX;
    int lastPenY = penY;
	int index = from;

	char colorPrefix[16] = {'\0'};

    char tempString[to - from + 1];
    int tempIndex = 0;
    int lastFittingChar = 0; //Last char that fits on the current line
    bool allCharsDrawn = true;

	while (index < to) {
		//Find next word start
        while (index < to && INVISIBLE_CHAR(string[index])) { //Skip whitespace
            index++;
            if (line >= lineOffset && (visibleLines < 0 || line - lineOffset < visibleLines)) {
            	charCount++;
            }
        }

        int oldIndex = index;
        int oldCharCount = charCount;

        int ww = 0; //Word Width (includes optional prefixed space)
        bool wordFits = true;
        bool newlineEncountered = false;

        if (index < to) {
	        if (wordCount > 0) {
	            ww = spaceW;
	            tempString[tempIndex] = ' ';
	            tempIndex++;
	        }

	        lastFittingChar = tempIndex; //Last char that fits on the current line

	        //Add letters to word
	        while (index < to && !INVISIBLE_CHAR(string[index])) {
	        	if (string[index] == '\n') {
	        		index++;
	        		lastFittingChar = index;
	        		newlineEncountered = true;
	        		wordFits = false;
	        		panic = false; //Word fits
	        		break;
	        	} else {
	        		//Parse ANSI escape codes
	        		if (string[index] == '\\' && (strncmp(string+index, "\\x1b[", 5) == 0
	        					|| strncmp(string+index, "\\x1B[", 5) == 0))
	        		{
		        		int len = 5 + ProcessEscapeCode(string+index+5, lastColor);
	        			strncpy(colorPrefix, string+index, len);
	        			strcpy(tempString+tempIndex, colorPrefix);
		            	index += len;
		            	tempIndex += len;
		        		continue;
	        		}
	        	}

                u32 c = string[index];
                int bytes = FontCache::GetCodePoint(string + index, &c);
                if (bytes == 0) bytes = 1;
                int advance = cache->GetAdvance(c);

	            if (lineWidth + ww + advance <= maxWidth) {
	                //Char still fits on this line
	                lastFittingChar += bytes;
	            } else if (ww + advance > maxWidth) {
	                //This single word is wider than an entire line. It needs to be
	                //cut on an unnatural position to be displayed.

	                wordFits = false; //Word will never fit in this state
	                break;
	            } else {
	            	ww = 9999;
	            	break;
	            }

	            for (int n = 0; n < bytes; n++) {
	            	tempString[tempIndex] = string[index];
	            	tempIndex++;
	            	index++;
	            }

                if (line >= lineOffset && (visibleLines < 0 || line - lineOffset < visibleLines)) {
                	charCount++;
                }
                if (visibleChars < 0 || charCount <= visibleChars) {
                	lineDrawEnd = tempIndex;
                } else {
            		allCharsDrawn = (visibleChars < 0);
                }

                ww += advance;
	        }
        }

        if (!wordFits || (index >= to && lineWidth + ww <= maxWidth)) {
            //if !wordFits :: Word longer than a full line, cut up word
        	//if index >= to :: Add final word to line
            tempIndex = lastFittingChar + 1;

            lineWidth += ww;
        } else if (lineWidth + ww > maxWidth) {
            tempIndex -= (index - oldIndex);
            lineDrawEnd = MIN(lineDrawEnd, tempIndex);
            charCount = oldCharCount;
            index = oldIndex;
            wordFits = false;
        }

        if (!wordFits || index >= to) {
            //Word doesn't fit on the current line or the end of the string
            //has been reached

        	if (lineDrawEnd < 0 && !newlineEncountered) {
        		break;
        	}
        	lineBreaks[line] = index;

            //Draw Line
            if (line >= lineOffset && (visibleLines < 0 || line - lineOffset < visibleLines)) {
            	tempString[lineDrawEnd] = '\0';
                if (draw) {
                	PrintLine(tempString);
                }
                if (drawFunc) {
                	drawFunc(tempString);
                }

            	lastPenX = penX;
            	lastPenY = penY;

				penX = marginLeft;
				penY += lineHeight;

				//Cursor shouldn't go back after a hard newline
				if (newlineEncountered) {
					lastPenX = penX;
					lastPenY = penY;
				}
            }

            int oldLineDrawEnd = lineDrawEnd;

            //New Line
           	line++;
            lineWidth = penX - marginLeft;
            lineDrawEnd = 0;
            wordCount = 0;
            tempIndex = 0;

            //Prefix line with colorPrefix
            strcpy(tempString+tempIndex, colorPrefix);
            tempIndex = strlen(colorPrefix);

            if (oldLineDrawEnd == 0) { //Can't even fit a single char two times in a row, time to quit
    			if (panic) {
    				break;
    			} else {
    				panic = true;
    			}
    		} else {
    			panic = false;
    		}
        } else {
            //Add word to line
            wordCount++;
            lineWidth += ww;
        }
    }

	color = lastColor;
	penX = lastPenX;
	penY = lastPenY;

	lineBreaksL = MAX(1, line);
	return lineBreaksL;
}

u8 Text::PrintString(const char* str, void(*drawFunc)(const char*)) {
    return (u8)WrapString(str, drawFunc == NULL, drawFunc);
}
void Text::PrintNewline() {
    penX = marginLeft;
    penY += GetLineHeight();
}

u16 Text::GetBufferWidth() {
    return bufferW;
}
u16 Text::GetBufferHeight() {
    return bufferH;
}
s16 Text::GetPenX() {
    return penX;
}
s16 Text::GetPenY() {
    return penY;
}
u16 Text::GetColor() {
    return color;
}
u8 Text::GetFontSize() {
    return fontsize;
}
u8 Text::GetLineHeight() {
	return cache->GetLineHeight();
}
u8 Text::GetStringLines(const char* str) {
    return (u8)WrapString(str, false);
}
u64 Text::GetMargins() {
    u64 result = (marginLeft<<16) | (marginRight);
    result = (result<<32) | (marginTop<<16) | marginBottom;
    return result;
}
u16 Text::GetMarginLeft() {
    return marginLeft;
}
u16 Text::GetMarginRight() {
    return marginRight;
}
u16 Text::GetMarginTop() {
    return marginTop;
}
u16 Text::GetMarginBottom() {
    return marginBottom;
}

void Text::SetBuffer(s16 w, s16 h) {
	if (w <= 0 || h <= 0) {
		if (buffer) {
			delete[] buffer;
			delete[] bufferAlpha;
		}
		buffer = NULL;
		bufferAlpha = NULL;
	} else if (!buffer || bufferW != w || bufferH != h) {
		if (!buffer || w > bufferW || h > bufferH) {
			if (buffer) {
				delete[] buffer;
				delete[] bufferAlpha;
			}

			buffer = new u16[w * h];
			bufferAlpha = new u8[w * h];
		}
	}

	bufferW = w;
	bufferH = h;

	ClearBuffer();
}
void Text::SetMargins(u64 m) {
    SetMargins(m>>48, (m>>32)&0xFFFF, (m>>16)&0xFFFF, m&0xFFFF);
}
void Text::SetMargins(s16 left, s16 right, s16 top, s16 bottom) {
    marginLeft = left;
    marginRight = right;
    marginTop = top;
    marginBottom = bottom;

    penX = marginLeft;
    penY = marginTop + GetLineHeight();
}
void Text::SetPen(s16 x, s16 y) {
    penX = x;
    penY = y;
}
void Text::SetColor(u16 c) {
    color = c;
}
u8 Text::SetFontSize(u8 s) {
    return fontsize = cache->SetFontSize(s);
}
FontIdentifier* Text::SetFont(const char* filename) {
    return font = cache->SetFont(filename);
}
void Text::SetVisibleChars(s16 vc) {
	visibleChars = vc;
}


//==============================================================================
