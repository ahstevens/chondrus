#pragma once

#include <string>
#include <vector>
#include <map>
#include <random>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <glSkel/Object.h>

class LSystem : public Object
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
	std::string process(std::string oldstr);
	std::string applyRules(char symbol);

	void generateLines();
	void generateQuads();
	void generateMesh(uint16_t numSubsegments);

private:
	struct Scaffold {
		struct Node;
		struct Segment {
			Node* origin;
			Node* terminus;

			Segment(Node* origin, Node* terminus)
				: origin(origin)
				, terminus(terminus)
			{
				calcLenSq();
			}
			void calcLenSq() { fLengthSq = glm::length2(terminus->vec3Pos - origin->vec3Pos); }

		private:
			float fLengthSq;
		};

		struct Node {
			std::vector<Segment*> vSegments;
			Node* parentNode;
			std::vector<Node*> vChildren;
			glm::vec3 vec3Pos;
			glm::quat qRot;
			glm::vec3 vec3Scale; // x = width; y = length; z = thickness

			Node(glm::vec3 pos, glm::quat rot, glm::vec3 scale) : vec3Pos(pos), qRot(rot), vec3Scale(scale) {}
		};

		std::vector<Node*> vNodes;
		std::vector<Segment*> vSegments;
	};

private:
	float m_fAngle, m_fSegLen;
	unsigned int m_nIters;
	std::map<char, std::vector<std::pair<float, std::string>>> m_mapRules; // symbols map to vectors of replacement/probability pairs
	char m_chStartSymbol;

	bool m_bNeedsRefresh;

	std::string m_strResult;

	Scaffold m_Scaffold;

	GLuint m_glVAO, m_glVBO, m_glEBO;
	std::vector<glm::vec3> m_vvec3Points;
	std::vector<glm::vec4> m_vvec4Colors;
	std::vector<GLushort> m_vusInds;

	std::uniform_real_distribution<float> m_UniformDist;
	std::mt19937 m_mtEngine; // Mersenne twister MT19937
};

