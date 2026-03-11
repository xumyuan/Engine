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

// ===== UBO blocks =====
layout (std140, binding = 0) uniform PerFrame {
	mat4 view;
	mat4 projection;
	mat4 viewInverse;
	mat4 projectionInverse;
	vec4 viewPos;
	vec2 screenSize;
	vec2 texelSize;
};

layout (std140, binding = 1) uniform PerObject {
	mat4 model;
	mat3 normalMatrix;  // std140: 3 x vec4
};

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
#define MAX_POINT_LIGHTS 6
#define MAX_SPOT_LIGHTS 6
const float PI = 3.14159265359;

in mat3 TBN;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 color;

// ===== UBO blocks =====
layout (std140, binding = 0) uniform PerFrame {
	mat4 view;
	mat4 projection;
	mat4 viewInverse;
	mat4 projectionInverse;
	vec4 viewPos;
	vec2 screenSize;
	vec2 texelSize;
};

layout (std140, binding = 2) uniform Lighting {
	ivec4 numDirPointSpotLights;
	DirLight dirLights[MAX_DIR_LIGHTS];
	PointLight pointLights[MAX_POINT_LIGHTS];
	SpotLight spotLights[MAX_SPOT_LIGHTS];
	ShadowData dirLightShadowData;
	ShadowData spotLightShadowData;
	ShadowDataPointLight pointLightShadowData;
};

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

layout (std140, binding = 4) uniform IBLParams {
	int reflectionProbeMipCount;
	int computeIBL;
	int useSSAO;
	int _iblPad0;
};

// ===== Texture samplers (remain as individual uniforms) =====
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

uniform sampler2D dirLightShadowmap;

uniform sampler2D texture_albedo;
uniform sampler2D texture_normal;
uniform sampler2D texture_metallic;
uniform sampler2D texture_roughness;
uniform sampler2D texture_ao;
uniform sampler2D texture_displacement;
uniform sampler2D texture_emission;

// Light radiance calculations
vec3 CalculateDirectionalLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity);
vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity);
vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity);

// Cook-Torrance BRDF function prototypes
float NormalDistributionGGX(vec3 normal, vec3 halfway, float roughness);
float GeometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness);
float GeometrySchlickGGX(float cosTheta, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 baseReflectivity);

// Other function prototypes
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
	normal = normalize(normal * 2.0f - 1.0f);
	normal = normalize(TBN * normal);

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
		vec3 halfway = normalize(lightDir + fragToView);
		vec3 radiance = dirLights[i].direction.w * dirLights[i].lightColour.xyz;

		// Cook-Torrance Specular BRDF calculations
		float normalDistribution = NormalDistributionGGX(normal, halfway, roughness);
		vec3 fresnel = FresnelSchlick(max(dot(halfway, fragToView), 0.0), baseReflectivity);
		float geometry = GeometrySmith(normal, fragToView, lightDir, roughness);

		// Calculate reflected and refracted light respectively, and since metals absorb all refracted light, we nullify the diffuse lighting based on the metallic parameter
		vec3 specularRatio = fresnel;
		vec3 diffuseRatio = vec3(1.0) - specularRatio;
		diffuseRatio *= 1.0 - metallic;

		vec3 numerator = specularRatio * normalDistribution * geometry;
		float denominator = 4 * max(dot(fragToView, normal), 0.1) * max(dot(lightDir, normal), 0.0) + 0.001;  // Prevents any division by zero
		vec3 specular = numerator / denominator;

		// Also calculate the diffuse, a lambertian calculation will be added onto the final radiance calculation
		vec3 diffuse = diffuseRatio * albedo / PI;

		// Calculate shadows
		float shadowAmount = CalculateShadow(normal, lightDir);

		// return the radiance of the directional light
		directLightIrradiance += (diffuse + specular) * radiance * max(dot(normal, lightDir), 0.0) * (1.0 - shadowAmount);
	}

	return directLightIrradiance;
}

vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity) {
	vec3 pointLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.y; ++i) {
		vec3 fragToLight = normalize(pointLights[i].position.xyz - FragPos);
		vec3 halfway = normalize(fragToView + fragToLight);
		float fragToLightDistance = length(pointLights[i].position.xyz - FragPos);

		// Attenuation calculation (based on Epic's UE4 falloff model)
		float d = fragToLightDistance / pointLights[i].lightColour.w;  // attenuationRadius
		float d2 = d * d;
		float d4 = d2 * d2;
		float falloffNumerator = clamp(1.0 - d4, 0.0, 1.0);
		float attenuation = (falloffNumerator * falloffNumerator) / ((fragToLightDistance * fragToLightDistance) + 1.0);
		vec3 radiance = pointLights[i].position.w * pointLights[i].lightColour.xyz * attenuation;  // intensity * colour * attenuation

		// Cook-Torrance Specular BRDF calculations
		float normalDistribution = NormalDistributionGGX(normal, halfway, roughness);
		vec3 fresnel = FresnelSchlick(max(dot(halfway, fragToView), 0.0), baseReflectivity);
		float geometry = GeometrySmith(normal, fragToView, fragToLight, roughness);

		// Calculate reflected and refracted light respectively, and since metals absorb all refracted light, we nullify the diffuse lighting based on the metallic parameter
		vec3 specularRatio = fresnel;
		vec3 diffuseRatio = vec3(1.0) - specularRatio;
		diffuseRatio *= 1.0 - metallic;

		// Finally calculate the specular part of the Cook-Torrance BRDF
		vec3 numerator = specularRatio * normalDistribution * geometry;
		float denominator = 4 * max(dot(fragToView, normal), 0.1) * max(dot(fragToLight, normal), 0.0) + 0.001; // Prevents any division by zero
		vec3 specular = numerator / denominator;

		// Also calculate the diffuse, a lambertian calculation will be added onto the final radiance calculation
		vec3 diffuse = diffuseRatio * albedo / PI;

		// Add light radiance to the irradiance sum
		pointLightIrradiance += (diffuse + specular) * radiance * max(dot(normal, fragToLight), 0.0);
	}

	return pointLightIrradiance;
}

// 近似与半程矢量正确对齐的微面数量，从而确定镜面光的强度和面积
float NormalDistributionGGX(vec3 normal, vec3 halfway, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float normDotHalf = dot(normal, halfway);
	float normDotHalf2 = normDotHalf * normDotHalf;

	float numerator = a2;
	float denominator = normDotHalf2 * (a2 - 1) + 1;
	denominator = PI * denominator * denominator;

	return numerator / denominator;
}

vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity) {
	vec3 spotLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.z; ++i) {
		vec3 fragToLight = normalize(spotLights[i].position.xyz - FragPos);
		vec3 halfway = normalize(fragToView + fragToLight);
		float fragToLightDistance = length(spotLights[i].position.xyz - FragPos);

		// Attenuation calculation (based on Epic's UE4 falloff model)
		float d = fragToLightDistance / spotLights[i].direction.w;  // attenuationRadius
		float d2 = d * d;
		float d4 = d2 * d2;
		float falloffNumerator = clamp(1.0 - d4, 0.0, 1.0);

		// Check if it is in the spotlight's circle
		float theta = dot(normalize(spotLights[i].direction.xyz), -fragToLight);
		float difference = spotLights[i].lightColour.w - spotLights[i].params.x;  // cutOff - outerCutOff
		float intensity = clamp((theta - spotLights[i].params.x) / difference, 0.0, 1.0);
		float attenuation = intensity * (falloffNumerator * falloffNumerator) / ((fragToLightDistance * fragToLightDistance) + 1.0);
		vec3 radiance = spotLights[i].position.w * spotLights[i].lightColour.xyz * attenuation;

		// Cook-Torrance Specular BRDF calculations
		float normalDistribution = NormalDistributionGGX(normal, halfway, roughness);
		vec3 fresnel = FresnelSchlick(max(dot(halfway, fragToView), 0.0), baseReflectivity);
		float geometry = GeometrySmith(normal, fragToView, fragToLight, roughness);

		// 分别计算反射光和折射光，由于金属吸收所有折射光，因此我们根据金属参数取消漫射光
		vec3 specularRatio = fresnel;
		vec3 diffuseRatio = vec3(1.0) - specularRatio;
		diffuseRatio *= 1.0 - metallic;

		// 最后计算Cook-Torrance BRDF的镜面部分
		vec3 numerator = specularRatio * normalDistribution * geometry;
		float denominator = 4 * max(dot(fragToView, normal), 0.1) * max(dot(fragToLight, normal), 0.0) + 0.001; // Prevents any division by zero
		vec3 specular = numerator / denominator;

		// 还计算漫反射，lambertian计算将添加到最终的辐射计算中
		vec3 diffuse = diffuseRatio * albedo / PI;

		// Add light radiance to the irradiance sum
		spotLightIrradiance += (diffuse + specular) * radiance * max(dot(normal, fragToLight), 0.0);
	}

	return spotLightIrradiance;
}

// 在微面级别上分别近似几何阻挡和几何阴影
float GeometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
	return GeometrySchlickGGX(max(dot(normal, viewDir), 0.0), roughness) * GeometrySchlickGGX(max(dot(normal, lightDir), 0.0), roughness);
}

float GeometrySchlickGGX(float cosTheta, float roughness) {
	float r = (roughness + 1.0);
	float k = (roughness * roughness) / 8.0;

	float numerator = cosTheta;
	float denominator = cosTheta * (1.0 - k) + k;

	return numerator / max(denominator, 0.001);
}

// 计算镜面（反射）光的量。 由于漫反射（折射）和镜面反射（反射）是互斥的，
// 我们还可以用它来确定漫射光的量
vec3 FresnelSchlick(float cosTheta, vec3 baseReflectivity) {
	return max(baseReflectivity + (1.0 - baseReflectivity) * pow(2, (-5.55473 * cosTheta - 6.98316) * cosTheta), 0.0);
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
