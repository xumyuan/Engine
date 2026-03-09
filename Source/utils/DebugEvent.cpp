#include "DebugEvent.h"

namespace engine {

static rhi::RHIDevice* gDebugDevice = nullptr;

void setDebugDevice(rhi::RHIDevice* device) {
    gDebugDevice = device;
}

rhi::RHIDevice* getDebugDevice() {
    return gDebugDevice;
}

} // namespace engine
