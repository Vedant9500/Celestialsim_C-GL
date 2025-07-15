#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>

namespace nbody {

class Body;
class PhysicsEngine;
struct QuadTreeNode;

/**
 * @brief Camera for 2D rendering
 */
struct Camera {
    glm::vec2 position{0.0f};
    float zoom = 1.0f;
    float targetZoom = 1.0f;
    float zoomSpeed = 0.1f;
    
    glm::mat4 GetViewMatrix() const {
        return glm::scale(glm::translate(glm::mat4(1.0f), 
                         glm::vec3(-position.x, -position.y, 0.0f)),
                         glm::vec3(zoom, zoom, 1.0f));
    }
    
    glm::mat4 GetProjectionMatrix(float width, float height) const {
        float aspect = width / height;
        return glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    }
    
    void Update(float /*deltaTime*/) {
        zoom += (targetZoom - zoom) * zoomSpeed;
    }
};

/**
 * @brief Rendering statistics
 */
struct RenderStats {
    int bodiesRendered = 0;
    int trailsRendered = 0;
    int drawCalls = 0;
    double renderTime = 0.0;
    double trailUpdateTime = 0.0;
    double bloomTime = 0.0;
};

/**
 * @brief Main renderer for N-body simulation
 */
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    /**
     * @brief Initialize the renderer
     * @param width Window width
     * @param height Window height
     * @return True if initialization successful
     */
    bool Initialize(int width, int height);
    
    /**
     * @brief Render all bodies
     * @param bodies Vector of bodies to render
     * @param physics Physics engine instance
     * @param selectedBody Currently selected body (highlighted)
     */
    void Render(const std::vector<std::unique_ptr<Body>>& bodies,
               const PhysicsEngine& physics,
               const Body* selectedBody);
    
    /**
     * @brief Clear the window
     */
    void Clear();
    
    /**
     * @brief Update trail points for all bodies
     * @param bodies Vector of bodies
     * @param deltaTime Time step in seconds
     */
    void UpdateTrails(const std::vector<std::unique_ptr<Body>>& bodies, float deltaTime);
    
    /**
     * @brief Resize the window
     * @param width New width
     * @param height New height
     */
    void Resize(int width, int height);
    
    // Camera controls
    void ZoomIn(float factor) { m_camera.targetZoom *= factor; }
    void ZoomOut(float factor) { m_camera.targetZoom /= factor; }
    
    /**
     * @brief Pan the camera by a screen-space delta
     * @param delta Screen-space delta (in pixels)
     */
    void PanCamera(const glm::vec2& delta) { 
        // Apply the zoom factor to convert from screen to world space
        m_camera.position += delta / m_camera.zoom; 
    }
    
    void SetCameraPosition(const glm::vec2& position) { m_camera.position = position; }
    void SetCameraZoom(float zoom) { m_camera.zoom = zoom; m_camera.targetZoom = zoom; }
    const Camera& GetCamera() const { return m_camera; }
    
    // Convert between screen and world coordinates
    glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 WorldToScreen(const glm::vec2& worldPos) const;
    
    // Getters and setters
    void SetBackgroundColor(const glm::vec3& color) { m_backgroundColor = color; }
    void SetShowTrails(bool show) { m_showTrails = show; }
    void SetTrailColor(const glm::vec3& color) { m_trailColor = color; }
    void SetTrailLength(float length) { m_trailLength = length; }
    void SetPointSize(float size) { m_pointSize = size; }
    void SetShowGrid(bool show) { m_showGrid = show; }
    void SetGridColor(const glm::vec3& color) { m_gridColor = color; }
    void SetShowQuadTree(bool show) { m_showQuadTree = show; }
    void SetBloomEnabled(bool enabled) { m_useBloom = enabled; }
    void SetBloomIntensity(float intensity) { m_bloomIntensity = intensity; }
    void SetBloomThreshold(float threshold) { m_bloomThreshold = threshold; }
    bool IsBloomEnabled() const { return m_useBloom; }
    
    const RenderStats& GetStats() const { return m_stats; }
    
    // Utility
    void FitAllBodies(const std::vector<std::unique_ptr<Body>>& bodies);
    
private:
    // Window state
    int m_windowWidth = 1200;
    int m_windowHeight = 800;
    
    // Camera and view
    Camera m_camera;
    
    // Shader programs
    GLuint m_bodyProgram = 0;
    GLuint m_trailProgram = 0;
    GLuint m_gridProgram = 0;
    GLuint m_quadTreeProgram = 0;
    
    // Buffers and VAOs
    GLuint m_bodyVAO = 0;
    GLuint m_bodyVBO = 0;
    GLuint m_trailVAO = 0;
    GLuint m_trailVBO = 0;
    GLuint m_gridVAO = 0;
    GLuint m_gridVBO = 0;
    GLuint m_quadTreeVAO = 0;
    GLuint m_quadTreeVBO = 0;
    
    // Render settings
    glm::vec3 m_backgroundColor = glm::vec3(0.05f, 0.05f, 0.05f);
    bool m_showTrails = true;
    bool m_showGrid = false;
    bool m_showQuadTree = false;
    glm::vec3 m_trailColor = glm::vec3(0.5f, 0.5f, 0.7f);
    glm::vec3 m_gridColor = glm::vec3(0.2f, 0.2f, 0.2f);
    float m_trailLength = 100.0f; // Number of frames to keep trail
    float m_pointSize = 3.0f;
    
    // Bloom settings
    bool m_useBloom = true;
    float m_bloomIntensity = 1.5f;
    float m_bloomThreshold = 0.85f;
    
    // Trail data
    struct TrailPoint {
        glm::vec2 position;
        glm::vec3 color;
        float age;
    };
    
    std::unordered_map<const Body*, std::vector<TrailPoint>> m_trails;
    
    // Statistics
    RenderStats m_stats;
    
    // Utility functions
    void RenderBodies(const std::vector<std::unique_ptr<Body>>& bodies, const Body* selectedBody);
    void RenderTrails();
    void RenderGrid();
    void RenderQuadTree(const PhysicsEngine& physics);
    
    bool InitializeShaders();
    GLuint LoadShader(const std::string& vertexPath, const std::string& fragmentPath);
    
    // Timer functions
    std::chrono::time_point<std::chrono::high_resolution_clock> m_frameStart;
    void StartTimer();
    void EndTimer(double& timeAccumulator);
    
    // Generate static geometry
    void GenerateGridGeometry();
};

} // namespace nbody
