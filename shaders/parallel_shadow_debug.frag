#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D uDepthMap;
uniform float uNearPlane;
uniform float uFarPlane;

// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * uNearPlane * uFarPlane) / (uFarPlane + uNearPlane - z * (uFarPlane - uNearPlane));	
}

void main()
{             
    float depthValue = texture(uDepthMap, TexCoords).r;
    FragColor = vec4(vec3(LinearizeDepth(depthValue) / uFarPlane), 1.0); // perspective
    //FragColor = vec4(vec3(depthValue), 1.0); // orthographic
}