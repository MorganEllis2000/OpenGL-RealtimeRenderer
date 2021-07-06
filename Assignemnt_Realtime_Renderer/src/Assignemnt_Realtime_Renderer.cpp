#include "Assignemnt_Realtime_Renderer.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <iostream>
#include <imgui.h>

#include "Application_Log.h"

#include <FBXFile.h>
#include <stb_image.h>

#include <iostream>
#include <imgui.h>

#include "Application_Log.h"

#define SHADOW_BUFFER_SIZE 1024

Assignemnt_Realtime_Renderer::Assignemnt_Realtime_Renderer()
{
	m_shadowMapOn = false; // shadow maps are not rendered on start
	m_waveSpeed = 1.f; // how quickly the wave moves up and down
	m_waveHeight = 0.6f; // the max height of the wave
}

Assignemnt_Realtime_Renderer::~Assignemnt_Realtime_Renderer()
{

}

/// <summary>
/// returns a random number between two values
/// </summary>
/// <param name="a_min"></param>
/// <param name="a_max"></param>
/// <returns></returns>
float randInRange(float a_min, float a_max)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float range = a_max - a_min;
	return (random * range) + a_min;
}


/// <summary>
/// converts a number into radians, used when calculating the inner and outer values of the spotlight
/// </summary>
/// <param name="angle"></param>
/// <returns></returns>
float ConvertToRadian(float angle)
{
	return (glm::pi<float>() * angle) / 180;
}

bool Assignemnt_Realtime_Renderer::onCreate()
{
	// initialise the Gizmos helper class
	Gizmos::create();

#pragma region load model
	// load in our fbx file
	m_fbxModel = new FBXFile();
	m_fbxModel->load("./models/ruinedtank/tank.fbx", FBXFile::UNITS_DECIMETER);
#pragma endregion

#pragma region Shadow maps shaders
	//Load the shaders for this program
	m_shadowVertexShader = Utility::loadShader("./shaders/shadowVertex.glsl", GL_VERTEX_SHADER);
	m_shadowFragmentShader = Utility::loadShader("./shaders/shadowFragment.glsl", GL_FRAGMENT_SHADER);
	//Define the input and output varialbes in the shaders
	//Note: these names are taken from the glsl files -- added in inputs for UV coordinates
	const char* shadowszInputs[] = { "Position", "Colour", "Normal","Tex1" };
	const char* shadowszOutputs[] = { "FragColor" };
	//bind the shaders to create our shader program
	m_shadowProgramID = Utility::createProgram(
		m_shadowVertexShader,
		0,
		0,
		0,
		m_shadowFragmentShader,
		4, shadowszInputs, 1, shadowszOutputs);

	glDeleteShader(m_shadowVertexShader);
	glDeleteShader(m_shadowFragmentShader);

	//Load the shaders for rendering the shadows
	m_shadowVertexShader = Utility::loadShader("./shaders/vertex_shadow.glsl", GL_VERTEX_SHADER);
	m_shadowFragmentShader = Utility::loadShader("./shaders/fragment_shadow.glsl", GL_FRAGMENT_SHADER);
	//Define the input and output varialbes in the shaders
	//Note: these names are taken from the glsl files -- added in inputs for UV coordinates
	const char* szInputs2[] = { "Position", "Colour", "Normal","Tex1" };
	const char* szOutputs2[] = { "FragDepth" };
	//bind the shaders to create our shader program
	m_shadowProgramID2 = Utility::createProgram(
		m_shadowVertexShader,
		0,
		0,
		0,
		m_shadowFragmentShader,
		4, szInputs2, 1, szOutputs2);

	glDeleteShader(m_shadowVertexShader);
	glDeleteShader(m_shadowFragmentShader);

	//Generate our OpenGL Vertex and Index Buffers for rendering our FBX Model Data
	// OPENGL: genorate the VBO, IBO and VAO
	glGenBuffers(1, &m_shadowVbo);
	glGenBuffers(1, &m_shadowIbo);
	glGenVertexArrays(1, &m_shadowVao);

	// OPENGL: Bind  VAO, and then bind the VBO and IBO to the VAO
	glBindVertexArray(m_shadowVao);
	glBindBuffer(GL_ARRAY_BUFFER, m_shadowVbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_shadowIbo);

	//There is no need to populate the vbo & ibo buffers with any data at this stage
	//this can be done when rendering each mesh component of the FBX model

	// enable the attribute locations that will be used on our shaders
	glEnableVertexAttribArray(0); //Pos
	glEnableVertexAttribArray(1); //Col
	glEnableVertexAttribArray(2); //Norm
	glEnableVertexAttribArray(3); //Tex1


								  // tell our shaders where the information within our buffers lie
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), ((char*)0) + FBXVertex::PositionOffset);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, sizeof(FBXVertex), ((char*)0) + FBXVertex::ColourOffset);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(FBXVertex), ((char*)0) + FBXVertex::NormalOffset);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_TRUE, sizeof(FBXVertex), ((char*)0) + FBXVertex::TexCoord1Offset);

	// finally, where done describing our mesh to the shader
	// we can describe the next mesh
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//\===========================================================================================================
	//\Set up the frame buffer for rendering our shadows into
	m_FBO_texture = -1;
	SetupFrameBuffer(m_FBO, SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE, m_FBO_texture, m_FBO_depth_texture);
	//\===========================================================================================================
	//create a texture to hold the linear depth buffer samples
	//texture for linear depth buffer visualisation
	glGenTextures(1, &m_FBO_Linear_Depth);

	// bind the texture for editing
	glBindTexture(GL_TEXTURE_2D, m_FBO_Linear_Depth);

	// create the texture. Note, the width and height are the dimensions of the screen and the final
	// data pointer is 0 as we aren't including any initial data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_windowWidth, m_windowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	// set the filtering if we intend to sample within a shader
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	//\===========================================================================================================

