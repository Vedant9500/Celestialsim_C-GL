#include "core/Application.h"
#include "core/Body.h"
#include "physics/PhysicsEngine.h"
#include "rendering/Renderer.h"
#include "ui/UIManager.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <random>

namespace nbody {

Application* Application::s_instance = nullptr;

Application::Application() {
    s_instance = this;
}

Application::~Application() {
    s_instance = nullptr;
}

bool Application::Initialize() {
    // Initialize GLFW
    glfwSetErrorCallback(ErrorCallback);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

    // Create window
    GLFWwindow* window = glfwCreateWindow(1200, 800, "N-Body Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }

    // Print OpenGL info
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;

    // Initialize components
    m_physics = std::make_unique<PhysicsEngine>();
    m_renderer = std::make_unique<Renderer>();
    m_ui = std::make_unique<UIManager>();

    if (!m_physics->Initialize()) {
        std::cerr << "Failed to initialize physics engine" << std::endl;
        return false;
    }

    if (!m_renderer->Initialize(window)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }

    if (!m_ui->Initialize(window)) {
        std::cerr << "Failed to initialize UI" << std::endl;
        return false;
    }

    // Set up callbacks
    glfwSetWindowSizeCallback(window, WindowSizeCallback);
    glfwSetCursorPosCallback(window, MouseMoveCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCharCallback(window, CharCallback);

    // Set up UI callbacks
    m_ui->OnPlayPause = [this]() { 
        if (m_running) {
            m_paused = !m_paused;
        } else {
            m_running = true;
            m_paused = false;
        }
    };
    
    m_ui->OnReset = [this]() { 
        m_running = false;
        m_paused = false;
        // Reset bodies to initial state if needed
    };
    
    m_ui->OnClear = [this]() { ClearBodies(); };
    
    m_ui->OnLoadPreset = [this](const std::string& preset) { LoadPreset(preset); };
    
    m_ui->OnDeleteBody = [this](Body* body) { RemoveBody(body); };
    
    m_ui->OnPhysicsParameterChanged = [this]() {
        // Update physics parameters from UI
        auto& config = m_physics->GetMutableConfig();
        config.gravitationalConstant = m_ui->GetGravitationalConstant();
        config.timeStep = m_ui->GetTimeStep();
        config.timeScale = m_ui->GetTimeScale();
        config.softeningLength = m_ui->GetSofteningLength();
        config.useBarnesHut = m_ui->GetUseBarnesHut();
        config.barnesHutTheta = m_ui->GetBarnesHutTheta();
        config.enableCollisions = m_ui->GetEnableCollisions();
        config.restitution = m_ui->GetRestitution();
    };
    
    // Initial sync of physics parameters from UI
    if (m_ui->OnPhysicsParameterChanged) {
        m_ui->OnPhysicsParameterChanged();
    }
    
    m_ui->OnRenderParameterChanged = [this]() {
        // Update renderer parameters from UI
        m_renderer->SetShowTrails(m_ui->IsShowingTrails());
        m_renderer->SetShowGrid(m_ui->IsShowingGrid());
        m_renderer->SetShowForces(m_ui->IsShowingForces());
        m_renderer->SetShowQuadTree(m_ui->IsShowingQuadTree());
    };
    
    m_ui->OnResetCamera = [this]() {
        m_renderer->SetCameraPosition(glm::vec2(0.0f));
        m_renderer->SetCameraZoom(1.0f);
    };
    
    m_ui->OnFitAllBodies = [this]() {
        m_renderer->FitAllBodies(m_bodies);
    };
    
    m_ui->OnSetCameraPosition = [this](const glm::vec2& position) {
        m_renderer->SetCameraPosition(position);
    };
    
    m_ui->OnSetCameraZoom = [this](float zoom) {
        m_renderer->SetCameraZoom(zoom);
    };
    
    m_ui->OnDeleteBody = [this](Body* body) { RemoveBody(body); };
    
    m_ui->OnDeleteBody = [this](Body* body) { RemoveBody(body); };
    
    // Camera callbacks (we'll need to add these to UIManager)
    // m_ui->OnResetCamera = [this]() { m_renderer->GetCamera().position = glm::vec2(0.0f); m_renderer->GetCamera().targetZoom = 1.0f; };
    // m_ui->OnFitAllBodies = [this]() { m_renderer->FitAllBodies(m_bodies); };
    // m_ui->OnCenterOnBody = [this](const Body* body) { m_renderer->CenterOnBody(body); };

    // Load default preset
    CreateSolarSystem();

    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    
    return true;
}

void Application::Run() {
    GLFWwindow* window = glfwGetCurrentContext();
    
    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_lastFrameTime);
        m_deltaTime = duration.count() / 1000000.0f; // Convert to seconds
        m_lastFrameTime = currentTime;

        // Cap delta time to prevent large jumps
        m_deltaTime = std::min(m_deltaTime, 0.033f); // Max 30 FPS

        glfwPollEvents();
        
        Update(m_deltaTime);
        Render();
        
        glfwSwapBuffers(window);
        
        UpdatePerformanceMetrics();
    }
}

void Application::Shutdown() {
    m_bodies.clear();
    m_ui.reset();
    m_renderer.reset();
    m_physics.reset();
    
    glfwTerminate();
}

void Application::Update(float deltaTime) {
    HandleInput();
    
    if (m_running && !m_paused) {
        UpdatePhysics(deltaTime);
    }
    
    // Update camera
    m_renderer->GetCamera().Update(deltaTime);
    
    UpdateUI();
}

void Application::Render() {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render simulation
    auto stats = m_renderer->Render(m_bodies, *m_physics, m_selectedBody);
    
    // Render UI
    m_ui->NewFrame();
    m_ui->Render(m_bodies, *m_physics, *m_renderer, m_selectedBody);
    m_ui->EndFrame();
}

void Application::HandleInput() {
    GLFWwindow* window = glfwGetCurrentContext();
    
    // Handle camera panning with middle mouse or Ctrl+left mouse
    static bool panning = false;
    static glm::vec2 lastPanPos;
    
    // Allow camera panning even over UI panels (middle mouse or Ctrl+left)
    bool shouldPan = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS ||
                      (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && 
                       (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
                        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS))) &&
                     !m_draggedBody; // Don't pan while dragging a body
    
