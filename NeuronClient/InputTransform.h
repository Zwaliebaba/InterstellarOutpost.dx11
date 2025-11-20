#ifndef INCLUDED_INPUTTRANSFORM_H
#define INCLUDED_INPUTTRANSFORM_H

#include <memory>
#include <string>

#include "input.h"

class InputTransform
{
  public:
  virtual ~InputTransform() = default;
  virtual bool operator()() = 0;
};

// ==================== class ControlEventFunctor ====================

class ControlEventFunctor : public InputTransform
{
  ControlType type;

  public:
    ControlEventFunctor(ControlType _type);

    bool operator()() override;

    bool operator()(InputDetails& details);

    std::string noun();
};

// ==================== class ToggleInputTransform ====================

class ToggleInputTransform : public InputTransform
{
  std::unique_ptr<InputTransform> m_on, m_off;
  bool m_state, m_change;

  void Advance();

  public:
    ToggleInputTransform(ControlType _on, ControlType _off);

    ToggleInputTransform(std::unique_ptr<InputTransform> _on, std::unique_ptr<InputTransform> _off);

    bool operator()() override;

    bool on();

    bool changed();
};

#endif // INCLUDED_INPUTTRANSFORM_H
