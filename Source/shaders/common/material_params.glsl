#ifndef ENGINE_COMMON_MATERIAL_PARAMS_GLSL
#define ENGINE_COMMON_MATERIAL_PARAMS_GLSL

layout (std140, binding = 3) uniform MaterialParams {
	vec4 albedoColour;
	vec4 emissionColour;             // xyz = emission, w = emissionIntensity
	float metallicValue;
	float roughnessValue;
	float parallaxStrength;
	float tilingAmount;
	int hasAlbedoTexture;
	int hasMetallicTexture;
	int hasRoughnessTexture;
	int hasEmissionTexture;
	int hasDisplacement;
	int hasEmission;
	vec2 minMaxDisplacementSteps;
};

#endif
