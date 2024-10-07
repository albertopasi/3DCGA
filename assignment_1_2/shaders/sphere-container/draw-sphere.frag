#version 410

uniform vec3 color;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 fragColor;


void main() {
    fragColor = vec4(color, 1.0);
}
