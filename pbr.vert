#version 460 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}  

//#version 460 core
//layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aNormal;
//layout (location = 2) in vec2 aTexCoords;
//
//out vec2 TexCoords;
//out vec3 WorldPos;
//out vec3 Normal;
//out vec4 WorldPosLightSpace;
//
//out vec3 FragPosViewSpace;
//out vec3 NormalViewSpace;
//
//uniform mat4 projection;
//uniform mat4 view;
//uniform mat4 model;
//uniform mat3 normalMatrix;
//
//uniform mat4 lightSpaceMatrix;
//
//void main()
//{
//    TexCoords = aTexCoords;
//
//    vec4 worldPos4 = model * vec4(aPos, 1.0);
//    WorldPos = vec3(worldPos4);
//    Normal = normalMatrix * aNormal;   
//
//    vec4 viewPos4 = view * worldPos4;
//    FragPosViewSpace = vec3(viewPos4);
//    NormalViewSpace = mat3(view) * Normal;
//
//    WorldPosLightSpace = lightSpaceMatrix * vec4(WorldPos, 1.0);
//
//    gl_Position =  projection * view * vec4(WorldPos, 1.0);
//}