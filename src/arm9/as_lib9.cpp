//anoNL: Changed so much stuff, you don't even wanna know

/*

  Advanced Sound Library (ASlib)
  ------------------------------

  file        : sound9.c
  author      : Lasorsa Yohan (Noda)
  description : ARM7 sound functions

  history :

    29/11/2007 - v1.0
      = Original release

    21/12/2007 - v1.1
      = corrected arm7/amr9 initialization (fix M3S/R4 problems)
      = fixed stereo detection problem (thanks to ThomasS)

*/

#include <nds.h>
#include <malloc.h>
#include <string.h>

#include "as_lib9.h"

IPC_SoundSystem* ipcSound;

// variable for the mp3 file stream
MP3FILE *mp3file = NULL;
u8* mp3filebuffer = NULL;

// default settings for sounds
u8 as_default_format;
s32 as_default_rate;
u8 as_default_delay;

// initialize the ASLib
void AS_Init(u8 mode, u32 mp3BufferSize) {
	//Set up memory in a non-cache area
	u32 mem = (u32)memalign(32, sizeof(IPC_SoundSystem)) | 0x400000;
	ipcSound = (IPC_SoundSystem*)mem;

    memset(ipcSound, 0, sizeof(IPC_SoundSystem));
	if (mp3BufferSize > 0) {
		//Set up memory in a non-cache area
		ipcSound->mp3.helixbuffer = (u32)memalign(32, mp3BufferSize) | 0x400000;
	}

    int i, nb_chan = 16;

    // initialize default settings
    as_default_format = AS_PCM_8BIT;
    as_default_rate = 22050;
    as_default_delay = AS_SURROUND;

    if (!fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM7, MSG_INIT_SOUND_ARM7)) {
    	consoleDemoInit();
    	iprintf("Fatal error while initializing sound.\n");
    	waitForAnyKey();
    	return;
    }

	//consoleDemoInit();
	//iprintf("Waiting\n");
	//iprintf("%d ", ipcSound->chan[0].cmd);

    // initialize channels
    for (i = 0; i < 16; i++) {
        ipcSound->chan[i].busy = false;
        ipcSound->chan[i].reserved = false;
        ipcSound->chan[i].volume = 0;
        ipcSound->chan[i].pan = 64;
        ipcSound->chan[i].cmd = SNDCMD_NONE;
    }

    // use only 8 channels
    if(mode & AS_MODE_8CH) {

        nb_chan = 8;
        for(i = 8; i < 16; i++)
            ipcSound->chan[i].reserved = true;
    }

    // use surround
    if(mode & AS_MODE_SURROUND) {

        ipcSound->surround = true;
        for(i = nb_chan / 2; i < nb_chan; i++)
            ipcSound->chan[i].reserved = true;

    } else {
        ipcSound->surround = false;
    }

    ipcSound->num_chan = nb_chan / 2;

    // use mp3
    if (mode & AS_MODE_MP3) {
        ipcSound->mp3.mixbuffer = (s8*)((u32)memalign(4, AS_AUDIOBUFFER_SIZE * 2) | 0x400000);
        ipcSound->mp3.buffersize = AS_AUDIOBUFFER_SIZE / 2;
        ipcSound->mp3.channelL = 0;
        ipcSound->mp3.prevtimer = 0;
        ipcSound->mp3.soundcursor = 0;
        ipcSound->mp3.numsamples = 0;
        ipcSound->mp3.delay = AS_SURROUND;
        ipcSound->mp3.cmd = MP3CMD_INIT;
        ipcSound->mp3.state = MP3ST_STOPPED;

        ipcSound->chan[0].reserved = true;

        if(ipcSound->surround) {
            ipcSound->mp3.channelR = nb_chan / 2;
            ipcSound->chan[nb_chan / 2].reserved = true;
        } else {
            ipcSound->mp3.channelR = 1;
            ipcSound->chan[1].reserved = true;
        }

        ipcSound->chan[ipcSound->mp3.channelL].snd.pan = 64;
        AS_SetMP3Volume(127);
    }
    AS_SetMasterVolume(127);
    DC_FlushRange(ipcSound, sizeof(IPC_SoundSystem));

    if (!fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM7, (u32)ipcSound)) { //SendAddress doesn't work, ipcSound outside normal RAM
    	consoleDemoInit();
    	iprintf("Fatal error while initializing sound. (IPC_INIT)\n");
    	waitForAnyKey();
    	return;
    }

    //Wait for the arm 7 to be ready
    while ((ipcSound->chan[0].cmd & SNDCMD_ARM7READY) == 0) {
        DC_FlushRange(ipcSound, sizeof(IPC_SoundSystem));
        swiWaitForVBlank();
    }
    if (mode & AS_MODE_MP3) {
    	//Wait for MP3 Init
		while (ipcSound->mp3.cmd & MP3CMD_INIT) {
			DC_FlushRange(ipcSound, sizeof(IPC_SoundSystem));
			swiWaitForVBlank();
		}
    }

	//iprintf("Done Waiting\n");
}

