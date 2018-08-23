#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {

	gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0f);

	// Get one fragment's position in World Space
	vs_out.FragPos = vec3(uModel * vec4(aPos, 1.0));

	// Also don't forget to transform normal vector
	//Normal = mat3(transpose(inverse(uModel))) * aNormal;
	vs_out.Normal = mat3(uModel) * aNormal;

	vs_out.TexCoords = aTexCoords;
}