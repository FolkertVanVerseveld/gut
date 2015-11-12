#include "common.h"

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT);
}

void init(void) {
	const char *list[] = {"yes", "enough", "next"};
	gutShowMessage("Welcome", "This example will show you message boxes");
	gutShowMessage("Types", "GUT supports three different message types:\nERROR, WARN, and INFO");
	gutShowMessage("Important", "It is important to note that the game\nwill freeze if you use these message boxes");
	gutShowMessage("Info", "This is an information message");
	gutShowWarning("Warning", "This is a warning message");
	gutShowError("Error", "This is an error message");
	for (unsigned i = 0; i < GUT_SCHEMESZ; ++i) {
		gutSchemeSet(i);
		switch (gutShowList("Yo!", "nice scheme ain't it?", GUT_SHOW_TYPE_INFO, 3, 0, 1, list)) {
		case 0:
			gutShowMessage("Nice", "Glad you like it!");
		case 1:
			goto done;
		}
	}
	gutShowError("Last scheme", "There are no next schemes");
done:
	gutExit(0);
}

int main(int argc, char **argv) {
	gutInit(&argc, argv);
	gutDisplayFunc(display);
	gutCreateWindow("Message boxes", DISPLAY_WIDTH, DISPLAY_HEIGHT);
	init();
	return gutMainLoop();
}
