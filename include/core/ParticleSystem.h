#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "core/Particle.h"

namespace nbody {

/**
 * @brief Manages a collection of particles
 * Provides efficient data access for physics solvers
 */
class ParticleSystem {
public:
    ParticleSystem() = default;
    ~ParticleSystem() = default;
    
    // Particle management
    void AddParticle(const Particle& particle);
    void AddParticle(const glm::vec2& position, const glm::vec2& velocity, float mass);
    void RemoveParticle(size_t index);
    void Clear();
    
    // Data access
    size_t GetParticleCount() const { return m_particles.size(); }
    bool IsEmpty() const { return m_particles.empty(); }
    
    // Direct access for performance
    std::vector<Particle>& GetParticles() { return m_particles; }
    const std::vector<Particle>& GetParticles() const { return m_particles; }
    
    Particle& GetParticle(size_t index) { return m_particles[index]; }
    const Particle& GetParticle(size_t index) const { return m_particles[index]; }
    
    // Iterator support
    auto begin() { return m_particles.begin(); }
    auto end() { return m_particles.end(); }
    auto begin() const { return m_particles.begin(); }
    auto end() const { return m_particles.end(); }
    
    // Statistics
    glm::vec2 GetCenterOfMass() const;
    float GetTotalMass() const;
    glm::vec4 GetBoundingBox() const; // min_x, min_y, max_x, max_y
    
private:
    std::vector<Particle> m_particles;
};

} // namespace nbody
