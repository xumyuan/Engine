 #shader-type vertex

 #version 450 core

layout (location = 0) in vec3 position;

#include "common/per_object.glsl"

layout (std140, binding = 4) uniform ShadowmapParams {
	mat4 lightSpaceViewProjectionMatrix;
};

void main() {
	gl_Position = lightSpaceViewProjectionMatrix * model * vec4(position, 1.0f);
}


#shader-type fragment
#version 450 core

void main() {}
