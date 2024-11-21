#version 450 core

layout(vertices = 4) out;

uniform float segmentCount;

void main()
{

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	gl_TessLevelOuter[0] = 1;
	gl_TessLevelOuter[1] = segmentCount;
}