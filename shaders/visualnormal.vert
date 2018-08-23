#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out VS_OUT {
	vec3 normal;
} vs_out;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {

	gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0f);

	//mat3 normalMatrix = mat3(transpose(inverse(uView * uModel)));
	mat3 normalMatrix = mat3(uView * uModel);
	vs_out.normal = normalize(vec3(uProjection * vec4(normalMatrix * aNormal, 0.0)));
}