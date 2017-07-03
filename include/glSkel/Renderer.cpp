#include "Renderer.h"

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glSkel/DebugDrawer.h>

#include <glSkel/GLSLpreamble.h>

Renderer::Renderer()
	: m_glFrameUBO(0)
	, m_bShowWireframe(false)
	, m_fNearClip(0.1f)
	, m_fFarClip(50.0f)
{
}

Renderer::~Renderer()
{
	Shutdown();
}

bool Renderer::init()
{	
	glCreateBuffers(1, &m_glFrameUBO);
	glNamedBufferData(m_glFrameUBO, sizeof(FrameUniforms), NULL, GL_STATIC_DRAW); // allocate memory
	glBindBufferRange(GL_UNIFORM_BUFFER, SCENE_UNIFORM_BUFFER_LOCATION, m_glFrameUBO, 0, sizeof(FrameUniforms));
	
	SetupShaders();

	return true;
}

GLuint* Renderer::getShader(const char * name)
{
	return m_mapShaders.count(name) > 0 ? m_mapShaders[name] : NULL;
}

const GLuint * Renderer::getFrameUBO()
{
	return &m_glFrameUBO;
}

void Renderer::addToStaticRenderQueue(RendererSubmission &rs)
{
	m_vStaticRenderQueue.push_back(rs);
}

void Renderer::addToDynamicRenderQueue(RendererSubmission &rs)
{
	m_vDynamicRenderQueue.push_back(rs);
}

void Renderer::toggleWireframe()
{
	m_bShowWireframe = !m_bShowWireframe;
}


//-----------------------------------------------------------------------------
// Purpose: Creates all the shaders used by HelloVR SDL
//-----------------------------------------------------------------------------
void Renderer::SetupShaders()
{
	m_Shaders.SetVersion("450");

	m_Shaders.SetPreambleFile("GLSLpreamble.h");

	m_mapShaders["lighting"] = m_Shaders.AddProgramFromExts({ "shaders/lighting.vert", "shaders/lighting.frag" });
	m_mapShaders["lightingWireframe"] = m_Shaders.AddProgramFromExts({ "shaders/lighting.vert", "shaders/lightingWF.geom", "shaders/lightingWF.frag" });
	m_mapShaders["debug"] = m_Shaders.AddProgramFromExts({ "shaders/debugDrawer.vert", "shaders/debugDrawer.frag" });
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Renderer::RenderFrame(GLsizei width, GLsizei height)
{
	m_Shaders.UpdatePrograms();

	// for now as fast as possible
	glClearColor(0.15f, 0.15f, 0.18f, 1.0f); // nice background color, but not black
	 //glClearColor(0.33, 0.39, 0.49, 1.0); //VTT4D background

	glEnable(GL_MULTISAMPLE);

	glViewport(0, 0, width, height);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	

	if (*m_mapShaders["debug"])
	{
		glUseProgram(*m_mapShaders["debug"]);
		glUniformMatrix4fv(MODEL_MAT_UNIFORM_LOCATION, 1, GL_FALSE, glm::value_ptr(glm::mat4()));
		DebugDrawer::getInstance().render();
		DebugDrawer::getInstance().flushLines();
	}

	// STATIC OBJECTS
	processRenderQueue(m_vStaticRenderQueue);

	// DYNAMIC OBJECTS
	processRenderQueue(m_vDynamicRenderQueue, true);

	glDisable(GL_BLEND);

	glUseProgram(0);	
}

void Renderer::processRenderQueue(std::vector<RendererSubmission> &renderQueue, bool clearQueueAfterProcessing)
{
	for (auto i : renderQueue)
	{
		if (m_mapShaders[i.shaderName] && *m_mapShaders[i.shaderName])
		{
			glUseProgram(*m_mapShaders[i.shaderName]);
			glUniformMatrix4fv(MODEL_MAT_UNIFORM_LOCATION, 1, GL_FALSE, glm::value_ptr(i.modelToWorldTransform));

			if (i.specularExponent > 0.f)
				glUniform1f(MATERIAL_SHININESS_UNIFORM_LOCATION, i.specularExponent);

			if (i.diffuseTex > 0u)
				glBindTextureUnit(DIFFUSE_TEXTURE_BINDING, i.diffuseTex);
			if (i.specularTex > 0u)
				glBindTextureUnit(SPECULAR_TEXTURE_BINDING, i.specularTex);

			glBindVertexArray(i.VAO);
			glDrawElements(i.primitiveType, i.vertCount, GL_UNSIGNED_SHORT, 0);
			glBindVertexArray(0);
		}
	}

	if (clearQueueAfterProcessing)
		renderQueue.clear();
}

void Renderer::Shutdown()
{
}
