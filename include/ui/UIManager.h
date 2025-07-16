#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>

namespace nbody {

class Body;
class PhysicsEngine;
class Renderer;
struct PhysicsStats;
struct EnergyStats;
struct RenderStats;

/**
 * @brief ImGui-based user interface manager
 */
class UIManager {
public:
    UIManager();
    ~UIManager();

    /**
     * @brief Initialize ImGui
     * @param window GLFW window
     * @return True if initialization successful
     */
    bool Initialize(GLFWwindow* window);

    /**
     * @brief Render the UI
     * @param bodies Vector of bodies
     * @param physics Physics engine instance
     * @param renderer Renderer instance
     * @param selectedBody Currently selected body
     */
    void Render(const std::vector<std::unique_ptr<Body>>& bodies,
               const PhysicsEngine& physics,
               const Renderer& renderer,
               const Body* selectedBody);

    /**
     * @brief Render Barnes-Hut tree visualization
     * @param bodies Vector of bodies
     * @param physics Physics engine instance
     */
    void RenderBarnesHutVisualization(const std::vector<std::unique_ptr<Body>>& bodies,
                                     const PhysicsEngine& physics);

    /**
     * @brief Handle window resize
     * @param width New width
     * @param height New height
     */
    void OnWindowResize(int width, int height);

    /**
     * @brief Process ImGui events
     */
    void NewFrame();
    void EndFrame();

    /**
     * @brief Sync UI parameters from physics engine
     * @param physics Physics engine to sync from
     * @param renderer Renderer to sync from
     */
    void SyncFromEngines(const PhysicsEngine& physics, const Renderer& renderer);

    /**
     * @brief Check if mouse is over UI
     * @return True if mouse is over UI elements
     */
    bool IsMouseOverUI() const;

    /**
     * @brief Get UI settings
     */
    bool IsOrbitMode() const { return m_orbitMode; }
    bool IsShowingTrails() const { return m_showTrails; }
    bool IsShowingGrid() const { return m_showGrid; }
    bool IsShowingForces() const { return m_showForces; }
    bool IsShowingQuadTree() const { return m_showQuadTree; }
    bool IsShowingBarnesHut() const { return m_visualizeBarnesHut; }
    
    float GetNewBodyMass() const { return m_newBodyMass; }
    glm::vec3 GetNewBodyColor() const { return m_newBodyColor; }
    glm::vec2 GetNewBodyVelocity() const { return m_newBodyVelocity; }
    
    // Quick spawn getters
    int GetSpawnCount() const { return m_spawnCount; }
    float GetSpawnRadius() const { return m_spawnRadius; }
    float GetSpawnMass() const { return m_spawnMass; }
    float GetSpawnSpeed() const { return m_spawnSpeed; }
    int GetSpawnPattern() const { return m_spawnPattern; }
    
    // Physics parameters
    float GetGravitationalConstant() const { return m_gravitationalConstant; }
    float GetTimeStep() const { return m_timeStep; }
    float GetTimeScale() const { return m_timeScale; }
    float GetSofteningLength() const { return m_softeningLength; }
    bool GetUseBarnesHut() const { return m_useBarnesHut; }
    float GetBarnesHutTheta() const { return m_barnesHutTheta; }
    bool GetEnableCollisions() const { return m_enableCollisions; }
    float GetRestitution() const { return m_restitution; }
    
    // GPU settings
    void SetGPUAvailable(bool available) { m_gpuAvailable = available; }
    
    // Camera state
    void SetCameraPosition(const glm::vec2& position) { m_cameraPosition = position; }
    void SetCameraZoom(float zoom) { m_cameraZoom = zoom; }
    glm::vec2 GetCameraPosition() const { return m_cameraPosition; }
    float GetCameraZoom() const { return m_cameraZoom; }
    
    // Callbacks for UI events
    std::function<void()> OnPlayPause;
    std::function<void()> OnReset;
    std::function<void()> OnClear;
    std::function<void(const std::string&)> OnLoadPreset;
    std::function<void(Body*)> OnDeleteBody;
    std::function<void(const std::string&)> OnSaveConfig;
    std::function<void(const std::string&)> OnLoadConfig;
    std::function<void()> OnPhysicsParameterChanged;
    std::function<void()> OnRenderParameterChanged;
    std::function<void(int)> OnTrailLengthChanged;  // Trail length parameter
    std::function<void()> OnResetCamera;
    std::function<void()> OnFitAllBodies;
    std::function<void(int, int)> OnSpawnBodies;  // (count, pattern)
    std::function<void(const glm::vec2&)> OnSetCameraPosition;
    std::function<void(float)> OnSetCameraZoom;
    std::function<void()> OnRunBenchmark;  // Performance benchmarking callback
    
private:
    // Window state
    // GLFWwindow* m_window = nullptr;
    // int m_monitorWidth = 0;
    // int m_monitorHeight = 0;
    int m_windowWidth = 1200;
    int m_windowHeight = 800;


    
    // UI state
    bool m_showMainWindow = true;
    bool m_showStatsWindow = true;
    bool m_showControlsWindow = true;
    bool m_showBodyWindow = false; // Body property panel hidden by default
    bool m_showDebugWindow = false;
    bool m_showAboutWindow = false;
    bool m_showBarnesHutWindow = false;
    
