#include "OpenGLShaderProgram.h"
#include "rhi/include/RHIDevice.h"
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

namespace engine {
namespace rhi {

OpenGLShaderProgram::OpenGLShaderProgram(ProgramHandle programHandle, GLuint glProgramId, RHIDevice& device)
    : mProgramHandle(programHandle)
    , mGLProgramId(glProgramId)
    , mDevice(&device) {
}

OpenGLShaderProgram::~OpenGLShaderProgram() {
    if (mDevice && static_cast<bool>(mProgramHandle)) {
        mDevice->destroyProgram(mProgramHandle);
    }
}

OpenGLShaderProgram::OpenGLShaderProgram(OpenGLShaderProgram&& other) noexcept
    : mProgramHandle(other.mProgramHandle)
    , mGLProgramId(other.mGLProgramId)
    , mDevice(other.mDevice)
    , mUniformLocationCache(std::move(other.mUniformLocationCache)) {
    other.mProgramHandle.clear();
    other.mGLProgramId = 0;
    other.mDevice = nullptr;
}

OpenGLShaderProgram& OpenGLShaderProgram::operator=(OpenGLShaderProgram&& other) noexcept {
    if (this != &other) {
        // 先销毁自己持有的资源
        if (mDevice && static_cast<bool>(mProgramHandle)) {
            mDevice->destroyProgram(mProgramHandle);
        }
        mProgramHandle = other.mProgramHandle;
        mGLProgramId = other.mGLProgramId;
        mDevice = other.mDevice;
        mUniformLocationCache = std::move(other.mUniformLocationCache);
        other.mProgramHandle.clear();
        other.mGLProgramId = 0;
        other.mDevice = nullptr;
    }
    return *this;
}

// ============================================================================
// Program 激活
// ============================================================================

void OpenGLShaderProgram::use() {
    glUseProgram(mGLProgramId);
}

void OpenGLShaderProgram::unuse() {
    glUseProgram(0);
}

// ============================================================================
// Uniform Location 缓存查询
// ============================================================================

GLint OpenGLShaderProgram::getUniformLocation(const char* name) {
    auto it = mUniformLocationCache.find(name);
    if (it != mUniformLocationCache.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(mGLProgramId, name);
    if (location == -1) {
        spdlog::warn("[OpenGLShaderProgram] Uniform '{}' not found in program {}", name, mGLProgramId);
    }
    mUniformLocationCache[name] = location;
    return location;
}

void OpenGLShaderProgram::invalidateUniformCache() {
    mUniformLocationCache.clear();
}

// ============================================================================
// Uniform 设置
// ============================================================================

void OpenGLShaderProgram::setUniform(const char* name, float value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform1f(loc, value);
}

void OpenGLShaderProgram::setUniform(const char* name, int value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform1i(loc, value);
}

void OpenGLShaderProgram::setUniform(const char* name, const glm::vec2& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform2f(loc, value.x, value.y);
}

void OpenGLShaderProgram::setUniform(const char* name, const glm::vec3& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform3f(loc, value.x, value.y, value.z);
}

void OpenGLShaderProgram::setUniform(const char* name, const glm::vec4& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform4f(loc, value.x, value.y, value.z, value.w);
}

void OpenGLShaderProgram::setUniform(const char* name, const glm::ivec4& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniform4i(loc, value.x, value.y, value.z, value.w);
}

void OpenGLShaderProgram::setUniform(const char* name, const glm::mat3& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLShaderProgram::setUniform(const char* name, const glm::mat4& value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace rhi
} // namespace engine
