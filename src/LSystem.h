#pragma once

#include <string>
#include <vector>
#include <map>

#include <GL/glew.h>

class LSystem
{
public:	
	LSystem();
	~LSystem();

	void setStart(char symbol);
	void setIterations(unsigned int iters);
	bool addRule(char symbol, std::string replacement);

	std::string run();

	void draw(float angle, float segmentLength);

private:
	unsigned int m_nIters;
	std::map<char, std::string> m_mapRules;
	char m_chStartSymbol;

	bool m_bNeedsRefresh;

	std::string m_strResult;

	GLuint m_glVAO, m_glVBO, m_glEBO;
	std::vector<GLushort> m_usInds;
	GLuint m_glTexture;

	std::string process(std::string oldstr);
	std::string applyRules(char symbol);
};

