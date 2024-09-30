#version 410

// Global variables for lighting calculations
uniform vec3 cameraPos;  // Position of the camera
uniform vec3 lightPos;   // Position of the light
uniform vec3 lightColor; // Color of the light
uniform vec3 ks;         // Specular coefficient from shadingData
uniform float shininess; // Shininess factor for specular highlights

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos;  // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 norm = normalize(fragNormal);                // Normalize normal vector
    vec3 lightDir = normalize(lightPos - fragPos);    // Light direction
    vec3 viewDir = normalize(cameraPos - fragPos);    // View direction
    
    // Calculate the half-vector H
    vec3 halfDir = normalize(lightDir + viewDir);     // Halfway vector

    // Calculate the Blinn-Phong specular component
    float specular = pow(max(dot(halfDir, norm), 0.0), shininess);

    vec3 finalColor = lightColor * ks * specular;      // Combine specular with light color
    outColor = vec4(finalColor, 1.0);                  // Output the final color
}
