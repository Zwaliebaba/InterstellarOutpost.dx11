#ifndef INCLUDED_INPUTSPEC_H
#define INCLUDED_INPUTSPEC_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using control_id_t = int;
using inputtype_t = int;
using condition_t = int;
using handler_id_t = int;

struct InputSpec
{
  unsigned driver;         // ID of InputDriver which handles this input
  inputtype_t type;        // Type of input details to expect
  control_id_t control_id; // Keycode, button number, etc.
  handler_id_t handler_id; // Maybe the driver contains several input handling functions
  condition_t condition;   // Condition upon which this triggers (down, up, held, clicked, etc.)
};

// Class to tokenise a prefs string
class InputSpecTokens
{
  std::vector<std::string> m_tokens;
  InputSpecTokens(std::vector<std::string> _tokens);

  public:
    InputSpecTokens(std::string _string);
    ~InputSpecTokens();
    unsigned length() const;
    const std::string& operator[](unsigned _index) const;
    std::unique_ptr<InputSpecTokens> operator()(int _start, int _end) const;
};

std::ostream& operator<<(std::ostream& stream, const InputSpecTokens& tokens);

#endif // INCLUDED_INPUTSPEC_H
