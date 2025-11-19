#include "pch.h"
#include "profilewindow.h"
#include "GameApp.h"
#include "input.h"
#include "profiler.h"
#include "targetcursor.h"
#include "DX9TextRenderer.h"

#ifdef PROFILER_ENABLED

// ****************************************************************************
// Class ProfilerButton
// ****************************************************************************

class ProfilerButton : public GuiButton
{
  public:
    void MouseUp() override
    {
      if (_stricmp(m_caption.c_str(), "Reset History") == 0)
        g_app->m_profiler->ResetHistory();
      else if (_stricmp(m_caption.c_str(), "Min") == 0)
        m_caption = "Avg";
      else if (_stricmp(m_caption.c_str(), "Avg") == 0)
        m_caption = "Max";
      else if (_stricmp(m_caption.c_str(), "Max") == 0)
        m_caption = "Min";
    }
};

ProfileWindow::ProfileWindow(const char* name)
  : GuiWindow(name),
    m_totalPerSecond(true) {}

ProfileWindow::~ProfileWindow() {  }

void ProfileWindow::RenderElementProfile(ProfiledElement* _pe, unsigned int _indent)
{
  if (_pe->m_children.size() == 0)
    return;

  int left = m_x + 10;
  char caption[256];
  GuiButton* minAvgMaxButton = GetButton("Avg");
  int minAvgMax = 0;
  if (_stricmp(minAvgMaxButton->m_caption.c_str(), "Avg") == 0)
    minAvgMax = 1;
  else if (_stricmp(minAvgMaxButton->m_caption.c_str(), "Max") == 0)
    minAvgMax = 2;

  float largestTime = 1000.0f * _pe->GetMaxChildTime();
  float totalTime = 0.0f;

  for (auto& child : _pe->m_children | std::views::values)
  {
    float time = static_cast<float>(child->m_lastTotalTime * 1000.0f);
    float avrgTime = 1000.0f * child->m_historyTotalTime / child->m_historyNumCalls;
    if (avrgTime > 0.0f)
    {
      totalTime += time;

      char icon[] = " ";
      if (child->m_children.size() > 0)
        icon[0] = child->m_isExpanded ? '-' : '+';

      float lastColumn;
      if (minAvgMax == 0)
        lastColumn = child->m_shortest;
      else if (minAvgMax == 1)
        lastColumn = avrgTime / 1000.0f;
      else if (minAvgMax == 2)
        lastColumn = child->m_longest;
      lastColumn *= 1000.0f;

      sprintf(caption, "%*s%-*s:%5d x%4.2f = %4.0f %4.2f", _indent + 1, icon, 24 - _indent, child->m_name.c_str(), child->m_lastNumCalls,
              time / static_cast<float>(child->m_lastNumCalls), time, lastColumn);
      int brightness = (time / largestTime) * 150.0f + 105.0f;
      if (brightness < 105)
        brightness = 105;
      else if (brightness > 255)
        brightness = 255;
      glColor3ub(brightness, brightness, brightness);

      // Deal with mouse clicks to expand or unexpand a node
      if (g_inputManager->controlEvent(ControlEclipseLMousePressed)) 
      {
        int x = g_target->X();
        int y = g_target->Y();
        if (x > m_x && x < m_x + m_w && y > m_yPos + 5 && y < m_yPos + 17)
        {
          ASSERT_TEXT(child != g_app->m_profiler->m_rootElement, "ProfileWindow::RenderElementProfile child==root");
          child->m_isExpanded = !child->m_isExpanded;
        }
      }

      g_editorFont.DrawText2D(left, m_yPos += 12, DEF_FONT_SIZE, caption);

      int lineLeft = left + 360;
      int lineY = m_yPos - 6;
      int lineWidth = sqrtf(time) * 10.0f;
      int lineHeight = 11.0f;
      glColor4ub(150, 150, 250, brightness);
      glBegin(GL_QUADS);
      glVertex2i(lineLeft, lineY);
      glVertex2i(lineLeft + lineWidth, lineY);
      glVertex2i(lineLeft + lineWidth, lineY + lineHeight);
      glVertex2i(lineLeft, lineY + lineHeight);
      glEnd();

      if (m_yPos > m_h)
        m_h += 12;

      if (child->m_isExpanded && child->m_children.size() > 0)
        RenderElementProfile(child, _indent + 2);
    }
  }

  glColor3ub(255, 255, 255);
  g_editorFont.DrawText2D(left + (_indent + 1) * 7.5f, m_yPos += 12, DEF_FONT_SIZE, "Total %.0f", totalTime);
}

void ProfileWindow::Render(bool hasFocus)
{
  GuiWindow::Render(hasFocus);

  ProfiledElement* root = g_app->m_profiler->m_rootElement;

  m_yPos = m_y + 42;

  g_editorFont.DrawText2DRight(m_x + 330, m_yPos, DEF_FONT_SIZE * 0.85f, "calls x avrg = total");

  START_PROFILE(g_app->m_profiler, "render profile");
  RenderElementProfile(root, 0);
  END_PROFILE(g_app->m_profiler, "render profile");
}

void ProfileWindow::Create()
{
  GuiWindow::Create();

  auto minAvgMax = new ProfilerButton();
  minAvgMax->SetProperties("Avg", 330, 18);
  RegisterButton(minAvgMax);

  auto resetHistBut = new ProfilerButton();
  resetHistBut->SetProperties("Reset History", 190, 18);
  RegisterButton(resetHistBut);
}

void ProfileWindow::Remove()
{
  GuiWindow::Remove();

  RemoveButton("Min");
  RemoveButton("Reset History");
}

#endif // PROFILER_ENABLED
