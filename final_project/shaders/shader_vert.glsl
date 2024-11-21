#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;
uniform vec3 lightPos;
uniform vec3 camPos;
uniform bool enableNormalMapping;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec3 lightPosition;
out vec3 camPosition;

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1.0);
    fragNormal      = normalModelMatrix * normal;
    fragTexCoord    = texCoord;
    if(enableNormalMapping){
        vec3 t = normalize(vec3(modelMatrix * vec4(tangent,   0.0)));
        vec3 b = normalize(vec3(modelMatrix * vec4(bitangent, 0.0)));
        vec3 n = normalize(vec3(modelMatrix * vec4(normal,    0.0)));
        mat3 tbnMatrix = transpose(mat3(t, b, n));

        fragPosition    = tbnMatrix * (modelMatrix * vec4(position, 1.0)).xyz;
        lightPosition   = tbnMatrix * lightPos;
        camPosition     = tbnMatrix * camPos;

    }else{
        fragPosition    = (modelMatrix * vec4(position, 1.0)).xyz;
        lightPosition   = lightPos;
        camPosition     = camPos;
    }
}