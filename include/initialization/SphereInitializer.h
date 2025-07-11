#pragma once

#include "ParticleSystemInitializer.h"

namespace nbody {

enum class SphereDistribution {
    SURFACE_ONLY,    // Particles only on the surface
    UNIFORM_VOLUME,  // Uniformly distributed throughout volume
    OUTWARD_EXPLOSION, // Particles with outward velocities
    INWARD_IMPLOSION,  // Particles with inward velocities
    TANGENTIAL_ORBIT   // Particles with tangential velocities
};

/**
 * @brief Initializes particles in various spherical formations
 */
class SphereInitializer : public ParticleSystemInitializer {
public:
    SphereInitializer(SphereDistribution distribution = SphereDistribution::UNIFORM_VOLUME);
    
    std::vector<std::unique_ptr<Body>> generateParticles(
        const glm::vec3& worldDimensions, 
        size_t numParticles
    ) override;

private:
    SphereDistribution m_distribution;
    
    /**
     * @brief Generate a point on sphere surface
     */
    glm::vec3 generateSpherePoint(float radius) const;
    
    /**
     * @brief Generate a point inside sphere volume
     */
    glm::vec3 generateVolumePoint(float radius) const;
    
    /**
     * @brief Calculate velocity based on distribution type
     */
    glm::vec3 calculateVelocity(
        const glm::vec3& position,
        const glm::vec3& center,
        SphereDistribution distribution
    ) const;
};

} // namespace nbody
