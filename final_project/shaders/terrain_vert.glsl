// vertex shader
#version 410 core

uniform mat4 mvpMatrix;

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec4 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;

void main()
{
    // convert XYZ vertex to XYZW homogeneous coordinate
    gl_Position = vec4(position, 1.0);

    fragPosition = (mvpMatrix * vec4(position, 1));
    fragTexCoord = texCoord;
}