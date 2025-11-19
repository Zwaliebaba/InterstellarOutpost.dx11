#ifndef _included_goddish_h
#define _included_goddish_h

#include "building.h"

class GodDish : public Building
{
  public:
    bool m_activated;
    float m_timer;
    int m_numSpawned;
    bool m_spawnSpam;

    GodDish();

    void Initialize(Building* _template) override;

    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    bool IsInView() override;

    void Activate();
    void DeActivate();
    void SpawnSpam(bool _isResearch) const;

    void TriggerSpam();
};

#endif
