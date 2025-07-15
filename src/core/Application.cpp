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
#include <cmath>

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
    
    // Set GPU availability in UI
    m_ui->SetGPUAvailable(m_physics->IsGPUAvailable());

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
    
    // Initial sync of render parameters from UI
    if (m_ui->OnRenderParameterChanged) {
        m_ui->OnRenderParameterChanged();
    }
    
    m_ui->OnResetCamera = [this]() {
        m_renderer->SetCameraPosition(glm::vec2(0.0f));
        m_renderer->SetCameraZoom(0.001f); // Set to minimum zoom to see everything
    };
    
    m_ui->OnFitAllBodies = [this]() {
        m_renderer->FitAllBodies(m_bodies);
    };
    
    m_ui->OnSpawnBodies = [this](int count, int pattern) {
        SpawnBodies(count, pattern);
    };
    
    m_ui->OnSetCameraPosition = [this](const glm::vec2& position) {
        m_renderer->SetCameraPosition(position);
    };
    
    m_ui->OnSetCameraZoom = [this](float zoom) {
        m_renderer->SetCameraZoom(zoom);
    };
    
    m_ui->OnDeleteBody = [this](Body* body) { RemoveBody(body); };
    
    m_ui->OnDeleteBody = [this](Body* body) { RemoveBody(body); };
    
    // Performance benchmarking
    m_ui->OnRunBenchmark = [this]() {
        if (m_physics) {
            m_physics->BenchmarkMethods(m_bodies);
        }
    };
    
    // Camera callbacks (we'll need to add these to UIManager)
    // m_ui->OnResetCamera = [this]() { m_renderer->GetCamera().position = glm::vec2(0.0f); m_renderer->GetCamera().targetZoom = 0.001f; };
    // m_ui->OnFitAllBodies = [this]() { m_renderer->FitAllBodies(m_bodies); };
    // m_ui->OnCenterOnBody = [this](const Body* body) { m_renderer->CenterOnBody(body); };

    // Load default preset
    CreateRandomCluster(100);

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
            // Invert the Y component to make camera move naturally with drag direction
            // When user drags up, the camera should move up (negative y in screen space is up)
            glm::vec2 correctedDelta = glm::vec2(delta.x, -delta.y) * 0.002f;
            m_renderer->PanCamera(correctedDelta); // Smoother panning
            lastPanPos = m_mousePosition;
        }
    } else {
        panning = false;
    }
    
    // Handle Delete key to delete selected body
    static bool deleteKeyPressed = false;
    bool deleteKeyDown = glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS;
    
    if (deleteKeyDown && !deleteKeyPressed && m_selectedBody) {
        // Delete key was just pressed and we have a selected body
        RemoveBody(m_selectedBody);
        m_selectedBody = nullptr;
        m_draggedBody = nullptr;
    }
    deleteKeyPressed = deleteKeyDown;
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

