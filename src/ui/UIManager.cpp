#include "ui/UIManager.h"
#include "core/Body.h"
#include "physics/PhysicsEngine.h"
#include "rendering/Renderer.h"
#include <algorithm>

namespace nbody {

UIManager::UIManager() = default;

UIManager::~UIManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool UIManager::Initialize(GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup Dear ImGui style
    SetupStyle();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, false); // Don't install callbacks automatically
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Initialize history buffers
    m_fpsHistory.resize(m_maxHistorySize, 0.0f);
    m_energyHistory.resize(m_maxHistorySize, 0.0f);
    
    return true;
}

void UIManager::SetupStyle() {
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.WindowMenuButtonPosition = ImGuiDir_None;
    
    // Increase window padding for better spacing
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(5, 3);
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);
    style.IndentSpacing = 20.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 0.95f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 0.95f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 0.70f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.80f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.50f, 0.50f, 0.50f, 0.90f);
    colors[ImGuiCol_Button] = ImVec4(0.35f, 0.35f, 0.35f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.45f, 0.45f, 0.90f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.30f, 0.80f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.90f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
}

void UIManager::NewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::Render(const std::vector<std::unique_ptr<Body>>& bodies,
                      const PhysicsEngine& physics,
                      const Renderer& renderer,
                      const Body* selectedBody) {
    if (m_showMainWindow) {
        RenderMainMenuBar();
    }
    
    if (m_showControlsWindow) {
        RenderControlPanel();
    }
    
    if (m_showStatsWindow) {
        RenderStatsPanel(bodies, physics, renderer);
    }
    
    if (m_showBodyWindow) {
        RenderBodyPanel(selectedBody);
    }
    
    if (m_showDebugWindow) {
        RenderDebugPanel();
    }
    
    if (m_showAboutWindow) {
        RenderAboutPanel();
    }
}

