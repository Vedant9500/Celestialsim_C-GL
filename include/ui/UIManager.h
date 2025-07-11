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
     * @param physics Physics engine reference
     * @param renderer Renderer reference
     * @param selectedBody Currently selected body
     */
    void Render(const std::vector<std::unique_ptr<Body>>& bodies,
               const PhysicsEngine& physics,
               const Renderer& renderer,
               const Body* selectedBody);

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
    
    float GetNewBodyMass() const { return m_newBodyMass; }
    glm::vec3 GetNewBodyColor() const { return m_newBodyColor; }
    glm::vec2 GetNewBodyVelocity() const { return m_newBodyVelocity; }
    
    // Physics parameters
    float GetGravitationalConstant() const { return m_gravitationalConstant; }
    float GetTimeStep() const { return m_timeStep; }
    float GetSofteningLength() const { return m_softeningLength; }
    bool GetUseBarnesHut() const { return m_useBarnesHut; }
    float GetBarnesHutTheta() const { return m_barnesHutTheta; }
    bool GetEnableCollisions() const { return m_enableCollisions; }
    float GetRestitution() const { return m_restitution; }
    
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
    std::function<void()> OnResetCamera;
    std::function<void()> OnFitAllBodies;
    std::function<void(const glm::vec2&)> OnSetCameraPosition;
    std::function<void(float)> OnSetCameraZoom;
    
private:
    // Window state
    int m_windowWidth = 1200;
    int m_windowHeight = 800;
    
    // UI state
    bool m_showMainWindow = true;
    bool m_showStatsWindow = true;
    bool m_showControlsWindow = true;
    bool m_showBodyWindow = true;
    bool m_showDebugWindow = false;
    bool m_showAboutWindow = false;
    
    // Simulation controls
    bool m_orbitMode = false;
    bool m_showTrails = true;
    bool m_showGrid = false;
    bool m_showForces = false;
    bool m_showQuadTree = false;
    
    // New body settings
    float m_newBodyMass = 50.0f;
    glm::vec3 m_newBodyColor{1.0f, 1.0f, 1.0f};
    glm::vec2 m_newBodyVelocity{0.0f, 0.0f};
    int m_trailLength = 100;
    
    // Physics settings
    float m_gravitationalConstant = 100.0f;
    float m_timeStep = 0.016f;
    float m_softeningLength = 2.0f;
    bool m_useBarnesHut = true;
    float m_barnesHutTheta = 0.5f;
    bool m_enableCollisions = true;
    float m_restitution = 0.8f;
    bool m_useGPU = false;
    
    // Rendering settings
    float m_cameraZoom = 1.0f;
    glm::vec2 m_cameraPosition{0.0f};
    
    // Performance tracking
    std::vector<float> m_fpsHistory;
    std::vector<float> m_energyHistory;
    size_t m_maxHistorySize = 100;
    
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
    
    // Color utilities
    ImU32 Vec3ToImU32(const glm::vec3& color);
    glm::vec3 ImU32ToVec3(ImU32 color);
    
    // Constants
    static constexpr float PANEL_WIDTH = 300.0f;
    static constexpr float PANEL_HEIGHT = 200.0f;
    static constexpr float MARGIN = 10.0f;
};

} // namespace nbody
