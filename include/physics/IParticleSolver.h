#pragma once

#include "core/ParticleSystem.h"

namespace nbody {

/**
 * @brief Abstract base class for physics solvers following REF implementation pattern
 * Provides clean separation between physics algorithms and data
 */
class IParticleSolver {
public:
    virtual ~IParticleSolver() = default;
    
    // Pure virtual methods that concrete solvers must implement
    virtual void UpdateParticlePositions(ParticleSystem* particles) = 0;
    virtual float GetSquaredSoftening() const = 0;
    virtual bool UsesGPU() const = 0;
    virtual const char* GetName() const = 0;
    
    // Common interface
    virtual void SetTimeStep(float timeStep) { m_timeStep = timeStep; }
    virtual float GetTimeStep() const { return m_timeStep; }
    
    virtual void SetGravitationalConstant(float G) { m_gravitationalConstant = G; }
    virtual float GetGravitationalConstant() const { return m_gravitationalConstant; }

protected:
    float m_timeStep = 0.016f;           // 60 FPS default
    float m_gravitationalConstant = 10.0f; // Simulation units
    float m_squaredSoftening = 1.0f;     // Softening length squared
};

/**
 * @brief CPU Sequential N-body solver (O(N²) complexity)
 * Simple brute-force implementation for small particle counts
 */
class CPUSequentialSolver : public IParticleSolver {
public:
    CPUSequentialSolver(float timeStep, float squaredSoftening);
    
    void UpdateParticlePositions(ParticleSystem* particles) override;
    float GetSquaredSoftening() const override { return m_squaredSoftening; }
    bool UsesGPU() const override { return false; }
    const char* GetName() const override { return "CPU Sequential"; }

private:
    void ComputeGravityForce(ParticleSystem* particles, size_t particleId);
};

/**
 * @brief CPU Parallel N-body solver (O(N²) complexity with threading)
 * Parallelized brute-force implementation for medium particle counts
 */
class CPUParallelSolver : public IParticleSolver {
public:
    CPUParallelSolver(float timeStep, float squaredSoftening);
    
    void UpdateParticlePositions(ParticleSystem* particles) override;
    float GetSquaredSoftening() const override { return m_squaredSoftening; }
    bool UsesGPU() const override { return false; }
    const char* GetName() const override { return "CPU Parallel"; }

private:
    void ComputeGravityForces(ParticleSystem* particles);
};

} // namespace nbody