// play a sound using the priority system
// return the sound channel allocated or -1 if the sound was skipped
int AS_SoundPlay(SoundInfo sound)
{
    int i, free_ch = -1, minp_ch = -1;

    // search a free channel
    for(i = 0; i < 16; i++) {
        if(!(ipcSound->chan[i].reserved || ipcSound->chan[i].busy))
            free_ch = i;
    }

    // if a free channel was found
    if(free_ch != -1) {

        // play the sound
        AS_SoundDirectPlay(free_ch, sound);
        return free_ch;

    } else {

        // find the channel with the least priority
        for(i = 0; i < 16; i++) {
            if(!ipcSound->chan[i].reserved) {
                if(minp_ch == -1)
                    minp_ch = i;
                else if(ipcSound->chan[i].snd.priority < ipcSound->chan[minp_ch].snd.priority)
                    minp_ch = i;
            }
        }

        // if the priority of the found channel is <= the one of the sound
        if(ipcSound->chan[minp_ch].snd.priority <= sound.priority) { //bugfix

            // play the sound
            AS_SoundDirectPlay(minp_ch, sound);
            return minp_ch;

        } else {

            // skip the sound
            return -1;
        }
    }
}

// set the panning of a sound (0=left, 64=center, 127=right)
void AS_SetSoundPan(u8 chan, u8 pan)
{
    ipcSound->chan[chan].snd.pan = pan;

    if(ipcSound->surround) {

        int difference = ((pan - 64) >> AS_PANNING_SHIFT) * ipcSound->chan[chan].snd.volume / AS_VOL_NORMALIZE;

        ipcSound->chan[chan].pan = 0;
        ipcSound->chan[chan].volume = ipcSound->chan[chan].snd.volume + difference;

        if(ipcSound->chan[chan].volume < 0)
            ipcSound->chan[chan].volume = 0;

        ipcSound->chan[chan].cmd |= SNDCMD_SETVOLUME;
        ipcSound->chan[chan].cmd |= SNDCMD_SETPAN;

        ipcSound->chan[chan + ipcSound->num_chan].pan = 127;
        ipcSound->chan[chan + ipcSound->num_chan].volume = ipcSound->chan[chan].snd.volume - difference;

        if(ipcSound->chan[chan + ipcSound->num_chan].volume < 0)
            ipcSound->chan[chan + ipcSound->num_chan].volume = 0;

        ipcSound->chan[chan + ipcSound->num_chan].cmd |= SNDCMD_SETVOLUME;
        ipcSound->chan[chan + ipcSound->num_chan].cmd |= SNDCMD_SETPAN;

    } else {

        ipcSound->chan[chan].cmd |= SNDCMD_SETPAN;
        ipcSound->chan[chan].pan = pan;

    }
}

// set the volume of a sound (0..127)
void AS_SetSoundVolume(u8 chan, u8 volume)
{
    if(ipcSound->surround) {
        ipcSound->chan[chan].snd.volume = volume * AS_BASE_VOLUME / 127;
        AS_SetSoundPan(chan, ipcSound->chan[chan].snd.pan);
    } else {
        ipcSound->chan[chan].volume = volume;
        ipcSound->chan[chan].cmd |= SNDCMD_SETVOLUME;
    }
}

// set the sound sample rate
void AS_SetSoundRate(u8 chan, u32 rate)
{
    ipcSound->chan[chan].snd.rate = rate;
    ipcSound->chan[chan].cmd |= SNDCMD_SETRATE;

    if(ipcSound->surround) {
        ipcSound->chan[chan + ipcSound->num_chan].snd.rate = rate;
        ipcSound->chan[chan + ipcSound->num_chan].cmd |= SNDCMD_SETRATE;
    }
}

