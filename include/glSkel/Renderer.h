#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glSkel/LightingSystem.h>
#include <glSkel/shaderset.h>

struct FrameUniforms {
	glm::vec4 v4Viewport;
	glm::mat4 m4View;
	glm::mat4 m4Projection;
	glm::mat4 m4ViewProjection;
};

class Renderer
{
public:
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
	// Singleton instance access
	static Renderer& getInstance()
	{
		static Renderer s_instance;
		return s_instance;
	}
	
	bool init();

	GLuint* getShader(const char *name);
	
	void addToStaticRenderQueue(RendererSubmission &rs);
	void addToDynamicRenderQueue(RendererSubmission &rs);

	void toggleWireframe();

	void RenderFrame(GLFWwindow *m_pWindow);

	void Shutdown();

private:
	Renderer();
	~Renderer();

	void SetupShaders();

	void processRenderQueue(std::vector<RendererSubmission> &renderQueue);

	void RenderScene();

private:
	ShaderSet m_Shaders;

	std::vector<RendererSubmission> m_vStaticRenderQueue;
	std::vector<RendererSubmission> m_vDynamicRenderQueue;

	bool m_bShowWireframe;

	std::map<std::string, GLuint*> m_mapShaders;

	GLuint m_glFrameUBO;

	int m_nWindowWidth;
	int m_nWindowHeight;
	
	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;
	float m_fNearClip;
	float m_fFarClip;

	glm::mat4 m_mat4Projection;
	glm::mat4 m_mat4View;

public:
	// DELETE THE FOLLOWING FUNCTIONS TO AVOID NON-SINGLETON USE
	Renderer(Renderer const&) = delete;
	void operator=(Renderer const&) = delete;
};

