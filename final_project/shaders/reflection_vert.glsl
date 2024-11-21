#version 410
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 Normal;
out vec3 Position;

uniform mat4 modelMatrix;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalModelMatrix;

void main()
{
    Normal = normalModelMatrix * aNormal;
    Position = vec3(modelMatrix * vec4(aPos, 1.0));
    gl_Position = projection * view * vec4(Position, 1.0);
}  