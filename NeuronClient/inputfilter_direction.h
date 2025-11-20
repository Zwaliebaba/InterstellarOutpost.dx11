#ifndef INCLUDED_INPUTFILTER_DIRECTION_H
#define INCLUDED_INPUTFILTER_DIRECTION_H

#include "inputfilter_withdelta.h"

// This class takes one 2D analogue input and converts it
// into a set of four queriable buttons, which correspond
// to up, down, left and right buttons

class DirectionInputFilter : public InputFilterWithDelta
{
  public:
    // Apply the filter to a set of InputDetails to get another InputDetails
    bool filter(const InputDetailsList& inDetails, const InputFilterSpec& filterSpec, InputDetails& outDetails) override;

    // Return true if the tokens were successfully parsed into an InputFilterSpec
    bool parseFilterSpecification(const InputSpecTokens& tokens, InputFilterSpec& spec) override;

    void calcDetails(const InputFilterSpec& spec, InputDetails& details) override;
};

#endif // INCLUDED_INPUTFILTER_DIRECTION_H
