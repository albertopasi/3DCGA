#version 330                                                                        
                                                                                    
layout(points) in;                                                                  
layout(triangle_strip) out;                                                         
layout(max_vertices = 4) out;                                                       
                                                                                    
uniform mat4 gVP;                                                                   
uniform vec3 gCameraPos;                                                            
uniform float gBillboardSize;  
uniform float maxLife;

in float life[];                                                          
out vec2 TexCoord;                                                      
                                                                                    
void main()                                                                         
{                                                                                   
    vec3 Pos = gl_in[0].gl_Position.xyz;                                            
    vec3 toCamera = normalize(gCameraPos - Pos);                                    
    vec3 up = vec3(0.0, 1.0, 0.0);                                                  
    vec3 right = cross(toCamera, up) * gBillboardSize; 

    float particleLife = life[0];                                                   
    float timeFract = maxLife / 24.0;
    int texNumb = int(particleLife / timeFract);
    int row = ((texNumb + 40) % 8);
    int col = (texNumb + 40) / 8;                                                                         
                                                                                    
    Pos -= right;                                                                   
    gl_Position = gVP * vec4(Pos, 1.0);                                             
    TexCoord = vec2(row / 8.0, col / 8.0);
    EmitVertex();                                                                   
                                                                                    
    Pos.y += gBillboardSize;                                                        
    gl_Position = gVP * vec4(Pos, 1.0);                                             
    TexCoord = vec2(row / 8.0, (col + 1) / 8.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    Pos.y -= gBillboardSize;                                                        
    Pos += right;                                                                   
    gl_Position = gVP * vec4(Pos, 1.0);                                             
    TexCoord = vec2((row + 1) / 8.0, col / 8.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    Pos.y += gBillboardSize;                                                        
    gl_Position = gVP * vec4(Pos, 1.0);                                             
    TexCoord = vec2((row + 1) / 8.0, (col + 1) / 8.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    EndPrimitive();                                                                 
}                                                                                   