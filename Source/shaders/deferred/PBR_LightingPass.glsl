#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;

out vec2 TexCoords;

void main() {
	gl_Position = vec4(position, 1.0);
	TexCoords = texCoord;
}




#shader-type fragment
#version 450 core

in vec2 TexCoords;

out vec4 color;

#include "../common/depth_utils.glsl"
#include "../common/lighting_ubo.glsl"
#include "../common/ibl_params.glsl"
#include "../common/pbr_brdf.glsl"
#include "../common/shadow_sampling.glsl"

// ===== Texture samplers (remain as individual uniforms) =====
// GBuffer
layout(binding = 6) uniform sampler2D albedoTexture;
layout(binding = 7) uniform sampler2D normalTexture;
layout(binding = 8) uniform sampler2D materialInfoTexture;
layout(binding = 9) uniform sampler2D ssaoTexture;
layout(binding = 10) uniform sampler2D depthTexture;

// IBL
layout(binding = 1) uniform samplerCube irradianceMap;
layout(binding = 2) uniform samplerCube prefilterMap;
layout(binding = 3) uniform sampler2D brdfLUT;

// Shadow
layout(binding = 0) uniform sampler2D dirLightShadowmap;
layout(binding = 11) uniform sampler2D spotLightShadowmap;
layout(binding = 12) uniform samplerCube pointLightShadowCubemap;

// Light radiance calculations
vec3 CalculateDirectionalLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragPos, vec3 fragToViewNorm, vec3 baseReflectivity);
vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragPos, vec3 fragToViewNorm, vec3 baseReflectivity);
vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragPos, vec3 fragToViewNorm, vec3 baseReflectivity);

void main() {
	// If this is an emissive fragment then we don't care about lighting it
	float emissiveIntensity = texture(materialInfoTexture, TexCoords).a;
	if (emissiveIntensity > 0.0) {
		vec4 sampledAlbedo = texture(albedoTexture, TexCoords).rgba;
		color = sampledAlbedo * emissiveIntensity * 255.0;
		return;
	}

	// Sample textures
	vec4 sampledAlbedo = texture(albedoTexture, TexCoords).rgba;
	vec3 albedo = sampledAlbedo.rgb;
	float albedoAlpha = sampledAlbedo.w;
	vec3 normal = texture(normalTexture, TexCoords).rgb;
	float metallic = texture(materialInfoTexture, TexCoords).r;
	float unclampedRoughness = texture(materialInfoTexture, TexCoords).g;
	float roughness = max(unclampedRoughness, 0.04);
	float materialAO = texture(materialInfoTexture, TexCoords).b;
	float sceneAO = (useSSAO == 1) ? texture(ssaoTexture, TexCoords).r : 1.0;
	float ao = materialAO * sceneAO;
	// Reconstruct fragPos
	vec3 fragPos = reconstructWorldPosition(TexCoords, texture(depthTexture, TexCoords).r);

	vec3 fragToViewNorm = normalize(viewPos.xyz - fragPos);
	vec3 reflectionVec = reflect(-fragToViewNorm, normal);

	vec3 baseReflectivity = vec3(0.04);
	baseReflectivity = mix(baseReflectivity, albedo, metallic);

	// Calculate per light radiance for all of the direct lighting
	vec3 directLightIrradiance = vec3(0.0);
	directLightIrradiance += CalculateDirectionalLightRadiance(albedo, normal, metallic, roughness, fragPos, fragToViewNorm, baseReflectivity);
	directLightIrradiance += CalculatePointLightRadiance(albedo, normal, metallic, roughness, fragPos, fragToViewNorm, baseReflectivity);
	directLightIrradiance += CalculateSpotLightRadiance(albedo, normal, metallic, roughness, fragPos, fragToViewNorm, baseReflectivity);

	// Calculate ambient IBL for both diffuse and specular
	vec3 ambient = vec3(0.03) * albedo * ao;
	if (computeIBL != 0) {
		vec3 specularRatio = FresnelSchlick(max(dot(normal, fragToViewNorm), 0.0), baseReflectivity);
		vec3 diffuseRatio = vec3(1.0) - specularRatio;
		diffuseRatio *= 1.0 - metallic;

		vec3 indirectDiffuse = texture(irradianceMap, normal).rgb * albedo * diffuseRatio;

		vec3 prefilterColour = textureLod(prefilterMap, reflectionVec, unclampedRoughness * (reflectionProbeMipCount - 1)).rgb;
		vec2 brdfIntegration = texture(brdfLUT, vec2(max(dot(normal, fragToViewNorm), 0.0), roughness)).rg;
		vec3 indirectSpecular = prefilterColour * (specularRatio * brdfIntegration.x + brdfIntegration.y);

		ambient = (indirectDiffuse + indirectSpecular) * ao;
	}

	color = vec4(ambient + directLightIrradiance, 1.0);
}

