#include "pch.h"
#include "input.h"
#include "inputfiltermanager.h"
#include "targetcursor.h"

#define Q(x) "\"" << x << "\""

using namespace std;

InputManager* g_inputManager = nullptr;

InputManager::InputManager()
  : m_idle(true),
    m_inputMode(INPUT_MODE_KEYBOARD) {}

InputManager::~InputManager()
{
  for (unsigned i = 0; i < drivers.size(); ++i)
  {
    if (drivers[i])
      delete drivers[i];
  }
}

void InputManager::parseInputPrefs(TextReader& reader, const bool replace)
{
  ostream& derr = cout;
  while (reader.ReadLine())
  {
    if (char* control = reader.GetNextToken())
    {
      bool iconline = false;
      char* eq = reader.GetNextToken();
      if (!eq || strcmp(eq, "=") != 0)
      {
        if (eq && !strcmp(eq, "~"))
          iconline = true;
        else
        {
          derr << "Assignment not found." << endl;
          continue;
        }
      }

      string inputspec = reader.GetRestOfLine();
      controltype_t control_id = getControlID(control);
      if (control_id >= 0)
      {
        if (iconline)
        {
          if (inputspec != "")
          {
            unsigned len = inputspec.length() - 1;
            if (inputspec[len] == '\n')
              inputspec = inputspec.substr(0, len--);
            if (inputspec[len] == '\r')
              inputspec = inputspec.substr(0, len);
            bindings.setIcon(control_id, inputspec);
            derr << "Treated as icon: " << Q(bindings.getIcon( control_id )) << endl;
          }
          else
            derr << "Empty icon line." << endl;
          continue;
        }

        InputSpec spec;
        string err;
        if (PARSE_SUCCESS(parseInputSpecString( inputspec, spec, err )))
        {
          if (bindings.bind(control_id, spec, replace))
            derr << "Success. Driver = " << drivers[spec.driver]->getName() << endl;
          else
            derr << "Binding failed." << endl;
        }
        else
          derr << "Parse failed - " << err << endl;
      }
      else
        derr << "Control ID not found." << endl;
    }
    else
      derr << "Blank line." << endl;
  }
}

InputParserState InputManager::parseInputSpecString(const string& description, InputSpec& spec, string& err)
{
  InputSpecTokens tokens(description);
  return parseInputSpecTokens(tokens, spec, err);
}

InputParserState InputManager::parseInputSpecTokens(const InputSpecTokens& tokens, InputSpec& spec, string& err)
{
  InputParserState state = STATE_ERROR;
  InputParserState curr = STATE_ERROR;
  err = "";
  if (tokens.length() == 0)
    return state;

  for (unsigned i = 0; i < drivers.size(); ++i)
  {
    if (PARSE_SUCCESS(curr = drivers[ i ]->parseInputSpecification( tokens, spec )))
    {
      spec.driver = i;
      return curr;
    }
    if (curr >= state)
    {
      state = curr;
      err = drivers[i]->getLastParseError(state);
    }
  }

  return state;
}

bool InputManager::controlEventA(const ControlType type, InputDetails& details)
{
  if (bindings.isActive(type))
  {
    const InputSpecList& specs = bindings[type];

    for (unsigned i = 0; i < specs.size(); ++i)
    {
      if (checkInput(*(specs[i]), details))
        return true;
    }
  }

  details.type = INPUT_TYPE_FAIL;
  return false;
}

bool InputManager::controlEvent(const ControlType type, InputDetails& details) { return controlEventA(type, details); }

bool InputManager::controlEvent(const ControlType type)
{
  InputDetails details;
  return controlEvent(type, details);
}

const std::string& InputManager::controlIcon(const ControlType type) const { return bindings.getIcon(type); }

void InputManager::Advance()
{
  bindings.Advance();

  bool idleNext = true;
  InputMode nextInputMode = INPUT_MODE_NONE;

  for (unsigned i = 0; i < drivers.size(); ++i)
  {
    InputDriver* driver = drivers[i];
    driver->Advance();
    bool driverIdle = driver->isIdle();
    idleNext = idleNext && driverIdle;

    if (!driverIdle)
    {
      InputMode driverInputMode = driver->getInputMode();
      if (driverInputMode > nextInputMode)
        nextInputMode = driverInputMode;
    }
  }

  m_idle = idleNext;

  // Record the mode, if we know it, otherwise stick with last recorded
  if (nextInputMode > INPUT_MODE_NONE)
    m_inputMode = nextInputMode;

  if (g_target)
    g_target->Advance();
}

void InputManager::PollForEvents()
{
  for (unsigned i = 0; i < drivers.size(); ++i)
  {
    InputDriver* driver = drivers[i];
    driver->PollForEvents();
  }
}

bool InputManager::checkInput(const InputSpec& spec, InputDetails& details)
{
  InputDriver* driver = drivers[spec.driver];
  bool ans = driver->getInput(spec, details);
  if (!ans)
    details.type = INPUT_TYPE_FAIL;
  return ans;
}

bool InputManager::getFirstActiveInput(InputSpec& spec, const bool instant)
{
  for (unsigned i = 0; i < drivers.size(); ++i)
  {
    InputDriver* driver = drivers[i];
    if (driver->getFirstActiveInput(spec, instant))
    {
      spec.driver = i;
      return true;
    }
  }
  return false;
}

void InputManager::addDriver(InputDriver* driver)
{
  if (driver)
    drivers.push_back(driver);
}

void InputManager::suppressEvent(const ControlType type) { bindings.suppress(type); }

bool InputManager::getBoundInputDescription(const ControlType type, InputDescription& desc)
{
  const InputSpecList& specs = bindings[type];
  if (specs.size() >= 1)
  {
    for (InputSpecIt i = specs.begin(); i != specs.end(); ++i)
    {
      if (getInputDescription(**i, desc))
      {
        desc.translate();
        return true;
      }
    }
  }
  return false;
}

bool InputManager::getInputDescription(const InputSpec& spec, InputDescription& desc)
{
  InputDriver* driver = drivers[spec.driver];
  return driver->getInputDescription(spec, desc);
}

controltype_t InputManager::getControlID(const string& control) { return bindings.getControlID(control); }

void InputManager::getControlString(const ControlType type, std::string& name) { bindings.getControlString(type, name); }

void InputManager::replacePrimaryBinding(const ControlType type, const string& prefString)
{
  InputSpec spec;
  string err;
  if (PARSE_SUCCESS(parseInputSpecString( prefString, spec, err )))
    bindings.bind(type, spec, true);
}

void InputManager::Clear() { bindings.Clear(); }

bool InputManager::isIdle() { return m_idle; }

InputMode InputManager::getInputMode() { return m_inputMode; }
