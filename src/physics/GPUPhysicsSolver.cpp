#include "physics/GPUPhysicsSolver.h"
#include "rendering/ComputeShader.h"
#include "core/Body.h"
#include <iostream>
#include <glm/glm.hpp>

namespace nbody {

GPUPhysicsSolver::GPUPhysicsSolver() {
    // Initialize member variables
}

GPUPhysicsSolver::~GPUPhysicsSolver() {
    Cleanup();
}

bool GPUPhysicsSolver::Initialize() {
    // Check if compute shaders are supported
    if (!ComputeShader::IsSupported()) {
        std::cerr << "GPU Physics Solver: Compute shaders not supported" << std::endl;
        return false;
    }
    
    // Create compute shaders
    m_forceComputeShader = std::make_unique<ComputeShader>();
    m_integrationShader = std::make_unique<ComputeShader>();
    
    // Load force calculation shader
    if (!m_forceComputeShader->LoadFromFile("shaders/compute/force_calculation.comp")) {
        std::cerr << "Failed to load force calculation compute shader" << std::endl;
        return false;
    }
    
    // Load integration shader
    if (!m_integrationShader->LoadFromFile("shaders/compute/integration.comp")) {
        std::cerr << "Failed to load integration compute shader" << std::endl;
        return false;
    }
    
    std::cout << "GPU Physics Solver initialized successfully" << std::endl;
    return true;
}

void GPUPhysicsSolver::Update(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) {
    if (bodies.empty()) return;
    
    const size_t particleCount = bodies.size();
    
    // Create or resize buffers if needed
    if (particleCount > m_maxParticles) {
        if (!CreateBuffers(particleCount)) {
            std::cerr << "Failed to create GPU buffers" << std::endl;
            return;
        }
        m_maxParticles = particleCount;
    }
    
    // Upload data to GPU
    UploadData(bodies);
    
    // Calculate forces on GPU
    m_forceComputeShader->Use();
    m_forceComputeShader->SetInt("numParticles", static_cast<int>(particleCount));
    m_forceComputeShader->SetFloat("gravitationalConstant", m_gravitationalConstant);
    m_forceComputeShader->SetFloat("softening", m_softeningLength);
    
    // Dispatch force calculation (64 threads per work group)
    const GLuint numWorkGroups = static_cast<GLuint>((particleCount + 63) / 64);
    m_forceComputeShader->Dispatch(numWorkGroups);
    
    // Memory barrier to ensure forces are calculated before integration
    ComputeShader::MemoryBarrier();
    
    // Integrate motion on GPU
    m_integrationShader->Use();
    m_integrationShader->SetInt("numParticles", static_cast<int>(particleCount));
    m_integrationShader->SetFloat("deltaTime", deltaTime);
    
    m_integrationShader->Dispatch(numWorkGroups);
    ComputeShader::MemoryBarrier();
    
    // Download results from GPU
    DownloadData(bodies);
}

bool GPUPhysicsSolver::CreateBuffers(size_t particleCount) {
    // Clean up existing buffers
    Cleanup();
    
    const size_t vec4Size = sizeof(float) * 4;
    const size_t floatSize = sizeof(float);
    
    // Create position buffer (vec4: x, y, z, w)
    m_positionBuffer = ComputeShader::CreateSSBO(nullptr, particleCount * vec4Size, GL_DYNAMIC_DRAW);
    ComputeShader::BindSSBO(m_positionBuffer, 0);
    
    // Create velocity buffer (vec4: vx, vy, vz, w)
    m_velocityBuffer = ComputeShader::CreateSSBO(nullptr, particleCount * vec4Size, GL_DYNAMIC_DRAW);
    ComputeShader::BindSSBO(m_velocityBuffer, 1);
    
    // Create mass buffer (float)
    m_massBuffer = ComputeShader::CreateSSBO(nullptr, particleCount * floatSize, GL_DYNAMIC_DRAW);
    ComputeShader::BindSSBO(m_massBuffer, 2);
    
    // Create force buffer (vec4: fx, fy, fz, w)
    m_forceBuffer = ComputeShader::CreateSSBO(nullptr, particleCount * vec4Size, GL_DYNAMIC_DRAW);
    ComputeShader::BindSSBO(m_forceBuffer, 3);
    
    std::cout << "Created GPU buffers for " << particleCount << " particles" << std::endl;
    return true;
}

void GPUPhysicsSolver::UploadData(const std::vector<std::unique_ptr<Body>>& bodies) {
    const size_t particleCount = bodies.size();
    
    // Prepare data arrays
    std::vector<glm::vec4> positions(particleCount);
    std::vector<glm::vec4> velocities(particleCount);
    std::vector<float> masses(particleCount);
    
    for (size_t i = 0; i < particleCount; ++i) {
        const auto& body = bodies[i];
        auto pos = body->GetPosition();
        auto vel = body->GetVelocity();
        
        positions[i] = glm::vec4(pos.x, pos.y, 0.0f, 1.0f);
        velocities[i] = glm::vec4(vel.x, vel.y, 0.0f, 0.0f);
        masses[i] = body->GetMass();
    }
    
    // Upload to GPU
    ComputeShader::UpdateSSBO(m_positionBuffer, 0, positions.size() * sizeof(glm::vec4), positions.data());
    ComputeShader::UpdateSSBO(m_velocityBuffer, 0, velocities.size() * sizeof(glm::vec4), velocities.data());
    ComputeShader::UpdateSSBO(m_massBuffer, 0, masses.size() * sizeof(float), masses.data());
}

void GPUPhysicsSolver::DownloadData(std::vector<std::unique_ptr<Body>>& bodies) {
    const size_t particleCount = bodies.size();
    
    // Download results from GPU
    std::vector<glm::vec4> positions(particleCount);
    std::vector<glm::vec4> velocities(particleCount);
    
    ComputeShader::ReadSSBO(m_positionBuffer, 0, positions.size() * sizeof(glm::vec4), positions.data());
    ComputeShader::ReadSSBO(m_velocityBuffer, 0, velocities.size() * sizeof(glm::vec4), velocities.data());
    
    // Update body data
    for (size_t i = 0; i < particleCount; ++i) {
        bodies[i]->SetPosition(glm::vec2(positions[i].x, positions[i].y));
        bodies[i]->SetVelocity(glm::vec2(velocities[i].x, velocities[i].y));
    }
}

void GPUPhysicsSolver::Cleanup() {
    ComputeShader::DeleteBuffer(m_positionBuffer);
    ComputeShader::DeleteBuffer(m_velocityBuffer);
    ComputeShader::DeleteBuffer(m_massBuffer);
    ComputeShader::DeleteBuffer(m_forceBuffer);
    
    m_positionBuffer = 0;
    m_velocityBuffer = 0;
    m_massBuffer = 0;
    m_forceBuffer = 0;
    m_maxParticles = 0;
}

} // namespace nbody
