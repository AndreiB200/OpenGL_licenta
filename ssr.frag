#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

// Uniforms - G-Buffer & Scene Textures (World Space)
uniform sampler2D gPosition;
uniform sampler2D gNormal;       
uniform sampler2D gAlbedo;
uniform sampler2D gARM;


// Uniforms - Matrices & Config
uniform mat4 uProjection;
uniform mat4 uView;              
uniform int uMaxSteps = 60;
uniform int uBinarySearchSteps = 8;
uniform float uStepSize = 0.1;
uniform float uThickness = 0.2;

vec3 ProjectToScreenSpace(vec3 viewPos) {
    vec4 clipPos = uProjection * vec4(viewPos, 1.0);
    vec3 ndcPos = clipPos.xyz / clipPos.w;
    return ndcPos * 0.5 + 0.5;
}

// Caută exact punctul de intersecție prin înjumătățirea pasului
vec3 BinarySearch(vec3 rayDir, inout vec3 rayPos) {
    for (int i = 0; i < uBinarySearchSteps; ++i) {
        vec3 uv = ProjectToScreenSpace(rayPos);
        
        // Citim World Position și o transformăm în View-Space pentru a-i lua Z-ul
        vec3 sampledWorldPos = texture(gPosition, uv.xy).xyz;
        float sampledViewZ  = (uView * vec4(sampledWorldPos, 1.0)).z;
        
        // În OpenGL (-Z e în fața camerei):
        // Dacă rayPos.z este mai mic decât sampledViewZ (ex: -6.0 față de -5.0), raza e în spatele geometriei
        float depthDiff = rayPos.z - sampledViewZ;

        rayDir *= 0.5;
        if (depthDiff > 0.0) {
            // Suntem în spatele geometriei -> dăm înapoi spre cameră
            rayPos -= rayDir; 
        } else {
            // Suntem în aer -> înaintăm în adâncime
            rayPos += rayDir; 
        }
    }
    return ProjectToScreenSpace(rayPos);
}

// Ray Marching în View-Space
vec4 RayMarch(vec3 rayOrigin, vec3 rayDir) {
    vec3 currentRayPos = rayOrigin;
    vec3 stepVector = rayDir * uStepSize;

    for (int i = 0; i < uMaxSteps; ++i) {
        currentRayPos += stepVector;

        vec3 uv = ProjectToScreenSpace(currentRayPos);

        // Verificăm dacă raza a părăsit ecranul
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            break;
        }

        vec3 sampledWorldPos = texture(gPosition, uv.xy).xyz;
        
        if (length(sampledWorldPos) == 0.0) continue;

        float sampledViewZ = (uView * vec4(sampledWorldPos, 1.0)).z;

        float depthDiff = sampledViewZ - currentRayPos.z;

        if (depthDiff > 0.0 && depthDiff < uThickness) {
            vec3 hitUV = BinarySearch(stepVector, currentRayPos);
            return vec4(hitUV, 1.0);
        }
    }
    return vec4(0.0);
}

void main() {
    vec4 normalSample = texture(gNormal, TexCoords);
    float roughness =  texture(gARM, TexCoords).r;
    
    if (roughness > 0.8) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 worldPos = texture(gPosition, TexCoords).xyz;
    if (length(worldPos) == 0.0) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 worldNormal = normalize(normalSample.xyz);

    vec3 viewPos    = vec3(uView * vec4(worldPos, 1.0));
    vec3 viewNormal = normalize(mat3(uView) * worldNormal);

    vec3 rayDirToPixel = normalize(viewPos);
    vec3 reflectDir    = reflect(rayDirToPixel, viewNormal);

    if (dot(reflectDir, viewNormal) < 0.0) {
        FragColor = vec4(0.0);
        return;
    }

    vec4 hitResult = RayMarch(viewPos, reflectDir);

    if (hitResult.w > 0.0) {
        vec2 hitUV = hitResult.xy;

        vec2 screenEdgeFade = smoothstep(vec2(0.0), vec2(0.2), hitUV) * (1.0 - smoothstep(vec2(0.8), vec2(1.0), hitUV));
        float edgeWeight = screenEdgeFade.x * screenEdgeFade.y;

        float roughnessWeight = 1.0 - roughness;
        float cameraFacingWeight = 1.0 - max(dot(-rayDirToPixel, reflectDir), 0.0);

        float finalWeight = edgeWeight * roughnessWeight * cameraFacingWeight;

        vec3 reflectedColor = texture(gAlbedo, hitUV).rgb;

        FragColor = vec4(reflectedColor, clamp(finalWeight, 0.0, 1.0));
    } 
    else {
        FragColor = vec4(0.0);
    }
}