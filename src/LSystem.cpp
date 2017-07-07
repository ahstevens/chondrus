#include "LSystem.h"

#include <glSkel/Renderer.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 tex;
};

LSystem::LSystem()
	: m_nIters(0)
	, m_bNeedsRefresh(true)
{
	glGenVertexArrays(1, &m_glVAO);
	glGenBuffers(1, &m_glVBO);
	glGenBuffers(1, &m_glEBO);

	glBindVertexArray(this->m_glVAO);
	// Load data into vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, this->m_glVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_glEBO);

	// Set the vertex attribute pointers
	// Vertex Positions
	glEnableVertexAttribArray(POSITION_ATTRIB_LOCATION);
	glVertexAttribPointer(POSITION_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, pos));
	// Vertex Normals
	glEnableVertexAttribArray(NORMAL_ATTRIB_LOCATION);
	glVertexAttribPointer(NORMAL_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, norm));
	// Vertex Texture Coords
	glEnableVertexAttribArray(TEXCOORD_ATTRIB_LOCATION);
	glVertexAttribPointer(TEXCOORD_ATTRIB_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tex));

	glBindVertexArray(0);


	// Load textures
	glGenTextures(1, &m_glTexture);
	int width = 1, height = 1;
	unsigned char image[3] = { 0x88, 0x11, 0x33 };

	// Diffuse map
	glBindTexture(GL_TEXTURE_2D, m_glTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &image);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
}

LSystem::~LSystem()
{
}

void LSystem::setStart(char symbol)
{
	m_chStartSymbol = symbol;
	m_bNeedsRefresh = true;
}

void LSystem::setIterations(unsigned int iters)
{
	m_nIters = iters;
	m_bNeedsRefresh = true;
}

void LSystem::setAngle(float angle)
{
	m_fAngle = angle;
}

void LSystem::setSegmentLength(float len)
{
	m_fSegLen = len;
}

// returns true if a rule already exists for the symbol
bool LSystem::addRule(char symbol, std::string replacement)
{
	std::map<char, std::string>::iterator it = m_mapRules.find(symbol);

	m_mapRules[symbol] = replacement;

	m_bNeedsRefresh = true;

	if (it != m_mapRules.end())
		return true;
	else
		return false;
}

std::string LSystem::run()
{
	if (!m_bNeedsRefresh)
		return m_strResult;

	std::string startStr = std::string(1, m_chStartSymbol);
	std::string endStr;

	for (unsigned int i = 0u; i < m_nIters; ++i)
	{
		endStr = process(startStr);
		startStr = endStr;
	}

	m_strResult = endStr;
	m_bNeedsRefresh = false;

	return m_strResult;
}

void LSystem::draw()
{
	if (m_bNeedsRefresh)
	{
		glm::vec3 turtlePos(0.f);
		glm::quat heading;

		std::vector<Vertex> pts;
		m_usInds.clear();
		GLushort currInd = 0;

		for (auto const& c : run())
		{
			Vertex v;
			v.tex = glm::vec2(0.5f);
			
			switch (c)
			{
			case 'F':
			{
				glm::vec3 headingVec = glm::rotate(heading, glm::vec3(1.f, 0.f, 0.f));
				v.pos = turtlePos;
				v.norm = v.pos;
				pts.push_back(v);
				m_usInds.push_back(currInd++);
				v.pos = turtlePos + headingVec * m_fSegLen;
				v.norm = v.pos;
				pts.push_back(v);
				m_usInds.push_back(currInd++);
				turtlePos += headingVec * m_fSegLen;
				break;
			}
			case 'B':
			{
				glm::vec3 headingVec = glm::rotate(heading, glm::vec3(1.f, 0.f, 0.f));
				v.pos = turtlePos;
				v.norm = v.pos;
				pts.push_back(v);
				m_usInds.push_back(currInd++);
				v.pos = turtlePos - headingVec * m_fSegLen;
				v.norm = v.pos;
				pts.push_back(v);
				m_usInds.push_back(currInd++);
				turtlePos -= headingVec * m_fSegLen;
				break;
			}
			case '+':
				heading = glm::rotate(heading, glm::radians(m_fAngle), glm::vec3(0.f, 0.f, 1.f));
				break;
			case '-':
				heading = glm::rotate(heading, glm::radians(-m_fAngle), glm::vec3(0.f, 0.f, 1.f));
				break;
			case '^':
				heading = glm::rotate(heading, glm::radians(m_fAngle), glm::vec3(0.f, 1.f, 0.f));
				break;
			case 'v':
				heading = glm::rotate(heading, glm::radians(-m_fAngle), glm::vec3(0.f, 1.f, 0.f));
				break;
			case '<':
				heading = glm::rotate(heading, glm::radians(m_fAngle), glm::vec3(1.f, 0.f, 0.f));
				break;
			case '>':
				heading = glm::rotate(heading, glm::radians(-m_fAngle), glm::vec3(1.f, 0.f, 0.f));
				break;
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, this->m_glVBO);
		glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(Vertex), 0, GL_STREAM_DRAW);
		glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(Vertex), &pts[0], GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_glEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_usInds.size() * sizeof(GLushort), 0, GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_usInds.size() * sizeof(GLushort), &m_usInds[0], GL_STATIC_DRAW);
	}

	Renderer::RendererSubmission rs;
	rs.primitiveType = GL_LINES;
	rs.shaderName = "lighting";
	rs.VAO = m_glVAO;
	rs.vertCount = m_usInds.size();
	rs.diffuseTex = m_glTexture;
	rs.specularTex = m_glTexture;
	rs.specularExponent = 32.f;
	rs.modelToWorldTransform = glm::mat4();

	Renderer::getInstance().addToDynamicRenderQueue(rs);
}

std::string LSystem::process(std::string oldstr)
{
	std::string newstr;

	for (auto const &c : oldstr)
		newstr += applyRules(c);

	return newstr;
}

std::string LSystem::applyRules(char symbol)
{
	std::map<char, std::string>::iterator it = m_mapRules.find(symbol);

	if (it != m_mapRules.end())
		return it->second;
	else
		return std::string(1, symbol);
}
