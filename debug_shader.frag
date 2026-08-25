#version 460 core
out vec4 FragColor;

uniform vec3 debugColor = vec3(0.0, 0.8, 0.0);

void main()
{
    FragColor = vec4(debugColor, 1.0);
}