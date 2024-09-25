#version 410 core

// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D texShadow;

// scene uniforms
uniform mat4 lightMVP;
uniform vec3 lightPos = vec3(3, 3, 3);
// config uniforms, use these to control the shader from UI
uniform int samplingMode = 0;
uniform int peelingMode = 0;
uniform int lightMode = 0;
uniform int lightColorMode = 0;



// Output for on-screen color.
out vec4 outColor;

// Interpolated output data from vertex shader.
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    // Output the normal as color.
    vec3 lightDir = normalize(lightPos - fragPos);

    vec3 normal = fragNormal;

    outColor = vec4(vec3(max(dot(fragNormal, lightDir), 0.0)), 1.0);
}