#version 410 core

in vec4 Color;
in vec3 Pos;
out vec4 FragColor;

//uniform int peel;
//uniform sampler2D prevDepthTex;  // Profondità del pass precedente
uniform sampler2D prevColorTex;

void main() {
    
    FragColor = texture(prevColorTex, gl_FragCoord.xy); // Esempio di colore trasparente
}
