#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "gut.h"

#define GUT_ERR_ENO 1
#define GUT_ERR_SDL 2
#define GUT_ERR_IMG 4
#define GUT_ERR_MIX 8

#define CTL_SDL_INIT 1
#define CTL_SDL_WINDOW 2
#define CTL_IMG_INIT 4
#define CTL_MIX_INIT 8

typedef struct gutcore_t {
	struct {
		SDL_Window *handle;
		SDL_GLContext context;
		SDL_Rect windowbounds;
		SDL_Rect physic;
		unsigned fullmode;
		unsigned mode;
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
	} audio;
} GutCore;

GutCore _gut_core = {
	.window = {
		.fullmode = GUT_MODE_FULLSCREEN_DESKTOP
	},
	.stats = {
		.fps = {
			.low = 1
		}
	}
};

GutConfig gut = {
	.window = {
		.title = "Is het niet mooi?",
		.flags = GUT_ESCAPE_AUTO_CLOSE,
	},
	.stats = {
		.fps = {
			.low = 1
		}
	},
	.core = &_gut_core,
	.abort = {
		.title = "Assertion failed",
		.status = 1
	}
};

#define SCHEMESZ 6

typedef GutColor GutScheme[SCHEMESZ];

GutScheme _gut_scheme[GUT_SCHEMESZ] = {
	[GUT_SCHEME_GRIEFED] =
	{
		{255, 0, 0},
		{0, 255, 0},
		{255, 255, 0},
		{0, 0, 255},
		{255, 0, 255},
		{0,   0, 0}
	},
	[GUT_SCHEME_DARK] = 
	{
		{0, 0, 0},
		{127, 127, 127},
		{127, 127, 127},
		{0, 0, 0},
		{255, 255, 255},
		{127, 127, 127},
	},
	[GUT_SCHEME_EYE_RAPE] = 
	{
		{255, 255, 255},
		{0, 0, 0},
		{127, 127, 127},
		{255, 255, 255},
		{127, 127, 127},
		{255, 255, 255}
	},
	[GUT_SCHEME_HAX] =
	{
		{0, 0, 0},
		{0, 255, 0},
		{0, 255, 0},
		{127, 63, 0},
		{0, 0, 255},
		{255, 255, 255}
	}
};

void gutExit(int status) {
	gut.running = false;
	exit(status);
}

static unsigned _gut_flags2sdl(unsigned gutflags, unsigned *mode);

#define MSGBTNSZ (sizeof(_gut_msgbtn)/sizeof(_gut_msgbtn[0]))
const char *_gut_msgbtn[] = {"OK"};

void gutShowMessage(const char *title, const char *message) {
	(void)gutShowList(
		title, message, GUT_SHOW_TYPE_INFO,
		MSGBTNSZ, 0, MSGBTNSZ, _gut_msgbtn
	);
}

void gutShowWarning(const char *title, const char *message) {
	(void)gutShowList(
		title, message, GUT_SHOW_TYPE_WARN,
		MSGBTNSZ, 0, MSGBTNSZ, _gut_msgbtn
	);
}

void gutShowError(const char *title, const char *message) {
	(void)gutShowList(
		title, message, GUT_SHOW_TYPE_ERROR,
		MSGBTNSZ, 0, MSGBTNSZ, _gut_msgbtn
	);
	++gut.core->errors;
}

static inline void _gut_getbnds(SDL_Rect *bounds) {
	SDL_GetDisplayBounds(
		SDL_GetWindowDisplayIndex(gut.core->window.handle),
		bounds
	);
}

static inline void _gut_setfps(GutCore *core) {
	if (core->time.sec < 1000)
		return;
	core->time.sec %= 1000;
	// compute frame rate
	core->stats.fps.now = core->stats.frames;
	core->stats.frames = 0;
	// if first time
	if (core->stats.fps.low > core->stats.fps.high) {
		core->stats.fps.low =
		core->stats.fps.high = core->stats.fps.now;
	}
	if (core->stats.fps.now < core->stats.fps.low)
		core->stats.fps.low = core->stats.fps.now;
	if (core->stats.fps.now > core->stats.fps.high)
		core->stats.fps.high = core->stats.fps.high;
}

