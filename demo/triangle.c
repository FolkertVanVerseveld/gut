#include "common.h"
#include <stdio.h>

struct {
	unsigned ms;
	unsigned frames;
	int angle;
} demo;

void tick(unsigned n) {
	demo.angle += n * 3;
	if (demo.angle >= 360)
		demo.angle %= 360;
}

void idle(unsigned ms) {
	demo.ms += ms;
	if (demo.ms > TICK_INTERVAL) {
		unsigned n = demo.ms / TICK_INTERVAL;
		demo.ms %= TICK_INTERVAL;
		tick(n);
	}
}

void display(void) {
	++demo.frames;
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 1.0f);
	glRotatef(demo.angle, 0, 0, 1);
	glScalef(4, 4, 1);
	glBegin(GL_TRIANGLES);
	glColor3f(0.4f, 0.6f, 0.2f);
	glColor3f(0, 1, 0);
	glVertex2f(-50.0f, 50.0f);
	glColor3f(0.8f, 0.2f, 0.4f);
	glColor3f(1, 0, 0);
	glVertex2f(0.0f, -50.0f);
	glColor3f(0.1f, 0.2f, 0.7f);
	glColor3f(0, 0, 1);
	glVertex2f(50.0f, 50.0f);
	glEnd();
}

void init(void) {
	glClearColor(0, 0, 0, 0);
}

int main(int argc, char **argv) {
	gutInit(&argc, argv);
	gutDisplayFunc(display);
	gutIdleFunc(idle);
	gutCreateWindow("Simple triangle", DISPLAY_WIDTH, DISPLAY_HEIGHT);
	init();
	return gutMainLoop();
}
