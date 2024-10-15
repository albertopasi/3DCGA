#version 410

#define M_PI 3.14159265359f

// Output for accumulated color
layout(location = 0) out vec4 outColor;

//Set gl_FragCoord to pixel center
layout(pixel_center_integer) in vec4 gl_FragCoord;


//Circle and line struct equivalent to the one in shape.h
struct Circle {
    vec4 color;
    vec2 position;
    float radius;
};

struct Line {
    vec2 start_point;
    vec2 end_point;
    vec4 color_left[2];
    vec4 color_right[2];
};

//The uniform buffers for the shapes containing the count of shapes in the first slot and after that, the actual shapes
//We need to give the arrays a fixed size to work with opengl 4.1
layout(std140) uniform circleBuffer
{
    int circle_count;
    Circle circles[32];
};

layout(std140) uniform lineBuffer
{
    int line_count;
    Line lines[800];
};

//Textures for the rasterized shapes, and the accumulator
uniform isampler2D rasterized_texture;
uniform sampler2D accumulator_texture;

//The type of the shape we are rasterizing, the same as the enumerator in shapes.h
// 0 - circles
// 1 - lines
// 2 - BezierCurves (Unused)
uniform uint shape_type;

//The number of samples we have already taken
uniform uint frame_nr;
//Screen dimensions
uniform ivec2 screen_dimensions;

//Step size for ray-marching
uniform float step_size; 

//The maximum amount of raymarching steps we can take
uniform uint max_raymarch_iter;


//Random number generator outputs numbers between [0-1]
float get_random_numbers(inout uint seed) {
    seed = 1664525u * seed + 1013904223u;
    seed += 1664525u * seed;
    seed ^= (seed >> 16u);
    seed += 1664525u * seed;
    seed ^= (seed >> 16u);
    return seed * pow(0.5, 32.0);
}
vec2 march_ray(vec2 origin, vec2 direction, float step_size) {
    vec2 current_position = origin / screen_dimensions;
    step_size /= screen_dimensions.x;
    // return current_position;

    for (uint i = 0; i < max_raymarch_iter; ++i) {
        
        if (current_position.x < 0.0 || current_position.x > 1.0 || current_position.y < 0.0 || current_position.y > 1.0) {
            break; // Stop if the ray marches out of bounds
        }

        // Convert current_position to screen-space pixel coordinates for texture lookup
        vec2 pixel_coords = current_position;
        // Sample the shape ID from the rasterized texture
        int shape_id = texture(rasterized_texture, pixel_coords).r;


        // If a circle is hit
        if (shape_type == 0 && shape_id != -1) {
            // Retrieve the circle data
            Circle c = circles[shape_id];

            // Calculate the distance from the current position to the circle's center
            //float dist_to_circle = distance(current_position, c.position);

            // Check if the current position is exactly on the circle's circumference
            return current_position; // found the intersection

        }

        // Move the ray forward in the direction, by the step size
        current_position += direction * step_size;

        // Check if we're out of texture bounds (screen space is [0,1]^2)
        
    }

    // If no hit was found, return a sentinel value (-1, -1)
    return vec2(-1.0, -1.0);
}

void main()
{   // PREVIOUS CODE
    // //If a shape is hit we can sample it
    // bool hit = false;
    // if(hit){
    //     // ---- Circle
    //     if (shape_type == 0) {
    //     }
    //     // ---- Line
    //     else if (shape_type == 1) {
    //     }
    // }
    // outColor = vec4(0);
    // END OF PREVIOUS CODE

    // Set up the ray's origin and direction
    // Create a unique seed based on pixel coordinates and frame number
    uint seed = uint(gl_FragCoord.x) + uint(gl_FragCoord.y) * 49157u + frame_nr;

    // Generate a random angle for uniform direction sampling
    float random_angle = get_random_numbers(seed) * 2.0 * M_PI; // Random angle in radians
    vec2 random_direction = vec2(cos(random_angle), sin(random_angle)); // Create direction vector

    // Start ray marching from the pixel center
    vec2 origin = gl_FragCoord.xy / vec2(screen_dimensions); // Normalize to [0, 1]
    // Call the ray marching function to check for intersections
    vec2 hit_position = march_ray(origin, random_direction, step_size);

    // Initialize output color to black (in case no hit is found)
    outColor = vec4(0.0);

    // If the ray hits a shape (hit_position is not the sentinel value (-1, -1))
    if (hit_position != vec2(-1.0, -1.0)) {
        // Convert hit position to pixel coordinates in the rasterized texture
        ivec2 pixel_coords = ivec2(hit_position * vec2(textureSize(rasterized_texture, 0)));

        // Fetch the shape ID from the rasterized texture
        int shape_id = texelFetch(rasterized_texture, pixel_coords, 0).r;

        // Check if a circle is hit
        if (shape_type == 0 && shape_id != -1) {
            Circle c = circles[shape_id];

            // Set the output color to the circle's color
            outColor = c.color;
        }
        // Check if a line is hit (if you implement lines later)
        else if (shape_type == 1 && shape_id != -1) {
            Line l = lines[shape_id];

            // Set the output color to something based on line color
            // For example, you can interpolate based on distance along the line
            outColor = mix(l.color_left[0], l.color_right[0], 0.5); // Example blending
        }
    }
}
