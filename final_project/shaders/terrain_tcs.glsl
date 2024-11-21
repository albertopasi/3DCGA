#version 410 core

layout (vertices=4) out;

// varying input from vertex shader
in vec4 fragPosition[];
in vec2 fragTexCoord[];
// varying output to evaluation shader
out vec2 TextureCoord[];

uniform vec3 camPos;

int octaves = 8;
float gDispFactor = 20.0;
float freq = 0.01f;

float power = 3.0f;

vec3 seed = vec3(1.0f);

// tassellation optimization for details ( the further away the camera is, the lower the level of detail.)
float GetTessLevel(float distance0, float distance1)
{
	float AvgDistance = (distance0 + distance1) / 2.0;
	if (AvgDistance <= 1.0)
		return 50.0;
	if (AvgDistance <= 2.0)
		return 10.0;
	else if (AvgDistance <= 5.0)
		return 10.0;
	else if(AvgDistance <= 12.0)
		return 2.5;
	else if (AvgDistance <= 300.0)
		return 1.0;
	else
		return 1.0;
}

void main()
{
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	TextureCoord[gl_InvocationID] = fragTexCoord[gl_InvocationID];

	if (gl_InvocationID == 0)
	{
		// const int MIN_TESS_LEVEL = 4;
		// const int MAX_TESS_LEVEL = 64;
		// const float MIN_DISTANCE = 20;
		// const float MAX_DISTANCE = 800;

		// // distance from camera
		// float distance00 = distance(camPos, fragPosition[0].xyz);
		// float distance01 = distance(camPos, fragPosition[1].xyz);
		// float distance10 = distance(camPos, fragPosition[2].xyz);
		// float distance11 = distance(camPos, fragPosition[3].xyz);

		// //tessellation level based on a GetTassLevel function
		// float tessLevel0 = GetTessLevel(distance10, distance00);
		// float tessLevel1 = GetTessLevel(distance00, distance01);
		// float tessLevel2 = GetTessLevel(distance01, distance11);
		// float tessLevel3 = GetTessLevel(distance11, distance10);

		// gl_TessLevelOuter[0] = tessLevel0;
		// gl_TessLevelOuter[1] = tessLevel1;
		// gl_TessLevelOuter[2] = tessLevel2;
		// gl_TessLevelOuter[3] = tessLevel3;
		// gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
		// gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
		const int MIN_TESS_LEVEL = 4;
        const int MAX_TESS_LEVEL = 64;
        const float MIN_DISTANCE = 1;
        const float MAX_DISTANCE = 10;

        vec4 eyeSpacePos00 = fragPosition[0];
        vec4 eyeSpacePos01 = fragPosition[1];
        vec4 eyeSpacePos10 = fragPosition[2];
        vec4 eyeSpacePos11 = fragPosition[3];

        // "distance" from camera scaled between 0 and 1
        float distance00 = clamp( (abs(eyeSpacePos00.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
        float distance01 = clamp( (abs(eyeSpacePos01.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
        float distance10 = clamp( (abs(eyeSpacePos10.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
        float distance11 = clamp( (abs(eyeSpacePos11.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );

        float tessLevel0 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance10, distance00) );
        float tessLevel1 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance01) );
        float tessLevel2 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance01, distance11) );
        float tessLevel3 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance11, distance10) );

        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
	}
}