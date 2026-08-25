#version 460 core

layout (location = 0) out vec3 gAlbedo;
layout (location = 1) out vec3 gPosition;
layout (location = 2) out vec3 gNormal;
layout (location = 3) out vec3 gARM; 

in vec3 WorldPos;
in vec2 TexCoords;
in vec3 Normal;

uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;

uniform int textureSelect = 1;

uniform vec3 u_albedo = vec3(0.6,0.6,0.6);
uniform float u_metallic = 0.1;
uniform float u_roughness = 0.1;


vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T   = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{
    gPosition = WorldPos;
    float roughness = 0.9;
    float metallic = 0.0;
    
    if(textureSelect == 0) // constants
    {
        gAlbedo = u_albedo;
        gNormal = Normal;
        gARM = vec3(roughness, metallic, 1.0);
    }
    if(textureSelect == 1) // textures
    {
        roughness = texture(roughnessMap, TexCoords).r;
        metallic  = texture(metallicMap, TexCoords).r;
    
        gAlbedo = texture(albedoMap, TexCoords).rgb;
        gNormal = getNormalFromMap();
        
        gARM = vec3(roughness, metallic, 1.0);
    }
}