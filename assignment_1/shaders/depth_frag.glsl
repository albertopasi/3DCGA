#version 410 

in vec4 Color;
in vec3 uv;
layout (location=0) out vec4 FragColor;

uniform int peel;
uniform sampler2D prevDepthTex;

void main() {
    if(peel != 0){
        float prevDepth = texture(prevDepthTex, uv.xy).r;
        if (uv.z <= prevDepth) {
            discard;     
        }
    }
        FragColor = Color;
        gl_FragDepth = gl_FragCoord.z;
}
