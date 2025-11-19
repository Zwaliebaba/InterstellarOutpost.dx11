#pragma once

#define PIXEL_EFFECT_GRID_RES	16

class Renderer
{
  public:
    int m_fps;
    bool m_displayFPS;
    bool m_renderDebug;
    bool m_displayInputMode;

  private:
    float m_nearPlane;
    float m_farPlane;
    int m_screenW;
    int m_screenH;
    int m_tileIndex;			// Used when rendering a poster

    float m_fadedness;			// 1.0 means black screen. 0.0 means not fade out at all.
    float m_fadeRate;				// +ve means fading out, -ve means fading in
    float m_fadeDelay;			// Amount of time left to wait before starting fade

    void RenderFadeOut();
    void RenderFrame(bool withFlip = true);

  public:
    Renderer();

    void Initialize();

    void Render();
    void FPSMeterAdvance();

    float GetNearPlane() const;
    float GetFarPlane() const;
    void SetNearAndFar(float _nearPlane, float _farPlane);

    void SetOpenGLState() const;

    void SetObjectLighting() const;
    void UnsetObjectLighting() const;

    void SetupProjMatrixFor3D() const;
    void SetupMatricesFor3D() const;
    void SetupMatricesFor2D() const;

    bool IsFadeComplete() const;
    void StartFadeOut();
    void StartFadeIn(float _delay);
};
