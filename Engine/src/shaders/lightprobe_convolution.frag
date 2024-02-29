#version 450 core

out vec4 FragColor;

in vec3 SampleDirection;

uniform samplerCube sceneCaptureCubemap;

const  float PI = 3.14159265359;

void main() {
	// SampleDirection是卷积半球方向
	vec3 normal = normalize(SampleDirection);
	// 将向量从切线转换到世界空间，因为立方体贴图位于世界空间中
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, normal);
	up = cross(normal, right);

	vec3 irradiance = vec3(0.0);
	float sampleDelta = 0.025;
	float nrSamples = 0.0;
	for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
		for(float theta = 0.0;theta < 0.5 * PI; theta += sampleDelta)
		{
			// 切线空间坐标
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

			// 转为世界空间坐标
			vec3 worldSample = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

			irradiance += texture(sceneCaptureCubemap, worldSample).rgb *cos(theta)*sin(theta);
			nrSamples++;
		}
	}

	irradiance = PI * irradiance * (1.0 / float(nrSamples));

	FragColor = vec4(irradiance, 1.0);
}