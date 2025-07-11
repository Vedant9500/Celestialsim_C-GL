#pragma once

#include <vector>
#include <memory>
#include <string>

namespace nbody {

class Body;

enum class PhysicsAlgorithm {
    NAIVE_CPU,           // O(n²) brute force on CPU
    PARALLEL_CPU,        // O(n²) brute force with OpenMP
    GPU_COMPUTE,         // O(n²) brute force on GPU
    BARNES_HUT_CPU,      // O(n log n) Barnes-Hut on CPU
    BARNES_HUT_GPU       // O(n log n) Barnes-Hut on GPU
};

/**
 * @brief Abstract base class for physics solvers
 */
class PhysicsSolver {
public:
    virtual ~PhysicsSolver() = default;
    
    /**
     * @brief Update particle positions and velocities
     * @param bodies Vector of bodies to update
     * @param deltaTime Time step
     */
    virtual void Update(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) = 0;
    
    /**
     * @brief Get the algorithm name for display
     */
    virtual std::string GetAlgorithmName() const = 0;
    
    /**
     * @brief Get whether this solver uses GPU
     */
    virtual bool UsesGPU() const = 0;
    
    /**
     * @brief Set gravitational constant
     */
    void SetGravitationalConstant(float G) { m_G = G; }
    float GetGravitationalConstant() const { return m_G; }
    
    /**
     * @brief Set softening parameter to avoid singularities
     */
    void SetSoftening(float softening) { m_softening = softening; }
    float GetSoftening() const { return m_softening; }

protected:
    float m_G = 6.67430e-11f;  // Gravitational constant
    float m_softening = 0.1f;  // Softening parameter
};

/**
 * @brief Factory for creating physics solvers
 */
class PhysicsSolverFactory {
public:
    static std::unique_ptr<PhysicsSolver> Create(PhysicsAlgorithm algorithm);
    static const char* GetAlgorithmName(PhysicsAlgorithm algorithm);
    static std::vector<PhysicsAlgorithm> GetAvailableAlgorithms();
};

} // namespace nbody
