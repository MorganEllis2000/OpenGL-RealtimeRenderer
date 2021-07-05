#version 330

const int NUMBER_OF_LIGHTS = 10;
const int NUMBER_OF_SPOTLIGHTS = 5;

in vec4 vPosition; // World space position
in vec4 vColour;
in vec4 vNormal;
in vec2 vTexCoord;

out vec4 fragColour;

uniform vec4 CameraPosition;
uniform vec4 LightPosition[NUMBER_OF_LIGHTS];

uniform vec4 LightDiffuse[NUMBER_OF_LIGHTS];
uniform vec4 LightAmbient[NUMBER_OF_LIGHTS];
uniform vec4 LightSpecular[NUMBER_OF_LIGHTS];
uniform float LightAttenuation[NUMBER_OF_LIGHTS];
uniform float LightInnerRadius[NUMBER_OF_LIGHTS];

uniform vec4 matAmbient;
uniform vec4 matDiffuse;
uniform vec4 matSpecular;

uniform sampler2D DiffuseTexture;

uniform float Cutoff[NUMBER_OF_LIGHTS];
uniform float OuterCutoff[NUMBER_OF_LIGHTS];

void main(){
	vec4 albedo = texture(DiffuseTexture, vTexCoord);
	vec4 litColour = vec4(0.f, 0.f, 0.f, 1.f);
	for(int i = 0; i < NUMBER_OF_LIGHTS; ++i){
		float distToLight = length(vPosition - LightPosition[i]);

		if(distToLight < LightAttenuation[i]){
			float attenuation = clamp(2.0 / distToLight, 0.0, 1.0);
			vec4 ambient = LightAmbient[i] * matAmbient;
			vec4 lightDir = normalize(vPosition - LightPosition[i]);
			vec4 diffuse = matDiffuse * LightDiffuse[i] * max(0.f, dot(vNormal, - lightDir));
			vec4 reflectionVec = reflect(lightDir, vNormal);
			vec4 eyeDir = normalize(CameraPosition - vPosition);
			vec4 specular = matSpecular * vec4(LightSpecular[i].xyz, 1.0) * max(0.f, pow(dot(eyeDir, reflectionVec), LightSpecular[i].a));

			if(i > 4 ){
				vec3 lightDirection = vec3(LightPosition[i].x, LightPosition[i].y, LightPosition[i].z); 
				float theta = dot(lightDir.xyz, normalize(-lightDirection));
				float epsilon = Cutoff[i] - OuterCutoff[i];
				float intensity = clamp((theta - OuterCutoff[i]) / epsilon, 0.0, 1.0);
				diffuse *= intensity;
				specular *= intensity;
				litColour += ambient * (diffuse + specular);
			} else{
				litColour += ambient * (attenuation + diffuse + specular);
			}
		}
	}
	fragColour = albedo * litColour;
}

