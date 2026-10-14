#ifndef ENGINE_COMMON_LIGHTING_UBO_GLSL
#define ENGINE_COMMON_LIGHTING_UBO_GLSL

struct DirLight {
	vec4 direction;        // xyz = direction, w = intensity
	vec4 lightColour;      // xyz = colour, w = padding
};

struct PointLight {
	vec4 position;         // xyz = position, w = intensity
	vec4 lightColour;      // xyz = colour, w = attenuationRadius
};

struct SpotLight {
	vec4 position;         // xyz = position, w = intensity
	vec4 direction;        // xyz = direction, w = attenuationRadius
	vec4 lightColour;      // xyz = colour, w = cutOff
	vec4 params;           // x = outerCutOff, yzw = padding
};

struct ShadowData {
	mat4 lightSpaceViewProjectionMatrix;
	float shadowBias;
	int lightShadowIndex;
	float _pad0;
	float _pad1;
};

struct ShadowDataPointLight {
	float farPlane;
	float shadowBias;
	int lightShadowIndex;
	float _pad0;
};

#define MAX_DIR_LIGHTS 3
#define MAX_POINT_LIGHTS 9
#define MAX_SPOT_LIGHTS 6

layout (std140, binding = 2) uniform Lighting {
	ivec4 numDirPointSpotLights;
	DirLight dirLights[MAX_DIR_LIGHTS];
	PointLight pointLights[MAX_POINT_LIGHTS];
	SpotLight spotLights[MAX_SPOT_LIGHTS];
	ShadowData dirLightShadowData;
	ShadowData spotLightShadowData;
	ShadowDataPointLight pointLightShadowData;
};

#endif
