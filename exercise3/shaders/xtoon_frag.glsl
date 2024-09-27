#version 410

// Global variables for lighting calculations
//uniform vec3 viewPos;
uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform sampler2D texToon;

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
    float distanceCameraToFrag = length(cameraPos - fragPos);

    float distanceFactor = (distanceCameraToFrag+10.0f);
    outColor = texture(texToon, vec2(lambertian, distanceCameraToFrag));
}