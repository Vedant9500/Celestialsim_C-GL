#include "rendering/ComputeShader.h"

namespace nbody {

ComputeShader::ComputeShader() = default;

ComputeShader::~ComputeShader() {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
    }
}

bool ComputeShader::LoadFromFile(const std::string& computePath) {
    std::string source = ReadFile(computePath);
    if (source.empty()) {
        std::cerr << "Failed to read compute shader file: " << computePath << std::endl;
        return false;
    }
    
    return LoadFromString(source);
}

bool ComputeShader::LoadFromString(const std::string& source) {
    return CompileShader(source);
}

void ComputeShader::Use() const {
    if (m_programID != 0) {
        glUseProgram(m_programID);
    }
}

void ComputeShader::Dispatch(unsigned int numGroupsX, unsigned int numGroupsY, unsigned int numGroupsZ) const {
    if (m_programID != 0) {
        glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
    }
}

void ComputeShader::MemoryBarrier() const {
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ComputeShader::SetInt(const std::string& name, int value) const {
    if (m_programID != 0) {
        GLint location = glGetUniformLocation(m_programID, name.c_str());
        if (location != -1) {
            glUniform1i(location, value);
        }
    }
}

void ComputeShader::SetFloat(const std::string& name, float value) const {
    if (m_programID != 0) {
        GLint location = glGetUniformLocation(m_programID, name.c_str());
        if (location != -1) {
            glUniform1f(location, value);
        }
    }
}

void ComputeShader::SetBool(const std::string& name, bool value) const {
    if (m_programID != 0) {
        GLint location = glGetUniformLocation(m_programID, name.c_str());
        if (location != -1) {
            glUniform1i(location, value ? 1 : 0);
        }
    }
}

bool ComputeShader::CompileShader(const std::string& source) {
    // Clean up existing program
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
        m_programID = 0;
    }
    
    // Create and compile compute shader
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    const char* sourceCStr = source.c_str();
    glShaderSource(computeShader, 1, &sourceCStr, nullptr);
    glCompileShader(computeShader);
    
    CheckCompileErrors(computeShader, "COMPUTE");
    
    // Create program and link
    m_programID = glCreateProgram();
    glAttachShader(m_programID, computeShader);
    glLinkProgram(m_programID);
    
    CheckCompileErrors(m_programID, "PROGRAM");
    
    // Clean up shader object
    glDeleteShader(computeShader);
    
    return m_programID != 0;
}

void ComputeShader::CheckCompileErrors(GLuint shader, const std::string& type) const {
    GLint success;
    GLchar infoLog[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
        }
    }
}

std::string ComputeShader::ReadFile(const std::string& filePath) const {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return "";
    }
    
    std::stringstream stream;
    stream << file.rdbuf();
    file.close();
    
    return stream.str();
}

} // namespace nbody
