#version 330 core
layout (location = 0) in vec3 aPos;

layout (std140) uniform uMatrices
{
    mat4 projection;
    mat4 view;
};

uniform mat4 uModel;

void main() {

    gl_Position = projection * view * uModel * vec4(aPos, 1.0);
}  