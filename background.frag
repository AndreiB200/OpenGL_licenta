#version 460 core

layout (location = 0) out vec3 gAlbedo;
layout (location = 1) out vec3 gPosition;
layout (location = 2) out vec3 gNormal;
layout (location = 3) out vec3 gARM;

in vec3 WorldPos;
uniform samplerCube environmentMap;

void main()
{		
    vec3 envColor = texture(environmentMap, WorldPos).rgb;
    
    // Tonemapping & Gamma correction
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
    
    gAlbedo   = envColor;
    gPosition = WorldPos;
    gNormal   = vec3(0.0); 
    gARM      = vec3(1.0, 0.0, 0.0); 
}