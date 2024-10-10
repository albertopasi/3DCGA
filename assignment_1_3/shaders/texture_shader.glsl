#version 410

// Output for on-screen color
layout(location = 0) out vec4 outColor;

uniform isampler2D rasterized_texture;
uniform sampler2D accumulator_texture;

uniform ivec2 screen_dimensions;
uniform int texture_id;

//Set gl_FragCoord to pixel center
layout(pixel_center_integer) in vec4 gl_FragCoord;


void main()
{
	//Get location in texture space
	vec2 texel_coord = gl_FragCoord.xy / screen_dimensions;
	//texture_id 1 means rasterized_texture
	if (texture_id == 1) {
		//Rasterized texture is an int ranging from -1 to circle count, this will simply show the circles except the circle in index 0.
		int shape_id = texture(rasterized_texture, texel_coord).x;
		outColor = vec4(shape_id, 0, 0, 1);
	}
	//texture_id 2 means accumulator_texture
	else if (texture_id == 2) {
		outColor = texture(accumulator_texture, texel_coord);
	}
}