void UIManager::RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Configuration")) {
                if (OnSaveConfig) OnSaveConfig(m_configFilename);
            }
            if (ImGui::MenuItem("Load Configuration")) {
                if (OnLoadConfig) OnLoadConfig(m_configFilename);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Simulation")) {
            if (ImGui::MenuItem("Play/Pause", "Space")) {
                if (OnPlayPause) OnPlayPause();
            }
            if (ImGui::MenuItem("Reset", "R")) {
                if (OnReset) OnReset();
            }
            if (ImGui::MenuItem("Clear All", "C")) {
                if (OnClear) OnClear();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Presets")) {
            if (ImGui::MenuItem("Solar System")) {
                if (OnLoadPreset) OnLoadPreset("Solar System");
            }
            if (ImGui::MenuItem("Binary System")) {
                if (OnLoadPreset) OnLoadPreset("Binary System");
            }
            if (ImGui::MenuItem("Galaxy")) {
                if (OnLoadPreset) OnLoadPreset("Galaxy");
            }
            if (ImGui::MenuItem("Random Cluster")) {
                if (OnLoadPreset) OnLoadPreset("Random Cluster");
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Controls", nullptr, &m_showControlsWindow);
            ImGui::MenuItem("Statistics", nullptr, &m_showStatsWindow);
            ImGui::MenuItem("Body Properties", nullptr, &m_showBodyWindow);
            ImGui::MenuItem("Debug Info", nullptr, &m_showDebugWindow);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About", nullptr, &m_showAboutWindow);
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void UIManager::RenderControlPanel() {
    // Position panel on the left side with responsive sizing
    float panelWidth = std::min(320.0f, m_windowWidth * 0.25f);
    float panelHeight = m_windowHeight - 2 * MARGIN;
    
    ImGui::SetNextWindowPos(ImVec2(MARGIN, MARGIN), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_FirstUseEver);
    
    // Use minimal flags to allow full interaction
    ImGuiWindowFlags flags = 0;
    
    if (!ImGui::Begin("Simulation Controls", &m_showControlsWindow, flags)) {
        ImGui::End();
        return;
    }
    
    // Simulation controls
    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Play/Pause/Reset buttons
        float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;
        
        if (ImGui::Button("Play/Pause", ImVec2(buttonWidth, 0))) {
            if (OnPlayPause) OnPlayPause();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(buttonWidth, 0))) {
            if (OnReset) OnReset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear", ImVec2(buttonWidth, 0))) {
            if (OnClear) OnClear();
        }
    }
    
    // Physics parameters with change detection
    if (ImGui::CollapsingHeader("Physics Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Gravity", &m_gravitationalConstant, 0.1f, 1000.0f, "%.1f")) {
            if (OnPhysicsParameterChanged) OnPhysicsParameterChanged();
        }
        if (ImGui::SliderFloat("Time Step", &m_timeStep, 0.001f, 0.1f, "%.3f")) {
            if (OnPhysicsParameterChanged) OnPhysicsParameterChanged();
        }
        if (ImGui::SliderFloat("Softening", &m_softeningLength, 0.001f, 1.0f, "%.3f")) {
            if (OnPhysicsParameterChanged) OnPhysicsParameterChanged();
        }
        
        if (ImGui::Checkbox("Barnes-Hut", &m_useBarnesHut)) {
            if (OnPhysicsParameterChanged) OnPhysicsParameterChanged();
        }
        if (m_useBarnesHut) {
            if (ImGui::SliderFloat("Theta", &m_barnesHutTheta, 0.1f, 2.0f, "%.2f")) {
                if (OnPhysicsParameterChanged) OnPhysicsParameterChanged();
            }
            ImGui::SameLine(); ShowHelpMarker("Lower theta = more accurate but slower");
        }
        
        if (ImGui::Checkbox("Collisions", &m_enableCollisions)) {
            if (OnPhysicsParameterChanged) OnPhysicsParameterChanged();
        }
        if (m_enableCollisions) {
            if (ImGui::SliderFloat("Restitution", &m_restitution, 0.0f, 1.0f, "%.2f")) {
                if (OnPhysicsParameterChanged) OnPhysicsParameterChanged();
            }
            ImGui::SameLine(); ShowHelpMarker("0 = perfectly inelastic, 1 = perfectly elastic");
        }
        
        if (ImGui::Checkbox("GPU Compute", &m_useGPU)) {
            if (OnPhysicsParameterChanged) OnPhysicsParameterChanged();
        }
    }
    
    // Presets
    if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Solar System", ImVec2(-1, 0))) {
            if (OnLoadPreset) OnLoadPreset("Solar System");
        }
        if (ImGui::Button("Binary System", ImVec2(-1, 0))) {
            if (OnLoadPreset) OnLoadPreset("Binary System");
        }
        if (ImGui::Button("Galaxy", ImVec2(-1, 0))) {
            if (OnLoadPreset) OnLoadPreset("Galaxy");
        }
        if (ImGui::Button("Random Cluster", ImVec2(-1, 0))) {
            if (OnLoadPreset) OnLoadPreset("Random Cluster");
        }
    }
    
    // Body creation
    if (ImGui::CollapsingHeader("Add Bodies", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Orbit Mode", &m_orbitMode);
        ShowHelpMarker("New bodies will be placed in orbit around nearest body");
        
        ImGui::SliderFloat("Mass", &m_newBodyMass, 1.0f, 1000.0f, "%.1f");
        
        float colorArray[3] = {m_newBodyColor.x, m_newBodyColor.y, m_newBodyColor.z};
        if (ImGui::ColorEdit3("Color", colorArray)) {
            m_newBodyColor = glm::vec3(colorArray[0], colorArray[1], colorArray[2]);
        }
        
        if (!m_orbitMode) {
            ImGui::SliderFloat("Velocity X", &m_newBodyVelocity.x, -50.0f, 50.0f, "%.2f");
            ImGui::SliderFloat("Velocity Y", &m_newBodyVelocity.y, -50.0f, 50.0f, "%.2f");
        }
        
        ImGui::Text("Controls:");
        ImGui::BulletText("Left click: Add/Select body");
        ImGui::BulletText("Right click: Delete body");
        ImGui::BulletText("Drag: Move selected body");
        ImGui::BulletText("Mouse wheel: Zoom");
        ImGui::BulletText("Middle mouse: Pan");
    }
    
    // Rendering options
    if (ImGui::CollapsingHeader("Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Checkbox("Show Trails", &m_showTrails)) {
            if (OnRenderParameterChanged) OnRenderParameterChanged();
        }
        if (m_showTrails) {
            ImGui::SliderInt("Trail Length", &m_trailLength, 10, 500);
        }
        if (ImGui::Checkbox("Show Grid", &m_showGrid)) {
            if (OnRenderParameterChanged) OnRenderParameterChanged();
        }
        if (ImGui::Checkbox("Show Forces", &m_showForces)) {
            if (OnRenderParameterChanged) OnRenderParameterChanged();
        }
        if (ImGui::Checkbox("Show QuadTree", &m_showQuadTree)) {
            if (OnRenderParameterChanged) OnRenderParameterChanged();
        }
    }
    
    // Camera controls
    if (ImGui::CollapsingHeader("Camera")) {
        if (ImGui::Button("Reset Camera", ImVec2(-1, 0))) {
            if (OnResetCamera) OnResetCamera();
        }
        if (ImGui::Button("Fit All Bodies", ImVec2(-1, 0))) {
            if (OnFitAllBodies) OnFitAllBodies();
        }
        
        ImGui::Text("Zoom: %.2f", m_cameraZoom);
        ImGui::Text("Position: (%.1f, %.1f)", m_cameraPosition.x, m_cameraPosition.y);
    }
    
    ImGui::End();
}

