#version 450 core

out vec4 FragColor;

in vec3 SampleDirection;

uniform samplerCube sceneCaptureCubemap;

void main() {
	// Sample direction is the hemisphere's orientation for convolution
	vec3 normal = normalize(SampleDirection);

	// Do the convolution (ie search the hemisphere and sample)
	vec3 hemisphereIrradiance = texture(sceneCaptureCubemap, normal).rgb;


	FragColor = vec4(hemisphereIrradiance, 1.0);
}