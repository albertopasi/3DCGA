#version 410
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 camPos;
uniform samplerCube cubemap;

void main()
{             
    vec3 pos = normalize(Position - camPos);
    vec3 reflection = reflect(pos, normalize(Normal));
    FragColor = vec4(texture(cubemap, reflection).rgb, 1.0);
}