#pragma once

#include "ParticleSystemInitializer.h"

namespace nbody {

/**
 * @brief Initializes particles in a galaxy-like spiral formation
 */
class GalaxyInitializer : public ParticleSystemInitializer {
public:
    GalaxyInitializer(float armCount = 2.0f, float armTightness = 0.5f, float centralMassRatio = 0.1f);
    
    std::vector<std::unique_ptr<Body>> generateParticles(
        const glm::vec3& worldDimensions, 
        size_t numParticles
    ) override;

private:
    float m_armCount;      // Number of spiral arms
    float m_armTightness;  // How tightly wound the arms are
    float m_centralMassRatio; // Fraction of particles in central bulge
    
    /**
     * @brief Generate particles in the central bulge
     */
    void generateBulgeParticles(
        std::vector<std::unique_ptr<Body>>& particles,
        const glm::vec3& center,
        float radius,
        size_t count
    );
    
    /**
     * @brief Generate particles in spiral arms
     */
    void generateSpiralParticles(
        std::vector<std::unique_ptr<Body>>& particles,
        const glm::vec3& center,
        float minRadius,
        float maxRadius,
        size_t count
    );
    
    /**
     * @brief Calculate orbital velocity for stable galaxy rotation
     */
    glm::vec3 calculateOrbitalVelocity(
        const glm::vec3& position,
        const glm::vec3& center,
        float totalMass
    );
};

} // namespace nbody
