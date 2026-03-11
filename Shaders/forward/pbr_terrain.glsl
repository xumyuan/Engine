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
in vec2 TexCoords;
in vec3 FragPos;

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

// ===== Texture samplers (remain as individual uniforms) =====
// Shadow
uniform sampler2D dirLightShadowmap;
uniform sampler2D spotLightShadowmap;
uniform samplerCube pointLightShadowCubemap;

// Terrain material textures
uniform sampler2D texture_albedo1;   // background texture
uniform sampler2D texture_albedo2;   // r texture
uniform sampler2D texture_albedo3;   // g texture
uniform sampler2D texture_albedo4;   // b texture

uniform sampler2D texture_normal1;   // background texture
uniform sampler2D texture_normal2;   // r texture
uniform sampler2D texture_normal3;   // g texture
uniform sampler2D texture_normal4;   // b texture

uniform sampler2D texture_roughness1; // background texture
uniform sampler2D texture_roughness2; // r texture
uniform sampler2D texture_roughness3; // g texture
uniform sampler2D texture_roughness4; // b texture

uniform sampler2D texture_metallic1; // background texture
uniform sampler2D texture_metallic2; // r texture
uniform sampler2D texture_metallic3; // g texture
uniform sampler2D texture_metallic4; // b texture

uniform sampler2D texture_AO1;      // background texture
uniform sampler2D texture_AO2;      // r texture
uniform sampler2D texture_AO3;      // g texture
uniform sampler2D texture_AO4;      // b texture

uniform sampler2D blendmap;

// Light radiance calculations
vec3 CalculateDirectionalLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity);
vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity);
vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity);

// Cook-Torrance BRDF functions adopted by Epic for UE4
float NormalDistributionGGX(vec3 normal, vec3 halfwayNorm, float roughness);
float GeometrySmith(vec3 normal, vec3 viewDirNorm, vec3 lightDirNorm, float roughness);
float GeometrySchlickGGX(float cosTheta, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 baseReflectivity);

// Other function prototypes
vec3 UnpackNormal(vec3 textureNormal);
float CalculateDirLightShadow();
float CalculateSpotLightShadow();
float CalculatePointLightShadow(vec3 lightToFrag);

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
		vec3 halfwayNorm = normalize(lightDirNorm + fragToViewNorm);
		vec3 radiance = dirLights[i].direction.w * dirLights[i].lightColour.xyz;

		// Cook-Torrance Specular BRDF calculations
		float normalDistribution = NormalDistributionGGX(normal, halfwayNorm, roughness);
		vec3 fresnel = FresnelSchlick(max(dot(halfwayNorm, fragToViewNorm), 0.0), baseReflectivity);
		float geometry = GeometrySmith(normal, fragToViewNorm, lightDirNorm, roughness);

		// Calculate reflected and refracted light respectively, and since metals absorb all refracted light, we nullify the diffuse lighting based on the metallic parameter
		vec3 specularRatio = fresnel;
		vec3 diffuseRatio = vec3(1.0) - specularRatio;
		diffuseRatio *= 1.0 - metallic;

		// Finally calculate the specular part of the Cook-Torrance BRDF (max 0.1 stops any visual artifacts)
		vec3 numerator = specularRatio * normalDistribution * geometry;
		float denominator = 4 * max(dot(fragToViewNorm, normal), 0.1) * max(dot(lightDirNorm, normal), 0.0) + 0.001;  // Prevents any division by zero
		vec3 specular = numerator / denominator;

		// Also calculate the diffuse, a lambertian calculation will be added onto the final radiance calculation
		vec3 diffuse = diffuseRatio * albedo / PI;

		// Calculate shadows, but first check to make sure the current light index is the shadow caster
		float shadowAmount = 0.0f;
		shadowAmount = CalculateDirLightShadow();

		// Add the light's radiance to the irradiance sum
		directLightIrradiance += (diffuse + specular) * radiance * max(dot(normal, lightDirNorm), 0.0) * (1.0 - shadowAmount);
	}

	return directLightIrradiance;
}


vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity) {
	vec3 pointLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.y; ++i) {
		vec3 fragToLightNorm = normalize(pointLights[i].position.xyz - FragPos);
		vec3 halfwayNorm = normalize(fragToViewNorm + fragToLightNorm);
		vec3 lightToFrag = FragPos - pointLights[i].position.xyz;
		float fragToLightDistance = length(lightToFrag);

		// Attenuation calculation (based on Epic's UE4 falloff model)
		float d = fragToLightDistance / pointLights[i].lightColour.w;  // attenuationRadius
		float d2 = d * d;
		float d4 = d2 * d2;
		float falloffNumerator = clamp(1.0 - d4, 0.0, 1.0);
		float attenuation = (falloffNumerator * falloffNumerator) / ((fragToLightDistance * fragToLightDistance) + 1.0);
		vec3 radiance = pointLights[i].position.w * pointLights[i].lightColour.xyz * attenuation;

		// Cook-Torrance Specular BRDF calculations
		float normalDistribution = NormalDistributionGGX(normal, halfwayNorm, roughness);
		vec3 fresnel = FresnelSchlick(max(dot(halfwayNorm, fragToViewNorm), 0.0), baseReflectivity);
		float geometry = GeometrySmith(normal, fragToViewNorm, fragToLightNorm, roughness);

		// Calculate reflected and refracted light respectively, and since metals absorb all refracted light, we nullify the diffuse lighting based on the metallic parameter
		vec3 specularRatio = fresnel;
		vec3 diffuseRatio = vec3(1.0) - specularRatio;
		diffuseRatio *= 1.0 - metallic;

		// Finally calculate the specular part of the Cook-Torrance BRDF (max 0.1 stops any visual artifacts)
		vec3 numerator = specularRatio * normalDistribution * geometry;
		float denominator = 4 * max(dot(fragToViewNorm, normal), 0.1) * max(dot(fragToLightNorm, normal), 0.0) + 0.001; // Prevents any division by zero
		vec3 specular = numerator / denominator;

		// Also calculate the diffuse, a lambertian calculation will be added onto the final radiance calculation
		vec3 diffuse = diffuseRatio * albedo / PI;

		// Calculate shadows, but first check to make sure the current light index is the shadow caster
		float shadowAmount = 0.0f;
		/*if (i == pointLightShadowData.lightShadowIndex)
			shadowAmount = CalculatePointLightShadow(lightToFrag);*/

		// Add the light's radiance to the irradiance sum
		pointLightIrradiance += (diffuse + specular) * radiance * max(dot(normal, fragToLightNorm), 0.0) * (1.0 - shadowAmount);
	}

	return pointLightIrradiance;
}


vec3 CalculateSpotLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToViewNorm, vec3 baseReflectivity) {
	vec3 spotLightIrradiance = vec3(0.0);

	for (int i = 0; i < numDirPointSpotLights.z; ++i) {
		vec3 fragToLightNorm = normalize(spotLights[i].position.xyz - FragPos);
		vec3 halfwayNorm = normalize(fragToViewNorm + fragToLightNorm);
		float fragToLightDistance = length(spotLights[i].position.xyz - FragPos);

		// Attenuation calculation (based on Epic's UE4 falloff model)
		float d = fragToLightDistance / spotLights[i].direction.w;  // attenuationRadius
		float d2 = d * d;
		float d4 = d2 * d2;
		float falloffNumerator = clamp(1.0 - d4, 0.0, 1.0);

		// Check if it is in the spotlight's circle
		float theta = dot(normalize(spotLights[i].direction.xyz), -fragToLightNorm);
		float difference = spotLights[i].lightColour.w - spotLights[i].params.x;  // cutOff - outerCutOff
		float intensity = clamp((theta - spotLights[i].params.x) / difference, 0.0, 1.0);
		float attenuation = intensity * (falloffNumerator * falloffNumerator) / ((fragToLightDistance * fragToLightDistance) + 1.0);
		vec3 radiance = spotLights[i].position.w * spotLights[i].lightColour.xyz * attenuation;

		// Cook-Torrance Specular BRDF calculations
		float normalDistribution = NormalDistributionGGX(normal, halfwayNorm, roughness);
		vec3 fresnel = FresnelSchlick(max(dot(halfwayNorm, fragToViewNorm), 0.0), baseReflectivity);
		float geometry = GeometrySmith(normal, fragToViewNorm, fragToLightNorm, roughness);

		// Calculate reflected and refracted light respectively, and since metals absorb all refracted light, we nullify the diffuse lighting based on the metallic parameter
		vec3 specularRatio = fresnel;
		vec3 diffuseRatio = vec3(1.0) - specularRatio;
		diffuseRatio *= 1.0 - metallic;

		// Finally calculate the specular part of the Cook-Torrance BRDF (max 0.1 stops any visual artifacts)
		vec3 numerator = specularRatio * normalDistribution * geometry;
		float denominator = 4 * max(dot(fragToViewNorm, normal), 0.1) * max(dot(fragToLightNorm, normal), 0.0) + 0.001; // Prevents any division by zero
		vec3 specular = numerator / denominator;

		// Also calculate the diffuse, a lambertian calculation will be added onto the final radiance calculation
		vec3 diffuse = diffuseRatio * albedo / PI;

		// Calculate shadows, but first check to make sure the current light index is the shadow caster
		float shadowAmount = 0.0f;
		/*if (i == spotLightShadowData.lightShadowIndex)
			shadowAmount = CalculateSpotLightShadow();*/

		// Add the light's radiance to the irradiance sum
		spotLightIrradiance += (diffuse + specular) * radiance * max(dot(normal, fragToLightNorm), 0.0) * (1.0 - shadowAmount);
	}

	return spotLightIrradiance;
}


// Approximates the amount of microfacets that are properly aligned with the halfway vector, thus determines the strength and area for specular light
float NormalDistributionGGX(vec3 normal, vec3 halfwayNorm, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float normDotHalf = dot(normal, halfwayNorm);
	float normDotHalf2 = normDotHalf * normDotHalf;

	float numerator = a2;
	float denominator = normDotHalf2 * (a2 - 1.0) + 1.0;
	denominator = PI * denominator * denominator;

	return numerator / denominator;
}