static inline void _gut_savebnds(GutCore *core) {
	if (gut.core->window.mode != GUT_MODE_WINDOWED) {
		// bounds undefined, use desktop bounds
		SDL_Rect bounds;
		_gut_getbnds(&bounds);
		int x, y;
		x = (bounds.w - gut.window.width) / 2;
		if (x < 0) x = 0;
		y = (bounds.h - gut.window.height) / 2;
		if (y < 0) y = 0;
		core->window.windowbounds.x = x;
		core->window.windowbounds.y = y;
		core->window.windowbounds.w = gut.window.width;
		core->window.windowbounds.h = gut.window.height;
		return;
	}
	// recycle old bounds
	SDL_GetWindowPosition(
		core->window.handle,
		&core->window.windowbounds.x, &core->window.windowbounds.y
	);
	SDL_GetWindowSize(
		core->window.handle,
		&core->window.windowbounds.w, &core->window.windowbounds.h
	);
}

static void _gut_destroyContext(SDL_GLContext *context) {
	if (context) {
		SDL_GL_DeleteContext(context);
		context = NULL;
	}
}

static void _gut_destroyWindow(SDL_Window **window) {
	if (*window) {
		SDL_DestroyWindow(*window);
		*window = NULL;
	}
}

static void _gut_destroy(SDL_Window **window, SDL_GLContext *context) {
	_gut_destroyContext(context);
	_gut_destroyWindow(window);
}

#define fail(msg) do{\
	++gut.core->errors;\
	gutPerror(msg);\
	goto err;\
	}while(0)

#define error(msg) do{\
	gut.core->errtype|=GUT_ERR_ENO;\
	fail(msg);\
	}while(0)

#define sdl_error(msg) do{\
	gut.core->errtype|=GUT_ERR_SDL;\
	fail(msg);\
	}while(0)

#define wrap_sdl_call(status,op,msg,func,...) if (!(SDL_##func(__VA_ARGS__) op status)) sdl_error(msg)

static inline void _gut_updatePhysic(SDL_Window *window) {
	int w, h;
	w = gut.core->window.physic.w;
	h = gut.core->window.physic.h;
	SDL_GetWindowSize(
		window,
		&gut.core->window.physic.w,
		&gut.core->window.physic.h
	);
	if (gut.reshape &&
		(w != gut.core->window.physic.w || h != gut.core->window.physic.h)) {
		gut.reshape(gut.core->window.physic.w, gut.core->window.physic.h);
	}
	if (!(gut.window.flags & GUT_NO_AUTO_VIEWPORT)) {
		glViewport(
			0, 0,
			gut.core->window.physic.w,
			gut.core->window.physic.h
		);
	}
}

static bool _gut_initWindow(const char *title, unsigned width, unsigned height, unsigned sdlflags, SDL_Window **window, SDL_GLContext *context) {
	if (!(gut.core->flags & CTL_SDL_INIT))
		error("GUT not initialised");
	SDL_GLContext old = SDL_GL_GetCurrentContext();
	*window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		sdlflags
	);
	if (!*window)
		sdl_error("window could not be created");
	if (old) {
		SDL_GL_MakeCurrent(*window, old);
		*context = old;
	} else {
		context = SDL_GL_CreateContext(*window);
		if (!context)
			sdl_error("context could not be created");
	}
	wrap_sdl_call(0,==,"vsync not available", GL_SetSwapInterval, 1);
	_gut_updatePhysic(*window);
	return true;
err:
	_gut_destroy(
		&gut.core->window.handle,
		gut.core->window.context
	);
	return false;
}

static inline void _gut_setwinpos(GutCore *core) {
	SDL_SetWindowPosition(
		core->window.handle,
		core->window.windowbounds.x, core->window.windowbounds.y
	);
}

