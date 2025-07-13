#pragma once

#include <glm/glm.hpp>

namespace nbody {

/**
 * @brief Simple 2D particle data container following REF architecture pattern
 * Optimized for 2D N-body simulation (not 3D like REF)
 * Separates physics data from rendering/UI concerns
 */
class Particle {
public:
    // Core 2D physics data
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 velocity{0.0f, 0.0f};
    glm::vec2 acceleration{0.0f, 0.0f};
    float mass{1.0f};
    
    // Default constructor
    Particle() = default;
    
    // 2D constructor 
    Particle(const glm::vec2& pos, const glm::vec2& vel, float m);
    
    // Full constructor with acceleration
    Particle(const glm::vec2& pos, const glm::vec2& vel, const glm::vec2& acc, float m);
    
    // Destructor
    ~Particle() = default;
    
    // Utility methods
    void SetVelocity(const glm::vec2& vel);
    void SetPosition(const glm::vec2& pos);
    void SetAcceleration(const glm::vec2& acc);
    
    // Direct access (already 2D)
    const glm::vec2& GetPosition() const { return position; }
    const glm::vec2& GetVelocity() const { return velocity; }
    const glm::vec2& GetAcceleration() const { return acceleration; }
};

} // namespace nbody
