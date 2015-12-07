#ifndef GUT_TTF_H
#define GUT_TTF_H

#include <stdbool.h>
#include <GL/gl.h>

bool gutFontLoad(unsigned *index, const char *path, unsigned ptsize);
unsigned gutFontHeight(unsigned index);
bool gutFontFree(unsigned index);
bool gutFontForeground3ub(unsigned index, GLubyte r, GLubyte g, GLubyte b);
bool gutFontForeground3f(unsigned index, GLfloat r, GLfloat g, GLfloat b);
bool gutFontBackground3ub(unsigned index, GLubyte r, GLubyte g, GLubyte b);
bool gutFontBackground3f(unsigned index, GLfloat r, GLfloat g, GLfloat b);
#define GUT_FONT_SOLID 0
#define GUT_FONT_SHADED 1
#define GUT_FONT_BLENDED 2
#define GUT_FONT_MAX GUT_FONT_BLENDED
#define gutFontRenderText(index,tex,text,type) gutFontRenderTextSize(index,tex,text,type,NULL,NULL)
bool gutFontRenderTextSize(unsigned index, GLuint tex, const char *text, unsigned type, unsigned *width, unsigned *height);

#endif
