#ifndef GUT_GUT_H
#define GUT_GUT_H

#include <stddef.h>
#include <stdbool.h>
#include <GL/gl.h>
#include "sfx.h"
#include "ttf.h"
#include "tex.h"

struct gutcore_t;

#define GUT_MOUSE_LEFT 1
#define GUT_MOUSE_MIDDLE 2
#define GUT_MOUSE_RIGHT 4

#define GUT_KEY_RIGHT 0x1000
#define GUT_KEY_UP 0x1001
#define GUT_KEY_LEFT 0x1002
#define GUT_KEY_DOWN 0x1003

#define GUT_KEY_F1 0x1004
#define GUT_KEY_F(x) (GUT_KEY_F1+(x)-1)

#define GUT_IS_ARROW_KEY(key) ((key)>=GUT_KEY_RIGHT&&(key)<=GUT_KEY_DOWN)

typedef struct {
	uint8_t r, g, b;
} GutColor;

/* Configuration settings for current state.
This is only for reference information.
You should not modify these directly,
as these members may change over time.
Critical information is not kept here anyway,
so it is not a big deal if you change them. */
typedef struct gutconf_t {
	struct gutcore_t *core;
	struct {
		unsigned width;
		unsigned height;
		const char *title;
		unsigned flags;
	} window;
	struct {
		const char *title;
		int status;
	} abort;
	bool running;
	struct {
		int x, y;
		unsigned state;
	} mold;
	void (*idle)(unsigned);
	void (*reshape)(unsigned, unsigned);
	void (*display)(void);
	void (*key_down)(unsigned);
	void (*key_up)(unsigned);
	void (*motion)(int, int);
	void (*mouse)(unsigned);
	struct {
		struct {
			unsigned low, high, now;
		} fps;
		unsigned frames;
	} stats;
	struct {
		GutColor background;
		GutColor text;
		// placeholder for future colors
		GutColor max;
		struct {
			GutColor border;
			GutColor background;
			GutColor hover;
		} button;
	} msgboxcol;
} GutConfig;

extern GutConfig gut;

#define GUT_SHOW_LIST_MAX 8

typedef enum {
	GUT_SHOW_TYPE_ERROR = 1,
	GUT_SHOW_TYPE_WARN,
	GUT_SHOW_TYPE_INFO
} GutShowType;

/* Initialisation/Shutdown routines */

int gutInit(int *argc, char **argv);
#define gutIdleFunc(f) gut.idle=f
#define gutReshapeFunc(f) gut.reshape=f
#define gutDisplayFunc(f) gut.display=f
#define gutKeyDownFunc(f) gut.key_down=f
#define gutKeyUpFunc(f) gut.key_up=f
#define gutMotionFunc(f) gut.motion=f
#define gutButtonFunc(f) gut.mouse=f
void gutExit(int status) __attribute__ ((noreturn));

/* Try to sleep for specified milliseconds.
It returns the number of elapsed milliseconds.
Very small amounts (<15) are not reliable on many non-linux OS'es! */
unsigned gutSleep(unsigned ms);

#define GUT_ESCAPE_AUTO_CLOSE 1
#define GUT_FULLSCREEN 2
#define GUT_FULLSCREEN_DESKTOP 4
#define GUT_HIDDEN 8
#define GUT_BORDERLESS 16
#define GUT_RESIZABLE 32
#define GUT_MINIMIZED 64
#define GUT_MAXIMIZED 128
#define GUT_GRAB_INPUT 256
#define GUT_ALLOW_HIGHDPI 512
#define GUT_NO_AUTO_VIEWPORT 1024
#define GUT_NO_VSYNC 2048
#define GUT_WINDOW_MASK ((GUT_WINDOW_MAX - 1) | GUT_WINDOW_MAX)
#define GUT_WINDOW_MAX GUT_NO_VSYNC

#define gutGetWindowFlags() gut.window.flags
unsigned gutSetWindowFlags(unsigned flags);