// create another window and discard old if success
static inline bool _gut_setwin(unsigned flags) {
	SDL_Window *window;
	SDL_GLContext context;
	unsigned dummy;
	if (!_gut_initWindow(
		gut.window.title,
		gut.window.width,
		gut.window.height,
		_gut_flags2sdl(flags, &dummy),
		&window,
		&context))
		return false;
	_gut_destroyWindow(&gut.core->window.handle);
	if (context != gut.core->window.context)
		_gut_destroyContext(gut.core->window.context);
	gut.core->window.context = context;
	gut.core->window.handle  = window;
	return true;
}

unsigned gutSetFullscreenMode(unsigned mode) {
	unsigned old = gut.core->window.fullmode;
	if (mode == GUT_MODE_WINDOWED || mode > GUT_MODE_MAX)
		goto done;
	gut.core->window.fullmode = mode;
done:
	return old;
}

bool gutIsFullscreen(void) {
	return gut.core->window.mode > GUT_MODE_WINDOWED;
}

bool gutToggleWindowMode(void) {
	return gut.core->window.mode == GUT_MODE_WINDOWED ?
		gutSetWindowMode(gut.core->window.fullmode) == gut.core->window.fullmode :
		gutSetWindowMode(GUT_MODE_WINDOWED) == GUT_MODE_WINDOWED;
}

unsigned gutSetWindowMode(unsigned mode) {
	unsigned flags = gut.window.flags;
	// get rid of modes
	flags &= ~(GUT_FULLSCREEN | GUT_FULLSCREEN_DESKTOP);
	if (mode > GUT_MODE_MAX || !(gut.core->flags & CTL_SDL_WINDOW))
		goto done;
	if (gut.core->window.mode == GUT_MODE_WINDOWED)
		_gut_savebnds(gut.core);
	switch (mode) {
	case GUT_MODE_WINDOWED:
		if (gut.core->window.mode == GUT_MODE_FAST_FULLSCREEN) {
			if (!(gut.window.flags & GUT_BORDERLESS)) {
				SDL_SetWindowBordered(
					gut.core->window.handle,
					SDL_TRUE
				);
			}
			SDL_SetWindowSize(
				gut.core->window.handle,
				gut.core->window.windowbounds.w,
				gut.core->window.windowbounds.h
			);
		} else if (!_gut_setwin(flags))
			goto done;
		_gut_setwinpos(gut.core);
		break;
	case GUT_MODE_FULLSCREEN_DESKTOP:
		flags |= GUT_FULLSCREEN_DESKTOP;
	case GUT_MODE_FULLSCREEN: {
		if (mode == GUT_MODE_FULLSCREEN)
			flags |= GUT_FULLSCREEN;
		if (!_gut_setwin(flags))
			goto done;
		break;
	}
	case GUT_MODE_FAST_FULLSCREEN: {
		SDL_Rect bounds;
		_gut_getbnds(&bounds);
		SDL_SetWindowBordered(gut.core->window.handle, SDL_FALSE);
		SDL_SetWindowPosition(gut.core->window.handle, bounds.x, bounds.y);
		SDL_SetWindowSize(gut.core->window.handle, bounds.w, bounds.h);
		break;
	}
	default: goto done;
	}
	_gut_updatePhysic(gut.core->window.handle);
	gut.core->window.mode = mode;
done:
	return gut.core->window.mode;
}

bool gutShowCursor(void) {
	return SDL_ShowCursor(1) == 1;
}

bool gutHideCursor(void) {
	return SDL_ShowCursor(0) == 0;
}

bool gutToggleCursor(void) {
	if (SDL_ShowCursor(-1) == 1)
		return gutHideCursor();
	return gutShowCursor();
}

static inline void _gut_cpycol(SDL_MessageBoxColor *dest, const GutColor *src) {
	dest->r = src->r;
	dest->g = src->g;
	dest->b = src->b;
}

static inline void _gut_cpyscheme(const GutScheme list) {
	gut.msgboxcol.background = list[0];
	gut.msgboxcol.text       = list[1];
	gut.msgboxcol.button.border     = list[2];
	gut.msgboxcol.button.background = list[3];
	gut.msgboxcol.button.hover      = list[4];
	gut.msgboxcol.max        = list[5];
}

