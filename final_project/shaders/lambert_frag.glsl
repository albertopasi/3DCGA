#version 410

// Global variables for lighting calculations
uniform vec3 lightPos;      // World-space position of the light
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);    // Color of the light
uniform vec3 kd = vec3(1.0, 1.0, 1.0);            // Diffuse reflection coefficient (material color)

// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D shadowMap;
uniform sampler2D shadowMap1;

uniform vec3 lightPos1;      // World-space position of the light
uniform vec3 lightColor1;   
 
// scene uniforms
uniform mat4 lightMVP;
uniform mat4 lightMVP1;
uniform int shadows;

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPosition; // World-space position
in vec3 fragNormal; // World-space normal


float calculateShadow(mat4 lightMatrix, sampler2D shadowText){
    vec4 fragLightCoord = lightMatrix * vec4(fragPosition, 1.0);

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

    // if(samplingMode != 0){
    //     // pcf mode
    //     vec2 texSize = textureSize(shadowMap, 0);
    //     float texelWidth = 1.0 / texSize.x;
    //     float texelHeight = 1.0 / texSize.y;
    //     vec2 texelSize = vec2(texelWidth, texelHeight);
    //     float shadowSum = 0.0;
    //     int filterSize = 4; //assuming it's a square
    //     int halfFilterSize = filterSize / 2;
    //     for(int y = -halfFilterSize; y < filterSize - halfFilterSize; y++){
    //         for(int x = -halfFilterSize; x < filterSize - halfFilterSize; x++){
    //             vec2 offset = vec2(x, y) * texelSize;
    //             float shadowMapDepth = texture(shadowMap, shadowMapCoord + offset).x;
    //             if(shadowMapDepth < fragLightDepth - bias){
    //                 shadowSum += 0.20;
    //             }else{
    //                 shadowSum += 1.0;
    //             }
    //         }
    //     }
    //     visibility = shadowSum / float(pow(filterSize, 2));
    // }else{
        // Shadow map value from the corresponding shadow map position
        float shadowMapDepth = texture(shadowText, shadowMapCoord).x;

        if(shadowMapDepth < fragLightDepth - bias){
            visibility = 0.20;
        }
    // }
    if(shadowMapCoord.x < 0.0 || shadowMapCoord.x > 1.0 || shadowMapCoord.y < 0.0 || shadowMapCoord.y > 1.0){
        visibility = 0.20;
    }
    return visibility;
}

void main(){
    // // Output the normal as color.
    vec3 lightDir = normalize(lightPos - fragPosition);

    vec3 normal = normalize(fragNormal);
    
    vec3 diffuseColor = kd * max(dot(normal,lightDir), 0.0);
    float visibility = 1.0;
    vec3 color = lightColor;
    if(shadows != 0){
        visibility = calculateShadow(lightMVP, shadowMap);
    }
    
    vec3 fragColor = vec3( color * visibility * diffuseColor);

    vec3 lightDir1 = normalize(lightPos1 - fragPosition);
    
    vec3 diffuseColor1 = kd * max(dot(normal, lightDir1), 0.0);
    float visibility1 = 1.0;
    vec3 color1 = lightColor1;
    if(shadows != 0){
        visibility1 = calculateShadow(lightMVP1, shadowMap1);
    }
    
    vec3 fragColor1 = vec3(color1 * visibility1* diffuseColor1);

    outColor = vec4(vec3(fragColor + fragColor1), 1.0);

}