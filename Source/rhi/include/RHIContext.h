#pragma once

#include "RHIDevice.h"

namespace engine {

// 全局 RHI 设备访问器（Service Locator）
// 在引擎初始化时调用 setRHIDevice() 注册设备实例
void setRHIDevice(rhi::RHIDevice* device);
rhi::RHIDevice* getRHIDevice();

} // namespace engine