bool gutSchemeSet(unsigned n) {
	if (n > GUT_SCHEMESZ)
		return false;
	_gut_cpyscheme(_gut_scheme[n]);
	return true;
}

void gutColorSet(GutColor *this, uint8_t r, uint8_t g, uint8_t b) {
	this->r = r;
	this->g = g;
	this->b = b;
}

unsigned gutShowList(const char *title, const char *message, GutShowType type, unsigned buttons, unsigned def, unsigned cancel, const char **list) {
	if (buttons > GUT_SHOW_LIST_MAX)
		gutPanic("Bad prompt", "Too many buttons");
	SDL_MessageBoxButtonData msgbut[GUT_SHOW_LIST_MAX];
	// grab scheme from gut.msgboxcol
	SDL_MessageBoxColorScheme scheme;
#define copy(dest,src) _gut_cpycol(&scheme.colors[SDL_MESSAGEBOX_COLOR_ ## dest], &gut.msgboxcol.src)
	copy(BACKGROUND, background);
	copy(TEXT, text);
	copy(MAX, max);
	copy(BUTTON_BORDER, button.border);
	copy(BUTTON_BACKGROUND, button.background);
	copy(BUTTON_SELECTED, button.hover);
#undef copy
	SDL_MessageBoxData msgdata = {
		0,
		gut.core->window.handle,
		title, message,
		buttons, msgbut, &scheme
	};
	switch (type) {
	case GUT_SHOW_TYPE_ERROR:
		msgdata.flags = SDL_MESSAGEBOX_ERROR;
		break;
	case GUT_SHOW_TYPE_WARN:
		msgdata.flags = SDL_MESSAGEBOX_WARNING;
		break;
	case GUT_SHOW_TYPE_INFO:
		msgdata.flags = SDL_MESSAGEBOX_INFORMATION;
		break;
	default:
		gutPanic("Bad prompt", "Invalid type");
	}
	for (unsigned i = 0; i < buttons; ++i) {
		msgbut[i].flags = 0;
		msgbut[i].buttonid = i;
		msgbut[i].text = list[i];
	}
	if (def < buttons)
		msgbut[def].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
	if (cancel < buttons)
		msgbut[cancel].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
	int ret;
	if (SDL_ShowMessageBox(&msgdata, &ret) < 0) {
		fputs("gutShowList failed\n", stderr);
		exit(1);
	}
	return ret >= 0 ? (unsigned) ret : def;
}

void gutAssertSet(const char *title, int status) {
	gut.abort.title = title;
	gut.abort.status = status;
}

void gutAssertMessage(int expr, const char *title, const char *message) {
	if (expr) gutPanic(title, message);
}

void gutPanic(const char *title, const char *message) {
	gutShowError(title, message);
	exit(gut.abort.status);
}

static void _gut_stop_mix(GutCore *core) {
	if (!(core->flags & CTL_MIX_INIT))
		return;
	Uint16 dummy2;
	for (int dummy, n = Mix_QuerySpec(&dummy, &dummy2, &dummy); n > 0; --n)
		Mix_CloseAudio();
	Mix_Quit();
	core->flags &= ~CTL_MIX_INIT;
}

static void _gut_stop_img(GutCore *core) {
	if (!(core->flags & CTL_IMG_INIT))
		return;
	IMG_Quit();
	core->flags &= ~CTL_IMG_INIT;
}

static void _gut_stop_sdl(GutCore *core) {
	if (!(core->flags & CTL_SDL_INIT))
		return;
	if (core->flags & CTL_SDL_WINDOW) {
		if (core->window.context) {
			SDL_GL_DeleteContext(core->window.context);
			core->window.context = NULL;
		}
		if (core->window.handle) {
			SDL_DestroyWindow(core->window.handle);
			core->window.handle = NULL;
		}
	}
	SDL_Quit();
	core->flags &= ~CTL_SDL_INIT;
}

static void _gut_stop(void) {
	GutCore *core = gut.core;
	_gut_stop_mix(core);
	_gut_stop_img(core);
	_gut_stop_sdl(core);
}

