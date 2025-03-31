#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out mat3 TBN;
out vec3 FragPos;
out vec4 FragPosLightClipSpace;
out vec2 TexCoords;

uniform mat3 normalMatrix;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceViewProjectionMatrix;

void main() {
	// Use the normal matrix to maintain the orthogonal property of a vector when it is scaled non-uniformly
	vec3 T = normalize(normalMatrix * tangent);
	vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * normal);
    TBN = mat3(T, B, N);

	FragPos = vec3(model * vec4(position, 1.0f));
	FragPosLightClipSpace = lightSpaceViewProjectionMatrix * vec4(FragPos, 1.0);
	TexCoords = texCoords;

	gl_Position = projection * view * vec4(FragPos, 1.0f);
}

#shader-type fragment
#version 450 core

struct Material {
	sampler2D texture_albedo;
	sampler2D texture_normal;
	sampler2D texture_metallic;
	sampler2D texture_roughness;
	sampler2D texture_ao;
};

struct DirLight {
	vec3 direction;

	vec3 lightColour; // radiant flux
};

struct PointLight {
	vec3 position;

	vec3 lightColour; // radiant flux
};

struct SpotLight {
	vec3 position;
	vec3 direction;

	float cutOff;
	float outerCutOff;

	vec3 lightColour; // radiant flux
};

#define MAX_POINT_LIGHTS 5
const float PI = 3.14159265359;

in mat3 TBN;
in vec3 FragPos;
in vec4 FragPosLightClipSpace;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;

// IBL
uniform int reflectionProbeMipCount;
uniform bool computeIBL;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

uniform sampler2D shadowmap;
uniform int numPointLights;
uniform DirLight dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLight;

uniform Material material;

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
	// Sample textures
	vec3 albedo = texture(material.texture_albedo, TexCoords).rgb;
	float albedoAlpha = texture(material.texture_albedo, TexCoords).w;
	vec3 normal = texture(material.texture_normal, TexCoords).rgb;
	float metallic = texture(material.texture_metallic, TexCoords).r;
	float unclampedRoughness = texture(material.texture_roughness, TexCoords).r; // ���ڼ�Ӿ��淴��
	float roughness = max(unclampedRoughness, 0.04);
	float ao = texture(material.texture_ao, TexCoords).r;

	// ������ͼ���롣 ѡ���˳����߿ռ䷨����ͼ����Ϊ�ұ��뽫���еƹ�ת��Ϊ���߿ռ�
	normal = normalize(normal * 2.0f - 1.0f);
	normal = normalize(TBN * normal);

	vec3 fragToView = normalize(viewPos - FragPos);
	vec3 reflectionVec = reflect(-fragToView, normal);

	// ����ʵ�ƽ���������淴����ԼΪ 0.04���������������������䣨���䣩����������䷴�������ڴ��澵�����������䣩
	vec3 baseReflectivity = vec3(0.04);
	baseReflectivity = mix(baseReflectivity, albedo, metallic);


	// ��������ֱ��������ÿ���������
	vec3 directLightIrradiance = vec3(0.0);
	directLightIrradiance += CalculateDirectionalLightRadiance(albedo, normal, metallic, roughness, fragToView, baseReflectivity);
	directLightIrradiance += CalculatePointLightRadiance(albedo, normal, metallic, roughness, fragToView, baseReflectivity);
	directLightIrradiance += CalculateSpotLightRadiance(albedo, normal, metallic, roughness, fragToView, baseReflectivity);

	// ����������;��淴��Ļ��� IBL
	vec3 ambient = vec3(0.03) * albedo * ao;
	if (computeIBL) {
		// ���㾵�淴����������ı���
		vec3 specularRatio = FresnelSchlick(max(dot(normal, fragToView), 0.0), baseReflectivity);

		vec3 diffuseRatio = vec3(1.0) - specularRatio;
		diffuseRatio *= 1.0 - metallic;

		// �������յ���������
		vec3 indirectDiffuse = texture(irradianceMap, normal).rgb * albedo;

		// ���淴����
		vec3 prefilterColour = textureLod(prefilterMap, reflectionVec, unclampedRoughness * (reflectionProbeMipCount - 1)).rgb;
		vec2 brdfIntegration = texture(brdfLUT, vec2(max(dot(normal, fragToView), 0.0), roughness)).rg;
		vec3 indirectSpecular = prefilterColour * (specularRatio * brdfIntegration.x + brdfIntegration.y);

		ambient = (diffuseRatio * indirectDiffuse + indirectSpecular) * ao;
		//ambient = vec3(brdfIntegration,0.0)*ao;
	}

	color = vec4(ambient + directLightIrradiance, albedoAlpha);
}

