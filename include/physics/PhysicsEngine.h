#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>
#include <memory>
#include <chrono>
#include "physics/BarnesHut.h"

namespace nbody {

class Body;
class ComputeShader;
class GPUPhysicsSolver;
struct BodyArrays;

/**
 * @brief Performance statistics for the physics engine
 */
struct PhysicsStats {
    double totalTime = 0.0;
    double forceCalculationTime = 0.0;
    double integrationTime = 0.0;
    double collisionTime = 0.0;
    double barnesHutTime = 0.0;
    int bodyCount = 0;
    int forceCalculations = 0;
    int collisions = 0;
    std::string method = "Direct";
};

/**
 * @brief Energy statistics for conservation monitoring
 */
struct EnergyStats {
    double kinetic = 0.0;
    double potential = 0.0;
    double total = 0.0;
    double initial = 0.0;
    double error = 0.0;
};

/**
 * @brief Configuration for physics simulation
 */
struct PhysicsConfig {
    float gravitationalConstant = 1.0f;   // Standard: G = 1.0 for unit scaling
    float timeStep = 0.016f;              // 60 FPS target
    float timeScale = 1.0f;               // Speed multiplier
    float softeningLength = 0.1f;         // Small softening for accuracy
    float dampingFactor = 1.0f;           // No damping by default
    bool useBarnesHut = true;
    float barnesHutTheta = 0.7f;          // Higher value for better performance (~0.5-1.0)
    bool enableCollisions = true;
    float restitution = 0.8f;
    bool adaptiveTimeStep = false;
    float maxTimeStep = 0.033f;
    float minTimeStep = 0.001f;
    bool useGPU = false;
    int maxBodiesForDirect = 1000;
};

/**
 * @brief Main physics engine for N-body simulation
 */
class PhysicsEngine {
public:
    PhysicsEngine();
    ~PhysicsEngine();

    /**
     * @brief Initialize the physics engine
     * @return True if initialization successful
     */
    bool Initialize();

    /**
     * @brief Update physics simulation for one time step
     * @param bodies Vector of bodies to simulate
     * @param deltaTime Time step in seconds
     */
    void Update(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime);

    /**
     * @brief Calculate forces between all bodies
     * @param bodies Vector of bodies
     */
    void CalculateForces(std::vector<std::unique_ptr<Body>>& bodies);

    /**
     * @brief Integrate positions and velocities
     * @param bodies Vector of bodies
     * @param deltaTime Time step in seconds
     */
    void IntegrateMotion(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime);

    /**
     * @brief Handle collisions between bodies
     * @param bodies Vector of bodies
     */
    void HandleCollisions(std::vector<std::unique_ptr<Body>>& bodies);

    // Configuration
    void SetConfig(const PhysicsConfig& config) { m_config = config; }
    const PhysicsConfig& GetConfig() const { return m_config; }
    PhysicsConfig& GetMutableConfig() { return m_config; }
    
    void SetGravitationalConstant(float G) { m_config.gravitationalConstant = G; }
    void SetTimeStep(float dt) { m_config.timeStep = dt; }
    void SetBarnesHutTheta(float theta) { m_config.barnesHutTheta = theta; }
    void SetCollisionEnabled(bool enabled) { m_config.enableCollisions = enabled; }
    void SetRestitution(float restitution) { m_config.restitution = restitution; }
    void SetUseBarnesHut(bool use) { m_config.useBarnesHut = use; }
    void SetUseGPU(bool use) { m_config.useGPU = use; }
    
    // GPU availability
    bool IsGPUAvailable() const { return m_gpuAvailable; }
    
    // Statistics
    const PhysicsStats& GetStats() const { return m_stats; }
    EnergyStats CalculateEnergyStats(const std::vector<std::unique_ptr<Body>>& bodies) const;
    
    // Utility
    void Reset();
    
    // Performance and debugging
    void BenchmarkMethods(std::vector<std::unique_ptr<Body>>& bodies);  // Performance benchmarking
    
    // Barnes-Hut tree access for visualization
    const BarnesHutTree* GetBarnesHutTree() const { return m_barnesHutTree.get(); }
    
    // Shared force calculation utility
    static glm::vec2 CalculateGravitationalForce(
        const glm::vec2& positionA,
        const glm::vec2& positionB, float massB,
        float G, float softeningLength
    );
    
private:
    PhysicsConfig m_config;
    PhysicsStats m_stats;
    bool m_gpuAvailable = false;
    
    // Timing
    std::chrono::high_resolution_clock::time_point m_frameStart;
    
    // Structure of Arrays for efficient calculations
    std::unique_ptr<BodyArrays> m_bodyArrays;
    
    // Barnes-Hut tree for force approximation
    std::unique_ptr<BarnesHutTree> m_barnesHutTree;
    
    // GPU physics solver
    std::unique_ptr<GPUPhysicsSolver> m_gpuSolver;
    
    // Private methods
    void StartTimer();
    void EndTimer(double& timeAccumulator);
    
    // Force calculation methods
    void CalculateForcesDirect(std::vector<std::unique_ptr<Body>>& bodies);
    void CalculateForcesOptimized(std::vector<std::unique_ptr<Body>>& bodies);      // Block-based for cache efficiency
    void CalculateForcesSpatiallyOptimized(std::vector<std::unique_ptr<Body>>& bodies); // Spatial sorting optimization
    void CalculateForcesBarnesHut(std::vector<std::unique_ptr<Body>>& bodies);
    void CalculateForcesGPU(std::vector<std::unique_ptr<Body>>& bodies);
    
    // Integration methods
    void IntegrateEuler(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime);
    void IntegrateLeapfrog(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime);
    void IntegrateVerlet(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime);
    
    // Collision detection
    bool CheckCollision(const Body& a, const Body& b) const;
    void ResolveCollision(Body& a, Body& b);
    
    // Adaptive time stepping
    float CalculateAdaptiveTimeStep(const std::vector<std::unique_ptr<Body>>& bodies) const;
    
    // Utility
    void ConvertToArrays(const std::vector<std::unique_ptr<Body>>& bodies);
    void ConvertFromArrays(std::vector<std::unique_ptr<Body>>& bodies);
    
    // GPU helper methods (temporarily disabled)
    // bool InitializeGPUBuffers(size_t particleCount);
    // void UploadDataToGPU(const std::vector<std::unique_ptr<Body>>& bodies);
    // void DownloadDataFromGPU(std::vector<std::unique_ptr<Body>>& bodies);
    // void CleanupGPUBuffers();

    // Constants
    static constexpr float MIN_DISTANCE = 1e-6f;
    static constexpr float MAX_FORCE = 1e6f;
    static constexpr int COLLISION_GRID_SIZE = 64;
};

} // namespace nbody
