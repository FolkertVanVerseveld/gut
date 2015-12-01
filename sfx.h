#ifndef GUT_SFX_H
#define GUT_SFX_H

#include <stdbool.h>

#define GUT_AUDIO_FLAC 1
#define GUT_AUDIO_MOD 2
#define GUT_AUDIO_MP3 4
#define GUT_AUDIO_OGG 8

/* Try to set format and return chosen format */
int gutAudioSetFormat(int format);
/* Open audio playback device.
You have to keep track how many times you have called this function */
bool gutAudioOpen(int frequency, int channels, int bufsz);
void gutAudioClose(void);
bool gutAudioLoad(unsigned *index, const char *name);
/* Free slot and stop audio sample.
You don't have to use this unless you really don't need it anymore. */
bool gutAudioFree(unsigned index);
/* Play audio sample index on channel specified number of loops.
Use channel=-1 to choose a free channel.
Use loop=-1 to loop indefinitely. */
bool gutAudioPlay(unsigned index, int channel, int loops);
float gutAudioVolume(unsigned index, float volume);
void gutAudioPause(int channel);
void gutAudioResume(int channel);
void gutAudioHalt(int channel);
void gutAudioExpire(int channel, int ms);
void gutAudioFadeOut(int channel, int ms);

#endif