void Application::OnMouseButton(int button, int action, int /*mods*/) {
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

void Application::OnMouseScroll(double /*xOffset*/, double yOffset) {
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

void Application::OnKeyboard(int key, int /*scancode*/, int action, int /*mods*/) {
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
            case GLFW_KEY_DELETE:
                if (m_selectedBody != nullptr) {
                    RemoveBody(m_selectedBody);
                }
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

void Application::AddBody(const glm::vec2& position, const glm::vec2& velocity, float mass, 
                         float density, const glm::vec3& color) {
    auto body = std::make_unique<Body>(position, velocity, mass, color);
    body->SetDensity(density);
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
        if (distance <= body->GetRadius() * 2.0f) // Allow some tolerance
            return body.get();
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
    } else if (name == "Triple Star") {
        CreateTripleStarSystem();
    } else if (name == "Figure Eight") {
        CreateFigureEight();
    } else if (name == "Collision Course") {
        CreateCollisionCourse();
    }
}

void Application::CreateSolarSystem() {
    // PHYSICALLY ACCURATE SOLAR SYSTEM
    // Proper scaling maintaining all physics relationships
    
    float G = m_physics->GetConfig().gravitationalConstant;
    
    // === REAL SOLAR SYSTEM DATA ===
    // Real masses relative to Earth (Earth = 1.0)
    const float earthMass = 1.0f;
    const float sunMass = 332946.0f * earthMass;    // Real mass ratio
    const float mercuryMass = 0.0553f * earthMass;
    const float venusMass = 0.815f * earthMass;
    const float marsMass = 0.107f * earthMass;
    const float jupiterMass = 318.0f * earthMass;
    const float saturnMass = 95.2f * earthMass;
    const float uranusMass = 14.5f * earthMass;
    const float neptuneMass = 17.1f * earthMass;
    
    // Real radii relative to Earth (Earth radius = 1.0)
    const float earthRadius = 1.0f;
    const float sunRadius = 109.3f * earthRadius;    // Real size ratio
    const float mercuryRadius = 0.383f * earthRadius;
    const float venusRadius = 0.949f * earthRadius;
    const float marsRadius = 0.532f * earthRadius;
    const float jupiterRadius = 11.21f * earthRadius;
    const float saturnRadius = 9.45f * earthRadius;
    const float uranusRadius = 4.01f * earthRadius;
    const float neptuneRadius = 3.88f * earthRadius;
    
    // === SCALING FOR SIMULATION ===
    // Scale factor: Make Earth orbit ~200 units for good visibility
    const float AU = 200.0f; // 1 AU = 200 simulation units
    
    // Orbital distances
    const float mercuryDist = 0.39f * AU;   // ~78 units
    const float venusDist = 0.72f * AU;     // ~144 units  
    const float earthDist = 1.0f * AU;      // 200 units
    const float marsDist = 1.52f * AU;      // ~304 units
    const float jupiterDist = 5.20f * AU;   // ~1040 units
    const float saturnDist = 9.54f * AU;    // ~1908 units
    const float uranusDist = 19.19f * AU;   // ~3838 units
    const float neptuneDist = 30.07f * AU;  // ~6014 units
    
    // Scale masses down for numerical stability but keep ratios
    const float massScale = 0.01f; // Scale all masses down
    
    // Visual size scaling: Make bodies visible but maintain relative sizes
    // Use density to control visual size (radius = sqrt(mass/(π*density)))
    // Higher density = smaller visual size for same mass
    const float baseDensity = 50.0f; // High base density for small sizes
    
    // Calculate densities to achieve realistic relative sizes
    // We want: visual_radius ∝ real_radius, so density ∝ mass/real_radius²
    const float sunDensity = baseDensity * (sunMass * massScale) / (sunRadius * sunRadius) * 0.01f; // Extra small Sun
    const float mercuryDensity = baseDensity * (mercuryMass * massScale) / (mercuryRadius * mercuryRadius);
    const float venusDensity = baseDensity * (venusMass * massScale) / (venusRadius * venusRadius);
    const float earthDensity = baseDensity * (earthMass * massScale) / (earthRadius * earthRadius);
    const float marsDensity = baseDensity * (marsMass * massScale) / (marsRadius * marsRadius);
    const float jupiterDensity = baseDensity * (jupiterMass * massScale) / (jupiterRadius * jupiterRadius);
    const float saturnDensity = baseDensity * (saturnMass * massScale) / (saturnRadius * saturnRadius);
    const float uranusDensity = baseDensity * (uranusMass * massScale) / (uranusRadius * uranusRadius);
    const float neptuneDensity = baseDensity * (neptuneMass * massScale) / (neptuneRadius * neptuneRadius);
    
    // Colors
    glm::vec3 sunColor(1.0f, 1.0f, 0.8f);      // Yellow
    glm::vec3 mercuryColor(0.8f, 0.7f, 0.7f);  // Gray
    glm::vec3 venusColor(1.0f, 0.8f, 0.0f);    // Yellow-orange
    glm::vec3 earthColor(0.2f, 0.5f, 1.0f);    // Blue
    glm::vec3 marsColor(1.0f, 0.4f, 0.2f);     // Red
    glm::vec3 jupiterColor(1.0f, 0.7f, 0.3f);  // Orange-brown
    glm::vec3 saturnColor(1.0f, 1.0f, 0.8f);   // Pale yellow
    glm::vec3 uranusColor(0.3f, 0.8f, 1.0f);   // Light blue
    glm::vec3 neptuneColor(0.2f, 0.3f, 1.0f);  // Deep blue
    
    // Scale Sun mass for physics (reduce for stability but maintain gravitational dominance)
    float sunMassForPhysics = sunMass * massScale;
    
    // Create the Sun
    AddBody(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 
            sunMassForPhysics, sunDensity, sunColor);
    
    // Create planets with proper orbital velocities: v = sqrt(GM/r)
    float mercuryVel = std::sqrt(G * sunMassForPhysics / mercuryDist);
    AddBody(glm::vec2(mercuryDist, 0.0f), glm::vec2(0.0f, mercuryVel),
            mercuryMass * massScale, mercuryDensity, mercuryColor);
    
    float venusVel = std::sqrt(G * sunMassForPhysics / venusDist);
    AddBody(glm::vec2(venusDist, 0.0f), glm::vec2(0.0f, venusVel),
            venusMass * massScale, venusDensity, venusColor);
    
    float earthVel = std::sqrt(G * sunMassForPhysics / earthDist);
    AddBody(glm::vec2(earthDist, 0.0f), glm::vec2(0.0f, earthVel),
            earthMass * massScale, earthDensity, earthColor);
    
    float marsVel = std::sqrt(G * sunMassForPhysics / marsDist);
    AddBody(glm::vec2(marsDist, 0.0f), glm::vec2(0.0f, marsVel),
            marsMass * massScale, marsDensity, marsColor);
    
    float jupiterVel = std::sqrt(G * sunMassForPhysics / jupiterDist);
    AddBody(glm::vec2(jupiterDist, 0.0f), glm::vec2(0.0f, jupiterVel),
            jupiterMass * massScale, jupiterDensity, jupiterColor);
    
    float saturnVel = std::sqrt(G * sunMassForPhysics / saturnDist);
    AddBody(glm::vec2(saturnDist, 0.0f), glm::vec2(0.0f, saturnVel),
            saturnMass * massScale, saturnDensity, saturnColor);
    
    float uranusVel = std::sqrt(G * sunMassForPhysics / uranusDist);
    AddBody(glm::vec2(uranusDist, 0.0f), glm::vec2(0.0f, uranusVel),
            uranusMass * massScale, uranusDensity, uranusColor);
    
    float neptuneVel = std::sqrt(G * sunMassForPhysics / neptuneDist);
    AddBody(glm::vec2(neptuneDist, 0.0f), glm::vec2(0.0f, neptuneVel),
            neptuneMass * massScale, neptuneDensity, neptuneColor);
}

void Application::CreateBinarySystem() {
    // Create a proper binary system with center of mass at origin
    float mass1 = 15.0f;
    float mass2 = 20.0f;
    float separation = 60.0f;  // Much larger separation
    
    // Calculate center of mass positions
    float totalMass = mass1 + mass2;
    float r1 = separation * mass2 / totalMass;  // Distance of mass1 from COM
    float r2 = separation * mass1 / totalMass;  // Distance of mass2 from COM
    
    // Calculate orbital velocity for circular orbit
    float G = m_physics->GetConfig().gravitationalConstant;
    float v1 = std::sqrt(G * mass2 * mass2 / (totalMass * separation));
    float v2 = std::sqrt(G * mass1 * mass1 / (totalMass * separation));
    
    // Position bodies on opposite sides of center of mass
    AddBody(glm::vec2(-r1, 0.0f), glm::vec2(0.0f, v1), mass1);
    AddBody(glm::vec2(r2, 0.0f), glm::vec2(0.0f, -v2), mass2);
    
    // Add some smaller bodies in interesting orbits around the binary system
    AddBody(glm::vec2(0.0f, 80.0f), glm::vec2(6.0f, 0.0f), 2.0f);    // High orbit
    AddBody(glm::vec2(0.0f, -90.0f), glm::vec2(-5.5f, 0.0f), 3.0f); // Opposite orbit
    AddBody(glm::vec2(100.0f, 0.0f), glm::vec2(0.0f, 4.0f), 2.5f);  // Distant orbit
}

void Application::CreateGalaxySpiral() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDist(20.0f, 150.0f);  // Much larger range
    std::uniform_real_distribution<float> massDist(0.5f, 2.0f);       // Smaller masses
    std::uniform_real_distribution<float> armNoise(-0.3f, 0.3f);      // Angular noise
    
    // Central supermassive object
    AddBody(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 50.0f);  // Smaller central mass
    
    float G = m_physics->GetConfig().gravitationalConstant;
    float centralMass = 50.0f;
    
    // Create spiral arms
    int armsCount = 2;
    int particlesPerArm = 40;
    
    for (int arm = 0; arm < armsCount; ++arm) {
        float armOffset = (2.0f * 3.14159f * arm) / armsCount;
        
        for (int i = 0; i < particlesPerArm; ++i) {
            float t = static_cast<float>(i) / particlesPerArm;
            float radius = 20.0f + t * 130.0f;  // Spiral outward from 20 to 150
            float spiralTightness = 1.5f;
            float angle = armOffset + spiralTightness * t * 4.0f * 3.14159f + armNoise(gen);
            
            glm::vec2 position(radius * std::cos(angle), radius * std::sin(angle));
            
            // Calculate stable orbital velocity with some randomness
            float baseSpeed = std::sqrt(G * centralMass / radius);
            float speedVariation = 0.8f + 0.4f * static_cast<float>(rand()) / RAND_MAX;
            float speed = baseSpeed * speedVariation;
            
            glm::vec2 velocity(-speed * std::sin(angle), speed * std::cos(angle));
            
            // Add some random velocity component
            velocity += glm::vec2(armNoise(gen) * 1.0f, armNoise(gen) * 1.0f);
            
            float mass = massDist(gen);
            AddBody(position, velocity, mass);
        }
    }
    
    // Add some random halo particles
    std::uniform_real_distribution<float> haloDist(150.0f, 200.0f);
    for (int i = 0; i < 15; ++i) {
        float angle = angleDist(gen);
        float radius = haloDist(gen);
        glm::vec2 position(radius * std::cos(angle), radius * std::sin(angle));
        
        float speed = std::sqrt(G * centralMass / radius) * 0.7f; // Slower halo
        glm::vec2 velocity(-speed * std::sin(angle), speed * std::cos(angle));
        
        AddBody(position, velocity, massDist(gen) * 0.5f);
    }
}

void Application::CreateRandomCluster(int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDist(10.0f, 100.0f);  // Much larger area
    std::uniform_real_distribution<float> velDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> massDist(1.0f, 8.0f);       // Smaller masses
    
    std::vector<glm::vec2> positions;
    positions.reserve(count);
    
    // Generate non-overlapping positions
    for (int i = 0; i < count; ++i) {
        glm::vec2 position;
        bool validPosition = false;
        int attempts = 0;
        
        while (!validPosition && attempts < 100) {
            // Try random position in circular area
            float angle = angleDist(gen);
            float radius = radiusDist(gen);
            position = glm::vec2(radius * std::cos(angle), radius * std::sin(angle));
            
            // Check minimum distance from existing bodies
            validPosition = true;
            float minDistance = 8.0f; // Larger minimum separation
            
            for (const auto& existingPos : positions) {
                if (glm::length(position - existingPos) < minDistance) {
                    validPosition = false;
                    break;
                }
            }
            attempts++;
        }
        
        if (validPosition) {
            positions.push_back(position);
            
            // Add some random velocity
            glm::vec2 velocity(velDist(gen), velDist(gen));
            float mass = massDist(gen);
            
            AddBody(position, velocity, mass);
        }
    }
}

void Application::CreateTripleStarSystem() {
    // Create a stable triple star system with hierarchical structure
    float G = m_physics->GetConfig().gravitationalConstant;
    
    // Inner binary pair - much smaller masses, larger separation
    float mass1 = 8.0f;
    float mass2 = 6.0f;
    float innerSeparation = 40.0f;
    
    // Calculate center of mass for inner binary
    float totalInner = mass1 + mass2;
    float r1 = innerSeparation * mass2 / totalInner;
    float r2 = innerSeparation * mass1 / totalInner;
    
    // Orbital velocity for inner binary
    float v_inner = std::sqrt(G * totalInner / innerSeparation);
    
    // Position inner binary stars
    AddBody(glm::vec2(-r1, 0.0f), glm::vec2(0.0f, v_inner * mass2 / totalInner), mass1);
    AddBody(glm::vec2(r2, 0.0f), glm::vec2(0.0f, -v_inner * mass1 / totalInner), mass2);
    
    // Third star in distant orbit
    float mass3 = 10.0f;
    float outerDistance = 120.0f;
    float v_outer = std::sqrt(G * (totalInner + mass3) / outerDistance) * 0.8f; // Slightly elliptical
    
    AddBody(glm::vec2(outerDistance, 0.0f), glm::vec2(0.0f, -v_outer), mass3);
    
    // Add a few test particles - larger distances
    AddBody(glm::vec2(-80.0f, 60.0f), glm::vec2(2.0f, 1.0f), 1.0f);
    AddBody(glm::vec2(80.0f, -60.0f), glm::vec2(-1.5f, 2.0f), 1.0f);
}

void Application::CreateFigureEight() {
    // Famous figure-8 solution discovered by Moore and Chenciner
    // Three equal masses in a figure-8 orbit
    float mass = 5.0f;  // Much smaller mass
    
    // Initial positions (approximate) - scaled up significantly
    float scale = 30.0f;
    AddBody(glm::vec2(-0.97000436f * scale, 0.24308753f * scale), 
            glm::vec2(0.466203685f * 2.0f, 0.43236573f * 2.0f), mass);
    
    AddBody(glm::vec2(0.97000436f * scale, -0.24308753f * scale), 
            glm::vec2(0.466203685f * 2.0f, 0.43236573f * 2.0f), mass);
    
    AddBody(glm::vec2(0.0f, 0.0f), 
            glm::vec2(-2.0f * 0.466203685f * 2.0f, -2.0f * 0.43236573f * 2.0f), mass);
    
    // Add some observer particles - much larger distances
    AddBody(glm::vec2(60.0f, 0.0f), glm::vec2(0.0f, 1.0f), 1.0f);
    AddBody(glm::vec2(-60.0f, 0.0f), glm::vec2(0.0f, -1.0f), 1.0f);
}

void Application::CreateCollisionCourse() {
    // Two clusters on collision course
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> offsetDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> massDist(1.0f, 4.0f);  // Smaller masses
    
    // Left cluster - much larger separation
    glm::vec2 leftCenter(-80.0f, 0.0f);
    glm::vec2 leftVelocity(2.0f, 0.2f);
    
    for (int i = 0; i < 12; ++i) {
        glm::vec2 offset(offsetDist(gen) * 15.0f, offsetDist(gen) * 15.0f);  // Larger spread
        glm::vec2 position = leftCenter + offset;
        glm::vec2 velocity = leftVelocity + glm::vec2(offsetDist(gen) * 0.3f, offsetDist(gen) * 0.3f);
        
        AddBody(position, velocity, massDist(gen));
    }
    
    // Right cluster
    glm::vec2 rightCenter(80.0f, 0.0f);
    glm::vec2 rightVelocity(-1.8f, -0.15f);
    
    for (int i = 0; i < 12; ++i) {
        glm::vec2 offset(offsetDist(gen) * 15.0f, offsetDist(gen) * 15.0f);  // Larger spread
        glm::vec2 position = rightCenter + offset;
        glm::vec2 velocity = rightVelocity + glm::vec2(offsetDist(gen) * 0.3f, offsetDist(gen) * 0.3f);
        
        AddBody(position, velocity, massDist(gen));
    }
    
    // Add observer particles at safe distances
    AddBody(glm::vec2(0.0f, 120.0f), glm::vec2(0.0f, 0.0f), 1.0f);
    AddBody(glm::vec2(0.0f, -120.0f), glm::vec2(0.0f, 0.0f), 1.0f);
}

void Application::SpawnBodies(int count, int pattern) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    float baseRadius = m_ui->GetSpawnRadius();
    float mass = m_ui->GetSpawnMass();
    float speed = m_ui->GetSpawnSpeed();
    
    // Use advanced spatial distribution algorithms
    auto positions = GenerateSpatialDistribution(count, pattern, baseRadius, gen);
    
    for (int i = 0; i < count && i < static_cast<int>(positions.size()); ++i) {
        glm::vec2 position = positions[i];
        glm::vec2 velocity = CalculateVelocityForPattern(position, pattern, speed, i, count, gen);
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

void Application::CharCallback(GLFWwindow* /*window*/, unsigned int codepoint) {
    // Forward to ImGui first
    ImGui_ImplGlfw_CharCallback(glfwGetCurrentContext(), codepoint);
}

void Application::WindowSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    if (s_instance) {
        s_instance->OnWindowResize(width, height);
    }
}

void Application::ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

float Application::CalculateDynamicSpawnRadius(int count, int pattern, float baseRadius) {
    // Constants for spacing calculation
    static constexpr float MIN_BODY_SPACING = 2.0f;  // Minimum distance between bodies
    static constexpr float MAX_RADIUS_MULTIPLIER = 50.0f; // Cap the maximum scaling
    
    // For small counts, use the base radius
    if (count <= 100) {
        return baseRadius;
    }
    
    float scaleFactor = 1.0f;
    
    switch (pattern) {
        case 0: { // Random
            // For random placement, we need enough area to spread bodies
            // Area needed = count * (spacing^2), radius = sqrt(area / π)
            float neededArea = count * MIN_BODY_SPACING * MIN_BODY_SPACING;
            float neededRadius = std::sqrt(neededArea / 3.14159f);
            scaleFactor = neededRadius / baseRadius;
            break;
        }
        case 1: { // Circle
            // For circle, circumference = 2πr, spacing = circumference / count
            // We want spacing >= MIN_BODY_SPACING, so r >= count * spacing / (2π)
            float neededRadius = (count * MIN_BODY_SPACING) / (2.0f * 3.14159f);
            scaleFactor = neededRadius / baseRadius;
            break;
        }
        case 2: { // Grid
            // For grid, we need gridSize x gridSize >= count
            // Total area = (2 * radius)^2, spacing = (2 * radius) / gridSize
            // We want spacing >= MIN_BODY_SPACING
            auto gridSize = static_cast<int>(std::ceil(std::sqrt(count)));
            float neededRadius = (gridSize * MIN_BODY_SPACING) / 2.0f;
            scaleFactor = neededRadius / baseRadius;
            break;
        }
        case 3: { // Spiral
            // For spiral, bodies are distributed along a spiral path
            // Use similar logic to circle but with some extra space for spiral arms
            float neededRadius = (count * MIN_BODY_SPACING) / (2.0f * 3.14159f * 3.0f); // 3 turns
            scaleFactor = neededRadius / baseRadius;
            break;
        }
    }
    
    // Apply reasonable limits
    scaleFactor = std::max(1.0f, scaleFactor); // Never smaller than base
    scaleFactor = std::min(MAX_RADIUS_MULTIPLIER, scaleFactor); // Cap maximum growth
    
    // Apply a small extra buffer for safety (10% extra space)
    scaleFactor *= 1.1f;
    
    return baseRadius * scaleFactor;
}

std::vector<glm::vec2> Application::GenerateSpatialDistribution(int count, int pattern, float baseRadius, std::mt19937& gen) {
    std::vector<glm::vec2> positions;
    positions.reserve(count);
    
    // Constants for proper spacing
    static constexpr float MIN_SEPARATION = 2.0f;
    static constexpr float PERTURBATION_SCALE = 0.1f;
    
    switch (pattern) {
        case 0: { // Random with uniform distribution and collision avoidance
            // Use rejection sampling for proper uniform distribution in circle
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            std::uniform_real_distribution<float> perturbDist(-PERTURBATION_SCALE, PERTURBATION_SCALE);
            
            // Calculate required radius for proper spacing
            float area = count * MIN_SEPARATION * MIN_SEPARATION * 3.14159f; // Area per particle
            float requiredRadius = std::sqrt(area / 3.14159f);
            float actualRadius = std::max(baseRadius, requiredRadius);
            
            int maxAttempts = count * 100; // Limit attempts to avoid infinite loops
            int attempts = 0;
            
            while (static_cast<int>(positions.size()) < count && attempts < maxAttempts) {
                glm::vec2 candidate;
                float radiusSquared;
                
                // Generate point in unit circle using rejection sampling
                do {
                    candidate.x = dist(gen);
                    candidate.y = dist(gen);
                    radiusSquared = candidate.x * candidate.x + candidate.y * candidate.y;
                } while (radiusSquared > 1.0f);
                
                // Scale to actual radius and add perturbation
                candidate *= actualRadius;
                candidate += glm::vec2(perturbDist(gen) * actualRadius, perturbDist(gen) * actualRadius);
                
                // Check minimum distance from existing positions
                bool validPosition = true;
                for (const auto& existing : positions) {
                    if (glm::length(candidate - existing) < MIN_SEPARATION) {
                        validPosition = false;
                        break;
                    }
                }
                
                if (validPosition) {
                    positions.push_back(candidate);
                }
                attempts++;
            }
            break;
        }
        
        case 1: { // Circle with proper spacing
            float circumference = 2.0f * 3.14159f * baseRadius;
            float naturalSpacing = circumference / count;
            
            // If natural spacing is too small, increase radius
            if (naturalSpacing < MIN_SEPARATION) {
                baseRadius = (count * MIN_SEPARATION) / (2.0f * 3.14159f);
            }
            
            std::uniform_real_distribution<float> perturbDist(-PERTURBATION_SCALE * baseRadius, 
                                                            PERTURBATION_SCALE * baseRadius);
            
            for (int i = 0; i < count; ++i) {
                float angle = 2.0f * 3.14159f * i / count;
                glm::vec2 position = glm::vec2(baseRadius * std::cos(angle), baseRadius * std::sin(angle));
                
                // Add small perturbation to prevent perfect regularity
                position += glm::vec2(perturbDist(gen), perturbDist(gen));
                positions.push_back(position);
            }
            break;
        }
        
        case 2: { // Grid with proper spacing
            int gridSize = static_cast<int>(std::ceil(std::sqrt(count)));
            float naturalSpacing = (2.0f * baseRadius) / gridSize;
            
            // Ensure minimum spacing
            if (naturalSpacing < MIN_SEPARATION) {
                float requiredDimension = gridSize * MIN_SEPARATION;
                baseRadius = requiredDimension / 2.0f;
                naturalSpacing = MIN_SEPARATION;
            }
            
            std::uniform_real_distribution<float> perturbDist(-PERTURBATION_SCALE * naturalSpacing,
                                                            PERTURBATION_SCALE * naturalSpacing);
            
            for (int i = 0; i < count; ++i) {
                int row = i / gridSize;
                int col = i % gridSize;
                
                glm::vec2 position = glm::vec2(
                    -baseRadius + col * naturalSpacing + naturalSpacing * 0.5f,
                    -baseRadius + row * naturalSpacing + naturalSpacing * 0.5f
                );
                
                // Add perturbation for more natural look
                position += glm::vec2(perturbDist(gen), perturbDist(gen));
                positions.push_back(position);
            }
            break;
        }
        
        case 3: { // Spiral with proper spacing along curve
            float spiralTurns = 3.0f;
            float totalAngle = spiralTurns * 2.0f * 3.14159f;
            
            // Calculate spiral length approximately
            float avgRadius = baseRadius / 2.0f;
            float approxLength = totalAngle * avgRadius;
            float naturalSpacing = approxLength / count;
            
            // Adjust radius if spacing is too tight
            if (naturalSpacing < MIN_SEPARATION) {
                float scale = MIN_SEPARATION / naturalSpacing;
                baseRadius *= scale;
            }
            
            std::uniform_real_distribution<float> perturbDist(-PERTURBATION_SCALE * MIN_SEPARATION,
                                                            PERTURBATION_SCALE * MIN_SEPARATION);
            
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / count;
                float angle = totalAngle * t;
                float r = baseRadius * t;
                
                glm::vec2 position = glm::vec2(r * std::cos(angle), r * std::sin(angle));
                
                // Add perturbation
                position += glm::vec2(perturbDist(gen), perturbDist(gen));
                positions.push_back(position);
            }
            break;
        }
        
        case 4: { // Poisson disk sampling (advanced non-overlapping distribution)
            positions = GeneratePoissonDiskSampling(count, baseRadius, MIN_SEPARATION, gen);
            break;
        }
        
        default: {
            // Fallback to simple uniform circle
            std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
            std::uniform_real_distribution<float> radiusDist(0.0f, baseRadius);
            
            for (int i = 0; i < count; ++i) {
                float angle = angleDist(gen);
                float r = std::sqrt(radiusDist(gen)) * baseRadius; // sqrt for uniform area distribution
                positions.emplace_back(r * std::cos(angle), r * std::sin(angle));
            }
            break;
        }
    }
    
    return positions;
}

