#version 410 core

// Model/view/projection matrix
uniform mat4 mvp;

// Per-vertex attributes
in vec3 pos; // World-space position

void main() {
	// Transform 3D position into on-screen position
    gl_Position = mvp * vec4(pos, 1.0);
}