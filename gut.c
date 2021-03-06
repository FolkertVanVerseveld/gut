#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "_gut.h"
#include "gut.h"
#include <SDL2/SDL_keycode.h>

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

unsigned gutSleep(unsigned ms) {
	Uint32 t = SDL_GetTicks();
	SDL_Delay(ms);
	return SDL_GetTicks() - t;
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

#define haswin() (gut.core->flags & CTL_SDL_WINDOW)
#define chkwin if(!haswin())return false

bool gutGetDisplayIndex(unsigned *display) {
	chkwin;
	int i;
	i = SDL_GetWindowDisplayIndex(gut.core->window.handle);
	if (i < 0) return false;
	gut.core->window.display = (unsigned) i;
	*display = (unsigned) i;
	return true;
}

static inline bool _gut_getbnds(SDL_Rect *bounds) {
	unsigned display;
	return gutGetDisplayIndex(&display) &&
		SDL_GetDisplayBounds(display, bounds) == 0;
}

bool gutGetDisplayCount(unsigned *count) {
	chkwin;
	int n = SDL_GetNumVideoDisplays();
	if (n < 1) return false;
	*count = n;
	return true;
}

bool gutGetDisplayBounds(unsigned index, unsigned *width, unsigned *height) {
	chkwin;
	SDL_Rect bnds;
	if (SDL_GetDisplayBounds((int) index, &bnds) == 0)
		return false;
	if (bnds.w < 0 || bnds.h < 0)
		return false;
	if (width) *width = bnds.w;
	if (height) *height = bnds.h;
	return true;
}

bool gutGetWindowPosition(unsigned *x, unsigned *y) {
	chkwin;
	int xp, yp;
	SDL_GetWindowPosition(
		gut.core->window.handle,
		&xp, &yp
	);
	if (xp < 0 || yp < 0)
		return false;
	if (x) *x = xp;
	if (y) *y = yp;
	return true;
}

bool gutGetWindowSize(unsigned *width, unsigned *height) {
	chkwin;
	int w, h;
	SDL_GetWindowSize(
		gut.core->window.handle,
		&w, &h
	);
	if (w < 0 || h < 0)
		return false;
	if (width) *width = w;
	if (height) *height = h;
	return true;
}

bool gutGetWindowBounds(unsigned *x, unsigned *y, unsigned *w, unsigned *h) {
	chkwin;
	unsigned xp, yp, wp, hp;
	if (!gutGetWindowSize(&wp ,&hp))
		return false;
	if (!gutGetWindowPosition(&xp, &yp))
		return false;
	if (x) *x = xp;
	if (y) *y = yp;
	if (w) *w = wp;
	if (h) *h = hp;
	return true;
}

bool gutSetWindowPosition(unsigned x, unsigned y) {
	chkwin;
	SDL_SetWindowPosition(gut.core->window.handle, x, y);
	return true;
}

bool gutSetWindowSize(unsigned w, unsigned h) {
	chkwin;
	/* prevent division by zero
	only checking for height should be sufficient,
	but width == 0 would be pointless anyway */
	if (w < 1 || h < 1)
		return false;
	SDL_SetWindowSize(gut.core->window.handle, w, h);
	gut.window.width = w;
	gut.window.height = h;
	return true;
}

bool gutSetWindowTitle(const char *title) {
	if (!(gut.core->flags & CTL_SDL_WINDOW))
		return false;
	SDL_SetWindowTitle(
		gut.core->window.handle,
		gut.window.title = title
	);
	return true;
}

bool gutCenterWindow(void) {
	chkwin;
	SDL_SetWindowPosition(
		gut.core->window.handle,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED
	);
	return true;
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
	// check if we have got a new record
	if (core->stats.fps.now < core->stats.fps.low)
		core->stats.fps.low = core->stats.fps.now;
	if (core->stats.fps.now > core->stats.fps.high)
		core->stats.fps.high = core->stats.fps.now;
}

static inline bool _gut_savebnds(GutCore *core) {
	if (gut.core->window.mode != GUT_MODE_WINDOWED) {
		// bounds undefined, use desktop bounds
		SDL_Rect bounds;
		if (!_gut_getbnds(&bounds))
			return false;
		int x, y;
		x = (bounds.w - gut.window.width) / 2;
		if (x < 0) x = 0;
		y = (bounds.h - gut.window.height) / 2;
		if (y < 0) y = 0;
		core->window.windowbounds.x = x;
		core->window.windowbounds.y = y;
		core->window.windowbounds.w = gut.window.width;
		core->window.windowbounds.h = gut.window.height;
		return true;
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
	return true;
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

#define img_error(msg) do{\
	gut.core->errtype|=GUT_ERR_IMG;\
	fail(msg);\
	}while(0)

#define sdl_error(msg) do{\
	gut.core->errtype|=GUT_ERR_SDL;\
	fail(msg);\
	}while(0)

bool gutSetWindowIcon(const char *path) {
	bool good = false;
	if (!(gut.core->flags & CTL_SDL_INIT))
		return false;
	SDL_Surface *icon = IMG_Load(path);
	if (!icon) img_error("invalid icon");
	gut.core->window.icon = icon;
	if (gut.core->flags & CTL_SDL_WINDOW)
		SDL_SetWindowIcon(gut.core->window.handle, icon);
	good = true;
err:
	return good;
}

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
	if (gut.core->window.icon)
		SDL_SetWindowIcon(*window, gut.core->window.icon);
	if (!(gut.window.flags & GUT_NO_VSYNC)) {
		wrap_sdl_call(0,==,"vsync not available", GL_SetSwapInterval, 1);
	}
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
	if (mode > GUT_MODE_MAX || !haswin())
		goto done;
	if (gut.core->window.mode == GUT_MODE_WINDOWED && !_gut_savebnds(gut.core))
		goto done;
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

static void _gut_stop_ttf(GutCore *core) {
	if (!(core->flags & CTL_TTF_INIT))
		return;
	TTF_Quit();
	core->flags &= ~CTL_TTF_INIT;
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
	_gut_stop_ttf(core);
	_gut_stop_mix(core);
	_gut_stop_img(core);
	_gut_stop_sdl(core);
}

void gutPerror(const char *message) {
	// determine type
	unsigned type = gut.core->errtype;
	if (type & GUT_ERR_TTF)
		fprintf(stderr, "ttf: %s: %s\n", message, TTF_GetError());
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
	#define SDL_KEYCODE_TO_SCANCODE(x) ((x) & ~(SDLK_SCANCODE_MASK))
	if (key >= SDLK_F1 && key <= SDLK_F12)
		return SDL_KEYCODE_TO_SCANCODE(key) - SDL_SCANCODE_F1 + GUT_KEY_F1;
	if (key >= SDLK_F13 && key <= SDLK_F24)
		return SDL_KEYCODE_TO_SCANCODE(key) - SDL_SCANCODE_F13 + GUT_KEY_F(13);
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
	if (mod & KMOD_SHIFT) {
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
	if ((key <= 0xff && !isprint(key)) && (mod & (KMOD_CTRL | KMOD_ALT)))
		return 1;
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
				if (wrap != 1 && gut.key_down)
					gut.key_down(wrap);
				break;
			case SDL_KEYUP:
				wrap = _gut_wrapkbp(&ev);
				if (!wrap) goto quit;
				if (wrap != 1 && gut.key_up)
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
		GUT_NO_AUTO_VIEWPORT |
		GUT_NO_VSYNC
	);
	if (flags & GUT_ESCAPE_AUTO_CLOSE)
		new |= GUT_ESCAPE_AUTO_CLOSE;
	if (flags & GUT_NO_AUTO_VIEWPORT)
		new |= GUT_NO_AUTO_VIEWPORT;
	if (flags & GUT_GRAB_INPUT)
		new |= GUT_GRAB_INPUT;
	if (flags & GUT_NO_VSYNC)
		new |= GUT_NO_VSYNC;
	if ((gut.core->flags & new & GUT_NO_VSYNC) == 0) {
		int mode = new & GUT_NO_VSYNC ? 0 : 1;
		SDL_GL_SetSwapInterval(mode);
		if (SDL_GL_GetSwapInterval() != mode)
			sdl_error("swap interval");
	}
	// set hard flags
	gut.window.flags = new != flags && _gut_setwin(flags) ? flags : new;
	// apply remaining
	SDL_SetWindowGrab(
		gut.core->window.handle,
		(flags & GUT_GRAB_INPUT) ? SDL_TRUE : SDL_FALSE
	);
err:
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
	// ignore whether bounds were saved correctly
	(void)_gut_savebnds(gut.core);
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
	if (TTF_Init() != 0) {
		gut.core->errtype |= GUT_ERR_TTF;
		++gut.core->errors;
		gutPerror("init failed");
		goto err;
	}
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
