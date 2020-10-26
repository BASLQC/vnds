/*

  Advanced Sound Library (ASlib)
  ------------------------------

  file        : sound7.c
  author      : Lasorsa Yohan (Noda)
  description : ARM7 sound definitions

  history :

    28/11/2007 - v1.0
      = Original release

    21/12/2007 - v1.1
      = corrected arm7/amr9 initialization (fix M3S/R4 problems)
      = fixed stereo detection problem (thanks to ThomasS)

*/

#ifndef __SOUND7_H__
#define __SOUND7_H__

#include <nds.h>

// mp3 static defines
#ifdef SHRINK_AS_LIB
#define AS_DECODEBUFFER_SIZE (MAX_NCHAN * 1 * MAX_NSAMP)
#else
#define AS_DECODEBUFFER_SIZE (MAX_NCHAN * MAX_NGRAN * MAX_NSAMP)
#endif

// mp3 commands
typedef enum
{
    // internal commands
    MP3CMD_NONE = 0,
    MP3CMD_MIX = 1,
    MP3CMD_MIXING = 2,
    MP3CMD_WAITING = 4,

    // user commands
    MP3CMD_INIT = 8,
    MP3CMD_STOP = 16,
    MP3CMD_PLAY = 32,
    MP3CMD_PAUSE = 64,
    MP3CMD_SETRATE = 128

} MP3Command;

// sound commands
typedef enum
{
    // internal commands
    SNDCMD_ARM7READY = 128,
    SNDCMD_NONE = 0,
    SNDCMD_DELAY = 1,

    // user commands
    SNDCMD_STOP = 2,
    SNDCMD_PLAY = 4,
    SNDCMD_SETVOLUME = 8,
    SNDCMD_SETPAN = 16,
    SNDCMD_SETRATE = 32,
    SNDCMD_SETMASTERVOLUME = 64

} SoundCommand;

// mp3 states
typedef enum
{
    MP3ST_STOPPED = 0,
    MP3ST_PLAYING = 1,
    MP3ST_PAUSED = 2,
    MP3ST_OUT_OF_DATA = 4,
    MP3ST_DECODE_ERROR = 8,
	MP3ST_INITFAILED = 16	// <PALIB-CHANGE>

} MP3Status;

// ASlib modes
typedef enum
{
    AS_MODE_MP3 = 1,        // use mp3
    AS_MODE_SURROUND = 2,   // use surround
    AS_MODE_16CH = 4,       // use all DS channels
    AS_MODE_8CH = 8         // use DS channels 1-8 only

} AS_MODE;

// delay values
typedef enum
{
    AS_NO_DELAY = 0,    // 0 ms delay
    AS_SURROUND = 1,    // 16 ms delay
    AS_REVERB = 4,      // 66 ms delay

} AS_DELAY;

// sound formats
typedef enum
{
    AS_PCM_8BIT = 0,
    AS_PCM_16BIT = 1,
    AS_ADPCM = 2

} AS_SOUNDFORMAT;

// sound info
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

// sound channel info
typedef struct
{
    SoundInfo snd;
    u8  busy;
    u8  reserved;
    s8  volume;
    s8  pan;
    u8  cmd;

} SoundChannel;

// MP3 player info
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

// IPC structure for the sound system
typedef struct
{
    MP3Player mp3;
    SoundChannel chan[16];
    u8 surround;
    u8 num_chan;
    u8 volume;

} IPC_SoundSystem;

extern IPC_SoundSystem* ipcSound;

// initialize the main system
void AS_Init(IPC_SoundSystem* ipc);

// the sound engine, must be called each VBlank
void AS_SoundVBL();

// the mp3 decoding engine, must be called on a regular basis (like after VBlank)
void AS_MP3Engine();

#endif

