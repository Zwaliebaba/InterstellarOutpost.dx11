#ifndef INCLUDED_INPUTSPECLIST_H
#define INCLUDED_INPUTSPECLIST_H

#include <memory>

#include "auto_vector.h"
#include "inputspec.h"

using InputSpecList = auto_vector<const InputSpec>;

using InputSpecIt = InputSpecList::const_iterator;

using InputSpecPtr = std::unique_ptr<const InputSpec>;

using InputSpecListPtr = std::unique_ptr<const InputSpecList>;

#endif // INCLUDED_INPUTSPECLIST_H
