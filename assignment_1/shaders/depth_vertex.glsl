#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

out vec4 Color;
out vec3 Pos;
out vec3 uv;

uniform mat4 mvp;

void main() {
    vec4 pos = mvp * vec4(aPos, 1.0);
    gl_Position = pos;
    Color = aColor;
    Pos = aPos;
    uv = vec3(vec2(pos.xy + vec2(1.0 , 1.0))/2.0, pos.z);
}
