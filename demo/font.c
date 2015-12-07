#include "common.h"

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 200

GLuint tex[3];
GLuint tex_solid, tex_shaded, tex_blended;
unsigned font;
GLuint tex_current;

void display(void) {
	glClearColor(0, 0, 0, 0);
	// blended text has 4 bytes per pixel
	// if you forget to enable blending in GL
	// you'll see just rectangular boxes and no text!
	if (tex_current == tex_blended) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glBindTexture(GL_TEXTURE_2D, tex_current);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(0, 0);
	glTexCoord2f(1, 0); glVertex2f(1, 0);
	glTexCoord2f(1, 1); glVertex2f(1, 1);
	glTexCoord2f(0, 1); glVertex2f(0, 1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	if (tex_current == tex_blended)
		glDisable(GL_BLEND);
}

void init(void) {
	if (!gutFontLoad(&font, "simplto2.ttf", 40))
		gutPanic("Missing resources", "Could not find simplto2.ttf");
	glGenTextures(3, tex);
	gutFontBackground3f(font, 0.5, 0, 0);
	gutFontRenderText(font, tex_solid = tex[0], "this is fast: solid", GUT_FONT_SOLID);
	gutFontRenderText(font, tex_shaded = tex[1], "still ok: shaded", GUT_FONT_SHADED);
	gutFontRenderText(font, tex_blended = tex[2], "very slow: blended", GUT_FONT_BLENDED);
	tex_current = tex_solid;
}

void cleanup(void) {
	glDeleteTextures(3, tex);
}

void down(unsigned key) {
	switch (key) {
	case '1':
		tex_current = tex_solid;
		break;
	case '2':
		tex_current = tex_shaded;
		break;
	case '3':
		tex_current = tex_blended;
		break;
	case 'f':
	case 'F':
		gutToggleWindowMode();
		break;
	}
}

int main(int argc, char **argv) {
	int ret;
	gutInit(&argc, argv);
	gutDisplayFunc(display);
	gutKeyDownFunc(down);
	gutHideCursor();
	gutCreateWindow("Font", SCREEN_WIDTH, SCREEN_HEIGHT);
	init();
	ret = gutMainLoop();
	cleanup();
	return ret;
}