    // Simulation controls
    bool m_orbitMode = false;
    bool m_showTrails = true;
    bool m_showGrid = false;
    bool m_showForces = false;
    bool m_showQuadTree = false;
    bool m_visualizeBarnesHut = false;
    int m_maxTreeDepthToShow = 5;
    float m_treeNodeAlpha = 0.5f;
    ImU32 m_treeColor = IM_COL32(0, 255, 0, 128);
    ImU32 m_treeCenterColor = IM_COL32(255, 0, 0, 192);
    
    // New body settings
    float m_newBodyMass = 50.0f;
    glm::vec3 m_newBodyColor{1.0f, 1.0f, 1.0f};
    glm::vec2 m_newBodyVelocity{0.0f, 0.0f};
    int m_trailLength = 100;
    
    // Quick spawn settings
    int m_spawnCount = 100;
    float m_spawnRadius = 20.0f;
    float m_spawnMass = 1.0f;
    float m_spawnSpeed = 5.0f;
    int m_spawnPattern = 0;  // 0=Random, 1=Circle, 2=Grid, 3=Spiral
    
    // Physics settings
    float m_gravitationalConstant = 10.0f;
    float m_timeStep = 0.016f;
    float m_timeScale = 1.0f;
    float m_softeningLength = 1.0f;
    bool m_useBarnesHut = true;
    float m_barnesHutTheta = 0.5f;
    bool m_enableCollisions = true;
    float m_restitution = 0.8f;
    bool m_useGPU = false;
    bool m_gpuAvailable = false; // Track GPU availability
    
    // Rendering settings
    float m_cameraZoom = 0.0f;
    glm::vec2 m_cameraPosition{0.0f};
    
    // Performance tracking
    std::vector<float> m_fpsHistory;
    std::vector<float> m_energyHistory;
    size_t m_maxHistorySize = 100;
    
    // Energy tracking
    size_t m_energyHistoryIndex = 0;
    
    // File dialog state
    std::string m_configFilename = "config.json";
    
    // Style settings
    void SetupStyle();
    
    // Render individual panels
    void RenderMainMenuBar();
    void RenderControlPanel();
    void RenderStatsPanel(const std::vector<std::unique_ptr<Body>>& bodies,
                         const PhysicsEngine& physics,
                         const Renderer& renderer);
    void RenderBodyPanel(const Body* selectedBody);
    void RenderDebugPanel();
    void RenderAboutPanel();
    void RenderBarnesHutPanel(const std::vector<std::unique_ptr<Body>>& bodies,
                             const PhysicsEngine& physics);
    
    // Utility functions
    void ShowPhysicsStats(const PhysicsStats& stats);
    void ShowEnergyStats(const EnergyStats& stats);
    void ShowRenderStats(const RenderStats& stats);
    void ShowPerformanceGraph();
    void ShowEnergyGraph();
    
    // Preset management
    void ShowPresetButtons();
    void ShowConfigButtons();
    
    // Body editing
    void ShowBodyEditor(const Body* body);
    void ShowBodyCreator();
    
    // Help and tooltips
    void ShowHelpMarker(const char* desc);
    void ShowTooltip(const char* text);
    
    // Enhanced UI controls
    bool SliderFloatWithInput(const char* label, float* value, float minVal, float maxVal, 
                             float defaultVal, const char* format = "%.3f", const char* helpText = nullptr);
    bool CheckboxWithReset(const char* label, bool* value, bool defaultVal, const char* helpText = nullptr);
    void ShowResetButton(const char* id, const std::function<void()>& resetCallback);
    void ResetPhysicsParameters();
    
    // Spawn preview calculation
    float CalculatePreviewRadius(int count, int pattern, float baseRadius);
    
    // Color utilities
    ImU32 Vec3ToImU32(const glm::vec3& color);
    glm::vec3 ImU32ToVec3(ImU32 color);
    
    // Constants
    static constexpr float PANEL_WIDTH = 300.0f;
    static constexpr float PANEL_HEIGHT = 200.0f;
    static constexpr float MARGIN = 10.0f;
    
    // Default physics parameter values (from PhysicsConfig)
    static constexpr float DEFAULT_GRAVITATIONAL_CONSTANT = 1.0f;
    static constexpr float DEFAULT_TIME_STEP = 0.016f;
    static constexpr float DEFAULT_TIME_SCALE = 1.0f;
    static constexpr float DEFAULT_SOFTENING_LENGTH = 0.1f;
    static constexpr bool DEFAULT_USE_BARNES_HUT = true;
    static constexpr float DEFAULT_BARNES_HUT_THETA = 0.7f;
    static constexpr bool DEFAULT_ENABLE_COLLISIONS = true;
    static constexpr float DEFAULT_RESTITUTION = 0.8f;
    static constexpr bool DEFAULT_USE_GPU = false;
    
    // Default body creation values
    static constexpr float DEFAULT_NEW_BODY_MASS = 10.0f;
    static constexpr float DEFAULT_SPAWN_RADIUS = 20.0f;
    static constexpr float DEFAULT_SPAWN_MASS = 1.0f;
    static constexpr float DEFAULT_SPAWN_SPEED = 5.0f;
    static constexpr int DEFAULT_SPAWN_COUNT = 100;
};

} // namespace nbody
