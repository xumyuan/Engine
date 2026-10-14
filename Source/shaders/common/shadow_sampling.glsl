#ifndef ENGINE_COMMON_SHADOW_SAMPLING_GLSL
#define ENGINE_COMMON_SHADOW_SAMPLING_GLSL

#include "lighting_ubo.glsl"

float CalculateShadow2DPCF(ShadowData shadowData, sampler2D shadowmap, vec3 fragPos) {
	if (shadowData.lightShadowIndex == -1)
		return 0.0;

	vec4 fragPosLightClipSpace = shadowData.lightSpaceViewProjectionMatrix * vec4(fragPos, 1.0);
	vec3 ndcCoords = fragPosLightClipSpace.xyz / fragPosLightClipSpace.w;
	vec3 depthmapCoords = ndcCoords * 0.5 + 0.5;

	float currentDepth = depthmapCoords.z;
	if (currentDepth > 1.0)
		return 0.0;

	float shadow = 0.0;
	vec2 texelSizeShadow = 1.0 / textureSize(shadowmap, 0);
	for (float y = -1.5; y < 1.0; y += 2.0) {
		for (float x = -1.5; x < 1.0; x += 2.0) {
			float sampledDepthPCF = texture(shadowmap, depthmapCoords.xy + (texelSizeShadow * vec2(x, y))).r;
			shadow += currentDepth > sampledDepthPCF + shadowData.shadowBias ? 1.0 : 0.0;
		}
	}
	return shadow * 0.25;
}

const vec3 kPointShadowSampleOffsetDirections[20] = vec3[](
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float CalculatePointLightShadowCubemap(ShadowDataPointLight shadowData, samplerCube shadowCubemap, vec3 lightToFrag) {
	if (shadowData.lightShadowIndex == -1)
		return 0.0;

	float currentDepth = length(lightToFrag);
	if (currentDepth > shadowData.farPlane)
		return 0.0;

	float shadow = 0.0;
	const int samples = 20;
	const float diskRadius = 0.05;
	for (int i = 0; i < samples; ++i) {
		float closestDepth = texture(shadowCubemap, lightToFrag + kPointShadowSampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= shadowData.farPlane;
		if (currentDepth - shadowData.shadowBias > closestDepth)
			shadow += 1.0;
	}
	return shadow / float(samples);
}

#endif
