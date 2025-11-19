#ifndef INCLUDED_CONTROL_TYPES_H
#define INCLUDED_CONTROL_TYPES_H

using controltype_t = int;

// If you change this, you must also change the actions in control_bindings.cpp
enum ControlType
{
#define DEF_CONTROL_TYPE(x,y) Control##x,
#include "control_types.inc"
#undef DEF_CONTROL_TYPE

  ControlNull,                              // This is never triggered

  NumControlTypes
};

#endif // INCLUDED_CONTROL_TYPES_H
