#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
} vs_out;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpaceMatrix;

void main() {

	gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0f);

	vs_out.FragPos = vec3(uModel * vec4(aPos, 1.0));

	vs_out.Normal = mat3(uModel) * aNormal;

	vs_out.TexCoords = aTexCoords;

	vs_out.FragPosLightSpace = uLightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
}