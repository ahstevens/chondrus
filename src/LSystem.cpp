#include "LSystem.h"

#include <glSkel/Renderer.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

#include <random>


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
		// Bind the array and element buffers
		glBindBuffer(GL_ARRAY_BUFFER, this->m_glVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_glEBO);

		// Enable attribute arrays (with layouts to be defined later)
		glEnableVertexAttribArray(POSITION_ATTRIB_LOCATION);
		glVertexAttribPointer(POSITION_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (GLvoid*)0);
		glEnableVertexAttribArray(COLOR_ATTRIB_LOCATION);
		// color attribute pointer will be set once position array size is known for attrib pointer offset

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
	float epsilon = 0.00001;
	for (auto const &rule : replacementRules)
	{
		sum += rule.first;
	}
	if (fabs(sum - 1.f) > epsilon)
	{
		std::cerr << "Error: Stochastic replacement rules for symbol '" << symbol << "' do not have probabilities that add up to 1.0 (total = " << sum << ")" << std::endl;
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

	glm::vec3 turtlePos(0.f);
	glm::quat turtleHeading;

	std::vector<Scaffold::Node*> turtleStack;

	for (auto &n : m_Scaffold.vNodes)
		delete n;
	m_Scaffold.vNodes.clear();

	for (auto &s : m_Scaffold.vSegments)
		delete s;
	m_Scaffold.vSegments.clear();

	Scaffold::Node *prevNode = new Scaffold::Node(turtlePos, turtleHeading);
	m_Scaffold.vNodes.push_back(prevNode);

	for (auto const& c : m_strResult)
	{
		switch (c)
		{
		case 'F':
		{
			glm::vec3 headingVec = glm::rotate(turtleHeading, glm::vec3(0.f, 1.f, 0.f));

			turtlePos += headingVec * m_fSegLen;

			Scaffold::Node *node = new Scaffold::Node(turtlePos, turtleHeading);
			m_Scaffold.vSegments.push_back(new Scaffold::Segment(prevNode, node));
			m_Scaffold.vNodes.push_back(node);
			prevNode = node;

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
			turtleStack.push_back(prevNode);
			break;
		case ']':
			prevNode = turtleStack.back();
			turtlePos = prevNode->vec3Pos;
			turtleHeading = prevNode->qRot;
			turtleStack.pop_back();
			break;
		default:
			std::cerr << "Error: Symbol '" << c << "' not found in turtle commands." << std::endl;
		}
	}

	m_bNeedsRefresh = false;

	return m_strResult;
}

void LSystem::draw()
{
	if (m_bNeedsRefresh)
	{
		run();

		m_vvec3Points.clear();
		m_vvec4Colors.clear();
		m_vusInds.clear();

		//generateLines();
		generateQuads();

		glBindBuffer(GL_ARRAY_BUFFER, this->m_glVBO);
		// Buffer orphaning
		glBufferData(GL_ARRAY_BUFFER, m_vvec3Points.size() * sizeof(glm::vec3) + m_vvec4Colors.size() * sizeof(glm::vec4), 0, GL_STREAM_DRAW);
		// Sub buffer data for points, then colors
		glBufferSubData(GL_ARRAY_BUFFER, 0, m_vvec3Points.size() * sizeof(glm::vec3), &m_vvec3Points[0]);
		glBufferSubData(GL_ARRAY_BUFFER, m_vvec3Points.size() * sizeof(glm::vec3), m_vvec4Colors.size() * sizeof(glm::vec4), &m_vvec4Colors[0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_glEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_vusInds.size() * sizeof(GLushort), 0, GL_STREAM_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_vusInds.size() * sizeof(GLushort), &m_vusInds[0], GL_STREAM_DRAW);

		// Set color sttribute pointer now that point array size is known
		glBindVertexArray(this->m_glVAO);
			glVertexAttribPointer(COLOR_ATTRIB_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (GLvoid*)(m_vvec3Points.size() * sizeof(glm::vec3)));
		glBindVertexArray(0);
	}

	Renderer::RendererSubmission rs;
	rs.primitiveType = GL_TRIANGLES;
	rs.shaderName = "flat";
	rs.VAO = m_glVAO;
	rs.vertCount = m_vusInds.size();
	rs.modelToWorldTransform = glm::mat4(m_mat3Rotation);

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

void LSystem::generateLines()
{
	GLushort currInd = 0;
	for (auto const& seg : m_Scaffold.vSegments)
	{
		glm::vec3 originHeading(glm::rotate(seg->origin->qRot, glm::vec3(0.f, 1.f, 0.f)));

		glm::mat3 tRot = glm::mat3_cast(seg->terminus->qRot);
		glm::vec3 terminusHeading(tRot[1]);

		m_vvec3Points.push_back(seg->origin->vec3Pos);
		m_vvec4Colors.push_back(glm::vec4((originHeading + 1.f) * 0.5f, 1.f));
		m_vusInds.push_back(currInd++);

		m_vvec3Points.push_back(seg->terminus->vec3Pos);
		m_vvec4Colors.push_back(glm::vec4((terminusHeading + 1.f) * 0.5f, 1.f));
		m_vusInds.push_back(currInd++);
	}
}

void LSystem::generateQuads()
{
	GLushort currInd = 0;
	for (auto const& seg : m_Scaffold.vSegments)
	{
		glm::vec3 originHeading(glm::rotate(seg->origin->qRot, glm::vec3(0.f, 1.f, 0.f)));
		glm::vec3 terminusHeading(glm::rotate(seg->terminus->qRot, glm::vec3(0.f, 1.f, 0.f)));

		glm::mat3 tRot = glm::mat3_cast(seg->terminus->qRot);

		glm::vec3 localRight = glm::normalize(tRot[0]) * (m_fSegLen / 10.f);
		glm::vec3 localLeft = -localRight;

		m_vvec3Points.push_back(seg->origin->vec3Pos + localLeft);
		m_vvec3Points.push_back(seg->origin->vec3Pos + localRight);
		m_vvec4Colors.push_back(glm::vec4((originHeading + 1.f) * 0.5f, 1.f));
		m_vvec4Colors.push_back(glm::vec4((originHeading + 1.f) * 0.5f, 1.f));

		m_vvec3Points.push_back(seg->terminus->vec3Pos + localLeft);
		m_vvec3Points.push_back(seg->terminus->vec3Pos + localRight);
		m_vvec4Colors.push_back(glm::vec4((terminusHeading + 1.f) * 0.5f, 1.f));
		m_vvec4Colors.push_back(glm::vec4((terminusHeading + 1.f) * 0.5f, 1.f));

		m_vusInds.push_back(currInd + 0u);
		m_vusInds.push_back(currInd + 1u);
		m_vusInds.push_back(currInd + 2u);

		m_vusInds.push_back(currInd + 1u);
		m_vusInds.push_back(currInd + 3u);
		m_vusInds.push_back(currInd + 2u);

		currInd += 4;
	}
}
