#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;  
uniform int u_mode = 0;             

float LinearizeDepth(float depth) {
    float near = 0.1;
    float far = 100.0;
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

float runDepth(float depth)
{   
    float near_plane = 0.001;
    float far_plane = 1000.0;
    float z = depth * 2.0 - 1.0; // Back to NDC 

    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main() {
    FragColor = texture(screenTexture, TexCoords);
    if (u_mode == 0) {
        FragColor = texture(screenTexture, TexCoords);
    } 
    else {
        float depth = texture(depthTexture, TexCoords).r;
        float near_plane = 0.001;
        float far_plane = 1.0;
        float z = depth * 2.0 - 1.0; // Back to NDC 

        float liniar = (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
        FragColor = vec4(vec3(depth), 1.0);
    }
}