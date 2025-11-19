#ifndef INCLUDED_INPUTDRIVER_CONJOIN_H
#define INCLUDED_INPUTDRIVER_CONJOIN_H

#include "auto_vector.h"
#include "inputdriver.h"
#include "inputspeclist.h"

// Enables input specifications to be combined using "&&" in the preferences
// file. Only allows up to one non-boolean (ie. complex) input in any spec.
// since otherwise it would have to choose the results to return arbitrarily.

class ConjoinInputDriver : public InputDriver
{
  // List of lists of InputSpec
  auto_vector<InputSpecList> m_specs;
  std::string& lastError;

  public:
    ConjoinInputDriver();

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

#endif // INCLUDED_INPUTDRIVER_CONJOIN_H