std::vector<glm::vec2> Application::GeneratePoissonDiskSampling(int targetCount, float radius, float minDistance, std::mt19937& gen) {
    // Bridson's Poisson disk sampling algorithm - guarantees no overlapping within minDistance
    std::vector<glm::vec2> points;
    std::vector<glm::vec2> activeList;
    
    // Grid for fast spatial lookup
    float cellSize = minDistance / std::sqrt(2.0f);
    int gridWidth = static_cast<int>(std::ceil(2.0f * radius / cellSize));
    int gridHeight = gridWidth;
    std::vector<std::vector<int>> grid(gridWidth, std::vector<int>(gridHeight, -1));
    
    auto getGridCoord = [&](const glm::vec2& p) -> glm::ivec2 {
        return glm::ivec2(
            static_cast<int>((p.x + radius) / cellSize),
            static_cast<int>((p.y + radius) / cellSize)
        );
    };
    
    auto isValidGrid = [&](const glm::ivec2& coord) -> bool {
        return coord.x >= 0 && coord.x < gridWidth && coord.y >= 0 && coord.y < gridHeight;
    };
    
    // Start with random point
    std::uniform_real_distribution<float> dist(-radius, radius);
    glm::vec2 firstPoint;
    do {
        firstPoint = glm::vec2(dist(gen), dist(gen));
    } while (glm::length(firstPoint) > radius);
    
    points.push_back(firstPoint);
    activeList.push_back(firstPoint);
    
    auto gridCoord = getGridCoord(firstPoint);
    if (isValidGrid(gridCoord)) {
        grid[gridCoord.x][gridCoord.y] = 0;
    }
    
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDistPoisson(minDistance, 2.0f * minDistance);
    std::uniform_int_distribution<int> activeDist;
    
    int maxAttempts = 30; // Attempts per active point
    
    while (!activeList.empty() && static_cast<int>(points.size()) < targetCount) {
        // Choose random active point
        activeDist = std::uniform_int_distribution<int>(0, static_cast<int>(activeList.size()) - 1);
        int activeIndex = activeDist(gen);
        glm::vec2 activePoint = activeList[activeIndex];
        
        bool foundNewPoint = false;
        
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            // Generate candidate around active point
            float angle = angleDist(gen);
            float r = radiusDistPoisson(gen);
            glm::vec2 candidate = activePoint + glm::vec2(r * std::cos(angle), r * std::sin(angle));
            
            // Check if candidate is within bounds
            if (glm::length(candidate) > radius) continue;
            
            // Check distance to existing points using grid
            auto candidateGrid = getGridCoord(candidate);
            if (!isValidGrid(candidateGrid)) continue;
            
            bool tooClose = false;
            
            // Check surrounding grid cells
            for (int dx = -2; dx <= 2 && !tooClose; ++dx) {
                for (int dy = -2; dy <= 2 && !tooClose; ++dy) {
                    glm::ivec2 neighborGrid = candidateGrid + glm::ivec2(dx, dy);
                    if (!isValidGrid(neighborGrid)) continue;
                    
                    int pointIndex = grid[neighborGrid.x][neighborGrid.y];
                    if (pointIndex >= 0) {
                        float pointDist = glm::length(candidate - points[pointIndex]);
                        if (pointDist < minDistance) {
                            tooClose = true;
                        }
                    }
                }
            }
            
            if (!tooClose) {
                // Valid point found
                points.push_back(candidate);
                activeList.push_back(candidate);
                grid[candidateGrid.x][candidateGrid.y] = static_cast<int>(points.size()) - 1;
                foundNewPoint = true;
                break;
            }
        }
        
        if (!foundNewPoint) {
            // Remove active point if no new points found
            activeList.erase(activeList.begin() + activeIndex);
        }
    }
    
    return points;
}

