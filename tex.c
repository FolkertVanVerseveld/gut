#include "_gut.h"
#include "gut.h"

#define PALSZ 256
static void _gut_set_palette(SDL_Surface *surf) {
	GLfloat r[PALSZ], g[PALSZ], b[PALSZ];
	int i, max = PALSZ;
	if (surf->format->palette->ncolors < max)
		max = surf->format->palette->ncolors;
	for (i = 0; i < max; ++i) {
		r[i] = (GLfloat)surf->format->palette->colors[i].r/PALSZ;
		g[i] = (GLfloat)surf->format->palette->colors[i].g/PALSZ;
		b[i] = (GLfloat)surf->format->palette->colors[i].b/PALSZ;
	}
	/* nvidia needs this, dunno if this is nvidia specific */
	glPixelTransferi(GL_MAP_COLOR, 1);
	glPixelMapfv(GL_PIXEL_MAP_I_TO_R, max, r);
	glPixelMapfv(GL_PIXEL_MAP_I_TO_G, max, g);
	glPixelMapfv(GL_PIXEL_MAP_I_TO_B, max, b);
}

static inline bool _gut_ispow2(unsigned x) {
	return x && !(x & (x - 1));
}

void _gut_maptex(SDL_Surface *surf, GLuint *tex) {
	GLint internal;
	GLenum format;
	glBindTexture(GL_TEXTURE_2D, *tex);
	if (surf->format->palette) {
		_gut_set_palette(surf);
		internal = GL_RGBA;
		format = GL_COLOR_INDEX;
	} else {
		// jpeg *needs* GL_RGB or else we get a segfault
		// png with GL_RGBA looks fine
		internal = (!surf->format->Amask || surf->format->BitsPerPixel == 24) ? GL_RGB : GL_RGBA,
		format = internal;
	}
	glTexImage2D(
		GL_TEXTURE_2D, 0,
		internal, surf->w, surf->h,
		0, format, GL_UNSIGNED_BYTE, surf->pixels
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	SDL_FreeSurface(surf);
}

static SDL_Surface *_gut_gettex(const char *name) {
	SDL_Surface *surf = IMG_Load(name);
	if (surf) return surf;
	gut.core->errtype |= GUT_ERR_IMG;
	++gut.core->errors;
	gutPerror(__func__);
	return NULL;
}

static inline unsigned _gut_nextpow2(unsigned x) {
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	++x;
	return x;
}

bool _gut_resizesurf(SDL_Surface **surf, unsigned size);

bool _gut_dirtymap(SDL_Surface **surf) {
	if ((*surf)->w != (*surf)->h || !_gut_ispow2((unsigned)(*surf)->w)) {
		unsigned max = (int) ((*surf)->w > (*surf)->h ? (*surf)->w : (*surf)->h);
		if (max < GUT_MIN_TEXTURE_SIZE)
			max = GUT_MIN_TEXTURE_SIZE;
		max = _gut_nextpow2(max);
		if (max > GUT_MAX_TEXTURE_SIZE) {
			++gut.core->errors;
			fprintf(stderr, "gut: %s: %s\n", "bad texture", "too big");
			return false;
		}
		if (!_gut_resizesurf(surf, max)) {
			++gut.core->errors;
			fprintf(stderr, "gut: %s: %s\n", "bad texture", "not power of two");
			return false;
		}
	}
	return true;
}

bool gutLoadTexture(GLuint *tex, const char *name) {
	SDL_Surface *surf = _gut_gettex(name);
	if (!surf) return false;
	if (surf->w != surf->h || !_gut_ispow2((unsigned)surf->w)) {
		++gut.core->errors;
		fprintf(stderr, "gut: %s: %s\n", "bad texture", "not power of two");
		return false;
	}
	_gut_maptex(surf, tex);
	return true;
}

bool gutLoadTextureDirty(GLuint *tex, const char *name) {
	SDL_Surface *surf = _gut_gettex(name);
	if (!surf || !_gut_dirtymap(&surf)) return false;
	_gut_maptex(surf, tex);
	return true;
}

bool gutLoadTextureResized(GLuint *tex, const char *name, GLsizei *resized, GLsizei *width, GLsizei *height) {
	SDL_Surface *surf = _gut_gettex(name);
	if (!surf) return false;
	if (width) *width = surf->w;
	if (height) *height = surf->h;
	if (!_gut_dirtymap(&surf)) return false;
	if (resized) *resized = surf->w;
	_gut_maptex(surf, tex);
	return true;
}

bool _gut_resizesurf(SDL_Surface **surf, unsigned size) {
	SDL_Surface *orig = *surf;
	SDL_Surface *new = SDL_CreateRGBSurface(
		orig->flags, size, size, 32,
		orig->format->Rmask, orig->format->Gmask, orig->format->Bmask, orig->format->Amask
	);
	if (!new) return false;
	size_t nrow, orow, nlen, olen;
	orow = (unsigned)orig->pitch;
	olen = (unsigned)orig->w * (unsigned)orig->h * orig->format->BytesPerPixel;
	nrow = (unsigned)new->pitch;
	nlen = (unsigned)new->w * (unsigned)new->h * new->format->BytesPerPixel;
	SDL_LockSurface(orig);
	SDL_LockSurface(new);
	// clear new pixel data
	memset(new->pixels, 0, nlen);
	// copy pixel data
	for (size_t opos = 0, npos = 0; opos < olen; opos += orow, npos += nrow)
		memcpy(&((Uint8*)new->pixels)[npos], &((Uint8*)orig->pixels)[opos], orow);
	SDL_UnlockSurface(new);
	SDL_UnlockSurface(orig);
	// replace surface
	*surf = new;
	SDL_FreeSurface(orig);
	return true;
}

bool gutLoadTexturePreciseBounds(GLuint *tex, const char *name, GLsizei w, GLsizei h, GLsizei *width, GLsizei *height) {
	SDL_Surface *surf = _gut_gettex(name);
	if (!surf) return false;
	if (surf->w != w || surf->h != h) {
		SDL_FreeSurface(surf);
		++gut.core->errors;
		fprintf(stderr, "gut: %s: expected dimension %d,%d but got %d,%d\n", "bad texture", w, h, surf->w, surf->h);
		return false;
	}
	if (!_gut_dirtymap(&surf))
		return false;
	_gut_maptex(surf, tex);
	if (width) *width = surf->w;
	if (height) *height = surf->h;
	return true;
}
