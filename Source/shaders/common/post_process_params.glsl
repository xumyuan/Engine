#ifndef ENGINE_COMMON_POST_PROCESS_PARAMS_GLSL
#define ENGINE_COMMON_POST_PROCESS_PARAMS_GLSL

layout (std140, binding = 4) uniform PostProcessParams {
	float gamma_inverse;
	float exposure;
	vec2 texel_size;
};

#endif
