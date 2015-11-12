#include "common.h"

typedef struct {
	GLfloat x0, y0;
	GLfloat x1, y1;
	GLfloat x2, y2;
} Triangle;

struct {
	unsigned ms;
	GLfloat r, g, b;
	Triangle t;
	int angle;
} demo;

void displayTriangle(Triangle *this) {
	glBegin(GL_TRIANGLES);
	glVertex2f(this->x0, this->y0);
	glVertex2f(this->x1, this->y1);
	glVertex2f(this->x2, this->y2);
	glEnd();
}

/* Do n game ticks.
It is very easy to write your game logic in game ticks.
This little demo just rotates the geometry in a frame rate independent way. */
void tick(unsigned n) {
	demo.angle -= n * 3;
	if (demo.angle <= -360)
		demo.angle = -(-demo.angle % 360);
}

/* Gets called when the game engine has nothing to do.
`ms' is the amount of milliseconds that have passed since the last call.
This little demo will determine how many game ticks have passed and calls tick() when necessary. */
void idle(unsigned ms) {
	demo.ms += ms;
	if (demo.ms > TICK_INTERVAL) {
		unsigned n = demo.ms / TICK_INTERVAL;
		demo.ms %= TICK_INTERVAL;
		tick(n);
	}
}

/* Renders the demo scene.
Just some simple OpenGL calls. */
void display(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-2.0f, 2.0f, -1.5f, 1.5f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GLfloat factor = 0.8 + 0.4 * (GLfloat) sin(degtorad(demo.angle / 2.0));
	glRotatef(demo.angle, 0, 0, 1);
	glScalef(factor, factor, 1);
	glColor3f(0.2f, 0.4f, 0.2f);
	displayTriangle(&demo.t);
}

/* Setup the game state */
void init(void) {
	srand(time(NULL));
	demo.r = 0.3f + 0.7f * (rand() & 0xff) / 255.0f;
	demo.g = 0.3f + 0.7f * (rand() & 0xff) / 255.0f;
	demo.b = 0.3f + 0.7f * (rand() & 0xff) / 255.0f;
	demo.t.x0 = demo.t.y0 = -0.5f;
	demo.t.x1 = 0.0f;
	demo.t.y1 = 0.5f;
	demo.t.x2 = 0.5f;
	demo.t.y2 = -0.5f;
	demo.angle = rand() % 360;
	glClearColor(demo.r, demo.g, demo.b, 0);
}

int main(int argc, char **argv) {
	gutInit(&argc, argv);
	gutDisplayFunc(display);
	gutIdleFunc(idle);
	gutCreateWindow("Hello GUT", DISPLAY_WIDTH, DISPLAY_HEIGHT);
	init();
	return gutMainLoop();
}
