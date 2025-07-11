#include "initialization/ParticleSystemInitializer.h"
#include <random>
#include <chrono>
#include <glm/gtc/constants.hpp>

namespace nbody {

glm::vec3 ParticleSystemInitializer::randomPosition(const glm::vec3& worldDimensions) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_real_distribution<float> distX(-worldDimensions.x/2, worldDimensions.x/2);
    std::uniform_real_distribution<float> distY(-worldDimensions.y/2, worldDimensions.y/2);
    std::uniform_real_distribution<float> distZ(-worldDimensions.z/2, worldDimensions.z/2);
    
    return glm::vec3(distX(gen), distY(gen), distZ(gen));
}

glm::vec3 ParticleSystemInitializer::randomVelocity(float minSpeed, float maxSpeed) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_real_distribution<float> speedDist(minSpeed, maxSpeed);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * glm::pi<float>());
    std::uniform_real_distribution<float> thetaDist(-1.0f, 1.0f);
    
    float speed = speedDist(gen);
    float phi = angleDist(gen);
    float cosTheta = thetaDist(gen);
    float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
    
    return glm::vec3(
        speed * sinTheta * std::cos(phi),
        speed * sinTheta * std::sin(phi),
        speed * cosTheta
    );
}

float ParticleSystemInitializer::randomMass(float minMass, float maxMass) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_real_distribution<float> massDist(minMass, maxMass);
    return massDist(gen);
}

} // namespace nbody
