#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out mat3 TBN;
out vec2 TexCoords;
out vec3 FragPosTangentSpace;
out vec3 ViewPosTangentSpace;

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

void main() {
	// Use the normal matrix to maintain the orthogonal property of a vector when it is scaled non-uniformly
	vec3 T = normalize(normalMatrix * tangent);
	vec3 B = normalize(normalMatrix * bitangent);
	vec3 N = normalize(normalMatrix * normal);
	TBN = mat3(T, B, N);

	TexCoords = texCoords;
	vec3 fragPos = vec3(model * vec4(position, 1.0f));
	if (hasDisplacement != 0) {
		mat3 inverseTBN = transpose(TBN); // Calculate matrix to go from world -> tangent (orthogonal matrix's transpose = inverse)
		FragPosTangentSpace = inverseTBN * fragPos;
		ViewPosTangentSpace = inverseTBN * viewPos.xyz;
	}

	gl_Position = projection * view * vec4(fragPos, 1.0);
}




#shader-type fragment
#version 450 core

layout (location = 0) out vec4 gb_Albedo;
layout (location = 1) out vec3 gb_Normal;
layout (location = 2) out vec4 gb_MaterialInfo;

in mat3 TBN;
in vec2 TexCoords;
in vec3 FragPosTangentSpace;
in vec3 ViewPosTangentSpace;

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

// Texture samplers remain as individual uniforms
uniform sampler2D texture_albedo;
uniform sampler2D texture_normal;
uniform sampler2D texture_metallic;
uniform sampler2D texture_roughness;
uniform sampler2D texture_ao;
uniform sampler2D texture_displacement;
uniform sampler2D texture_emission;

// Functions
vec3 UnpackNormal(vec3 textureNormal);
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDirTangentSpace);

void main() {
	// Parallax mapping
	vec2 textureCoordinates = TexCoords;
	if (hasDisplacement != 0) {
		vec3 viewDirTangentSpace = normalize(ViewPosTangentSpace - FragPosTangentSpace);
		textureCoordinates = ParallaxMapping(TexCoords, viewDirTangentSpace);
	}

	// Sample textures and build up the GBuffer
	vec4 albedo = (hasAlbedoTexture != 0) ? texture(texture_albedo, textureCoordinates).rgba * albedoColour : albedoColour;

	// If we have emission, hijack the albedo and replace it with the emission colour. Then since we want HDR values and albedo RT is LDR, we can store the emission intensity in the alpha of the gb_MaterialInfo RT
	bool overwriteAlbedoWithEmission = false;
	if (hasEmission != 0) {
		if (hasEmissionTexture != 0) {
			vec3 emissiveSample = texture(texture_emission, textureCoordinates).rgb;

			// Check emission map sample, if it is black (ie. no emission) then we just skip emission for this fragment, otherwise hijack the albedo RT
			if (!all(equal(emissiveSample, vec3(0.0)))) {
				albedo = vec4(emissiveSample, 1.0);
				overwriteAlbedoWithEmission = true;
			}
		}
		else {
			albedo = vec4(emissionColour.xyz, 1.0);
			overwriteAlbedoWithEmission = true;
		}
	}

	vec3 normal = texture(texture_normal, textureCoordinates).rgb;
	float metallic = (hasMetallicTexture != 0) ? texture(texture_metallic, textureCoordinates).r : metallicValue;
	float roughness = (hasRoughnessTexture != 0) ? texture(texture_roughness, textureCoordinates).r : roughnessValue;
	float ao = texture(texture_ao, textureCoordinates).r;
	float emissionIntensity = overwriteAlbedoWithEmission ? emissionColour.w / 255.0 : 0.0; // Converting u8 [0, 255] -> float [0.0, 1.0]

	// Normal mapping code. Opted out of tangent space normal mapping since I would have to convert all of my lights to tangent space
	normal = normalize(TBN * UnpackNormal(normal));

	gb_Albedo = albedo;
	gb_Normal = normal;
	gb_MaterialInfo = vec4(metallic, roughness, ao, emissionIntensity);
}

// Unpacks the normal from the texture and returns the normal in tangent space
vec3 UnpackNormal(vec3 textureNormal) {
	return normalize(textureNormal * 2.0 - 1.0);
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDirTangentSpace) {
	// Figure out the LoD we should sample from while raymarching the heightfield in tangent space. Required to fix an artifacting issue
	vec2 lodInfo = textureQueryLod(texture_displacement, texCoords);
	float lodToSample = lodInfo.x;
	float expectedLod = lodInfo.y; // Even if mip mapping isn't enabled this will still give us a mip level

	const float minSteps = minMaxDisplacementSteps.x;
	const float maxSteps = minMaxDisplacementSteps.y;
	float numSteps = mix(maxSteps, minSteps, clamp(expectedLod * 0.4, 0, 1)); // More steps are required at lower mip levels since the camera is closer to the surface

	float layerDepth = 1.0 / numSteps;
	float currentLayerDepth = 0.0;

	// Calculate the direction and the amount we should raymarch each iteration
	vec2 p = viewDirTangentSpace.xy * parallaxStrength;
	vec2 deltaTexCoords = p / numSteps;

	// Get the initial values
	vec2 currentTexCoords = texCoords;
	float currentSampledDepth = textureLod(texture_displacement, currentTexCoords, lodToSample).r;

	// Keep ray marching along vector p by the texture coordinate delta, until the raymarching depth catches up to the sampled depth (ie the -view vector intersects the surface)
	while (currentLayerDepth < currentSampledDepth) {
		currentTexCoords -= deltaTexCoords;
		currentSampledDepth = textureLod(texture_displacement, currentTexCoords, lodToSample).r;
		currentLayerDepth += layerDepth;
	}

	// Now we need to get the previous step and the current step, and interpolate between the two texture coordinates
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
	float afterDepth = currentSampledDepth - currentLayerDepth;
	float beforeDepth = textureLod(texture_displacement, prevTexCoords, lodToSample).r - currentLayerDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = mix(currentTexCoords, prevTexCoords, weight);

	return finalTexCoords;
}
