#version 410

// Global variables for lighting calculations
//uniform vec3 viewPos;
uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform sampler2D texToon;
uniform float shininess; // Shininess factor for specular highlights

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal


void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float lambertian = max(dot(normal, lightDir), 0.0);
    
    vec3 H = normalize(lightPos + normalize(cameraPos));
    float blinn = pow(dot(H, normal), shininess);

    float brightness = lambertian + blinn;
    float distanceCameraToFrag = length(cameraPos - fragPos);
    
    float zmin = 1.0f;

    float zmax = shininess * zmin;
    float value = 1.0 - log(distanceCameraToFrag/zmin)/log(zmax/zmin);
    outColor = texture(texToon, vec2(brightness, -value));
}