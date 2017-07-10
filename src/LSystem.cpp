#include "LSystem.h"

#include <glSkel/Renderer.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

#include <random>

struct Vertex {
	glm::vec3 pos;
	glm::vec4 col;
};

LSystem::LSystem()
	: m_nIters(0)
	, m_bNeedsRefresh(true)
	, m_mtEngine(std::random_device()())
	, m_UniformDist(0.f, 1.f)
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
	// Vertex Colors
	glEnableVertexAttribArray(COLOR_ATTRIB_LOCATION);
	glVertexAttribPointer(COLOR_ATTRIB_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, col));

	glBindVertexArray(0);
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

void LSystem::setRefreshNeeded()
{
	m_bNeedsRefresh = true;
}

// returns true if a rule already exists for the symbol
bool LSystem::addRule(char symbol, std::string replacement)
{
	std::map<char, std::vector<std::pair<float, std::string>>>::iterator it = m_mapRules.find(symbol);

	m_mapRules[symbol].clear();
	m_mapRules[symbol].push_back(std::make_pair(1.f, replacement));

	m_bNeedsRefresh = true;

	if (it != m_mapRules.end())
		return true;
	else
		return false;
}

bool LSystem::addStochasticRules(char symbol, std::vector<std::pair<float, std::string>> replacementRules)
{
	float sum = 0.f;
	for (auto const &rule : replacementRules)
	{
		sum += rule.first;
	}
	if (sum != 1.f)
	{
		std::cerr << "Error: Stochastic replacement rules for symbol '" << symbol << "' do not have probabilities that add up to 1.0" << std::endl;
		return false;
	}

	m_mapRules[symbol] = replacementRules;

	return true;
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
		glm::quat turtleHeading;

		std::vector<std::pair<glm::vec3, glm::quat>> turtleStack;

		std::vector<Vertex> pts;
		m_usInds.clear();
		GLushort currInd = 0;

		for (auto const& c : run())
		{
			Vertex v;
			
			switch (c)
			{
			case 'F':
			{
				glm::vec3 headingVec = glm::rotate(turtleHeading, glm::vec3(1.f, 0.f, 0.f));
				v.pos = turtlePos;
				v.col = glm::vec4((headingVec + 1.f) * 0.5f, 1.f);
				pts.push_back(v);
				m_usInds.push_back(currInd++);
				v.pos = turtlePos + headingVec * m_fSegLen;
				pts.push_back(v);
				m_usInds.push_back(currInd++);
				turtlePos += headingVec * m_fSegLen;
				break;
			}
			case 'B':
			{
				glm::vec3 headingVec = glm::rotate(turtleHeading, glm::vec3(1.f, 0.f, 0.f));
				v.pos = turtlePos;
				v.col = glm::vec4((headingVec + 1.f) * 0.5f, 1.f);
				pts.push_back(v);
				m_usInds.push_back(currInd++);
				v.pos = turtlePos - headingVec * m_fSegLen;
				pts.push_back(v);
				m_usInds.push_back(currInd++);
				turtlePos -= headingVec * m_fSegLen;
				break;
			}
			case '+':
				turtleHeading = glm::rotate(turtleHeading, glm::radians(m_fAngle), glm::vec3(0.f, 0.f, 1.f));
				break;
			case '-':
				turtleHeading = glm::rotate(turtleHeading, glm::radians(-m_fAngle), glm::vec3(0.f, 0.f, 1.f));
				break;
			case '^':
				turtleHeading = glm::rotate(turtleHeading, glm::radians(m_fAngle), glm::vec3(0.f, 1.f, 0.f));
				break;
			case 'v':
				turtleHeading = glm::rotate(turtleHeading, glm::radians(-m_fAngle), glm::vec3(0.f, 1.f, 0.f));
				break;
			case '<':
				turtleHeading = glm::rotate(turtleHeading, glm::radians(m_fAngle), glm::vec3(1.f, 0.f, 0.f));
				break;
			case '>':
				turtleHeading = glm::rotate(turtleHeading, glm::radians(-m_fAngle), glm::vec3(1.f, 0.f, 0.f));
				break;
			case '[':
				turtleStack.push_back(std::make_pair(turtlePos, turtleHeading));
				break;
			case ']':
				turtlePos = turtleStack.back().first;
				turtleHeading = turtleStack.back().second;
				turtleStack.pop_back();
				break;
			default:
				std::cerr << "Error: Symbol '" << c << "' not found in ruleset." << std::endl;
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, this->m_glVBO);
		glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(Vertex), 0, GL_STREAM_DRAW);
		glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(Vertex), &pts[0], GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_glEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_usInds.size() * sizeof(GLushort), 0, GL_STREAM_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_usInds.size() * sizeof(GLushort), &m_usInds[0], GL_STREAM_DRAW);
	}

	Renderer::RendererSubmission rs;
	rs.primitiveType = GL_LINES;
	rs.shaderName = "flat";
	rs.VAO = m_glVAO;
	rs.vertCount = m_usInds.size();
	rs.modelToWorldTransform = glm::mat4_cast(glm::rotate(glm::quat(), glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f)));

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
	// Check if replacement rule exists for symbol
	std::map<char, std::vector<std::pair<float, std::string>>>::iterator it = m_mapRules.find(symbol);

	// Rule Exists
	if (it != m_mapRules.end())
	{
		float val = m_UniformDist(m_mtEngine);
		float cumsum = 0.f;

		for (auto const &rule : it->second)
		{
			cumsum += rule.first;
			if (val <= cumsum)
				return rule.second;
		}

		std::cerr << "Error: Failed to apply stochastic rules for symbol '" << symbol << "'!" << std::endl;

		return std::string(1, symbol);
	}
	else // Rule does not exist
	{
		return std::string(1, symbol);
	}
}
