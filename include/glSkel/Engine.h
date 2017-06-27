#pragma once
#include <glSkel/mesh.h>
#include <glSkel/shader.h>

#include <glSkel/BroadcastSystem.h>
#include <glSkel/shader.h>
#include <glSkel/camera.h>
#include <glSkel/mesh.h>
#include <glSkel/lighting.h>

#include "GLFWInputBroadcaster.h"
#include "Chondrus.h"

#define MS_PER_UPDATE 0.0333333333f
#define CAST_RAY_LEN 1000.f

class Engine : public BroadcastSystem::Listener
{
public:
	GLFWwindow* m_pWindow;
	LightingSystem* m_pLightingSystem;

	bool m_bRunPhysics;
	bool m_bShowLights;
	bool m_bShowNormals;
	bool m_bExplode; 

	// Constants
	const int m_iWidth = 1280;
	const int m_iHeight = 800;
	const float m_fStepSize = 1.f / 120.f;

	float m_fDeltaTime;	// Time between current frame and last frame
	float m_fLastTime; // Time of last frame

	Camera  *m_pCamera;
	std::vector<Shader*> m_vpShaders;
	Shader *m_pShaderLighting, *m_pShaderLamps, *m_pShaderNormals, *m_pShaderLines; 
	
	GLint m_iViewLocLightingShader;
	GLint m_iProjLocLightingShader;
	GLint m_iViewPosLocLightingShader;
	GLint m_iShininessLightingShader;

	std::vector<Chondrus*> chonds;

public:
	Engine()
		: m_pWindow(NULL)
		, m_pLightingSystem(NULL)
		, m_bRunPhysics(false)
		, m_bShowLights(true)
		, m_bShowNormals(false)
		, m_bExplode(false)
		, m_fDeltaTime(0.f)
		, m_fLastTime(0.f)
		, m_pCamera(NULL)
		, m_pShaderLighting(NULL)
		, m_pShaderLamps(NULL)
		, m_pShaderNormals(NULL)
		, m_pShaderLines(NULL)
		, m_iViewLocLightingShader(-1)
		, m_iProjLocLightingShader(-1)
		, m_iViewPosLocLightingShader(-1)
		, m_iShininessLightingShader(-1)
	{
	}

	~Engine()
	{
		if (chonds.size())
		{
			for (auto s : chonds)
				delete s;
			chonds.clear();
		}
	}

