#ifndef _included_trunkport_h
#define _included_trunkport_h

#include "building.h"

#define TRUNKPORT_HEIGHTMAP_MAXSIZE 16

class TrunkPort : public Building
{
  public:
    TrunkPort();

    void Initialize(Building* _template) override;
    bool Advance() override;
    void Render(float predictionTime) override;
    void RenderAlphas(float predictionTime) override;

    bool PerformDepthSort(LegacyVector3& _centerPos) override;

    void ReprogramComplete() override;

    void Read(TextReader* _in, bool _dynamic) override;

    int m_targetLocationId;

    ShapeMarker* m_destination1;
    ShapeMarker* m_destination2;

    int m_heightMapSize;
    std::vector<LegacyVector3> m_heightMap;
    float m_openTimer;
};

#endif
