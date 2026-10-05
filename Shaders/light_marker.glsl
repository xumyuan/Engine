#shader-type vertex
#version 450 core

layout(location = 0) in vec3 position;

layout(std140, binding = 0) uniform PerFrame {
    mat4 view;
    mat4 projection;
    mat4 viewInverse;
    mat4 projectionInverse;
    vec4 viewPos;
    vec2 screenSize;
    vec2 texelSize;
};

struct DirLight { vec4 direction; vec4 lightColour; };
struct PointLight { vec4 position; vec4 lightColour; };
struct SpotLight { vec4 position; vec4 direction; vec4 lightColour; vec4 params; };

// Matches the Lighting UBO prefix in UniformBufferData.h; shadow fields are unused.
layout(std140, binding = 2) uniform Lighting {
    ivec4 numDirPointSpotLights;
    DirLight dirLights[3];
    PointLight pointLights[6];
    SpotLight spotLights[6];
};

layout(std140, binding = 4) uniform MarkerParams {
    vec4 markerSettings; // radius, display brightness, unused, unused
};

layout(location = 0) flat out vec3 markerColor;

void main() {
    int pointCount = clamp(numDirPointSpotLights.y, 0, 6);
    vec4 lightPosition;
    vec3 lightColor;
    if (gl_InstanceID < pointCount) {
        lightPosition = pointLights[gl_InstanceID].position;
        lightColor = pointLights[gl_InstanceID].lightColour.rgb;
    } else {
        int index = gl_InstanceID - pointCount;
        lightPosition = spotLights[index].position;
        lightColor = spotLights[index].lightColour.rgb;
    }

    lightColor = max(lightColor, vec3(0.0));
    float peak = max(lightColor.r, max(lightColor.g, lightColor.b));
    if (peak <= 0.0 || lightPosition.w <= 0.0) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        markerColor = vec3(0.0);
        return;
    }
    markerColor = lightColor / peak * markerSettings.y;
    vec3 worldPosition = lightPosition.xyz + position * markerSettings.x;
    gl_Position = projection * view * vec4(worldPosition, 1.0);
}

#shader-type fragment
#version 450 core

layout(location = 0) flat in vec3 markerColor;
layout(location = 0) out vec4 color;

void main() {
    color = vec4(markerColor, 1.0);
}
