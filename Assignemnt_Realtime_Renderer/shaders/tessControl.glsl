#version 400

layout(vertices = 3) out;

in Vertex
{
	vec4 Normal;
	vec2 UV;
	vec4 eyePos;
	vec4 lightDir;
}vertex[];

out TessCont
{
	vec4 Normal;
	vec2 UV;
	vec4 eyeDir;
	vec4 lightDir;
}tessCS[];

uniform int innerEdge;
uniform int outerEdge;

void main()
{
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	tessCS[gl_InvocationID].UV			= vertex[gl_InvocationID].UV;
	tessCS[gl_InvocationID].Normal		= vertex[gl_InvocationID].Normal;
	tessCS[gl_InvocationID].lightDir	= vertex[gl_InvocationID].lightDir;
	tessCS[gl_InvocationID].eyeDir		= vertex[gl_InvocationID].eyePos;

	gl_TessLevelOuter[0] = outerEdge;
	gl_TessLevelOuter[1] = outerEdge;
	gl_TessLevelOuter[2] = outerEdge;
	
	gl_TessLevelInner[0] = innerEdge;
}