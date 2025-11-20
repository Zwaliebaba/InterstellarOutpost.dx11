#pragma once

#include "building.h"

class DynamicBase : public Building
{
public:
  char m_shapeName[256];

  DynamicBase(BuildingType _type);

  void Initialize(Building *_template) override;
  bool Advance() override;
  void Render(float _predictionTime) override;

  void Read(TextReader *_in, bool _dynamic) override;

  int GetBuildingLink() override;
  void SetBuildingLink(int _buildingId) override;

  void SetShapeName(char *_shapeName);

protected:
  int m_buildingLink;
};

class DynamicHub : public DynamicBase
{
public:
  DynamicHub();

  void Initialize(Building *_template) override;

  void ReprogramComplete() override;
  const char *GetObjectiveCounter() override;

  bool Advance() override;
  void Render(float _predictionTime) override;

  void ActivateLink();
  void DeactivateLink();

  bool ChangeScore(int _points);

  void Read(TextReader *_in, bool _dynamic) override;

  int PointsPerHub(); // the number of points each node supplies if there is a minimum active node limit

  int m_currentScore;  // the current number of 'points' this hub has recieved from connected Nodes
  int m_requiredScore; // the required number of 'points' before this Hub will activate

  // if scores mode is being used, this is the mimimum number of buildings required in addition to the score requirement
  int m_minActiveLinks; 

protected:
  bool m_enabled;      // set to true once all connected nodes are active
  bool m_reprogrammed; // set to true if a connected Control Tower is reprogrammed, or no tower is connected

  int m_numLinks;    // the number of Nodes linked to this Hub
  int m_activeLinks; // the number of active nodes linked to this hub
};

class DynamicNode : public DynamicBase
{
public:
  int
      m_scoreValue; // the number of points that will be added to the connected hubs score every second, if this Node is active
  float m_scoreTimer;
  int m_scoreSupplied; // the number of points this node has already given the hub

  DynamicNode();

  void Initialize(Building *_template) override;
  bool Advance() override;

  void Render(float _predictionTime) override;
  void RenderPorts() override;
  void RenderAlphas(float _predictionTime) override;

  void ReprogramComplete() override;

  void Read(TextReader *_in, bool _dynamic) override;

protected:
  bool m_operating;
};
