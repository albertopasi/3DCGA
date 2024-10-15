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
    vec2 tex_coords = gl_FragCoord.xy / vec2(screen_dimensions);
    vec4 accumulated_data = texture(accumulator_texture, tex_coords);
    
    vec3 accumulated_color = accumulated_data.rgb; // RGB contains total accumulated color
    float sample_count = accumulated_data.a;       // Alpha contains the number of samples

    // Avoid division by zero in case no samples were accumulated
    if (sample_count > 0.0) {
        // Calculate the final color by dividing the accumulated color by the sample count (weighted average)
        outColor = vec4(accumulated_color / sample_count, 1.0); // Final color with alpha = 1.0
    } else {
        // If no samples, output black
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

}