void gutPerror(const char *message) {
	// determine type
	unsigned type = gut.core->errtype;
	if (type & GUT_ERR_MIX)
		fprintf(stderr, "mix: %s: %s\n", message, Mix_GetError());
	if (type & GUT_ERR_IMG)
		fprintf(stderr, "img: %s: %s\n", message, IMG_GetError());
	if (type & GUT_ERR_SDL)
		fprintf(stderr, "sdl: %s: %s\n", message, SDL_GetError());
	if (type & GUT_ERR_ENO)
		perror(message);
}

/* apply modifiers to keycode
note: assumes qwerty keyboard layout
note: this code assumes range a-z as in ASCII */
static unsigned _gut_wrapkbp(SDL_Event *ev) {
	unsigned mod, key;
	mod = ev->key.keysym.mod;
	key = ev->key.keysym.sym;
	/* arrow keys */
	switch (key) {
	case SDLK_RIGHT: return GUT_KEY_RIGHT;
	case SDLK_UP: return GUT_KEY_UP;
	case SDLK_LEFT: return GUT_KEY_LEFT;
	case SDLK_DOWN: return GUT_KEY_DOWN;
	}
	if ((gut.window.flags & GUT_ESCAPE_AUTO_CLOSE) && key == 27) {
		gut.running = false;
		return 0;
	}
	if (mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
		unsigned char numup[] = {")!@#$%^&*("};
		unsigned mkey = key & 0xff;
		if (isprint(mkey)) {
			/* map ranged keys */
			if (mkey >= 'a' && mkey <= 'z')
				return mkey - 'a' + 'A';
			if (mkey >= '0' && mkey <= '9')
				return numup[mkey - '0'];
			/* map special keys */
			switch (mkey) {
			case '`': return '~';
			case '-': return '_';
			case '=': return '+';
			case '[': return '{';
			case ']': return '}';
			case '\\': return '|';
			case ';': return ':';
			case '\'': return '"';
			case ',': return '<';
			case '.': return '>';
			case '/': return '?';
			}
		}
	}
	return key;
}

int gutMainLoop(void) {
	const unsigned DELAY = 10;
	#define oops(msg) gutPanic("mainloop failed", msg)
	if (!(gut.core->flags & CTL_SDL_INIT))
		oops("GUT not initialised");
	if (!(gut.core->flags & CTL_SDL_WINDOW))
		oops("no window available or failed to create one");
	if (gut.window.width <= 0 || gut.window.height <= 0)
		oops("invalid window size");
	if (!gut.window.title)
		gut.window.title = "Game Utility Toolkit";
	if (!gut.display)
		oops("DisplayFunc not set");
	gut.running = true;
	gut.core->time.last = gut.core->time.current = SDL_GetTicks();
	while (gut.running) {
		SDL_Event ev;
		unsigned wrap;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_KEYDOWN:
				wrap = _gut_wrapkbp(&ev);
				if (!wrap) goto quit;
				if (gut.key_down)
					gut.key_down(wrap);
				break;
			case SDL_KEYUP:
				wrap = _gut_wrapkbp(&ev);
				if (!wrap) goto quit;
				if (gut.key_up)
					gut.key_up(wrap);
				break;
			case SDL_QUIT:
			quit:
				gut.running = false;
				return 0;
			}
		}
		gut.core->time.next = SDL_GetTicks();
		gut.core->time.diff = 
			gut.core->time.next < gut.core->time.last ?
			DELAY : gut.core->time.next - gut.core->time.last;
		gut.core->time.sec += gut.core->time.diff;
		if (gut.idle)
			gut.idle(gut.core->time.diff);
		if (gut.mouse) {
			unsigned mstate = 0;
			int mx, my;
			Uint32 state = SDL_GetMouseState(&mx, &my);
			if (state & SDL_BUTTON(SDL_BUTTON_LEFT))
				mstate |= GUT_MOUSE_LEFT;
			if (state & SDL_BUTTON(SDL_BUTTON_MIDDLE))
				mstate |= GUT_MOUSE_MIDDLE;
			if (state & SDL_BUTTON(SDL_BUTTON_RIGHT))
				mstate |= GUT_MOUSE_RIGHT;
			if (state != gut.mold.state)
				gut.mouse(state);
			gut.mold.state = state;
		}
		if (gut.motion) {
			int mx, my;
			(void)SDL_GetMouseState(&mx, &my);
			if (mx != gut.mold.x || my != gut.mold.y) {
				// project on virtual screen
				unsigned pw, ph, vw, vh, px, py;
				pw = gut.core->window.physic.w;
				ph = gut.core->window.physic.h;
				vw = gut.window.width;
				vh = gut.window.height;
				px = mx;
				py = my;
				mx = vw * px / pw;
				my = vh * py / ph;
				if (mx >= 0 && my >= 0 &&
					(unsigned)mx < vw && (unsigned)my < vh)
					gut.motion(mx, my);
			}
			gut.mold.x = mx;
			gut.mold.y = my;
		}
		_gut_updatePhysic(gut.core->window.handle);
		gut.display();
		gutIsError();
		SDL_GL_SwapWindow(gut.core->window.handle);
		gut.core->time.last = gut.core->time.next;
		++gut.stats.frames;
		++gut.core->stats.frames;
		_gut_setfps(gut.core);
		// copy stats to config
		gut.stats.fps.low  = gut.core->stats.fps.low;
		gut.stats.fps.high = gut.core->stats.fps.high;
		gut.stats.fps.now  = gut.core->stats.fps.now;
	}
	return 0;
	#undef oops
}

