#version 330

in vec4 Position;
in vec4 Colour;
in vec4 Normal;
in vec2 TexCoord;

out vec4 vPosition;
out vec4 vColour;
out vec4 vNormal;
out vec2 vTexCoord;

uniform mat4 ProjectionView;
uniform mat4 ModelMatrix;
uniform mat4 NormalMatrix;

void main (){
	vColour = Colour;
	vNormal = normalize(NormalMatrix * Normal);
	vPosition = ModelMatrix * Position;
	vTexCoord = TexCoord;
	gl_Position = ProjectionView * ModelMatrix * Position;
}