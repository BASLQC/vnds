#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#include "common.h"

#define SOUND_BUFFER_LENGTH (96 * 1024)
#define SOUND_COMMAND_DELAY 6

enum SoundType {
	ST_none,
	ST_adpcm,
	ST_aac
};

class SoundEngine {
	private:
		Archive* soundArchive;
		bool mute;

		//MP3
		char musicPath[MAXPATHLEN];

		//SFX
		SoundType soundType;

		//ADPCM
		char soundPath[MAXPATHLEN];
		u8   soundBuffer[SOUND_BUFFER_LENGTH];
		int  soundBufferL;
		s8   soundChannel;

	public:
		SoundEngine(Archive* soundArchive);
		virtual ~SoundEngine();

		void Reset();
		void Update();
		void SetMusic(const char* path);
		void StopMusic();
		void PlaySound(const char* path, int times=1);
		void StopSound();
		void ReplaySound();
		void SetMuted(bool m);

		bool IsMuted();
		const char* GetMusicPath();

		void SetSoundVolume(u8 v);
		void SetMusicVolume(u8 v);
};

#endif