unsigned gutSetWindowFlags(unsigned flags) {
	if (flags > GUT_WINDOW_MAX)
		gut.window.flags &= GUT_WINDOW_MASK;
	if (!(gut.core->flags & CTL_SDL_WINDOW))
		return gut.window.flags = flags;
	// unmask soft flags
	unsigned new = gut.window.flags & ~(
		GUT_ESCAPE_AUTO_CLOSE |
		GUT_GRAB_INPUT |
		GUT_NO_AUTO_VIEWPORT
	);
	if (flags & GUT_ESCAPE_AUTO_CLOSE)
		new |= GUT_ESCAPE_AUTO_CLOSE;
	if (flags & GUT_NO_AUTO_VIEWPORT)
		new |= GUT_NO_AUTO_VIEWPORT;
	if (flags & GUT_GRAB_INPUT)
		new |= GUT_GRAB_INPUT;
	// set hard flags
	gut.window.flags = new != flags && _gut_setwin(flags) ? flags : new;
	// apply remaining
	SDL_SetWindowGrab(
		gut.core->window.handle,
		(flags & GUT_GRAB_INPUT) ? SDL_TRUE : SDL_FALSE
	);
	return gut.window.flags;
}

static unsigned _gut_flags2sdl(unsigned gutflags, unsigned *mode) {
	unsigned sdlflags = SDL_WINDOW_OPENGL;
	*mode = GUT_MODE_WINDOWED;
	if (gutflags & GUT_FULLSCREEN) {
		sdlflags |= SDL_WINDOW_FULLSCREEN;
		*mode = GUT_MODE_FULLSCREEN;
	}
	if (gutflags & GUT_FULLSCREEN_DESKTOP) {
		sdlflags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		*mode = GUT_MODE_FULLSCREEN_DESKTOP;
	}
	if (gutflags & GUT_HIDDEN)
		sdlflags |= SDL_WINDOW_HIDDEN;
	if (gutflags & GUT_BORDERLESS)
		sdlflags |= SDL_WINDOW_BORDERLESS;
	if (gutflags & GUT_RESIZABLE)
		sdlflags |= SDL_WINDOW_RESIZABLE;
	if (gutflags & GUT_MINIMIZED)
		sdlflags |= SDL_WINDOW_MINIMIZED;
	if (gutflags & GUT_MAXIMIZED)
		sdlflags |= SDL_WINDOW_MAXIMIZED;
	if (gutflags & GUT_GRAB_INPUT)
		sdlflags |= SDL_WINDOW_INPUT_GRABBED;
	if (gutflags & GUT_ALLOW_HIGHDPI)
		sdlflags |= SDL_WINDOW_ALLOW_HIGHDPI;
	return sdlflags;
}

