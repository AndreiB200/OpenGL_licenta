#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gAlbedo;
uniform sampler2D depthTexture;  
uniform sampler2D gPositionDepth;
uniform sampler2D gNormalRoughness;  
uniform sampler2D ssrTexture;
uniform sampler2D ssaoTexture;
uniform sampler2D ssaoBlurTexture;



uniform int u_mode = 0;             
uniform float uMaxMipLevel = 5.0;

float LinearizeDepth(float depth) {
    float near = 0.1;
    float far = 100.0;
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

float runDepth(float depth)
{   
    float near_plane = 0.1;
    float far_plane = 1000.0;
    float z = depth * 2.0 - 1.0; // Back to NDC 

    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

float FresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    FragColor = texture(gAlbedo, TexCoords);
    if (u_mode == 1) {
        float depth = texture(depthTexture, TexCoords).r;
        float near_plane = 0.001;
        float far_plane = 1.0;
        float z = depth * 2.0 - 1.0; // Back to NDC 

        float liniar = (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
        FragColor = vec4(vec3(depth), 1.0);
    } 
    else if (u_mode == 2) {
        FragColor = texture(gPositionDepth, TexCoords);
    }
    else if (u_mode == 3) {
        FragColor = texture(gNormalRoughness, TexCoords);
    }
    else if (u_mode == 4) {
        FragColor = texture(ssrTexture, TexCoords);
    }
    else if (u_mode == 5) {
        FragColor = vec4(vec3(texture(ssaoTexture, TexCoords).r), 1.0);
    }
     else if (u_mode == 6) {
        FragColor = vec4(vec3(texture(ssaoBlurTexture, TexCoords).r), 1.0);
    }
    else {
        vec3 baseColor = texture(gAlbedo, TexCoords).rgb;
        float roughness = texture(gNormalRoughness, TexCoords).a;
        float lod = clamp((roughness + 0.4) * uMaxMipLevel * 1.2, 0.0, 1.0);

        vec4 ssrSample = textureLod(ssrTexture, TexCoords, lod);
        vec3 ssrColor = ssrSample.rgb;
        float ssrAlpha = ssrSample.a;

        if (ssrAlpha <= 0.0) {
            FragColor = vec4(baseColor, 1.0);
            return;
        }

        vec3 viewPos = texture(gPositionDepth, TexCoords).xyz;
        vec3 viewNormal = normalize(texture(gNormalRoughness, TexCoords).xyz);
        vec3 viewDir = normalize(-viewPos);

        float NdotV = max(dot(viewNormal, viewDir), 0.0);
        float fresnel = FresnelSchlick(NdotV, 0.04);

    
        float reflectionFactor = clamp(ssrAlpha * fresnel * (1.0 - roughness * 0.5), 0.0, 1.0);

        vec3 finalColor = mix(baseColor, ssrColor, reflectionFactor);
        FragColor = vec4(finalColor, 1.0);
    }
}