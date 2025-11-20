#include "pch.h"

#include "opengl_directx_dlist_dev.h"
#include "opengl_directx_dlist.h"

using namespace OpenGLD3D;

// STDMETHOD macro includes a virtual declaration which is not valid
#define virtual
#define STDMETHODI(method) STDMETHOD(DisplayListDevice::method)

static int calculateNumVertices(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount)
{
  switch (PrimitiveType)
  {
    case D3DPT_POINTLIST:
      return PrimitiveCount;
    case D3DPT_LINELIST:
      return PrimitiveCount * 2;
    case D3DPT_LINESTRIP:
      return PrimitiveCount + 1;
    case D3DPT_TRIANGLELIST:
      return PrimitiveCount * 3;
    case D3DPT_TRIANGLESTRIP:
      return PrimitiveCount + 2;
    case D3DPT_TRIANGLEFAN:
      return PrimitiveCount + 2;
    default: DEBUG_ASSERT(FALSE);
      return 0;
  }
}

STDMETHODI(DrawPrimitiveUP)(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,CONST void* pVertexStreamZeroData,
                            UINT VertexStreamZeroStride)
{
  if (PrimitiveCount == 0)
    return 0;

  int numVerts = calculateNumVertices(PrimitiveType, PrimitiveCount);
  auto vertices = (CustomVertex*)pVertexStreamZeroData;

  int startVertex = m_vertices.size();

  for (int i = 0; i < numVerts; i++)
    m_vertices.push_back(vertices[i]);

  m_commands.push_back(NEW CommandDrawPrimitive(PrimitiveType, startVertex, PrimitiveCount));
  return 0;
}

void DisplayListDevice::RecordLoadTransform(MatrixStack* _matrixStack, const D3DXMATRIX& _mat)
{
  m_commands.push_back(NEW CommandLoadTransform(_matrixStack, _mat));
}

void DisplayListDevice::RecordPushMatrix(MatrixStack* _matrixStack) { m_commands.push_back(NEW CommandPushMatrix(_matrixStack)); }

void DisplayListDevice::RecordPopMatrix(MatrixStack* _matrixStack) { m_commands.push_back(NEW CommandPopMatrix(_matrixStack)); }

void DisplayListDevice::RecordMultMatrix(MatrixStack* _matrixStack, const D3DXMATRIX& _mat)
{
  m_commands.push_back(NEW CommandMultiplyTransform(_matrixStack, _mat));
}

// Other methods

DisplayList* DisplayListDevice::Compile() { return NEW DisplayList(m_commands, m_vertices); }
