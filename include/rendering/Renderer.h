#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm> // For std::max
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
    float zoom = 0.001f; // Set to minimum zoom to see everything
    float targetZoom = 0.001f; // Set target zoom to minimum as well
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
        zoom = std::max(0.0001f, zoom); // Ensure zoom never goes below minimum
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
    double frameTime = 0.0;
    float fps = 0.0f;
    std::string renderer = "OpenGL";
};

/**
 * @brief OpenGL shader wrapper
 */
class Shader {
public:
    Shader() = default;
    ~Shader();

    bool LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    bool LoadFromString(const std::string& vertexSource, const std::string& fragmentSource);
    
    void Use() const;
    void Unuse() const;
    
    // Uniform setters
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMat4(const std::string& name, const glm::mat4& value) const;
    
    GLuint GetProgram() const { return m_program; }
    bool IsValid() const { return m_program != 0; }
    
private:
    GLuint m_program = 0;
    mutable std::unordered_map<std::string, GLint> m_uniformCache;
    
    GLuint CompileShader(const std::string& source, GLenum type);
    GLint GetUniformLocation(const std::string& name) const;
    std::string ReadFile(const std::string& path) const;
};

/**
 * @brief High-performance OpenGL renderer
 */
class Renderer {
public:
    Renderer();
    ~Renderer();

    /**
     * @brief Initialize OpenGL context and resources
     * @param window GLFW window
     * @return True if initialization successful
     */
    bool Initialize(GLFWwindow* window);

    /**
     * @brief Render the simulation
     * @param bodies Vector of bodies to render
     * @param physics Physics engine for additional data
     * @param selectedBody Currently selected body
     * @return Rendering statistics
     */
    RenderStats Render(const std::vector<std::unique_ptr<Body>>& bodies,
                      const PhysicsEngine& physics,
                      const Body* selectedBody = nullptr);

    /**
     * @brief Handle window resize
     * @param width New width
     * @param height New height
     */
    void OnWindowResize(int width, int height);

    // Camera controls
    Camera& GetCamera() { return m_camera; }
    const Camera& GetCamera() const { return m_camera; }
    
    void SetCameraPosition(const glm::vec2& position) { m_camera.position = position; }
    void SetCameraZoom(float zoom) { m_camera.targetZoom = std::max(0.0001f, zoom); } // Ensure minimum zoom
    void ZoomIn(float factor = 1.2f) { m_camera.targetZoom *= factor; }
    void ZoomOut(float factor = 1.2f) { 
        m_camera.targetZoom /= factor;
        m_camera.targetZoom = std::max(0.0001f, m_camera.targetZoom); // Prevent zoom from going too low
    }
    void PanCamera(const glm::vec2& delta) { m_camera.position += delta; }
    
    // Coordinate conversion
    glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 WorldToScreen(const glm::vec2& worldPos) const;
    
    // Rendering options
    void SetShowTrails(bool show) { m_showTrails = show; }
    void SetShowGrid(bool show) { m_showGrid = show; }
    void SetShowForces(bool show) { m_showForces = show; }
    void SetShowQuadTree(bool show) { m_showQuadTree = show; }
    void SetShowUI(bool show) { m_showUI = show; }
    
    bool GetShowTrails() const { return m_showTrails; }
    bool GetShowGrid() const { return m_showGrid; }
    bool GetShowForces() const { return m_showForces; }
    bool GetShowQuadTree() const { return m_showQuadTree; }
    bool GetShowUI() const { return m_showUI; }
    
    // Utility
    void FitAllBodies(const std::vector<std::unique_ptr<Body>>& bodies);
    void CenterOnBody(const Body* body);
    
    // Statistics
    const RenderStats& GetStats() const { return m_stats; }
    
private:
    // OpenGL state
    GLFWwindow* m_window = nullptr;
    int m_windowWidth = 1200;
    int m_windowHeight = 800;
    
    // Camera
    Camera m_camera;
    
    // Shaders
    std::unique_ptr<Shader> m_bodyShader;
    std::unique_ptr<Shader> m_trailShader;
    std::unique_ptr<Shader> m_gridShader;
    std::unique_ptr<Shader> m_forceShader;
    std::unique_ptr<Shader> m_quadTreeShader;
    
    // Vertex buffers
    GLuint m_bodyVAO = 0;
    GLuint m_bodyVBO = 0;
    GLuint m_bodyInstanceVBO = 0;
    
    GLuint m_trailVAO = 0;
    GLuint m_trailVBO = 0;
    
    GLuint m_gridVAO = 0;
    GLuint m_gridVBO = 0;
    
    GLuint m_forceVAO = 0;
    GLuint m_forceVBO = 0;
    
    GLuint m_quadTreeVAO = 0;
    GLuint m_quadTreeVBO = 0;
    
    // Rendering options
    bool m_showTrails = true;
    bool m_showGrid = false;
    bool m_showForces = false;
    bool m_showQuadTree = false;
    bool m_showUI = true;
    
    // Performance tracking
    RenderStats m_stats;
    std::chrono::high_resolution_clock::time_point m_frameStart;
    
    // Instance data for efficient rendering
    struct BodyInstance {
        glm::vec2 position;
        float radius;
        glm::vec3 color;
        float selected; // 0.0 or 1.0
    };
    
    std::vector<BodyInstance> m_bodyInstances;
    std::vector<glm::vec2> m_trailVertices;
    std::vector<glm::vec2> m_gridVertices;
    std::vector<glm::vec2> m_forceVertices;
    std::vector<glm::vec2> m_quadTreeVertices;
    
    // Private methods
    bool InitializeShaders();
    bool InitializeBuffers();
    void CleanupGL();
    
    void UpdateBodyInstances(const std::vector<std::unique_ptr<Body>>& bodies,
                           const Body* selectedBody);
    void UpdateTrailVertices(const std::vector<std::unique_ptr<Body>>& bodies);
    void UpdateGridVertices();
    void UpdateForceVertices(const std::vector<std::unique_ptr<Body>>& bodies,
                           const PhysicsEngine& physics);
    void UpdateQuadTreeVertices(const QuadTreeNode* root);
    
    void RenderBodies();
    
    void StartTimer();
    void EndTimer();
    
    // Utility
    void CheckGLError(const std::string& operation) const;
    void GenerateCircleVertices(std::vector<glm::vec2>& vertices, int segments = 32);
    void TraverseQuadTree(const QuadTreeNode* node, std::vector<glm::vec2>& vertices);
    
    // Constants
    static constexpr int MAX_BODIES = 1000000;
    static constexpr int MAX_TRAIL_POINTS = 10000;
    static constexpr int CIRCLE_SEGMENTS = 32;
    static constexpr float GRID_SPACING = 1.0f;
    static constexpr float FORCE_SCALE = 0.1f;
};

} // namespace nbody
