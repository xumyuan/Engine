#pragma once

#include "rhi/include/RHIShaderProgram.h"
#include "rhi/include/RHITypes.h"
#include <GL/glew.h>

namespace engine {
namespace rhi {

class RHIDevice;

// OpenGL 后端的着色器程序实现
// 封装 GL program 对象，提供 program 激活；数据通过 UBO、纹理通过 shader 中的 layout(binding) 绑定
// 析构时自动通过 RHIDevice 销毁底层 GL program
class OpenGLShaderProgram final : public RHIShaderProgram {
public:
    // programHandle: RHI 句柄（供上层查询）
    // glProgramId:   GL program 对象 ID
    // device:        创建此 program 的 RHIDevice（析构时用于资源回收）
    OpenGLShaderProgram(ProgramHandle programHandle, GLuint glProgramId, RHIDevice& device);
    ~OpenGLShaderProgram() override;

    // 不可复制，可移动
    OpenGLShaderProgram(const OpenGLShaderProgram&) = delete;
    OpenGLShaderProgram& operator=(const OpenGLShaderProgram&) = delete;
    OpenGLShaderProgram(OpenGLShaderProgram&& other) noexcept;
    OpenGLShaderProgram& operator=(OpenGLShaderProgram&& other) noexcept;

    // ---------- RHIShaderProgram 接口 ----------
    void use() override;
    void unuse() override;


    ProgramHandle getProgramHandle() const override { return mProgramHandle; }

    // ---------- OpenGL 特有查询 ----------
    GLuint getGLProgramId() const { return mGLProgramId; }

private:
    // 首次使用时等待编译结果；失败的 program 不能交给 glUseProgram
    bool ensureReady();

    ProgramHandle mProgramHandle;
    GLuint mGLProgramId = 0;
    RHIDevice* mDevice = nullptr;
    ProgramStatus mStatus = ProgramStatus::Pending;
};

} // namespace rhi
} // namespace engine
