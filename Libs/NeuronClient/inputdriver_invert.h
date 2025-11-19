#ifndef INCLUDED_INPUTDRIVER_INVERT_H
#define INCLUDED_INPUTDRIVER_INVERT_H

#include "auto_vector.h"
#include "inputdriver.h"

using InputSpecList = auto_vector<const InputSpec>;

// Enables boolean input specifications to be negated with "not" or "!"

class InvertInputDriver : public InputDriver
{
  // List of InputSpec
  InputSpecList m_specs;
  std::string& lastError;

  public:
    InvertInputDriver();

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
};

#endif // INCLUDED_INPUTDRIVER_INVERT_H
