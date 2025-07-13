#include "physics/IParticleSolver.h"
#include <execution>
#include <algorithm>
#include <cmath>

namespace nbody {

// Constants
static constexpr float MIN_DISTANCE = 0.1f;  // Prevent singularities
static constexpr float MAX_FORCE = 1000.0f;  // Cap force for stability

// ============================================================================
// CPUSequentialSolver Implementation
// ============================================================================

CPUSequentialSolver::CPUSequentialSolver(float timeStep, float squaredSoftening)
    : m_squaredSoftening(squaredSoftening) 
{
    m_timeStep = timeStep;
}

void CPUSequentialSolver::UpdateParticlePositions(ParticleSystem* particles) {
    if (!particles || particles->IsEmpty()) return;
    
    auto& particleData = particles->GetParticles();
    
    // Clear accelerations
    for (auto& particle : particleData) {
        particle.acceleration = glm::vec2(0.0f);
    }
    
    // Compute forces for all particles
    for (size_t i = 0; i < particleData.size(); ++i) {
        ComputeGravityForce(particles, i);
    }
    
    // Integrate motion using Leapfrog integration
    for (auto& particle : particleData) {
        // Update velocity: v = v + a * dt
        particle.velocity += particle.acceleration * m_timeStep;
        
        // Update position: x = x + v * dt
        particle.position += particle.velocity * m_timeStep;
    }
}

void CPUSequentialSolver::ComputeGravityForce(ParticleSystem* particles, size_t particleId) {
    auto& particleData = particles->GetParticles();
    auto& particle = particleData[particleId];
    
    glm::vec2 totalForce(0.0f);
    
    for (size_t j = 0; j < particleData.size(); ++j) {
        if (j == particleId) continue;
        
        const auto& other = particleData[j];
        
        // Calculate distance vector
        glm::vec2 r = other.GetPosition() - particle.GetPosition();
        float rSqr = glm::dot(r, r) + m_squaredSoftening;
        
        // Avoid singularities
        if (rSqr < MIN_DISTANCE * MIN_DISTANCE) {
            continue;
        }
        
        float invR = 1.0f / std::sqrt(rSqr);
        float invR3 = invR * invR * invR;
        
        // F = G * m1 * m2 / r² * r_hat
        glm::vec2 force = m_gravitationalConstant * particle.mass * other.mass * invR3 * r;
        
        // Cap force to prevent instability
        float forceMag = glm::length(force);
        if (forceMag > MAX_FORCE) {
            force = force * (MAX_FORCE / forceMag);
        }
        
        totalForce += force;
    }
    
    // Convert force to acceleration: a = F / m
    glm::vec2 acceleration = totalForce / particle.mass;
    particle.acceleration = acceleration;
}

// ============================================================================
// CPUParallelSolver Implementation  
// ============================================================================

CPUParallelSolver::CPUParallelSolver(float timeStep, float squaredSoftening)
    : m_squaredSoftening(squaredSoftening)
{
    m_timeStep = timeStep;
}

void CPUParallelSolver::UpdateParticlePositions(ParticleSystem* particles) {
    if (!particles || particles->IsEmpty()) return;
    
    auto& particleData = particles->GetParticles();
    
    // Clear accelerations
    std::for_each(std::execution::par_unseq, particleData.begin(), particleData.end(),
        [](auto& particle) {
            particle.acceleration = glm::vec2(0.0f);
        });
    
    // Compute forces in parallel
    ComputeGravityForces(particles);
    
    // Integrate motion in parallel
    std::for_each(std::execution::par_unseq, particleData.begin(), particleData.end(),
        [this](auto& particle) {
            // Update velocity: v = v + a * dt
            particle.velocity += particle.acceleration * m_timeStep;
            
            // Update position: x = x + v * dt  
            particle.position += particle.velocity * m_timeStep;
        });
}

void CPUParallelSolver::ComputeGravityForces(ParticleSystem* particles) {
    auto& particleData = particles->GetParticles();
    const size_t numParticles = particleData.size();
    
    // Create indices for parallel processing
    std::vector<size_t> indices(numParticles);
    std::iota(indices.begin(), indices.end(), 0);
    
    // Parallel force computation
    std::for_each(std::execution::par_unseq, indices.begin(), indices.end(),
        [&](size_t i) {
            auto& particle = particleData[i];
            glm::vec2 totalForce(0.0f);
            
            for (size_t j = 0; j < numParticles; ++j) {
                if (j == i) continue;
                
                const auto& other = particleData[j];
                
                // Calculate distance vector
                glm::vec2 r = other.GetPosition() - particle.GetPosition();
                float rSqr = glm::dot(r, r) + m_squaredSoftening;
                
                // Avoid singularities
                if (rSqr < MIN_DISTANCE * MIN_DISTANCE) {
                    continue;
                }
                
                float invR = 1.0f / std::sqrt(rSqr);
                float invR3 = invR * invR * invR;
                
                // F = G * m1 * m2 / r² * r_hat
                glm::vec2 force = m_gravitationalConstant * particle.mass * other.mass * invR3 * r;
                
                // Cap force to prevent instability
                float forceMag = glm::length(force);
                if (forceMag > MAX_FORCE) {
                    force = force * (MAX_FORCE / forceMag);
                }
                
                totalForce += force;
            }
            
            // Convert force to acceleration: a = F / m
            glm::vec2 acceleration = totalForce / particle.mass;
            particle.acceleration = acceleration;
        });
}

} // namespace nbody
