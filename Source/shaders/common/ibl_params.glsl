#ifndef ENGINE_COMMON_IBL_PARAMS_GLSL
#define ENGINE_COMMON_IBL_PARAMS_GLSL

layout (std140, binding = 4) uniform IBLParams {
	int reflectionProbeMipCount;
	int computeIBL;
	int useSSAO;
	int _iblPad0;
};

#endif
