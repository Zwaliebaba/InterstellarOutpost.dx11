#ifndef INCLUDED_INPUTFILTERSPEC_H
#define INCLUDED_INPUTFILTERSPEC_H

#include "input_types.h"
#include "inputspec.h"

using filter_mode_t = int;
using filterspec_id_t = unsigned;

struct InputFilterSpec
{
  unsigned filter;         // ID of InputDriver which handles this input
  filterspec_id_t id;      // A unique identifier to identify this in the filter
  InputType type;          // Type of input details to expect out
  unsigned min_inputs;     // Number of inputs expected
  unsigned max_inputs;
  filter_mode_t mode;      // Mode of the filter (eg left)
  condition_t condition;   // Exact condition to poll
};

#endif // INCLUDED_INPUTFILTERSPEC_H
