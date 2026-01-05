#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

in vec4 WorldPosLightSpace;

// material parameters
/*uniform vec3 u_albedo;
uniform float u_metallic;
uniform float u_roughness;
uniform float ao;*/

vec3 u_albedo = vec3(0.7,0.7,0.7);
float u_metallic = 0.0;
float u_roughness = 0.0;
float ao = 0.8;

uniform vec3 emision;

// material textures
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;

//textures or parameter switch
int Switch = 0;
uniform bool start;

//shadow map
uniform sampler2D shadowMap;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
// lights
uniform vec3 lightPositions[1];
uniform vec3 lightColors[1];

uniform vec3 camPos;

const float PI = 3.14159265359;

uniform float multi;

vec3 bbMaxDefault = vec3(10.0, 10.0, 10.0); ////AABB hardcoded !
vec3 bbMinDefault = vec3(-10.0, -10.0, -10.0);
vec3 bbPosDefault = vec3(0.0, 0.0, 0.0);

vec3 lightDebug;

// ----------------------------------------------------------------------------
vec3 bbReflection(vec3 R, vec3 bbMax, vec3 bbMin, vec3 bbPos)
{
    vec3 intersectMaxPointPlanes = (bbMax-WorldPos) / R;
    vec3 intersectMinPointPlanes = (bbMin-WorldPos) / R;
    vec3 largestParams = max(intersectMaxPointPlanes,intersectMinPointPlanes);
    float distToIntersect = min(min(largestParams.x, largestParams.y), largestParams.z);
    vec3 intersectPointsWS = WorldPos + R * distToIntersect;
    vec3 localCorrReflDirWS = intersectPointsWS - bbPos;

    return localCorrReflDirWS;
}
// ----------------------------------------------------------------------------
float ShadowCalculation(vec4 fragPosLightSpace)
{   
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float shadow = 0.0;
    if(projCoords.z > 1.0) {shadow = 0.0; return shadow;}
    
    float currentDepth = projCoords.z;
    
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPositions[0] - WorldPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.0002);   //edit shadows
    
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    //float size = 10;
    float size = 3;
    for(float x = -size; x <= size; ++x)
    {
        for(float y = -size; y <= size; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= (size*2+1) * (size*2+1);
    lightDebug = vec3(shadow);
    return shadow;
}
// ----------------------------------------------------------------------------
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
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
    vec3 N;
  
    if(Switch == 0)
    {   
        albedo = u_albedo;
        metallic = u_metallic;
        roughness = u_roughness;
        N = Normal;
    }
    if(Switch == 1)
    {
        albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
        metallic = texture(metallicMap, TexCoords).r;
        roughness = texture(roughnessMap, TexCoords).r;
        N = getNormalFromMap();
    }
    N = N - vec3(0.2);
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N); 

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
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

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }   
    
    // ambient lighting from IBL
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	
 
    float shadowCol, shadow = ShadowCalculation(WorldPosLightSpace);
    shadowCol = 1.0 - shadow;

    vec3 bbMax = vec3(3.0,4.0,3.0); ////AABB hardcoded !
    vec3 bbMin = vec3(-3.0,-1.0,-3.0);
    vec3 bbPos = vec3(0.0,1.5,0.0);

    vec3 totalRefletion = bbReflection(R, bbMaxDefault, bbMinDefault, bbPosDefault);
    bvec3 a = lessThanEqual(WorldPos, bbMax + vec3(0.0001));
    bvec3 b = lessThanEqual(bbMin - vec3(0.0001), WorldPos);
    if(a == b)
    {
        totalRefletion = bbReflection(R, bbMax, bbMin, bbPos);
    }

    vec3 irradiance = texture(irradianceMap, totalRefletion).rgb;
    vec3 diffuse    = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, totalRefletion,  roughness * MAX_REFLECTION_LOD).rgb;

    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 color = (shadowCol+multi)*ambient + (shadowCol * Lo);


    // HDR tonemapping
    color = (color / (color + vec3(1.0)));

    // gamma correct (baking process doesn't need gamma)
    if(emision.x >= 0.2)
        color = emision;

    if(start == false)
        color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
    //FragColor = vec4(lightDebug, 1.0);
}

void main()
{	
    runLight();
}