#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "CircularTrail.h"

namespace nbody {

/**
 * @brief Represents a celestial body in the N-body simulation
 */
class Body {
public:
    Body(const glm::vec2& position, const glm::vec2& velocity, float mass, 
         const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f));
    ~Body() = default;
    
    // Explicitly delete copy operations to prevent expensive copies
    Body(const Body&) = delete;
    Body& operator=(const Body&) = delete;
    
    // Allow move operations
    Body(Body&&) = default; 
    Body& operator=(Body&&) = default;

    // Getters
    const glm::vec2& GetPosition() const { return m_position; }
    const glm::vec2& GetVelocity() const { return m_velocity; }
    const glm::vec2& GetAcceleration() const { return m_acceleration; }
    const glm::vec2& GetForce() const { return m_force; }
    float GetMass() const { return m_mass; }
    float GetRadius() const { return m_radius; }
    const glm::vec3& GetColor() const { return m_color; }
    const CircularTrail& GetTrail() const { return m_trail; }
    
    bool IsSelected() const { return m_selected; }
    bool IsBeingDragged() const { return m_beingDragged; }
    bool IsFixed() const { return m_fixed; }
    
    // Setters
    void SetPosition(const glm::vec2& position);
    void SetVelocity(const glm::vec2& velocity);
    void SetAcceleration(const glm::vec2& acceleration);
    void SetForce(const glm::vec2& force);
    void SetMass(float mass);
    void SetColor(const glm::vec3& color);
    void SetSelected(bool selected);
    void SetBeingDragged(bool dragged);
    void SetFixed(bool fixed);
    void SetDensity(float density);
    
    // Physics
    void ApplyForce(const glm::vec2& force);
    void ClearForce();
    void Update(float deltaTime);
    void UpdateRadius();
    
    // Trail management
    void AddTrailPoint();
    void ClearTrail();
    void SetMaxTrailLength(int length);
    int GetMaxTrailLength() const { return m_maxTrailLength; }
    
    // Collision detection
    bool IsColliding(const Body& other) const;
    float GetCollisionRadius() const;
    
    // Utility
    float GetKineticEnergy() const;
    float GetSpeed() const;
    glm::vec2 GetMomentum() const;
    
private:
    // Physical properties
    glm::vec2 m_position{0.0f};
    glm::vec2 m_velocity{0.0f};
    glm::vec2 m_acceleration{0.0f};
    glm::vec2 m_force{0.0f};
    float m_mass = 1.0f;
    float m_radius = 1.0f;
    float m_density = 1.0f;
    
    // Visual properties
    glm::vec3 m_color{1.0f, 1.0f, 1.0f};
    CircularTrail m_trail;
    int m_maxTrailLength = 100;
    
    // State flags
    bool m_selected = false;
    bool m_beingDragged = false;
    bool m_fixed = false;
    
    // Constants
    static constexpr float DEFAULT_DENSITY = 0.1f;  // Lower density = larger bodies for same mass
    static constexpr float MIN_RADIUS = 2.0f;       // Much larger minimum radius
    static constexpr float MAX_RADIUS = 100.0f;     // Larger maximum radius
    
    void UpdateTrail();
};

/**
 * @brief Structure of Arrays layout for efficient physics calculations
 */
struct BodyArrays {
    std::vector<glm::vec2> positions;
    std::vector<glm::vec2> velocities;
    std::vector<glm::vec2> accelerations;
    std::vector<glm::vec2> forces;
    std::vector<float> masses;
    std::vector<float> radii;
    std::vector<glm::vec3> colors;
    std::vector<bool> fixed;
    
    size_t size() const { return positions.size(); }
    
    void reserve(size_t capacity) {
        positions.reserve(capacity);
        velocities.reserve(capacity);
        accelerations.reserve(capacity);
        forces.reserve(capacity);
        masses.reserve(capacity);
        radii.reserve(capacity);
        colors.reserve(capacity);
        fixed.reserve(capacity);
    }
    
    void clear() {
        positions.clear();
        velocities.clear();
        accelerations.clear();
        forces.clear();
        masses.clear();
        radii.clear();
        colors.clear();
        fixed.clear();
    }
    
    void push_back(const Body& body) {
        positions.push_back(body.GetPosition());
        velocities.push_back(body.GetVelocity());
        accelerations.push_back(body.GetAcceleration());
        forces.push_back(glm::vec2(0.0f));
        masses.push_back(body.GetMass());
        radii.push_back(body.GetRadius());
        colors.push_back(body.GetColor());
        fixed.push_back(body.IsFixed());
    }
    
    void erase(size_t index) {
        positions.erase(positions.begin() + index);
        velocities.erase(velocities.begin() + index);
        accelerations.erase(accelerations.begin() + index);
        forces.erase(forces.begin() + index);
        masses.erase(masses.begin() + index);
        radii.erase(radii.begin() + index);
        colors.erase(colors.begin() + index);
        fixed.erase(fixed.begin() + index);
    }
};

} // namespace nbody