vec3 CalculateDirectionalLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragPos, vec3 fragToViewNorm, vec3 baseReflectivity) {
	vec3 directLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.x; ++i) {
		vec3 lightDirNorm = normalize(-dirLights[i].direction.xyz);
		vec3 radiance = dirLights[i].direction.w * dirLights[i].lightColour.xyz;
		vec3 brdf = EvaluateDirectBRDF(albedo, normal, metallic, roughness, fragToViewNorm, lightDirNorm, baseReflectivity);
		float shadowAmount = CalculateShadow2DPCF(dirLightShadowData, dirLightShadowmap, fragPos);

		directLightIrradiance += brdf * radiance * max(dot(normal, lightDirNorm), 0.0) * (1.0 - shadowAmount);
	}

	return directLightIrradiance;
}

vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragPos, vec3 fragToViewNorm, vec3 baseReflectivity) {
	vec3 pointLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.y; ++i) {
		vec3 fragToLightNorm = normalize(pointLights[i].position.xyz - fragPos);
		vec3 lightToFrag = fragPos - pointLights[i].position.xyz;
		float fragToLightDistance = length(lightToFrag);

		float attenuation = AttenuationUE4(fragToLightDistance, pointLights[i].lightColour.w);
		vec3 radiance = pointLights[i].position.w * pointLights[i].lightColour.xyz * attenuation;
		vec3 brdf = EvaluateDirectBRDF(albedo, normal, metallic, roughness, fragToViewNorm, fragToLightNorm, baseReflectivity);

		float shadowAmount = 0.0;
		if (false)
			shadowAmount = CalculatePointLightShadowCubemap(pointLightShadowData, pointLightShadowCubemap, lightToFrag);

		pointLightIrradiance += brdf * radiance * max(dot(normal, fragToLightNorm), 0.0) * (1.0 - shadowAmount);
	}

	return pointLightIrradiance;
}

vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragPos, vec3 fragToViewNorm, vec3 baseReflectivity) {
	vec3 spotLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.z; ++i) {
		vec3 fragToLightNorm = normalize(spotLights[i].position.xyz - fragPos);
		float fragToLightDistance = length(spotLights[i].position.xyz - fragPos);

		float theta = dot(normalize(spotLights[i].direction.xyz), -fragToLightNorm);
		float difference = spotLights[i].lightColour.w - spotLights[i].params.x;
		float intensity = clamp((theta - spotLights[i].params.x) / difference, 0.0, 1.0);
		float attenuation = intensity * AttenuationUE4(fragToLightDistance, spotLights[i].direction.w);
		vec3 radiance = spotLights[i].position.w * spotLights[i].lightColour.xyz * attenuation;

		vec3 brdf = EvaluateDirectBRDF(albedo, normal, metallic, roughness, fragToViewNorm, fragToLightNorm, baseReflectivity);

		float shadowAmount = 0.0f;
		if (false)
			shadowAmount = CalculateShadow2DPCF(spotLightShadowData, spotLightShadowmap, fragPos);

		spotLightIrradiance += brdf * radiance * max(dot(normal, fragToLightNorm), 0.0) * (1.0 - shadowAmount);
	}

	return spotLightIrradiance;
}