#pragma endregion

#pragma region load standard shader and vertex array object
	//load in and bind link our shaders
	m_vertexShader = Utility::loadShader("./shaders/vertex.glsl", GL_VERTEX_SHADER);
	m_fragmentShader = Utility::loadShader("./shaders/fragment.glsl", GL_FRAGMENT_SHADER);

	//define inputs and outputs for our shaders
	const char* szInputs[] = { "Position", "Colour", "Normal", "TexCoord" };
	const char* szOutputs[] = { "fragColour" };
	//link our shaders to create shader program
	m_programID = Utility::createProgram(m_vertexShader, 0, 0, 0, m_fragmentShader, 4, szInputs, 1, szOutputs);

	// Generate our vertex array object
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(2, m_modelBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_modelBuffer[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_modelBuffer[1]);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), ((char*)0) + FBXVertex::PositionOffset);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), ((char*)0) + FBXVertex::ColourOffset);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(FBXVertex), ((char*)0) + FBXVertex::NormalOffset);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_TRUE, sizeof(FBXVertex), ((char*)0) + FBXVertex::TexCoord1Offset);


	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#pragma endregion

#pragma region Tessellation shaders for tessellated plane
	//\================================================================================
	//\ Create a flat grid of vertex data that contains position, UV and normal data
	//\================================================================================
	unsigned int width = 45; unsigned int height = 45;
	maxVerts = width * height;
	m_tessVertices = new SimpleVertex[maxVerts];
	glm::vec4 currentPosition = glm::vec4(-(width * 0.5), 0.f, -(height * 0.5f), 1.f);
	glm::vec2 currentUV = glm::vec2(0.f, 0.f);
	float uvXIncrement = 1.f / width;
	float uvYIncrement = 1.f / height;
	for (unsigned int i = 0; i < maxVerts; ++i)
	{
		m_tessVertices[i].position = currentPosition;
		m_tessVertices[i].normal = glm::vec4(0.f, 0.f, 0.f, 0.f);
		m_tessVertices[i].tangent = glm::vec4(0.f, 0.f, 0.f, 0.f);
		m_tessVertices[i].texCoord = currentUV;

		currentPosition.x += 1.f;
		currentUV.x += uvXIncrement;
		if (i > 0 && i % width == (width - 1))
		{
			currentPosition.x = -((float)(width) * 0.5f);
			currentPosition.z += 1.f;
			currentUV.x = 0.f;
			currentUV.y += uvYIncrement;
		}

	}
	//\===============================================================================
	//\Populate the index buffer with the indices to draw the mesh
	//\===============================================================================
	m_maxIndices = (width - 1) * (height - 1) * 6; //for every square we need 6 indicies to render two triangles
	unsigned int* indexBuffer = new unsigned int[m_maxIndices];
	for (unsigned int i = 0, j = 0; i < m_maxIndices; i += 6, j++)
	{
		if (j > 0 && j % width == (width - 1))
		{
			i -= 6;
			continue;
		}
		indexBuffer[i] = j;
		indexBuffer[i + 1] = j + width;
		indexBuffer[i + 2] = j + 1;

		indexBuffer[i + 3] = j + width + 1;
		indexBuffer[i + 4] = j + 1;
		indexBuffer[i + 5] = j + width;

	}

	//Calculate Normals and Tangents for this surface
	for (unsigned int i = 0; i < m_maxIndices; i += 3)
	{
		glm::vec3 v1 = m_tessVertices[indexBuffer[i + 2]].position.xyz - m_tessVertices[indexBuffer[i]].position.xyz;
		glm::vec3 v2 = m_tessVertices[indexBuffer[i + 1]].position.xyz - m_tessVertices[indexBuffer[i]].position.xyz;

		glm::vec4 normal = glm::vec4(glm::cross(v2, v1), 0.f);

		m_tessVertices[indexBuffer[i]].normal += normal;
		m_tessVertices[indexBuffer[i + 1]].normal += normal;
		m_tessVertices[indexBuffer[i + 2]].normal += normal;

		//This is to calculate the tangent to the normal to be used in lighting 
		//and use of the normal map.
		//The maths used here is taken from Mathematics for 3D Games Programming and Computer Graphics by Eric Lengyel
		glm::vec2 uv0 = m_tessVertices[indexBuffer[i]].texCoord;
		glm::vec2 uv1 = m_tessVertices[indexBuffer[i + 1]].texCoord;
		glm::vec2 uv2 = m_tessVertices[indexBuffer[i + 2]].texCoord;

		glm::vec2 uvVec1 = uv2 - uv0;
		glm::vec2 uvVec2 = uv1 - uv0;

		float coefficient = 1.0f / uvVec1.x * uvVec2.y - uvVec1.y * uvVec2.x;

		glm::vec4 tangent;
		tangent[0] = coefficient * (v2[0] * uvVec2[1] + v1[0] * -uvVec1[1]);
		tangent[1] = coefficient * (v2[1] * uvVec2[1] + v1[1] * -uvVec1[1]);
		tangent[2] = coefficient * (v2[2] * uvVec2[1] + v1[2] * -uvVec1[1]);
		tangent[3] = 0.f;

		m_tessVertices[indexBuffer[i]].tangent += tangent;
		m_tessVertices[indexBuffer[i + 1]].tangent += tangent;
		m_tessVertices[indexBuffer[i + 2]].tangent += tangent;

	}
	for (unsigned int i = 0; i < maxVerts; ++i)
	{
		m_tessVertices[i].normal = glm::normalize(m_tessVertices[i].normal);
		m_tessVertices[i].tangent = glm::normalize(m_tessVertices[i].tangent);
	}
	//Load the shaders for this program
	m_tessVertexShader = Utility::loadShader("./shaders/tessVertex.glsl", GL_VERTEX_SHADER);
	m_tessFragmentShader = Utility::loadShader("./shaders/tessFragment.glsl", GL_FRAGMENT_SHADER);
	m_tessControlShader = Utility::loadShader("./shaders/tessControl.glsl", GL_TESS_CONTROL_SHADER);
	m_tessEvaluationShader = Utility::loadShader("./shaders/tessEval.glsl", GL_TESS_EVALUATION_SHADER);
	//Define the input and output varialbes in the shaders
	//Note: these names are taken from the glsl files -- added in inputs for UV coordinates
	const char* tessszInputs[] = { "Position", "Normal", "Tangent","UV" };
	const char* tessszOutputs[] = { "FragColor" };
	//bind the shaders to create our shader program
	m_tessProgramID = Utility::createProgram(
		m_tessVertexShader,
		m_tessControlShader,
		m_tessEvaluationShader,
		0,
		m_tessFragmentShader,
		4, tessszInputs, 1, tessszOutputs);


	//Generate and bind our Vertex Array Object
	glGenVertexArrays(1, &m_tessVao);
	glBindVertexArray(m_tessVao);

	//Generate our Vertex Buffer Object
	glGenBuffers(1, &m_tessVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_tessVbo);
	glBufferData(GL_ARRAY_BUFFER, maxVerts * sizeof(SimpleVertex), m_tessVertices, GL_STREAM_DRAW);
	//Define our vertex attribute data for this buffer we have to do this for our vertex input data
	//Position, Normal & Colour this attribute order is taken from our szInputs string array order
	glEnableVertexAttribArray(0); //position
	glEnableVertexAttribArray(1); //normal
	glEnableVertexAttribArray(2); //tangent
	glEnableVertexAttribArray(3); //texCoords
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), ((char*)0) + SimpleVertex::PositionOffset);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, sizeof(SimpleVertex), ((char*)0) + SimpleVertex::NormalOffset);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(SimpleVertex), ((char*)0) + SimpleVertex::TangentOffset);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_TRUE, sizeof(SimpleVertex), ((char*)0) + SimpleVertex::TexCoordOffset);

	//generate and bind an index buffer
	glGenBuffers(1, &m_tessIbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_tessIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_maxIndices * sizeof(unsigned int), indexBuffer, GL_STATIC_DRAW);


	glBindVertexArray(0);
	//unbind our current vertex Buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//unbind our current index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	glPatchParameteri(GL_PATCH_VERTICES, 3);

	m_bRenderWireFrame = false;
	m_tessDisplacementScale = 0.f;
	m_innerTessEdge = 1;
	m_outerTessEdge = 1;
	//load in our Texture
	LoadImageFromFile("./images/Water/Water_001_COLOR.jpg", m_tessDiffuseTex);
	LoadImageFromFile("./images/Water/Water_001_NORM.jpg", m_tessNormalTex);
	LoadImageFromFile("./images/Water/Water_001_SPEC.jpg", m_tessSpecularTex);
	LoadImageFromFile("./images/Water/Water_001_DISP.png", m_tessDisplacementTex);
	LoadImageFromFile("./images/Water/Water_001_OCC.jpg", m_tessOcclusionTex);
