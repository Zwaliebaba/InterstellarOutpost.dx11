#ifndef INCLUDED_INPUTFILTERMANAGER_H
#define INCLUDED_INPUTFILTERMANAGER_H

#include <string>

#include "auto_vector.h"
#include "input_types.h"
#include "inputfilter.h"
#include "inputfilterspec.h"
#include "inputspeclist.h"

// This class is in charge of storing and accessing a set of InputFilters
// for use by the PipeInputDriver

class InputFilterManager
{
  std::vector<InputFilter*> filters;

  public:
    InputFilterManager();
    ~InputFilterManager();

    // Add a new filter to the filters collection. The filter will be deleted
    // at the end of the life of this InputFilterManager, so must have been
    // created with "new". Never do this twice for the same filter.
    void addFilter(InputFilter* filter);

    // Turn a string into an InputFilterSpec
    bool parseFilterSpecString(const std::string& description, InputFilterSpec& spec, std::string& err);

    // Turn a set of tokens into an InputFilterSpec. Not for general use outside of the input system
    bool parseFilterSpecTokens(const InputSpecTokens& tokens, InputFilterSpec& spec, std::string& err);

    // Apply a filter to a set of InputSpec to get an InputDetails
    bool filter(const InputSpecList& inDetails, const InputFilterSpec& filterSpec, InputDetails& outDetails);

    // Apply a filter to a set of InputDetails to get another InputDetails
    bool filter(const InputDetailsList& inDetails, const InputFilterSpec& filterSpec, InputDetails& outDetails);

    // Should be called once per input system update
    void Advance();
};

extern InputFilterManager* g_inputFilterManager;

#endif // INCLUDED_INPUTFILTERMANAGER_H
