#pragma once

#include <GL/glew.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace nbody {

/**
 * @brief Simple compute shader wrapper
 */
class ComputeShader {
public:
    ComputeShader();
    ~ComputeShader();
    
    bool LoadFromFile(const std::string& computePath);
    bool LoadFromString(const std::string& source);
    
    void Use() const;
    void Dispatch(unsigned int numGroupsX, unsigned int numGroupsY = 1, unsigned int numGroupsZ = 1) const;
    void MemoryBarrier() const;
    
    // Uniform setters
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetBool(const std::string& name, bool value) const;
    
    GLuint GetID() const { return m_programID; }
    bool IsValid() const { return m_programID != 0; }

private:
    GLuint m_programID = 0;
    
    bool CompileShader(const std::string& source);
    void CheckCompileErrors(GLuint shader, const std::string& type) const;
    std::string ReadFile(const std::string& filePath) const;
};

} // namespace nbody
