#version 410

// Global variables for lighting calculations
uniform vec3 cameraPos;  // Position of the camera
uniform vec3 lightPos;   // Position of the light
uniform float shininess; // Shininess factor for specular highlights
uniform float toonSpecularThreshold;
// scene uniforms
uniform mat4 lightMVP;
// config uniforms, use these to control the shader from UI
uniform int shadows = 0;
uniform int samplingMode = 0;
uniform int lightMode = 0;
// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D texShadow;  

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos;  // World-space position
in vec3 fragNormal; // World-space normal


float calculateSpotlight(){
    vec4 fragLightCoord = lightMVP * vec4(fragPos, 1.0);
    // Divide by w because fragLightCoord are homogeneous coordinates
    fragLightCoord.xyz /= fragLightCoord.w;
    // The resulting value is in NDC space (-1 to +1),
    //  we transform them to texture space (0 to 1).
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;
    vec2 shadowMapCoord = fragLightCoord.xy;
    //spotlight mode
    vec2 centre = vec2(0.5, 0.5);
    float dist = distance(shadowMapCoord, centre);
    float visibility = clamp(-2.0 * dist +1.0, 0.0, 1.0);
    return visibility;
}

float calculateShadow(){
    vec4 fragLightCoord = lightMVP * vec4(fragPos, 1.0);

    // Divide by w because fragLightCoord are homogeneous coordinates
    fragLightCoord.xyz /= fragLightCoord.w;

    // The resulting value is in NDC space (-1 to +1),
    //  we transform them to texture space (0 to 1).
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;

    // Depth of the fragment with respect to the light
    float fragLightDepth = fragLightCoord.z;

    // Shadow map coordinate corresponding to this fragment
    vec2 shadowMapCoord = fragLightCoord.xy;

    float visibility = 1.0;
    float bias = 0.0005;
    //float bias = max(0.0001 * (dot(normal, lightDir)), 0.00005);

    if(samplingMode != 0){
        // pcf mode
        vec2 texSize = textureSize(texShadow, 0);
        float texelWidth = 1.0 / texSize.x;
        float texelHeight = 1.0 / texSize.y;
        vec2 texelSize = vec2(texelWidth, texelHeight);
        float shadowSum = 0.0;
        int filterSize = 4; //assuming it's a square
        int halfFilterSize = filterSize / 2;
        for(int y = -halfFilterSize; y < filterSize - halfFilterSize; y++){
            for(int x = -halfFilterSize; x < filterSize - halfFilterSize; x++){
                vec2 offset = vec2(x, y) * texelSize;
                float shadowMapDepth = texture(texShadow, shadowMapCoord + offset).x;
                if(shadowMapDepth < fragLightDepth - bias){
                    shadowSum += 0.20;
                }else{
                    shadowSum += 1.0;
                }
            }
        }
        visibility = shadowSum / float(pow(filterSize, 2));
    }else{
        // Shadow map value from the corresponding shadow map position
        float shadowMapDepth = texture(texShadow, shadowMapCoord).x;

        if(shadowMapDepth < fragLightDepth - bias){
            visibility = 0.20;
        }
    }
    if(shadowMapCoord.x < 0.0 || shadowMapCoord.x > 1.0 || shadowMapCoord.y < 0.0 || shadowMapCoord.y > 1.0){
        visibility = 0.0;
    }
    return visibility;
}

void main(){
    vec3 normal = normalize(fragNormal);                // Normalize normal vector
    vec3 lightDir = normalize(lightPos - fragPos);    // Light direction
    vec3 viewDir = normalize(cameraPos - fragPos);    // View direction
    
    // Calculate the half-vector H
    vec3 halfDir = normalize(lightDir + viewDir);     // Halfway vector

    // Calculate the Blinn-Phong specular component
    float specular = pow(max(dot(halfDir, normal), 0.0), shininess);

    float visibility = 1.0;

    if(shadows!=0){
        visibility = calculateShadow();
    }
    if(lightMode != 0){
        visibility *= calculateSpotlight();
    }
    
    if(specular >= toonSpecularThreshold){
        outColor = vec4(vec3(1.0, 1.0, 1.0)  * visibility, 1.0);;
    }else{
        outColor = vec4(0.0, 0.0, 0.0, 1.0);    // Output the final color
    }
}