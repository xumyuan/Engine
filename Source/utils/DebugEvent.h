#pragma once

#include "rhi/include/RHIDevice.h"

namespace engine {

// 设置当前调试事件使用的 RHI 设备（通常在初始化时调用一次）
void setDebugDevice(rhi::RHIDevice* device);

// 获取当前调试设备
rhi::RHIDevice* getDebugDevice();

} // namespace engine

#if _DEBUG
#define BEGIN_EVENT(eventname) \
    do { if (auto* _dev = engine::getDebugDevice()) { std::string _s(eventname); _dev->pushDebugGroup(_s.c_str()); } } while(0)
#define END_EVENT() \
    do { if (auto* _dev = engine::getDebugDevice()) _dev->popDebugGroup(); } while(0)
#else
#define BEGIN_EVENT(eventname)
#define END_EVENT()
#endif
