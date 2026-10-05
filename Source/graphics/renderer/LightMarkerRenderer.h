#pragma once

#include "graphics/mesh/common/Sphere.h"
#include "rhi/include/RHICommandBuffer.h"

namespace engine {

class Shader;

// Main-view debug geometry; light data comes from the same UBO used for lighting.
class LightMarkerRenderer {
public:
    LightMarkerRenderer();
    void draw(rhi::CommandBuffer& cmd, bool multisampled);

private:
    Sphere m_Sphere{16, 12};
    Shader* m_Shader = nullptr;
};

}