	void receiveEvent(Object * obj, const int event, void * data)
	{
		if (event == BroadcastSystem::EVENT::KEY_PRESS)
		{
			int key;
			memcpy(&key, data, sizeof(key));

			if (key == GLFW_KEY_L)
				m_bShowLights = !m_bShowLights;
			if (key == GLFW_KEY_N)
				m_bShowNormals = !m_bShowNormals;
			if (key == GLFW_KEY_B)
				m_bExplode = !m_bExplode;
			if (key == GLFW_KEY_R)
				generateModels();
			if (key == GLFW_KEY_SPACE)
				m_bRunPhysics = !m_bRunPhysics;
		}

		if (event == BroadcastSystem::EVENT::MOUSE_UNCLICK)
		{
			int button;
			memcpy(&button, data, sizeof(button));

			if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				glm::vec3 rayFrom = m_pCamera->getPosition();
				glm::vec3 rayTo = rayFrom + m_pCamera->getOrientation()[2] * CAST_RAY_LEN;
				glm::vec3 payload[2] = { rayFrom, rayTo };

				BroadcastSystem::EVENT rayType = button == GLFW_MOUSE_BUTTON_LEFT ? BroadcastSystem::EVENT::GROW_RAY : BroadcastSystem::EVENT::SHRINK_RAY;

				for (auto& s : chonds)
				{
					s->receiveEvent(m_pCamera, rayType, &payload);
				}
			}
		}
	}

	bool init()
	{
		m_pWindow = init_gl_context("Saccharina latissima");

		if (!m_pWindow)
			return false;

		GLFWInputBroadcaster::getInstance().init(m_pWindow);
		GLFWInputBroadcaster::getInstance().attach(this);  // Register self with input broadcaster
		
		init_shaders();
		init_camera();
		init_lighting();
		generateModels();

		return true;
	}

	void mainLoop()
	{
		m_fLastTime = static_cast<float>(glfwGetTime());

		// Main Rendering Loop
		while (!glfwWindowShouldClose(m_pWindow)) {
			// Calculate deltatime of current frame
			float newTime = static_cast<float>(glfwGetTime());
			m_fDeltaTime = newTime - m_fLastTime;
			m_fLastTime = newTime;

			// Poll input events first
			GLFWInputBroadcaster::getInstance().poll();

			update(m_fStepSize);

			render();

			// Flip buffers and render to screen
			glfwSwapBuffers(m_pWindow);
		}
	}

	void update(float dt)
	{
		m_pCamera->update(dt);

		// update soft mesh vertices
		for (auto &s : chonds)
			s->update();
	}

	void render()
	{
		// OpenGL options
		glEnable(GL_DEPTH_TEST);
		glLineWidth(5.f);

		// Background Fill Color
		glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use corresponding shader when setting uniforms/drawing objects
		m_pShaderLighting->Use();
		glUniform3f(m_iViewPosLocLightingShader, m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z);

		m_pLightingSystem->sLight.position = m_pCamera->getPosition();
		m_pLightingSystem->sLight.direction = glm::vec3(m_pCamera->getOrientation()[2]);

		m_pLightingSystem->SetupLighting(*m_pShaderLighting);

		// Create camera transformations
		glm::mat4 view = m_pCamera->getViewMatrix();
		glm::mat4 projection = glm::perspective(
			glm::radians(m_pCamera->getZoom()),
			static_cast<float>(m_iWidth) / static_cast<float>(m_iHeight),
			0.01f,
			1000.0f
			);

		// Pass the matrices to the shader
		glUniformMatrix4fv(m_iViewLocLightingShader, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(m_iProjLocLightingShader, 1, GL_FALSE, glm::value_ptr(projection));

		// Set material properties
		glUniform1f(m_iShininessLightingShader, 32.0f);		

		for (auto& shader : m_vpShaders)
		{
			if (shader->status())
			{
				shader->Use();
				glUniformMatrix4fv(glGetUniformLocation(shader->Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
				glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

				if (shader == m_pShaderLines)
				{
					;
				}
				else if (shader == m_pShaderLamps)
				{
					m_pLightingSystem->Draw(*shader);
				}
				else
				{
					for (auto &s : chonds)
						s->Draw(*shader);
				}
			}
		}

		Shader::Off();
	}

private:
	GLFWwindow* init_gl_context(std::string winName)
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		glfwWindowHint(GLFW_SAMPLES, 16);
		GLFWwindow* mWindow = glfwCreateWindow(m_iWidth, m_iHeight, winName.c_str(), nullptr, nullptr);

		// Check for Valid Context
		if (mWindow == nullptr)
			return nullptr;

		// Create Context and Load OpenGL Functions
		glfwMakeContextCurrent(mWindow);

		// GLFW Options
		glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
		glewExperimental = GL_TRUE;
		glewInit();
		fprintf(stderr, "OpenGL %s\n", glGetString(GL_VERSION));

		// Define the viewport dimensions
		glViewport(0, 0, m_iWidth, m_iHeight);

		return mWindow;
	}

	// Initialize the lighting system
	void init_lighting()
	{
		m_pLightingSystem = new LightingSystem();
		GLFWInputBroadcaster::getInstance().attach(m_pLightingSystem);

		// Directional light
		m_pLightingSystem->addDLight(glm::vec3(0.f, -1.f, 1.f), glm::vec3(0.1f), glm::vec3(1.f), glm::vec3(1.f));
		//m_pLightingSystem->addDLight(glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.1f), glm::vec3(0.25f), glm::vec3(0.5f));

		// Positions of the point lights
		m_pLightingSystem->addPLight(glm::vec3(25.f, 0.f, 25.f));
		m_pLightingSystem->addPLight(glm::vec3(25.f, 0.f, -25.f));
		m_pLightingSystem->addPLight(glm::vec3(-25.f, 0.f, 25.f));
		m_pLightingSystem->addPLight(glm::vec3(-25.f, 0.f, -25.f));

		// Spotlight
		m_pLightingSystem->addSLight();
	}

	void init_camera()
	{
		m_pCamera = new Camera(glm::vec3(0.0f, 25.0f, 50.0f));
		GLFWInputBroadcaster::getInstance().attach(m_pCamera);
	}

	void init_shaders()
	{
		// Build and compile our shader program
		m_pShaderLighting = new Shader(
			"shaders/multiple_lights.vs",
			"shaders/multiple_lights.frag"
			);
		m_pShaderLighting->enable();
		m_vpShaders.push_back(m_pShaderLighting);

		m_pShaderLamps = new Shader(
			"shaders/lamp.vs",
			"shaders/lamp.frag"
			);
		m_pShaderLamps->enable();
		m_vpShaders.push_back(m_pShaderLamps);

		m_pShaderNormals = new Shader(
			"shaders/normals.vs",
			"shaders/normals.frag",
			"shaders/normals.geom"
			);
		m_vpShaders.push_back(m_pShaderNormals);

		m_pShaderLines = new Shader(
			"shaders/line.vs",
			"shaders/line.frag"
			);
		m_pShaderLines->enable();
		m_vpShaders.push_back(m_pShaderLines);
		
		// Get the uniform locations
		m_iViewLocLightingShader = glGetUniformLocation(m_pShaderLighting->Program, "view");
		m_iProjLocLightingShader = glGetUniformLocation(m_pShaderLighting->Program, "projection");
		m_iViewPosLocLightingShader = glGetUniformLocation(m_pShaderLighting->Program, "viewPos"); 
		m_iShininessLightingShader = glGetUniformLocation(m_pShaderLighting->Program, "material.shininess");
	}

	void generateModels()
	{
		Chondrus *chond;
		unsigned int nSlats = 1u;
		float spaceBetween = 7.5f;

		bool isEmpty = chonds.size() == 0;

		if (!isEmpty)
		{
			chonds.clear();
		}

		for (int i = 0u; i < nSlats; ++i)
		{
			if (!isEmpty)
			{
				GLFWInputBroadcaster::getInstance().detach(chonds[i]);
				delete chonds[i];
			}

			glm::vec3 pos;// (-(nSlats * spaceBetween / 2) + i * spaceBetween, 0.f, 0.f);
			glm::mat3 rot;// (glm::angleAxis(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)));

			chond = new Chondrus(100.f, 33.f, 5.f, pos, rot);

			GLFWInputBroadcaster::getInstance().attach(chond);

			chonds.push_back(chond);
		}
	}
};

