#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out mat3 TBN;
out vec2 TexCoords;
out vec3 FragPos;

#include "../common/per_frame.glsl"
#include "../common/per_object.glsl"

layout (std140, binding = 4) uniform ClipPlaneParams {
	vec4 clipPlane;
	int usesClipPlane;
	float _clipPad0;
	float _clipPad1;
	float _clipPad2;
};

void main() {
	// Use the normal matrix to maintain the orthogonal property of a vector when it is scaled non-uniformly
	vec3 T = normalize(normalMatrix * tangent);
	vec3 B = normalize(normalMatrix * bitangent);
	vec3 N = normalize(normalMatrix * normal);
	TBN = mat3(T, B, N);

	FragPos = vec3(model * vec4(position, 1.0f));
	TexCoords = texCoords;

	if (usesClipPlane != 0) {
		gl_ClipDistance[0] = dot(vec4(FragPos, 1.0), clipPlane);
	}
	gl_Position = projection * view * vec4(FragPos, 1.0);
}




#shader-type fragment
#version 450 core

in mat3 TBN;
in vec2 TexCoords;
in vec3 FragPos;

out vec4 color;

#include "../common/per_frame.glsl"
#include "../common/lighting_ubo.glsl"
#include "../common/material_params.glsl"
#include "../common/pbr_brdf.glsl"
#include "../common/shadow_sampling.glsl"

// ===== Texture samplers (remain as individual uniforms) =====
// Shadow
layout(binding = 0) uniform sampler2D dirLightShadowmap;
layout(binding = 11) uniform sampler2D spotLightShadowmap;
layout(binding = 12) uniform samplerCube pointLightShadowCubemap;

// Terrain material textures
layout(binding = 13) uniform sampler2D texture_albedo1;   // background texture
layout(binding = 14) uniform sampler2D texture_albedo2;   // r texture
layout(binding = 15) uniform sampler2D texture_albedo3;   // g texture
layout(binding = 16) uniform sampler2D texture_albedo4;   // b texture

layout(binding = 17) uniform sampler2D texture_normal1;   // background texture
layout(binding = 18) uniform sampler2D texture_normal2;   // r texture
layout(binding = 19) uniform sampler2D texture_normal3;   // g texture
layout(binding = 20) uniform sampler2D texture_normal4;   // b texture

layout(binding = 21) uniform sampler2D texture_roughness1; // background texture
layout(binding = 22) uniform sampler2D texture_roughness2; // r texture
layout(binding = 23) uniform sampler2D texture_roughness3; // g texture
layout(binding = 24) uniform sampler2D texture_roughness4; // b texture

layout(binding = 25) uniform sampler2D texture_metallic1; // background texture
layout(binding = 26) uniform sampler2D texture_metallic2; // r texture
layout(binding = 27) uniform sampler2D texture_metallic3; // g texture
layout(binding = 28) uniform sampler2D texture_metallic4; // b texture

layout(binding = 29) uniform sampler2D texture_AO1;      // background texture
layout(binding = 30) uniform sampler2D texture_AO2;      // r texture
layout(binding = 31) uniform sampler2D texture_AO3;      // g texture
layout(binding = 32) uniform sampler2D texture_AO4;      // b texture

layout(binding = 33) uniform sampler2D blendmap;

// Light radiance calculations
vec3 CalculateDirectionalLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity);
vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity);
vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity);

