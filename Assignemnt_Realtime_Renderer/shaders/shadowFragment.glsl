#version 330

const int NUMBER_OF_LIGHTS = 3;

in Vertex
{
	vec4 Pos;
	vec4 Normal;
	vec4 Colour;
	vec2 UV;
	vec3 HalfDir;
	vec3 LightDir;
	vec4 ShadowCoord;
}vertex;

out vec4 FragColor;

uniform sampler2D DiffuseTexture;
uniform sampler2D ShadowTexture;

struct Light{
	vec4 pos;
	float attenuation;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform vec4 LightPosition[NUMBER_OF_LIGHTS];
uniform vec4 LightDiffuse[NUMBER_OF_LIGHTS];
uniform vec4 LightAmbient[NUMBER_OF_LIGHTS];
uniform vec4 LightSpecular[NUMBER_OF_LIGHTS];
uniform float LightAttenuation[NUMBER_OF_LIGHTS];
uniform float LightInnerRadius[NUMBER_OF_LIGHTS];
uniform float Cutoff[NUMBER_OF_LIGHTS];
uniform float OuterCutoff[NUMBER_OF_LIGHTS];

uniform vec4 matAmbient;
uniform vec4 matDiffuse;
uniform vec4 matSpecular;

const float specularTerm = 32.0;
const float SHADOW_BIAS = 0.002f;

uniform vec4 CameraPosition;

vec2 poissonDisk[4] = vec2[](
	vec2( -0.94201624, -0.39906216 ),
	vec2( 0.94558609, -0.76890725 ),
	vec2( -0.094184101, -0.92938870 ),
	vec2( 0.34495938, 0.29387760 )
);

void main()
{
	float shadowFactor = .2f;
	vec4 litColour = vec4(0.f, 0.f, 0.f, 1.f);
	//calculate shadow by testing depth

	// Poisson Sampling
	for (int i = 0; i < 4; ++i){
		if(texture(ShadowTexture, vertex.ShadowCoord.xy + poissonDisk[i]/700.0 ).z < vertex.ShadowCoord.z - SHADOW_BIAS)
		{
			shadowFactor -= 0.2;
		}
	}


	for(int i = 0; i < NUMBER_OF_LIGHTS; ++i){

		if(i == 0){
			vec3 albedo = texture(DiffuseTexture, vertex.UV).xyz;

			float distToLight = length(vertex.Pos - LightPosition[i]);
			float attenuation = clamp(2.0 / distToLight, 0.0, 1.0);

			//phong ambient colour
			vec3 ambient = LightAmbient[i].xyz;

			//phong diffuse
			vec3 diffuse = max(0, dot(vertex.Normal.xyz, vertex.LightDir))* LightDiffuse[i].xyz;

			//Calculate specular component
			vec3 specular = pow(max(0, dot(vertex.HalfDir, vertex.Normal.xyz)), specularTerm)* LightSpecular[i].xyz;

			vec3 linearColour = albedo * ambient + albedo * (diffuse + specular + attenuation) * shadowFactor;

			//gama correction
			vec3 gammaCorrected = pow(linearColour, vec3(1.0/2.2));

			FragColor = vec4(gammaCorrected, 1.0f);
		}
	}
}