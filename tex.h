#ifndef GUT_TEX_H
#define GUT_TEX_H

#include <GL/gl.h>

#define GUT_MIN_TEXTURE_SIZE 2
#define GUT_MAX_TEXTURE_SIZE 2048

/* Load texture from file specified by name and store in tex.
Width must be a power of two and equal height.
You have to call glGenTextures or something similar before passing tex. */
bool gutLoadTexture(GLuint *tex, const char *name);
/* Load texture from file specified by name and store in tex.
Width does not have to be a power of two.
You have to call glGenTextures or something similar before passing tex. */
bool gutLoadTextureDirty(GLuint *tex, const char *name);
/* Load texture from file specified by name and store in tex.
The texture will be resized to a power of two if it isn't.
Original dimensions are stored in width and height.
New dimension will be stored in resized.
These pointers may be null pointers.
You have to call glGenTextures or something similar before passing tex. */
bool gutLoadTextureResized(GLuint *tex, const char *name, GLsizei *resized, GLsizei *width, GLsizei *height);
/* Load texture from file that exactly matches w and h and store in tex.
The texture will be resized to a power of two if it isn't.
width and/or height get the power of two dimensions (if not NULL).
You have to call glGenTextures or something similar before passing tex. */
bool gutLoadTexturePreciseBounds(GLuint *tex, const char *name, GLsizei w, GLsizei h, GLsizei *width, GLsizei *height);

#endif
