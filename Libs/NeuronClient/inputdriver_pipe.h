#ifndef INCLUDED_INPUTDRIVER_PIPE_H
#define INCLUDED_INPUTDRIVER_PIPE_H

#include <sstream>

#include "auto_vector.h"
#include "inputdriver.h"
#include "inputspeclist.h"

// Enables input specifications to be fed through converters
// using [spec1, spec2, ..., specn] -> converter_spec syntax
// in the input preferences file.

struct InputFilterWithArgs;

class PipeInputDriver : public InputDriver
{
  // List of lists of InputSpec
  auto_vector<InputFilterWithArgs> m_specs;
  std::string& lastError;

  // Parse an individual spec which is part of the left hand side of the
  // piping operator, add it to the list and empty the stream.
  InputParserState parseInputSpec(std::ostringstream& stream, InputSpecList& speclist);

  public:
    PipeInputDriver();

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
};

#endif // INCLUDED_INPUTDRIVER_PIPE_H
