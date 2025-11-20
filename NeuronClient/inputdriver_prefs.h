#ifndef INCLUDED_INPUTDRIVER_PREFS_H
#define INCLUDED_INPUTDRIVER_PREFS_H

#include <string>

#include "auto_vector.h"
#include "inputdriver.h"

class PrefsInputDriver : public InputDriver
{
  // List of preference keys we use, not ordered since we want to preserve
  // positions despite additions
  auto_vector<std::string> m_keys;

  // Searches for a key in m_keys. Adds one if it has to.
  int keyPosition(const std::string& key);

  public:
    PrefsInputDriver();

    // Return STATE_DONE if we managed to parse these tokens, and put the parsed information
    // into spec. Anything else means we failed.
    InputParserState parseInputSpecification(const InputSpecTokens& tokens, InputSpec& spec) override;

    // Get input state. True if the input was triggered (input condition met). If true,
    // details are placed in details.
    bool getInput(const InputSpec& spec, InputDetails& details) override;

    // This triggers a read from the input hardware and does message polling
    void Advance() override;

    // Return a helpful error string when there's a problem
    const std::string& getLastParseError(InputParserState state) override;

    // Fill out a description of the input defined by spec
    bool getInputDescription(const InputSpec& spec, InputDescription& desc) override;

    // Get the name of the driver (debuggung purposes)
    const std::string& getName();
};

#endif // INCLUDED_INPUTDRIVER_PREFS_H
