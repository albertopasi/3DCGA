#version 410

// Per-vertex attributes
in vec3 pos; // World-space position


void main() {
    //Simply pass vertex positon
    gl_Position = vec4(pos,1);
}