/* List of window modes
fast fullscreen switches the fastest,
fullscreen desktop is next,
fullscreen is slow and evil with multiple monitors attached */
#define GUT_MODE_WINDOWED 0
#define GUT_MODE_FULLSCREEN 1
#define GUT_MODE_FULLSCREEN_DESKTOP 2
#define GUT_MODE_FAST_FULLSCREEN 3
#define GUT_MODE_MAX GUT_MODE_FAST_FULLSCREEN

/* Query window or display properties.
You must create a window first before you can use these. */
bool gutGetDisplayCount(unsigned *count);
bool gutGetDisplayBounds(unsigned index, unsigned *width, unsigned *height);
bool gutGetDisplayIndex(unsigned *index);
bool gutGetWindowPosition(unsigned *x, unsigned *y);
bool gutGetWindowSize(unsigned *width, unsigned *height);
bool gutGetWindowBounds(unsigned *x, unsigned *y, unsigned *width, unsigned *height);
bool gutSetWindowPosition(unsigned x, unsigned y);
bool gutSetWindowSize(unsigned width, unsigned height);
bool gutSetWindowBounds(unsigned x, unsigned y, unsigned width, unsigned height);
bool gutSetWindowTitle(const char *title);
bool gutSetWindowIcon(const char *path);
bool gutCenterWindow(void);
/* Change window mode and return mode that has been set.
If it is the different from the requested mode,
the window mode could not be changed or is unavailable */
unsigned gutSetWindowMode(unsigned mode);
// Reassign default fullscreen modus and return previous setting
unsigned gutSetFullscreenMode(unsigned mode);
bool gutIsFullscreen(void);
bool gutToggleWindowMode(void);
bool gutCreateWindow(const char *title, unsigned width, unsigned height);
int gutMainLoop(void);

/* If gutIsError yields true: prints lib specific error using specified message. */
void gutPerror(const char *message);

#define gutMsgBoxSet(color,r,g,b) gutColorSet(&gut.msgboxcol. color,r,g,b)
/* Update color used in gutShowList.
See GutConfig.msgboxcol for all colors */
void gutColorSet(GutColor *color, uint8_t r, uint8_t g, uint8_t b);
#define GUT_SCHEMESZ 4
#define GUT_SCHEME_GRIEFED 0
#define GUT_SCHEME_DARK 1
#define GUT_SCHEME_EYE_RAPE 2
#define GUT_SCHEME_HAX 3
bool gutSchemeSet(unsigned n);

/* User input routines */

void gutShowMessage(const char *title, const char *message);
void gutShowWarning(const char *title, const char *message);
void gutShowError(const char *title, const char *message);
/* Ask for option using specified list.
def is the highlighted element and escape returns cancel.
list must have at least buttons as number of elements. */
unsigned gutShowList(
	const char *title, const char *message, GutShowType type,
	unsigned buttons, unsigned def, unsigned cancel, const char **list
);

/* Graphics routines */
void gutPerspective(GLdouble fovy, GLdouble aspect, GLdouble znear, GLdouble zfar);
#define getGetFPSLow() gut.stats.fps.low
#define getGetFPSHigh() gut.stats.fps.high
#define gutGetFPSNow() gut.stats.fps.now

/* User interface routines */
bool gutShowCursor(void);
bool gutHideCursor(void);
bool gutToggleCursor(void);
void gutWarpMouse(void);

/* Abort routines */

void gutPanic(const char *title, const char *message) __attribute__ ((noreturn));
#define gutAssert(expr) gutAssertMessage(expr, gut.abort.title, gutStr(expr) ", file " __FILE__ ", line " __LINE__)
void gutAssertSet(const char *title, int status);
void gutAssertMessage(int expr, const char *title, const char *message);

/* Check for OpenGL errors and print if we have found an error
it just calls gutIsErrorp which will also print the current location */
#define gutIsError() gutIsErrorp(__FILE__,__LINE__,__func__)
bool gutIsErrorp(const char *file, const size_t line, const char *func);

#endif
