#ifndef INCLUDED_INPUTDRIVER_SIMPLE_H
#define INCLUDED_INPUTDRIVER_SIMPLE_H

#include <string>

#include "inputdriver.h"

class SimpleInputDriver : public InputDriver
{
  protected:
    virtual bool acceptToken(InputParserState& state, const std::string& token, InputSpec& spec);

    virtual bool acceptDriver(const std::string& name) = 0;

    virtual control_id_t getControlID(const std::string& name) = 0;

    virtual inputtype_t getControlType(control_id_t control_id) = 0;

    virtual InputParserState writeExtraSpecInfo(InputSpec& spec);

    virtual InputParserState parseExtraToken(const std::string& token, InputSpec& spec);

    virtual condition_t getConditionID(const std::string& name, inputtype_t& type);

  public:
    // Return STATE_DONE if we managed to parse these tokens, and put the parsed information
    // into spec. Anything else means we failed.
    InputParserState parseInputSpecification(const InputSpecTokens& tokens, InputSpec& spec) override;

    // Return a helpful error string when there's a problem
    const std::string& getLastParseError(InputParserState state) override;
};

#endif // INCLUDED_INPUTDRIVER_SIMPLE_H
