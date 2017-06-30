#pragma once
#include <glSkel/mesh.h>

#include <glSkel/BroadcastSystem.h>
#include <glSkel/renderer.h>
#include <glSkel/camera.h>
#include <glSkel/mesh.h>
#include <glSkel/LightingSystem.h>
#include <glSkel/GLSLpreamble.h>

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
	const float m_fAspect = static_cast<float>(m_iWidth) / static_cast<float>(m_iHeight);
	const float m_fStepSize = 1.f / 120.f;

	float m_fDeltaTime;	// Time between current frame and last frame
	float m_fLastTime; // Time of last frame

	Camera  *m_pCamera;

	std::vector<Chondrus*> chonds;

	struct RendererSubmission
	{
		GLenum			primitiveType;
		GLuint			VAO;
		int				vertCount;
		std::string		shaderName;
		GLuint			diffuseTex;
		GLuint			specularTex;
		float			specularExponent;
		glm::mat4		modelToWorldTransform;

		RendererSubmission()
			: primitiveType(GL_NONE)
			, VAO(0)
			, vertCount(0)
			, shaderName("")
			, diffuseTex(0)
			, specularTex(0)
			, specularExponent(0.f)
			, modelToWorldTransform(glm::mat4())
		{}
	};

public:
	Engine()
		: m_pWindow(NULL)
		, m_pLightingSystem(NULL)
		, m_pCamera(NULL)
		, m_bRunPhysics(false)
		, m_bShowLights(true)
		, m_bShowNormals(false)
		, m_bExplode(false)
		, m_fDeltaTime(0.f)
		, m_fLastTime(0.f)
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
		m_pWindow = init_gl_context("Chondrus crispus");

		if (!m_pWindow)
			return false;

		GLFWInputBroadcaster::getInstance().init(m_pWindow);
		GLFWInputBroadcaster::getInstance().attach(this);  // Register self with input broadcaster
		
		Renderer::getInstance(); // this will init the renderer singleton
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

			Renderer::getInstance().RenderFrame(m_iWidth, m_iHeight);

			// Flip buffers and render to screen
			glfwSwapBuffers(m_pWindow);
		}
	}

	void update(float dt)
	{
		m_pCamera->update(dt);

		// Create camera transformations
		glm::mat4 view = m_pCamera->getViewMatrix();
		glm::mat4 projection = glm::perspective(
			glm::radians(m_pCamera->getZoom()),
			m_fAspect,
			m_pCamera->getNearPlane(),
			m_pCamera->getFarPlane()
			);
		glm::mat4 viewProjection = projection * view;

		m_pLightingSystem->update(view);

		// Set viewport for shader uniforms
		glNamedBufferSubData(*Renderer::getInstance().getFrameUBO(), offsetof(FrameUniforms, v4Viewport), sizeof(FrameUniforms::v4Viewport), glm::value_ptr(glm::vec4(0, 0, m_iWidth, m_iHeight)));
		glNamedBufferSubData(*Renderer::getInstance().getFrameUBO(), offsetof(FrameUniforms, m4View), sizeof(FrameUniforms::m4View), glm::value_ptr(view));
		glNamedBufferSubData(*Renderer::getInstance().getFrameUBO(), offsetof(FrameUniforms, m4Projection), sizeof(FrameUniforms::m4Projection), glm::value_ptr(projection));
		glNamedBufferSubData(*Renderer::getInstance().getFrameUBO(), offsetof(FrameUniforms, m4ViewProjection), sizeof(FrameUniforms::m4ViewProjection), glm::value_ptr(viewProjection));
		
		// update soft mesh vertices
		for (auto &s : chonds)
			s->update();
	}

private:
	GLFWwindow* init_gl_context(std::string winName)
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
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

		m_pLightingSystem->addDirectLight()->ambientCoefficient = 0.5f;

		// Positions of the point lights
		m_pLightingSystem->addPointLight(glm::vec4(25.f, 0.f, 25.f, 1.f));
		m_pLightingSystem->addPointLight(glm::vec4(25.f, 0.f, -25.f, 1.f));
		m_pLightingSystem->addPointLight(glm::vec4(-25.f, 0.f, 25.f, 1.f));
		m_pLightingSystem->addPointLight(glm::vec4(-25.f, 0.f, -25.f, 1.f));
				
		m_pLightingSystem->addShaderToUpdate(Renderer::getInstance().getShader("lighting"));
		m_pLightingSystem->addShaderToUpdate(Renderer::getInstance().getShader("lightingWireframe"));
	}

	void init_camera()
	{
		m_pCamera = new Camera(glm::vec3(0.0f, 25.0f, 50.0f), m_vec3DefaultUp, m_iWidth, m_iHeight);
		GLFWInputBroadcaster::getInstance().attach(m_pCamera);
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

