#include "OpenGLProgram.h"
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

namespace engine {
namespace rhi {

OpenGLProgram::OpenGLProgram(GLuint glProgramId)
    : mGLProgramId(glProgramId) {
}

OpenGLProgram::OpenGLProgram(OpenGLProgram&& other) noexcept
    : mGLProgramId(other.mGLProgramId)
    , mUniformLocationCache(std::move(other.mUniformLocationCache)) {
    other.mGLProgramId = 0;
}

OpenGLProgram& OpenGLProgram::operator=(OpenGLProgram&& other) noexcept {
    if (this != &other) {
        mGLProgramId = other.mGLProgramId;
        mUniformLocationCache = std::move(other.mUniformLocationCache);
        other.mGLProgramId = 0;
    }
    return *this;
}

// ============================================================================
// Program 激活
// ============================================================================

void OpenGLProgram::use() const {
    glUseProgram(mGLProgramId);
}

void OpenGLProgram::unuse() {
    glUseProgram(0);
}

// ============================================================================
// Uniform Location 缓存查询
// ============================================================================

GLint OpenGLProgram::getUniformLocation(const char* name) {
    auto it = mUniformLocationCache.find(name);
    if (it != mUniformLocationCache.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(mGLProgramId, name);
    if (location == -1) {
        spdlog::warn("[OpenGLProgram] Uniform '{}' not found in program {}", name, mGLProgramId);
    }
    mUniformLocationCache[name] = location;
    return location;
}

void OpenGLProgram::invalidateUniformCache() {
    mUniformLocationCache.clear();
}

// ============================================================================
// Uniform 设置
// ============================================================================

void OpenGLProgram::setUniform(const char* name, float value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform1f(loc, value);
}

void OpenGLProgram::setUniform(const char* name, int value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform1i(loc, value);
}

void OpenGLProgram::setUniform(const char* name, const glm::vec2& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform2f(loc, value.x, value.y);
}

void OpenGLProgram::setUniform(const char* name, const glm::vec3& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform3f(loc, value.x, value.y, value.z);
}

void OpenGLProgram::setUniform(const char* name, const glm::vec4& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform4f(loc, value.x, value.y, value.z, value.w);
}

void OpenGLProgram::setUniform(const char* name, const glm::ivec4& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform4i(loc, value.x, value.y, value.z, value.w);
}

void OpenGLProgram::setUniform(const char* name, const glm::mat3& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLProgram::setUniform(const char* name, const glm::mat4& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace rhi
} // namespace engine
