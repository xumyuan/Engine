#pragma once

#include "rhi/include/RHIDevice.h"

namespace engine {

// 设置/获取全局 RHI 设备（在初始化时调用一次）
void setRHIDevice(rhi::RHIDevice* device);
rhi::RHIDevice* getRHIDevice();

// 兼容旧接口
inline void setDebugDevice(rhi::RHIDevice* device) { setRHIDevice(device); }
inline rhi::RHIDevice* getDebugDevice() { return getRHIDevice(); }

} // namespace engine

#if _DEBUG
#define BEGIN_EVENT(eventname) \
    do { if (auto* _dev = engine::getRHIDevice()) { std::string _s(eventname); _dev->pushDebugGroup(_s.c_str()); } } while(0)
#define END_EVENT() \
    do { if (auto* _dev = engine::getRHIDevice()) _dev->popDebugGroup(); } while(0)
#else
#define BEGIN_EVENT(eventname)
#define END_EVENT()
#endif