#pragma endregion
	// create a world-space matrix for a camera
	m_cameraMatrix = glm::inverse(glm::lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)));

	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f, m_windowWidth / (float)m_windowHeight, 0.1f, 1000.0f);

	m_modelMatrix = glm::mat4();

#pragma region Setting up lights
	// set up one light to act as the sun
	m_lights[0].m_lightActive = true;
	m_lights[0].m_lightPos = glm::vec4(20.f, 15.f, 0.f, 1.f); // set the starting pos of the light
	m_lights[0].m_lightAmbientColour = glm::vec3(0.3, 0.3f, 0.3f); // set the starting ambient colour of the light
	m_lights[0].m_lightDiffuseColour = glm::vec3(1.f, 0.85f, 0.05f); // set the starting diffuse colour of the light
	m_lights[0].m_lightSpecularColour = glm::vec3(1.f, 1.f, 1.f); // set the starting specular colour of the light
	m_lights[0].m_lightSpecularPower = 1;// set the starting specular power
	m_lights[0].m_attenuationDistance = 10000.f; // a large attenuation value to make this a directional light

	for (int i = 1; i < NUMBER_OF_LIGHTS; ++i)
	{
		m_lights[i].m_lightActive = true;

		// set up 4 point lights
		if (i < 5)
		{
			m_lights[i].m_lightPos = glm::vec4(randInRange(-10.f, 10.f), randInRange(0.f, 8.f), randInRange(-7.f, 7.f), 0.f);
			m_lights[i].m_lightAmbientColour = glm::vec3(0.3, 0.3f, 0.3f);
			m_lights[i].m_lightDiffuseColour = glm::vec3(1.f, 0.85f, 0.05f);
			m_lights[i].m_lightSpecularColour = glm::vec3(1.f, 1.f, 1.f);
			m_lights[i].m_lightSpecularPower = 1;
			m_lights[i].m_attenuationDistance = randInRange(1.f, 100.f);
		}

		// set up 5 spot lights
		if (i > 4)
		{
			m_lights[i].m_lightPos = glm::vec4(randInRange(-10.f, 10.f), randInRange(0.f, 8.f), randInRange(-7.f, 7.f), 1.f);
			m_lights[i].m_lightAmbientColour = glm::vec3(0.3, 0.3f, 0.3f);
			m_lights[i].m_lightDiffuseColour = glm::vec3(1.f, 0.85f, 0.05f);
			m_lights[i].m_lightSpecularColour = glm::vec3(1.f, 1.f, 1.f);
			m_lights[i].m_lightSpecularPower = 1;
			m_lights[i].m_attenuationDistance = randInRange(1.f, 100.f);
			m_lights[i].cutoff = cosf(ConvertToRadian(360));
			m_lights[i].outerCutOff = cosf(ConvertToRadian(360));
		}
	}
