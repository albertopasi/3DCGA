#version 410 core

in vec4 Color;
in vec3 Pos;
out vec4 FragColor;

uniform int peel = 0;
uniform sampler2D prevDepthTex;  // Profondità del pass precedente

void main() {
    if(peel != 0){
        float prevDepth = texture(prevDepthTex, gl_FragCoord.xy).r;
        if (gl_FragCoord.z <= prevDepth) {
            discard;  // Scarta i frammenti che non sono "oltre" il precedente strato
        }
    }
    
    FragColor = Color; // Esempio di colore trasparente
    //gl_FragColor = Color;
}
