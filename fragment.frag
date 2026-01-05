#version 460 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

//uniform vec3 color;
uniform vec3 specular;

uniform bool Switch;
uniform bool Shadow;
//uniform float specValue;

uniform sampler2D shadowMap;

uniform sampler2D albedoMap;
uniform sampler2D roughnessMap;


float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;

    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int size = 6;
    for(int x = -size; x <= size; ++x)
    {
        for(int y = -size; y <= size; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= (size*2+1) * (size*2+1);

    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

vec4 normalLight()
{
	vec3 colorMap = texture(albedoMap, TexCoords).rgb;
    vec3 color = vec3(0.9,0.9,0.9);
	float ambientStrength = 0.1;
	vec3 ambient = vec3(ambientStrength);

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	float specularStrength = texture(roughnessMap, TexCoords).r + 0.0;
	specularStrength = max(specularStrength,1.0);
    specularStrength = 1.0;

	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 6);
	vec3 specular = specularStrength * spec * lightColor;
    vec4 theLight;

	float shadow = 1.0 - ShadowCalculation(FragPosLightSpace);

	theLight = vec4((ambient + diffuse + specular) * color, 1.0);

	theLight = vec4((ambient + shadow*(diffuse + specular)) * color, 1.0);
	return theLight;
}
void main()
{	
    //FragColor = vec4(FragPos,1.0);
	FragColor = normalLight();
}