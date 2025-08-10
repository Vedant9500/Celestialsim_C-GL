#include "rendering/ComputeShader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

namespace nbody {

ComputeShader::ComputeShader() = default;

ComputeShader::~ComputeShader() {
    Cleanup();
}

ComputeShader::ComputeShader(ComputeShader&& other) noexcept 
    : m_programId(other.m_programId), m_shaderId(other.m_shaderId) {
    other.m_programId = 0;
    other.m_shaderId = 0;
}

ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_programId = other.m_programId;
        m_shaderId = other.m_shaderId;
        other.m_programId = 0;
        other.m_shaderId = 0;
    }
    return *this;
}

bool ComputeShader::LoadFromFile(const std::string& shaderPath) {
    std::ifstream file(shaderPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open compute shader file: " << shaderPath << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return LoadFromSource(buffer.str());
}

bool ComputeShader::LoadFromSource(const std::string& shaderSource) {
    if (!IsSupported()) {
        std::cerr << "Compute shaders are not supported on this system" << std::endl;
        return false;
    }

    Cleanup(); // Clean up any existing shader

    return CompileShader(shaderSource);
}

bool ComputeShader::CompileShader(const std::string& source) {
    // Create compute shader
    m_shaderId = glCreateShader(GL_COMPUTE_SHADER);
    if (m_shaderId == 0) {
        std::cerr << "Failed to create compute shader" << std::endl;
        return false;
    }

    // Compile shader
    const char* sourceCStr = source.c_str();
    glShaderSource(m_shaderId, 1, &sourceCStr, nullptr);
    glCompileShader(m_shaderId);

    // Check compilation status
    GLint success;
    glGetShaderiv(m_shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(m_shaderId, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Compute shader compilation failed:\\n" << infoLog << std::endl;
        glDeleteShader(m_shaderId);
        m_shaderId = 0;
        return false;
    }

    // Create program
    m_programId = glCreateProgram();
    if (m_programId == 0) {
        std::cerr << "Failed to create shader program" << std::endl;
        glDeleteShader(m_shaderId);
        m_shaderId = 0;
        return false;
    }

    // Link program
    glAttachShader(m_programId, m_shaderId);
    glLinkProgram(m_programId);

    // Check linking status
    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(m_programId, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Compute shader program linking failed:\\n" << infoLog << std::endl;
        Cleanup();
        return false;
    }

    // Shader can be deleted after linking
    glDeleteShader(m_shaderId);
    m_shaderId = 0;

    std::cout << "Compute shader compiled and linked successfully" << std::endl;
    return true;
}

void ComputeShader::Use() const {
    if (m_programId != 0) {
        glUseProgram(m_programId);
    }
}

void ComputeShader::Dispatch(GLuint numGroupsX, GLuint numGroupsY, GLuint numGroupsZ) const {
    if (m_programId != 0) {
        glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
    }
}

void ComputeShader::MemoryBarrier() {
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

GLuint ComputeShader::CreateSSBO(const void* data, size_t size, GLenum usage) {
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(size), data, usage);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return buffer;
}

void ComputeShader::BindSSBO(GLuint buffer, GLuint bindingPoint) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, buffer);
}

void ComputeShader::UpdateSSBO(GLuint buffer, size_t offset, size_t size, const void* data) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, static_cast<GLintptr>(offset), 
                    static_cast<GLsizeiptr>(size), data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ComputeShader::ReadSSBO(GLuint buffer, size_t offset, size_t size, void* data) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, static_cast<GLintptr>(offset), 
                                static_cast<GLsizeiptr>(size), GL_MAP_READ_BIT);
    if (ptr) {
        std::memcpy(data, ptr, size);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ComputeShader::DeleteBuffer(GLuint buffer) {
    if (buffer != 0) {
        glDeleteBuffers(1, &buffer);
    }
}

void ComputeShader::SetInt(const std::string& name, int value) const {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void ComputeShader::SetFloat(const std::string& name, float value) const {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void ComputeShader::SetVec2(const std::string& name, float x, float y) const {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void ComputeShader::SetVec3(const std::string& name, float x, float y, float z) const {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

bool ComputeShader::IsSupported() {
    // Check if OpenGL 4.3 or higher is available (compute shaders requirement)
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    return (major > 4) || (major == 4 && minor >= 3);
}

void ComputeShader::GetMaxWorkGroupSizes(int& x, int& y, int& z) {
    GLint sizes[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &sizes[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &sizes[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &sizes[2]);
    x = sizes[0];
    y = sizes[1];
    z = sizes[2];
}

int ComputeShader::GetMaxWorkGroupInvocations() {
    GLint maxInvocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocations);
    return maxInvocations;
}

GLint ComputeShader::GetUniformLocation(const std::string& name) const {
    if (m_programId == 0) {
        return -1;
    }
    return glGetUniformLocation(m_programId, name.c_str());
}

void ComputeShader::Cleanup() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        m_programId = 0;
    }
    if (m_shaderId != 0) {
        glDeleteShader(m_shaderId);
        m_shaderId = 0;
    }
}

} // namespace nbody
