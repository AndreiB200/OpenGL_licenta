#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

// Uniforms - G-Buffer & Scene Textures
uniform sampler2D uPositionDepth; 
uniform sampler2D uNormal;        
uniform sampler2D uColor;         

// Uniforms - Matrices & Config
uniform mat4 uProjection;
uniform mat4 uInvProjection;

uniform int uMaxSteps = 60;
uniform int uBinarySearchSteps = 8;
uniform float uStepSize = 0.1;
uniform float uThickness = 0.2;

vec3 ProjectToScreenSpace(vec3 viewPos) {
    vec4 clipPos = uProjection * vec4(viewPos, 1.0);
    vec3 ndcPos = clipPos.xyz / clipPos.w;
    return ndcPos * 0.5 + 0.5;
}

vec3 BinarySearch(vec3 rayDir, inout vec3 rayPos) {
    for (int i = 0; i < uBinarySearchSteps; ++i) {
        vec3 uv = ProjectToScreenSpace(rayPos);
        float sampledDepth = texture(uPositionDepth, uv.xy).z;
        float depthDiff = sampledDepth - rayPos.z;

        rayDir *= 0.5;
        if (depthDiff > 0.0) {
            rayPos -= rayDir; // Am intrat în geometrie, dăm înapoi
        } else {
            rayPos += rayDir; // Suntem în aer, înaintăm
        }
    }
    return ProjectToScreenSpace(rayPos);
}

// Ray Marching Principal
vec4 RayMarch(vec3 rayOrigin, vec3 rayDir) {
    vec3 currentRayPos = rayOrigin;
    vec3 stepVector = rayDir * uStepSize;

    for (int i = 0; i < uMaxSteps; ++i) {
        currentRayPos += stepVector;

        vec3 uv = ProjectToScreenSpace(currentRayPos);

        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            break;
        }

        float sampledViewZ = texture(uPositionDepth, uv.xy).z;
        float depthDiff = sampledViewZ - currentRayPos.z;

        if (depthDiff > 0.0 && depthDiff < uThickness) {
            vec3 hitUV = BinarySearch(stepVector, currentRayPos);
            return vec4(hitUV, 1.0);
        }
    }
    return vec4(0.0);
}

void main() {
    float roughness = texture(uNormal, TexCoords).a; // will be extracted from the Alpha Value .a
    
    if (roughness > 0.8) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 viewPos = texture(uPositionDepth, TexCoords).xyz;
    vec3 viewNormal = normalize(texture(uNormal, TexCoords).xyz);

    vec3 rayDirToPixel = normalize(viewPos);
    vec3 reflectDir = reflect(rayDirToPixel, viewNormal);

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

        vec3 reflectedColor = texture(uColor, hitUV).rgb;

        FragColor = vec4(reflectedColor, clamp(finalWeight, 0.0, 1.0));
    } 
    else {
        FragColor = vec4(0.0);
    }
}