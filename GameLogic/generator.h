#pragma once

#include "building.h"

class PowerBuilding : public Building
{
  public:
    PowerBuilding(BuildingType _type);

    void Initialize(Building* _template) override;
    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    bool IsInView() override;
    LegacyVector3 GetPowerLocation();
    virtual void TriggerSurge(float _initValue);

    void Read(TextReader* _in, bool _dynamic) override;

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

  protected:
    int m_powerLink;
    ShapeMarker* m_powerLocation;

    LList<float> m_surges;
};

class Generator : public PowerBuilding
{
  public:
    float m_throughput;

    Generator();

    void TriggerSurge(float _initValue) override;

    void ReprogramComplete() override;

    const char* GetObjectiveCounter() override;

    bool Advance() override;
    void Render(float _predictionTime) override;

  protected:
    ShapeMarker* m_counter;

    float m_timerSync;
    int m_numThisSecond;
    bool m_enabled;
};

class Pylon : public PowerBuilding
{
  public:
    Pylon();
    bool Advance() override;
};

class PylonStart : public PowerBuilding
{
  public:
    int m_reqBuildingId;

    PylonStart();

    void Initialize(Building* _template) override;
    bool Advance() override;

    void Read(TextReader* _in, bool _dynamic) override;
};

class PylonEnd : public PowerBuilding
{
  public:
    PylonEnd();

    void TriggerSurge(float _initValue) override;
};

#define SOLARPANEL_NUMGLOWS 4
#define SOLARPANEL_NUMSTATUSMARKERS 5

class SolarPanel : public PowerBuilding
{
  public:
    SolarPanel();

    void Initialize(Building* _template) override;
    bool Advance() override;

    void Render(float _predictionTime) override;
    void RenderPorts() override;
    void RenderAlphas(float _predictionTime) override;

  protected:
    ShapeMarker* m_glowMarker[SOLARPANEL_NUMGLOWS];
    ShapeMarker* m_statusMarkers[SOLARPANEL_NUMSTATUSMARKERS];

    bool m_operating;
};
