#version 330 core

struct MatTexMap_t {
	sampler2D texture1;
	sampler2D texture2;
};

const float offset = 1.0 / 300.0;
vec2 offsets[9] = vec2[] (
	vec2(-offset,  offset), vec2( 0.0,     offset), vec2( offset,  offset),
	vec2(-offset,  0.0),    vec2( 0.0,     0.0),    vec2( offset,  0.0),   
	vec2(-offset, -offset), vec2( 0.0,    -offset), vec2( offset, -offset) 
);

vec4 origin(sampler2D tid);
vec4 inversion(sampler2D tid);
vec4 grayscale(sampler2D tid);
vec4 nightversion(sampler2D tid);

vec4 sharpen(sampler2D tid);
vec4 blur(sampler2D tid);
vec4 edge(sampler2D tid);
vec4 test_kernel(sampler2D tid);

out vec4 FragColor;

in vec2 TexCoords;

uniform MatTexMap_t uMaterial;

uniform int uProcessMode;

void main() {

	/**
	* Post-processing
	* Now that the entire scene is rendered to a single texture we can create
	* some interesting effects simply by manipulating the texture data
	*/

	if (uProcessMode == 0)
		FragColor = origin(uMaterial.texture1);
	else if (uProcessMode == 1)
		FragColor = sharpen(uMaterial.texture1);
	else if (uProcessMode == 2)
		FragColor = blur(uMaterial.texture1);
	else if (uProcessMode == 3)
		FragColor = edge(uMaterial.texture1);
	else if (uProcessMode == 4)
		FragColor = test_kernel(uMaterial.texture1);
}

vec4 origin(sampler2D tid) {
	return texture(tid, TexCoords);
}

vec4 inversion(sampler2D tid) {
	return vec4(vec3(1.0 - texture(tid, TexCoords)), 1.0);
}

vec4 grayscale(sampler2D tid) {
	vec4 color = texture(tid, TexCoords);
	//float average = (color.r + color.g + color.b) / 3.0;
	float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
	return vec4(vec3(average), 1.0);
}

vec4 nightversion(sampler2D tid) {
	vec4 color = texture(tid, TexCoords);
	float average = 0.7 * color.r + 0.15 * color.g + 0.15 * color.b;
	return vec4(0.1, average, 0.1, 1.0);
}

/** Kernel effect */

vec4 kernel3x3(sampler2D tid, float kernel[9]) {

	vec3 sampleTex[9];
	for (int i=0; i<9; i++)
		sampleTex[i] = vec3(texture(tid, TexCoords.st + offsets[i]));

	vec3 color = vec3(0.0);
	for (int i=0; i<9; i++)
		color += sampleTex[i] * kernel[i];

	return vec4(color, 1.0);
}

vec4 sharpen(sampler2D tid) {

	float kernel[9] = float[] (
		-1, -1, -1,
		-1,  9, -1,
		-1, -1, -1
	);

	return kernel3x3(tid, kernel);
}

vec4 blur(sampler2D tid) {

	float kernel[9] = float[] (
		1.0 / 16, 2.0 / 16, 1.0 / 16,
		2.0 / 16, 4.0 / 16, 2.0 / 16,
		1.0 / 16, 2.0 / 16, 1.0 / 16
	);

	return kernel3x3(tid, kernel);
}

vec4 edge(sampler2D tid) {

	float kernel[9] = float[] (
		 1,  1,  1,
		 1, -8,  1,
		 1,  1,  1
	);

	return kernel3x3(tid, kernel);
}

vec4 test_kernel(sampler2D tid) {

	float kernel[9] = float[] (
		 2,   2,  2,
		 2, -15,  2,
		 2,   2,  2
	);

	return kernel3x3(tid, kernel);
}
