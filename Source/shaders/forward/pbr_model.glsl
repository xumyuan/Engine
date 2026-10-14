#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out mat3 TBN;
out vec3 FragPos;
out vec2 TexCoords;

#include "../common/per_frame.glsl"
#include "../common/per_object.glsl"

void main() {
	// Use the normal matrix to maintain the orthogonal property of a vector when it is scaled non-uniformly
	vec3 T = normalize(normalMatrix * tangent);
	vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * normal);
    TBN = mat3(T, B, N);

	FragPos = vec3(model * vec4(position, 1.0f));
	TexCoords = texCoords;

	gl_Position = projection * view * vec4(FragPos, 1.0f);
}

#shader-type fragment
#version 450 core

in mat3 TBN;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 color;

#include "../common/per_frame.glsl"
#include "../common/lighting_ubo.glsl"
#include "../common/material_params.glsl"
#include "../common/ibl_params.glsl"
#include "../common/pbr_brdf.glsl"

// ===== Texture samplers (remain as individual uniforms) =====
layout(binding = 1) uniform samplerCube irradianceMap;
layout(binding = 2) uniform samplerCube prefilterMap;
layout(binding = 3) uniform sampler2D brdfLUT;

layout(binding = 0) uniform sampler2D dirLightShadowmap;

layout(binding = 4) uniform sampler2D texture_albedo;
layout(binding = 5) uniform sampler2D texture_normal;
layout(binding = 6) uniform sampler2D texture_metallic;
layout(binding = 7) uniform sampler2D texture_roughness;
layout(binding = 8) uniform sampler2D texture_ao;
layout(binding = 9) uniform sampler2D texture_displacement;
layout(binding = 10) uniform sampler2D texture_emission;

// Light radiance calculations
vec3 CalculateDirectionalLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity);
vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity);
vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity);
float CalculateShadow(vec3 normal, vec3 fragToDirLight);

void main() {
	// Sample textures - Use material values/textures based on availability
	vec4 albedo = (hasAlbedoTexture != 0) ? texture(texture_albedo, TexCoords).rgba * albedoColour : albedoColour;
	float albedoAlpha = albedo.w;
	vec3 normal = texture(texture_normal, TexCoords).rgb;
	float metallic = (hasMetallicTexture != 0) ? texture(texture_metallic, TexCoords).r : metallicValue;
	float unclampedRoughness = (hasRoughnessTexture != 0) ? texture(texture_roughness, TexCoords).r : roughnessValue;
	float roughness = max(unclampedRoughness, 0.04);
	float ao = texture(texture_ao, TexCoords).r;

	// 法线贴图代码。 选择退出切线空间法线贴图，因为我必须将所有灯光转换为切线空间
	normal = normalize(TBN * UnpackNormal(normal));

	vec3 fragToView = normalize(viewPos.xyz - FragPos);
	vec3 reflectionVec = reflect(-fragToView, normal);

	// 电介质的平均基础镜面反射率约为 0.04，金属吸收其所有漫反射（折射）照明，因此其反照率用于代替镜面照明（反射）
	vec3 baseReflectivity = vec3(0.04);
	baseReflectivity = mix(baseReflectivity, albedo.rgb, metallic);


	// 计算所有直接照明的每个光辐射率
	vec3 directLightIrradiance = vec3(0.0);
	directLightIrradiance += CalculateDirectionalLightRadiance(albedo.rgb, normal, metallic, roughness, fragToView, baseReflectivity);
	directLightIrradiance += CalculatePointLightRadiance(albedo.rgb, normal, metallic, roughness, fragToView, baseReflectivity);
	directLightIrradiance += CalculateSpotLightRadiance(albedo.rgb, normal, metallic, roughness, fragToView, baseReflectivity);

	// 计算漫反射和镜面反射的环境 IBL
	vec3 ambient = vec3(0.03) * albedo.rgb * ao;
	if (computeIBL != 0) {
		// 计算镜面反射和漫反射光的比例
		vec3 specularRatio = FresnelSchlick(max(dot(normal, fragToView), 0.0), baseReflectivity);

		vec3 diffuseRatio = vec3(1.0) - specularRatio;
		diffuseRatio *= 1.0 - metallic;

		// 环境光照的漫反射项
		vec3 indirectDiffuse = texture(irradianceMap, normal).rgb * albedo.rgb;

		// 镜面反射项
		vec3 prefilterColour = textureLod(prefilterMap, reflectionVec, unclampedRoughness * (reflectionProbeMipCount - 1)).rgb;
		vec2 brdfIntegration = texture(brdfLUT, vec2(max(dot(normal, fragToView), 0.0), roughness)).rg;
		vec3 indirectSpecular = prefilterColour * (specularRatio * brdfIntegration.x + brdfIntegration.y);

		ambient = (diffuseRatio * indirectDiffuse + indirectSpecular) * ao;
	}
	
	// Check for emission and add it to final color
	vec3 emission = vec3(0.0);
	if (hasEmissionTexture != 0) {
		vec3 emissiveSample = texture(texture_emission, TexCoords).rgb;
		if (!all(equal(emissiveSample, vec3(0.0)))) {
			emission = emissiveSample * emissionColour.w;
		}
	}
	else if (length(emissionColour.xyz) > 0.0) {
		emission = emissionColour.xyz * emissionColour.w;
	}
	
	color = vec4(ambient + directLightIrradiance + emission, albedoAlpha);
}

