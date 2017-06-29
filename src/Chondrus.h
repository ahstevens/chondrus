#pragma once
#include <glSkel/Object.h>
#include <glSkel/BroadcastSystem.h>
#include <glSkel/Mesh.h>
#include <glSkel/Shader.h>

#include <glSkel\BulletDebugDrawer.h>

class Chondrus : public Object, public BroadcastSystem::Listener
{
public:
	Chondrus(float length, float width, float thickness, glm::vec3 position, glm::mat3 orientation);
	~Chondrus();

	float getLength();
	float getWidth();
	
	void update();

	void receiveEvent(Object* obj, const int event, void* data);

	void Draw();

private:
	Mesh* mesh;
	std::vector<GLuint> indices;

	GLfloat m_fLength, m_fWidth;
	GLuint nVertsTall;

	void buildModel();

	float calculateEnvelope(float currentRatio, float begin, float max1, float max2, float end);
	float getRandRatio();
	
	std::vector<Texture> loadTextures();

	bool saveAsObj(std::string name = "untitled_model");	

	BulletDebugDrawer *debugDrawer;
};

