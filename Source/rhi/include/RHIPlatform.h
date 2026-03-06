#pragma once
#include "RHITypes.h"
#include <memory>
namespace engine {

namespace rhi {

class RHIDevice;

// 平台配置
struct PlatformConfig {
    void*       nativeWindow    = nullptr;  // HWND / NSWindow* / ANativeWindow*
    void*       nativeDisplay   = nullptr;  // HDC / Display* / EGLDisplay
    uint32_t    width           = 1280;
    uint32_t    height          = 720;
    bool        vsync           = true;
    bool        debug           = false;    // 启用验证层/调试
};

// 平台层 —— 负责创建窗口系统相关的上下文
class RHIPlatform {
public:
    virtual ~RHIPlatform() = default;

    virtual bool initialize(const PlatformConfig& config) = 0;
    virtual void terminate() = 0;

    // 创建对应的 RHI 设备
    virtual std::unique_ptr<RHIDevice> createDevice() = 0;

    // 工厂
    static std::unique_ptr<RHIPlatform> create(Backend backend);
};

} // namespace rhi
} // namespace engine