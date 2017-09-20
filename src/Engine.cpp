#include "Engine.h"

#include "LSystem.h"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

LSystem* lsys;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

Engine::Engine()
	: m_pWindow(NULL)
	, m_pLightingSystem(NULL)
	, m_pCamera(NULL)
	, m_pArcball(NULL)
	, m_bRunPhysics(false)
	, m_fDeltaTime(0.f)
	, m_fLastTime(0.f)
{
}

Engine::~Engine()
{
}

void Engine::receiveEvent(Object * obj, const int event, void * data)
{	
	if (event == BroadcastSystem::EVENT::KEY_PRESS)
	{
		int key;
		memcpy(&key, data, sizeof(key));

		if (key == GLFW_KEY_R)
		{
			lsys->setRefreshNeeded();
			//generateModels();
		}
		if (key == GLFW_KEY_SPACE)
			m_bRunPhysics = !m_bRunPhysics;
	}

	if (event == BroadcastSystem::EVENT::KEY_PRESS || event == BroadcastSystem::EVENT::KEY_REPEAT)
	{
		int key;
		memcpy(&key, data, sizeof(key));

		if (key == GLFW_KEY_LEFT)
		{
			lsys->setOrientation(glm::mat3(glm::rotate(glm::mat4(lsys->getOrientation()), glm::radians(-1.f), glm::vec3(0.f, 1.f, 0.f))));
		}
		if (key == GLFW_KEY_RIGHT)
		{
			lsys->setOrientation(glm::mat3(glm::rotate(glm::mat4(lsys->getOrientation()), glm::radians(1.f), glm::vec3(0.f, 1.f, 0.f))));
		}
		if (key == GLFW_KEY_UP)
		{
			lsys->setOrientation(glm::mat3(glm::rotate(glm::mat4(lsys->getOrientation()), glm::radians(-1.f), glm::vec3(1.f, 0.f, 0.f))));
		}
		if (key == GLFW_KEY_DOWN)
		{
			lsys->setOrientation(glm::mat3(glm::rotate(glm::mat4(lsys->getOrientation()), glm::radians(1.f), glm::vec3(1.f, 0.f, 0.f))));
		}
	}

	if (event == BroadcastSystem::EVENT::MOUSE_CLICK)
	{
		int button;
		memcpy(&button, data, sizeof(button));

		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			double xpos, ypos;
			glfwGetCursorPos(m_pWindow, &xpos, &ypos);

			m_pArcball->beginDrag(glm::vec2(xpos, m_iHeight - ypos));
		}
	}

	if (event == BroadcastSystem::EVENT::MOUSE_UNCLICK)
	{
		int button;
		memcpy(&button, data, sizeof(button));
	}

	if (event == BroadcastSystem::EVENT::MOUSE_MOVE)
	{
		if (GLFWInputBroadcaster::getInstance().mouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
		{
			float offsets[2];
			memcpy(&offsets, data, sizeof(float) * 2);

			double xpos, ypos;
			glfwGetCursorPos(m_pWindow, &xpos, &ypos);

			m_pArcball->drag(glm::vec2(xpos, m_iHeight - ypos));
		}
	}
}

bool Engine::init()
{
	m_pWindow = init_gl_context("Chondrus crispus");

	if (!m_pWindow)
		return false;

	GLFWInputBroadcaster::getInstance().init(m_pWindow);
	GLFWInputBroadcaster::getInstance().attach(this);  // Register self with input broadcaster

	Renderer::getInstance().init(); // this will init the renderer singleton
	init_camera();
	init_lighting();

	lsys = new LSystem();
	lsys->setIterations(4);
	lsys->setAngle(30.f);
	lsys->setSegmentLength(1.f);
	lsys->setStart('X');

	lsys->addRule('X', "YZ[+X]-X");
	lsys->addStochasticRules('Y',
	{
		std::make_pair(0.5f, std::string("Y")),
		std::make_pair(0.5f, std::string("Y"))
	});
	lsys->addFinishRule('X', "F");
	lsys->addFinishRule('Y', "F");
	lsys->addStochasticFinishRules('Z',
	{
		std::make_pair(0.25f, std::string("<")),
		std::make_pair(0.25f, std::string(">")),
		std::make_pair(0.25f, std::string("^")),
		std::make_pair(0.25f, std::string("v"))
	});

	//lsys->addRule('F', "FF-[vF^F^F]+[^FvFvF]<[^F^FvF]");

	//lsys->addStochasticRules('F',
	//{
	//	std::make_pair(1.f / 6.f, std::string("F-F++F-F")),
	//	std::make_pair(1.f / 6.f, std::string("F--F+F")),
	//	std::make_pair(1.f / 6.f, std::string("FvF^^FvF")),
	//	std::make_pair(1.f / 6.f, std::string("FvvF^F")),
	//	std::make_pair(1.f / 6.f, std::string("F<F>>F<F")),
	//	std::make_pair(1.f / 6.f, std::string("F<<F>F")),
	//});

	//lsys->addStochasticRules('F',
	//{
	//	std::make_pair(1.f / 3.f, std::string("F[+F][-F]")),
	//	std::make_pair(1.f / 3.f, std::string("F[^F][vF]")),
	//	std::make_pair(1.f / 3.f, std::string("F[<F][>F]")),
	//});

	//lsys->addStochasticRules('F',
	//{
	//	std::make_pair(0.5f, std::string("F[-F][+F]F")),
	//	std::make_pair(0.5f, std::string("FF"))
	//});
	//lsys->addStochasticRules('X', 
	//{
	//	std::make_pair(0.5f, std::string("^")),
	//	std::make_pair(0.5f, std::string("v"))
	//});

	//lsys->addRule('X', "-YF+XFX+FY-");
	//lsys->addRule('Y', "+XF-YFY-FX+");

	//std::cout << lsys->run() << std::endl;

	return true;
}

