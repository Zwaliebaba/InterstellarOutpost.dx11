#ifndef _included_blueprintstore_h
#define _included_blueprintstore_h

#include "building.h"

class BlueprintBuilding : public Building
{
  public:
    BlueprintBuilding(BuildingType _type);

    void Initialize(Building* _template) override;
    bool Advance() override;
    bool IsInView() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    virtual void SendBlueprint(int _segment, bool _infected);

    Matrix34 GetMarker(float _predictionTime);

    void Read(TextReader* _in, bool _dynamic) override;

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

  public:
    int m_buildingLink;
    float m_infected;
    int m_segment;

  protected:
    ShapeMarker* m_marker;
    LegacyVector3 m_vel;
};

// ============================================================================

#define BLUEPRINTSTORE_NUMSEGMENTS  4

class BlueprintStore : public BlueprintBuilding
{
  public:
    float m_segments[BLUEPRINTSTORE_NUMSEGMENTS];

    BlueprintStore();

    void GetDisplay(LegacyVector3& _pos, LegacyVector3& _right, LegacyVector3& _up, float& _size);

    void Initialize(Building* _template) override;
    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;
    void SendBlueprint(int _segment, bool _infected) override;

    const char* GetObjectiveCounter() override;

    int GetNumInfected();                           // Returns number of segments totally infected ie == 100.0f
    int GetNumClean();                              // Returns number of segments totally clean ie == 0.0f
};

// ============================================================================

class BlueprintConsole : public BlueprintBuilding
{
  public:
    BlueprintConsole();

    void Initialize(Building* _template) override;

    void RecalculateOwnership();
    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderPorts() override;

    void Read(TextReader* _in, bool _dynamic) override;
    
};

// ============================================================================

class BlueprintRelay : public BlueprintBuilding
{
  public:
    float m_altitude;

    BlueprintRelay();

    void Initialize(Building* _template) override;

    bool Advance() override;

    void Read(TextReader* _in, bool _dynamic) override;
    
};

#endif
