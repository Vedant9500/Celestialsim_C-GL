#include "initialization/GalaxyInitializer.h"
#include <random>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace nbody {

GalaxyInitializer::GalaxyInitializer(float armCount, float armTightness, float centralMassRatio)
    : m_armCount(armCount)
    , m_armTightness(armTightness)
    , m_centralMassRatio(centralMassRatio) {
}

std::vector<std::unique_ptr<Body>> GalaxyInitializer::generateParticles(
    const glm::vec3& worldDimensions, 
    size_t numParticles
) {
    std::vector<std::unique_ptr<Body>> particles;
    particles.reserve(numParticles);
    
    glm::vec3 center = glm::vec3(0.0f); // Center at origin
    float galaxyRadius = std::min({worldDimensions.x, worldDimensions.y, worldDimensions.z}) * 0.4f;
    float bulgeRadius = galaxyRadius * 0.2f;
    
    // Calculate particle counts
    auto bulgeParticles = static_cast<size_t>(numParticles * m_centralMassRatio);
    size_t spiralParticles = numParticles - bulgeParticles;
    
    // Generate bulge particles
    generateBulgeParticles(particles, center, bulgeRadius, bulgeParticles);
    
    // Generate spiral arm particles
    generateSpiralParticles(particles, center, bulgeRadius, galaxyRadius, spiralParticles);
    
    return particles;
}

void GalaxyInitializer::generateBulgeParticles(
    std::vector<std::unique_ptr<Body>>& particles,
    const glm::vec3& center,
    float radius,
    size_t count
) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);
    
    for (size_t i = 0; i < count; ++i) {
        // Generate random point in sphere
        glm::vec3 point;
        float lengthSquared;
        do {
            point = glm::vec3(dist(gen), dist(gen), dist(gen));
            lengthSquared = glm::dot(point, point);
        } while (lengthSquared > 1.0f);
        
        // Scale to bulge radius with some randomness
        float r = std::cbrt(radiusDist(gen)) * radius;
        glm::vec3 position = center + glm::normalize(point) * r;
        
        // Low velocity for bulge particles
        glm::vec3 velocity = randomVelocity(0.0f, 0.5f);
        float mass = randomMass(0.8f, 1.2f);
        
        auto body = std::make_unique<Body>(
            glm::vec2(position.x, position.y),
            glm::vec2(velocity.x, velocity.y),
            mass
        );
        
        particles.push_back(std::move(body));
    }
}

void GalaxyInitializer::generateSpiralParticles(
    std::vector<std::unique_ptr<Body>>& particles,
    const glm::vec3& center,
    float minRadius,
    float maxRadius,
    size_t count
) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> radiusDist(minRadius, maxRadius);
    std::uniform_real_distribution<float> armDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> noiseDist(-0.1f, 0.1f);
    
    for (size_t i = 0; i < count; ++i) {
        // Choose spiral arm
        float armIndex = std::floor(armDist(gen) * m_armCount);
        float armOffset = (2.0f * glm::pi<float>() * armIndex) / m_armCount;
        
        // Generate radius
        float r = radiusDist(gen);
        
        // Calculate spiral angle
        float spiralAngle = m_armTightness * std::log(r / minRadius) + armOffset;
        
        // Add some noise to make it more natural
        spiralAngle += noiseDist(gen);
        r += noiseDist(gen) * 0.1f * r;
        
        // Convert to cartesian coordinates
        glm::vec3 position = center + glm::vec3(
            r * std::cos(spiralAngle),
            r * std::sin(spiralAngle),
            noiseDist(gen) * 0.05f * maxRadius  // Small z variation
        );
        
        // Calculate orbital velocity for stable rotation
        float totalMass = count * 1.0f; // Approximate total mass
        glm::vec3 velocity = calculateOrbitalVelocity(position, center, totalMass);
        
        // Add some velocity dispersion
        velocity += randomVelocity(0.0f, 0.2f);
        
        float mass = randomMass(0.5f, 1.5f);
        
        auto body = std::make_unique<Body>(
            glm::vec2(position.x, position.y),
            glm::vec2(velocity.x, velocity.y),
            mass
        );
        
        particles.push_back(std::move(body));
    }
}

glm::vec3 GalaxyInitializer::calculateOrbitalVelocity(
    const glm::vec3& position,
    const glm::vec3& center,
    float totalMass
) {
    glm::vec3 r = position - center;
    float distance = glm::length(r);
    
    if (distance < 1e-6f) {
        return glm::vec3(0.0f);
    }
    
    // Simplified galaxy rotation curve
    // v = sqrt(GM/r) for Keplerian, but galaxies have flat rotation curves
    float G = 1.0f; // Gravitational constant (scaled for simulation)
    float enclosedMass = totalMass * (distance * distance) / (distance * distance + 1.0f);
    float orbitalSpeed = std::sqrt(G * enclosedMass / distance) * 0.5f; // Scale down for stability
    
    // Tangential velocity (perpendicular to radius vector)
    glm::vec3 tangent = glm::vec3(-r.y, r.x, 0.0f);
    if (glm::length(tangent) > 1e-6f) {
        tangent = glm::normalize(tangent);
    }
    
    return tangent * orbitalSpeed;
}

} // namespace nbody
