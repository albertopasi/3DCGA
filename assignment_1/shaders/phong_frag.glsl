#version 410

// Global variables for lighting calculations
uniform vec3 cameraPos;  // Position of the camera
uniform vec3 lightPos;   // Position of the light
uniform vec3 lightColor; // Color of the light
uniform vec3 ks;        // Specular coefficient from shadingData
uniform float shininess; // Shininess factor for specular highlights

// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D texShadow;  
uniform sampler2D texLight;  

// scene uniforms
uniform mat4 lightMVP;
// config uniforms, use these to control the shader from UI
uniform int shadows = 0;
uniform int samplingMode = 0;
uniform int lightMode = 0;
uniform int lightColorMode = 0;
// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

vec3 lightTextureColor(){
     vec4 fragLightCoord = lightMVP * vec4(fragPos, 1.0);

    // Divide by w because fragLightCoord are homogeneous coordinates
    fragLightCoord.xyz /= fragLightCoord.w;

    // The resulting value is in NDC space (-1 to +1),
    //  we transform them to texture space (0 to 1).
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;
    vec2 shadowMapCoord = fragLightCoord.xy;
    
    return texture(texLight, shadowMapCoord).xyz;
}

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
    vec3 norm = normalize(fragNormal);              // Normalize normal vector
    vec3 lightDir = normalize(lightPos - fragPos);  // Light direction
    vec3 viewDir = normalize(cameraPos - fragPos);  // View direction
    vec3 refl = reflect(-lightDir, norm);            // Reflection vector

    // Calculate the specular component
    vec3 specular = ks * pow(max(dot(refl, viewDir), 0.0), shininess);
    float visibility = 1.0;
    vec3 color = lightColor;
    if(shadows!=0){
        visibility = calculateShadow();
    }
    if(lightMode != 0){
        visibility *= calculateSpotlight();
    }
    if(lightColorMode != 0){
        color = lightTextureColor();
    }
    outColor = vec4(vec3( color * visibility * specular), 1.0);

}