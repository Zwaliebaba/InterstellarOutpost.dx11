#pragma once

#include "building.h"

#define YARD_NUMPRIMITIVES 9
#define YARD_NUMRUNGSPIKES 6

class ConstructionYard : public Building
{
  protected:
    Shape* m_rung;
    Shape* m_primitive;
    ShapeMarker* m_primitives[YARD_NUMPRIMITIVES];
    ShapeMarker* m_rungSpikes[YARD_NUMRUNGSPIKES];

    int m_numPrimitives;
    int m_numSurges;
    int m_numTanksProduced;
    float m_fractionPopulated;
    float m_timer;

    float m_alpha;

    bool IsPopulationLocked();                       // Are there too many tanks already

  public:
    ConstructionYard();

    bool Advance() override;

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    Matrix34 GetRungMatrix1();
    Matrix34 GetRungMatrix2();

    bool AddPrimitive();
    void AddPowerSurge();
};

// ****************************************************************************
// Class DisplayScreen
// ****************************************************************************

#define DISPLAYSCREEN_NUMRAYS 3

class DisplayScreen : public Building
{
  protected:
    Shape* m_armour;
    ShapeMarker* m_rays[DISPLAYSCREEN_NUMRAYS];

  public:
    DisplayScreen();

    void RenderAlphas(float _predictionTime) override;
};
