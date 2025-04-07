#shader-type vertex

#version 450 core

layout (location = 0) in vec3 position;

out vec3 SampleDirection;

uniform mat4 view;
uniform mat4 projection;

void main() {
	SampleDirection = position;

	// �ж���ͼ�����ƽ�Ʋ��֣���������岻��ƽ�ƣ���Ϊ����ϣ��������Χ��̽���ԭ�㣩
	gl_Position = projection * mat4(mat3(view)) * vec4(position, 1.0f); 
}

#shader-type fragment
#version 450 core

out vec4 FragColor;

in vec3 SampleDirection;

uniform samplerCube sceneCaptureCubemap;

const  float PI = 3.14159265359;

void main() {
	// SampleDirection�Ǿ��������
	vec3 normal = normalize(SampleDirection);
	// ������������ת��������ռ䣬��Ϊ��������ͼλ������ռ���
	//vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 up = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 right = normalize(cross(up, normal));
	up = cross(normal, right);

	vec3 irradiance = vec3(0.0, 0.0, 0.0);
	float sampleDelta = 0.05;
	float nrSamples = 0.0;

	for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
		for(float theta = 0.0;theta < 0.5 * PI; theta += sampleDelta)
		{
			// ���߿ռ�����
			vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));

			// תΪ����ռ�����
			vec3 worldSample = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

			irradiance += texture(sceneCaptureCubemap, worldSample).rgb * cos(theta) * sin(theta);
			nrSamples+=1.0;
		}
	}

	irradiance = PI * irradiance * (1.0 / float(nrSamples));

	FragColor = vec4(irradiance, 1.0);
}