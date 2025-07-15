#include "rendering/Renderer.h"
#include "core/Body.h"
#include "physics/PhysicsEngine.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>

namespace nbody {

// Simple circle vertex data
static const std::vector<glm::vec2> circleVertices = {
    {-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f},
    {-1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}
};

Shader::~Shader() {
    if (m_program) {
        glDeleteProgram(m_program);
    }
}

bool Shader::LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource = ReadFile(vertexPath);
    std::string fragmentSource = ReadFile(fragmentPath);
    
    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "Failed to read shader files" << std::endl;
        return false;
    }
    
    return LoadFromString(vertexSource, fragmentSource);
}

bool Shader::LoadFromString(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vertexShader = CompileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = CompileShader(fragmentSource, GL_FRAGMENT_SHADER);
    
    if (!vertexShader || !fragmentShader) {
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return false;
    }
    
    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    glLinkProgram(m_program);
    
    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
        
        glDeleteProgram(m_program);
        m_program = 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return success;
}

void Shader::Use() const {
    glUseProgram(m_program);
}

void Shader::Unuse() const {
    glUseProgram(0);
}

void Shader::SetInt(const std::string& name, int value) const {
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetFloat(const std::string& name, float value) const {
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

GLuint Shader::CompileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed (" << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                  << "): " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLint Shader::GetUniformLocation(const std::string& name) const {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second;
    }
    
    GLint location = glGetUniformLocation(m_program, name.c_str());
    m_uniformCache[name] = location;
    return location;
}

std::string Shader::ReadFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

Renderer::Renderer() {
    m_fpsHistory.resize(FPS_HISTORY_SIZE, 0.0f);
    m_lastFrameTime = std::chrono::high_resolution_clock::now();
}

Renderer::~Renderer() {
    CleanupGL();
}

bool Renderer::Initialize(GLFWwindow* window) {
    m_window = window;
    
    // Get window size
    glfwGetWindowSize(window, &m_windowWidth, &m_windowHeight);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set clear color
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    
    if (!InitializeShaders()) {
        return false;
    }
    
    if (!InitializeBuffers()) {
        return false;
    }
    
    std::cout << "Renderer initialized successfully" << std::endl;
    return true;
}

bool Renderer::InitializeShaders() {
    // Body shader
    m_bodyShader = std::make_unique<Shader>();
    if (!m_bodyShader->LoadFromFile("shaders/body.vert", "shaders/body.frag")) {
        // Fallback to embedded shaders
        std::string vertexShader = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aInstancePos;
            layout (location = 2) in float aInstanceRadius;
            layout (location = 3) in vec3 aInstanceColor;
            layout (location = 4) in float aInstanceSelected;
            uniform mat4 uProjection;
            uniform mat4 uView;
            out vec3 fragColor;
            out float fragSelected;
            out vec2 localPos;
            void main() {
                vec2 scaledPos = aPos * aInstanceRadius;
                vec2 worldPos = aInstancePos + scaledPos;
                gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
                fragColor = aInstanceColor;
                fragSelected = aInstanceSelected;
                localPos = aPos;
            }
        )";
        
        std::string fragmentShader = R"(
            #version 330 core
            in vec3 fragColor;
            in float fragSelected;
            in vec2 localPos;
            out vec4 FragColor;
            void main() {
                float dist = length(localPos);
                if (dist > 1.0) discard;
                float alpha = 1.0 - smoothstep(0.9, 1.0, dist);
                vec3 color = fragColor;
                if (fragSelected > 0.5) {
                    color = mix(color, vec3(1.0, 1.0, 0.0), 0.3);
                }
                FragColor = vec4(color, alpha);
            }
        )";
        
        if (!m_bodyShader->LoadFromString(vertexShader, fragmentShader)) {
            std::cerr << "Failed to load body shader" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool Renderer::InitializeBuffers() {
    // Body VAO and VBO
    glGenVertexArrays(1, &m_bodyVAO);
    glGenBuffers(1, &m_bodyVBO);
    glGenBuffers(1, &m_bodyInstanceVBO);
    
    glBindVertexArray(m_bodyVAO);
    
    // Vertex data (circle vertices)
    glBindBuffer(GL_ARRAY_BUFFER, m_bodyVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(glm::vec2), 
                 circleVertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Instance data
    glBindBuffer(GL_ARRAY_BUFFER, m_bodyInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_BODIES * sizeof(BodyInstance), nullptr, GL_DYNAMIC_DRAW);
    
    // Position
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BodyInstance), (void*)offsetof(BodyInstance, position));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);
    
    // Radius
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(BodyInstance), (void*)offsetof(BodyInstance, radius));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    
    // Color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(BodyInstance), (void*)offsetof(BodyInstance, color));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    
    // Selected
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(BodyInstance), (void*)offsetof(BodyInstance, selected));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    
    glBindVertexArray(0);
    
    CheckGLError("Buffer initialization");
    return true;
}