void UIManager::RenderStatsPanel(const std::vector<std::unique_ptr<Body>>& bodies,
                                const PhysicsEngine& physics,
                                const Renderer& renderer) {
    // Position panel on the right side with responsive sizing
    float panelWidth = std::min(300.0f, m_windowWidth * 0.25f);
    float panelHeight = m_windowHeight * 0.6f;
    
    ImGui::SetNextWindowPos(ImVec2(m_windowWidth - panelWidth - MARGIN, MARGIN), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags flags = 0;
    
    if (!ImGui::Begin("Statistics", &m_showStatsWindow, flags)) {
        ImGui::End();
        return;
    }
    
    // Basic stats
    ImGui::Text("Bodies: %zu", bodies.size());
    ImGui::Separator();
    
    // Physics stats
    const auto& physicsStats = physics.GetStats();
    if (ImGui::CollapsingHeader("Physics Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Method: %s", physicsStats.method.c_str());
        ImGui::Text("Total Time: %.2f ms", physicsStats.totalTime);
        ImGui::Text("Force Calc: %.2f ms", physicsStats.forceCalculationTime);
        ImGui::Text("Integration: %.2f ms", physicsStats.integrationTime);
        ImGui::Text("Collisions: %.2f ms", physicsStats.collisionTime);
        ImGui::Text("Force Calculations: %d", physicsStats.forceCalculations);
        ImGui::Text("Collisions: %d", physicsStats.collisions);
    }
    
    // Energy stats
    auto energyStats = physics.CalculateEnergyStats(bodies);
    if (ImGui::CollapsingHeader("Energy", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Kinetic: %.2e", energyStats.kinetic);
        ImGui::Text("Potential: %.2e", energyStats.potential);
        ImGui::Text("Total: %.2e", energyStats.total);
        
        // Update energy history
        static size_t historyIndex = 0;
        m_energyHistory[historyIndex] = static_cast<float>(energyStats.total);
        historyIndex = (historyIndex + 1) % m_maxHistorySize;
        
        // Plot energy graph
        ImGui::PlotLines("Energy", m_energyHistory.data(), static_cast<int>(m_energyHistory.size()), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 80));
    }
    
    // Performance stats
    const auto& renderStats = renderer.GetStats();
    if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", renderStats.fps);
        ImGui::Text("Render Time: %.2f ms", renderStats.renderTime);
        ImGui::Text("Bodies Rendered: %d", renderStats.bodiesRendered);
        ImGui::Text("Draw Calls: %d", renderStats.drawCalls);
    }
    
    ImGui::End();
}

