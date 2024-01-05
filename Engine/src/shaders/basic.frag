#version 450 core

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	sampler2D emission;

	float shininess;
};

struct Light {
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 color;

uniform Material material;
uniform Light light;

uniform vec3 viewPos;

uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;

void main() {
	// Ambient
	float ambientStrength = 0.1f;
	vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

	// Diffuse
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(light.position - FragPos); // points from the fragments position to the light source
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

	// Specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm); // reflect assumes the first vector points from the light source towards the fragments position
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

	vec3 emission = vec3(texture(material.emission, TexCoords));

	// Result
	vec3 result = ambient + diffuse + specular+emission;
	color = vec4(result, 1.0f);
}