// play a sound directly using the given channel
void AS_SoundDirectPlay(u8 chan, SoundInfo sound)
{
    ipcSound->chan[chan].snd = sound;
    ipcSound->chan[chan].busy = true;
    ipcSound->chan[chan].cmd = SNDCMD_PLAY;
    ipcSound->chan[chan].volume = sound.volume;
    ipcSound->chan[chan].pan = sound.pan;

    if(ipcSound->surround) {
        ipcSound->chan[chan + ipcSound->num_chan].snd = sound;
        ipcSound->chan[chan + ipcSound->num_chan].busy = true;
        ipcSound->chan[chan + ipcSound->num_chan].cmd = SNDCMD_DELAY;

        // set the correct surround volume & pan
        AS_SetSoundVolume(chan, sound.volume);
    }
    //DC_FlushRange(ipcSound->chan[chan], sizeof(SoundChannel));
}

// fill the given buffer with the required amount of mp3 data
void AS_MP3FillBuffer(u8 *buffer, u32 bytes)
{
    u32 read = FILE_READ(buffer, 1, bytes, mp3file);
    //DC_FlushRange(buffer, read);

    if((read < bytes) && ipcSound->mp3.loop) {
        FILE_SEEK(mp3file, 0, SEEK_SET);
        FILE_READ(buffer + read, 1, bytes - read, mp3file);
        //DC_FlushRange(buffer+read, read2);
    }
}

// play an mp3 directly from memory
void AS_MP3DirectPlay(u8 *mp3_data, u32 size)
{
    if(ipcSound->mp3.state & (MP3ST_PLAYING | MP3ST_PAUSED))
        return;

    ipcSound->mp3.mp3buffer = mp3_data;
    ipcSound->mp3.mp3filesize = size;
    ipcSound->mp3.stream = false;
    ipcSound->mp3.cmd = MP3CMD_PLAY;
}

// play an mp3 stream
void AS_MP3StreamPlay(FileHandle* fh)
{
    if(ipcSound->mp3.state & (MP3ST_PLAYING | MP3ST_PAUSED))
        return;

    if (mp3file && mp3file != fh) {
        FILE_CLOSE(mp3file);
    }

    mp3file = fh;

    if(mp3file) {

        // allocate the file buffer the first time
        if(!mp3filebuffer) {
            mp3filebuffer = (u8*)memalign(4, AS_FILEBUFFER_SIZE * 2);   // 2 buffers, to swap
            ipcSound->mp3.mp3buffer = mp3filebuffer;
            ipcSound->mp3.mp3buffersize = AS_FILEBUFFER_SIZE;
        }

        // get the file size
        FILE_SEEK(mp3file, 0, SEEK_END);
        ipcSound->mp3.mp3filesize = FILE_TELL(mp3file);

        // fill the file buffer
        FILE_SEEK(mp3file, 0, SEEK_SET);
        AS_MP3FillBuffer(mp3filebuffer, AS_FILEBUFFER_SIZE * 2);

        // start playing
        ipcSound->mp3.stream = true;
        ipcSound->mp3.cmd = MP3CMD_PLAY;
    }

}

// set the mp3 panning (0=left, 64=center, 127=right)
void AS_SetMP3Pan(u8 pan)
{
    int difference = ((pan - 64) >> AS_PANNING_SHIFT) * ipcSound->chan[ipcSound->mp3.channelL].snd.volume / AS_VOL_NORMALIZE;

    ipcSound->chan[ipcSound->mp3.channelL].snd.pan = pan;
    ipcSound->chan[ipcSound->mp3.channelL].pan = 0;
    ipcSound->chan[ipcSound->mp3.channelL].volume = ipcSound->chan[ipcSound->mp3.channelL].snd.volume - difference;

    if(ipcSound->chan[ipcSound->mp3.channelL].volume < 0)
        ipcSound->chan[ipcSound->mp3.channelL].volume = 0;

    ipcSound->chan[ipcSound->mp3.channelL].cmd |= SNDCMD_SETVOLUME;
    ipcSound->chan[ipcSound->mp3.channelL].cmd |= SNDCMD_SETPAN;

    ipcSound->chan[ipcSound->mp3.channelR].pan = 127;
    ipcSound->chan[ipcSound->mp3.channelR].volume = ipcSound->chan[ipcSound->mp3.channelL].snd.volume + difference;

    if(ipcSound->chan[ipcSound->mp3.channelR].volume < 0)
        ipcSound->chan[ipcSound->mp3.channelR].volume = 0;

    ipcSound->chan[ipcSound->mp3.channelR].cmd |= SNDCMD_SETVOLUME;
    ipcSound->chan[ipcSound->mp3.channelR].cmd |= SNDCMD_SETPAN;

}

