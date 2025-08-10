#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>

namespace nbody {

/**
 * @brief OpenGL Compute Shader wrapper for GPU-accelerated computations
 */
class ComputeShader {
public:
    ComputeShader();
    ~ComputeShader();

    // Non-copyable
    ComputeShader(const ComputeShader&) = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;
    
    // Movable
    ComputeShader(ComputeShader&& other) noexcept;
    ComputeShader& operator=(ComputeShader&& other) noexcept;

    /**
     * @brief Load and compile compute shader from file
     * @param shaderPath Path to the compute shader file
     * @return True if successful
     */
    bool LoadFromFile(const std::string& shaderPath);

    /**
     * @brief Load and compile compute shader from source code
     * @param shaderSource GLSL compute shader source code
     * @return True if successful
     */
    bool LoadFromSource(const std::string& shaderSource);

    /**
     * @brief Use this compute shader program
     */
    void Use() const;

    /**
     * @brief Dispatch compute work
     * @param numGroupsX Number of work groups in X dimension
     * @param numGroupsY Number of work groups in Y dimension
     * @param numGroupsZ Number of work groups in Z dimension
     */
    void Dispatch(GLuint numGroupsX, GLuint numGroupsY = 1, GLuint numGroupsZ = 1) const;

    /**
     * @brief Wait for all compute operations to complete
     */
    static void MemoryBarrier();

    /**
     * @brief Create a shader storage buffer object (SSBO)
     * @param data Initial data (can be nullptr)
     * @param size Size in bytes
     * @param usage GL usage hint (e.g., GL_DYNAMIC_DRAW)
     * @return Buffer handle
     */
    static GLuint CreateSSBO(const void* data, size_t size, GLenum usage = GL_DYNAMIC_DRAW);

    /**
     * @brief Bind SSBO to a binding point
     * @param buffer Buffer handle
     * @param bindingPoint Binding point index
     */
    static void BindSSBO(GLuint buffer, GLuint bindingPoint);

    /**
     * @brief Update SSBO data
     * @param buffer Buffer handle
     * @param offset Offset in bytes
     * @param size Size in bytes
     * @param data New data
     */
    static void UpdateSSBO(GLuint buffer, size_t offset, size_t size, const void* data);

    /**
     * @brief Read data from SSBO
     * @param buffer Buffer handle
     * @param offset Offset in bytes
     * @param size Size in bytes
     * @param data Output buffer
     */
    static void ReadSSBO(GLuint buffer, size_t offset, size_t size, void* data);

    /**
     * @brief Delete buffer
     * @param buffer Buffer handle
     */
    static void DeleteBuffer(GLuint buffer);

    // Uniform setters
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, float x, float y) const;
    void SetVec3(const std::string& name, float x, float y, float z) const;

    /**
     * @brief Check if compute shaders are supported
     * @return True if supported
     */
    static bool IsSupported();

    /**
     * @brief Get maximum work group sizes
     */
    static void GetMaxWorkGroupSizes(int& x, int& y, int& z);

    /**
     * @brief Get maximum work group invocations per group
     */
    static int GetMaxWorkGroupInvocations();

    bool IsValid() const { return m_programId != 0; }

private:
    GLuint m_programId = 0;
    GLuint m_shaderId = 0;

    bool CompileShader(const std::string& source);
    GLint GetUniformLocation(const std::string& name) const;
    void Cleanup();
};

} // namespace nbody
