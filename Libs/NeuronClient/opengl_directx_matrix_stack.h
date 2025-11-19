#ifndef __OPENGL_DIRECTX_MATRIX_STACK
#define __OPENGL_DIRECTX_MATRIX_STACK

namespace OpenGLD3D
{
  class MatrixStack
  {
    public:
      MatrixStack(D3DTRANSFORMSTATETYPE _transformType);
      virtual ~MatrixStack();

      virtual void Load(const D3DXMATRIX& _m) = 0;
      virtual void Multiply(const D3DXMATRIX& _m) = 0;
      virtual void Push() = 0;
      virtual void Pop() = 0;

      virtual void SetTransform() = 0;
      virtual const D3DXMATRIX& GetTransform() const = 0;

      void FastSetTransform();
      bool Modified() const;

      D3DTRANSFORMSTATETYPE m_transformType;
      bool m_modified;
  };

  class MatrixStackActual : public MatrixStack
  {
    public:
      MatrixStackActual(D3DTRANSFORMSTATETYPE _transformType);
      ~MatrixStackActual() override;

      void Load(const D3DXMATRIX& _m) override;
      void Multiply(const D3DXMATRIX& _m) override;
      void Push() override;
      void Pop() override;

      void SetTransform() override;
      bool Modified() const;
      const D3DXMATRIX& GetTransform() const override;

    private:
      LPD3DXMATRIXSTACK m_matrixStack;
  };

  class DisplayListDevice;

  class MatrixStackDisplayList : public MatrixStack
  {
    public:
      MatrixStackDisplayList(D3DTRANSFORMSTATETYPE _transformType, DisplayListDevice* _device, MatrixStack* _actual);
      ~MatrixStackDisplayList() override;

      void Load(const D3DXMATRIX& _m) override;
      void Multiply(const D3DXMATRIX& _m) override;
      void Push() override;
      void Pop() override;

      void SetTransform() override;
      bool Modified() const;
      const D3DXMATRIX& GetTransform() const override;

    private:
      LPD3DXMATRIXSTACK m_matrixStack;
      DisplayListDevice* m_device;
      MatrixStack* m_actual;
  };

  // Inlines

  inline bool MatrixStack::Modified() const { return m_modified; }

  inline void MatrixStack::FastSetTransform()
  {
    if (m_modified)
      SetTransform();
  }
};

#endif // __OPENGL_DIRECTX_MATRIX_STACK