    if (shouldPan) {
        if (!panning) {
            panning = true;
            lastPanPos = m_mousePosition;
        } else {
            glm::vec2 delta = m_mousePosition - lastPanPos;
            // Convert screen delta to world delta based on zoom
            float panScale = 1.0f / m_renderer->GetCamera().zoom;
            m_renderer->PanCamera(-delta * panScale * 0.002f); // Smoother panning
            lastPanPos = m_mousePosition;
        }
    } else {
        panning = false;
    }
}

void Application::UpdatePhysics(float deltaTime) {
    m_physics->Update(m_bodies, deltaTime);
}

void Application::UpdateUI() {
    // Update world mouse position
    m_worldMousePosition = m_renderer->ScreenToWorld(m_mousePosition);
    
    // Sync camera state to UI
    const auto& camera = m_renderer->GetCamera();
    m_ui->SetCameraPosition(camera.position);
    m_ui->SetCameraZoom(camera.zoom);
    
    // Handle body dragging
    if (m_draggedBody && glfwGetMouseButton(glfwGetCurrentContext(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        m_draggedBody->SetPosition(m_worldMousePosition);
        m_draggedBody->SetVelocity(glm::vec2(0.0f)); // Stop the body when dragging
        m_draggedBody->SetBeingDragged(true);
    }
    
    // Apply UI settings to renderer (only when needed)
    static bool lastTrails = m_ui->IsShowingTrails();
    static bool lastGrid = m_ui->IsShowingGrid();
    static bool lastForces = m_ui->IsShowingForces();
    static bool lastQuadTree = m_ui->IsShowingQuadTree();
    
    if (lastTrails != m_ui->IsShowingTrails()) {
        m_renderer->SetShowTrails(m_ui->IsShowingTrails());
        lastTrails = m_ui->IsShowingTrails();
    }
    if (lastGrid != m_ui->IsShowingGrid()) {
        m_renderer->SetShowGrid(m_ui->IsShowingGrid());
        lastGrid = m_ui->IsShowingGrid();
    }
    if (lastForces != m_ui->IsShowingForces()) {
        m_renderer->SetShowForces(m_ui->IsShowingForces());
        lastForces = m_ui->IsShowingForces();
    }
    if (lastQuadTree != m_ui->IsShowingQuadTree()) {
        m_renderer->SetShowQuadTree(m_ui->IsShowingQuadTree());
        lastQuadTree = m_ui->IsShowingQuadTree();
    }
}

void Application::OnMouseMove(double x, double y) {
    m_mousePosition = glm::vec2(static_cast<float>(x), static_cast<float>(y));
}

void Application::OnMouseButton(int button, int action, int mods) {
    if (m_ui->IsMouseOverUI()) {
        return; // Don't handle simulation input when over UI
    }
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        Body* clickedBody = FindBodyAtPosition(m_worldMousePosition);
        
        if (clickedBody) {
            // Select and start dragging body
            m_selectedBody = clickedBody;
            m_draggedBody = clickedBody;
            clickedBody->SetSelected(true);
        } else {
            // Add new body
            if (m_ui->IsOrbitMode() && !m_bodies.empty()) {
                // Find nearest body for orbital velocity calculation
                Body* nearest = nullptr;
                float minDist = std::numeric_limits<float>::max();
                
                for (auto& body : m_bodies) {
                    float dist = glm::length(body->GetPosition() - m_worldMousePosition);
                    if (dist < minDist) {
                        minDist = dist;
                        nearest = body.get();
                    }
                }
                
                if (nearest) {
                    // Calculate orbital velocity
                    glm::vec2 r = m_worldMousePosition - nearest->GetPosition();
                    float distance = glm::length(r);
                    float speed = std::sqrt(m_physics->GetConfig().gravitationalConstant * nearest->GetMass() / distance);
                    glm::vec2 velocity = glm::vec2(-r.y, r.x) * speed / distance;
                    
                    AddBody(m_worldMousePosition, velocity, m_ui->GetNewBodyMass());
                }
            } else {
                AddBody(m_worldMousePosition, m_ui->GetNewBodyVelocity(), m_ui->GetNewBodyMass());
            }
            
            // Deselect current body
            if (m_selectedBody) {
                m_selectedBody->SetSelected(false);
                m_selectedBody = nullptr;
            }
        }
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        if (m_draggedBody) {
            m_draggedBody->SetBeingDragged(false);
            m_draggedBody = nullptr;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        Body* clickedBody = FindBodyAtPosition(m_worldMousePosition);
        if (clickedBody) {
            // Show context menu or delete body
            RemoveBody(clickedBody);
        }
    }
}

void Application::OnMouseScroll(double xOffset, double yOffset) {
    if (m_ui->IsMouseOverUI()) {
        return;
    }
    
    float zoomFactor = 1.1f;
    if (yOffset > 0) {
        m_renderer->ZoomIn(zoomFactor);
    } else {
        m_renderer->ZoomOut(zoomFactor);
    }
}

void Application::OnKeyboard(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_SPACE:
                if (m_running) {
                    m_paused = !m_paused;
                } else {
                    m_running = true;
                    m_paused = false;
                }
                break;
            case GLFW_KEY_R:
                m_running = false;
                m_paused = false;
                break;
            case GLFW_KEY_C:
                ClearBodies();
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
                break;
        }
    }
}

