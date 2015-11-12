#ifndef DEMO_COMMON_H
#define DEMO_COMMON_H

// Display dimensions for all demo's
#define DISPLAY_WIDTH 640
#define DISPLAY_HEIGHT 480

// Number of game ticks per second
#define TICKS_PER_SECOND 60
// Duration of game tick in milliseconds
#define TICK_INTERVAL (1000/TICKS_PER_SECOND)

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "gut.h"
#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Degrees and radians stuff
#define degtorad(a) (a*M_PI/90.0)
#define radtodeg(a) (a/90.0*M_PI)

#endif