// regenerate buffers for mp3 stream, must be called each VBlank (only needed if mp3 is used)
void AS_SoundVBL()
{
	if (ipcSound->mp3.state & MP3ST_DECODE_ERROR) {
		ipcSound->mp3.state = MP3ST_STOPPED;
		if (ipcSound->mp3.loop) {
			AS_MP3StreamPlay(mp3file);
		}
		return;
	}

    // refill mp3 file  buffer if needed
    if (mp3file && ipcSound->mp3.needdata) {
        AS_MP3FillBuffer(ipcSound->mp3.mp3buffer + AS_FILEBUFFER_SIZE, AS_FILEBUFFER_SIZE);
        ipcSound->mp3.needdata = false;
    }
}

/// reserve a particular DS channel (so it won't be used for the sound pool)
void AS_ReserveChannel(u8 channel) {
	ipcSound->chan[channel].reserved = true;
}

/// set the master volume (0..127)
void AS_SetMasterVolume(u8 volume) {
	ipcSound->volume = volume;
    ipcSound->chan[0].cmd |= SNDCMD_SETMASTERVOLUME;
}

/// set the default sound settings
void AS_SetDefaultSettings(u8 format, s32 rate, u8 delay) {
    as_default_format = format;
    as_default_rate = rate;
    as_default_delay = delay;
}

/// play a sound using the priority system with the default settings
/// return the sound channel allocated or -1 if the sound was skipped
int AS_SoundDefaultPlay(u8 *data, u32 size, u8 volume, u8 pan, u8 loop, u8 prio) {
    SoundInfo snd = {
        data,
        size,
        as_default_format,
        as_default_rate,
        volume,
        pan,
        loop,
        prio,
        as_default_delay
    };
    return AS_SoundPlay(snd);
}

/// stop playing a sound
void AS_SoundStop(u8 chan) {
	ipcSound->chan[chan].cmd = SNDCMD_STOP;

    if(ipcSound->surround)
    	ipcSound->chan[chan + ipcSound->num_chan].cmd = SNDCMD_STOP;
}

/// pause an mp3
void AS_MP3Pause() {
    if(ipcSound->mp3.state & MP3ST_PLAYING)
    	ipcSound->mp3.cmd = MP3CMD_PAUSE;
}

/// unpause an mp3
void AS_MP3Unpause() {
    if(ipcSound->mp3.state & MP3ST_PAUSED)
    	ipcSound->mp3.cmd = MP3CMD_PLAY;
}

/// stop an mp3
void AS_MP3Stop() {
	ipcSound->mp3.cmd = MP3CMD_STOP;
    FILE_CLOSE(mp3file);
    mp3file = NULL; ///VNDS Edit
}

/// get the current mp3 status
int AS_GetMP3Status() {
    return ipcSound->mp3.state;
}

/// set the mp3 volume (0..127)
void AS_SetMP3Volume(u8 volume) {
	ipcSound->chan[ipcSound->mp3.channelL].snd.volume = volume * AS_BASE_VOLUME / 127;
    AS_SetMP3Pan(ipcSound->chan[ipcSound->mp3.channelL].snd.pan);
}

/// set the default mp3 delay mode (warning: high values can cause glitches)
void AS_SetMP3Delay(u8 delay) {
    ipcSound->mp3.delay = delay;
}

/// set the mp3 loop mode (false = one shot, true = loop indefinitely)
void AS_SetMP3Loop(u8 loop) {
    ipcSound->mp3.loop = loop;
}

/// set the mp3 sample rate
void AS_SetMP3Rate(s32 rate) {
    ipcSound->mp3.rate = rate;
    ipcSound->mp3.cmd |= MP3CMD_SETRATE;
}
