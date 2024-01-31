#version 450 core

out vec4 FragColor;

in vec2 TexCoords;


uniform sampler2D screen_texture;
uniform float gamma_inverse;
uniform vec2 read_offset;
uniform bool blur_enabled;

void main() {
	// Sample the fragments around the current fragment for post processing using kernels (convulution matrices)
	// Note: 后处理可能会导致锯齿，因为它使用位块传输帧缓冲区（非多重采样缓冲区）
	vec2 readOffsets[9] = vec2[] (
		vec2(-read_offset.x, read_offset.y),
		vec2(0.0, read_offset.y),
		vec2(read_offset.x, read_offset.y),
		vec2(-read_offset.x, 0.0),
		vec2(0.0, 0.0),
		vec2(read_offset.x, 0.0),
		vec2(-read_offset.x, -read_offset.y),
		vec2(0.0, -read_offset.y),
		vec2(read_offset.x, -read_offset.y)
	);

	vec3 hdrColour = vec3(0.0);
	if (blur_enabled) {
		// Blur kernel
		float kernel[9] = float[] (
			1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
			2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
			1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
		);

		// Apply the kernel (post-processing effect)
		for(int i = 0; i < 9; ++i) {
			hdrColour += texture(screen_texture, TexCoords + readOffsets[i]).rgb * kernel[i];
		}
	}
	else {
		hdrColour = texture(screen_texture, TexCoords).rgb;
	}

	// Apply a simple Reinhard tonemapper (HDR -> SDR)
	vec3 tonemappedColour = hdrColour / (hdrColour + vec3(1.0));

	// Apply gamma correction
	FragColor = vec4(pow(tonemappedColour, vec3(gamma_inverse)), 1.0);
}