#include "LSystem.h"

#include <glSkel/Renderer.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

#include <random>


LSystem::LSystem()
	: Dataset("Chondrus crispus")
	, m_nIters(0)
	, m_pCurrentNode(NULL)
	, m_bNeedsRefresh(true)
	, m_mtEngine(std::random_device()())
	, m_UniformDist(0.f, 1.f)
{
	makeTurtleCommands();

	initGL();
}

LSystem::~LSystem()
{
}

void LSystem::makeTurtleCommands()
{
	m_mapTurtleCommands['F'] = std::function<void()>([&]() {
		glm::vec3 headingVec = glm::rotate(m_Turtle.orientation, glm::vec3(0.f, 1.f, 0.f));

		m_Turtle.position += headingVec * m_Turtle.stepSize;

		checkNewRawPosition(m_Turtle.position);

		Scaffold::Node *newNode = new Scaffold::Node(m_Turtle.position, m_Turtle.orientation, m_Turtle.size);
		newNode->parentNode = m_pCurrentNode;
		m_pCurrentNode->vChildren.push_back(newNode);
		m_Scaffold.vNodes.push_back(newNode);

		Scaffold::Segment *seg = new Scaffold::Segment(m_pCurrentNode, newNode);
		m_pCurrentNode->vSegments.push_back(seg);
		newNode->vSegments.push_back(seg);
		m_Scaffold.vSegments.push_back(seg);

		m_pCurrentNode = newNode;
	});

	m_mapTurtleCommands['+'] = std::function<void()>([&]() {
		m_Turtle.orientation = glm::rotate(
			m_Turtle.orientation, 
			glm::radians(m_Turtle.turnAngle), 
			glm::vec3(0.f, 0.f, 1.f)
		);
	});

	m_mapTurtleCommands['-'] = std::function<void()>([&]() {
		m_Turtle.orientation = glm::rotate(
			m_Turtle.orientation,
			glm::radians(-m_Turtle.turnAngle),
			glm::vec3(0.f, 0.f, 1.f)
		);
	});

	m_mapTurtleCommands['<'] = std::function<void()>([&]() {
		m_Turtle.orientation = glm::rotate(
			m_Turtle.orientation,
			glm::radians(m_Turtle.turnAngle),
			glm::vec3(0.f, 1.f, 0.f)
		);
	});

	m_mapTurtleCommands['>'] = std::function<void()>([&]() {
		m_Turtle.orientation = glm::rotate(
			m_Turtle.orientation,
			glm::radians(-m_Turtle.turnAngle),
			glm::vec3(0.f, 1.f, 0.f)
		);
	});

	m_mapTurtleCommands['^'] = std::function<void()>([&]() {
		m_Turtle.orientation = glm::rotate(
			m_Turtle.orientation,
			glm::radians(m_Turtle.turnAngle),
			glm::vec3(1.f, 0.f, 0.f)
		);
	});

	m_mapTurtleCommands['v'] = std::function<void()>([&]() {
		m_Turtle.orientation = glm::rotate(
			m_Turtle.orientation,
			glm::radians(-m_Turtle.turnAngle),
			glm::vec3(1.f, 0.f, 0.f)
		);
	});

	m_mapTurtleCommands['['] = std::function<void()>([&]() {
		m_vTurtleStack.push_back(m_Turtle);

		m_vNodeStack.push_back(m_pCurrentNode);
	});

	m_mapTurtleCommands[']'] = std::function<void()>([&]() {
		m_Turtle = m_vTurtleStack.back();
		m_vTurtleStack.pop_back();

		m_pCurrentNode = m_vNodeStack.back();
		m_vNodeStack.pop_back();
	});
}

void LSystem::initGL()
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
	m_Turtle.turnAngle = angle;
}

void LSystem::setSegmentLength(float len)
{
	m_Turtle.stepSize = len;
}

void LSystem::setRefreshNeeded()
{
	m_bNeedsRefresh = true;
}

