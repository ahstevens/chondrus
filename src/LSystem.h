#pragma once

#include <string>
#include <vector>
#include <map>
#include <random>
#include <functional>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <glSkel/Object.h>
#include <glSkel/Dataset.h>

class LSystem : public Object, public Dataset
{
	typedef std::map<char, std::vector<std::pair<float, std::string>>> RuleMap;
	typedef std::map<char, std::function<void()>> CommandMap;

public:	
	LSystem();
	~LSystem();

	void setStart(char symbol);
	void setIterations(unsigned int iters);
	void setAngle(float angle);
	void setSegmentLength(float len);
	void setSize(glm::vec3 size);
	void setRefreshNeeded();
	bool addRule(char symbol, std::string replacement);
	bool addStochasticRules(char symbol, std::vector<std::pair<float, std::string>> replacementRules);
	bool addFinishRule(char symbol, std::string replacement);
	bool addStochasticFinishRules(char symbol, std::vector<std::pair<float, std::string>> replacementRules);

	void update();

	std::string run();

	GLuint getVAO();
	GLushort getIndexCount();

private:
	void makeTurtleCommands();
	void initGL();

	std::string iterate(std::string oldstr);
	std::string finish(std::string oldstr);
	bool applyRules(char symbol, RuleMap rules, std::string *out);
	void build();

	void reset();

	void refreshGL();

	void generateLines();
	void generateQuads();
	void generateMesh(uint16_t numSubsegments);

private:
	struct TurtleState {
		glm::vec3 position;
		glm::quat orientation;
		glm::vec3 size; // x = width; y = forwardStepLength; z = thickness
		float turnAngle; // degrees

		TurtleState() : position(glm::vec3(0.f)), orientation(glm::quat()), size(glm::vec3(1.f)), turnAngle(25.f) {}
		TurtleState(glm::vec3 pos, glm::quat orientation, glm::vec3 size, float stepSize, float turnAngle) : position(pos), orientation(orientation), size(size), turnAngle(turnAngle) {}
	};

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
			glm::vec3 vec3Scale;

			Node(glm::vec3 pos, glm::quat rot, glm::vec3 scale) : vec3Pos(pos), qRot(rot), vec3Scale(scale) {}
		};

		std::vector<Node*> vNodes;
		std::vector<Segment*> vSegments;
	};

private:
	unsigned int m_nIters;
	RuleMap m_mapRules; // symbols map to vectors of probability/replacement string pairs
	RuleMap m_mapFinishRules; // finishing symbols map to vectors of probability/replacement string pairs
	char m_chStartSymbol;

	CommandMap m_mapTurtleCommands;

	TurtleState m_Turtle, m_TurtleOriginalState;

	std::vector<TurtleState> m_vTurtleStack;

	Scaffold::Node* m_pCurrentNode;
	std::vector<Scaffold::Node*> m_vNodeStack;

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

