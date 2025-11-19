#pragma once

#include "InputTransform.h"

class Route;
class GlobalEventCondition;

class ScreenZone
{
  public:
    std::string m_name;
    std::string m_toolTip;
    float m_x;
    float m_y;
    float m_w;
    float m_h;
    int m_data;
    int m_scrollZone;   // is this screenzone accessable using the zone scroll button on a pad?
    bool m_subZones;		// does the zone have sub icons? (ie, weapons)

    ScreenZone(std::string_view _name, std::string_view _tooltip, float _x, float _y, float _w, float _h, int _data);
};

class KeyboardShortcut : public ControlEventFunctor
{
  std::string m_name;
  int m_data;

  public:
    KeyboardShortcut(std::string _name, int _data, ControlType _controltype);
    std::string name() { return m_name; }
    int data() { return m_data; }
};

class TaskManagerInterface
{
  public:
    bool m_visible;
    int m_highlightedTaskId;
    bool m_verifyTargetting;                                     // True means you will only be able to create things in valid areas
    int m_currentMessageType;                                   // These are used to co-ordinate the
    int m_currentTaskType;                                      // on-screen status messages, eg
    float m_messageTimer;                                         // "Run Program : Squad" or "Syntax Error"
    bool m_viewingDefaultObjective;
    bool m_lockTaskManager;
    bool m_quickUnitVisible;                                     // the quickunit creation is visible

    enum
    {
      MessageSuccess,
      MessageFailure,
      MessageShutdown,
      MessageResearch,
      MessageResearchUpgrade,
      MessageObjectivesComplete,
      NumMessages
    };

    enum TMControl
    {
      TMTerminate,
      TMDisplay
    };

    TaskManagerInterface();
    virtual ~TaskManagerInterface() = default;

    void SetCurrentMessage(int _messageType, int _taskType, float _timer);

    void RunDefaultObjective(GlobalEventCondition* _cond);            // Runs basic cut-scene for trunk ports + research items

    virtual void Advance() = 0;
    virtual void Render() = 0;

    void AdvanceTab();

    void SetVisible(bool _visible = true);

    virtual bool ControlEvent(TMControl _type) = 0;

    // ControlHelp: If pressing the blue button would create a unit
    virtual bool AdviseCreateControlHelpBlue() = 0;
    virtual bool AdviseCreateControlHelpGreen() = 0;
    virtual bool AdviseCloseControlHelp() = 0;
    virtual bool AdviseOverSelectableZone() = 0;
};