vec3 CalculateDirectionalLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity) {
	vec3 lightDir = normalize(-dirLight.direction);
	vec3 halfway = normalize(lightDir + fragToView);
	vec3 radiance = dirLight.lightColour;

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

	// return the radiance of the directional light
	return (diffuse + specular) * radiance * max(dot(normal, lightDir), 0.0) * 
	(1.0 - CalculateShadow(normal, lightDir));
}

vec3 CalculatePointLightRadiance(vec3 albedo, vec3 normal, float metallic, float roughness, vec3 fragToView, vec3 baseReflectivity) {
	vec3 pointLightIrradiance = vec3(0.0);

	for (int i = 0; i < numPointLights; ++i) {
		vec3 fragToLight = normalize(pointLights[i].position - FragPos);
		vec3 halfway = normalize(fragToView + fragToLight);
		float fragToLightDistance = length(pointLights[i].position - FragPos);
		float attenuation = 1.0 / (fragToLightDistance * fragToLightDistance);
		vec3 radiance = pointLights[i].lightColour * attenuation;

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

// ��������ʸ����ȷ�����΢���������Ӷ�ȷ��������ǿ�Ⱥ����
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

	vec3 fragToLight = normalize(spotLight.position - FragPos);
	vec3 halfway = normalize(fragToView + fragToLight);
	float fragToLightDistance = length(spotLight.position - FragPos);

	// Check if it is in the spotlight's circle
	float theta = dot(normalize(spotLight.direction), -fragToLight);
	float difference = spotLight.cutOff - spotLight.outerCutOff;
	float intensity = clamp((theta - spotLight.outerCutOff) / difference, 0.0, 1.0);
	float attenuation = intensity * (1.0 / (fragToLightDistance * fragToLightDistance));
	vec3 radiance = spotLight.lightColour * attenuation;

	// Cook-Torrance Specular BRDF calculations
	float normalDistribution = NormalDistributionGGX(normal, halfway, roughness);
	vec3 fresnel = FresnelSchlick(max(dot(halfway, fragToView), 0.0), baseReflectivity);
	float geometry = GeometrySmith(normal, fragToView, fragToLight, roughness);

	// �ֱ���㷴��������⣬���ڽ���������������⣬������Ǹ��ݽ�������ȡ�������
	vec3 specularRatio = fresnel;
	vec3 diffuseRatio = vec3(1.0) - specularRatio;
	diffuseRatio *= 1.0 - metallic;

	// ������Cook-Torrance BRDF�ľ��沿��
	vec3 numerator = specularRatio * normalDistribution * geometry;
	float denominator = 4 * max(dot(fragToView, normal), 0.1) * max(dot(fragToLight, normal), 0.0) + 0.001; // Prevents any division by zero
	vec3 specular = numerator / denominator;

	// �����������䣬lambertian���㽫��ӵ����յķ��������
	vec3 diffuse = diffuseRatio * albedo / PI;

	// Add light radiance to the irradiance sum
	return (diffuse + specular) * radiance * max(dot(normal, fragToLight), 0.0);
}

// ��΢�漶���Ϸֱ���Ƽ����赲�ͼ�����Ӱ
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

// ���㾵�棨���䣩������� ���������䣨���䣩�;��淴�䣨���䣩�ǻ���ģ�
// ���ǻ�����������ȷ����������
vec3 FresnelSchlick(float cosTheta, vec3 baseReflectivity) {
	return max(baseReflectivity + (1.0 - baseReflectivity) * pow(2, (-5.55473 * cosTheta - 6.98316) * cosTheta), 0.0);
}

float CalculateShadow(vec3 normal, vec3 fragToLight) {
	vec3 ndcCoords = FragPosLightClipSpace.xyz / FragPosLightClipSpace.w;
	vec3 depthmapCoords = ndcCoords * 0.5 + 0.5;

	float shadow = 0.0;
	float currentDepth = depthmapCoords.z;

	// �����Ӱƫ���Ա�����Ӱʧ�棬���ݷ��߷���͹��߷���֮��ĽǶ���Ҫ������Ӱƫ��
	// ̫���ƫ�ƻᵼ�� peter panning
	float shadowBias = max(0.01, 0.1 * (1.0 - dot(normal, fragToLight)));

	// ִ�аٷֱȽӽ����� (PCF) �Բ�����͵���Ӱ
	vec2 texelSize = 1.0 / textureSize(shadowmap, 0);
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			float sampledDepthPCF = texture(shadowmap, depthmapCoords.xy + (texelSize * vec2(x, y))).r;
			shadow += currentDepth > sampledDepthPCF + shadowBias ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;

	if (currentDepth > 1.0)
		shadow = 0.0;
	return shadow;
}
