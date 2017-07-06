#pragma once

#include <glSkel/renderer.h>
#include <glSkel/camera.h>
#include <glSkel/LightingSystem.h>
#include <glSkel/DebugDrawer.h>

#include "GLSLpreamble.h"
#include "GLFWInputBroadcaster.h"
#include "Chondrus.h"

#define MS_PER_UPDATE 0.0333333333f
#define CAST_RAY_LEN 1000.f

class Engine : public BroadcastSystem::Listener
{
public:
	GLFWwindow* m_pWindow;
	LightingSystem* m_pLightingSystem;


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
	Engine();
	~Engine();

	void receiveEvent(Object * obj, const int event, void * data);

	bool init();

	void mainLoop();

	void update(float dt);

private:
	bool m_bRunPhysics;

	GLFWwindow* init_gl_context(std::string winName);

	// Initialize the lighting system
	void init_lighting();

	void init_camera();

	void generateModels();
};
