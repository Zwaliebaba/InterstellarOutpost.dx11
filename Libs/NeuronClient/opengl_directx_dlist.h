#ifndef OPENGL_DIRECTX_DLIST_H
#define OPENGL_DIRECTX_DLIST_H

#include <vector>
#include "opengl_directx_internals.h"

namespace OpenGLD3D
{
  class MatrixStack;

  class Command
  {
    public:
      virtual void Execute() = 0;
  };

  class CommandDrawPrimitive : public Command
  {
    public:
      CommandDrawPrimitive(D3DPRIMITIVETYPE _primitiveType, unsigned _startVertex, unsigned _primitiveCount);
      void Execute() override;

    private:
      D3DPRIMITIVETYPE m_primitiveType;
      unsigned m_startVertex, m_primitiveCount;
  };

  class CommandLoadTransform : public Command
  {
    public:
      CommandLoadTransform(MatrixStack* _matrixStack, const D3DMATRIX& _atrix);
      void Execute() override;

    private:
      MatrixStack* m_matrixStack;
      D3DXMATRIX m_matrix;
  };

  class CommandMultiplyTransform : public Command
  {
    public:
      CommandMultiplyTransform(MatrixStack* _matrixStack, const D3DXMATRIX& _matrix);
      void Execute() override;

    private:
      MatrixStack* m_matrixStack;
      D3DXMATRIX m_matrix;
  };

  class CommandPushMatrix : public Command
  {
    public:
      CommandPushMatrix(MatrixStack* _matrixStack);
      void Execute() override;

    private:
      MatrixStack* m_matrixStack;
  };

  class CommandPopMatrix : public Command
  {
    public:
      CommandPopMatrix(MatrixStack* _matrixStack);
      void Execute() override;

    private:
      MatrixStack* m_matrixStack;
  };

  class DisplayList
  {
    public:
      DisplayList(const std::vector<Command*>& _commands, const std::vector<CustomVertex>& _vertices);
      ~DisplayList();
      void Draw();

    private:
      IDirect3DVertexBuffer9* m_pVertexBuffer;
      Command** m_commands;
  };
}

#endif // OPENGL_DIRECTX_DLIST_H
