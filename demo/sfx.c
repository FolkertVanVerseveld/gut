#include "common.h"

struct {
	unsigned ms;
	GLuint tex;
	unsigned jump;
	GLfloat a, y;
} demo;

void drawTexture(GLfloat x, GLfloat y, GLfloat scale) {
	glBindTexture(GL_TEXTURE_2D, demo.tex);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
	glTranslatef(x, y, 0);
	glScalef(scale, scale, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(-0.5, -0.5);
	glTexCoord2f(1, 0); glVertex2f(1 - 0.5, -0.5);
	glTexCoord2f(1, 1); glVertex2f(1 - 0.5, 1 - 0.5);
	glTexCoord2f(0, 1); glVertex2f(-0.5, 1 - 0.5);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	drawTexture(0.5, 0.5 + demo.y, .25);
}

void init(void) {
	gutAudioSetFormat(GUT_AUDIO_OGG);
	if (!gutAudioOpen(11025, 1, 1024))
		gutPanic("No audio", "Could not initialise audio");
	if (!gutAudioLoad(&demo.jump, "jumpland.wav"))
		gutPanic("No audio", "Could not find jumpland.wav");
	glGenTextures(1, &demo.tex);
	if (!gutLoadTexture(&demo.tex, "vim.png"))
		gutPanic("Missing resources", "Could not find vim.png");
}

void cleanup(void) {
	glDeleteTextures(1, &demo.tex);
	gutAudioFree(demo.jump);
}

void tick(unsigned n) {
	if (demo.y == 0)
		return;
	demo.y += demo.a;
	demo.a += n * 0.02;
	if (demo.y > 0) {
		demo.y = 0;
		demo.a = 0;
		gutAudioPlay(demo.jump, -1, 0);
	}
}

void idle(unsigned ms) {
	demo.ms += ms;
	if (demo.ms > TICK_INTERVAL) {
		tick(demo.ms / TICK_INTERVAL);
		demo.ms %= TICK_INTERVAL;
	}
}

void up(unsigned key) {
	if (key == ' ') {
		demo.a = 0;
		demo.y = -0.5;
	}
	if (key == 'f' || key == 'F')
		gutToggleWindowMode();
}

int main(int argc, char **argv) {
	int ret;
	gutInit(&argc, argv);
	gutIdleFunc(idle);
	gutDisplayFunc(display);
	gutKeyUpFunc(up);
	gutHideCursor();
	gutCreateWindow("Sound effects", DISPLAY_WIDTH, DISPLAY_HEIGHT);
	init();
	ret = gutMainLoop();
	cleanup();
	return ret;
}
