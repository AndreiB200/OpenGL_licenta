#version 460 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition; 
uniform sampler2D gNormal;  
uniform sampler2D texNoise;  

uniform vec3 samples[64];
uniform vec2 noiseScale;

uniform mat4 projection;
uniform mat4 view;

const int kernelSize = 64;
uniform float radius = 0.5;  
uniform float bias = 0.025;  
uniform float aoIntensity = 1.0;

void main() {
    
    vec3 fragPosWorld = texture(gPosition, TexCoords).xyz;
    vec3 normalWorld  = texture(gNormal, TexCoords).xyz;
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

    if (length(fragPosWorld) == 0.0) {
        FragColor = 1.0;
        return;
    }

    vec3 fragPos = vec3(view * vec4(fragPosWorld, 1.0));
    vec3 normal  = normalize(mat3(view) * normalWorld);

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i) {
        vec3 samplePos = TBN * samples[i]; 
        samplePos = fragPos + samplePos * radius; 
        
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;         
        offset.xy /= offset.w;                 
        offset.xy = offset.xy * 0.5 + 0.5;     
        
        vec3 sampleWorldPos = texture(gPosition, offset.xy).xyz;
        if (length(sampleWorldPos) == 0.0) {
            continue;
        }

        float sampleDepth   = (view * vec4(sampleWorldPos, 1.0)).z;
        
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    occlusion = pow(occlusion, aoIntensity);

    FragColor = occlusion;
}