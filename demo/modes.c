#include "common.h"

#define INTERPOLATE_STEPS 100

typedef GLfloat color[3];

struct {
	unsigned ms;
	color old, new;
	unsigned timer;
} demo;

void randomize(void);

void tick(unsigned n) {
	demo.timer += n;
	if (demo.timer >= INTERPOLATE_STEPS) {
		randomize();
		demo.timer %= INTERPOLATE_STEPS;
	}
}

void idle(unsigned ms) {
	demo.ms += ms;
	if (demo.ms > TICK_INTERVAL) {
		tick(demo.ms / TICK_INTERVAL);
		demo.ms %= TICK_INTERVAL;
	}
}

void interpolate(color diff, color old, color new, float alpha) {
	GLfloat dr, dg, db;
	dr = new[0] - old[0];
	dg = new[1] - old[1];
	db = new[2] - old[2];
	diff[0] = old[0] + alpha * dr;
	diff[1] = old[1] + alpha * dg;
	diff[2] = old[2] + alpha * db;
}

void display(void) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// compute interpolated color
	color col;
	interpolate(
		col, demo.old, demo.new,
		demo.timer / (float) INTERPOLATE_STEPS
	);
	glColor3f(col[0], col[1], col[2]);
	glBegin(GL_QUADS);
	glVertex2f(0, 1);
	glVertex2f(1, 1);
	glVertex2f(1, 0);
	glVertex2f(0, 0);
	glEnd();
}


void randomize(void) {
	demo.old[0] = demo.new[0];
	demo.old[1] = demo.new[1];
	demo.old[2] = demo.new[2];
#define rndCol ((rand()&0xff)/255.0f)
	demo.new[0] = rndCol;
	demo.new[1] = rndCol;
	demo.new[2]= rndCol;
#undef rndCol
}

void init(void) {
	srand(time(NULL));
	randomize();
}

void downie(unsigned key) {
	if (key == 'f' || key == 'F')
		gutToggleWindowMode();
}

int main(int argc, char **argv) {
	gutInit(&argc, argv);
	gutDisplayFunc(display);
	gutIdleFunc(idle);
	gutKeyDownFunc(downie);
	gutSetWindowFlags(GUT_FULLSCREEN_DESKTOP | GUT_ESCAPE_AUTO_CLOSE | GUT_BORDERLESS);
	gutHideCursor();
	gutCreateWindow("Window modes", DISPLAY_WIDTH, DISPLAY_HEIGHT);
	init();
	return gutMainLoop();
}
