#shader-type vertex
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texCoord;

out vec2 TexCoords;

void main() {
	gl_Position = vec4(position, 1.0);
	TexCoords = texCoord;
}


#shader-type fragment
#version 450 core

out vec4 FragColor;

in vec2 TexCoords;


uniform sampler2D screen_texture;
uniform float gamma_inverse;
uniform float exposure;

void main() {

	vec3 hdrColor = vec3(0.0);

	hdrColor = texture(screen_texture, TexCoords).rgb;

	// 应用简单的曝光色调图（HDR -> SDR）（黑暗场景应具有较高的曝光度，而明亮的场景应具有较低的曝光度）
	vec3 tonemappedColor = vec3(1.0) - exp(exposure * -hdrColor);

	// Apply gamma correction
	FragColor = vec4(pow(tonemappedColor, vec3(gamma_inverse)), 1.0);
}
