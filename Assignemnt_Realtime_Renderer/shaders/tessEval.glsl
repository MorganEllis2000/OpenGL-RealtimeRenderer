#version 400

layout(triangles, equal_spacing, ccw) in;

in TessCont
{
	vec4 Normal;
	vec2 UV;
	vec4 eyeDir;
	vec4 lightDir;
}tessCS[];

out TessEval
{
	vec4 Normal;
	vec2 UV;
	vec4 eyeDir;
	vec4 lightDir;
}tessEval;

uniform sampler2D displacementTexture;
float displacementScale;
float yHeight;

uniform vec4 cameraPosition;

uniform float waveHeight;
uniform float waveSpeed;

uniform float u_Time;

void main()
{
	vec4 p0 = gl_in[0].gl_Position;
	vec4 p1 = gl_in[1].gl_Position;
	vec4 p2 = gl_in[2].gl_Position;
	vec3 p = gl_TessCoord.xyz;

	tessEval.Normal		= normalize(tessCS[0].Normal	* p.x + tessCS[1].Normal	* p.y + tessCS[2].Normal	* p.z);
	tessEval.UV			= tessCS[0].UV		* p.x + tessCS[1].UV		* p.y + tessCS[2].UV		* p.z;
	tessEval.eyeDir		= normalize(tessCS[0].eyeDir	* p.x + tessCS[1].eyeDir	* p.y + tessCS[2].eyeDir	* p.z);
	tessEval.lightDir	= normalize(tessCS[0].lightDir	* p.x + tessCS[1].lightDir	* p.y + tessCS[2].lightDir	* p.z);

	gl_Position = p0 * p.x + p1 * p.y + p2 * p.z;

	float distToCamera = length(gl_Position - tessEval.eyeDir);

	p.y = 10.f;

	if(distToCamera > 30.f){
		displacementScale = 0.5f;
	}else if(distToCamera < 30.f && distToCamera > 20.f){
		displacementScale = 1.f;
	}else if(distToCamera < 20.f && distToCamera > 10.f){
		displacementScale = 2.f;
	}else if(distToCamera < 10.f && distToCamera > 5.f){
		displacementScale = 3.f;
	} else if(distToCamera < 5.f){
		displacementScale = 4.f;
	}else{
		displacementScale = 0.f;
	}

	float y = sin(u_Time * waveSpeed) * waveHeight;
	//y = y * waveSpeed;

	const float timespan = 3000.0f; // e.g. 3 seconds
    //float y = sin(2.0f * 3.14159f * u_Time / timespan) * 0.5f + 0.5f;

	float dist = texture(displacementTexture, tessEval.UV).r;
	gl_Position.y += displacementScale * dist + y;
}