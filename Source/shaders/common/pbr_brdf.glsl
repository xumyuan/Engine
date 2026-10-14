#ifndef ENGINE_COMMON_PBR_BRDF_GLSL
#define ENGINE_COMMON_PBR_BRDF_GLSL

#ifndef ENGINE_PI_DEFINED
#define ENGINE_PI_DEFINED
const float PI = 3.14159265359;
#endif

// Unpacks the normal from the texture and returns the normal in tangent space
vec3 UnpackNormal(vec3 textureNormal) {
	return normalize(textureNormal * 2.0 - 1.0);
}

// Cook-Torrance BRDF functions adopted by Epic for UE4
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

float GeometrySchlickGGX(float cosTheta, float roughness) {
	float r = (roughness + 1.0);
	float k = (roughness * roughness) / 8.0;

	float numerator = cosTheta;
	float denominator = cosTheta * (1.0 - k) + k;

	return numerator / max(denominator, 0.001);
}

float GeometrySmith(vec3 normal, vec3 viewDirNorm, vec3 lightDirNorm, float roughness) {
	return GeometrySchlickGGX(max(dot(normal, viewDirNorm), 0.0), roughness) *
	       GeometrySchlickGGX(max(dot(normal, lightDirNorm), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 baseReflectivity) {
	return max(baseReflectivity + (1.0 - baseReflectivity) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta), 0.0);
}

// UE4 distance falloff model
float AttenuationUE4(float fragToLightDistance, float attenuationRadius) {
	float d = fragToLightDistance / attenuationRadius;
	float d2 = d * d;
	float d4 = d2 * d2;
	float falloffNumerator = clamp(1.0 - d4, 0.0, 1.0);
	return (falloffNumerator * falloffNumerator) / ((fragToLightDistance * fragToLightDistance) + 1.0);
}

// Evaluates diffuse + specular Cook-Torrance BRDF for a single light direction
vec3 EvaluateDirectBRDF(vec3 albedo, vec3 normal, float metallic, float roughness,
                        vec3 fragToViewNorm, vec3 lightDirNorm, vec3 baseReflectivity) {
	vec3 halfwayNorm = normalize(lightDirNorm + fragToViewNorm);

	float normalDistribution = NormalDistributionGGX(normal, halfwayNorm, roughness);
	vec3 fresnel = FresnelSchlick(max(dot(halfwayNorm, fragToViewNorm), 0.0), baseReflectivity);
	float geometry = GeometrySmith(normal, fragToViewNorm, lightDirNorm, roughness);

	vec3 specularRatio = fresnel;
	vec3 diffuseRatio = (vec3(1.0) - specularRatio) * (1.0 - metallic);

	vec3 numerator = specularRatio * normalDistribution * geometry;
	float denominator = 4.0 * max(dot(fragToViewNorm, normal), 0.1) * max(dot(lightDirNorm, normal), 0.0) + 0.001;
	vec3 specular = numerator / denominator;

	vec3 diffuse = diffuseRatio * albedo / PI;
	return diffuse + specular;
}

#endif