// Approximates the geometry obstruction and geometry shadowing respectively, on the microfacet level
float GeometrySmith(vec3 normal, vec3 viewDirNorm, vec3 lightDirNorm, float roughness) {
	return GeometrySchlickGGX(max(dot(normal, viewDirNorm), 0.0), roughness) * GeometrySchlickGGX(max(dot(normal, lightDirNorm), 0.0), roughness);
}
float GeometrySchlickGGX(float cosTheta, float roughness) {
	float r = (roughness + 1.0);
	float k = (roughness * roughness) / 8.0;

	float numerator = cosTheta;
	float denominator = cosTheta * (1.0 - k) + k;

	return numerator / max(denominator, 0.001);
}


// Calculates the amount of specular light. Since diffuse(refraction) and specular(reflection) are mutually exclusive, 
// we can also use this to determine the amount of diffuse light
// Taken from UE4's implementation which is faster and basically identical to the usual Fresnel calculations: https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
vec3 FresnelSchlick(float cosTheta, vec3 baseReflectivity) {
	return max(baseReflectivity + (1.0 - baseReflectivity) * pow(2, (-5.55473 * cosTheta - 6.98316) * cosTheta), 0.0);
}


// Unpacks the normal from the texture and returns the normal in tangent space
vec3 UnpackNormal(vec3 textureNormal) {
	return normalize(textureNormal * 2.0 - 1.0);
}


float CalculateDirLightShadow() {
	if (dirLightShadowData.lightShadowIndex == -1)
		return 0.0;

	vec4 fragPosLightClipSpace = dirLightShadowData.lightSpaceViewProjectionMatrix * vec4(FragPos, 1.0);
	vec3 ndcCoords = fragPosLightClipSpace.xyz / fragPosLightClipSpace.w;
	vec3 depthmapCoords = ndcCoords * 0.5 + 0.5;

	float currentDepth = depthmapCoords.z;
	if (currentDepth > 1.0)
		return 0.0;

	// Perform Percentage Closer Filtering (PCF) in order to produce soft shadows - Use bilinear filtering to get some free samples (4 bilinear samples, 4 samples -> 16 values actually processed)
	float shadow = 0.0;
	vec2 shadowTexelSize = 1.0 / textureSize(dirLightShadowmap, 0);
	for (float y = -1.5; y < 1.0; y += 2.0) {
		for (float x = -1.5; x < 1.0; x += 2.0) {
			float sampledDepthPCF = texture(dirLightShadowmap, depthmapCoords.xy + (shadowTexelSize * vec2(x, y))).r;
			shadow += (currentDepth > sampledDepthPCF + dirLightShadowData.shadowBias) ? 1.0 : 0.0; // Add shadow bias to avoid shadow acne. However too much bias can cause peter panning
		}
	}
	shadow *= 0.25;

	return shadow;
}

float CalculateSpotLightShadow() {
	if (spotLightShadowData.lightShadowIndex == -1)
		return 0.0;

	vec4 fragPosLightClipSpace = spotLightShadowData.lightSpaceViewProjectionMatrix * vec4(FragPos, 1.0);
	vec3 ndcCoords = fragPosLightClipSpace.xyz / fragPosLightClipSpace.w;
	vec3 depthmapCoords = ndcCoords * 0.5 + 0.5;

	float currentDepth = depthmapCoords.z;
	if (currentDepth > 1.0)
		return 0.0;

	// Perform Percentage Closer Filtering (PCF) in order to produce soft shadows - Use bilinear filtering to get some free samples (4 bilinear samples, 4 samples -> 16 values actually processed)
	float shadow = 0.0;
	vec2 shadowTexelSize = 1.0 / textureSize(spotLightShadowmap, 0);
	for (float y = -1.5; y < 1.0; y += 2.0) {
		for (float x = -1.5; x < 1.0; x += 2.0) {
			float sampledDepthPCF = texture(spotLightShadowmap, depthmapCoords.xy + (shadowTexelSize * vec2(x, y))).r;
			shadow += currentDepth > sampledDepthPCF + spotLightShadowData.shadowBias ? 1.0 : 0.0; // Add shadow bias to avoid shadow acne. However too much bias can cause peter panning
		}
	}
	shadow *= 0.25;

	return shadow;
}

vec3 sampleOffsetDirections[20] = vec3[]
(
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float CalculatePointLightShadow(vec3 lightToFrag) {
	if (pointLightShadowData.lightShadowIndex == -1)
		return 0.0;

	float currentDepth = length(lightToFrag);
	if (currentDepth > pointLightShadowData.farPlane)
		return 0.0;

	float shadow = 0.0;
	float samples = 20;
	float diskRadius = 0.05;
	for (int i = 0; i < samples; i++)
	{
		float closestDepth = texture(pointLightShadowCubemap, lightToFrag + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= pointLightShadowData.farPlane; // undo the [0,1] mapping
		if (currentDepth - pointLightShadowData.shadowBias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);

	return shadow;
}
