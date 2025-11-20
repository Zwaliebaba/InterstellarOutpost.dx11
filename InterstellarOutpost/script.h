#ifndef _included_script_h
#define _included_script_h

#include "text_stream_readers.h"

class LevelFile;

class Script
{
  public:
    // If you modify this remember to update g_opCodeNames in script.cpp
    enum
    {
      OpCamCut,
      OpCamMove,
      OpCamAnim,
      OpCamFov,
      OpCamBuildingFocus,
      OpCamBuildingApproach,
      OpCamLocationFocus,                     // global world only please
      OpCamGlobalWorldFocus,                  // global world only please
      OpCamReset,
      OpEnterLocation,
      OpExitLocation,
      OpSay,
      OpShutUp,
      OpWait,
      OpWaitSay,
      OpWaitCam,
      OpWaitFade,
      OpWaitRocket,
      OpWaitPlayerNotBusy,
      OpHighlight,
      OpClearHighlights,
      OpTriggerSound,
      OpStopSound,
      OpGiveResearch,
      OpSetMission,
      OpGameOver,
      OpResetResearch,                        // Currently only affects darwinians
      OpRestoreResearch,                      // Currently only affects darwinians
      OpRunCredits,
      OpSetCutsceneMode,
      OpGodDishActivate,
      OpGodDishDeactivate,
      OpGodDishSpawnSpam,
      OpGodDishSpawnResearch,
      OpTriggerSpam,
      OpPurityControl,
      OpShowDarwinLogo,
      OpShowDemoEndSequence,
      OpPermitEscape,
      OpDestroyBuilding,
      OpActivateTrunkPort,
      OpActivateTrunkPortFull,
      OpNumOps
    };

    TextReader* m_in;
    double m_waitUntil;
    bool m_waitForSpeech;
    bool m_waitForCamera;
    bool m_waitForFade;
    bool m_waitForPlayerNotBusy;

    int m_requestedLocationId;
    int m_darwinianResearchLevel;

    int m_rocketId;
    int m_rocketState;
    int m_rocketData;
    bool m_waitForRocket;
    bool m_permitEscape;

  protected:
    void ReportError(const LevelFile* _levelFile, const char* _fmt, ...);

    void RunCommand_CamCut(const char* _mountName);
    void RunCommand_CamMove(const char* _mountName, float _duration);
    void RunCommand_CamAnim(const char* _animName);
    void RunCommand_CamFov(float _fov, bool _immediate);
    void RunCommand_CamBuildingFocus(int _buildingId, float _range, float _height);
    void RunCommand_CamBuildingApproach(int _buildingId, float _range, float _height, float _duration);
    void RunCommand_CamGlobalWorldFocus();
    void RunCommand_LocationFocus(const char* _locationName, float _fov);
    void RunCommand_CamReset();

    void RunCommand_EnterLocation(char* _name);
    void RunCommand_ExitLocation();
    void RunCommand_SetMission(std::string_view _locName, std::string_view _missionName);
    void RunCommand_Say(char* _stringId);
    void RunCommand_ShutUp();

    void RunCommand_Wait(double _time);
    void RunCommand_WaitSay();
    void RunCommand_WaitCam();
    void RunCommand_WaitFade();
    void RunCommand_WaitRocket(int _buildingId, char* _state, int _data);
    void RunCommand_WaitPlayerNotBusy();

    void RunCommand_Highlight(int _buildingId);
    void RunCommand_ClearHighlights();

    void RunCommand_TriggerSound(const char* _event);
    void RunCommand_StopSound(const char* _event);

    void RunCommand_GiveResearch(const char* _name);

    void RunCommand_ResetResearch();
    void RunCommand_RestoreResearch();

    void RunCommand_GameOver();
    void RunCommand_RunCredits();

    void RunCommand_GodDishActivate();
    void RunCommand_GodDishDeactivate();
    void RunCommand_GodDishSpawnSpam();
    void RunCommand_GodDishSpawnResearch();
    void RunCommand_SpamTrigger();

    void RunCommand_PurityControl();

    void RunCommand_ShowDarwinLogo();
    void RunCommand_ShowDemoEndSequence();

    void RunCommand_PermitEscape();

    void RunCommand_DestroyBuilding(int _buildingId, float _intensity);
    void RunCommand_ActivateTrunkPort(int _buildingId, bool _fullActivation);

  public:
    Script();

    void Advance();
    void AdvanceScript();
    void RunScript(std::string_view _filename);
    void TestScript(const char* _filename);
    bool IsRunningScript();
    bool Skip();

    int GetOpCode(const char* _word);
};

#endif
