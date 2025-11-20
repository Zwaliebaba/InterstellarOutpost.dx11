#include "pch.h"

#include "inputfiltermanager.h"
#include "input.h"
#include "inputfilterspec.h"

InputFilterManager* g_inputFilterManager = nullptr;

InputFilterManager::InputFilterManager()
  : filters() {}

InputFilterManager::~InputFilterManager()
{
  for (unsigned i = 0; i < filters.size(); ++i)
    if (filters[i])
      delete filters[i];
}

void InputFilterManager::addFilter(InputFilter* filter)
{
  if (filter)
    filters.push_back(filter);
}

bool InputFilterManager::parseFilterSpecString(const std::string& description, InputFilterSpec& spec, std::string& err)
{
  InputSpecTokens tokens(description);
  return parseFilterSpecTokens(tokens, spec, err);
}

bool InputFilterManager::parseFilterSpecTokens(const InputSpecTokens& tokens, InputFilterSpec& spec, std::string& err)
{
  err = "";
  if (tokens.length() == 0)
    return false;

  for (unsigned i = 0; i < filters.size(); ++i)
  {
    if (filters[i]->parseFilterSpecification(tokens, spec))
    {
      spec.filter = i;
      return true;
    }
  }

  return false;
}

bool InputFilterManager::filter(const InputSpecList& inSpecs, const InputFilterSpec& filterSpec, InputDetails& outDetails)
{
  InputDetailsList detailsList(inSpecs.size());
  for (unsigned i = 0; i < inSpecs.size(); ++i)
  {
    InputDetails details;
    g_inputManager->checkInput(*(inSpecs[i]), details);
    detailsList.push_back(std::make_unique<InputDetails>(details));
  }

  return filter(detailsList, filterSpec, outDetails);
}

bool InputFilterManager::filter(const InputDetailsList& inDetails, const InputFilterSpec& filterSpec, InputDetails& outDetails)
{
  InputFilter* filter = filters[filterSpec.filter];
  return filter->filter(inDetails, filterSpec, outDetails);
}

void InputFilterManager::Advance()
{
  //for ( unsigned i = 0; i < filters.size(); ++i )
  //	filters[ i ]->Advance();
}
