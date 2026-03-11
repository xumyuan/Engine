#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out mat3 TBN;
out vec2 TexCoords;

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
	mat3 normalMatrix;
};

void main() {
	// Use the normal matrix to maintain the orthogonal property of a vector when it is scaled non-uniformly
	vec3 T = normalize(normalMatrix * tangent);
	vec3 B = normalize(normalMatrix * bitangent);
	vec3 N = normalize(normalMatrix * normal);
	TBN = mat3(T, B, N);

	TexCoords = texCoords;

	gl_Position = projection * view * model * vec4(position, 1.0);
}




#shader-type fragment
#version 450 core

layout (location = 0) out vec4 gb_Albedo;
layout (location = 1) out vec3 gb_Normal;
layout (location = 2) out vec4 gb_MaterialInfo;

layout (std140, binding = 3) uniform MaterialParams {
	vec4 albedoColour;
	vec4 emissionColour;
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

// Terrain texture samplers
uniform sampler2D texture_albedo1;
uniform sampler2D texture_albedo2;
uniform sampler2D texture_albedo3;
uniform sampler2D texture_albedo4;
uniform sampler2D texture_normal1;
uniform sampler2D texture_normal2;
uniform sampler2D texture_normal3;
uniform sampler2D texture_normal4;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_roughness2;
uniform sampler2D texture_roughness3;
uniform sampler2D texture_roughness4;
uniform sampler2D texture_metallic1;
uniform sampler2D texture_metallic2;
uniform sampler2D texture_metallic3;
uniform sampler2D texture_metallic4;
uniform sampler2D texture_AO1;
uniform sampler2D texture_AO2;
uniform sampler2D texture_AO3;
uniform sampler2D texture_AO4;
uniform sampler2D blendmap;

in mat3 TBN;
in vec2 TexCoords;

// Functions
vec3 UnpackNormal(vec3 textureNormal);

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
	roughness = max(roughness, 0.04);

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

	gb_Albedo = vec4(albedo, 1.0);
	gb_Normal = normal;
	gb_MaterialInfo = vec4(metallic, roughness, ao, 0.0);
}

// Unpacks the normal from the texture and returns the normal in tangent space
vec3 UnpackNormal(vec3 textureNormal) {
	return normalize(textureNormal * 2.0 - 1.0);
}
