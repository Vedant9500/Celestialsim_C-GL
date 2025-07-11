#include "physics/PhysicsEngine.h"
#include "physics/BarnesHut.h"
#include "core/Body.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <execution>

namespace nbody {

// Physics constants
static constexpr float MIN_DISTANCE = 1.0f;  // Minimum distance to prevent singularities
static constexpr float MAX_FORCE = 10000.0f; // Maximum force to prevent instability

PhysicsEngine::PhysicsEngine() {
    m_bodyArrays = std::make_unique<BodyArrays>();
    m_barnesHutTree = std::make_unique<BarnesHutTree>();
}

PhysicsEngine::~PhysicsEngine() = default;

bool PhysicsEngine::Initialize() {
    // Check for GPU support
    // For now, we'll implement CPU-only physics
    m_gpuAvailable = false;
    
    std::cout << "Physics Engine initialized" << std::endl;
    std::cout << "GPU Acceleration: " << (m_gpuAvailable ? "Available" : "Not Available") << std::endl;
    
    return true;
}

void PhysicsEngine::Update(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) {
    if (bodies.empty()) return;
    
    StartTimer();
    
    // Use adaptive time stepping if enabled
    float actualDeltaTime = deltaTime;
    if (m_config.adaptiveTimeStep) {
        actualDeltaTime = CalculateAdaptiveTimeStep(bodies);
    }
    
    // Calculate forces
    CalculateForces(bodies);
    
    // Handle collisions
    if (m_config.enableCollisions) {
        HandleCollisions(bodies);
    }
    
    // Integrate motion
    IntegrateMotion(bodies, actualDeltaTime);
    
    // Update statistics
    m_stats.bodyCount = static_cast<int>(bodies.size());
    EndTimer(m_stats.totalTime);
}

void PhysicsEngine::CalculateForces(std::vector<std::unique_ptr<Body>>& bodies) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Clear all forces and accelerations
    for (auto& body : bodies) {
        body->ClearForce();
        body->SetAcceleration(glm::vec2(0.0f));
    }
    
    // Choose calculation method based on body count and settings
    if (m_config.useGPU && m_gpuAvailable) {
        CalculateForcesGPU(bodies);
        m_stats.method = "GPU";
    } else if (m_config.useBarnesHut && bodies.size() > m_config.maxBodiesForDirect) {
        CalculateForcesBarnesHut(bodies);
        m_stats.method = "Barnes-Hut";
    } else {
        CalculateForcesDirect(bodies);
        m_stats.method = "Direct";
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    m_stats.forceCalculationTime = std::chrono::duration<double, std::milli>(end - start).count();
}

void PhysicsEngine::CalculateForcesDirect(std::vector<std::unique_ptr<Body>>& bodies) {
    const float G = m_config.gravitationalConstant;
    const float softening = m_config.softeningLength;
    const float softeningSq = softening * softening;
    
    m_stats.forceCalculations = 0;
    
    // Sequential calculation for now (avoid parallel issues)
    for (size_t i = 0; i < bodies.size(); ++i) {
        auto& bodyA = bodies[i];
        if (bodyA->IsFixed()) continue;
        
        glm::vec2 totalAcceleration(0.0f);
        
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (i == j) continue;
            
            auto& bodyB = bodies[j];
            
            // Calculate direction vector from bodyA to bodyB
            glm::vec2 r = bodyB->GetPosition() - bodyA->GetPosition();
            float distanceSq = glm::dot(r, r) + softeningSq;
            
            if (distanceSq > MIN_DISTANCE * MIN_DISTANCE) {
                // Calculate gravitational acceleration (F/m = GMm/r^2 / m = GM/r^2)
                float distance = std::sqrt(distanceSq);
                float accelerationMagnitude = G * bodyB->GetMass() / distanceSq;
                
                // Cap maximum acceleration to prevent instability
                accelerationMagnitude = std::min(accelerationMagnitude, MAX_FORCE / bodyA->GetMass());
                
                glm::vec2 accelerationDirection = r / distance;
                totalAcceleration += accelerationMagnitude * accelerationDirection;
                
                m_stats.forceCalculations++;
            }
        }
        
        bodyA->SetAcceleration(totalAcceleration);
    }
}

