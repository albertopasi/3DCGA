#version 330                                                                        
                                                                                    
layout(points) in;                                                                  
layout(points) out;                                                                 
layout(max_vertices = 30) out;                                                      
                                                                                    
in float Type0[];                                                                   
in vec3 Position0[];                                                                
in vec3 Velocity0[];                                                                
in float Age0[];                                                                    
                                                                                    
out float Type1;                                                                    
out vec3 Position1;                                                                 
out vec3 Velocity1;                                                                 
out float Age1;                                                                     
                                                                                    
uniform float gDeltaTimeMillis;                                                     
uniform float gTime;                                                                
uniform sampler1D gRandomTexture;                                                   
uniform float gLauncherLifetime;                                                    
uniform float gShellLifetime;                                                       
//uniform float gSecondaryShellLifetime;                                              
                                                                                    
#define PARTICLE_TYPE_LAUNCHER 0.0f                                                 
#define PARTICLE_TYPE_SHELL 1.0f                                                    
#define PARTICLE_TYPE_SECONDARY_SHELL 2.0f                                          
                                                                                    
vec3 GetRandomDir(float TexCoord)                                                   
{                                                                                   
     vec3 Dir = texture(gRandomTexture, TexCoord).xyz;                              
     Dir -= vec3(0.5, 0.5, 0.5);                                                    
     return Dir;                                                                    
}                                                                                   
                                                                                    
float distancePointToAxis(vec3 point, vec3 axisPoint, vec3 axisDir) {
    // Calcolo del vettore dal punto sull'asse al punto specificato
    vec3 diff = point - axisPoint;
    
    // Calcolo del prodotto vettoriale tra `axisDir` e `diff`
    vec3 crossProd = cross(axisDir, diff);

    // La distanza è la lunghezza del prodotto vettoriale
    return length(crossProd);
}

vec3 biasedRandomDirectionInCone(vec3 axis, float maxAngle, float biasFactor, vec2 randomSeed) {
    // Normalizza l'asse per sicurezza
    axis = normalize(axis);

    // Generazione del coseno dell'angolo theta con bias verso valori vicini a 1
    float cosMaxAngle = cos(maxAngle);
    float randomCosTheta = mix(cosMaxAngle, 1.0, pow(randomSeed.x, biasFactor));
    float sinTheta = sqrt(1.0 - randomCosTheta * randomCosTheta);

    // Generazione di un angolo phi casuale tra 0 e 2*pi
    float phi = 2.0 * 3.14159265358979323846 * randomSeed.y;

    // Calcolo della direzione casuale iniziale rispetto all'asse z
    vec3 direction = vec3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        randomCosTheta
    );

    // Se l'asse è già l'asse z, restituiamo direttamente il vettore
    if (abs(axis.z - 1.0) < 0.0001) {
        return direction;
    }

    // Altrimenti, ruotiamo `direction` per allinearlo con l'asse `axis`
    vec3 up = vec3(0.0, 0.0, 1.0);
    vec3 v = normalize(cross(up, axis));
    float c = dot(up, axis);
    float k = 1.0 / (1.0 + c);

    // Rotazione di Rodrigues per allineare `direction` con `axis`
    vec3 rotatedDirection = direction * c + cross(v, direction) + v * dot(v, direction) * k;

    return rotatedDirection;
}

void main()                                                                         
{                                                                                   
    float Age = Age0[0] + gDeltaTimeMillis;                                         
    vec3 axis = vec3(0.0, 1.0, 0.0);
    float maxAngle = radians(70.0);
    vec3 origin = vec3(0.0, 1.0, 0.0);
    float minShellLifeTime = 10.0;
    float biasFactor = 1.0;                                                        
    if (Type0[0] == PARTICLE_TYPE_LAUNCHER) {                                       
        if (Age >= gLauncherLifetime) {                                             
            for(int i = 0; i < 20; i++){
                Type1 = PARTICLE_TYPE_SHELL;                                            
                Position1 = Position0[0];                                               
                vec2 randomSeed = vec2(fract(sin((gTime + i) * 12.9898) * 43758.5453),
                           fract(sin((gTime + i + 1) * 78.233) * 43758.5453));
                vec3 Dir = biasedRandomDirectionInCone(axis, maxAngle, biasFactor, randomSeed);                                   
                Velocity1 = normalize(Dir) / 20.0;                                      
                Age1 = 0.0;                                                             
                EmitVertex();                                                           
                EndPrimitive();   
            }                                                      
            Age = 0.0;                                                              
        }                                                                           
                                                                                    
        Type1 = PARTICLE_TYPE_LAUNCHER;                                             
        Position1 = Position0[0];                                                   
        Velocity1 = Velocity0[0];                                                   
        Age1 = Age;                                                                 
        EmitVertex();                                                               
        EndPrimitive();                                                             
    }                                                                               
    else {                                                                          
        float DeltaTimeSecs = gDeltaTimeMillis / 1000.0f;                           
        float t1 = Age0[0] / 1000.0;                                                
        float t2 = Age / 1000.0;                                                    
        vec3 DeltaP = DeltaTimeSecs * Velocity0[0];                                 
        vec3 DeltaV = vec3(DeltaTimeSecs) * (0.0, 0.0, 0.0);                      
                                                                                    
        if (Type0[0] == PARTICLE_TYPE_SHELL)  {    
            vec3 pos = Position0[0];
            float dist = distancePointToAxis(pos, origin, axis);
            float t = clamp((dist - 0.0) / (0.05 - 0.0), 0.0, 1.0);    
            float shellLifetime = mix(minShellLifeTime, gShellLifetime, 1.0 - t);
	        if (Age < shellLifetime) {                                             
	            Type1 = PARTICLE_TYPE_SHELL;                                        
	            Position1 = Position0[0] + DeltaP;                                  
	            Velocity1 = Velocity0[0] + DeltaV;                                  
	            Age1 = Age;                                                         
	            EmitVertex();                                                       
	            EndPrimitive();                                                     
	        }                                                                       
            else {                                                                  
                for (int i = 0 ; i < 10 ; i++) {                                    
                     Type1 = PARTICLE_TYPE_SECONDARY_SHELL;                         
                     Position1 = Position0[0];                                      
                     vec3 Dir = GetRandomDir((gTime + i)/1000.0);                   
                     Velocity1 = normalize(Dir) / 20.0;                             
                     Age1 = 0.0f;                                                   
                     EmitVertex();                                                  
                     EndPrimitive();                                                
                }                                                                   
            }                                                                       
        }                                                                                                                                                  
    }                                                                               
}                                                                                   