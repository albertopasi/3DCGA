#version 410 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D minimapTexture;

void main()
{
    FragColor = texture(minimapTexture, TexCoords);
}
