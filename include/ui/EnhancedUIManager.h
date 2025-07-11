#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace nbody {

class Application;
class PhysicsSolver;
enum class PhysicsAlgorithm;

/**
 * @brief Enhanced UI manager with simulation controls
 */
class EnhancedUIManager {
public:
    EnhancedUIManager(Application* app);
    ~EnhancedUIManager();
    
    bool Initialize();
    void Render();
    void Shutdown();
    
    // UI state
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }

private:
    Application* m_app;
    bool m_visible = true;
    
    // UI panels
    void RenderMainControlPanel();
    void RenderSimulationControls();
    void RenderPhysicsSettings();
    void RenderRenderingSettings();
    void RenderParticleSystemControls();
    void RenderPerformancePanel();
    void RenderPresetPanel();
    
    // Settings state
    struct SimulationSettings {
        bool paused = false;
        float timeStep = 0.016f;
        float gravitationalConstant = 6.67430e-11f;
        float softeningParameter = 0.1f;
        PhysicsAlgorithm currentAlgorithm;
        int targetFPS = 60;
    } m_simulationSettings;
    
    struct RenderingSettings {
        bool bloomEnabled = true;
        float bloomIntensity = 1.0f;
        float bloomThreshold = 0.8f;
        bool pointSizeEnabled = true;
        float baseParticleSize = 2.0f;
        float velocityColorScale = 1.0f;
        bool showTrails = false;
        float trailLength = 100.0f;
    } m_renderingSettings;
    
    struct ParticleSystemSettings {
        int particleCount = 1000;
        int selectedInitializer = 0;
        float galaxyArmCount = 2.0f;
        float galaxyArmTightness = 0.5f;
        float centralMassRatio = 0.1f;
    } m_particleSettings;
    
    // Performance monitoring
    struct PerformanceMetrics {
        float frameTime = 0.0f;
        float fps = 0.0f;
        float physicsTime = 0.0f;
        float renderTime = 0.0f;
        int particleCount = 0;
        std::string algorithmName;
    } m_performance;
    
    void UpdatePerformanceMetrics();
    void ApplySimulationSettings();
    void ApplyRenderingSettings();
    void RegenerateParticleSystem();
};

} // namespace nbody
