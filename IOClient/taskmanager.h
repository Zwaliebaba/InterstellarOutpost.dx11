#ifndef _included_taskmanager_h
#define _included_taskmanager_h

#include "global_world.h"
#include "llist.h"
#include "worldobject.h"

class Route;
class GlobalEventCondition;

// ============================================================================
// class Task

class Task
{
  public:
    enum
    {
      StateIdle,                              // Not yet run
      StateStarted,                           // Running, not yet targetted
      StateRunning,                           // Targetted, running
      StateStopping                           // Stopping
    };

    int m_id;
    int m_type;
    int m_state;
    WorldObjectId m_objId;

    Route* m_route;                   // Only used when this is a Controller task

    Task();
    ~Task();

    void Start();
    bool Advance();
    void SwitchTo();
    void Stop();

    void Target(const LegacyVector3& _pos);
    void TargetSquad(const LegacyVector3& _pos);
    void TargetEngineer(const LegacyVector3& _pos);
    void TargetOfficer(const LegacyVector3& _pos);
    void TargetArmour(const LegacyVector3& _pos);

    WorldObjectId Promote(WorldObjectId _id);
    static WorldObjectId Demote(WorldObjectId _id);
    static WorldObjectId FindDarwinian(const LegacyVector3& _pos);

    static std::string GetTaskName(int _type)  { return GlobalResearch::GetTypeName(_type); }
    static std::string GetTaskNameTranslated(int _type)  { return GlobalResearch::GetTypeNameTranslated(_type); }
};

// ============================================================================
// class TaskTargetArea

class TaskTargetArea
{
  public:
    LegacyVector3 m_center;
    float m_radius;
    bool m_stationary;
};

// ============================================================================
// class TaskManager

class TaskManager
{
  public:
    LList<Task*> m_tasks;

    int m_nextTaskId;
    int m_currentTaskId;
    bool m_verifyTargetting;

  protected:
    void AdvanceTasks();

  public:
    TaskManager();

    int Capacity();
    int CapacityUsed();

    bool RunTask(Task* _task);                            // Starts the task, registers it
    bool RunTask(int _type);
    bool RegisterTask(Task* _task);                            // Assumes task is already started, just registers it

    Task* GetCurrentTask();
    Task* GetTask(int _id);
    Task* GetTask(WorldObjectId _id);
    void SelectTask(int _id);
    void SelectTask(WorldObjectId _id);                      // Selects the corrisponding task
    bool IsValidTargetArea(int _id, const LegacyVector3& _pos);
    bool TargetTask(int _id, const LegacyVector3& _pos);
    bool TerminateTask(int _id);

    void StopAllTasks();

    LList<TaskTargetArea>* GetTargetArea(int _id);                  // Returns all valid placement areas

    void Advance();
};

#endif
