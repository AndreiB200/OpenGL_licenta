#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPositionGeom;
out vec3 normalGeom;

void main()
{
	worldPositionGeom = vec3(model * vec4(aPos, 1));
	normalGeom = normalize(mat3(transpose(inverse(model))) * aNormal);
	gl_Position = projection * view * vec4(worldPositionGeom, 1);
}