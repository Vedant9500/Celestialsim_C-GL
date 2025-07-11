#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "core/Body.h"

namespace nbody {

/**
 * @brief Abstract base class for particle system initializers
 */
class ParticleSystemInitializer {
public:
    virtual ~ParticleSystemInitializer() = default;
    
    /**
     * @brief Generate particles according to the specific initialization pattern
     * @param worldDimensions The size of the simulation world
     * @param numParticles Number of particles to generate
     * @return Vector of initialized bodies
     */
    virtual std::vector<std::unique_ptr<Body>> generateParticles(
        const glm::vec3& worldDimensions, 
        size_t numParticles
    ) = 0;
    
protected:
    /**
     * @brief Generate a random position within world bounds
     */
    glm::vec3 randomPosition(const glm::vec3& worldDimensions) const;
    
    /**
     * @brief Generate a random velocity within specified range
     */
    glm::vec3 randomVelocity(float minSpeed, float maxSpeed) const;
    
    /**
     * @brief Generate a random mass within specified range
     */
    float randomMass(float minMass, float maxMass) const;
};

} // namespace nbody
