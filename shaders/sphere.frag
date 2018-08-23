#version 330 core

/** Uniform variables */

// Camera
uniform vec3 uCameraPos;

// Special effect texture
uniform sampler2D sphereMap;

/** Stream variables */

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

void main() {

	//vec3 normal = normalize(Normal);
	//vec3 viewDir = normalize(uCameraPos - FragPos);
	vec3 resultColor = vec3(0.0, 0.0, 0.0);

	resultColor = vec3(texture(sphereMap, TexCoords));

	// Result
	FragColor = vec4(resultColor, 1.0);
}
