#ifndef _included_powerstation_h
#define _included_powerstation_h

#include "building.h"

class Shape;
class ShapeFragment;
class ShapeMarker;

class Powerstation : public Building
{
  protected:
    int m_linkedBuildingId;

  public:
    Powerstation();

    void Initialize(Building* _template) override;

    bool Advance() override;
    void Render(float predictionTime) override;

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

    void Read(TextReader* _in, bool _dynamic) override;
};

#endif
