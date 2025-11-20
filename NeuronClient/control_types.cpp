#include "pch.h"

#include "control_bindings.h"
#include "input_types.h"

static ControlAction s_actions[] = {

#define DEF_CONTROL_TYPE(x,y) "x", y,
#include "control_types.inc"
#undef DEF_CONTROL_TYPE

  "",
  INPUT_TYPE_ANY,
  nullptr,
  NULL
};
