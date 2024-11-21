//
//  Created by Lorenzo Sibi on 17/10/2024
//

#version 410 core

out vec4 FragColor;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec3 camPosition;
in vec3 lightPosition;

uniform vec3 lightColor;
uniform bool isMetallic;

uniform bool enableNormalMapping;

// Material parameters
uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D normalMap;

// Constants
const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// Shadowing-masking  geometry (Smith's method)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Material properties
    vec3 albedo = texture(albedoMap, fragTexCoord).rgb;
    float metallic = isMetallic ? texture(metallicMap, fragTexCoord).r : 0.0f;
    float roughness = texture(roughnessMap, fragTexCoord).r;
    float ao = texture(aoMap, fragTexCoord).r;
    
    // Lighting and camera directions
    vec3 N = normalize(fragNormal); // Normale
    if(enableNormalMapping){
        vec3 normal = texture(normalMap, fragTexCoord).rgb;
        N = normalize(normal * 2.0 - 1.0);
    }
    vec3 V = normalize(camPosition - fragPosition); // Camera's direction
    vec3 L = normalize(lightPosition - fragPosition); // Light's direction
    vec3 H = normalize(V + L); // Halfway vector

    // Fresnel factor
    vec3 F0;
    if(isMetallic)
        F0 = mix(vec3(0.04), albedo, metallic); // The albedo's vector influences on F0 ans metallic > 0
    else
        F0 = vec3(0.04);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // Normal Distribution Function (GGX)
    float NDF = DistributionGGX(N, H, roughness);

    // Geometry (Smith model)
    float G = GeometrySmith(N, V, L, roughness);

    // Specular BRDF
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // Evita divisione per zero
    vec3 specular = numerator / denominator;

    // Diffuse term (Lambertian)
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;  // Specular reflection subtracted from 1 to obtain the diffusive factor

    if(isMetallic)
        kD *= 1.0 - metallic;

    vec3 irradiance = lightColor * max(dot(N, L), 0.0); // Light's irradiance
    vec3 diffuse = albedo * irradiance / PI;

    // Ambient occlusion
    vec3 ambient = vec3(0.3) * albedo * ao;

    vec3 color = ambient + (kD * diffuse + specular) * irradiance;

    FragColor = vec4(color, 1.0);
}