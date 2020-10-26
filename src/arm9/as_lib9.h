//anoNL: Changed so much stuff, you don't even wanna know

/*

  Advanced Sound Library (ASlib)
  ------------------------------

  file        : sound9.h
  author      : Lasorsa Yohan (Noda)
  description : ARM9 sound functions

  history :

    29/11/2007 - v1.0
      = Original release

    21/12/2007 - v1.1
      = corrected arm7/amr9 initialization (fix M3S/R4 problems)
      = fixed stereo detection problem (thanks to ThomasS)

*/

#ifndef __SOUND9_H__
#define __SOUND9_H__

#include <nds.h>
#include <stdio.h>
#include "tcommon/filehandle.h"
#include "../common/fifo.h"

//---- ASlib options ------------------------------------------
// uncomment the defines below to activate the desired option

// use the EFSlib to stream mp3 files instead of libfat
//#define AS_USE_EFS

// use expanded stereo panning separation in surround mode, but decrease volume
//#define AS_USE_EXPANDED_SURROUND_STEREO

//-----------------------------------------------------------

// mp3 static defines
#define MAX_NGRAN           2           // max granules
#define MAX_NCHAN           2           // max channels
#define MAX_NSAMP           576         // max samples per channel, per granule

// buffer sizes, should fit any usage (feel free to adjust if needed)
#define AS_AUDIOBUFFER_SIZE 24 * MAX_NCHAN * MAX_NGRAN * MAX_NSAMP
#define AS_FILEBUFFER_SIZE  8 * 1024   // file buffer size

// file access functions
#define MP3FILE                         FileHandle
#define FILE_CLOSE(f)                   fhClose(f)
#define FILE_OPEN(f)                    fhOpen(NULL, f, true)
#define FILE_READ(buf, size, num, f)    fhRead(buf, num * size, f)
#define FILE_SEEK(f, pos, off)          fhSeek(f, pos, off)
#define FILE_TELL(f)                    fhTell(f)

// set up the surround stereo panning mode
#ifdef AS_USE_EXPANDED_SURROUND_STEREO

#define AS_BASE_VOLUME      64
#define AS_VOL_NORMALIZE    65
#define AS_PANNING_SHIFT    0

#else

#define AS_BASE_VOLUME      96
#define AS_VOL_NORMALIZE    97
#define AS_PANNING_SHIFT    1

#endif

/// mp3 commands
typedef enum
{
    /// internal commands
    MP3CMD_NONE = 0,
    MP3CMD_MIX = 1,
    MP3CMD_MIXING = 2,
    MP3CMD_WAITING = 4,

    /// user commands
    MP3CMD_INIT = 8,
    MP3CMD_STOP = 16,
    MP3CMD_PLAY = 32,
    MP3CMD_PAUSE = 64,
    MP3CMD_SETRATE = 128

} MP3Command;

/// sound commands
typedef enum
{
    /// internal commands
    SNDCMD_ARM7READY = 128,
    SNDCMD_NONE = 0,
    SNDCMD_DELAY = 1,

    /// user commands
    SNDCMD_STOP = 2,
    SNDCMD_PLAY = 4,
    SNDCMD_SETVOLUME = 8,
    SNDCMD_SETPAN = 16,
    SNDCMD_SETRATE = 32,
    SNDCMD_SETMASTERVOLUME = 64

} SoundCommand;

/// mp3 states
typedef enum
{
    MP3ST_STOPPED = 0,
    MP3ST_PLAYING = 1,
    MP3ST_PAUSED = 2,
    MP3ST_OUT_OF_DATA = 4,
    MP3ST_DECODE_ERROR = 8,
} MP3Status;

/// ASlib modes
typedef enum
{
    AS_MODE_MP3 = 1,        /// use mp3
    AS_MODE_SURROUND = 2,   /// use surround
    AS_MODE_16CH = 4,       /// use all DS channels
    AS_MODE_8CH = 8         /// use DS channels 1-8 only

} AS_MODE;

/// delay values
typedef enum
{
    AS_NO_DELAY = 0,    /// 0 ms delay
    AS_SURROUND = 1,    /// 16 ms delay
    AS_REVERB = 4,      /// 66 ms delay

} AS_DELAY;

