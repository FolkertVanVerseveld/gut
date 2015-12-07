#ifndef GUT__GUT_H
#define GUT__GUT_H

#include <stdbool.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#define GUT_ERR_ENO 1
#define GUT_ERR_SDL 2
#define GUT_ERR_IMG 4
#define GUT_ERR_MIX 8
#define GUT_ERR_TTF 16

#define CTL_SDL_INIT 1
#define CTL_SDL_WINDOW 2
#define CTL_IMG_INIT 4
#define CTL_MIX_INIT 8
#define CTL_TTF_INIT 16

#define CLIP_SFX 1
#define CLIP_LOADED 2

#define FONT_LOADED 1

typedef struct {
	void *cookie;
	int channel;
	unsigned flags;
} GutClip;

typedef struct gutfont_t {
	TTF_Font *ttf;
	SDL_Color fg, bg;
	unsigned ptsize;
	unsigned height;
	unsigned flags;
} GutFont;

typedef struct gutcore_t {
	struct {
		SDL_Window *handle;
		SDL_GLContext context;
		SDL_Rect windowbounds;
		SDL_Rect physic;
		SDL_Surface *icon;
		unsigned fullmode;
		unsigned mode;
		unsigned display;
	} window;
	unsigned flags;
	unsigned errors;
	unsigned errtype;
	struct {
		Uint32 current;
		Uint32 last, next;
		Uint32 diff, sec;
		unsigned frames;
	} time;
	struct {
		struct {
			unsigned low, high, now;
		} fps;
		unsigned frames;
	} stats;
	struct {
		int format;
		uint8_t max, rpos;
		uint8_t pop[UINT8_MAX];
		GutClip list[UINT8_MAX];
	} audio;
	struct {
		uint8_t max, rpos;
		uint8_t pop[UINT8_MAX];
		GutFont list[UINT8_MAX];
	} ttf;
} GutCore;

extern GutCore _gut_core;

#endif
