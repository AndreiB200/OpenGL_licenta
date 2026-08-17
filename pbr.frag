
#version 460 core
//layout (location = 0) out vec4 FragColor;
//layout (location = 1) out vec4 FragLinearDepth;
//layout (location = 2) out vec4 FragPositionDepth;
//layout (location = 3) out vec4 FragNormalRoughness;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec3 SaveDepth;


uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gARM;
uniform sampler2D ssaoBlured;

in vec2 TexCoords;

uniform mat4 lightSpaceMatrix;

//in vec3 WorldPos;
//in vec3 Normal;


// material parameters
uniform vec3 u_albedo = vec3(0.7,0.7,0.7);
uniform float u_metallic = 0.0;
uniform float u_roughness = 0.0;
uniform vec3 u_normal = vec3(0.0f, 0.0f, 0.0f);
uniform float ao = 1.0;
//uniform bool useAlpha = false;

uniform vec3 emision;

// material textures
//uniform sampler2D albedoMap;
//uniform sampler2D normalMap;
//uniform sampler2D metallicMap;
//uniform sampler2D roughnessMap;
//uniform sampler2D alphaMap;
//

//textures or parameter switch
uniform int textureSelect = 1;
uniform bool start = false;

//shadow map
uniform sampler2D shadowMap;
uniform float depth_near, depth_far;
uniform float multiplayer = 0.0f;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
// lights
uniform vec3 lightPositions[1];
uniform vec3 lightColors[1];

uniform vec3 camPos;
uniform mat4 view;

const float PI = 3.14159265359;

//uniform float multi;

vec3 bbMaxDefault = vec3(10.0, 10.0, 10.0); ////AABB hardcoded !
vec3 bbMinDefault = vec3(-10.0, -10.0, -10.0);
vec3 bbPosDefault = vec3(0.0, 0.0, 0.0);

uniform float shadowUp = 0.05;
uniform float shadowBias = 0.001;
uniform int pcfSize = 3;

vec3 lightDebug;

const vec2 poissonDisk[32] = vec2[](
    vec2(-0.613392,  0.617481),
    vec2( 0.170019, -0.040254),
    vec2(-0.299417,  0.791925),
    vec2( 0.645680,  0.493210),
    vec2(-0.651784,  0.223851),
    vec2( 0.421003,  0.027100),
    vec2(-0.467140, -0.405100),
    vec2(-0.816201, -0.452241),
    vec2(-0.242929, -0.211463),
    vec2( 0.054950, -0.936000),
    vec2(-0.562140, -0.743300),
    vec2( 0.180300, -0.611200),
    vec2(-0.026400, -0.357000),
    vec2( 0.500000, -0.300000),
    vec2( 0.683000, -0.602000),
    vec2( 0.852000, -0.211000),
    vec2( 0.228000,  0.401000),
    vec2(-0.070000,  0.345000),
    vec2( 0.251000,  0.751000),
    vec2(-0.380000,  0.120000),
    vec2(-0.841000, -0.101000),
    vec2(-0.211000, -0.810000),
    vec2( 0.410000, -0.820000),
    vec2( 0.881000,  0.151000),
    vec2( 0.512000,  0.810000),
    vec2(-0.611000,  0.880000),
    vec2(-0.951000,  0.301000),
    vec2(-0.412000,  0.512000),
    vec2( 0.001000,  0.981000),
    vec2( 0.301000,  0.181000),
    vec2( 0.812000, -0.810000),
    vec2(-0.710000, -0.810000)
);

uniform float lightWidth = 0.05;

// ----------------------------------------------------------------------------
//vec3 bbReflection(vec3 R, vec3 bbMax, vec3 bbMin, vec3 bbPos)
//{
//    vec3 intersectMaxPointPlanes = (bbMax-WorldPos) / R;
//    vec3 intersectMinPointPlanes = (bbMin-WorldPos) / R;
//    vec3 largestParams = max(intersectMaxPointPlanes,intersectMinPointPlanes);
//    float distToIntersect = min(min(largestParams.x, largestParams.y), largestParams.z);
//    vec3 intersectPointsWS = WorldPos + R * distToIntersect;
//    vec3 localCorrReflDirWS = intersectPointsWS - bbPos;
//
//    return localCorrReflDirWS;
//}
// ----------------------------------------------------------------------------
float randomAngle(vec2 seed) {
    return dot(sin(seed * vec2(12.9898, 78.233)), vec2(43758.5453)) * 6.28318530718;
}

float FindBlockerDistance(vec3 projCoords, float currentDepth, float bias)
{
    int blockers = 0;
    float avgBlockerDepth = 0.0;
    vec2 searchRadius = (1.0 / textureSize(shadowMap, 0)) * 5.0; // Zona fixa de cautare

    for(int i = 0; i < 32; i++)
    {
        float shadowMapDepth = texture(shadowMap, projCoords.xy + poissonDisk[i] * searchRadius).r;
        if(shadowMapDepth < currentDepth - bias)
        {
            blockers++;
            avgBlockerDepth += shadowMapDepth;
        }
    }

    if(blockers == 0) return -1.0; // Nu exista blocker-i
    return avgBlockerDepth / float(blockers);
}

