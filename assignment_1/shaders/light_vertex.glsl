#version 410
uniform vec4 pos; // Screen-space position

void main()
{
    gl_Position = pos;
}

