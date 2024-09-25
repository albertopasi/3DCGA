#version 410

uniform vec3 color;

// Output for on-screen color
out vec4 outColor;

void main()
{
    outColor = vec4(color, 1);
}