float ShadowCalculation(vec3 worldPos)
{   
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float shadow = 0.0;
    if(projCoords.z > 1.0) {shadow = 0.0; return shadow;}
    
    float currentDepth = projCoords.z;
    
    vec3 normal = normalize(texture(gNormal,TexCoords).rgb);
    vec3 lightDir = normalize(lightPositions[0] - texture(gPosition,TexCoords).rgb);
    float bias = max(shadowUp * (1.0 - dot(normal, lightDir)), shadowBias);

    float avgBlockerDepth = FindBlockerDistance(projCoords, currentDepth, bias);

    if(avgBlockerDepth < 0.0) return 0.0;

    float penumbraSize = ((currentDepth - avgBlockerDepth) / avgBlockerDepth) * lightWidth;

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    vec2 filterRadius = penumbraSize * texelSize;

    float diskRadius = 3.0 * texelSize.x;

    float angle = randomAngle(gl_FragCoord.xy);
    float sinAngle = sin(angle);
    float cosAngle = cos(angle);
    mat2 rotationMatrix = mat2(cosAngle, -sinAngle, sinAngle, cosAngle);

    for(int i = 0; i < 32; i++)
    {
        vec2 rotatedOffset = rotationMatrix * poissonDisk[i];
        vec2 sampleUV = projCoords.xy + rotatedOffset * filterRadius;
        float closestDepth = texture(shadowMap, sampleUV).r;
        
        // Daca adancimea curenta - bias e mai mare, pixelul e in umbra
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    shadow /= 32.0;
    shadow = clamp(shadow * shadow * (3.0 - 2.0 * shadow), 0.2, 0.8);
    lightDebug = vec3(shadow);
    return shadow;
}
// ----------------------------------------------------------------------------
//vec3 getNormalFromMap()
//{
//    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
//
//    vec3 Q1  = dFdx(WorldPos);
//    vec3 Q2  = dFdy(WorldPos);
//    vec2 st1 = dFdx(TexCoords);
//    vec2 st2 = dFdy(TexCoords);
//
//    vec3 N   = normalize(Normal);
//    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
//    vec3 B  = -normalize(cross(N, T));
//    mat3 TBN = mat3(T, B, N);
//
//    return normalize(TBN * tangentNormal);
//}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------

void runLight()
{
    vec3 albedo;
    float metallic;
    float roughness;
    vec3 ARM = texture(gARM, TexCoords).rgb;
    vec3 N = vec3(0.0,0.0,0.0);
  
    if(textureSelect == 0)
    {   
        albedo = u_albedo;
        metallic = u_metallic;
        roughness = u_roughness;
        N = texture(gNormal, TexCoords).rgb;
    }
    if(textureSelect == 1)
    {
        albedo = pow(texture(gAlbedo, TexCoords).rgb, vec3(2.2));
        metallic = ARM.g;
        roughness = ARM.r;
        N = texture(gNormal, TexCoords).rgb;
    }


    if(textureSelect == 2)
    {
        albedo = pow(texture(gAlbedo, TexCoords).rgb, vec3(2.2));
        metallic = u_metallic; // screen buffer...
        roughness = u_roughness;  // screen buffer...
        N = texture(gNormal, TexCoords).rgb;
    }
    if(textureSelect == 3)
    {
        albedo = pow(vec3(ARM.g), vec3(2.2));
        metallic = u_metallic;
        roughness = u_roughness;
        N = texture(gNormal, TexCoords).rgb;
    }
    if(textureSelect == 4)
    {
        albedo = pow(vec3(ARM.r), vec3(2.2));
        metallic = u_metallic;
        roughness = u_roughness;
        N = texture(gNormal, TexCoords).rgb;
    }

    vec3 WorldPos = texture(gPosition, TexCoords).rgb;
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N); 

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    float shadowCol, shadow = ShadowCalculation(WorldPos);
    shadowCol = 1.0 - shadow;

    for(int i = 0; i < 1; ++i) 
    {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);    
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);        
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	                
            
        float NdotL = max(dot(N, L), 0.0);        

        Lo += (kD * albedo / PI + specular) * (radiance * shadowCol) * NdotL;
    }   
    
    // ambient lighting from IBL
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	

    vec3 bbMax = vec3(3.0,4.0,3.0); ////AABB hardcoded !
    vec3 bbMin = vec3(-3.0,-1.0,-3.0);
    vec3 bbPos = vec3(0.0,1.5,0.0);

//    vec3 totalRefletion = bbReflection(R, bbMaxDefault, bbMinDefault, bbPosDefault);
//    bvec3 a = lessThanEqual(WorldPos, bbMax + vec3(0.0001));
//    bvec3 b = lessThanEqual(bbMin - vec3(0.0001), WorldPos);
//    if(a == b)
//    {
//        totalRefletion = bbReflection(R, bbMax, bbMin, bbPos);
//    }

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse    = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;

    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    float ambientOcclusion = texture(ssaoBlured, TexCoords).r;
    vec3 ambient = (kD * diffuse + specular) * (ambientOcclusion * ao);
   

    vec3 color = (shadowCol + multiplayer) * ambient + ((shadowCol + multiplayer) * Lo);

    //vec3 color = ambient + Lo;

    // HDR tonemapping 
    color = (color / (color + vec3(1.0)));

    // gamma correct (baking process doesn't need gamma)
    if(emision.x >= 0.2)
        color = emision;

    if(start == false)
        color = pow(color, vec3(1.0/2.2)); 

    float alphaVal = 1.0;
//    if(useAlpha)
//        alphaVal = texture(alphaMap, TexCoords).r;


    FragColor = vec4(color,1.0);
    //FragColor = vec4(lightDebug, 1.0);
}

void runDepth()
{   
    vec3 position = texture(gPosition, TexCoords).rgb;
    vec3 fragPos = vec3(view * vec4(position, 1.0));
    float z = fragPos.z * 2.0 - 1.0; // Back to NDC 

    float liniar = (2.0 * depth_near * depth_far) / (depth_far + depth_near - z * (depth_far - depth_near));
    SaveDepth = vec3(liniar);
}

void main()
{	
    runLight();
    runDepth();
}