#include "Renderer.h"

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glSkel/DebugDrawer.h>

#include <glSkel/GLSLpreamble.h>

Renderer::Renderer()
	: m_pLightingSystem(NULL)
	, m_glFrameUBO(0)
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
	m_pLightingSystem = new LightingSystem();
	// add a directional light and change its ambient coefficient
	m_pLightingSystem->addDirectLight()->ambientCoefficient = 0.5f;
	
	glCreateBuffers(1, &m_glFrameUBO);
	glNamedBufferData(m_glFrameUBO, sizeof(FrameUniforms), NULL, GL_STATIC_DRAW); // allocate memory
	glBindBufferRange(GL_UNIFORM_BUFFER, SCENE_UNIFORM_BUFFER_LOCATION, m_glFrameUBO, 0, sizeof(FrameUniforms));

	// Set viewport for shader uniforms
	glNamedBufferSubData(m_glFrameUBO, offsetof(FrameUniforms, v4Viewport), sizeof(FrameUniforms::v4Viewport), glm::value_ptr(glm::vec4(0, 0, m_nRenderWidth, m_nRenderHeight)));

	SetupShaders();
	SetupCameras();

	return true;
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
	m_Shaders.SetVersion("420");

	m_Shaders.SetPreambleFile("GLSLpreamble.h");

	m_mapShaders["lighting"] = m_Shaders.AddProgramFromExts({ "shaders/lighting.vert", "shaders/lighting.frag" });
	m_mapShaders["lightingWireframe"] = m_Shaders.AddProgramFromExts({ "shaders/lighting.vert", "shaders/lightingWF.geom", "shaders/lightingWF.frag" });
	m_mapShaders["debug"] = m_Shaders.AddProgramFromExts({ "shaders/debugDrawer.vert", "shaders/debugDrawer.frag" });

	m_pLightingSystem->addShaderToUpdate(m_mapShaders["lighting"]);
	m_pLightingSystem->addShaderToUpdate(m_mapShaders["lightingWireframe"]);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Renderer::SetupCameras()
{
	m_mat4Projection = glm::perspective(
		glm::radians(m_pCamera->getZoom()),
		static_cast<float>(m_iWidth) / static_cast<float>(m_iHeight),
		0.01f,
		1000.0f
		);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Renderer::RenderFrame()
{
	m_mat4CurrentHMDView = HMDView;
	SDL_GetWindowSize(win, &m_nCompanionWindowWidth, &m_nCompanionWindowHeight);

	m_Shaders.UpdatePrograms();

	// for now as fast as possible
	if (m_pHMD)
	{
		RenderStereoTargets();
		RenderCompanionWindow();

		vr::Texture_t leftEyeTexture = { (void*)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}

	m_vDynamicRenderQueue.clear();
	DebugDrawer::getInstance().flushLines();

	// SwapWindow
	{
		SDL_GL_SwapWindow(win);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Renderer::RenderScene()
{
	glClearColor(0.15f, 0.15f, 0.18f, 1.0f); // nice background color, but not black
											 //glClearColor(0.33, 0.39, 0.49, 1.0); //VTT4D background
	glEnable(GL_MULTISAMPLE);

	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glm::mat4 thisEyesViewMatrix = (nEye == vr::Eye_Left ? m_mat4eyePoseLeft : m_mat4eyePoseRight) * m_mat4CurrentHMDView;
	glm::mat4 thisEyesProjectionMatrix = (nEye == vr::Eye_Left ? m_mat4ProjectionLeft : m_mat4ProjectionRight);
	glm::mat4 thisEyesViewProjectionMatrix = thisEyesProjectionMatrix * thisEyesViewMatrix;

	glNamedBufferSubData(m_glFrameUBO, offsetof(FrameUniforms, m4View), sizeof(FrameUniforms::m4View), glm::value_ptr(thisEyesViewMatrix));
	glNamedBufferSubData(m_glFrameUBO, offsetof(FrameUniforms, m4Projection), sizeof(FrameUniforms::m4Projection), glm::value_ptr(thisEyesProjectionMatrix));
	glNamedBufferSubData(m_glFrameUBO, offsetof(FrameUniforms, m4ViewProjection), sizeof(FrameUniforms::m4ViewProjection), glm::value_ptr(thisEyesViewProjectionMatrix));

	m_pLighting->update(thisEyesViewMatrix);


	if (*m_mapShaders["debug"])
	{
		glUseProgram(*m_mapShaders["debug"]);
		glUniformMatrix4fv(MODEL_MAT_UNIFORM_LOCATION, 1, GL_FALSE, glm::value_ptr(glm::mat4()));
		DebugDrawer::getInstance().render();
	}

	// STATIC OBJECTS
	processRenderQueue(m_vStaticRenderQueue);

	// DYNAMIC OBJECTS
	processRenderQueue(m_vDynamicRenderQueue);

	glDisable(GL_BLEND);

	glUseProgram(0);
}

void Renderer::processRenderQueue(std::vector<RendererSubmission> &renderQueue)
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
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Renderer::RenderCompanionWindow()
{
	if (m_mapShaders["companionWindow"] == NULL)
		return;

	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, m_nCompanionWindowWidth, m_nCompanionWindowHeight);

	glUseProgram(*m_mapShaders["companionWindow"]);
	glBindVertexArray(m_unCompanionWindowVAO);

		// render left eye (first half of index array )
		glBindTextureUnit(DIFFUSE_TEXTURE_BINDING, leftEyeDesc.m_nResolveTextureId);
		glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize, GL_UNSIGNED_SHORT, 0);

		// render right eye (second half of index array )
		//glBindTextureUnit(DIFFUSE_TEXTURE_BINDING, rightEyeDesc.m_nResolveTextureId);
		//glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(uintptr_t)(m_uiCompanionWindowIndexSize));

	glBindVertexArray(0);
	glUseProgram(0);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
glm::mat4 Renderer::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return glm::mat4();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);

	return glm::mat4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
glm::mat4 Renderer::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return glm::mat4();

	vr::HmdMatrix34_t matEyeToHead = m_pHMD->GetEyeToHeadTransform(nEye);
	glm::mat4 matrixObj(
		matEyeToHead.m[0][0], matEyeToHead.m[1][0], matEyeToHead.m[2][0], 0.f,
		matEyeToHead.m[0][1], matEyeToHead.m[1][1], matEyeToHead.m[2][1], 0.f,
		matEyeToHead.m[0][2], matEyeToHead.m[1][2], matEyeToHead.m[2][2], 0.f,
		matEyeToHead.m[0][3], matEyeToHead.m[1][3], matEyeToHead.m[2][3], 1.f
	);

	return glm::inverse(matrixObj);
}

void Renderer::Shutdown()
{
	glDeleteBuffers(1, &m_glCompanionWindowIDVertBuffer);
	glDeleteBuffers(1, &m_glCompanionWindowIDIndexBuffer);
}
