#ifndef __Assignemnt_Realtime_Renderer_H_
#define __Assignemnt_Realtime_Renderer_H_

#include "Application.h"
#include <glm/glm.hpp>
#include <string>

class FBXFile;

//A simple vertex structure containing position, normal and texture coordinate data
class SimpleVertex
{
public:
	enum VertexAttributeFlags
	{
		POSITION = (1 << 0), //The Position of the Vertex
		NORMAL = (1 << 1), //The Normal for the Vertex
		TANGENT = (1 << 2),
		TEXCOORD = (1 << 3), //The UV Coordinates for the mesh
	};

	enum Offsets
	{
		PositionOffset = 0,
		NormalOffset = PositionOffset + sizeof(glm::vec4),
		TangentOffset = NormalOffset + sizeof(glm::vec4),
		TexCoordOffset = TangentOffset + sizeof(glm::vec4),
	};

	SimpleVertex();
	~SimpleVertex();

	glm::vec4 position;
	glm::vec4 normal;
	glm::vec4 tangent;
	glm::vec2 texCoord;
};

//SimpleVertex inline constructor and destructor
inline SimpleVertex::SimpleVertex() : position(0, 0, 0, 1),
normal(0, 0, 0, 0), tangent(0, 0, 0, 0), texCoord(0, 0)
{
}

inline SimpleVertex::~SimpleVertex() {}

// Derived application class that wraps up all globals neatly
class Assignemnt_Realtime_Renderer : public Application
{
public:

	Assignemnt_Realtime_Renderer();
	virtual ~Assignemnt_Realtime_Renderer();

protected:


	virtual bool onCreate();
	virtual void Update(float a_deltaTime);
	virtual void Draw();
	virtual void Destroy();

	FBXFile* m_fbxModel;

	unsigned int m_programID; // ID of the main program
	unsigned int m_vertexShader; // vertex show of the main program
	unsigned int m_fragmentShader; // fragment shader for the main program

	unsigned int m_vao; // vertex arrat object for the main program
	unsigned int m_modelBuffer[2]; 

	glm::mat4	m_cameraMatrix; // camera matrix of our scene
	glm::mat4	m_projectionMatrix; // projection matrix of our scene
	glm::mat4	m_modelMatrix; // model matrix for our scene

	// struct that defines how all the lights work
	typedef struct Light {
		bool			m_lightActive;

		glm::vec4		m_lightPos; // position of each light

		glm::vec3		m_lightAmbientColour; // ambient colour of each light
		glm::vec3		m_lightDiffuseColour; // diffuse colour of each light
		glm::vec3		m_lightSpecularColour; // specular colour of each light
		float			m_lightSpecularPower; //specular power of each light
		float			m_attenuationDistance; // attenuation distance of each light

		float			cutoff; // cut off values for spot light
		float			outerCutOff;

	}Light;

#define NUMBER_OF_LIGHTS 10 // number of lights that will be created in our scene

	Light m_lights[NUMBER_OF_LIGHTS];

#pragma region Tessellation components
	bool LoadImageFromFile(std::string a_filePath, unsigned int& a_textureID); // function that allows us to use images with stb
	unsigned int	m_tessProgramID; // program ID for our tesselation shader
	unsigned int	m_tessVertexShader; 
	unsigned int	m_tessControlShader;
	unsigned int	m_tessEvaluationShader;
	unsigned int	m_tessFragmentShader;

	unsigned int m_maxIndices; // max indicies of our plane
	unsigned int maxVerts; // max verts of our plane
	unsigned int m_tessVao; // vertex array object for the tesselation scene
	unsigned int m_tessVbo; // vertex buffer object for the tesselation scene
	unsigned int m_tessIbo; // Index buffer object for the tesselation scene

	//Texture Handles for OpenGL
	unsigned int m_tessDiffuseTex;
	unsigned int m_tessNormalTex;
	unsigned int m_tessSpecularTex;
	unsigned int m_tessOcclusionTex;
	unsigned int m_tessDisplacementTex;

	SimpleVertex* m_tessVertices;

	bool m_bRenderWireFrame;
	int m_innerTessEdge;
	int m_outerTessEdge;
	float m_tessDisplacementScale;
#pragma endregion

#pragma region Shadow componenets
	glm::mat4		m_shadowProjectionViewMatrix; // projectionview that is used to render the shodow map scene

	unsigned int    m_shadowVertexShader;
	unsigned int	m_shadowFragmentShader;
	unsigned int	m_shadowProgramID; // Program Id for the shadow map scene that renders the scene from the camera POV
	unsigned int	m_shadowProgramID2; // Program Id for the shadow map scene that renders the scene from the lights POV
	unsigned int	m_shadowVao; // vertex array object for the shadow map scene
	unsigned int	m_shadowVbo; // vertex buffer object for the shadow map scene
	unsigned int	m_shadowIbo; // Index buffer object for the shadow map scene

	//frame buffer variables
	unsigned int m_FBO;
	unsigned int m_FBO_texture;
	unsigned int m_FBO_depth_texture;
	unsigned int m_FBO_Linear_Depth;

	void SetupFrameBuffer(unsigned int& a_fbo, unsigned int a_fboWidth, unsigned int a_fboHeight, unsigned int& a_renderTexture, unsigned int& a_depthTexture);
	void DrawScene(unsigned int a_program, unsigned int a_vao);

	bool	m_shadowMapOn;
	float   m_waveHeight;
	float   m_waveSpeed;

#pragma endregion
};

#endif // __Assignemnt_Realtime_Renderer_H_