RenderStats Renderer::Render(const std::vector<std::unique_ptr<Body>>& bodies,
                             const PhysicsEngine& /*physics*/,
                             const Body* selectedBody) {
    StartTimer();
    
    // Update camera
    m_camera.Update(1.0f / 60.0f); // Assume 60 FPS for smooth camera
    
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Set up matrices
    glm::mat4 projection = m_camera.GetProjectionMatrix(static_cast<float>(m_windowWidth), static_cast<float>(m_windowHeight));
    glm::mat4 view = m_camera.GetViewMatrix();
    
    // Render bodies
    UpdateBodyInstances(bodies, selectedBody);
    RenderBodies();
    
    // TODO: Implement visualization features properly with modern OpenGL
    // For now, let's keep it simple to not break existing functionality
    
    // Update statistics
    EndTimer();
    
    m_stats.bodiesRendered = static_cast<int>(bodies.size());
    m_stats.drawCalls = 1;
    
    return m_stats;
}

void Renderer::UpdateBodyInstances(const std::vector<std::unique_ptr<Body>>& bodies,
                                  const Body* selectedBody) {
    m_bodyInstances.clear();
    m_bodyInstances.reserve(bodies.size());
    
    for (const auto& body : bodies) {
        BodyInstance instance;
        instance.position = body->GetPosition();
        instance.radius = body->GetRadius();
        instance.color = body->GetColor();
        instance.selected = (body.get() == selectedBody) ? 1.0f : 0.0f;
        
        m_bodyInstances.push_back(instance);
    }
}

void Renderer::RenderBodies() {
    if (m_bodyInstances.empty()) return;
    
    m_bodyShader->Use();
    
    // Set matrices
    glm::mat4 projection = m_camera.GetProjectionMatrix(static_cast<float>(m_windowWidth), static_cast<float>(m_windowHeight));
    glm::mat4 view = m_camera.GetViewMatrix();
    
    m_bodyShader->SetMat4("uProjection", projection);
    m_bodyShader->SetMat4("uView", view);
    m_bodyShader->SetFloat("uZoom", m_camera.zoom);
    
    // Update instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_bodyInstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 
                    m_bodyInstances.size() * sizeof(BodyInstance), 
                    m_bodyInstances.data());
    
    // Render
    glBindVertexArray(m_bodyVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(m_bodyInstances.size()));
    glBindVertexArray(0);
    
    m_bodyShader->Unuse();
}

void Renderer::OnWindowResize(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
    glViewport(0, 0, width, height);
}

glm::vec2 Renderer::ScreenToWorld(const glm::vec2& screenPos) const {
    // Convert screen coordinates to normalized device coordinates
    float x = (2.0f * screenPos.x) / m_windowWidth - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y) / m_windowHeight;
    
    // Apply inverse camera transformation
    float aspect = static_cast<float>(m_windowWidth) / m_windowHeight;
    x = (x * aspect) / m_camera.zoom + m_camera.position.x;
    y = y / m_camera.zoom + m_camera.position.y;
    
    return glm::vec2(x, y);
}

