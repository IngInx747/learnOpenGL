#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uCameraPos;
uniform samplerCube uSkybox;

void main() {

	float ratio = 1.00 / 1.52;
	vec3 I = normalize(FragPos - uCameraPos);
	//vec3 R = reflect(I, normalize(Normal));
	vec3 R = refract(I, normalize(Normal), ratio);
	FragColor = vec4(texture(uSkybox, R).rgb, 1.0);
}
