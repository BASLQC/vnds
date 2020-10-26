#include "aac.h"
#include "as_lib9.h"
#include "sound_engine.h"
#include "tcommon/filehandle.h"

#define SBR_MUL 1 //Use 2 instead of 1 if you're using SBR

#define FREQ 22050 //44100
#define BUFFER_SIZE (64 * 1024 + FRAME_SIZE) //PCM buffers used by ARM7
#define READBUF_SIZE (AAC_MAX_NCHANS * AAC_MAINBUF_SIZE) //AAC file read buffer used by aac-decoder
#define FRAME_SIZE (AAC_MAX_NCHANS * AAC_MAX_NSAMPS * SBR_MUL)
#define BYTES_PER_OVERFLOW (2)

AACPlayer* aacPlayer;
bool playing;
int bufferReadPos;
int bufferWritePos;
int bufferStopPos;
int delay;

//TODO: The decoder assumes FREQ Hz, mono. The code should also work with other
//      sample rates and numbers of channels.

AACPlayer::AACPlayer() {
    buffer = new u8[BUFFER_SIZE];
    bufferReadPos = bufferWritePos = 0;
    bufferStopPos = -1;

    aacDec = (HAACDecoder*)AACInitDecoder();
	//Set up memory in a non-cache area
    out = (s16*)((u32)memalign(32, FRAME_SIZE*sizeof(s16)) | 0x400000);
    readBuf = new u8[READBUF_SIZE];

    fileHandle = NULL;
    channel = -1;
    playing = false;
}

AACPlayer::~AACPlayer() {
    StopSound();

    if (aacDec) {
        AACFreeDecoder(aacDec);
    }

    free(out);
    delete[] readBuf;
    delete[] buffer;
}

void timerOverflow() {
    if (playing) {
        if (delay > 0) {
            delay--;
        }
        if (delay <= 0) {
            int oldReadPos = bufferReadPos;

            bufferReadPos += BYTES_PER_OVERFLOW;
            if (bufferReadPos >= BUFFER_SIZE) {
                bufferReadPos -= BUFFER_SIZE;

                if (bufferStopPos >= 0) {
                    if (oldReadPos < bufferStopPos || bufferReadPos >= bufferStopPos) {
                        playing = false;
                        //iprintf("finished");
                    }
                }
            } else {
                if (bufferStopPos >= 0) {
                    if (oldReadPos < bufferStopPos && bufferReadPos >= bufferStopPos) {
                        playing = false;
                        //iprintf("finished");
                    }
                }
            }
        }
    }
}

void InitAACPlayer() {
    if (aacPlayer) {
        delete aacPlayer;
    }

    aacPlayer = new AACPlayer();
}

bool AACPlayer::PlaySound(Archive* archive, char* filename, u8 volume) {
    StopSound();

    readPtr = readBuf;
    bytesLeft = 0;
    eof = false;

    fileHandle = fhOpen(archive, filename);
    if (!fileHandle) {
        return false;
    }

    AACFlushCodec(aacDec);

    memset(buffer, 0, BUFFER_SIZE);
    bufferStopPos = -1;
    bufferReadPos = 0; //Has to be lower than writepos, or fillbuffer won't do anything
    bufferWritePos = 2; //Pos has to be a multiple of the number of bytes per sample
    //delay = FREQ/2;
    delay = 0;
    FillBuffer();

    PlayPCM(buffer, 0, BUFFER_SIZE, volume);
    swiWaitForVBlank();

    //Setup timer for AAC sound
    TIMER_DATA(2) = 0x10000 - (0x1000000 / FREQ) * 2;
    TIMER_CR(2) = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_1;
    irqEnable(IRQ_TIMER2);
    irqSet(IRQ_TIMER2, timerOverflow);

    playing = true;

	for (int n = 0; n < SOUND_COMMAND_DELAY; n++) {
		swiWaitForVBlank();
	}
    return true;
}
void AACPlayer::StopSound() {
    playing = false;
    bufferStopPos = -1;

    irqDisable(IRQ_TIMER2);
    TIMER_DATA(2) = 0;
    TIMER_CR(2) = 0;

    if (channel >= 0) {
        AS_SoundStop(channel);
        channel = -1;

        for (int n = 0; n < SOUND_COMMAND_DELAY; n++) {
        	swiWaitForVBlank();
        }
    }

    if (fileHandle) {
        fhClose(fileHandle);
        fileHandle = NULL;
    }
}

