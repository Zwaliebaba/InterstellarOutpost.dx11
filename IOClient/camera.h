#ifndef CAMERA_H
#define CAMERA_H

#include "LegacyVector3.h"
#include "entity.h"
#include "worldobject.h"

class Building;
class Teleport;
class CameraAnimation;

class Camera
{
  public:
    enum Mode
    {
      ModeReplay              = 0,
      ModeSphereWorld         = 1,
      ModeFreeMovement        = 2,		// Remember to update the static string table
      ModeBuildingFocus       = 3,		// at the end of camera.cpp when you update this
      ModeEntityTrack         = 4,
      ModeRadarAim            = 5,
      ModeFirstPerson         = 6,
      ModeMoveToTarget        = 7,
      ModeDoNothing           = 8,
      ModeEntityFollow        = 9,
      ModeTurretAim           = 10,
      ModeSphereWorldScripted = 11,
      ModeSphereWorldIntro    = 12,
      ModeSphereWorldOutro    = 13,
      ModeSphereWorldFocus    = 14,
      ModeMainMenu            = 15,
      ModeNumModes
    };

  protected:
    void AdvanceAnim();

    void AdvanceComponentZoom();
    void AdvanceComponentMouseWheelHeight();
    void AdvanceSphereWorldMode();
    void AdvanceFreeMovementMode();
    void AdvanceBuildingFocusMode();
    void AdvanceEntityTrackMode();
    void AdvanceRadarAimMode();
    void AdvanceFirstPersonMode();
    void AdvanceMoveToTargetMode();
    void AdvanceEntityFollowMode();
    void AdvanceTurretAimMode();
    void AdvanceSphereWorldScriptedMode();
    void AdvanceSphereWorldIntroMode();
    void AdvanceSphereWorldOutroMode();
    void AdvanceSphereWorldFocusMode();
    void AdvanceMainMenuMode();

    float DistanceToBlockage(const LegacyVector3& _dir, float _maxDist);
    float DirectDistanceToBlockage(const LegacyVector3& _from, const LegacyVector3& _to, float _maxDist);
    void GetHighestTangentPoint(const LegacyVector3& _from, const LegacyVector3& _to, float _maxDist, LegacyVector3& location);

    bool GetEntityToTrack(WorldObjectId& selection);
    void AdvanceAutomaticTracking();
    void RotateTowardsEntity(Entity* entity);

    bool AdvanceNotTooLow(LegacyVector3& targetCamera);
    bool AdvanceCanSeeUnits(LegacyVector3& targetCamera);
    bool AdvanceNotTooFarAway(LegacyVector3& targetCamera);
    bool AdvanceRopeModel(LegacyVector3& cameraTarget);
    bool AdvanceManualRotateCamera(LegacyVector3& cameraTarget);
    bool AdvanceManualCameraHeight(LegacyVector3& cameraTarget);

    void UpdateControlVector();		// updates the vector used by units in direct control for the purposes of determining directions

  private:
    LegacyVector3 m_pos;
    LegacyVector3 m_front;
    LegacyVector3 m_up;
    float m_fov;
    float m_cosFov;			// Updated once per frame from m_fov in SetupProjectionMatrix
    float m_maxFovRadians;    // Updated once per frame from m_fov in SetupProjectionMatrix
    float m_height;			// Distance above ground (in metres)
    LegacyVector3 m_vel;

    float m_minX, m_maxX;		// Bounds of camera movement
    float m_minZ, m_maxZ;
    LegacyVector3 m_targetPos;
    LegacyVector3 m_targetFront;
    LegacyVector3 m_targetUp;
    float m_targetFov;
    LegacyVector3 m_cameraTarget;		// Target Position of camera for automatic entity tracking
    LegacyVector3 m_predictedEntityPos;	// Predicted Position of entity for automatic entity tracking
    Entity* m_trackingEntity;	// The entity we are tracking

    LegacyVector3 m_startPos;			// Camera pos and orientation at the start of a "MoveToTarget"
    LegacyVector3 m_startFront;
    LegacyVector3 m_startUp;
    float m_startTime;
    float m_moveDuration;

    float m_distFromEntity;	// Used in EntityFollowMode and MicroUnitMode
    float m_currentDistance;	// Used in Manual Camera Rotation when tracking entities
    float m_heightMultiplier;

    int m_mode;
    int m_framesInThisMode;

    WorldObjectId m_objectId;		// WorldObjectId of creature to track
    float m_trackRange;
    float m_trackHeight;
    float m_trackTimer;
    LegacyVector3 m_trackVector;          // Used to rotate around tracking object

    CameraAnimation* m_anim;
    int m_animCurrentNode;
    float m_animNodeStartTime;
    int m_modeBeforeAnim;
    LegacyVector3 m_posBeforeAnim;
    LegacyVector3 m_upBeforeAnim;
    LegacyVector3 m_frontBeforeAnim;

    float m_cameraShake;

    bool m_entityTrack;         // the current state of automatic entity tracking
    LegacyVector3 m_controlVector;			// previous right value of the camera, if the unit is not directly below
    bool m_skipDirectionCalculation;

  public:
    Camera();

    LegacyVector3 GetPos() { return m_pos; }
    LegacyVector3 GetFront() { return m_front; }
    LegacyVector3 GetUp() { return m_up; }
    LegacyVector3 GetRight() { return m_up ^ m_front; }
    LegacyVector3 GetVel() { return m_vel; }
    float GetFov() { return m_fov; }

    void SetupProjectionMatrix(float _nearPlane, float _farPlane);
    void SetupModelviewMatrix();

    bool PosInViewFrustum(const LegacyVector3& _pos);
    bool SphereInViewFrustum(const LegacyVector3& _center, float _radius);

    void Advance();

    void RequestMode(int _mode);
    void RequestBuildingFocusMode(Building* _building, float _range, float _height);
    void RequestSphereFocusMode();
    void RequestRadarAimMode(Building* _building);
    void RequestEntityTrackMode(const WorldObjectId& _id);
    void RequestTurretAimMode(Building* _building);

    void CreateCameraShake(float _intensity);

    bool IsInteractive();
    bool IsInMode(int _mode);

    // SetTarget() only sets the target data. To make these changes take effect, call either
    // CutToTarget() or RequestMode(Camera::ModeMoveToTarget), depending on whether you
    // want an instant cut or a smooth transition
    void SetTarget(const LegacyVector3& _pos, const LegacyVector3& _front, const LegacyVector3& _up = g_upVector);
    bool SetTarget(std::string_view _mountName); // Returns false if mount not found
    void SetTarget(const LegacyVector3& _focusPos, float _distance, float _height);
    void SetMoveDuration(float _duration);
    void CutToTarget();
    void SetHeight(float _height);
    void SetFOV(float _fov);
    void SetTargetFOV(float _fov);

    void Normalize(); // Needs to be called reasonably regularly to prevent the front
    // and up vectors becoming non-orthogonal

    void GetClickRay(int _x, int _y, LegacyVector3* _rayStart, LegacyVector3* _rayDir);
    void Get2DScreenPos(const LegacyVector3& _vector, float* _screenX, float* _screenY);

    void SetBounds(float _minX, float _maxX, float _minZ, float _maxZ);

    void PlayAnimation(CameraAnimation* _anim);
    void StopAnimation();
    bool IsAnimPlaying();

    void SwitchEntityTracking(bool _onOrOff);
    void UpdateEntityTrackingMode();

    LegacyVector3 GetControlVector();
};

#endif
