#shader-type vertex
#version 450 core

layout(location = 0) in vec3 position;

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

layout (std140, binding = 4) uniform FluidParams {
	vec4 lightPos;      // xyz = lightPos, w = pointScale
	vec4 lightColor;    // xyz = lightColor, w = pointSize
	vec4 objectColor;   // xyz = objectColor, w = padding
};

out vec3 eyeSpacePos;
out vec3 eyeSpaceLightPos;

void main() {
	eyeSpacePos = (view * model * vec4(position, 1.0f)).xyz;
	eyeSpaceLightPos = (view * vec4(lightPos.xyz, 1.0f)).xyz;
	float pointScale = lightPos.w;
	float pointSize = lightColor.w;
	gl_PointSize = -pointScale * pointSize / eyeSpacePos.z;
	gl_Position = projection * view * model * vec4(position, 1.0f);
}

#shader-type fragment
#version 450 core

layout (std140, binding = 0) uniform PerFrame {
	mat4 view;
	mat4 projection;
	mat4 viewInverse;
	mat4 projectionInverse;
	vec4 viewPos;
	vec2 screenSize;
	vec2 texelSize;
};

layout (std140, binding = 4) uniform FluidParams {
	vec4 lightPos;      // xyz = lightPos, w = pointScale
	vec4 lightColor;    // xyz = lightColor, w = pointSize
	vec4 objectColor;   // xyz = objectColor, w = padding
};

in vec3 eyeSpacePos;
in vec3 eyeSpaceLightPos;

out vec4 FragColor;

vec3 BlinnPhong(vec3 normal) {
	// ambient
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor.xyz;

	// diffuse 
	vec3 norm = normalize(normal);
	vec3 lightDir = normalize(eyeSpaceLightPos - eyeSpacePos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor.xyz;

	// specular
	float specularStrength = 0.5;
	vec3 viewDir = normalize(vec3(0.0f) - eyeSpacePos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor.xyz;

	vec3 result = (ambient + diffuse * 5.0f + specular) * objectColor.xyz;
	return result;
}

void main() {

	vec3 normal;
	normal.xy = gl_PointCoord.xy * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float mag = dot(normal.xy, normal.xy);
	if (mag > 1.0) discard;
	normal.z = sqrt(1.0 - mag);

	float pointSize = lightColor.w;
	vec4 pixelEyePos = vec4(eyeSpacePos + normal * pointSize, 1.0f);
	vec4 pixelClipPos = projection * pixelEyePos;
	float ndcZ = pixelClipPos.z / pixelClipPos.w;
	gl_FragDepth = ndcZ;
	vec3 depth = vec3(ndcZ, ndcZ, ndcZ);

	vec3 color = BlinnPhong(normal);

	FragColor = vec4(color, 1.0f);
}