vec3 CalculateDirectionalLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity) {
	vec3 directLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.x; ++i) {
		vec3 lightDir = normalize(-dirLights[i].direction.xyz);
		vec3 radiance = dirLights[i].direction.w * dirLights[i].lightColour.xyz;
		vec3 brdf = EvaluateDirectBRDF(albedo, normal, metallic, roughness, fragToView, lightDir, baseReflectivity);
		float shadowAmount = CalculateShadow(normal, lightDir);

		directLightIrradiance += brdf * radiance * max(dot(normal, lightDir), 0.0) * (1.0 - shadowAmount);
	}

	return directLightIrradiance;
}

vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity) {
	vec3 pointLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.y; ++i) {
		vec3 fragToLight = normalize(pointLights[i].position.xyz - FragPos);
		float fragToLightDistance = length(pointLights[i].position.xyz - FragPos);

		float attenuation = AttenuationUE4(fragToLightDistance, pointLights[i].lightColour.w);
		vec3 radiance = pointLights[i].position.w * pointLights[i].lightColour.xyz * attenuation;
		vec3 brdf = EvaluateDirectBRDF(albedo, normal, metallic, roughness, fragToView, fragToLight, baseReflectivity);

		pointLightIrradiance += brdf * radiance * max(dot(normal, fragToLight), 0.0);
	}

	return pointLightIrradiance;
}

vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity) {
	vec3 spotLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.z; ++i) {
		vec3 fragToLight = normalize(spotLights[i].position.xyz - FragPos);
		float fragToLightDistance = length(spotLights[i].position.xyz - FragPos);

		float theta = dot(normalize(spotLights[i].direction.xyz), -fragToLight);
		float difference = spotLights[i].lightColour.w - spotLights[i].params.x;
		float intensity = clamp((theta - spotLights[i].params.x) / difference, 0.0, 1.0);
		float attenuation = intensity * AttenuationUE4(fragToLightDistance, spotLights[i].direction.w);
		vec3 radiance = spotLights[i].position.w * spotLights[i].lightColour.xyz * attenuation;

		vec3 brdf = EvaluateDirectBRDF(albedo, normal, metallic, roughness, fragToView, fragToLight, baseReflectivity);

		spotLightIrradiance += brdf * radiance * max(dot(normal, fragToLight), 0.0);
	}

	return spotLightIrradiance;
}

float CalculateShadow(vec3 normal, vec3 fragToLight) {
	if (dirLightShadowData.lightShadowIndex == -1)
		return 0.0;

	vec4 fragPosLightClipSpace = dirLightShadowData.lightSpaceViewProjectionMatrix * vec4(FragPos, 1.0);
	vec3 ndcCoords = fragPosLightClipSpace.xyz / fragPosLightClipSpace.w;
	vec3 depthmapCoords = ndcCoords * 0.5 + 0.5;

	float shadow = 0.0;
	float currentDepth = depthmapCoords.z;

	// 添加阴影偏置以避免阴影失真，根据法线方向和光线方向之间的角度需要更多阴影偏置
	// 太多的偏移会导致 peter panning
	float shadowBias = max(0.01, 0.1 * (1.0 - dot(normal, fragToLight)));

	// 执行百分比接近过滤 (PCF) 以产生柔和的阴影
	vec2 shadowTexelSize = 1.0 / textureSize(dirLightShadowmap, 0);
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			float sampledDepthPCF = texture(dirLightShadowmap, depthmapCoords.xy + (shadowTexelSize * vec2(x, y))).r;
			shadow += currentDepth > sampledDepthPCF + shadowBias ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;

	if (currentDepth > 1.0)
		shadow = 0.0;
	return shadow;
}
