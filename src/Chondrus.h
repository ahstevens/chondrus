#pragma once
#include <glSkel/Object.h>
#include <glSkel/BroadcastSystem.h>
#include <glSkel/Mesh.h>

#include <glSkel/BulletDebugDrawer.h>

struct Segment {
	glm::vec3 begin;
	glm::vec3 end;
	float len;
	float beginWidth;
	float endWidth;
	glm::vec3 beginNormal;
	glm::vec3 endNormal;
	Segment *parent;
	std::vector<Segment*> children;

	Segment()
		: parent()
	{}

	std::vector<Segment*> getSiblings()
	{
		std::vector<Segment*> sibs;

		if (parent != NULL)
			for (auto const &child : parent->children)
				if (child != this)
					sibs.push_back(child);

		return sibs;
	}
};

class Chondrus : public Object, public BroadcastSystem::Listener
{
public:

public:
	Chondrus(float length, float width, float thickness, glm::vec3 position, glm::mat3 orientation);
	~Chondrus();

	float getLength();
	float getWidth();
	
	void update();

	void receiveEvent(Object* obj, const int event, void* data);

	void Draw();

private:
	Segment* m_pRoot;

	Mesh* m_pMesh;
	GLuint m_glDiffTex, m_glSpecTex;

	GLfloat m_fLength, m_fWidth;
	GLuint m_nVertsTall;

	bool m_bWireframe;

	void buildModel();

	float calculateEnvelope(float currentRatio, float begin, float max1, float max2, float end);
	float getRandRatio();
	
	void loadTextures();

	void drawBranches(Segment* root);

	bool saveAsObj(std::string name = "untitled_model");	

	BulletDebugDrawer* m_pDebugDrawer;
};