void main() {
	vec4 blendMapColour = texture(blendmap, TexCoords);
	float backTextureWeight = 1 - (blendMapColour.r + blendMapColour.g + blendMapColour.b);
	vec2 tiledCoords = TexCoords * tilingAmount;

	vec3 backgroundTextureAlbedo = texture(texture_albedo1, tiledCoords).rgb * backTextureWeight;
	vec3 rTextureAlbedo = texture(texture_albedo2, tiledCoords).rgb * blendMapColour.r;
	vec3 gTextureAlbedo = texture(texture_albedo3, tiledCoords).rgb * blendMapColour.g;
	vec3 bTextureAlbedo = texture(texture_albedo4, tiledCoords).rgb * blendMapColour.b;
	vec3 albedo = backgroundTextureAlbedo + rTextureAlbedo + gTextureAlbedo + bTextureAlbedo;

	vec3 backgroundTextureNormal = UnpackNormal(texture(texture_normal1, tiledCoords).rgb) * backTextureWeight;
	vec3 rTextureNormal = UnpackNormal(texture(texture_normal2, tiledCoords).rgb) * blendMapColour.r;
	vec3 gTextureNormal = UnpackNormal(texture(texture_normal3, tiledCoords).rgb) * blendMapColour.g;
	vec3 bTextureNormal = UnpackNormal(texture(texture_normal4, tiledCoords).rgb) * blendMapColour.b;
	vec3 normal = normalize(backgroundTextureNormal + rTextureNormal + gTextureNormal + bTextureNormal);

	float backgroundTextureRoughness = texture(texture_roughness1, tiledCoords).r * backTextureWeight;
	float rTextureRoughness = texture(texture_roughness2, tiledCoords).r * blendMapColour.r;
	float gTextureRoughness = texture(texture_roughness3, tiledCoords).r * blendMapColour.g;
	float bTextureRoughness = texture(texture_roughness4, tiledCoords).r * blendMapColour.b;
	float roughness = max(max(backgroundTextureRoughness, rTextureRoughness), max(gTextureRoughness, bTextureRoughness));
	roughness = max(roughness, 0.04); // Used for calculations since specular highlights will be too fine, and will cause flicker

	float backgroundTextureMetallic = texture(texture_metallic1, tiledCoords).r * backTextureWeight;
	float rTextureMetallic = texture(texture_metallic2, tiledCoords).r * blendMapColour.r;
	float gTextureMetallic = texture(texture_metallic3, tiledCoords).r * blendMapColour.g;
	float bTextureMetallic = texture(texture_metallic4, tiledCoords).r * blendMapColour.b;
	float metallic = max(max(backgroundTextureMetallic, rTextureMetallic), max(gTextureMetallic, bTextureMetallic));

	float backgroundTextureAO = texture(texture_AO1, tiledCoords).r * backTextureWeight;
	float rTextureAO = texture(texture_AO2, tiledCoords).r * blendMapColour.r;
	float gTextureAO = texture(texture_AO3, tiledCoords).r * blendMapColour.g;
	float bTextureAO = texture(texture_AO4, tiledCoords).r * blendMapColour.b;
	float ao = max(max(backgroundTextureAO, rTextureAO), max(gTextureAO, bTextureAO));

	// Normal mapping code. Opted out of tangent space normal mapping since I would have to convert all of my lights to tangent space
	normal = normalize(TBN * UnpackNormal(normal));

	vec3 fragToViewNorm = normalize(viewPos.xyz - FragPos);

	// Dielectrics have an average base specular reflectivity around 0.04, and metals absorb all of their diffuse (refraction) lighting so their albedo is used instead for their specular lighting (reflection)
	vec3 baseReflectivity = vec3(0.04);
	baseReflectivity = mix(baseReflectivity, albedo, metallic);

	// Calculate per light radiance for all of the direct lighting
	vec3 directLightIrradiance = vec3(0.0);
	directLightIrradiance += CalculateDirectionalLightRadiance(albedo, normal, metallic, roughness, fragToViewNorm, baseReflectivity);
	directLightIrradiance += CalculatePointLightRadiance(albedo, normal, metallic, roughness, fragToViewNorm, baseReflectivity);
	directLightIrradiance += CalculateSpotLightRadiance(albedo, normal, metallic, roughness, fragToViewNorm, baseReflectivity);

	// Calculate ambient term
	vec3 ambient = vec3(0.05) * albedo * ao;
	
	// Result
	color = vec4(ambient + directLightIrradiance, 1.0);
}

vec3 CalculateDirectionalLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity) {
	vec3 directLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.x; ++i) {
		vec3 lightDirNorm = normalize(-dirLights[i].direction.xyz);
		vec3 radiance = dirLights[i].direction.w * dirLights[i].lightColour.xyz;
		vec3 brdf = EvaluateDirectBRDF(albedo, normal, metallic, roughness, fragToViewNorm, lightDirNorm, baseReflectivity);
		float shadowAmount = CalculateShadow2DPCF(dirLightShadowData, dirLightShadowmap, FragPos);

		directLightIrradiance += brdf * radiance * max(dot(normal, lightDirNorm), 0.0) * (1.0 - shadowAmount);
	}

	return directLightIrradiance;
}

vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity) {
	vec3 pointLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.y; ++i) {
		vec3 fragToLightNorm = normalize(pointLights[i].position.xyz - FragPos);
		vec3 lightToFrag = FragPos - pointLights[i].position.xyz;
		float fragToLightDistance = length(lightToFrag);

		float attenuation = AttenuationUE4(fragToLightDistance, pointLights[i].lightColour.w);
		vec3 radiance = pointLights[i].position.w * pointLights[i].lightColour.xyz * attenuation;
		vec3 brdf = EvaluateDirectBRDF(albedo, normal, metallic, roughness, fragToViewNorm, fragToLightNorm, baseReflectivity);

		float shadowAmount = 0.0f;
		/*if (i == pointLightShadowData.lightShadowIndex)
			shadowAmount = CalculatePointLightShadowCubemap(pointLightShadowData, pointLightShadowCubemap, lightToFrag);*/

		pointLightIrradiance += brdf * radiance * max(dot(normal, fragToLightNorm), 0.0) * (1.0 - shadowAmount);
	}

	return pointLightIrradiance;
}

vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity) {
	vec3 spotLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.z; ++i) {
		vec3 fragToLightNorm = normalize(spotLights[i].position.xyz - FragPos);
		float fragToLightDistance = length(spotLights[i].position.xyz - FragPos);

		float theta = dot(normalize(spotLights[i].direction.xyz), -fragToLightNorm);
		float difference = spotLights[i].lightColour.w - spotLights[i].params.x;  // cutOff - outerCutOff
		float intensity = clamp((theta - spotLights[i].params.x) / difference, 0.0, 1.0);
		float attenuation = intensity * AttenuationUE4(fragToLightDistance, spotLights[i].direction.w);
		vec3 radiance = spotLights[i].position.w * spotLights[i].lightColour.xyz * attenuation;

		vec3 brdf = EvaluateDirectBRDF(albedo, normal, metallic, roughness, fragToViewNorm, fragToLightNorm, baseReflectivity);

		float shadowAmount = 0.0f;
		/*if (i == spotLightShadowData.lightShadowIndex)
			shadowAmount = CalculateShadow2DPCF(spotLightShadowData, spotLightShadowmap, FragPos);*/

		spotLightIrradiance += brdf * radiance * max(dot(normal, fragToLightNorm), 0.0) * (1.0 - shadowAmount);
	}

	return spotLightIrradiance;
}
