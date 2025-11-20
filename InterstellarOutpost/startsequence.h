#pragma once

class StartSequenceCaption
{
  public:
    std::string m_caption;
    float m_x;
    float m_y;
    float m_size;
    float m_startTime;
    float m_endTime;
};

class StartSequence
{
  protected:
    float m_startTime;
    std::vector<StartSequenceCaption> m_captions;

    void RegisterCaption(std::string_view _caption, float _x, float _y, float _size, float _startTime, float _endTime);

  public:
    StartSequence();

    bool Advance();
    void Render();
};
