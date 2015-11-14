#include "common.h"

GLuint tex;

void display(void) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glBindTexture(GL_TEXTURE_2D, tex);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(0, 0);
	glTexCoord2f(1, 0); glVertex2f(1, 0);
	glTexCoord2f(1, 1); glVertex2f(1, 1);
	glTexCoord2f(0, 1); glVertex2f(0, 1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void init(void) {
	glGenTextures(1, &tex);
	if (!gutLoadTexture(&tex, "vim.png"))
		gutPanic("Missing resources", "Could not find vim.png");
}

void cleanup(void) {
	glDeleteTextures(1, &tex);
}

void downie(unsigned key) {
	if (key == 'f' || key == 'F')
		gutToggleWindowMode();
}

int main(int argc, char **argv) {
	int ret;
	gutInit(&argc, argv);
	gutDisplayFunc(display);
	gutKeyDownFunc(downie);
	gutHideCursor();
	gutCreateWindow("Textured cube", DISPLAY_WIDTH, DISPLAY_HEIGHT);
	init();
	ret = gutMainLoop();
	cleanup();
	return ret;
}
