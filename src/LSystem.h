#pragma once

#include <string>
#include <vector>
#include <map>
#include <random>

#include <GL/glew.h>

#include <glm/glm.hpp>

class LSystem
{
public:	
	LSystem();
	~LSystem();

	void setStart(char symbol);
	void setIterations(unsigned int iters);
	void setAngle(float angle);
	void setSegmentLength(float len);
	void setRefreshNeeded();
	bool addRule(char symbol, std::string replacement);
	bool addStochasticRules(char symbol, std::vector<std::pair<float, std::string>> replacementRules);

	std::string run();

	void draw();

private:
	float m_fAngle, m_fSegLen;
	unsigned int m_nIters;
	std::map<char, std::vector<std::pair<float, std::string>>> m_mapRules; // symbols map to vectors of replacement/probability pairs
	char m_chStartSymbol;

	bool m_bNeedsRefresh;

	std::string m_strResult;

	GLuint m_glVAO, m_glVBO, m_glEBO;
	std::vector<glm::vec3> m_vvec3Points;
	std::vector<glm::vec4> m_vvec4Colors;
	std::vector<GLushort> m_vusInds;

	std::uniform_real_distribution<float> m_UniformDist;
	std::mt19937 m_mtEngine; // Mersenne twister MT19937

	std::string process(std::string oldstr);
	std::string applyRules(char symbol);
};

