#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

out vec2 fragTexCoord;
out float timeOffset; // Time offset for unique animation per particle

uniform mat4 viewProj;
uniform float time; // Current time passed from the application

void main()
{
    fragTexCoord = texCoord;
    timeOffset = fract(position.x * 3.1415 + position.y * 2.718); // Random offset per particle

    gl_Position = viewProj * vec4(position , 1.0);
}
