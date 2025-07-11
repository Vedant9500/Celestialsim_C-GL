#include "core/Body.h"
#include <algorithm>
#include <cmath>

namespace nbody {

Body::Body(const glm::vec2& position, const glm::vec2& velocity, float mass, const glm::vec3& color)
    : m_position(position)
    , m_velocity(velocity)
    , m_mass(mass)
    , m_color(color)
    , m_density(DEFAULT_DENSITY)
{
    UpdateRadius();
    m_trail.reserve(m_maxTrailLength);
}

void Body::SetPosition(const glm::vec2& position) {
    m_position = position;
}

void Body::SetVelocity(const glm::vec2& velocity) {
    m_velocity = velocity;
}

void Body::SetAcceleration(const glm::vec2& acceleration) {
    m_acceleration = acceleration;
}

void Body::SetMass(float mass) {
    m_mass = std::max(0.1f, mass); // Ensure mass is never zero or negative
    UpdateRadius();
}

void Body::SetColor(const glm::vec3& color) {
    m_color = color;
}

void Body::SetSelected(bool selected) {
    m_selected = selected;
}

void Body::SetBeingDragged(bool dragged) {
    m_beingDragged = dragged;
}

void Body::SetFixed(bool fixed) {
    m_fixed = fixed;
}

void Body::ApplyForce(const glm::vec2& force) {
    m_force += force;
}

void Body::Update(float deltaTime) {
    if (m_fixed || m_beingDragged) {
        m_acceleration = glm::vec2(0.0f);
        m_force = glm::vec2(0.0f);
        return;
    }
    
    // Calculate acceleration from force
    m_acceleration = m_force / m_mass;
    
    // Integrate velocity and position (Verlet integration)
    m_velocity += m_acceleration * deltaTime;
    m_position += m_velocity * deltaTime;
    
    // Reset force for next frame
    m_force = glm::vec2(0.0f);
    
    // Update trail
    UpdateTrail();
}

void Body::UpdateRadius() {
    // Calculate radius based on mass and density
    // Volume = mass / density, for 2D circle: Area = π * r²
    // So r = sqrt(mass / (π * density))
    m_radius = std::sqrt(m_mass / (3.14159f * m_density));
    m_radius = std::clamp(m_radius, MIN_RADIUS, MAX_RADIUS);
}

void Body::AddTrailPoint() {
    m_trail.push_back(m_position);
    LimitTrailLength();
}

void Body::ClearTrail() {
    m_trail.clear();
}

void Body::SetMaxTrailLength(int length) {
    m_maxTrailLength = std::max(0, length);
    LimitTrailLength();
}

bool Body::IsColliding(const Body& other) const {
    float distance = glm::length(m_position - other.m_position);
    return distance <= (m_radius + other.m_radius);
}

float Body::GetCollisionRadius() const {
    return m_radius;
}

float Body::GetKineticEnergy() const {
    float speedSq = glm::dot(m_velocity, m_velocity);
    return 0.5f * m_mass * speedSq;
}

float Body::GetSpeed() const {
    return glm::length(m_velocity);
}

glm::vec2 Body::GetMomentum() const {
    return m_mass * m_velocity;
}

void Body::UpdateTrail() {
    // Add current position to trail every few frames to avoid too many points
    static int frameCounter = 0;
    frameCounter++;
    
    if (frameCounter % 5 == 0) { // Add point every 5 frames
        AddTrailPoint();
    }
}

void Body::LimitTrailLength() {
    while (static_cast<int>(m_trail.size()) > m_maxTrailLength) {
        m_trail.erase(m_trail.begin());
    }
}

} // namespace nbody
