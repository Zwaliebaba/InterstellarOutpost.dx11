#include "pch.h"

#include <memory>

#include "control_bindings.h"
#include "input_types.h"

using std::unique_ptr;

// This has to be in the same order as the enum in
// control_types.h
// These are the names of the control actions as they
// appear in an input configuration file

static ControlAction s_actions[] = {
#define DEF_CONTROL_TYPE(name, type)	{ #name, type },
#include "control_types.inc"
#undef DEF_CONTROL_TYPE
  {"", INPUT_TYPE_ANY},
  {nullptr, 0}
};

ControlBindings::ControlBindings() { memset(suppressed, 0, NumControlTypes); }

controltype_t ControlBindings::getControlID(const std::string& name)
{
  int i = 0;
  const char* cmd = s_actions[i].name;
  while (cmd && _stricmp(name.c_str(), cmd) != 0)
    cmd = s_actions[++i].name;
  if (i < NumControlTypes)
    return i;
  return -1;
}

void ControlBindings::getControlString(ControlType type, std::string& name)
{
  if (0 <= type && type < NumControlTypes)
    name = s_actions[type].name;
  else
    throw "Bad control type.";
}

bool ControlBindings::isAcceptibleInputType(controltype_t binding, inputtype_t type)
{
  if (0 <= binding && binding < NumControlTypes)
  {
    int validTypes = s_actions[binding].type;
    if ((validTypes & INPUT_TYPE_BOOL) == INPUT_TYPE_BOOL)
      validTypes = validTypes | INPUT_TYPE_1D; // BOOL implies 1D (can use triggers)
    return (validTypes & type) == type;
  }
  return false;
}

const InputSpecList& ControlBindings::operator[](ControlType id) const { return bindings[id]; }

const InputSpecList& ControlBindings::operator[](controltype_t id) const
{
  if (0 <= id && id < NumControlTypes)
    return bindings[id];
  throw "Invalid control type.";
}

const std::string& ControlBindings::getIcon(ControlType id) const { return icons[id]; }

const std::string& ControlBindings::getIcon(controltype_t id) const
{
  if (0 <= id && id < NumControlTypes)
    return icons[id];
  throw "Invalid control type.";
}

void ControlBindings::setIcon(controltype_t id, const std::string& iconfile)
{
  if (0 <= id && id < NumControlTypes)
    icons[id] = iconfile;
  else
    throw "Invalid control type.";
}

bool ControlBindings::bind(int type, const InputSpec& spec, bool replace)
{
  if (isAcceptibleInputType(type, spec.type))
  {
    unique_ptr<const InputSpec> specCopy(NEW InputSpec(spec));
    if (replace && bindings[type].size() > 0)
    {
      bindings[type].erase(0);
      bindings[type].insert(0, std::move(specCopy));
    }
    else
      bindings[type].push_back(std::move(specCopy));
    return true;
  }
  return false;
}

void ControlBindings::Advance() { memset(suppressed, 0, NumControlTypes); }

void ControlBindings::Clear()
{
  for (unsigned i = 0; i < NumControlTypes; ++i)
    bindings[i].clear();
}

void ControlBindings::suppress(ControlType id) { suppressed[id] = 1; }

bool ControlBindings::isActive(ControlType id) const { return (suppressed[id] == 0); }
