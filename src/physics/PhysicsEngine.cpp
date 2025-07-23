#include "physics/PhysicsEngine.h"
#include "physics/BarnesHut.h"
#include "core/Body.h"
// #include "rendering/ComputeShader.h"  // TODO: Enable when ComputeShader is ready
#include <GL/glew.h>
#include <omp.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <execution>
#include <numeric>
#include <functional>
#include <chrono>

namespace nbody {

// Physics constants
static constexpr float MIN_DISTANCE = 1.0f;  // Minimum distance to prevent singularities
static constexpr float MAX_FORCE = 10000.0f; // Maximum force to prevent instability

PhysicsEngine::PhysicsEngine() {
    m_bodyArrays = std::make_unique<BodyArrays>();
    m_barnesHutTree = std::make_unique<BarnesHutTree>(); // Already in nbody namespace
    
    // Configure OpenMP for maximum parallelization
    int numThreads = omp_get_max_threads();
    omp_set_num_threads(numThreads);
    
    #ifdef _DEBUG
    std::cout << "Physics Engine: Using " << numThreads << " threads for parallel computation" << std::endl;
    #endif
}

PhysicsEngine::~PhysicsEngine() {
    // CleanupGPUBuffers();  // Disabled temporarily
}

bool PhysicsEngine::Initialize() {
    // Check for GPU compute shader support
    GLint workGroupSizes[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workGroupSizes[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workGroupSizes[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workGroupSizes[2]);
    
    // Check if compute shaders are supported (OpenGL 4.3+)
    
    // Check compute shader support
    GLint maxComputeWorkGroupInvocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxComputeWorkGroupInvocations);
    
    // Enable GPU if compute shaders are supported and we have reasonable limits
    m_gpuAvailable = (maxComputeWorkGroupInvocations > 0 && workGroupSizes[0] >= 64);
    
    #ifdef _DEBUG
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;
    std::cout << "Physics Engine initialized" << std::endl;
    std::cout << "Max compute work group size: " << workGroupSizes[0] << "x" << workGroupSizes[1] << "x" << workGroupSizes[2] << std::endl;
    std::cout << "Max work group invocations: " << maxComputeWorkGroupInvocations << std::endl;
    std::cout << "GPU Acceleration: " << (m_gpuAvailable ? "Available" : "Not Available") << std::endl;
    #endif
    
    // Initialize GPU resources if available
    if (m_gpuAvailable) {
        #ifdef _DEBUG
        std::cout << "Setting up GPU compute shaders..." << std::endl;
        #endif
    }
    
    return true;
}

void PhysicsEngine::Update(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) {
    if (bodies.empty()) return;
    
    StartTimer();
    
    // Apply time scale multiplier
    float scaledDeltaTime = deltaTime * m_config.timeScale;
    
    // Use adaptive time stepping if enabled
    float actualDeltaTime = scaledDeltaTime;
    if (m_config.adaptiveTimeStep) {
        actualDeltaTime = CalculateAdaptiveTimeStep(bodies) * m_config.timeScale;
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
    
    // Clear all forces
    for (auto& body : bodies) {
        body->ClearForce();
    }
    
    // Choose calculation method based on body count and settings
    if (m_config.useGPU && m_gpuAvailable) {
        CalculateForcesGPU(bodies);
        m_stats.method = "GPU";
    } else if (m_config.useBarnesHut && bodies.size() > m_config.maxBodiesForDirect) {
        CalculateForcesBarnesHut(bodies);
        m_stats.method = "Barnes-Hut";
    } else if (bodies.size() > 100) {
        // Use spatially optimized method for medium-large simulations
        CalculateForcesSpatiallyOptimized(bodies);
        m_stats.method = "Spatial-Optimized";
    } else if (bodies.size() > 50) {
        // Use block-optimized method for medium simulations
        CalculateForcesOptimized(bodies);
        m_stats.method = "Block-Optimized";
    } else {
        // Use direct method for small simulations
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
    
    // Parallel force calculation for better performance
    #pragma omp parallel for schedule(dynamic) shared(bodies)
    for (int i = 0; i < static_cast<int>(bodies.size()); ++i) {
        auto& bodyA = bodies[i];
        if (bodyA->IsFixed()) continue;
        
        glm::vec2 totalForce(0.0f);
        int localForceCalculations = 0;
        
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (static_cast<size_t>(i) == j) continue;
            
            auto& bodyB = bodies[j];
            
            // Use shared gravitational force calculation
            glm::vec2 force = CalculateGravitationalForce(
                bodyA->GetPosition(),
                bodyB->GetPosition(), bodyB->GetMass(),
                G, softening
            );
            
            // Cap maximum force magnitude to prevent instability
            float forceMagnitude = glm::length(force);
            if (forceMagnitude > MAX_FORCE) {
                force = (force / forceMagnitude) * MAX_FORCE;
            }
            
            totalForce += force;
            localForceCalculations++;
        }
        
        // Apply total force to body
        bodyA->ApplyForce(totalForce);
        
        #pragma omp atomic
        m_stats.forceCalculations += localForceCalculations;
    }
}

void PhysicsEngine::CalculateForcesBarnesHut(std::vector<std::unique_ptr<Body>>& bodies) {
    auto start = std::chrono::high_resolution_clock::now();
    
    #ifdef _DEBUG
    static int debugFrameCount = 0;
    if (++debugFrameCount % 300 == 0) { // Only print every 300 frames
        std::cout << "Using Barnes-Hut for " << bodies.size() << " bodies" << std::endl;
    }
    #endif
    
    // Build Barnes-Hut tree
    m_barnesHutTree->BuildTree(bodies);
    
    auto buildEnd = std::chrono::high_resolution_clock::now();
    m_stats.barnesHutTime = std::chrono::duration<double, std::milli>(buildEnd - start).count();
    
    const float G = m_config.gravitationalConstant;
    const float theta = m_config.barnesHutTheta;
    
    // Reset force calculation counter before starting
    m_barnesHutTree->ResetForceCalculations();
    
    std::cout << "Using G=" << G << ", theta=" << theta << std::endl;
    
    // Calculate forces using Barnes-Hut approximation
    // Use parallel execution for better performance with many bodies
    #pragma omp parallel for schedule(dynamic, 32)
    for (int i = 0; i < static_cast<int>(bodies.size()); ++i) {
        auto& body = bodies[i];
        if (body->IsFixed()) continue;
        
        // Add debugging for the first few bodies
        if (i < 20) {
            std::cout << "Force calc for body at (" << body->GetPosition().x << "," 
                     << body->GetPosition().y << "), body mass=" << body->GetMass() 
                     << ", tree root mass=" << m_barnesHutTree->GetRoot()->totalMass 
                     << ", tree center=(" << m_barnesHutTree->GetRoot()->centerOfMass.x 
                     << "," << m_barnesHutTree->GetRoot()->centerOfMass.y << ")" << std::endl;
        }
        
        glm::vec2 force = m_barnesHutTree->CalculateForce(*body, theta, G);
        body->ApplyForce(force);
    }
    
    auto treeStats = m_barnesHutTree->GetStats();
    m_stats.forceCalculations = treeStats.forceCalculations;
    std::cout << "Total force calculations: " << m_stats.forceCalculations << std::endl;
}

void PhysicsEngine::CalculateForcesGPU(std::vector<std::unique_ptr<Body>>& bodies) {
    // GPU implementation temporarily disabled - need to build ComputeShader first
    CalculateForcesDirect(bodies);
    m_stats.method = "Direct (GPU disabled)";
}

void PhysicsEngine::IntegrateMotion(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Use leapfrog integration for better stability
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
    // Leapfrog integration for better stability
    const float dtDividedBy2 = deltaTime * 0.5f;
    const float damping = m_config.dampingFactor;
    const float maxVelocity = 500.0f; // Maximum velocity to prevent instability
    
    for (auto& body : bodies) {
        if (body->IsFixed() || body->IsBeingDragged()) continue;
        
        // Get current state
        glm::vec2 position = body->GetPosition();
        glm::vec2 velocity = body->GetVelocity() * damping;
        glm::vec2 force = body->GetForce();
        
        // Calculate acceleration from force: F = ma -> a = F/m
        glm::vec2 acceleration = force / body->GetMass();
        
        // Step 1: Compute velocity at half timestep (i + 1/2)
        velocity += acceleration * dtDividedBy2;
        
        // Step 2: Compute next position (i+1) using half-step velocity
        position += velocity * deltaTime;
        
        // Step 3: Update acceleration at new position (will be done in next force calculation)
        // This is where the next force calculation happens in the main loop
        
        // Step 4: Complete velocity update for next timestep (i+1)
        velocity += acceleration * dtDividedBy2;
        
        // Apply velocity limits for stability
        float velMagnitude = glm::length(velocity);
        if (velMagnitude > maxVelocity) {
            velocity = glm::normalize(velocity) * maxVelocity;
        }
        
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

void PhysicsEngine::CalculateForcesOptimized(std::vector<std::unique_ptr<Body>>& bodies) {
    // Optimized force calculation with better memory access patterns
    const float G = m_config.gravitationalConstant;
    const float softeningSq = m_config.softeningLength * m_config.softeningLength;
    const size_t bodyCount = bodies.size();
    
    // Block-based calculation to improve cache locality
    constexpr size_t BLOCK_SIZE = 32; // Smaller blocks for better load balancing
    
    m_stats.forceCalculations = 0;
    
    // Clear all forces first
    for (auto& body : bodies) {
        body->ClearForce();
    }
    
    // Calculate number of blocks
    const size_t numBlocks = (bodyCount + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    // Parallel block-based force calculation for better cache performance
    #pragma omp parallel for schedule(dynamic) shared(bodies)
    for (int blockIdx = 0; blockIdx < static_cast<int>(numBlocks); ++blockIdx) {
        size_t blockStart = blockIdx * BLOCK_SIZE;
        size_t blockEnd = std::min(blockStart + BLOCK_SIZE, bodyCount);
        int localForceCalculations = 0;
        
        // Calculate forces for current block against all other bodies
        for (size_t i = blockStart; i < blockEnd; ++i) {
            auto& bodyA = bodies[i];
            if (bodyA->IsFixed()) continue;
            
            glm::vec2 totalForce(0.0f);
            glm::vec2 posA = bodyA->GetPosition();
            
            // Inner loop: calculate force from all other bodies
            for (size_t j = 0; j < bodyCount; ++j) {
                if (i == j) continue;
                
                auto& bodyB = bodies[j];
                glm::vec2 posB = bodyB->GetPosition();
                
                // Vector calculation
                glm::vec2 vector_i_j = posB - posA;
                float distanceSquared = glm::dot(vector_i_j, vector_i_j);
                
                // Distance calculation with numerical stability
                float distance_i_j = std::pow(distanceSquared + softeningSq, 1.5f);
                
                if (distance_i_j > 1e-10f) {
                    // Accumulate force without mass of bodyA (cancelled in acceleration)
                    float forceMagnitude = (G * bodyB->GetMass()) / distance_i_j;
                    totalForce += forceMagnitude * vector_i_j;
                    localForceCalculations++;
                }
            }
            
            // Apply accumulated force to body
            bodyA->ApplyForce(totalForce);
        }
        
        #pragma omp atomic
        m_stats.forceCalculations += localForceCalculations;
    }
}

void PhysicsEngine::CalculateForcesSpatiallyOptimized(std::vector<std::unique_ptr<Body>>& bodies) {
    // Spatial optimization using sorting for cache locality
    const float G = m_config.gravitationalConstant;
    const float softeningSq = m_config.softeningLength * m_config.softeningLength;
    
    // Create sorted indices based on spatial position (simplified Z-order curve)
    std::vector<size_t> sortedIndices(bodies.size());
    std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
    
    // Sort by spatial hash (simplified Morton codes)
    std::sort(sortedIndices.begin(), sortedIndices.end(), [&bodies](size_t a, size_t b) {
        auto posA = bodies[a]->GetPosition();
        auto posB = bodies[b]->GetPosition();
        
        // Simple spatial hash: interleave x and y bits (simplified Morton code)
        auto hash = [](float x, float y) -> uint32_t {
            uint32_t ix = static_cast<uint32_t>((x + 1000.0f) * 100.0f); // Offset and scale
            uint32_t iy = static_cast<uint32_t>((y + 1000.0f) * 100.0f);
            
            // Interleave bits (simplified Morton encoding)
            uint32_t result = 0;
            for (int i = 0; i < 16; i++) {
                result |= ((ix & (1 << i)) << i) | ((iy & (1 << i)) << (i + 1));
            }
            return result;
        };
        
        return hash(posA.x, posA.y) < hash(posB.x, posB.y);
    });
    
    m_stats.forceCalculations = 0;
    
    // Clear all forces
    for (auto& body : bodies) {
        body->ClearForce();
    }
    
    // Calculate forces using spatially sorted order for better cache locality
    #pragma omp parallel for schedule(dynamic) shared(bodies, sortedIndices)
    for (int idx_i = 0; idx_i < static_cast<int>(sortedIndices.size()); ++idx_i) {
        size_t i = sortedIndices[idx_i];
        auto& bodyA = bodies[i];
        if (bodyA->IsFixed()) continue;
        
        glm::vec2 totalForce(0.0f);
        glm::vec2 posA = bodyA->GetPosition();
        int localForceCalculations = 0;
        
        for (size_t idx_j = 0; idx_j < sortedIndices.size(); ++idx_j) {
            if (static_cast<size_t>(idx_i) == idx_j) continue;
            
            size_t j = sortedIndices[idx_j];
            auto& bodyB = bodies[j];
            glm::vec2 posB = bodyB->GetPosition();
            
            // Force calculation
            glm::vec2 vector_i_j = posB - posA;
            float distanceSquared = glm::dot(vector_i_j, vector_i_j);
            float distance_i_j = std::pow(distanceSquared + softeningSq, 1.5f);
            
            if (distance_i_j > 1e-10f) {
                float forceMagnitude = (G * bodyB->GetMass()) / distance_i_j;
                totalForce += forceMagnitude * vector_i_j;
                localForceCalculations++;
            }
        }
        
        bodyA->ApplyForce(totalForce);
        
        #pragma omp atomic
        m_stats.forceCalculations += localForceCalculations;
    }
}

void PhysicsEngine::BenchmarkMethods(std::vector<std::unique_ptr<Body>>& bodies) {
    // Performance benchmarking of different force calculation methods
    if (bodies.size() < 10) return; // Skip for very small simulations
    
    const int numIterations = 5;
    std::cout << "\n=== Physics Method Benchmark (Body Count: " << bodies.size() << ") ===" << std::endl;
    
    // Backup current forces
    std::vector<glm::vec2> originalForces;
    for (const auto& body : bodies) {
        originalForces.push_back(body->GetForce());
    }
    
    // Test Direct Method
    auto testMethod = [&](const std::string& name, std::function<void()> method) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < numIterations; ++i) {
            method();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double avgTime = std::chrono::duration<double, std::milli>(end - start).count() / numIterations;
        std::cout << name << ": " << avgTime << "ms (avg)" << std::endl;
    };
    
    testMethod("Direct           ", [&]() { CalculateForcesDirect(bodies); });
    testMethod("Block-Optimized  ", [&]() { CalculateForcesOptimized(bodies); });
    testMethod("Spatial-Optimized", [&]() { CalculateForcesSpatiallyOptimized(bodies); });
    
    // Restore original forces
    for (size_t i = 0; i < bodies.size(); ++i) {
        bodies[i]->SetForce(originalForces[i]);
    }
    
    std::cout << "=== Benchmark Complete ===" << std::endl;
}

// GPU Helper Methods Implementation (Temporarily disabled)
/*
bool PhysicsEngine::InitializeGPUBuffers(size_t particleCount) {
    if (particleCount <= m_maxGPUParticles && m_positionBuffer != 0) {
        return true; // Already initialized with sufficient capacity
    }
    
    // Clean up existing buffers
    CleanupGPUBuffers();
    
    // Create compute shaders if not already done
    if (!m_forceComputeShader) {
        m_forceComputeShader = std::make_unique<ComputeShader>();
        if (!m_forceComputeShader->LoadFromFile("shaders/compute/force_calculation.comp")) {
            std::cerr << "Failed to load force calculation compute shader" << std::endl;
            return false;
        }
    }
    
    // Create GPU buffers
    m_maxGPUParticles = particleCount * 2; // Allow some growth
    
    // Position buffer (vec4)
    glGenBuffers(1, &m_positionBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_positionBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxGPUParticles * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_positionBuffer);
    
    // Velocity buffer (vec4)  
    glGenBuffers(1, &m_velocityBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_velocityBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxGPUParticles * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_velocityBuffer);
    
    // Mass buffer (float)
    glGenBuffers(1, &m_massBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_massBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxGPUParticles * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_massBuffer);
    
    // Force buffer (vec4)
    glGenBuffers(1, &m_forceBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_forceBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxGPUParticles * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_forceBuffer);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    return true;
}

void PhysicsEngine::UploadDataToGPU(const std::vector<std::unique_ptr<Body>>& bodies) {
    if (m_positionBuffer == 0) return;
    
    std::vector<float> positions, velocities, masses;
    positions.reserve(bodies.size() * 4);
    velocities.reserve(bodies.size() * 4);
    masses.reserve(bodies.size());
    
    for (const auto& body : bodies) {
        const auto& pos = body->GetPosition();
        const auto& vel = body->GetVelocity();
        
        positions.push_back(pos.x);
        positions.push_back(pos.y);
        positions.push_back(0.0f); // z component
        positions.push_back(1.0f); // w component
        
        velocities.push_back(vel.x);
        velocities.push_back(vel.y);
        velocities.push_back(0.0f); // z component
        velocities.push_back(0.0f); // w component
        
        masses.push_back(body->GetMass());
    }
    
    // Upload to GPU
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_positionBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, positions.size() * sizeof(float), positions.data());
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_velocityBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, velocities.size() * sizeof(float), velocities.data());
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_massBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, masses.size() * sizeof(float), masses.data());
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void PhysicsEngine::DownloadDataFromGPU(std::vector<std::unique_ptr<Body>>& bodies) {
    if (m_forceBuffer == 0) return;
    
    std::vector<float> forces(bodies.size() * 4);
    
    // Download forces from GPU
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_forceBuffer);
    float* forceData = (float*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    
    if (forceData) {
        std::copy(forceData, forceData + forces.size(), forces.begin());
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        
        // Apply forces to bodies
        for (size_t i = 0; i < bodies.size(); ++i) {
            glm::vec2 force(forces[i * 4], forces[i * 4 + 1]);
            bodies[i]->ApplyForce(force);
        }
    }
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void PhysicsEngine::CleanupGPUBuffers() {
    if (m_positionBuffer != 0) {
        glDeleteBuffers(1, &m_positionBuffer);
        m_positionBuffer = 0;
    }
    if (m_velocityBuffer != 0) {
        glDeleteBuffers(1, &m_velocityBuffer);
        m_velocityBuffer = 0;
    }
    if (m_massBuffer != 0) {
        glDeleteBuffers(1, &m_massBuffer);
        m_massBuffer = 0;
    }
    if (m_forceBuffer != 0) {
        glDeleteBuffers(1, &m_forceBuffer);
        m_forceBuffer = 0;
    }
    m_maxGPUParticles = 0;
}
*/

glm::vec2 PhysicsEngine::CalculateGravitationalForce(
    const glm::vec2& positionA,
    const glm::vec2& positionB, float massB,
    float G, float softeningLength
) {
    // Calculate direction vector from A to B (force direction on A)
    glm::vec2 direction = positionB - positionA;
    
    // Calculate distance squared
    float distanceSquared = glm::dot(direction, direction);
    
    // Apply softening to prevent singularities
    float softenedDistanceSquared = distanceSquared + softeningLength * softeningLength;
    
    // Calculate force magnitude: F = G * mB / r² (acceleration on A)
    float forceMagnitude = G * massB / softenedDistanceSquared;
    
    // Normalize direction and apply magnitude
    if (distanceSquared > 1e-10f) {
        float invDistance = 1.0f / std::sqrt(distanceSquared);
        return forceMagnitude * direction * invDistance;
    }
    
    return glm::vec2(0.0f);
}

} // namespace nbody
