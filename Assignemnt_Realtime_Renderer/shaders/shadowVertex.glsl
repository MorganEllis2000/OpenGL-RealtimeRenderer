#version 150

in vec4 Position;
in vec4 Colour;
in vec4 Normal;
in vec2 Tex1;

out Vertex
{	
	vec4 Pos;
	vec4 Normal;
	vec4 Colour;
	vec2 UV;
	vec3 HalfDir;
	vec3 LightDir;
	vec4 ShadowCoord;
}vertex;

uniform mat4 ProjectionView;
uniform mat4 Model;
uniform mat4 NormalMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ShadowProjectionView;

uniform vec4 cameraPosition;
uniform vec4 lightDirection;

const mat4 SHADOW_BIAS_MATRIX = mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.5, 1.0
					);



void main()
{
	vertex.Pos = Model * Position;
	vertex.Normal = ViewMatrix * NormalMatrix * Normal;
	vertex.Colour = Colour;
	vertex.UV = Tex1;

	vertex.LightDir = (ViewMatrix * -lightDirection).xyz;

	vec3 eyeDir = (ViewMatrix * normalize(cameraPosition - vertex.Pos)).xyz;
	vertex.HalfDir = normalize(eyeDir + vertex.LightDir);

	vertex.ShadowCoord = SHADOW_BIAS_MATRIX * ShadowProjectionView * Position;

	gl_Position = ProjectionView * Model * Position;
}