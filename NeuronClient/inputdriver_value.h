#ifndef INCLUDED_INPUTDRIVER_VALUE_H
#define INCLUDED_INPUTDRIVER_VALUE_H

#include "inputdriver.h"

class ValueInputDriver : public InputDriver
{
  public:
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
    //bool getInputDescription( InputSpec const &spec, InputDescription &desc );
};

#endif // INCLUDED_INPUTDRIVER_VALUE_H
