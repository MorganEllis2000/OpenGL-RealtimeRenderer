#version 400

in vec4 Position; 
in vec4 Normal;
in vec4 Tangent;
in vec2 UV;

out Vertex
{
	vec4 Normal;
	vec2 UV;
	vec4 eyePos;
	vec4 lightDir;

} vertex;


uniform mat4 ProjectionView; 
uniform mat4 ViewMatrix;
uniform mat4 Model;

uniform vec4 lightDirection;
uniform vec4 cameraPosition;

void main() 
{ 
	vertex.Normal = Normal;
	vertex.UV = UV;
	vec3 bitangent = cross(Tangent.xyz, Normal.xyz);
	mat3 TBN = transpose(mat3(Tangent.xyz, bitangent, vertex.Normal.xyz));

	vertex.eyePos = vec4( TBN * normalize(ViewMatrix * cameraPosition - (ViewMatrix * Model * Position)).xyz, 0);
	vertex.lightDir = vec4(TBN* normalize(ViewMatrix * -lightDirection).xyz, 0);
	gl_Position = ProjectionView * Model * Position; 
}