#include "rhi/include/RHIDevice.h"
#include "rhi/null/NullDevice.h"
#include "rhi/opengl/OpenGLDevice.h"

namespace engine {
namespace rhi {

std::unique_ptr<RHIDevice> RHIDevice::create(Backend backend) {
    switch (backend) {
        case Backend::OpenGL:
            return std::make_unique<OpenGLDevice>();
        case Backend::Null:
            return std::make_unique<NullDevice>();
        default:
            return nullptr;
    }
}

} // namespace rhi
} // namespace engine