bool gutCreateWindow(const char *title, unsigned width, unsigned height) {
	if (gut.core->flags & CTL_SDL_WINDOW)
		goto err;
	unsigned gutflags = gutGetWindowFlags();
	unsigned sdlflags, mode;
	gut.window.title = title;
	gut.window.width = width;
	gut.window.height = height;
	sdlflags = _gut_flags2sdl(gutflags, &mode);
	if (!_gut_initWindow(
		title, width, height,
		sdlflags,
		&gut.core->window.handle,
		&gut.core->window.context)) {
		goto err;
	}
	gut.core->window.mode = mode;
	_gut_savebnds(gut.core);
	gut.core->flags |= CTL_SDL_WINDOW;
	return true;
err:
	return false;
}

int gutInit(int *argc, char **argv) {
	(void)argc;
	(void)argv;
	atexit(_gut_stop);
	wrap_sdl_call(0,==,"init failed", Init, SDL_INIT_VIDEO);
	wrap_sdl_call(
		0,==,"double buffering not available",
		GL_SetAttribute, SDL_GL_DOUBLEBUFFER, 1
	);
	gut.core->flags |= CTL_SDL_INIT;
	int expected = IMG_INIT_PNG | IMG_INIT_JPG;
	gutSchemeSet(GUT_SCHEME_HAX);
	if ((IMG_Init(expected) & expected) != expected) {
		gut.core->errtype |= GUT_ERR_IMG;
		++gut.core->errors;
		gutPerror("init failed");
		goto err;
	}
	gut.core->flags |= CTL_IMG_INIT;
	return 0;
err:
	fputs("gut: init failed\n", stderr);
	fprintf(stderr, "error count: %u\n", gut.core->errors);
	return 1;
}

void gutPerspective(GLdouble fovy, GLdouble aspect, GLdouble znear, GLdouble zfar) {
	GLdouble fw, fh;
	fh = tan(fovy / 360 * M_PI) * znear;
	fw = fh * aspect;
	glFrustum(-fw, fw, -fh, fh, znear, zfar);
}

/* see macro in header, you may call this directly */
bool gutIsErrorp(const char *file, const size_t line, const char *function) {
	GLenum err;
	if ((err = glGetError()) != GL_NO_ERROR) {
		const char *str;
		switch (err) {
		case GL_INVALID_ENUM     : str = "invalid enum"; break;
		case GL_INVALID_VALUE    : str = "invalid value"; break;
		case GL_INVALID_OPERATION: str = "invalid operation"; break;
		case GL_STACK_OVERFLOW   : str = "stack overflow"; break;
		case GL_STACK_UNDERFLOW  : str = "stack underflow"; break;
		case GL_OUT_OF_MEMORY    : str = "out of memory"; break;
		default: str = "unknown"; break;
		}
		fprintf(
			stderr, "glerror: %s %s:%zu: %s (code: %x)\n",
			file, function, line, str, err
		);
		++gut.core->errors;
		return true;
	}
	return false;
}

void gutWarpMouse(void) {
	SDL_WarpMouseInWindow(
		gut.core->window.handle,
		gut.core->window.physic.w / 2,
		gut.core->window.physic.h / 2
	);
}

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
	if (resize) *resized = surf->w;
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
	//printf("row: %zu, length: %zu (%d, %d)\n", nrow, nlen, orig->w, orig->h);
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

int gutAudioSetFormat(int format) {
	int flags = 0, got;
	if (gut.core->flags & CTL_MIX_INIT)
		return gut.core->audio.format;
	if (gut.core->audio.format == format)
		return format;
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
	if (!(gut.core->flags & CTL_MIX_INIT))
		return false;
	return Mix_OpenAudio(frequency, MIX_DEFAULT_FORMAT, channels, bufsz) != 0;
}

void gutAudioClose(void) {
	if (gut.core->flags & CTL_MIX_INIT)
		Mix_CloseAudio();
}
