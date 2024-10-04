#version 410 core

in vec2 uv;
out vec4 FragColor;

uniform sampler2D prevColorTex;

void main() {
    
    FragColor = texture(prevColorTex, uv);
}
