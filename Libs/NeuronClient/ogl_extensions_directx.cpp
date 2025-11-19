#include "pch.h"

#ifdef USE_DIRECT3D

#include "ogl_extensions.h"

#include "opengl_directx_internals.h"

#include <map>

MultiTexCoord2fARB gglMultiTexCoord2fARB = nullptr;
ActiveTextureARB gglActiveTextureARB = nullptr;

glBindBufferARB gglBindBufferARB = nullptr;
glDeleteBuffersARB gglDeleteBuffersARB = nullptr;
glGenBuffersARB gglGenBuffersARB = nullptr;
glIsBufferARB gglIsBufferARB = nullptr;
glBufferDataARB gglBufferDataARB = nullptr;
glBufferSubDataARB gglBufferSubDataARB = nullptr;
glGetBufferSubDataARB gglGetBufferSubDataARB = nullptr;
glMapBufferARB gglMapBufferARB = nullptr;
glUnmapBufferARB gglUnmapBufferARB = nullptr;
glGetBufferParameterivARB gglGetBufferParameterivARB = nullptr;
glGetBufferPointervARB gglGetBufferPointervARB = nullptr;

ChoosePixelFormatARB gglChoosePixelFormatARB = nullptr;

namespace OpenGLD3D
{
  IDirect3DVertexBuffer9** g_currentVertexBuffer;
}

using namespace OpenGLD3D;

static std::map<unsigned, IDirect3DVertexBuffer9*> s_vertexBuffers;
static unsigned s_lastVertexBufferId = 0;

void __stdcall glGenBuffersD3D(GLsizei _howmany, GLuint* _buffers)
{
  for (unsigned i = 0; i < _howmany; i++)
    _buffers[i] = ++s_lastVertexBufferId;
}

void __stdcall glDeleteBuffersD3D(GLsizei _howmany, const GLuint* _buffers)
{
  for (unsigned i = 0; i < _howmany; i++)
  {
    unsigned id = _buffers[i];
    IDirect3DVertexBuffer9* buffer = s_vertexBuffers[id];

    if (buffer)
      buffer->Release();

    s_vertexBuffers.erase(id);
  }
}

void __stdcall glBindBufferD3D(GLenum _target, GLuint _bufferId)
{
  // Index buffers not supported for now
  DEBUG_ASSERT(_target == GL_ARRAY_BUFFER_ARB);

  if (_bufferId > 0)
    g_currentVertexBuffer = &s_vertexBuffers[_bufferId];
  else
    g_currentVertexBuffer = nullptr;
}

void __stdcall glBufferDataD3D(GLenum _target, GLsizeiptrARB _size, const GLvoid* _data, GLenum _usage)
{
  // Index buffers not supported for now
  DEBUG_ASSERT(_target == GL_ARRAY_BUFFER_ARB);

  // Ensure that glBindBufferD3D has been called already
  DEBUG_ASSERT(g_currentVertexBuffer != NULL);

  IDirect3DVertexBuffer9*& buffer = *g_currentVertexBuffer;

  // Discard the old buffer if necessary
  if (buffer)
    buffer->Release();

  // Create a buffer
  HRESULT hr = g_pd3dDeviceActual->CreateVertexBuffer(
    _size, D3DUSAGE_WRITEONLY | (g_supportsHwVertexProcessing ? 0 : D3DUSAGE_SOFTWAREPROCESSING), 0, D3DPOOL_MANAGED, &buffer,
    nullptr);

  DEBUG_ASSERT(hr != D3DERR_INVALIDCALL);
  DEBUG_ASSERT(hr != D3DERR_OUTOFVIDEOMEMORY);
  DEBUG_ASSERT(hr != E_OUTOFMEMORY);
  DEBUG_ASSERT(hr == D3D_OK);

  // Copy the data in
  void* vbData = nullptr;
  hr = buffer->Lock(0, 0, &vbData, 0/*D3DLOCK_DISCARD*/);
  DEBUG_ASSERT(hr == D3D_OK);

  memcpy(vbData, _data, _size);
  buffer->Unlock();
}

void InitialiseOGLExtensions()
{
  gglMultiTexCoord2fARB = glMultiTexCoord2fD3D;
  gglActiveTextureARB = glActiveTextureD3D;

  gglBindBufferARB = glBindBufferD3D;
  gglDeleteBuffersARB = glDeleteBuffersD3D;
  gglGenBuffersARB = glGenBuffersD3D;
  gglIsBufferARB = nullptr;
  gglBufferDataARB = glBufferDataD3D;
  gglBufferSubDataARB = nullptr;
  gglGetBufferSubDataARB = nullptr;
  gglMapBufferARB = nullptr;
  gglUnmapBufferARB = nullptr;
  gglGetBufferParameterivARB = nullptr;
  gglGetBufferPointervARB = nullptr;

  gglChoosePixelFormatARB = nullptr;
}

int IsOGLExtensionSupported(const char* extension) { return 0; }
#endif // USE_DIRECT3D
