#include "pch.h"
#include "input_types.h"

InputDescription::InputDescription()
  : noun(),
    verb(),
    pref() {}

InputDescription::InputDescription(const InputDescription& _desc)
  : noun(_desc.noun),
    verb(_desc.verb),
    pref(_desc.pref),
    translated(_desc.translated) {}

InputDescription::InputDescription(std::string _noun, std::string _verb, std::string _pref, bool _translated)
  : noun(std::move(_noun)),
    verb(std::move(_verb)),
    pref(std::move(_pref)),
    translated(_translated) {}

void InputDescription::translate()
{
  if (!translated)
  {
    noun = Strings::Get(noun);
    verb = Strings::Get(verb);
    translated = true;
  }
}
