#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>

namespace nbody {

class Shader;

/**
 * @brief Bloom post-processing effect for enhanced particle rendering
 */
class BloomRenderer {
public:
    BloomRenderer(int width, int height);
    ~BloomRenderer();
    
    /**
     * @brief Initialize bloom rendering resources
     */
    bool Initialize();
    
    /**
     * @brief Begin bloom rendering (bind framebuffer)
     */
    void BeginRender();
    
    /**
     * @brief End bloom rendering and apply effect
     */
    void EndRender();
    
    /**
     * @brief Render the final composited image
     */
    void RenderFinal();
    
    /**
     * @brief Toggle bloom effect on/off
     */
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    
    /**
     * @brief Set bloom intensity
     */
    void SetIntensity(float intensity) { m_intensity = intensity; }
    float GetIntensity() const { return m_intensity; }
    
    /**
     * @brief Handle window resize
     */
    void Resize(int width, int height);

private:
    int m_width, m_height;
    bool m_enabled = true;
    float m_intensity = 1.0f;
    
    // Framebuffers
    GLuint m_sceneFramebuffer = 0;
    GLuint m_bloomFramebuffer = 0;
    GLuint m_pingpongFramebuffers[2] = {0, 0};
    
    // Textures
    GLuint m_sceneTexture = 0;          // Normal scene
    GLuint m_brightTexture = 0;         // Bright parts only
    GLuint m_pingpongTextures[2] = {0, 0}; // For blur passes
    
    // Renderbuffer for depth
    GLuint m_depthRenderbuffer = 0;
    
    // Shaders
    std::unique_ptr<Shader> m_bloomExtractShader;
    std::unique_ptr<Shader> m_blurShader;
    std::unique_ptr<Shader> m_combineShader;
    
    // Quad for full-screen rendering
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;
    
    /**
     * @brief Create framebuffers and textures
     */
    bool CreateFramebuffers();
    
    /**
     * @brief Create the full-screen quad
     */
    void CreateQuad();
    
    /**
     * @brief Extract bright parts from scene
     */
    void ExtractBrightParts();
    
    /**
     * @brief Apply Gaussian blur to bright parts
     */
    void BlurBrightParts();
    
    /**
     * @brief Combine original scene with blurred bright parts
     */
    void CombineScenes();
    
    /**
     * @brief Cleanup resources
     */
    void Cleanup();
};

} // namespace nbody