#pragma endregion
	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	return true;
}

void Assignemnt_Realtime_Renderer::Update(float a_deltaTime)
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement(m_cameraMatrix, a_deltaTime, 10);

	// clear all gizmos from last frame
	Gizmos::clear();

	// add an identity matrix gizmo
	Gizmos::addTransform(glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));


	if (m_bRenderWireFrame)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	float s = sinf(a_deltaTime * 0.2);
	float c = cosf(a_deltaTime * 0.2);
	glm::vec4 temp = m_lights[0].m_lightPos;
	m_lights[0].m_lightPos.x = temp.x * c - temp.z * s;
	m_lights[0].m_lightPos.z = temp.x * s + temp.z * c;
	m_lights[0].m_lightPos.w = 0;
	m_lights[0].m_lightPos = glm::normalize(m_lights[0].m_lightPos) * 25.f;
	if (m_lights[0].m_lightActive == true)
	{
		Gizmos::addBox(m_lights[0].m_lightPos.xyz, glm::vec3(0.2f, 0.2f, 0.2f), true, glm::vec4(m_lights[0].m_lightDiffuseColour, 1.f));
	}

	glm::vec4 vertexPos = m_tessVertices[0].position;
	m_tessVertices[0].position.y = 10.f;
	m_tessVertices[0].position.w = 0;
	m_tessVertices[0].position = glm::normalize(m_tessVertices[0].position);
	Gizmos::addBox(m_tessVertices[0].position.xyz, glm::vec3(0.05f, 0.05f, 0.05f), true, glm::vec4(1.f, 0.f, 0.f, 1.f));


	//Update the shadow projection view matrix 
	glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(m_lights[0].m_lightPos.xyz),
		glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 depthProjectionMatrix = glm::ortho<float>(-20, 20, -20, 20, -100, 100);

	m_shadowProjectionViewMatrix = depthProjectionMatrix * depthViewMatrix;

	for (int i = 1; i < NUMBER_OF_LIGHTS; ++i)
	{
		if (m_lights[i].m_lightActive == true)
		{
			Gizmos::addBox(m_lights[i].m_lightPos.xyz, glm::vec3(0.2f, 0.2f, 0.2f), true, glm::vec4(m_lights[i].m_lightDiffuseColour, 1.f));
		}
	}

	// for each mesh in the model file update child nodes
	for (int i = 0; i < m_fbxModel->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = m_fbxModel->getMeshByIndex(i);
		mesh->updateGlobalTransform();
	}

	static bool show_demo_window = true;
	//ImGui::ShowDemoWindow(&show_demo_window);
	Application_Log* log = Application_Log::Get();
	if (log != nullptr && show_demo_window)
	{
		log->showLog(&show_demo_window);
	}
	//show application log window
	if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(m_window, GLFW_KEY_L) == GLFW_PRESS)
	{
		show_demo_window = !show_demo_window;
	}
	// quit our application when escape is pressed
	if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();

	//set next ImGui window size and position
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 window_size = ImVec2(400.f, 600.f);
	ImVec2 window_pos = ImVec2(io.DisplaySize.x * 0.01f, io.DisplaySize.y * 0.05f);
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
	ImGui::Begin("Light Properties"); {

		static int sceneLight = 0;
		ImGui::SliderInt("Scene Light", &sceneLight, 0, NUMBER_OF_LIGHTS - 1);
		ImGui::Checkbox("Light Active", &m_lights[sceneLight].m_lightActive);

		// Codeblock to seperate ImGui window from rest of code
		ImGui::ColorEdit3("Light Ambient Colour", glm::value_ptr(m_lights[sceneLight].m_lightAmbientColour)); // allows the user to change the lights ambient value
		ImGui::ColorEdit3("Light Diffuse Colour", glm::value_ptr(m_lights[sceneLight].m_lightDiffuseColour)); // allows the user to change the lights diffuse value
		ImGui::ColorEdit3("Light Specular Colour", glm::value_ptr(m_lights[sceneLight].m_lightSpecularColour)); // allows the user to change the lights specular value
		ImGui::DragFloat("Specular Power", &m_lights[sceneLight].m_lightSpecularPower, 1.f, 1.f, 128.f, "%.0f", 1.f); // allows the user to change the lights specular power value

		if (m_lights[sceneLight].m_lightActive == false)
		{
			m_lights[sceneLight].m_lightAmbientColour = glm::vec3(0, 0, 0);
			m_lights[sceneLight].m_lightDiffuseColour = glm::vec3(0, 0, 0);
			m_lights[sceneLight].m_lightSpecularColour = glm::vec3(0, 0, 0);
		}

		// allows us to change the attenuation distance as long as it is not our directional light and the positions of the lights
		if (sceneLight > 0)
		{
			ImGui::DragFloat("Attenuation Distance", &m_lights[sceneLight].m_attenuationDistance, 1.f, 1.f, 100.f, "%.0f", 1.f);
			ImGui::InputFloat("Light X Pos", &m_lights[sceneLight].m_lightPos.x, 1.f, 1.f, "%.3f", 0);
			ImGui::InputFloat("Light Y Pos", &m_lights[sceneLight].m_lightPos.y, 1.f, 1.f, "%.3f", 0);
			ImGui::InputFloat("Light Z Pos", &m_lights[sceneLight].m_lightPos.z, 1.f, 1.f, "%.3f", 0);
		}

		// if the light is a spotlight than the cutoff distances can be changed
		if (sceneLight > 4)
		{
			ImGui::DragFloat("Cutoff", &m_lights[sceneLight].cutoff, 1.f, 1.f, 180.f, "%.0f", 1.f);
			ImGui::DragFloat("Outer Cutoff", &m_lights[sceneLight].outerCutOff, 1.f, 1.f, 180.f, "%.0f", 1.f);
		}

		ImGui::Text("Shader Control Uniforms");
		ImGui::Checkbox("RenderWireFrame", &m_bRenderWireFrame); // render the wireframe of the scene
		ImGui::SliderInt("Inner Tessellation Value", &m_innerTessEdge, 0, 5); // allows us to change the inner value of the tess shader
		ImGui::SliderInt("Outer Tessellation Value", &m_outerTessEdge, 0, 5); // allows us to change the outer value of the tess shader
		ImGui::DragFloat("Wave Height", &m_waveHeight, 0.01f, 0.01f, 10.f, "%.01f", 1.f); // allows us to change the max height of the wave
		ImGui::DragFloat("Wave Speed", &m_waveSpeed, 0.01f, 0.01f, 10.f, "%.01f", 1.f); 

		ImGui::SetNextWindowPos(ImVec2(m_windowWidth - m_windowWidth * 0.3, m_windowHeight - m_windowHeight * 0.4));
		ImGui::SetNextWindowSize(ImVec2(m_windowWidth * 0.3, m_windowHeight * 0.4));
		ImGui::Checkbox("Shadows On", &m_shadowMapOn); // renders the shadow scene
		ImGui::BeginTabBar("FrameBuffer Textures");

		if (ImGui::BeginTabItem("Depth Buffer"))
		{
			ImTextureID texID = (void*)(intptr_t)m_FBO_depth_texture;
			ImGui::Image(texID, ImVec2(m_windowWidth * 0.25, m_windowHeight * 0.25), ImVec2(0, 1), ImVec2(1, 0));
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}
	ImGui::End();
}

