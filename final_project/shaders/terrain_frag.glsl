#version 410 core

in float Height;
in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D textureSnow;
uniform sampler2D textureGrass;
uniform sampler2D textureGround;
uniform sampler2D textureRock;

uniform float heightSnow = 0.875;
uniform float heightGrass = 0.625;
uniform float heightGround = 0.375;
uniform float heightRock = 0.275;

void main()
{
	float h = (Height)/8.0;
	// float h = Height;
	vec4 color;

	if (h < heightRock) {
       color = texture(textureRock, texCoord);
    } else if (h < heightGround) {
       vec4 Color0 = texture(textureRock, texCoord);
       vec4 Color1 = texture(textureGround, texCoord);
       float Delta = heightGround - heightRock;
       float Factor = (h - heightRock) / Delta;
       color = mix(Color0, Color1, Factor);
    } else if (h < heightGrass) {
       vec4 Color0 = texture(textureGround, texCoord);
       vec4 Color1 = texture(textureGrass, texCoord);
       float Delta = heightGrass - heightGround;
       float Factor = (h - heightGround) / Delta;
       color = mix(Color0, Color1, Factor);
    } else if (h < heightSnow) {
       vec4 Color0 = texture(textureGrass, texCoord);
       vec4 Color1 = texture(textureSnow, texCoord);
       float Delta = heightSnow - heightGrass;
       float Factor = (h - heightGrass) / Delta;
       color = mix(Color0, Color1, Factor);
    } else {
       color = texture(textureSnow, texCoord);
    }
	FragColor = color;
	//FragColor = vec4(h, h, h, 1.0);
}