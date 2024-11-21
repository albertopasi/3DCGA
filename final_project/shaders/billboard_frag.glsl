#version 330                                                                        
                                                                                    
uniform sampler2D gColorMap;                                                       
                                                                                    
in vec2 TexCoord;                                                                
out vec4 FragColor;                                                                 
                                                                                    
void main()                                                                         
{                                                                                   
    FragColor = texture(gColorMap, TexCoord); 

    float threshold = 0.6;

    // here we discard any fragment that has FragColor.rgb <= vec3(threshold, threshold, threshold)
    if(FragColor.r <= threshold && FragColor.g <= threshold && FragColor.b <= threshold){
        discard;
    }

    // Same here but for FragColor.rgb >= vec3(0.9, 0.9, 0.9)
    if (FragColor.r >= 0.9 && FragColor.g >= 0.9 && FragColor.b >= 0.9) {           
        discard;                                                                    
    }                                                                               
}