void PhysicsEngine::CalculateForcesBarnesHut(std::vector<std::unique_ptr<Body>>& bodies) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Build Barnes-Hut tree
    m_barnesHutTree->BuildTree(bodies);
    
    auto buildEnd = std::chrono::high_resolution_clock::now();
    m_stats.barnesHutTime = std::chrono::duration<double, std::milli>(buildEnd - start).count();
    
    const float G = m_config.gravitationalConstant;
    const float theta = m_config.barnesHutTheta;
    
    // Calculate forces using Barnes-Hut approximation
    std::for_each(std::execution::par_unseq, bodies.begin(), bodies.end(),
        [&](std::unique_ptr<Body>& body) {
            if (body->IsFixed()) return;
            
            glm::vec2 force = m_barnesHutTree->CalculateForce(*body, theta, G);
            body->ApplyForce(force);
        });
    
    auto treeStats = m_barnesHutTree->GetStats();
    m_stats.forceCalculations = treeStats.forceCalculations;
}

void PhysicsEngine::CalculateForcesGPU(std::vector<std::unique_ptr<Body>>& bodies) {
    // GPU implementation would go here
    // For now, fallback to direct calculation
    CalculateForcesDirect(bodies);
    m_stats.method = "Direct (GPU fallback)";
}

void PhysicsEngine::IntegrateMotion(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Use leapfrog integration to match reference implementation
    IntegrateLeapfrog(bodies, deltaTime);
    
    auto end = std::chrono::high_resolution_clock::now();
    m_stats.integrationTime = std::chrono::duration<double, std::milli>(end - start).count();
}

void PhysicsEngine::IntegrateEuler(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) {
    for (auto& body : bodies) {
        body->Update(deltaTime);
    }
}

void PhysicsEngine::IntegrateLeapfrog(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) {
    // Leapfrog integration matching reference implementation
    const float dtDividedBy2 = deltaTime * 0.5f;
    const float damping = m_config.dampingFactor;
    
    for (auto& body : bodies) {
        if (body->IsFixed() || body->IsBeingDragged()) continue;
        
        // Get current state
        glm::vec2 position = body->GetPosition();
        glm::vec2 velocity = body->GetVelocity() * damping;
        glm::vec2 acceleration = body->GetAcceleration();
        
        // Compute velocity (i + 1/2)
        velocity += acceleration * dtDividedBy2;
        
        // Compute next position (i+1)
        position += velocity * deltaTime;
        
        // Update acceleration is already done in force calculation
        // Compute next velocity (i+1)
        velocity += acceleration * dtDividedBy2;
        
        // Update body state
        body->SetPosition(position);
        body->SetVelocity(velocity);
    }
}

void PhysicsEngine::IntegrateVerlet(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) {
    const float damping = m_config.dampingFactor;
    
    for (auto& body : bodies) {
        if (body->IsFixed() || body->IsBeingDragged()) continue;
        
        // Velocity Verlet integration
        glm::vec2 acceleration = body->GetAcceleration();
        glm::vec2 velocity = body->GetVelocity() * damping;
        glm::vec2 position = body->GetPosition();
        
        // Update position
        position += velocity * deltaTime + 0.5f * acceleration * deltaTime * deltaTime;
        
        // Update velocity
        velocity += acceleration * deltaTime;
        
        body->SetPosition(position);
        body->SetVelocity(velocity);
        body->Update(0.0f); // Update trail without physics
    }
}