/// sound formats
typedef enum
{
    AS_PCM_8BIT = 0,
    AS_PCM_16BIT = 1,
    AS_ADPCM = 2

} AS_SOUNDFORMAT;

/// sound info
typedef struct
{
    u8  *data;
    u32 size;
    u8  format;
    s32 rate;
    u8  volume;
    s8  pan;
    u8  loop;
    u8  priority;
    u8  delay;

} SoundInfo;

/// sound channel info
typedef struct
{
    SoundInfo snd;;
    u8  busy;
    u8  reserved;
    s8  volume;
    s8  pan;
    u8  cmd;

} SoundChannel;

/// MP3 player info
typedef struct
{
	u32 helixbuffer;
    s8  *mixbuffer;
    u32 buffersize;
    s32 rate;
    u32 state;
    u32 soundcursor;
    u32 numsamples;
    s32 prevtimer;
    u8  *mp3buffer;
    u32 mp3buffersize;
    u32 mp3filesize;
    u32 cmd;
    u8  channelL, channelR;
    u8  loop;
    u8  stream;
    u8  needdata;
    u8  delay;

} MP3Player;

/// IPC structure for the sound system
typedef struct
{
    MP3Player mp3;
    SoundChannel chan[16];
    u8 surround;
    u8 num_chan;
    u8 volume;

} IPC_SoundSystem;

extern IPC_SoundSystem* ipcSound;

/// easiest way to play a sound, using default settings
#define AS_SoundQuickPlay(name)     AS_SoundDefaultPlay((u8*)name, (u32)name##_size, 127, 64, false, 0)

/// initialize the ASLib
void AS_Init(u8 mode, u32 mp3BufferSize);

/// reserve a particular DS channel (so it won't be used for the sound pool)
void AS_ReserveChannel(u8 channel);

/// set the master volume (0..127)
void AS_SetMasterVolume(u8 volume);

/// set the default sound settings
void AS_SetDefaultSettings(u8 format, s32 rate, u8 delay);

/// play a sound using the priority system
/// return the sound channel allocated or -1 if the sound was skipped
int AS_SoundPlay(SoundInfo sound);

/// play a sound using the priority system with the default settings
/// return the sound channel allocated or -1 if the sound was skipped
int AS_SoundDefaultPlay(u8 *data, u32 size, u8 volume, u8 pan, u8 loop, u8 prio);

/// set the panning of a sound (0=left, 64=center, 127=right)
void AS_SetSoundPan(u8 chan, u8 pan);

/// set the volume of a sound (0..127)
void AS_SetSoundVolume(u8 chan, u8 volume);

/// set the sound sample rate
void AS_SetSoundRate(u8 chan, u32 rate);

/// stop playing a sound
void AS_SoundStop(u8 chan);

/// play a sound directly using the given channel
void AS_SoundDirectPlay(u8 chan, SoundInfo sound);

/// play an mp3 directly from memory
void AS_MP3DirectPlay(u8 *buffer, u32 size);

/// play an mp3 stream
void AS_MP3StreamPlay(MP3FILE* file);

/// pause an mp3
void AS_MP3Pause();

/// unpause an mp3
void AS_MP3Unpause();

/// stop an mp3
void AS_MP3Stop();

/// get the current mp3 status
int AS_GetMP3Status();

/// set the mp3 volume (0..127)
void AS_SetMP3Volume(u8 volume);

/// set the mp3 panning (0=left, 64=center, 127=right)
void AS_SetMP3Pan(u8 pan);

/// set the default mp3 delay mode (warning: high values can cause glitches)
void AS_SetMP3Delay(u8 delay);

/// set the mp3 loop mode (false = one shot, true = loop indefinitely)
void AS_SetMP3Loop(u8 loop);

/// set the mp3 sample rate
void AS_SetMP3Rate(s32 rate);

/// regenerate buffers for mp3 stream
/// must be called each VBlank (only needed if mp3 is used)
void AS_SoundVBL();


///-------------------------------------------------------------------------------
/// definition of inlined functions
///-------------------------------------------------------------------------------

/// variables defined in as_lib9.cpp
extern MP3FILE *mp3file;
extern u8 as_default_format;
extern s32 as_default_rate;
extern u8 as_default_delay;

/// private functions, defined in as_lib9.cpp
extern void AS_MP3FillBuffer(u8 *buffer, u32 bytes);

#endif

