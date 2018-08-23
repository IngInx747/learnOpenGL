#version 330 core

in vec4 FragPos;

uniform vec3 uLightPos;
uniform float uFarPlane;

void main()
{
    float dist_light2pixel = length(FragPos.xyz - uLightPos);
    
    // map to [0:1] range by dividing by far_plane
    dist_light2pixel = dist_light2pixel / uFarPlane;
    
    // write this as modified depth
    gl_FragDepth = dist_light2pixel;
}