void PhysicsEngine::HandleCollisions(std::vector<std::unique_ptr<Body>>& bodies) {
    auto start = std::chrono::high_resolution_clock::now();
    
    m_stats.collisions = 0;
    
    // Simple O(N²) collision detection
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            if (CheckCollision(*bodies[i], *bodies[j])) {
                ResolveCollision(*bodies[i], *bodies[j]);
                m_stats.collisions++;
            }
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    m_stats.collisionTime = std::chrono::duration<double, std::milli>(end - start).count();
}

bool PhysicsEngine::CheckCollision(const Body& a, const Body& b) const {
    return a.IsColliding(b);
}

void PhysicsEngine::ResolveCollision(Body& a, Body& b) {
    glm::vec2 delta = b.GetPosition() - a.GetPosition();
    float distance = glm::length(delta);
    float minDistance = a.GetRadius() + b.GetRadius();
    
    if (distance < minDistance && distance > 0.0f) {
        // Separate bodies
        glm::vec2 separation = delta * ((minDistance - distance) / distance) * 0.5f;
        
        if (!a.IsFixed() && !a.IsBeingDragged()) {
            a.SetPosition(a.GetPosition() - separation);
        }
        if (!b.IsFixed() && !b.IsBeingDragged()) {
            b.SetPosition(b.GetPosition() + separation);
        }
        
        // Calculate collision response
        glm::vec2 normal = glm::normalize(delta);
        glm::vec2 relativeVelocity = b.GetVelocity() - a.GetVelocity();
        float velocityAlongNormal = glm::dot(relativeVelocity, normal);
        
        if (velocityAlongNormal > 0) return; // Objects separating
        
        float restitution = m_config.restitution;
        float impulse = -(1 + restitution) * velocityAlongNormal;
        impulse /= (1.0f / a.GetMass() + 1.0f / b.GetMass());
        
        glm::vec2 impulseVector = impulse * normal;
        
        if (!a.IsFixed() && !a.IsBeingDragged()) {
            a.SetVelocity(a.GetVelocity() - impulseVector / a.GetMass());
        }
        if (!b.IsFixed() && !b.IsBeingDragged()) {
            b.SetVelocity(b.GetVelocity() + impulseVector / b.GetMass());
        }
    }
}

float PhysicsEngine::CalculateAdaptiveTimeStep(const std::vector<std::unique_ptr<Body>>& bodies) const {
    float maxAcceleration = 0.0f;
    
    for (const auto& body : bodies) {
        float acceleration = glm::length(body->GetAcceleration());
        maxAcceleration = std::max(maxAcceleration, acceleration);
    }
    
    if (maxAcceleration > 0.0f) {
        float adaptiveStep = std::sqrt(m_config.softeningLength / maxAcceleration);
        return std::clamp(adaptiveStep, m_config.minTimeStep, m_config.maxTimeStep);
    }
    
    return m_config.timeStep;
}

EnergyStats PhysicsEngine::CalculateEnergyStats(const std::vector<std::unique_ptr<Body>>& bodies) const {
    EnergyStats stats;
    
    // Calculate kinetic energy
    for (const auto& body : bodies) {
        stats.kinetic += body->GetKineticEnergy();
    }
    
    // Calculate potential energy
    const float G = m_config.gravitationalConstant;
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            glm::vec2 r = bodies[j]->GetPosition() - bodies[i]->GetPosition();
            float distance = glm::length(r);
            if (distance > MIN_DISTANCE) {
                stats.potential -= G * bodies[i]->GetMass() * bodies[j]->GetMass() / distance;
            }
        }
    }
    
    stats.total = stats.kinetic + stats.potential;
    
    return stats;
}

void PhysicsEngine::Reset() {
    m_stats = PhysicsStats();
}

void PhysicsEngine::StartTimer() {
    m_frameStart = std::chrono::high_resolution_clock::now();
}

void PhysicsEngine::EndTimer(double& timeAccumulator) {
    auto end = std::chrono::high_resolution_clock::now();
    timeAccumulator = std::chrono::duration<double, std::milli>(end - m_frameStart).count();
}

} // namespace nbody
