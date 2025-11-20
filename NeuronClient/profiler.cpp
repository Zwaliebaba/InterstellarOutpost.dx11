#include "pch.h"
#include "profiler.h"
#include "hi_res_time.h"

#ifdef PROFILER_ENABLED

#define PROFILE_HISTORY_LENGTH  10

ProfiledElement::ProfiledElement(const char* _name, ProfiledElement* _parent)
  : m_currentTotalTime(0.0),
    m_currentNumCalls(0),
    m_lastTotalTime(0.0),
    m_lastNumCalls(0),
    m_historyTotalTime(0.0),
    m_historyNumSeconds(0.0),
    m_historyNumCalls(0),
    m_shortest(DBL_MAX),
    m_longest(DBL_MIN),
    m_callStartTime(0.0),
    m_parent(_parent),
    m_isExpanded(false) { m_name = _name; }

ProfiledElement::~ProfiledElement() {}

// *** Start
void ProfiledElement::Start() { m_callStartTime = GetHighResTime(); }

// *** End
void ProfiledElement::End()
{
  const double timeNow = GetHighResTime();

  m_currentNumCalls++;
  const double duration = timeNow - m_callStartTime;
  m_currentTotalTime += duration;

  if (duration > m_longest)
    m_longest = duration;
  if (duration < m_shortest)
    m_shortest = duration;
}

// *** Advance
void ProfiledElement::Advance()
{
  m_lastTotalTime = m_currentTotalTime;
  m_lastNumCalls = m_currentNumCalls;
  m_currentTotalTime = 0.0;
  m_currentNumCalls = 0;
  m_historyTotalTime += m_lastTotalTime;
  m_historyNumSeconds += 1.0;
  m_historyNumCalls += m_lastNumCalls;

  for (auto& pe : m_children | std::views::values) { pe->Advance(); }
}

// *** ResetHistory
void ProfiledElement::ResetHistory()
{
  m_historyTotalTime = 0.0;
  m_historyNumSeconds = 0.0;
  m_historyNumCalls = 0;
  m_longest = DBL_MIN;
  m_shortest = DBL_MAX;

  for (auto& pe : m_children | std::views::values) { pe->ResetHistory(); }
}

double ProfiledElement::GetMaxChildTime()
{
  double rv = 0.0;

  for (auto& pe : m_children | std::views::values)
  {
    float val = pe->m_historyTotalTime;
    if (val > rv)
      rv = val;
  }

  return rv / m_children.begin()->second->m_historyNumSeconds;
}

Profiler::Profiler()
  : m_insideRenderSection(false),
    m_currentElement(nullptr)
{
  m_rootElement = NEW ProfiledElement("Root", nullptr);
  m_rootElement->m_isExpanded = true;
  m_currentElement = m_rootElement;
  m_endOfSecond = GetHighResTime() + 1.0f;
}

Profiler::~Profiler() { delete m_rootElement; }

// *** Advance
void Profiler::Advance()
{
  double timeNow = GetHighResTime();
  if (timeNow > m_endOfSecond)
  {
    m_lengthOfLastSecond = timeNow - (m_endOfSecond - 1.0);
    m_endOfSecond = timeNow + 1.0;

    m_rootElement->Advance();
  }
}

// *** RenderStarted
void Profiler::RenderStarted() { m_insideRenderSection = true; }

// *** RenderEnded
void Profiler::RenderEnded() { m_insideRenderSection = false; }

// *** ResetHistory
void Profiler::ResetHistory() { m_rootElement->ResetHistory(); }

static bool s_expanded = false;

// *** StartProfile
void Profiler::StartProfile(const char* _name)
{
  ProfiledElement* pe;

  auto it = m_currentElement->m_children.find(_name);
  if (it == m_currentElement->m_children.end())
  {
    pe = NEW ProfiledElement(_name, m_currentElement);
    m_currentElement->m_children.emplace(_name, pe);
  }
  else
    pe = it->second;

  ASSERT_TEXT(m_rootElement->m_isExpanded, "Profiler root element has been un-expanded");

  bool wasExpanded = m_currentElement->m_isExpanded;

  if (m_currentElement->m_isExpanded) { pe->Start(); }
  m_currentElement = pe;

  m_currentElement->m_wasExpanded = wasExpanded;
}

// *** EndProfile
void Profiler::EndProfile(const char* _name)
{
  DEBUG_ASSERT(m_currentElement->m_wasExpanded == m_currentElement->m_parent->m_isExpanded);

  if (m_currentElement->m_parent->m_isExpanded)
  {
    DEBUG_ASSERT(m_currentElement != m_rootElement);
    DEBUG_ASSERT(_stricmp(_name, m_currentElement->m_name.c_str()) == 0);

    m_currentElement->End();
  }

  DEBUG_ASSERT(strcmp( m_currentElement->m_name.c_str(), m_currentElement->m_parent->m_name.c_str()) != 0);
  m_currentElement = m_currentElement->m_parent;
}

#endif // PROFILER_ENABLED