void Application::OnWindowResize(int width, int height) {
    m_renderer->OnWindowResize(width, height);
    m_ui->OnWindowResize(width, height);
}

void Application::AddBody(const glm::vec2& position, const glm::vec2& velocity, float mass) {
    auto body = std::make_unique<Body>(position, velocity, mass, m_ui->GetNewBodyColor());
    m_bodies.push_back(std::move(body));
}

void Application::RemoveBody(Body* body) {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [body](const std::unique_ptr<Body>& b) { return b.get() == body; });
    
    if (it != m_bodies.end()) {
        if (m_selectedBody == body) {
            m_selectedBody = nullptr;
        }
        if (m_draggedBody == body) {
            m_draggedBody = nullptr;
        }
        m_bodies.erase(it);
    }
}

void Application::ClearBodies() {
    m_bodies.clear();
    m_selectedBody = nullptr;
    m_draggedBody = nullptr;
}

Body* Application::FindBodyAtPosition(const glm::vec2& position) {
    for (auto& body : m_bodies) {
        float distance = glm::length(body->GetPosition() - position);
        if (distance <= body->GetRadius() * 2.0f) { // Allow some tolerance
            return body.get();
        }
    }
    return nullptr;
}

glm::vec2 Application::ScreenToWorld(const glm::vec2& screenPos) const {
    return m_renderer->ScreenToWorld(screenPos);
}

