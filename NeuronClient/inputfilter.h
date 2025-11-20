#ifndef INCLUDED_INPUTFILTER_H
#define INCLUDED_INPUTFILTER_H

#include <iostream>
#include <string>

#include "input_types.h"
#include "inputfilterspec.h"

class InputFilter
{
  std::string m_name;

  protected:
    void setName(const std::string& name);

  public:
    // Apply the filter to a set of InputDetails to get another InputDetails
    virtual bool filter(const InputDetailsList& inDetails, const InputFilterSpec& filterSpec, InputDetails& outDetails) = 0;

    // Return true if the tokens were successfully parsed into an InputFilterSpec
    virtual bool parseFilterSpecification(const InputSpecTokens& tokens, InputFilterSpec& spec) = 0;

    // Called once per input system update
    virtual void Advance() = 0;

    virtual const std::string& getName() const;
};

std::ostream& operator<<(std::ostream& stream, const InputFilter& filter);

#endif // INCLUDED_INPUTFILTER_H
