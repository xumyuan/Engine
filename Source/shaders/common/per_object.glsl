#ifndef ENGINE_COMMON_PER_OBJECT_GLSL
#define ENGINE_COMMON_PER_OBJECT_GLSL

layout (std140, binding = 1) uniform PerObject {
	mat4 model;
	mat3 normalMatrix;  // std140: 3 x vec4
};

#endif