glm::vec2 Renderer::WorldToScreen(const glm::vec2& worldPos) const {
    // Apply camera transformation
    float aspect = static_cast<float>(m_windowWidth) / m_windowHeight;
    float x = ((worldPos.x - m_camera.position.x) * m_camera.zoom) / aspect;
    float y = (worldPos.y - m_camera.position.y) * m_camera.zoom;
    
    // Convert to screen coordinates
    x = (x + 1.0f) * m_windowWidth * 0.5f;
    y = (1.0f - y) * m_windowHeight * 0.5f;
    
    return glm::vec2(x, y);
}

void Renderer::FitAllBodies(const std::vector<std::unique_ptr<Body>>& bodies) {
    if (bodies.empty()) return;
    
    // Calculate bounding box
    glm::vec2 minPos = bodies[0]->GetPosition();
    glm::vec2 maxPos = bodies[0]->GetPosition();
    
    for (const auto& body : bodies) {
        glm::vec2 pos = body->GetPosition();
        minPos.x = std::min(minPos.x, pos.x - body->GetRadius());
        minPos.y = std::min(minPos.y, pos.y - body->GetRadius());
        maxPos.x = std::max(maxPos.x, pos.x + body->GetRadius());
        maxPos.y = std::max(maxPos.y, pos.y + body->GetRadius());
    }
    
    // Set camera to show all bodies
    m_camera.position = (minPos + maxPos) * 0.5f;
    
    glm::vec2 extent = maxPos - minPos;
    float maxExtent = std::max(extent.x, extent.y);
    float aspect = static_cast<float>(m_windowWidth) / m_windowHeight;
    
    m_camera.targetZoom = std::min(2.0f / maxExtent, 2.0f / (maxExtent / aspect)) * 0.8f;
}

void Renderer::CenterOnBody(const Body* body) {
    if (body) {
        m_camera.position = body->GetPosition();
    }
}

void Renderer::StartTimer() {
    m_frameStart = std::chrono::high_resolution_clock::now();
}

void Renderer::EndTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    m_stats.renderTime = std::chrono::duration<double, std::milli>(end - m_frameStart).count();
    
    // Calculate frame time in seconds
    m_stats.frameTime = std::chrono::duration<double>(end - m_lastFrameTime).count();
    
    // Calculate instantaneous FPS
    float instantFPS = 0.0f;
    if (m_stats.frameTime > 0.0) {
        instantFPS = static_cast<float>(1.0 / m_stats.frameTime);
    }
    
    // Store in history for smoothing
    m_fpsHistory[m_fpsHistoryIndex] = instantFPS;
    m_fpsHistoryIndex = (m_fpsHistoryIndex + 1) % FPS_HISTORY_SIZE;
    if (m_fpsHistoryIndex == 0) {
        m_fpsHistoryFull = true;
    }
    
    // Calculate smoothed FPS (average of recent frames)
    int sampleCount = m_fpsHistoryFull ? FPS_HISTORY_SIZE : m_fpsHistoryIndex;
    if (sampleCount > 0) {
        float sum = 0.0f;
        for (int i = 0; i < sampleCount; ++i) {
            sum += m_fpsHistory[i];
        }
        m_stats.fps = sum / sampleCount;
    } else {
        m_stats.fps = 0.0f;
    }
    
    // Update last frame time for next calculation
    m_lastFrameTime = end;
}

void Renderer::CheckGLError(const std::string& operation) const {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after " << operation << ": " << error << std::endl;
    }
}

void Renderer::CleanupGL() {
    if (m_bodyVAO) glDeleteVertexArrays(1, &m_bodyVAO);
    if (m_bodyVBO) glDeleteBuffers(1, &m_bodyVBO);
    if (m_bodyInstanceVBO) glDeleteBuffers(1, &m_bodyInstanceVBO);
}

} // namespace nbody
