#version 410 core

// Model/view/projection matrix
uniform mat4 mvp;

// Per-vertex attributes
in vec3 pos; // World-space position
in vec3 normal; // World-space normal

// Data to pass to fragment shader
out vec3 fragPos;
out vec3 fragNormal;

void main() {
	// Transform 3D position into on-screen position
    gl_Position = mvp * vec4(pos, 1.0);

    // Pass position and normal through to fragment shader
    fragPos = pos;
    fragNormal = normal;
}
