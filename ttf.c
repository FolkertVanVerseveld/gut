#include "ttf.h"
#include "_tex.h"
#include "gut.h"

static inline void _gut_fontfg(GutFont *this, GLubyte r, GLubyte g, GLubyte b) {
	this->fg.r = r;
	this->fg.g = g;
	this->fg.b = b;
	this->fg.a = 0;
}

static inline void _gut_fontbg(GutFont *this, GLubyte r, GLubyte g, GLubyte b) {
	this->bg.r = r;
	this->bg.g = g;
	this->bg.b = b;
	this->bg.a = 0;
}

bool gutFontLoad(unsigned *index, const char *path, unsigned ptsize) {
	TTF_Font *ttf = NULL;
	bool good = false;
	uint8_t p = gut.core->ttf.max;
	if (!index) goto err;
	if (gut.core->ttf.max == UINT8_MAX) {
		fputs("gut: font cache is full\n", stderr);
		goto err;
	}
	ttf = TTF_OpenFont(path, ptsize);
	if (!ttf) goto err;
	if (gut.core->ttf.rpos)
		p = gut.core->ttf.pop[--gut.core->ttf.rpos];
	else
		++gut.core->ttf.max;
	GutFont *font = &gut.core->ttf.list[p];
	font->ttf = ttf;
	font->ptsize = ptsize;
	font->flags = FONT_LOADED;
	font->height = TTF_FontHeight(ttf);
	_gut_fontfg(font, 255, 255, 255);
	_gut_fontbg(font, 0, 0, 0);
	*index = p;
	good = true;
err:
	if (!good) {
		if (ttf) TTF_CloseFont(ttf);
	}
	return good;
}

static bool _gut_chkttf(GutFont **font, unsigned index) {
	if (index > gut.core->ttf.max)
		return false;
	*font = &gut.core->ttf.list[index];
	return ((*font)->flags & FONT_LOADED);
}

bool gutFontForeground3ub(unsigned index, GLubyte r, GLubyte g, GLubyte b) {
	GutFont *font;
	if (!_gut_chkttf(&font, index))
		return false;
	_gut_fontfg(font, r, g, b);
	return true;
}

bool gutFontForeground3f(unsigned index, GLfloat r, GLfloat g, GLfloat b) {
	return gutFontForeground3ub(index, r * 255, g * 255, b * 255);
}

bool gutFontBackground3ub(unsigned index, GLubyte r, GLubyte g, GLubyte b) {
	GutFont *font;
	if (!_gut_chkttf(&font, index))
		return false;
	_gut_fontbg(font, r, g, b);
	return true;
}

bool gutFontBackground3f(unsigned index, GLfloat r, GLfloat g, GLfloat b) {
	return gutFontBackground3ub(index, r * 255, g * 255, b * 255);
}

unsigned gutFontHeight(unsigned index) {
	GutFont *font;
	if (!_gut_chkttf(&font, index))
		return 0;
	return font->height;
}

bool gutFontFree(unsigned index) {
	GutFont *font;
	if (!_gut_chkttf(&font, index))
		return false;
	TTF_CloseFont(font->ttf);
	font->flags &= ~FONT_LOADED;
	return true;
}

bool gutFontRenderTextSize(unsigned index, GLuint tex, const char *text, unsigned type, unsigned *width, unsigned *height) {
	SDL_Surface *surf = NULL;
	if (type > GUT_FONT_MAX)
		return false;
	GutFont *font;
	if (!_gut_chkttf(&font, index))
		return false;
	switch (type) {
	case GUT_FONT_SOLID:
		surf = TTF_RenderText_Solid(font->ttf, text, font->fg);
		break;
	case GUT_FONT_SHADED:
		surf = TTF_RenderText_Shaded(font->ttf, text, font->fg, font->bg);
		break;
	case GUT_FONT_BLENDED:
		surf = TTF_RenderText_Blended(font->ttf, text, font->fg);
		break;
	}
	//if (!surf || !_gut_dirtymap(&surf)) return false;
	if (!surf) return false;
	if (width) *width = surf->w;
	if (height) *height = surf->h;
	_gut_maptex(surf, tex);
	return true;
}