void Engine::mainLoop()
{
	m_fLastTime = static_cast<float>(glfwGetTime());

	// Main Rendering Loop
	while (!glfwWindowShouldClose(m_pWindow)) {
		update(m_fStepSize);

		draw();

		render();

		// Flip buffers and render to screen
		glfwSwapBuffers(m_pWindow);
	}
}

void Engine::update(float dt)
{
	// Calculate deltatime of current frame
	float newTime = static_cast<float>(glfwGetTime());
	m_fDeltaTime = newTime - m_fLastTime;
	m_fLastTime = newTime;

	// Poll input events first
	GLFWInputBroadcaster::getInstance().poll();

	m_pCamera->update(dt);

	// Create camera transformations
	glm::mat4 view = m_pCamera->getViewMatrix() * glm::inverse(m_pArcball->getTransformation());
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
}

void Engine::draw()
{
	Renderer::RendererSubmission rs;
	rs.primitiveType = GL_TRIANGLES;
	rs.shaderName = "flat";
	rs.VAO = lsys->getVAO();
	rs.vertCount = lsys->getIndexCount();
	rs.modelToWorldTransform = glm::mat4(lsys->getOrientation()) * glm::translate(glm::mat4(), glm::vec3(lsys->getDataCenteringAdjustments()));

	Renderer::getInstance().addToDynamicRenderQueue(rs);
}

void Engine::render()
{
	Renderer::getInstance().RenderFrame(m_iWidth, m_iHeight);
}

GLFWwindow * Engine::init_gl_context(std::string winName)
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 16);
#if _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
	GLFWwindow* mWindow = glfwCreateWindow(m_iWidth, m_iHeight, winName.c_str(), nullptr, nullptr);

	// Check for Valid Context
	if (mWindow == nullptr)
		return nullptr;

	// Create Context and Load OpenGL Functions
	glfwMakeContextCurrent(mWindow);

	// GLFW Options
	//glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	glewInit();
	fprintf(stderr, "OpenGL %s\n", glGetString(GL_VERSION));

	// Define the viewport dimensions
	glViewport(0, 0, m_iWidth, m_iHeight);

#if _DEBUG
	glDebugMessageCallback((GLDEBUGPROC)DebugCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

	return mWindow;
}

void Engine::init_lighting()
{
	m_pLightingSystem = new LightingSystem();

	m_pLightingSystem->addDirectLight()->ambientCoefficient = 0.01f;

	// Positions of the point lights
	//m_pLightingSystem->addPointLight(glm::vec4(25.f, 0.f, 25.f, 1.f));
	//m_pLightingSystem->addPointLight(glm::vec4(25.f, 0.f, -25.f, 1.f));
	//m_pLightingSystem->addPointLight(glm::vec4(-25.f, 0.f, 25.f, 1.f));
	//m_pLightingSystem->addPointLight(glm::vec4(-25.f, 0.f, -25.f, 1.f));

	m_pLightingSystem->registerShader(Renderer::getInstance().getShader("lighting"));
	m_pLightingSystem->registerShader(Renderer::getInstance().getShader("lightingWireframe"));
}

void Engine::init_camera()
{
	m_pCamera = new Camera(glm::vec3(0.f, 0.f, 25.f), m_vec3DefaultUp, m_iWidth, m_iHeight);
	GLFWInputBroadcaster::getInstance().attach(m_pCamera);

	m_pArcball = new ArcBall(glm::vec3(glm::vec2(m_iWidth, m_iHeight) / 2.f, 0.f), 0.45f * (std::min)(m_iWidth, m_iHeight));
}