// returns true if a rule already exists for the symbol
bool LSystem::addRule(char symbol, std::string replacement)
{
	RuleMap::iterator it = m_mapRules.find(symbol);

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

// returns true if a rule already exists for the symbol
bool LSystem::addFinishRule(char symbol, std::string replacement)
{
	RuleMap::iterator it = m_mapFinishRules.find(symbol);

	m_mapFinishRules[symbol].clear();
	m_mapFinishRules[symbol].push_back(std::make_pair(1.f, replacement));

	m_bNeedsRefresh = true;

	if (it != m_mapFinishRules.end())
		return true;
	else
		return false;
}

bool LSystem::addStochasticFinishRules(char symbol, std::vector<std::pair<float, std::string>> replacementRules)
{
	float sum = 0.f;
	float epsilon = 0.00001;
	for (auto const &rule : replacementRules)
	{
		sum += rule.first;
	}
	if (fabs(sum - 1.f) > epsilon)
	{
		std::cerr << "Error: Finishing stochastic replacement rules for symbol '" << symbol << "' do not have probabilities that add up to 1.0 (total = " << sum << ")" << std::endl;
		return false;
	}

	m_mapFinishRules[symbol] = replacementRules;

	return true;
}

void LSystem::update()
{
	reset();

	std::cout << run() << std::endl;

	//generateLines();
	//generateQuads();
	generateMesh(10);

	refreshGL();

	m_bNeedsRefresh = false;
}

std::string LSystem::run()
{
	// just return the result string if no refresh is necessary
	if (!m_bNeedsRefresh)
		return m_strResult;

	// set the axiom
	std::string startStr = std::string(1, m_chStartSymbol);
	std::string endStr;

	// iterate parallel rewriting the specified number of times
	for (unsigned int i = 0u; i < m_nIters; ++i)
	{
		endStr = iterate(startStr);
		startStr = endStr;
	}

	// apply rules to finish the rewriting
	m_strResult = finish(endStr);

	build();

	return m_strResult;
}

std::string LSystem::iterate(std::string oldstr)
{
	std::string newstr;

	for (auto const &c : oldstr)
	{
		std::string result;
		applyRules(c, m_mapRules, &result);
		newstr += result;
	}

	return newstr;
}

std::string LSystem::finish(std::string oldstr)
{
	std::string newstr;

	for (auto const &c : oldstr)
	{
		std::string result;
		if (m_mapFinishRules.count(c) != 0)
		{
			applyRules(c, m_mapFinishRules, &result);
			newstr += result;
		}
		else if (m_mapTurtleCommands.count(c) != 0)
		{
			newstr += std::string(1, c);
		}
	}

	return newstr;
}

bool LSystem::applyRules(char symbol, RuleMap rules, std::string *out)
{
	// Check if replacement rule exists for symbol
	std::map<char, std::vector<std::pair<float, std::string>>>::iterator it = rules.find(symbol);

	// Rule Exists
	if (it != rules.end())
	{
		float val = m_UniformDist(m_mtEngine);
		float cumsum = 0.f;

		for (auto const &rule : it->second)
		{
			cumsum += rule.first;
			if (val <= cumsum)
			{
				*out = rule.second;
				return true;
			}
		}

		std::cerr << "Error: Failed to apply stochastic rules for symbol '" << symbol << "'!" << std::endl;

		*out = std::string(1, symbol);
		return false;
	}
	else // Rule does not exist
	{
		*out = std::string(1, symbol);
		return false;
	}
}

void LSystem::build()
{
	m_pCurrentNode = new Scaffold::Node(m_Turtle.position, m_Turtle.orientation, m_Turtle.size);
	m_Scaffold.vNodes.push_back(m_pCurrentNode);

	for (auto const& c : m_strResult)
	{
		if (m_mapTurtleCommands.count(c))
			m_mapTurtleCommands[c]();
		else
			std::cerr << "Error: Symbol '" << c << "' not found in turtle commands." << std::endl;
	}
}

GLuint LSystem::getVAO()
{
	if (m_bNeedsRefresh)
		update();

	return m_glVAO;
}

GLushort LSystem::getIndexCount()
{
	if (m_bNeedsRefresh)
		update();

	return m_vusInds.size();
}

void LSystem::reset()
{
	m_vvec3Points.clear();
	m_vvec4Colors.clear();
	m_vusInds.clear();

	m_Turtle = TurtleState();
	m_vTurtleStack.clear();
	
	for (auto &n : m_Scaffold.vNodes)
		delete n;
	m_Scaffold.vNodes.clear();

	for (auto &s : m_Scaffold.vSegments)
		delete s;
	m_Scaffold.vSegments.clear();

	resetDataBounds();
}

void LSystem::refreshGL()
{
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

		glm::vec3 segVector = seg->terminus->vec3Pos - seg->origin->vec3Pos;
		float segLen = glm::length(segVector);

		glm::vec3 localRight = glm::normalize(tRot[0]) * (segLen / 10.f);
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

void LSystem::generateMesh(uint16_t numSubsegments)
{
	for (auto const& seg : m_Scaffold.vSegments)
	{
		glm::vec3 originHeading(glm::rotate(seg->origin->qRot, glm::vec3(0.f, 1.f, 0.f)));
		glm::vec3 terminusHeading(glm::rotate(seg->terminus->qRot, glm::vec3(0.f, 1.f, 0.f)));
		
		glm::vec3 segVector = seg->terminus->vec3Pos - seg->origin->vec3Pos;
		float segLen = glm::length(segVector);

		float beginSize = seg->origin->vec3Scale.x;
		float endSize = seg->terminus->vec3Scale.x;

		float stepSize = 1.f / (float)(numSubsegments);

		for (uint16_t i = 0u; i < numSubsegments; ++i)
		{
			float mixRatioStart = (float)i * stepSize;
			float mixRatioEnd = (float)(i + 1) * stepSize;
			float mixRatioHalf = (mixRatioStart + mixRatioEnd) / 2.f;

			glm::quat interpQuatStart = glm::slerp(seg->origin->qRot, seg->terminus->qRot, mixRatioStart);
			glm::quat interpQuatEnd = glm::slerp(seg->origin->qRot, seg->terminus->qRot, mixRatioEnd);

			glm::mat3 rotStart = glm::mat3_cast(interpQuatStart);
			glm::mat3 rotEnd = glm::mat3_cast(interpQuatEnd);

			glm::vec3 localRightStart = glm::normalize(rotStart[0]) * (segLen / 10.f);
			glm::vec3 localLeftStart = -localRightStart;

			glm::vec3 localRightEnd = glm::normalize(rotEnd[0]) * (segLen / 10.f);
			glm::vec3 localLeftEnd = -localRightEnd;

			glm::vec3 startPos = seg->origin->vec3Pos + segVector * mixRatioStart;
			glm::vec3 endPos = seg->origin->vec3Pos + segVector * mixRatioEnd;

			glm::vec3 startColor = glm::mix(glm::vec3(0.f), glm::vec3(1.f), mixRatioStart);
			glm::vec3 endColor = glm::mix(glm::vec3(0.f), glm::vec3(1.f), mixRatioEnd);

			if (i == 0)
			{
				m_vvec3Points.push_back(startPos + localLeftStart);
				m_vvec3Points.push_back(startPos);
				m_vvec3Points.push_back(startPos + localRightStart);
				//m_vvec4Colors.push_back(glm::vec4((rotStart[1] + 1.f) * 0.5f, 1.f));
				//m_vvec4Colors.push_back(glm::vec4((rotStart[1] + 1.f) * 0.5f, 1.f));
				//m_vvec4Colors.push_back(glm::vec4((rotStart[1] + 1.f) * 0.5f, 1.f));
				m_vvec4Colors.push_back(glm::vec4(startColor, 1.f));
				m_vvec4Colors.push_back(glm::vec4(startColor, 1.f));
				m_vvec4Colors.push_back(glm::vec4(startColor, 1.f));
			}

			m_vvec3Points.push_back(endPos + localLeftEnd);
			m_vvec3Points.push_back(endPos);
			m_vvec3Points.push_back(endPos + localRightEnd);
			//m_vvec4Colors.push_back(glm::vec4((rotEnd[1] + 1.f) * 0.5f, 1.f));
			//m_vvec4Colors.push_back(glm::vec4((rotEnd[1] + 1.f) * 0.5f, 1.f));
			//m_vvec4Colors.push_back(glm::vec4((rotEnd[1] + 1.f) * 0.5f, 1.f));
			m_vvec4Colors.push_back(glm::vec4(endColor, 1.f));
			m_vvec4Colors.push_back(glm::vec4(endColor, 1.f));
			m_vvec4Colors.push_back(glm::vec4(endColor, 1.f));

			m_vusInds.push_back(m_vvec3Points.size() - 6u);
			m_vusInds.push_back(m_vvec3Points.size() - 5u);
			m_vusInds.push_back(m_vvec3Points.size() - 3u);

			m_vusInds.push_back(m_vvec3Points.size() - 5u);
			m_vusInds.push_back(m_vvec3Points.size() - 2u);
			m_vusInds.push_back(m_vvec3Points.size() - 3u);

			m_vusInds.push_back(m_vvec3Points.size() - 5u);
			m_vusInds.push_back(m_vvec3Points.size() - 4u);
			m_vusInds.push_back(m_vvec3Points.size() - 1u);

			m_vusInds.push_back(m_vvec3Points.size() - 5u);
			m_vusInds.push_back(m_vvec3Points.size() - 1u);
			m_vusInds.push_back(m_vvec3Points.size() - 2u);
		}
	}
}