void AACPlayer::Update() {
    if (playing) {
    	//iprintf("d=06%d r=%06d w=%06d\n", delay, bufferReadPos, bufferWritePos);

    	if (bufferStopPos < 0) {
            FillBuffer();
        } else {
            //Fill writeable area with zeroes

            int writeBufferLeft = bufferReadPos - bufferWritePos;
            if (writeBufferLeft < 0) writeBufferLeft += BUFFER_SIZE;
            int space = BUFFER_SIZE / 4; //BUFFER_SIZE / 3;
            int b = writeBufferLeft - space;
            if (b > 0) {
                int l = MIN(BUFFER_SIZE - bufferWritePos, b);
                memset(buffer + bufferWritePos, 0, l);

                bufferWritePos += l;
                if (bufferWritePos >= BUFFER_SIZE) bufferWritePos = 0;

                if (l < b) {
                    memset(buffer + bufferWritePos, 0, b - l);
                    bufferWritePos += (b - l);
                }
                writeBufferLeft -= b;
            }
        }
    } else {
        if (bufferStopPos >= 0) {
            bufferStopPos = -1;

            irqDisable(IRQ_TIMER2);
            TIMER_DATA(2) = 0;
            TIMER_CR(2) = 0;

            if (channel >= 0) {
                AS_SoundStop(channel);
                channel = -1;
            }
            memset(buffer, 0, BUFFER_SIZE);

            for (int n = 0; n < SOUND_COMMAND_DELAY; n++) {
            	swiWaitForVBlank();
            }
        }
    }
}

void AACPlayer::FillBuffer() {
    if (!fileHandle) return;

    int writeBufferLeft = bufferReadPos - bufferWritePos;
    if (writeBufferLeft < 0) writeBufferLeft += BUFFER_SIZE;

    //Keep some extra distance because of the inaccurate way the bufferReadPos
    //is calculated/updated.
    int space = BUFFER_SIZE / 4;//BUFFER_SIZE / 3;
    if (writeBufferLeft < FRAME_SIZE + space) {
        return;
    }

    AACFrameInfo aacFrameInfo;

    do {
    	if (!eof && bytesLeft < AAC_MAX_NCHANS * AAC_MAINBUF_SIZE) {
    		int nRead = FillReadBuffer(fileHandle, readBuf, readPtr, READBUF_SIZE, bytesLeft);
    		bytesLeft += nRead;
    		readPtr = readBuf;
    		if (nRead == 0) {
    			eof = true;
            }
    	}

        if (AACDecode(aacDec, &readPtr, &bytesLeft, out) != 0) {
            bufferStopPos = bufferWritePos - space;
            if (bufferStopPos < 0) bufferStopPos += BUFFER_SIZE;
            break;
        } else {
    		AACGetLastFrameInfo(aacDec, &aacFrameInfo);

            int b = aacFrameInfo.outputSamps * (aacFrameInfo.bitsPerSample>>3);

            //Write to circle buffer
            int l = MIN(BUFFER_SIZE - bufferWritePos, b);
            memcpy(buffer + bufferWritePos, (u8*)out, l);

            bufferWritePos += l;
            if (bufferWritePos >= BUFFER_SIZE) bufferWritePos = 0;

            if (l < b) {
                memcpy(buffer + bufferWritePos, ((u8*)out) + l, b - l);
                bufferWritePos += (b - l);
            }

            writeBufferLeft -= b;
            if (writeBufferLeft < FRAME_SIZE + space) {
                //Break before an overflow can occur
                break;
            }
        }
    } while (true);
}

int AACPlayer::FillReadBuffer(FileHandle* fh, u8* readBuf, u8* readPtr, int bufSize, int bytesLeft) {
	int nRead;

	/* move last, small chunk from end of buffer to start, then fill with new data */
	memmove(readBuf, readPtr, bytesLeft);
	nRead = fh->Read(readBuf + bytesLeft, bufSize - bytesLeft);

	/* zero-pad to avoid finding false sync word after last frame (from old data in readBuf) */
	if (nRead < bufSize - bytesLeft) {
		memset(readBuf + bytesLeft + nRead, 0, bufSize - bytesLeft - nRead);
    }

	return nRead;
}

void AACPlayer::PlayPCM(u8* buffer, int from, int to, u8 volume) {
    if (channel >= 0) {
        AS_SoundStop(channel);
    }
    for (int n = 0; n < SOUND_COMMAND_DELAY; n++) {
    	swiWaitForVBlank();
    }

    //Send data to AS_Lib
    SoundInfo si;
    si.data = buffer + from;
    si.size = to - from;
    si.format = AS_PCM_16BIT;
    si.rate = FREQ;
    si.volume = volume;
    si.pan = 64;
    si.loop = 1;
    si.priority = 0;
    si.delay = 0;

    channel = AS_SoundPlay(si);
}
