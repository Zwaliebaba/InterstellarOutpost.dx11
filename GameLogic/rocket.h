#ifndef _included_rocket_h
#define _included_rocket_h

#include "building.h"

class FuelBuilding : public Building
{
  public:
    int m_fuelLink;
    float m_currentLevel;

    FuelBuilding(BuildingType _type);

    void Initialize(Building* _template) override;

    virtual void ProvideFuel(float _level);

    LegacyVector3 GetFuelPosition();

    FuelBuilding* GetLinkedBuilding();

    bool Advance() override;

    bool IsInView() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    void Read(TextReader* _in, bool _dynamic) override;
    

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

    void Destroy(float _intensity) override;

  protected:
    ShapeMarker* m_fuelMarker;
    static Shape* s_fuelPipe;
};

// ============================================================================

class FuelGenerator : public FuelBuilding
{
  protected:
    Shape* m_pump;
    ShapeMarker* m_pumpTip;
    float m_pumpMovement;
    float m_previousPumpPos;

    LegacyVector3 GetPumpPos();

  public:
    float m_surges;

    FuelGenerator();

    void ProvideSurge();

    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    const char* GetObjectiveCounter() override;
};

// ============================================================================

class FuelPipe : public FuelBuilding
{
  public:
    FuelPipe();

    bool Advance() override;
};

// ============================================================================

class FuelStation : public FuelBuilding
{
  protected:
    ShapeMarker* m_entrance;

  public:
    FuelStation();

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    LegacyVector3 GetEntrance();

    bool Advance() override;
    bool IsLoading();
    bool BoardRocket(WorldObjectId id);

    bool PerformDepthSort(LegacyVector3& _centerPos) override;
};

// ============================================================================

class EscapeRocket : public FuelBuilding
{
  protected:
    ShapeMarker* m_booster;
    ShapeMarker* m_window[3];
    Shape* m_rocketLowRes;
    float m_shadowTimer;
    float m_cameraShake;

    LegacyVector3 m_vel;

  public:
    enum
    {
      StateRefueling,
      StateLoading,
      StateIgnition,
      StateReady,
      StateCountdown,
      StateExploding,
      StateFlight,
      NumStates
    };

    int m_state;
    float m_fuel;
    int m_pipeCount;
    int m_passengers;
    float m_countdown;
    float m_damage;
    int m_spawnBuildingId;
    bool m_spawnCompleted;

  protected:
    void Refuel();
    void SetupSpectacle();
    void SetupAttackers();

    void AdvanceRefueling();
    void AdvanceLoading();
    void AdvanceIgnition();
    void AdvanceReady();
    void AdvanceCountdown();
    void AdvanceFlight();
    void AdvanceExploding();

    void SetupSounds();

  public:
    EscapeRocket();

    void Initialize(Building* _template) override;
    bool Advance() override;

    void ProvideFuel(float _level) override;
    bool SafeToLaunch();
    bool BoardRocket(WorldObjectId _id);
    void Damage(float _damage) override;

    bool IsSpectacle();
    bool IsInView() override;

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    void Read(TextReader* _in, bool _dynamic) override;
    

    const char* GetObjectiveCounter() override;

    static int GetStateId(const char* _state);
};

#endif