glm::vec2 Application::CalculateVelocityForPattern(const glm::vec2& position, int pattern, float speed, int /*index*/, int /*totalCount*/, std::mt19937& gen) {
    if (speed <= 0.0f) {
        return glm::vec2(0.0f);
    }
    
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> speedVariation(0.7f, 1.3f);
    
    glm::vec2 velocity(0.0f);
    
    switch (pattern) {
        case 0: { // Random velocities
            float angle = angleDist(gen);
            float vel = speed * speedVariation(gen);
            velocity = glm::vec2(vel * std::cos(angle), vel * std::sin(angle));
            break;
        }
        
        case 1: { // Circular orbital motion
            float distance = glm::length(position);
            if (distance > 0.001f) {
                glm::vec2 normalized = position / distance;
                velocity = glm::vec2(-normalized.y, normalized.x) * speed;
            }
            break;
        }
        
        case 2: { // Grid - random individual velocities
            float angle = angleDist(gen);
            velocity = glm::vec2(speed * std::cos(angle), speed * std::sin(angle));
            break;
        }
        
        case 3: { // Spiral - tangential velocity
            float distance = glm::length(position);
            if (distance > 0.001f) {
                glm::vec2 normalized = position / distance;
                velocity = glm::vec2(-normalized.y, normalized.x) * speed;
                
                // Add slight radial component for spiral expansion
                velocity += normalized * (speed * 0.1f);
            }
            break;
        }
        
        case 4: { // Poisson disk - converging/diverging motion
            float distance = glm::length(position);
            if (distance > 0.001f) {
                glm::vec2 normalized = position / distance;
                // Mix of radial and tangential motion
                glm::vec2 radial = normalized * speed * 0.3f;
                glm::vec2 tangential = glm::vec2(-normalized.y, normalized.x) * speed * 0.7f;
                velocity = radial + tangential;
            }
            break;
        }
        
        default: {
            float angle = angleDist(gen);
            velocity = glm::vec2(speed * std::cos(angle), speed * std::sin(angle));
            break;
        }
    }
    
    return velocity;
}

} // namespace nbody
