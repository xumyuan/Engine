#pragma once

#include "RHIHandle.h"
#include <glm/glm.hpp>
#include <string>

namespace engine {
namespace rhi {

// RHI 着色器程序抽象接口
// 封装一个已编译链接的着色器程序，提供：
//   - 激活/停用
//   - Program handle 查询
//
// 各后端（OpenGL/Vulkan/...）提供具体实现
class RHIShaderProgram {
public:
    virtual ~RHIShaderProgram() = default;

    // ---------- Program 激活 ----------
    virtual void use() = 0;
    virtual void unuse() = 0;

    // ---------- 查询 ----------
    virtual ProgramHandle getProgramHandle() const = 0;
};

} // namespace rhi
} // namespace engine
