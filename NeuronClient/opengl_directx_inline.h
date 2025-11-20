#pragma once

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

inline void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
#define CLAMPF2B(x) ((x<0)?0:((x>1)?255:(unsigned char)(x*255)))
  glColor4ub(CLAMPF2B(red),CLAMPF2B(green),CLAMPF2B(blue),CLAMPF2B(alpha));
#undef CLAMPF2B
}

inline void glColor4fv(const GLfloat* v) { glColor4f(v[0], v[1], v[2], v[3]); }

inline void glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
  glColor4f(red, green, blue, 1.0f);
}

inline void glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
  glColor4ub(red, green, blue, 255);
}

inline void glColor3ubv(const GLubyte* v)
{
  glColor4ub(v[0], v[1], v[2], 255);
}

inline void glColor4ubv(const GLubyte* v)
{
  glColor4ub(v[0], v[1], v[2], v[3]);
}

// NORMALS

void glNormal3f_impl(GLfloat nx, GLfloat ny, GLfloat nz);

inline void glNormal3fv(const GLfloat* v)
{
  glNormal3f_impl(v[0], v[1], v[2]);
}

inline void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
  glNormal3f_impl(nx, ny, nz);
}

// VERTICES

void glVertex3f_impl(GLfloat x, GLfloat y, GLfloat z);

inline void glVertex3fv(const GLfloat* v)
{
  glVertex3f_impl(v[0], v[1], v[2]);
}

inline void glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
  glVertex3f_impl(x, y, z);
}

inline void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
  glVertex3f_impl(x, y, z);
}

inline void glVertex2fv(const GLfloat* v)
{
  glVertex3f_impl(v[0], v[1], 0.0f);
}

inline void glVertex2f(GLfloat x, GLfloat y)
{
  glVertex3f_impl(x, y, 0.0f);
}

inline void glVertex2i(GLint x, GLint y)
{
  glVertex3f_impl(static_cast<float>(x), static_cast<float>(y), 0.0f);
}

// TEXTURE COORDS

void glTexCoord2f_impl(GLfloat u, GLfloat v);

inline void glTexCoord2f(GLfloat s, GLfloat t)
{
  glTexCoord2f_impl(s, t);
}

inline void glTexCoord2i(GLint s, GLint t)
{
  glTexCoord2f_impl(static_cast<float>(s), static_cast<float>(t));
}

// FOG

void glFogf_impl(GLenum pname, GLfloat param);
void glFogi_impl(GLenum pname, GLint param);

inline void glFogf(GLenum pname, GLfloat param)
{
  glFogf_impl(pname, param);
}

inline void glFogi(GLenum pname, GLint param)
{
  glFogi_impl(pname, param);
}
