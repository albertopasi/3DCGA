#version 410
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 camPos;
uniform samplerCube cubemap;

void main()
{   
    float ratio = 1.00 / 1.52 ;             //refractive index  (air: 1.00, water:1.33, ice:1.309, glass:1.52, diamond: 2.42)        
    vec3 pos = normalize(Position - camPos);
    vec3 refraction = refract(pos, normalize(Normal), ratio);
    FragColor = vec4(texture(cubemap, refraction).rgb, 1.0);
}