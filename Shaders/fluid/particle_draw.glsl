#shader-type vertex
#version 450 core

layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;
uniform float pointScale;
uniform float pointSize;

out vec3 eyeSpacePos;
out vec3 eyeSpaceLightPos;

void main() {
	eyeSpacePos = (view * model * vec4(position, 1.0f)).xyz;
	eyeSpaceLightPos = (view * vec4(lightPos, 1.0f)).xyz;
	gl_PointSize = -pointScale * pointSize / eyeSpacePos.z;
	gl_Position = projection * view * model * vec4(position, 1.0f);
}

#shader-type fragment
#version 450 core

uniform float pointSize;
uniform mat4 projection;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

in vec3 eyeSpacePos;
in vec3 eyeSpaceLightPos;

out vec4 FragColor;

vec3 BlinnPhong(vec3 normal) {
	// ambient
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;

	// diffuse 
	vec3 norm = normalize(normal);
	vec3 lightDir = normalize(eyeSpaceLightPos - eyeSpacePos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	// specular
	float specularStrength = 0.5;
	vec3 viewDir = normalize(vec3(0.0f) - eyeSpacePos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;

	vec3 result = (ambient + diffuse * 5.0f + specular) * objectColor;
	return result;
}

void main() {

	vec3 normal;
	normal.xy = gl_PointCoord.xy * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float mag = dot(normal.xy, normal.xy);
	if (mag > 1.0) discard;
	normal.z = sqrt(1.0 - mag);

	vec4 pixelEyePos = vec4(eyeSpacePos + normal * pointSize, 1.0f);
	vec4 pixelClipPos = projection * pixelEyePos;
	float ndcZ = pixelClipPos.z / pixelClipPos.w;
	gl_FragDepth = ndcZ;
	vec3 depth = vec3(ndcZ, ndcZ, ndcZ);

	vec3 color = BlinnPhong(normal);

	FragColor = vec4(color, 1.0f);
}
