#include "pch.h"

#include "inputfilter.h"

void InputFilter::setName(const std::string& name) { m_name = name; }

const std::string& InputFilter::getName() const { return m_name; }

std::ostream& operator<<(std::ostream& stream, const InputFilter& filter)
{
  return stream << "InputFilter (" << filter.getName() << ")";
}
