#version 410

// Output for on-screen color
layout(location = 0) out vec4 outColor;

//Textures
uniform sampler2D accumulator_texture;
uniform ivec2 screen_dimensions;

//Set gl_FragCoord to pixel center
layout(pixel_center_integer) in vec4 gl_FragCoord;

void main()
{
}
