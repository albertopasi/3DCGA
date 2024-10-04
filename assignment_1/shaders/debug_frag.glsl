#version 410

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragNormal; // World-space normal

void main() {
    // Output the normal as color
    outColor = vec4(abs(fragNormal), 1.0);
}