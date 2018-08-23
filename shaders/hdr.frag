#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D uHDRBuffer;
uniform bool uHDR;
uniform float uExposure;

void main()
{             
    const float gamma = 2.2;

    vec3 hdrColor = texture(uHDRBuffer, TexCoords).rgb;

    if(uHDR) {
        // reinhard
        // vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
        // exposure
        vec3 mapped = vec3(1.0) - exp(-hdrColor * uExposure);
        // also gamma correct while we're at it       
        mapped = pow(mapped, vec3(1.0 / gamma));
        FragColor = vec4(mapped, 1.0);
    }
    else {
        vec3 mapped = pow(hdrColor, vec3(1.0 / gamma));
        FragColor = vec4(mapped, 1.0);
    }
}