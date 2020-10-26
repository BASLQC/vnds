#ifndef AAC_H
#define AAC_H

#include <nds.h>
#include "helix-aac/pub/aacdec.h"
#include "common.h"

class AACPlayer {
    private:
        u8* buffer;
        int channel;

        HAACDecoder* aacDec;

        FileHandle* fileHandle;
        s16* out;
        u8* readBuf;
        u8* readPtr;
        int bytesLeft;
        bool eof;

        void FillBuffer();
        int FillReadBuffer(FileHandle* fh, u8* readBuf, u8* readPtr, int bufSize, int bytesLeft);
        void PlayPCM(u8* buffer, int from, int to, u8 volume=127);

    public:
        AACPlayer();
        ~AACPlayer();

        void Update();

        bool PlaySound(Archive* archive, char* filename, u8 volume=127);
        void StopSound();
};

extern AACPlayer* aacPlayer;

void InitAACPlayer();

#endif
