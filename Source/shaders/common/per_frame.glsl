#ifndef ENGINE_COMMON_PER_FRAME_GLSL
#define ENGINE_COMMON_PER_FRAME_GLSL

layout (std140, binding = 0) uniform PerFrame {
	mat4 view;
	mat4 projection;
	mat4 viewInverse;
	mat4 projectionInverse;
	vec4 viewPos;
	vec2 screenSize;
	vec2 texelSize;
};

#endif
