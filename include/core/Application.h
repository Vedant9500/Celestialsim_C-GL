#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace nbody {

// Forward declarations
class Body;
class PhysicsEngine;
class Renderer;
class UIManager;

/**
 * @brief Main application class that manages the N-body simulation
 */
class Application {
public:
    Application();
    ~Application();

    /**
     * @brief Initialize the application
     * @return True if initialization was successful
     */
    bool Initialize();

    /**
     * @brief Run the main application loop
     */
    void Run();

    /**
     * @brief Shutdown the application
     */
    void Shutdown();

private:
    // Core components
    std::unique_ptr<PhysicsEngine> m_physics;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<UIManager> m_ui;

    // Simulation state
    std::vector<std::unique_ptr<Body>> m_bodies;
    bool m_running = false;
    bool m_paused = false;
    
    // Timing
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    float m_deltaTime = 0.0f;
    float m_fps = 0.0f;
    
    // Input state
    glm::vec2 m_mousePosition{0.0f};
    glm::vec2 m_worldMousePosition{0.0f};
    bool m_mouseDown = false;
    bool m_rightMouseDown = false;
    Body* m_selectedBody = nullptr;
    Body* m_draggedBody = nullptr;
    
    // Camera state
    glm::vec2 m_cameraPosition{0.0f};
    float m_cameraZoom = 1.0f;
    
    // UI state
    bool m_showUI = true;
    bool m_showDebugInfo = false;
    bool m_orbitMode = false;

    // Private methods
    void Update(float deltaTime);
    void Render();
    void HandleInput();
    void UpdatePhysics(float deltaTime);
    void UpdateUI();
    
    // Event handlers
    void OnMouseMove(double x, double y);
    void OnMouseButton(int button, int action, int mods);
    void OnMouseScroll(double xOffset, double yOffset);
    void OnKeyboard(int key, int scancode, int action, int mods);
    void OnWindowResize(int width, int height);
    
    // Body management
    void AddBody(const glm::vec2& position, const glm::vec2& velocity, float mass);
    void RemoveBody(Body* body);
    void ClearBodies();
    Body* FindBodyAtPosition(const glm::vec2& position);
    
    // Coordinate conversion
    glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 WorldToScreen(const glm::vec2& worldPos) const;
    
    // Presets
    void LoadPreset(const std::string& name);
    void CreateSolarSystem();
    void CreateBinarySystem();
    void CreateGalaxySpiral();
    void CreateRandomCluster(int count);
    void SpawnBodies(int count, int pattern);
    
    // Performance monitoring
    void UpdatePerformanceMetrics();
    
    static Application* s_instance;
    
    // Static callback wrappers for GLFW
    static void MouseMoveCallback(GLFWwindow* window, double x, double y);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void CharCallback(GLFWwindow* window, unsigned int codepoint);
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);
    static void ErrorCallback(int error, const char* description);
};

} // namespace nbody
