#version 410

// Global variables for lighting calculations
uniform vec3 lightPos;      // World-space position of the light
uniform vec3 lightColor;    // Color of the light
uniform vec3 kd;            // Diffuse reflection coefficient (material color)

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 norm = normalize(fragNormal);               // Normalize normal vector

    vec3 lightDir = normalize(lightPos - fragPos);
    
    // Final diffuse color using kd and lightColor
    vec3 diffuseColor = kd * max(dot(norm,lightDir), 0.0) * lightColor;

    // Output the final color, with an alpha value of 1.0
    outColor = vec4(diffuseColor, 1.0);
    
}