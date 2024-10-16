#version 410

// Output for shape id
layout(location = 0) out int shape_id;

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

//The type of the shape we are rasterizing, the same as the enumerator in shapes.h
// 0 - circles
// 1 - lines
// 2 - BezierCurves (Unused)
uniform uint shape_type;

//The maximum distance to the shape for a pixel to be part of the shape
uniform float rasterize_width;

void main()
{   
    // Default value: -1 means no circle found
    shape_id = -1;
    vec2 pixel_pos = gl_FragCoord.xy;

    // ---- CIRCLE
    if (shape_type == 0) {
        for (int i = 0; i < circle_count; i++) {
            Circle c = circles[i];

            float dist = distance(pixel_pos, c.position);
            if (dist <= c.radius + rasterize_width && dist >= c.radius - rasterize_width) {
                shape_id = i;
                break;
            }
        }
    }
    // ---- LINE
    else if (shape_type == 1) {
        for(int i=0; i< line_count; i++){
            Line l = lines[i];

            float distanceStart = distance(pixel_pos, l.start_point);
            float distanceEnd = distance(pixel_pos, l.end_point);

            //check if pixel is the two semicircles of at the start and end of the line
            if(distanceStart <= rasterize_width || distanceEnd <= rasterize_width){
                shape_id = i;
                break;
            }

            vec2 lineDir = normalize(l.end_point - l.start_point);
            vec2 pixelToStart = pixel_pos - l.start_point;

            float projectionLength = dot(pixelToStart, lineDir);
            vec2 projectedPoint = l.start_point + projectionLength * lineDir;

            float distanceProjToStart = distance(l.start_point, projectedPoint);
            float distanceProjToEnd = distance(l.end_point, projectedPoint);
            float lineLength = length(l.end_point - l.start_point);
           
           //check if pixel is within rasterize_with from the line (excluding the semicircles)
            if (distanceProjToStart <= lineLength && distanceProjToEnd <= lineLength) {
                float distancePixelToLine = distance(pixel_pos, projectedPoint);
                if(distancePixelToLine <= rasterize_width){
                    shape_id = i;
                    break;
                }
            }
        }
    }
}