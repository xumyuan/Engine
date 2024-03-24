#version 450 core

out vec4 FragColor;

in vec2 TexCoords;


uniform sampler2D screen_texture;
uniform float gamma_inverse;
uniform float exposure;

void main() {

	vec3 hdrColor = vec3(0.0);

	hdrColor = texture(screen_texture, TexCoords).rgb;

	// Ӧ�ü򵥵��ع�ɫ��ͼ��HDR -> SDR�����ڰ�����Ӧ���нϸߵ��ع�ȣ��������ĳ���Ӧ���нϵ͵��ع�ȣ�
	vec3 tonemappedColor = vec3(1.0) - exp(exposure * -hdrColor);

	// Apply gamma correction
	FragColor = vec4(pow(tonemappedColor, vec3(gamma_inverse)), 1.0);
}