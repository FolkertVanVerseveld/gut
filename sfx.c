#include "sfx.h"
#include "_gut.h"
#include "gut.h"

int gutAudioSetFormat(int format) {
	int flags = 0, got;
	if (gut.core->flags & CTL_MIX_INIT)
		return gut.core->audio.format;
	if (gut.core->audio.format == format) {
		if (!format) {
			got = Mix_Init(0);
			gut.core->flags |= CTL_MIX_INIT;
			return got;
		}
		return format;
	}
	if (format & GUT_AUDIO_FLAC) flags |= MIX_INIT_FLAC;
	if (format & GUT_AUDIO_MOD ) flags |= MIX_INIT_MOD;
	if (format & GUT_AUDIO_MP3 ) flags |= MIX_INIT_MP3;
	if (format & GUT_AUDIO_OGG ) flags |= MIX_INIT_OGG;
	flags = Mix_Init(flags);
	gut.core->audio.format = format;
	gut.core->flags |= CTL_MIX_INIT;
	got = 0;
	if (flags & MIX_INIT_FLAC) got |= MIX_INIT_FLAC;
	if (flags & MIX_INIT_MOD ) got |= MIX_INIT_MOD;
	if (flags & MIX_INIT_MP3 ) got |= MIX_INIT_MP3;
	if (flags & MIX_INIT_OGG ) got |= MIX_INIT_OGG;
	if (got != format) {
		gut.core->errtype |= GUT_ERR_MIX;
		++gut.core->errors;
	}
	return got;
}

bool gutAudioOpen(int frequency, int channels, int bufsz) {
	if (!(gut.core->flags & CTL_MIX_INIT)) {
		puts("no mix");
		return false;
	}
	return Mix_OpenAudio(frequency, MIX_DEFAULT_FORMAT, channels, bufsz) == 0;
}

void gutAudioClose(void) {
	if (gut.core->flags & CTL_MIX_INIT)
		Mix_CloseAudio();
}

bool gutAudioLoad(unsigned *index, const char *name) {
	void *blk = NULL;
	bool good = false;
	uint8_t p = gut.core->audio.max;
	if (!index) goto err;
	if (gut.core->audio.max == UINT8_MAX) {
		fputs("gut: audio cache is full\n", stderr);
		goto err;
	}
	blk = Mix_LoadWAV(name);
	if (!blk) goto err;
	if (gut.core->audio.rpos)
		p = gut.core->audio.pop[--gut.core->audio.rpos];
	else
		++gut.core->audio.max;
	GutClip *clip = &gut.core->audio.list[p];
	clip->cookie = blk;
	clip->flags = CLIP_SFX | CLIP_LOADED;
	good = true;
err:
	if (!good) {
		if (blk) free(blk);
	}
	return good;
}

bool _gut_chksfx(GutClip **clip, unsigned index) {
	if (index > gut.core->audio.max)
		return false;
	*clip = &gut.core->audio.list[index];
	if (!((*clip)->flags & CLIP_LOADED))
		return false;
	return true;
}

bool gutAudioFree(unsigned index) {
	GutClip *clip;
	if (!_gut_chksfx(&clip, index))
		return false;
	if (Mix_Playing(clip->channel))
		Mix_HaltChannel(clip->channel);
	if (clip->flags & CLIP_SFX) {
		Mix_FreeChunk(clip->cookie);
		clip->flags &= ~CLIP_SFX;
	}
	clip->flags &= ~CLIP_LOADED;
	gut.core->audio.pop[gut.core->audio.rpos++] = index;
	return true;
}

bool gutAudioPlay(unsigned index, int channel, int loops) {
	GutClip *clip;
	if (!_gut_chksfx(&clip, index))
		return false;
	int ch = Mix_PlayChannel(channel, clip->cookie, loops);
	if (ch == -1)
		return false;
	clip->channel = ch;
	return true;
}

float gutAudioVolume(unsigned index, float volume) {
	GutClip *clip;
	if (!_gut_chksfx(&clip, index))
		return 0.0f;
	int vol = (int) (MIX_MAX_VOLUME * volume);
	Mix_Chunk *chunk = clip->cookie;
	return (float) MIX_MAX_VOLUME / Mix_VolumeChunk(chunk, vol);
}

void gutAudioPause(int channel) {
	Mix_Pause(channel);
}

void gutAudioResume(int channel) {
	Mix_Resume(channel);
}

void gutAudioHalt(int channel) {
	Mix_HaltChannel(channel);
}

void gutAudioExpire(int channel, int ms) {
	Mix_ExpireChannel(channel, ms);
}

void gutAudioFadeOut(int channel, int ms) {
	Mix_FadeOutChannel(channel, ms);
}
