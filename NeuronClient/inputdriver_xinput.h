#ifndef INCLUDED_INPUT_XINPUT_H
#define INCLUDED_INPUT_XINPUT_H

#include <XInput.h>
#include <string>
#include "inputdriver_simple.h"

class XInputDriver : public SimpleInputDriver
{
  struct ControllerState
  {
    bool isConnected;
    XINPUT_STATE state;
  };

  enum OptionalTokenState
  {
    BEFORE_OPTIONAL_TOKENS,
    AFTER_TOKEN_WITH,
    AFTER_TOKEN_SENSITIVITY
  };

  static const float SCALE_JOYSTICK;
  static const float SCALE_TRIGGER;
  static const double REPEAT_FREQ;

  // At the moment this only supports the first controller
  ControllerState m_state;
  ControllerState m_oldState;

  OptionalTokenState m_optTokenState;

  bool acceptDriver(const std::string& name) override;

  control_id_t getControlID(const std::string& name) override;

  inputtype_t getControlType(control_id_t control_id) override;

  InputParserState writeExtraSpecInfo(InputSpec& spec) override;

  InputParserState parseExtraToken(const std::string& token, InputSpec& spec) override;

  bool getButtonInput(WORD button, const InputSpec& spec, InputDetails& details);

  bool getThumbInput(const InputSpec& spec, InputDetails& details);

  bool getTriggerInput(const InputSpec& spec, InputDetails& details);

  void correctDeadZones(XINPUT_STATE& state);

  void scaleInputs(XINPUT_STATE& state);

  bool getThumbDirection(control_id_t control, const InputSpec& spec, unsigned direction, InputDetails& details);

  protected:
    condition_t getConditionID(const std::string& name, inputtype_t& type) override;

  public:
    XInputDriver();

    // Get input state. True if the input was triggered (input condition met). If true,
    // details are placed in details.
    bool getInput(const InputSpec& spec, InputDetails& details) override;

    // Returns true if the InputDriver is receiving no user input. Used to access screensaver
    // type modes.
    bool isIdle() override;

    // Returns the input mode associated with the InputDriver (keyboard or gamepad or none)
    InputMode getInputMode() override;

    // This triggers a read from the input hardware and does message polling
    void Advance() override;

    // Fill out a description of the input defined by spec
    bool getInputDescription(const InputSpec& spec, InputDescription& desc) override;

    // Get the name of the driver (debuggung purposes)
    const std::string& getName();
};

#endif //INCLUDED_INPUT_XINPUT_H
