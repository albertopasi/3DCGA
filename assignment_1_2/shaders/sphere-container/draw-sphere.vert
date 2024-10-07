#version 410

uniform mat4 viewProjection;
uniform vec3 center;
uniform float radius; 

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;


void main() {
    // Compute world-space and NDC coordinates
    vec3 worldSpacePosition = (position * radius) + center;
    gl_Position             = viewProjection * vec4(worldSpacePosition, 1);
    
    // Set output variables
    fragPosition    = worldSpacePosition;
    fragNormal      = normal;
}
