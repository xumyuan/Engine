#include "rhi/include/RHIContext.h"

namespace engine {

static rhi::RHIDevice* gRHIDevice = nullptr;

void setRHIDevice(rhi::RHIDevice* device) {
    gRHIDevice = device;
}

rhi::RHIDevice* getRHIDevice() {
    return gRHIDevice;
}

} // namespace engine
