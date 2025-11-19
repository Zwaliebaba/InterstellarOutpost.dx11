#ifndef INCLUDED_INPUT_W32_H
#define INCLUDED_INPUT_W32_H

#include "inputdriver_simple.h"
#include "keydefs.h"

#define NUM_MB 3
#define NUM_AXES 3

class W32InputDriver : public SimpleInputDriver
{
  int lastAcceptedDriver; // We're handling multiple driver types

  bool acceptDriver(const std::string& name) override;

  control_id_t getControlID(const std::string& name) override;

  inputtype_t getControlType(control_id_t control_id) override;

  InputParserState writeExtraSpecInfo(InputSpec& spec) override;

  control_id_t getKeyId(const std::string& keyName);

  inputtype_t getMouseControlType(control_id_t control_id);

  bool getKeyInput(const InputSpec& spec, InputDetails& details);

  bool getMouseInput(const InputSpec& spec, InputDetails& details);

  public:
    W32InputDriver();

    ~W32InputDriver();

    // Get input state. True if the input was triggered (input condition met). If true,
    // details are placed in details.
    bool getInput(const InputSpec& spec, InputDetails& details) override;

    // Returns true if the InputDriver is receiving no user input. Used to access screensaver
    // type modes.
    bool isIdle() override;

    // Returns the input mode associated with the InputDriver (keyboard or gamepad or none)
    InputMode getInputMode() override;

    // Returns true if there was an "active" input event this frame. Fills spec with
    // the details of the input. Active inputs are primarily things like button presses
    // but not buttons held down or released or 2D analog events.
    bool getFirstActiveInput(InputSpec& spec, bool instant) override;

    // This triggers a read from the input hardware and does message polling
    void Advance() override;

    // Poll for system events that may require immediate, hard-coded action
    void PollForEvents() override;

    // Fill out a description of the input defined by spec
    bool getInputDescription(const InputSpec& spec, InputDescription& desc) override;

    // Get the name of the driver (debuggung purposes)
    const std::string& getName();

    // This is a callback for Windows events
    // Returns 0 if the event is handled here, -1 otherwise
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Warp the mouse to a particular position and pretend it has always been there
    void SetMousePosNoVelocity(int _x, int _y);

  private:
    inline static bool m_mb[NUM_MB];            // Mouse button states from this frame
    inline static bool m_mbOld[NUM_MB];         // Mouse button states from last frame
    inline static int m_mbDeltas[NUM_MB];      // Mouse button diffs

    inline static int m_mousePos[NUM_AXES];    // X Y Z
    inline static int m_mousePosOld[NUM_AXES]; // X Y Z
    inline static int m_mouseVel[NUM_AXES];    // X Y Z

    inline static signed char m_keyNewDeltas[KEY_MAX];
};

#endif //INCLUDED_INPUT_W32_H
