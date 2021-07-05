#version 400

in TessEval
{
	vec4 Normal;
	vec2 UV;
	vec4 eyeDir;
	vec4 lightDir;
}tessEval;

out vec4 FragColor;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D specularTexture;
uniform sampler2D occlusionTexture;

struct Light
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

const Light sceneLight = Light(vec4( 0.25, 0.25,0.25,1.0), vec4(1.f,1.f,1.f,1.f), vec4(1.f,0.0f,0.0f,1.f));
const float specularTerm = 32.f;

#define PI 3.14159265

const int NUMBER_OF_LIGHTS = 3;
uniform vec4 LightPosition[NUMBER_OF_LIGHTS];
uniform float LightAttenuation[NUMBER_OF_LIGHTS];
uniform vec4 LightAmbient[NUMBER_OF_LIGHTS];
uniform vec4 LightDiffuse[NUMBER_OF_LIGHTS];
uniform vec4 LightSpecular[NUMBER_OF_LIGHTS];

float orenNayarDiffuse(  vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness, float albedo)
{
	float LdotV = dot(lightDirection, viewDirection);
	float NdotL = dot(lightDirection, surfaceNormal);
	float NdotV = dot(surfaceNormal, viewDirection);

	float s = LdotV - NdotL * NdotV;
	float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));

	float sigma2 = roughness * roughness;
	float A = 1.0 + sigma2 * (albedo / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
	float B = 0.45 * sigma2 / (sigma2 + 0.09);

	return albedo * max(0.0, NdotL) * (A + B * s / t) / PI;
}

void main()
{
	//get the texture
	vec4 albedo		=	texture2D( diffuseTexture,		tessEval.UV);
	vec4 occlusion	=	texture2D( occlusionTexture,	tessEval.UV);
	vec4 normal		=	texture2D( normalTexture,		tessEval.UV) * 2.0 - 1.0;
	
	//calulate diffuse 
	vec3 diffuse = max(0,dot(normal, tessEval.lightDir)) * LightDiffuse[0].xyz;
	//float intensity = orenNayarDiffuse( vertex.lightDir.xyz, vertex.eyeDir.xyz, normal.xyz, roughness,oAlbedo);
	//vec3 diffuse = vec3(intensity);
	//calculate specular
	vec3 halfDir = normalize(tessEval.eyeDir + tessEval.lightDir).xyz;
	vec3 specular = pow(max(0,dot(halfDir, normal.xyz)),specularTerm) * LightSpecular[0].xyz;
	vec3 linearColour = vec3(0.5) + diffuse.xyz + specular.xyz;

	FragColor = vec4(albedo.rgb * linearColour * occlusion.rgb,1.0);	
}