void Assignemnt_Realtime_Renderer::Draw()
{
	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//\===============================================================================
	//\ Draw the scene from the POV of the light
	//\===============================================================================
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE);
	DrawScene(m_shadowProgramID2, m_vao);

	if (m_shadowMapOn)
	{
		//\===============================================================================
		//\ Draw the scene from the regular POV of the camera
		//\===============================================================================
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, m_windowWidth, m_windowHeight);
		DrawScene(m_shadowProgramID, m_vao);
	}


	//\===============================================================================
	//\ Draw the scene from the regular POV of the camera
	//\===============================================================================
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_windowWidth, m_windowHeight);
	DrawScene(m_programID, m_vao);

	// get the view matrix from the world-space camera matrix
	glm::mat4 viewMatrix = glm::inverse(m_cameraMatrix);
	// draw the gizmos from this frame
	Gizmos::draw(viewMatrix, m_projectionMatrix);


#pragma endregion
#pragma region tessellation bit

	//bing our shader program
	glUseProgram(m_tessProgramID);
	//bind our vertex array object
	glBindVertexArray(m_tessVao);

	int timeUniformLocation = glGetUniformLocation(m_tessProgramID, "u_Time");
	glUniform1f(timeUniformLocation, Utility::getTotalTime());

	unsigned int waveHeightUniformLocation = glGetUniformLocation(m_tessProgramID, "waveHeight");
	glUniform1f(waveHeightUniformLocation, m_waveHeight);

	unsigned int waveSpeedUniformLocation = glGetUniformLocation(m_tessProgramID, "waveSpeed");
	glUniform1f(waveSpeedUniformLocation, m_waveSpeed);

	//get our shaders uniform location for our projectionViewMatrix and then use glUniformMatrix4fv to fill it with the correct data
	unsigned int projectionViewUniform = glGetUniformLocation(m_tessProgramID, "ProjectionView");
	glUniformMatrix4fv(projectionViewUniform, 1, false, glm::value_ptr(m_projectionMatrix * viewMatrix));

	unsigned int ViewMatrixUniform = glGetUniformLocation(m_tessProgramID, "ViewMatrix");
	glUniformMatrix4fv(ViewMatrixUniform, 1, false, glm::value_ptr(viewMatrix));

	//get our shaders uniform location for our projectionViewMatrix and then use glUniformMatrix4fv to fill it with the correct data
	glm::mat4 m4Model = glm::mat4(1.f);
	unsigned int ModelMatrixUniform = glGetUniformLocation(m_tessProgramID, "Model");
	glUniformMatrix4fv(ModelMatrixUniform, 1, false, glm::value_ptr(m4Model));

	unsigned int cameraPosUniform = glGetUniformLocation(m_tessProgramID, "cameraPosition");
	glUniform4fv(cameraPosUniform, 1, glm::value_ptr(m_cameraMatrix[3]));

	//pass the directional light direction to our fragment shader
	glm::vec4 lightDir = -m_lights[0].m_lightPos;
	lightDir.w = 0.f;
	lightDir = glm::normalize(lightDir);
	unsigned int lightDirUniform = glGetUniformLocation(m_tessProgramID, "lightDirection");
	glUniform4fv(lightDirUniform, 1, glm::value_ptr(lightDir));

	//Tessellation location edge uniforms
	unsigned int tesUniformID = glGetUniformLocation(m_tessProgramID, "innerEdge");
	glUniform1i(tesUniformID, m_innerTessEdge);

	tesUniformID = glGetUniformLocation(m_tessProgramID, "outerEdge");
	glUniform1i(tesUniformID, m_outerTessEdge);

	tesUniformID = glGetUniformLocation(m_tessProgramID, "displacementScale");
	glUniform1f(tesUniformID, m_tessDisplacementScale);
	//bind our textureLocation variables from the shaders and set it's value to 0 as the active texture is texture 0
	unsigned int texUniformID = glGetUniformLocation(m_tessProgramID, "diffuseTexture");
	glUniform1i(texUniformID, 0);
	//set our active texture, and bind our loaded texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_tessDiffuseTex);

	texUniformID = glGetUniformLocation(m_tessProgramID, "normalTexture");
	glUniform1i(texUniformID, 1);
	//set our active texture, and bind our loaded texture
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_tessNormalTex);

	texUniformID = glGetUniformLocation(m_tessProgramID, "specularTexture");
	glUniform1i(texUniformID, 2);
	//set our active texture, and bind our loaded texture
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_tessSpecularTex);

	texUniformID = glGetUniformLocation(m_tessProgramID, "displacementTexture");
	glUniform1i(texUniformID, 3);
	//set our active texture, and bind our loaded texture
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_tessDisplacementTex);

	texUniformID = glGetUniformLocation(m_tessProgramID, "occlusionTexture");
	glUniform1i(texUniformID, 4);
	//set our active texture, and bind our loaded texture
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, m_tessOcclusionTex);

	for (int i = 0; i < NUMBER_OF_LIGHTS; ++i)
	{
		char strBuff[32];
		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightPosition[%i]", i);
		glm::vec4 lightPos = m_lights[i].m_lightPos;
		int lightPosUniformLoc = glGetUniformLocation(m_tessProgramID, strBuff);
		glUniform4fv(lightPosUniformLoc, 1, glm::value_ptr(lightPos));

		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightAttenuation[%i]", i);
		int lightAttenuationUniformLoc = glGetUniformLocation(m_tessProgramID, strBuff);
		glUniform1f(lightAttenuationUniformLoc, m_lights[i].m_attenuationDistance);

		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightAmbient[%i]", i);
		int lightAmbientUniformLoc = glGetUniformLocation(m_tessProgramID, strBuff);
		glUniform4fv(lightAmbientUniformLoc, 1, glm::value_ptr(glm::vec4(m_lights[i].m_lightAmbientColour, 1.f)));

		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightDiffuse[%i]", i);
		int lightColourUniformLoc = glGetUniformLocation(m_tessProgramID, strBuff);
		glUniform4fv(lightColourUniformLoc, 1, glm::value_ptr(glm::vec4(m_lights[i].m_lightDiffuseColour, 1.f)));

		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightSpecular[%i]", i);
		int lightSpecularUniformLoc = glGetUniformLocation(m_tessProgramID, strBuff);
		glUniform4fv(lightSpecularUniformLoc, 1, glm::value_ptr(glm::vec4(m_lights[i].m_lightAmbientColour, m_lights[i].m_lightSpecularPower)));
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//depending on the state call draw with glDrawElements to draw our buffer
	//glDrawElements uses the index array in our index buffer to draw the vertices in our vertex buffer
	glDrawElements(GL_PATCHES, m_maxIndices, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glUseProgram(0);
#pragma endregion
}

void Assignemnt_Realtime_Renderer::DrawScene(unsigned int a_programID, unsigned int a_vao)
{
#pragma region draw standard vertex buffer content
	//draw our model using our shaders
	glUseProgram(a_programID);
	glm::mat4 viewMatrix = glm::inverse(m_cameraMatrix);
	glBindVertexArray(a_vao);


	unsigned int shadowProjectionViewUniform = glGetUniformLocation(a_programID, "ShadowProjectionView");
	glUniformMatrix4fv(shadowProjectionViewUniform, 1, false, glm::value_ptr(m_shadowProjectionViewMatrix));

	unsigned int projectionViewUniformLocation = glGetUniformLocation(a_programID, "ProjectionView");
	glUniformMatrix4fv(projectionViewUniformLocation, 1, false, glm::value_ptr(m_projectionMatrix * viewMatrix));

	//pass throught the view matrix
	unsigned int viewMatrixUniform = glGetUniformLocation(a_programID, "ViewMatrix");
	glUniformMatrix4fv(viewMatrixUniform, 1, false, glm::value_ptr(viewMatrix));

	int cameraPositionUniformLocation = glGetUniformLocation(a_programID, "CameraPosition");
	glUniform4fv(cameraPositionUniformLocation, 1, glm::value_ptr(m_cameraMatrix[3]));

	//pass the directional light direction to our fragment shader
	glm::vec4 lightDir = -m_lights[0].m_lightPos;
	lightDir.w = 0.f;
	lightDir = glm::normalize(lightDir);
	unsigned int lightDirUniform = glGetUniformLocation(a_programID, "lightDirection");
	glUniform4fv(lightDirUniform, 1, glm::value_ptr(lightDir));

	//Set the shadow texture
	unsigned int shadowTexUniformID = glGetUniformLocation(a_programID, "ShadowTexture");
	glUniform1i(shadowTexUniformID, 1);
	//Set our active texture, and bind our loaded texture
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_FBO_depth_texture);

	for (int i = 0; i < NUMBER_OF_LIGHTS; ++i)
	{
		char strBuff[32];
		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightPosition[%i]", i);
		glm::vec4 lightPos = m_lights[i].m_lightPos;
		int lightPosUniformLoc = glGetUniformLocation(a_programID, strBuff);
		glUniform4fv(lightPosUniformLoc, 1, glm::value_ptr(lightPos));

		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightDiffuse[%i]", i);
		int lightColourUniformLoc = glGetUniformLocation(a_programID, strBuff);
		glUniform4fv(lightColourUniformLoc, 1, glm::value_ptr(glm::vec4(m_lights[i].m_lightDiffuseColour, 1.f)));

		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightAmbient[%i]", i);
		int lightAmbientUniformLoc = glGetUniformLocation(a_programID, strBuff);
		glUniform4fv(lightAmbientUniformLoc, 1, glm::value_ptr(glm::vec4(m_lights[i].m_lightAmbientColour, 1.f)));

		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightSpecular[%i]", i);
		int lightSpecularUniformLoc = glGetUniformLocation(a_programID, strBuff);
		glUniform4fv(lightSpecularUniformLoc, 1, glm::value_ptr(glm::vec4(m_lights[i].m_lightAmbientColour, m_lights[i].m_lightSpecularPower)));

		memset(strBuff, 0, 32);
		sprintf(strBuff, "LightAttenuation[%i]", i);
		int lightAttenuationUniformLoc = glGetUniformLocation(a_programID, strBuff);
		glUniform1f(lightAttenuationUniformLoc, m_lights[i].m_attenuationDistance);

		if (i > 4)
		{
			memset(strBuff, 0, 32);
			sprintf(strBuff, "Cutoff[%i]", i);
			int lightCutoffUniformLoc = glGetUniformLocation(a_programID, strBuff);
			glUniform1f(lightCutoffUniformLoc, cosf(ConvertToRadian(m_lights[i].cutoff)));

			memset(strBuff, 0, 32);
			sprintf(strBuff, "OuterCutoff[%i]", i);
			int lightOuterCutoffUniformLoc = glGetUniformLocation(a_programID, strBuff);
			glUniform1f(lightOuterCutoffUniformLoc, cosf(ConvertToRadian(m_lights[i].outerCutOff)));
		}
	}

	for (int i = 0; i < m_fbxModel->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = m_fbxModel->getMeshByIndex(i);

		// send the Model
		glm::mat4 m4Model = mesh->m_globalTransform;// *m_modelMatrix;
		unsigned int modelUniform = glGetUniformLocation(a_programID, "Model");
		glUniformMatrix4fv(modelUniform, 1, false, glm::value_ptr(m4Model));

		//set each model matrix for mesh 
		int modelMatrixUniformLoc = glGetUniformLocation(a_programID, "ModelMatrix");
		glUniformMatrix4fv(modelMatrixUniformLoc, 1, false, glm::value_ptr(mesh->m_globalTransform));

		int normalMatrixUniformLoc = glGetUniformLocation(a_programID, "NormalMatrix");
		glUniformMatrix4fv(normalMatrixUniformLoc, 1, false, glm::value_ptr(glm::transpose(glm::inverse(mesh->m_globalTransform))));

		int materialAmbientUniformLoc = glGetUniformLocation(a_programID, "matAmbient");
		glUniform4fv(materialAmbientUniformLoc, 1, glm::value_ptr(mesh->m_material->ambient));

		int materialDiffuseUniformLoc = glGetUniformLocation(a_programID, "matDiffuse");
		glUniform4fv(materialDiffuseUniformLoc, 1, glm::value_ptr(mesh->m_material->diffuse));

		int materialSpecularUniformLoc = glGetUniformLocation(a_programID, "matSpecular");
		glUniform4fv(materialSpecularUniformLoc, 1, glm::value_ptr(mesh->m_material->specular));

		if (modelMatrixUniformLoc < 0)
		{
			Application_Log* log = Application_Log::Create();
			if (log != nullptr)
			{
				//log->addLog(LOG_LEVEL::LOG_WARNING, "Warning: Uniform location: ModelLocation not found in model!\n");
			}
		}

		//textures on the model send to shader
		int textUniformLoc = glGetUniformLocation(a_programID, "DiffuseTexture");
		glUniform1i(textUniformLoc, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mesh->m_material->textureIDs[FBXMaterial::DiffuseTexture]);

		glBindBuffer(GL_ARRAY_BUFFER, m_modelBuffer[0]);
		glBufferData(GL_ARRAY_BUFFER, mesh->m_vertices.size() * sizeof(FBXVertex), mesh->m_vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, m_modelBuffer[1]);
		glBufferData(GL_ARRAY_BUFFER, mesh->m_indices.size() * sizeof(unsigned int), mesh->m_indices.data(), GL_STATIC_DRAW);

		glDrawElements(GL_TRIANGLES, mesh->m_indices.size(), GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);
	glUseProgram(0);


}

void Assignemnt_Realtime_Renderer::Destroy()
{
	m_fbxModel->unload();
	delete m_fbxModel;
	Gizmos::destroy();

	//if we generate it destroy it
	glDeleteTextures(1, &m_tessDiffuseTex);
	glDeleteTextures(1, &m_tessNormalTex);
	glDeleteTextures(1, &m_tessSpecularTex);
	glDeleteTextures(1, &m_tessDisplacementTex);

	glDeleteBuffers(1, &m_tessVbo);
	glDeleteBuffers(1, &m_tessIbo);
	glDeleteVertexArrays(1, &m_tessVao);
	glDeleteProgram(m_tessProgramID);
	glDeleteShader(m_tessFragmentShader);
	glDeleteShader(m_tessVertexShader);

	glDeleteBuffers(1, &m_shadowVbo);
	glDeleteBuffers(1, &m_shadowIbo);
	glDeleteVertexArrays(1, &m_shadowVao);
	glDeleteProgram(m_programID);
	glDeleteProgram(m_shadowProgramID);
	Gizmos::destroy();
}

//simple function to load in a GL texture using SOIL
bool Assignemnt_Realtime_Renderer::LoadImageFromFile(std::string a_filePath, unsigned int& a_textureID)
{
	int width = 0, height = 0, channels = 0;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* imageData = stbi_load(a_filePath.c_str(), &width, &height, &channels, 0);
	if (imageData != nullptr)
	{
		a_textureID = 0;
		glGenTextures(1, &a_textureID);

		glBindTexture(GL_TEXTURE_2D, a_textureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		if (channels == 1)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, imageData);
		}
		if (channels == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(imageData);
		Application_Log* log = Application_Log::Get();
		if (log != nullptr)
		{
			log->addLog(LOG_LEVEL::LOG_ERROR, "Successfully loaded texture : %s", a_filePath.c_str());
		}
		return true;
	}
	else
	{
		Application_Log* log = Application_Log::Get();
		if (log != nullptr)
		{
			//log->addLog(LOG_LEVEL::LOG_ERROR, "Failed to load texture : %s", a_filePath.c_str());
		}
	}
	return false;

}

void Assignemnt_Realtime_Renderer::SetupFrameBuffer(unsigned int& a_fbo,
	unsigned int a_targetWidth,
	unsigned int a_targetHeight,
	unsigned int& a_renderTexture,
	unsigned int& a_depthTexture)
{
	//\======================================================================================
	// Create our frame buffer object
	//\=====================================================================================
	// this would be within your onCreate() function
	glGenFramebuffers(1, &a_fbo);

	// bind the framebuffer for editing
	glBindFramebuffer(GL_FRAMEBUFFER, a_fbo);

	if (a_renderTexture != -1)
	{
		// create a texture to be attached to the framebuffer, stored in the derived app class as a member
		glGenTextures(1, &a_renderTexture);

		// bind the texture for editing
		glBindTexture(GL_TEXTURE_2D, a_renderTexture);

		// create the texture. Note, the width and height are the dimensions of the screen and the final
		// data pointer is 0 as we aren't including any initial data
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, a_targetWidth, a_targetHeight,
			0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		// set the filtering if we intend to sample within a shader
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, a_renderTexture, 0);
	}
	// m_FBO_depth would be a member for the derived application class
	glGenTextures(1, &a_depthTexture);
	glBindTexture(GL_TEXTURE_2D, a_depthTexture);

	// note the use of GL_DEPTH_COMPONENT32F and GL_DEPTH_COMPONENT
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, a_targetWidth,
		a_targetHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// attach it as a GL_DEPTH_ATTACHMENT
	// attach the texture to the 0th color attachment of the framebuffer
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, a_depthTexture, 0);

	// Here we tell the framebuffer which color attachments we will be drawing to and how many
	if (a_renderTexture != -1)
	{
		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawBuffers);
	}
	else
	{
		glDrawBuffer(GL_NONE);
	}
	// if Status doesn't equal GL_FRAMEBUFFER_COMPLETE there has been an error when creating it
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		printf("Framebuffer Error!\n");

	// binding 0 to the framebuffer slot unbinds the framebuffer and means future render calls will be sent to 
	// the back buffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

}


