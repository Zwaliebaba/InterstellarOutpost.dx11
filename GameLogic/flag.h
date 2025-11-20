#pragma once

#include "LegacyVector3.h"

#define FLAG_RESOLUTION 6

class Flag
{
  protected:
    LegacyVector3 m_flag[FLAG_RESOLUTION][FLAG_RESOLUTION];

    int m_texId;
    LegacyVector3 m_pos;
    LegacyVector3 m_front;
    LegacyVector3 m_up;
    float m_size;

  public:
    Flag();

    void Initialize();

    void SetTexture(int _textureId);
    void SetPosition(const LegacyVector3& _pos);
    void SetOrientation(const LegacyVector3& _front, const LegacyVector3& _up);
    void SetSize(float _size);

    void Render();
    void RenderText(int _posX, int _posY, char* _caption);
};
