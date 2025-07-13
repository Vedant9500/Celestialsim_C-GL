#include "core/ParticleSystem.h"
#include <algorithm>
#include <limits>

namespace nbody {

void ParticleSystem::AddParticle(const Particle& particle) {
    m_particles.push_back(particle);
}

void ParticleSystem::AddParticle(const glm::vec2& position, const glm::vec2& velocity, float mass) {
    m_particles.emplace_back(position, velocity, mass);
}

void ParticleSystem::RemoveParticle(size_t index) {
    if (index < m_particles.size()) {
        m_particles.erase(m_particles.begin() + index);
    }
}

void ParticleSystem::Clear() {
    m_particles.clear();
}

glm::vec2 ParticleSystem::GetCenterOfMass() const {
    if (m_particles.empty()) {
        return glm::vec2(0.0f);
    }
    
    glm::vec2 com(0.0f);
    float totalMass = 0.0f;
    
    for (const auto& particle : m_particles) {
        com += particle.GetPosition() * particle.mass;
        totalMass += particle.mass;
    }
    
    return totalMass > 0.0f ? com / totalMass : glm::vec2(0.0f);
}

float ParticleSystem::GetTotalMass() const {
    float total = 0.0f;
    for (const auto& particle : m_particles) {
        total += particle.mass;
    }
    return total;
}

glm::vec4 ParticleSystem::GetBoundingBox() const {
    if (m_particles.empty()) {
        return glm::vec4(0.0f);
    }
    
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    
    for (const auto& particle : m_particles) {
        const auto pos = particle.GetPosition();
        minX = std::min(minX, pos.x);
        minY = std::min(minY, pos.y);
        maxX = std::max(maxX, pos.x);
        maxY = std::max(maxY, pos.y);
    }
    
    return glm::vec4(minX, minY, maxX, maxY);
}

} // namespace nbody
