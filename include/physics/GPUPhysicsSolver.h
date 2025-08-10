#pragma once

#include "PhysicsSolver.h"
#include <GL/glew.h>
#include <memory>

namespace nbody {

class ComputeShader;

/**
 * @brief GPU-based physics solver using compute shaders
 */
class GPUPhysicsSolver : public PhysicsSolver {
public:
    GPUPhysicsSolver();
    ~GPUPhysicsSolver();
    
    bool Initialize();
    
    void SetGravitationalConstant(float G) { m_gravitationalConstant = G; }
    void SetSoftening(float softening) { m_softeningLength = softening; }
    
    void Update(std::vector<std::unique_ptr<Body>>& bodies, float deltaTime) override;
    std::string GetAlgorithmName() const override { return "GPU Compute Shader"; }
    bool UsesGPU() const override { return true; }

private:
    std::unique_ptr<ComputeShader> m_forceComputeShader;
    std::unique_ptr<ComputeShader> m_integrationShader;
    
    // GPU buffers
    GLuint m_positionBuffer = 0;
    GLuint m_velocityBuffer = 0;
    GLuint m_massBuffer = 0;
    GLuint m_forceBuffer = 0;
    
    // Physics parameters
    float m_gravitationalConstant = 1.0f;
    float m_softeningLength = 0.1f;
    
    size_t m_maxParticles = 0;
    
    /**
     * @brief Create and configure GPU buffers
     */
    bool CreateBuffers(size_t particleCount);
    
    /**
     * @brief Upload particle data to GPU
     */
    void UploadData(const std::vector<std::unique_ptr<Body>>& bodies);
    
    /**
     * @brief Download results from GPU
     */
    void DownloadData(std::vector<std::unique_ptr<Body>>& bodies);
    
    /**
     * @brief Cleanup GPU resources
     */
    void Cleanup();
};

} // namespace nbody