void UIManager::RenderBodyPanel(const Body* selectedBody) {
    // Position panel on the bottom right
    float panelWidth = std::min(300.0f, m_windowWidth * 0.25f);
    float panelHeight = m_windowHeight * 0.35f;
    
    ImGui::SetNextWindowPos(ImVec2(m_windowWidth - panelWidth - MARGIN, m_windowHeight - panelHeight - MARGIN), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags flags = 0;
    
    if (!ImGui::Begin("Body Properties", &m_showBodyWindow, flags)) {
        ImGui::End();
        return;
    }
    
    if (selectedBody) {
        ImGui::Text("Selected Body Properties:");
        ImGui::Separator();
        
        // Position
        ImGui::Text("Position:");
        ImGui::Text("  X: %.3f", selectedBody->GetPosition().x);
        ImGui::Text("  Y: %.3f", selectedBody->GetPosition().y);
        
        // Velocity
        glm::vec2 vel = selectedBody->GetVelocity();
        ImGui::Text("Velocity:");
        ImGui::Text("  X: %.3f", vel.x);
        ImGui::Text("  Y: %.3f", vel.y);
        ImGui::Text("  Speed: %.3f", selectedBody->GetSpeed());
        
        // Physical properties
        ImGui::Separator();
        ImGui::Text("Physical Properties:");
        ImGui::Text("Mass: %.2f", selectedBody->GetMass());
        ImGui::Text("Radius: %.3f", selectedBody->GetRadius());
        ImGui::Text("Kinetic Energy: %.2e", selectedBody->GetKineticEnergy());
        
        // Color
        glm::vec3 color = selectedBody->GetColor();
        ImGui::Text("Color: (%.2f, %.2f, %.2f)", color.r, color.g, color.b);
        
        ImGui::Separator();
        
        // Actions
        if (ImGui::Button("Delete Body", ImVec2(-1, 0))) {
            if (OnDeleteBody) OnDeleteBody(const_cast<Body*>(selectedBody));
        }
        
        if (ImGui::Button("Center Camera", ImVec2(-1, 0))) {
            if (OnSetCameraPosition) OnSetCameraPosition(selectedBody->GetPosition());
        }
    } else {
        ImGui::Text("No body selected");
        ImGui::Separator();
        ImGui::Text("Instructions:");
        ImGui::BulletText("Left click on a body to select it");
        ImGui::BulletText("Drag selected body to move it");
        ImGui::BulletText("Right click to delete a body");
        
        ImGui::Separator();
        ImGui::Text("Camera Controls:");
        ImGui::BulletText("Mouse wheel: Zoom in/out");
        ImGui::BulletText("Middle mouse: Pan camera");
        ImGui::BulletText("Ctrl + Left mouse: Pan camera");
    }
    
    ImGui::End();
}

void UIManager::RenderDebugPanel() {
    ImGui::Begin("Debug Info", &m_showDebugWindow);
    
    ImGui::Text("Camera Position: (%.2f, %.2f)", m_cameraPosition.x, m_cameraPosition.y);
    ImGui::Text("Camera Zoom: %.2f", m_cameraZoom);
    
    if (ImGui::Button("Reset Camera")) {
        m_cameraPosition = glm::vec2(0.0f);
        m_cameraZoom = 1.0f;
    }
    
    ImGui::End();
}

void UIManager::RenderAboutPanel() {
    ImGui::Begin("About", &m_showAboutWindow);
    
    ImGui::Text("N-Body Simulation");
    ImGui::Text("Version 1.0.0");
    ImGui::Separator();
    ImGui::Text("A high-performance gravitational simulation");
    ImGui::Text("Built with C++ and OpenGL");
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("Left click: Add body / Select body");
    ImGui::BulletText("Right click: Delete body");
    ImGui::BulletText("Mouse wheel: Zoom");
    ImGui::BulletText("Middle mouse: Pan");
    ImGui::BulletText("Space: Play/Pause");
    ImGui::BulletText("R: Reset");
    ImGui::BulletText("C: Clear all");
    
    ImGui::End();
}

void UIManager::ShowHelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool UIManager::IsMouseOverUI() const {
    return ImGui::GetIO().WantCaptureMouse;
}

void UIManager::OnWindowResize(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

} // namespace nbody