glm::vec2 Application::WorldToScreen(const glm::vec2& worldPos) const {
    return m_renderer->WorldToScreen(worldPos);
}

void Application::LoadPreset(const std::string& name) {
    ClearBodies();
    
    if (name == "Solar System") {
        CreateSolarSystem();
    } else if (name == "Binary System") {
        CreateBinarySystem();
    } else if (name == "Galaxy") {
        CreateGalaxySpiral();
    } else if (name == "Random Cluster") {
        CreateRandomCluster(50);
    }
}

void Application::CreateSolarSystem() {
    // Sun
    AddBody(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 1000.0f);
    
    // Planets (simplified)
    AddBody(glm::vec2(2.0f, 0.0f), glm::vec2(0.0f, 15.0f), 10.0f);  // Mercury-like
    AddBody(glm::vec2(3.0f, 0.0f), glm::vec2(0.0f, 12.0f), 15.0f);  // Venus-like
    AddBody(glm::vec2(4.0f, 0.0f), glm::vec2(0.0f, 10.0f), 20.0f);  // Earth-like
    AddBody(glm::vec2(6.0f, 0.0f), glm::vec2(0.0f, 8.0f), 12.0f);   // Mars-like
}

void Application::CreateBinarySystem() {
    AddBody(glm::vec2(-2.0f, 0.0f), glm::vec2(0.0f, 5.0f), 500.0f);
    AddBody(glm::vec2(2.0f, 0.0f), glm::vec2(0.0f, -5.0f), 500.0f);
}

void Application::CreateGalaxySpiral() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDist(1.0f, 10.0f);
    std::uniform_real_distribution<float> massDist(1.0f, 10.0f);
    
    // Central black hole
    AddBody(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 2000.0f);
    
    // Spiral arms
    for (int i = 0; i < 100; ++i) {
        float angle = angleDist(gen);
        float radius = radiusDist(gen);
        float mass = massDist(gen);
        
        glm::vec2 position(radius * std::cos(angle), radius * std::sin(angle));
        
        // Calculate orbital velocity
        float speed = std::sqrt(6.674e-11f * 2000.0f / radius);
        glm::vec2 velocity(-speed * std::sin(angle), speed * std::cos(angle));
        
        AddBody(position, velocity, mass);
    }
}

void Application::CreateRandomCluster(int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-5.0f, 5.0f);
    std::uniform_real_distribution<float> velDist(-2.0f, 2.0f);
    std::uniform_real_distribution<float> massDist(1.0f, 20.0f);
    
    for (int i = 0; i < count; ++i) {
        glm::vec2 position(posDist(gen), posDist(gen));
        glm::vec2 velocity(velDist(gen), velDist(gen));
        float mass = massDist(gen);
        
        AddBody(position, velocity, mass);
    }
}

void Application::UpdatePerformanceMetrics() {
    static int frameCount = 0;
    static auto lastTime = std::chrono::high_resolution_clock::now();
    
    frameCount++;
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);
    
    if (duration.count() >= 1000) {
        m_fps = frameCount / (duration.count() / 1000.0f);
        frameCount = 0;
        lastTime = currentTime;
    }
}

// Static callback implementations
void Application::MouseMoveCallback(GLFWwindow* window, double x, double y) {
    // Forward to ImGui first
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);
    
    if (s_instance) {
        s_instance->OnMouseMove(x, y);
    }
}

void Application::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Forward to ImGui first
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    
    if (s_instance) {
        s_instance->OnMouseButton(button, action, mods);
    }
}

void Application::ScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    // Forward to ImGui first
    ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
    
    if (s_instance) {
        s_instance->OnMouseScroll(xOffset, yOffset);
    }
}

void Application::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Forward to ImGui first
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    
    if (s_instance) {
        s_instance->OnKeyboard(key, scancode, action, mods);
    }
}

void Application::CharCallback(GLFWwindow* window, unsigned int codepoint) {
    // Forward to ImGui first
    ImGui_ImplGlfw_CharCallback(window, codepoint);
}

void Application::WindowSizeCallback(GLFWwindow* window, int width, int height) {
    if (s_instance) {
        s_instance->OnWindowResize(width, height);
    }
}

void Application::ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

} // namespace nbody
