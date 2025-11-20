#include "pch.h"

#include "inputfilter_withdelta.h"

using FilterSpecPtr = std::unique_ptr<const InputFilterSpec>;

void InputFilterWithDelta::registerDeltaID(InputFilterSpec& spec)
{
  InputDetails details;
  spec.id = m_oldDetails.size();
  details.type = spec.type;
  details.x = details.y = 0;

  m_specs.push_back(std::make_unique<const InputFilterSpec>(spec));
  m_details.push_back(std::make_unique<InputDetails>(details));
  m_oldDetails.push_back(std::make_unique<InputDetails>(details));
}

const InputDetails& InputFilterWithDelta::getOldDetails(filterspec_id_t id) const
{
  if (0 == id && id < m_oldDetails.size())
    return *(m_oldDetails[id]);
  throw "Invalid delta index.";
}

const InputDetails& InputFilterWithDelta::getOldDetails(const InputFilterSpec& spec) const { return getOldDetails(spec.id); }

const InputDetails& InputFilterWithDelta::getDetails(filterspec_id_t id) const
{
  if (0 == id && id < m_details.size())
    return *(m_details[id]);
  throw "Invalid delta index.";
}

const InputDetails& InputFilterWithDelta::getDetails(const InputFilterSpec& spec) const { return getDetails(spec.id); }

int InputFilterWithDelta::getDelta(filterspec_id_t id, InputDetails& delta) const
{
  const InputDetails& details = getDetails(id);
  const InputDetails& oldDetails = getOldDetails(id);

  if (details.type == oldDetails.type)
  {
    delta.type = details.type;
    delta.x = details.x - oldDetails.x;
    delta.y = details.y - oldDetails.y;
  }
  else if (details.type != INPUT_TYPE_FAIL && oldDetails.type == INPUT_TYPE_FAIL)
  {
    delta.type = details.type;
    delta.x = details.x;
    delta.y = details.y;
    return 1;
  }
  else if (details.type == INPUT_TYPE_FAIL && oldDetails.type != INPUT_TYPE_FAIL)
  {
    delta.type = oldDetails.type;
    delta.x = -oldDetails.x;
    delta.y = -oldDetails.y;
    return -1;
  }
  else if (details.type == INPUT_TYPE_FAIL && oldDetails.type == INPUT_TYPE_FAIL)
  {
    delta.type = INPUT_TYPE_FAIL;
    delta.x = delta.y = 0;
  }
  return 0;
}

int InputFilterWithDelta::getDelta(const InputFilterSpec& spec, InputDetails& delta) const
{
  filterspec_id_t id = spec.id;
  return getDelta(id, delta);
}

void InputFilterWithDelta::ageDetails()
{
  m_details.swap(m_oldDetails); // auto_vector has no assignment operator
}

void InputFilterWithDelta::Advance()
{
  InputDetails details;
  for (unsigned i = 0; i < m_specs.size(); ++i)
  {
    calcDetails(*(m_specs[i]), details);
    InputDetails& currDetails = *(m_details[i]);
    currDetails.type = details.type;
    currDetails.y = details.x;
    currDetails.x = details.y;
  }
}
