#ifndef _included_incubator_h
#define _included_incubator_h

#include "fast_darray.h"
#include "building.h"
#include "entity.h"
#include "spirit.h"

#define INCUBATOR_PROCESSTIME       5.0f

struct IncubatorIncoming
{
  LegacyVector3 m_pos;
  int m_entrance;
  float m_alpha;
};

class Incubator : public Building
{
  protected:
    FastDArray<Spirit> m_spirits;
    ShapeMarker* m_spiritCenter;
    ShapeMarker* m_exit;
    ShapeMarker* m_dock;
    ShapeMarker* m_spiritEntrance[3];

    EntityType m_troopType;
    float m_timer;

    LList<IncubatorIncoming*> m_incoming;

  public:
    int m_numStartingSpirits;

    Incubator();
    ~Incubator() override;

    void Initialize(Building* _template) override;

    bool Advance() override;
    void SpawnEntity();
    void AddSpirit(Spirit* _spirit);

    void RenderAlphas(float _predictionTime) override;

    int NumSpiritsInside();

    void Read(TextReader* _in, bool _dynamic) override;
    

    void GetDockPoint(LegacyVector3& _pos, LegacyVector3& _front);
};

#endif
