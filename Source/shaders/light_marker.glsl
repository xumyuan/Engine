#shader-type vertex
#version 450 core

layout(location = 0) in vec3 position;

#include "common/per_frame.glsl"
#include "common/lighting_ubo.glsl"

layout(std140, binding = 4) uniform MarkerParams {
    vec4 markerSettings; // radius, display brightness, unused, unused
};

layout(location = 0) flat out vec3 markerColor;

void main() {
    int pointCount = clamp(numDirPointSpotLights.y, 0, 9);
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
