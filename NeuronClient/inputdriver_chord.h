#ifndef INCLUDED_INPUTDRIVER_CHORD_H
#define INCLUDED_INPUTDRIVER_CHORD_H

#include "inputdriver.h"
#include "inputspeclist.h"

// Enables partial input specifications to be combined using "++" in the
// preferences file. Only allows boolean inputs that can handle "down"
// and "pressed" actions.

class ChordInputDriver : public InputDriver
{
  // List of lists of InputSpec
  auto_vector<InputSpecList> m_specs;
  std::string& lastError;

  public:
    ChordInputDriver();

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

#endif // INCLUDED_INPUTDRIVER_CHORD_H
