#version 450 core

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	sampler2D emission;

	float shininess;
};

struct Light {
	vec3 direction;

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
uniform float time;

uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;

void main() {

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(-light.direction);
	// Ambient
	float ambientStrength = 0.1f;
	vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

	// Diffuse
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

	// Specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm); // reflect assumes the first vector points from the light source towards the fragments position
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

	// Emission
	vec3 emission = vec3(texture(material.emission, TexCoords)) + abs(sin(time));

	// Result
	vec3 result = ambient + diffuse + specular ;
	color = vec4(result, 1.0f);
}