#include "pch.h"
#include "opengl_directx_dlist.h"
#include "opengl_directx_internals.h"
#include "opengl_directx_matrix_stack.h"

using namespace OpenGLD3D;

DisplayList::DisplayList(const std::vector<Command*>& _commands, const std::vector<CustomVertex>& _vertices)
  : m_pVertexBuffer(nullptr)
{
  m_commands = NEW Command*[_commands.size() + 1];
  for (int i = 0; i < _commands.size(); i++)
    m_commands[i] = _commands[i];
  m_commands[_commands.size()] = nullptr;

  if (_vertices.size() > 0)
  {
    // Set up the vertex buffer
    unsigned vbSize = _vertices.size() * sizeof(CustomVertex);

    HRESULT hr = g_pd3dDeviceActual->CreateVertexBuffer(
      vbSize, D3DUSAGE_WRITEONLY | (g_supportsHwVertexProcessing ? 0 : D3DUSAGE_SOFTWAREPROCESSING), 0, D3DPOOL_MANAGED,
      &m_pVertexBuffer, nullptr);

    DEBUG_ASSERT(hr != D3DERR_INVALIDCALL);
    DEBUG_ASSERT(hr != D3DERR_OUTOFVIDEOMEMORY);
    DEBUG_ASSERT(hr != E_OUTOFMEMORY);
    DEBUG_ASSERT(hr == D3D_OK);

    void* vbData = nullptr;

    hr = m_pVertexBuffer->Lock(0, 0, &vbData, 0/*D3DLOCK_DISCARD*/);
    DEBUG_ASSERT(hr == D3D_OK);

    memcpy(vbData, &_vertices[0], vbSize);
    m_pVertexBuffer->Unlock();
  }
}

DisplayList::~DisplayList()
{
  if (m_pVertexBuffer)
    m_pVertexBuffer->Release();

  for (Command** c = m_commands; *c; c++)
    delete *c;
  delete [] m_commands;
}

void DisplayList::Draw()
{
  g_pd3dDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(CustomVertex));
  for (Command** c = m_commands; *c; c++)
    (*c)->Execute();
}

// Commands

// --- CommandSetPrimitive ---

CommandDrawPrimitive::CommandDrawPrimitive(D3DPRIMITIVETYPE _primitiveType, unsigned _startVertex, unsigned _primitiveCount)
  : m_primitiveType(_primitiveType),
    m_startVertex(_startVertex),
    m_primitiveCount(_primitiveCount) {}

void CommandDrawPrimitive::Execute() { g_pd3dDevice->DrawPrimitive(m_primitiveType, m_startVertex, m_primitiveCount); }

// --- CommandLoadTransform ---

CommandLoadTransform::CommandLoadTransform(MatrixStack* _matrixStack, const D3DMATRIX& _matrix)
  : m_matrixStack(_matrixStack),
    m_matrix(_matrix) {}

void CommandLoadTransform::Execute()
{
  m_matrixStack->Load(m_matrix);
  m_matrixStack->SetTransform();
}

// --- CommandMultiplyTransform ---

CommandMultiplyTransform::CommandMultiplyTransform(MatrixStack* _matrixStack, const D3DXMATRIX& _matrix)
  : m_matrixStack(_matrixStack),
    m_matrix(_matrix) {}

void CommandMultiplyTransform::Execute()
{
  m_matrixStack->Multiply(m_matrix);
  m_matrixStack->SetTransform();
}

// --- CommandPushMatrix ---

CommandPushMatrix::CommandPushMatrix(MatrixStack* _matrixStack)
  : m_matrixStack(_matrixStack) {}

void CommandPushMatrix::Execute() { m_matrixStack->Push(); }

// --- CommandPopMatrix ---

CommandPopMatrix::CommandPopMatrix(MatrixStack* _matrixStack)
  : m_matrixStack(_matrixStack) {}

void CommandPopMatrix::Execute()
{
  m_matrixStack->Pop();
  m_matrixStack->SetTransform();
}
